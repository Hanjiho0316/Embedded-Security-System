# 🛡️ STM32 Multi-Layer Security Control System
> **Keypad 비밀번호와 LDR 광학 인증을 결합한 2중 보안 및 UART 모터 제어 시스템**

본 프로젝트는 **STM32F411RE** 보드를 사용하여, 물리적인 보안(비밀번호)과 환경적인 보안(조도 센서)이 모두 충족되었을 때만 시스템이 잠금 해제되는 로직을 구현했습니다. 인증 후에는 UART를 통해 실시간으로 장치를 제어할 수 있습니다.

---

## 📺 Demo (동작 예시)




https://github.com/user-attachments/assets/0d01a46a-372a-4b9e-bb80-a1413f6abdf1



---

## 🕹️ UART 실시간 제어 가이드 (Control Guide)
시스템 인증(`ID: 1234` + `Light > 2500`) 성공 후, 시리얼 터미널을 통해 다음 명령어로 제어 가능합니다.

| 명령어 | 기능 (Command) | 상세 설명 |
| :---: | :--- | :--- |
| **`f`** | **Forward** | 모터 정회전 시작 |
| **`r`** | **Reverse** | 모터 역회전 시작 |
| **`s`** | **Stop & Lock** | **모터 정지 및 시스템 즉시 재잠금 (보안 모드 복귀)** |
| **`1 ~ 9`** | **Speed Set** | 모터 속도 단계 설정 (1: 저속 ~ 9: 고속) |

---

## 🛠️ 핵심 기능 (Main Features)
- **Multi-Factor Authentication**: 
  - 1단계: 4x4 Keypad를 통한 4자리 비밀번호 인증
  - 2단계: LDR(조도센서)을 이용한 특정 광량 감지 인증
- **Real-time Monitoring**: UART(115200bps)를 통해 현재 조도값 및 시스템 상태 출력
- **Motor Control**: 인증 성공 시에만 PWM 기반 DC 모터 제어 가능
- **Safe Mode**: 's' 명령 시 즉시 시스템 재잠금 및 모든 제어 권한 회수

---

## 🔌 하드웨어 구성 (Pin Mapping)
| 기능 | 핀 (Pin) | 비고 |
| :--- | :--- | :--- |
| **LDR (Light)** | **PB1** | ADC1_IN9 (Voltage Divider 회로) |
| **Keypad** | **PC0-PC2, PC4-PC8** | 4x4 Matrix 스캔 방식 |
| **DC Motor** | **PA1, PA5, PA6** | PWM 및 방향 제어 |
| **Debug Port** | **PA2, PA3** | UART2 (Serial 통신) |

---

## 🛠️ 트러블슈팅 (Troubleshooting)

### 1. UART 통신핀(PA2)과 ADC 채널 충돌
- **문제**: ADC 활성화 시 UART 시리얼 터미널의 데이터가 깨지거나 전송이 중단됨.
- **원인**: `PA2` 핀이 **UART2_TX**와 ADC 입력 기능을 동시에 가지고 있어 하드웨어 설정 충돌 발생.
- **해결**: ADC 입력 핀을 **`PB1` (ADC1_IN9)** 채널로 변경하여 독립적인 데이터 경로 확보.

### 2. 조도센서 ADC 값 고정 문제
- **문제**: 주변 밝기 변화에도 ADC 값이 `4095` 근처에서 고정됨.
- **원인**: LDR을 전원(3.3V)에만 직렬로 연결하여 전압 분배가 이루어지지 않음.
- **해결**: 10kΩ 저항과 LDR을 이용한 **전압 분배(Voltage Divider)** 회로를 구성하고 GND를 연결하여 전압 가변 범위 확보.

---

## 📂 프로젝트 구조 (Project Structure)
```text
.
├── code/                   # 모든 소스 코드 및 헤더 파일
│   ├── main.c              # 메인 로직 및 인증/제어 프로세스
│   ├── device_driver.c     # ADC, Motor, Keypad 드라이버 구현
│   └── device_driver.h     # 레지스터 정의 및 매크로
├── imgs/                   # 데모 영상 및 이미지 파일
├── .gitignore              # 불필요한 빌드 파일 제외
└── README.md               # 프로젝트 설명서


