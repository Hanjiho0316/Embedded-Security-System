#include "device_driver.h"
#include <stdio.h>
#include <string.h>

// --- 설정값 ---
#define LIGHT_THRESHOLD 2500  // 이 값 "이하"일 때만 밝다고 판단 (0~4095)
#define PASS_LEN 4

static void Sys_Init(int baud) 
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
}


void Motor_Init(void) {
    // 1. GPIOA 클럭 활성화
    Macro_Set_Bit(RCC->AHB1ENR, 0); 

    // 2. PA4, PA5를 Output(01)으로 설정 (방향 제어)
    // 비트 8-9 (PA4), 비트 10-11 (PA5)
    GPIOA->MODER &= ~(0xF << 8); // 초기화
    GPIOA->MODER |= (0x5 << 8);  // 01 , 01 : GPIO output mode 설정

    // 3. PA1을 AF(10) 모드로 설정 (PWM - EN1,2) PA1 : A1 핀
    GPIOA->MODER &= ~(0x3 << 2); // Moder 초기화
    GPIOA->MODER |= (0x2 << 2);   // 01 : GPIO output mode 설정
    GPIOA->AFR[0] &= ~(0xF << 4); // AF 초기화
    GPIOA->AFR[0] |= (0x1 << 4); // AF1 (TIM2)

    // AFRL (또는 AFR[0]): 0번 핀 ~ 7번 핀 설정 전용
    // AFRH (또는 AFR[1]): 8번 핀 ~ 15번 핀 설정 전용
    // 이중에서 AFR[0] 이 PA1 이 소속되어있고 이중 01 를 4번째 비트에 삽입


    // 4. TIM2 PWM 설정 (1kHz)
    Macro_Set_Bit(RCC->APB1ENR, 0);
    TIM2->PSC = 84 - 1;
    TIM2->ARR = 1000 - 1;

    // F clk = 84MHz가 들어오게 된다
    // F out = Fclk / [(PSC + 1) * (ARR + 1)]
    // 1KHz가 목표였으므로 PSC = 84 -1 , ARR = 1000 -1

    TIM2->CCMR1 |= (0x6 << 12); // PWM Mode 1 
    
    // AF1 로 설정하게 되면서 채널이 2번이 됨에따라 
    // 12번째 비트를 사용 110 : PWM mode 로 설정

    TIM2->CCER |= (1 << 4);     // CC2E 활성화
    // 2번째 채널이므로 CCER 도 2E 활성화

    TIM2->CR1 |= (1 << 0);      // Counter ON
    // 카운터 시작
}


void Motor_Control(int dir, int speed) {
    if(dir == 1) { // 정회전
        GPIOA->ODR |= (1 << 4);  // PA4 High
        GPIOA->ODR &= ~(1 << 5); // PA5 Low
    } else if(dir == 2) { // 역회전
        GPIOA->ODR &= ~(1 << 4); // PA4 Low
        GPIOA->ODR |= (1 << 5);  // PA5 High
    } else { // 정지
        GPIOA->ODR &= ~((1 << 4) | (1 << 5));
    }
    TIM2->CCR2 = speed; // high 구간을 결정하는 모터의 속도를 제어하는 레지스터
    // 앞서 ARR을 1000-1로 설정해서 "0부터 999까지 세라"고 주기를 정함
    // CCR2는 그 1000이라는 숫자 중에서 "얼마만큼 전압을 내보낼 것인가(Duty Cycle)"를 정하는 기준선입니다.
    //speed = 0: 0% 출력 → 모터 정지
    // speed = 500: 50% 출력 → 중간 속도 (1000 중 500만큼만 전기가 흐름)
    // speed = 800: 80% 출력 → 빠른 속도
    // speed = 1000: 100% 출력 → 최대 속도 (계속 전기가 흐름)
}


// --- 지연 함수 ---
void Delay_ms(int ms) {
    volatile int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 3500; j++); 
}

// --- 조도센서(ADC1 채널2 - PA2) 설정 ---
void ADC_Init(void) {
    // 1. GPIOB 및 ADC1 클럭 활성화
    RCC->AHB1ENR |= (1 << 1);          // GPIOB 활성화 (비트 1)
    RCC->APB2ENR |= (1 << 8);          // ADC1 활성화 (비트 8)

    // 2. PB1 핀을 아날로그 모드(11)로 설정
    GPIOB->MODER &= ~(0x3 << (1 * 2)); // PB1 비트 초기화
    GPIOB->MODER |=  (0x3 << (1 * 2)); // 아날로그 모드(11) 설정

    // 3. ADC 전원 ON
    ADC1->CR2 |= (1 << 0);             // ADON (ADC ON)
    
    // 안정적인 측정을 위해 샘플링 타임 설정 (채널 9)
    ADC1->SMPR2 &= ~(0x7 << (9 * 3));  // 채널 9 초기화
    ADC1->SMPR2 |=  (0x3 << (9 * 3));  // 56 cycles 설정
}

uint16_t Get_LDR_Value(void) {
    // 1. 채널 9번 선택 (PB1)
    ADC1->SQR3 = 9;                    
    
    // 2. 변환 시작
    ADC1->CR2 |= (1 << 30);            // SWSTART
    
    // 3. 변환 완료 대기 (EOC 비트 확인)
    while(!(ADC1->SR & (1 << 1)));     
    
    // 4. 결과값 반환
    return (uint16_t)ADC1->DR;
}

// --- 4x4 키패드(PC0~PC7) 설정 ---
void Keypad_Init(void) {
    RCC->AHB1ENR |= (1 << 2);         // GPIOC 활성화
    GPIOC->MODER &= ~(0xFFFF); 
    GPIOC->MODER |= (0x0055);         // PC0-3: Out, PC4-7: In
    GPIOC->PUPDR &= ~(0xFF00);
    GPIOC->PUPDR |= (0x5500);         // PC4-7: Pull-up
}

char Keypad_Scan(void) {
    char keys[4][4] = {
        {'1','2','3','A'}, {'4','5','6','B'},
        {'7','8','9','C'}, {'*','0','#','D'}
    };
    for (int r = 0; r < 4; r++) {
        GPIOC->ODR = 0x0F;            // 모든 행 High
        GPIOC->ODR &= ~(1 << r);      // 현재 행만 Low
        for (volatile int i = 0; i < 50; i++); 
        
        uint32_t cols = (GPIOC->IDR >> 4) & 0x0F;
        if (cols != 0x0F) {           // 버튼 눌림 감지
            while (((GPIOC->IDR >> 4) & 0x0F) != 0x0F); // 뗄 때까지 대기
            for (int c = 0; c < 4; c++) {
                if (!(cols & (1 << c))) return keys[r][c];
            }
        }
    }
    return 0;
}

// --- 메인 로직 ---
void Main(void) {
    Sys_Init(115200);   
    ADC_Init();         
    Motor_Init();       
    Keypad_Init();      
    Uart1_Init(115200);
    char master_password[] = "1234";
    char input_buffer[5] = {0};
    int input_idx = 0;
    int is_unlocked = 0; 
    int speeddef = 750;

    printf("\n[SYSTEM] Security System Active.\n");
    printf("Enter Password: ");

    while(1) {
        // --- [실시간 조도 모니터링] ---
        // 0.1~0.2초마다 현재 조도값을 한 줄에 계속 표시 (\r 사용)
        uint16_t current_light = Get_LDR_Value();
        if (is_unlocked == 0 && input_idx == 0) {
            printf("\r[MONITOR] Current Light: %4d  | Enter PW: ", current_light);
        }

        // 보드 내장 버튼(PC13) - 단순 LED 테스트용
        if (!(GPIOC->IDR & (1 << 13))) GPIOA->ODR |= (1 << 5);
        else GPIOA->ODR &= ~(1 << 5);

        // --- CASE 1: 잠금 상태 (인증 단계) ---
        if (is_unlocked == 0) {
            char key = Keypad_Scan();
            if (key) {
                // 키 입력 시 모니터링 라인 줄바꿈 후 숫자 표시
                if (input_idx == 0) printf("\n"); 
                printf("[%c] ", key); 

                if (key >= '0' && key <= '9') {
                    input_buffer[input_idx++] = key;

                    if (input_idx == 4) {
                        printf("\n[CHECK] Input: %s, Light: %d\n", input_buffer, current_light);

                        if (strcmp(input_buffer, master_password) == 0 && current_light <= LIGHT_THRESHOLD) {
                            printf("[SUCCESS] Welcome! UART Control Enabled.\n");
                            printf("f: Fwd, r: Rev, s: Stop&Lock, 1-9: Speed\n");
                            is_unlocked = 1;
                        } else {
                            printf("[FAILED] Wrong PW or Too Dark (%d/%d)\n", current_light, LIGHT_THRESHOLD);
                        }
                        input_idx = 0;
                        memset(input_buffer, 0, sizeof(input_buffer));
                    }
                }
            }
        }

        // --- CASE 2: 가동 상태 (UART 실시간 제어) ---
        else if (is_unlocked == 1) {
            if (USART1->SR & (1 << 5)) { 
                char x = Uart1_Get_Char();
                if (x == 'f') { Motor_Control(1, speeddef); printf("[RUN] Fwd\n"); } 
                else if (x == 'r') { Motor_Control(2, speeddef); printf("[RUN] Rev\n"); } 
                else if (x == 's') {
                    Motor_Control(0, 0);
                    is_unlocked = 0;
                    printf("[STOP] System Re-Locked.\n");
                }
                else if (x >= '1' && x <= '9') {
                    speeddef = 500 + (62 * (x - '1'));
                    printf("[SPEED] %d\n", speeddef);
                }
            }
            if(current_light >= LIGHT_THRESHOLD){
                Motor_Control(0,0);
                printf("[STOP] light is too dark\n");
                is_unlocked = 0;
            }
        }
        // 모니터링 속도와 키패드 반응 속도를 위해 적절히 조절된 딜레이
        for(volatile int i=0; i<100000; i++); 
    }
}