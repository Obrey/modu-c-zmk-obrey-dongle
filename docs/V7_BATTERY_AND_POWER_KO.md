# v7 — 배터리 3.32/3.33V와 연결 대기 개선

## 확인된 것과 확인되지 않은 것

사용자가 약 20분 충전 후 왼쪽 3.32, 오른쪽 3.33, 양쪽 0%를 관찰했습니다.
펌웨어는 원본 센서 드라이버가 반환한 전압/퍼센트를 표시합니다. ADC 측정이 성공했다는 사실이 배터리 단자 전압을 정확히 읽었다는 뜻은 아닙니다.

고정 ZMK `641514a97db345f499dd50b0360e594270f008fe`의 `battery_common.c`는 3450mV 이하를 0%, 4200mV 이상을 100%로 환산합니다.
일반적인 3.7V 표기 단셀 리튬폴리머 배터리는 완충 전압이 약 4.2V입니다. 사용자 배터리의 라벨/회로가 직접 확인된 것은 아닙니다.
따라서 실제 배터리 전압이 3.32/3.33V라면 충분한 충전 상태라고 보기는 어렵습니다. 하지만 실제 회로를 확인하지 않고 충전 불량이나 방전으로 단정하지 않습니다.

원본 MODU-C 설정은 `zmk,battery-voltage-divider`, AIN6, `output-ohms=1000`, `full-ohms=1000`입니다.
실물의 측정 지점과 분압 회로가 이 설정과 일치하는지는 소스만으로 확인되지 않았습니다.
충전 완료 후에도 3.3V 부근에 거의 고정된다면 충전 시간을 무조건 늘리거나 3.3V를 100%로 보정하지 말고 측정 지점/분압비 및 실제 충전 상태를 확인해야 합니다.
이번 패키지는 이 센서 핀, 분압비, 기본 잔량 곡선, 충전 제어를 바꾸지 않습니다.

## 사용자가 할 수 있는 확인

제품의 정상 충전 방법과 완료 기준에 따라 충전하고, 충전 USB를 뺀 뒤 좌우 키를 눌러 대기에서 복귀시켜 표시를 확인합니다.
사용 중 추가 측정은 1분, 동글 조회는 30초 간격입니다. 충전 중과 케이블 제거 후의 값은 같은 조건의 값이 아닐 수 있습니다.
제품 배터리 용량/충전 전류가 확인되지 않아 '20분이면 충분', '정확히 몇 시간 충전'이라는 기준을 제공하지 않습니다.
지나친 열, 부풀음, 냄새 등 이상이 있으면 사용과 충전을 멈추고 판매자에게 문의하세요. 배터리 단자에 직접 전원을 넣거나 회로를 우회하지 않습니다.

판매자/개발자 확인 문구:
> 기본 펌웨어의 배터리 센서가 AIN6, 저항비 1000:1000인데 이 핀이 실제 배터리 전압을 측정하는 것이 맞나요? 충전 후 양쪽이 약 3.32/3.33V이고 원래 ZMK 곡선에서 0%입니다. 보드 버전에 맞는 측정 핀/분압비와 충전 완료 표시를 확인 부탁드립니다.

## 연결 대기를 택한 근거

원본 left.conf는 키 스캐너 폴링을 켜며 양쪽 PMW3610 폴링 간격은 15ms입니다.
원본 PMW3610 Kconfig는 이 기판에서 GPIO SENSE/PORT 인터럽트가 동작하지 않아 폴링을 사용하는 경우가 있고 깊은 절전에 비용/제약이 있음을 명시합니다.
원본 트랙볼 드라이버에는 activity 상태에 따른 성능 조정도 이미 있습니다. 그 동작과 모션/키 감지 경로를 유지합니다.
공식 ZMK 깊은 절전은 전원 꺼짐 상태로 들어가 Bluetooth가 끊기며 별도의 작동하는 wakeup source가 필요합니다.
따라서 이번에는 `CONFIG_ZMK_SLEEP=n`을 유지하고 30초 연결 대기에서 불필요한 LED/진단 작업을 줄입니다.
'키로 깨어나는 완전절전이 불가능'하다고 확정하는 것이 아니라, 이 하드웨어에서 검증 없이 기본값으로 켜지 않는 결정입니다.
깊은 절전 수준의 소비 전류/사용 시간을 보장하지 않습니다. 장시간 미사용 시 제품의 전원 스위치를 이용할 수 있습니다.

## 구체적인 변경

- 30초 idle 후 LED off. 연결 여부는 기존의 실제 keyboard→dongle BLE 상태로 판정.
- 밝기 설정 12→6%. 연결/idle 상태에서는 반복 작업 중단. active 검색 중에만 점멸 갱신.
- 추가 배터리 ADC 샘플: active 60초, idle 300초, 연결 해제 시 중단, idle→active 때 즉시 샘플 요청.
- BLE 읽기는 캐시만 응답. idle 동안 중앙의 읽기 요청이 ADC 측정을 강제로 되살리지 않음.
- 동글 상세 조회: 성공 active 30초, 성공 idle 60초, 미측정 초기 2초 재시도, 오류 10초 재시도.
- 값이 수신되지 않거나 오래되었을 때 0%를 만들어 표시하지 않음. 수신 경로 자체가 180초 이상 멈춘 경우 idle 플래그가 있어도 오래된 값으로 처리.
- 14바이트 v2 데이터에 idle 플래그 추가. 새 동글은 기존 12바이트 v1 데이터도 해석. 14바이트는 기본 ATT MTU의 단일 읽기 응답 범위 안에 유지.
- 배터리 %와 전압을 모든 정상 잔량 구간에서 교대. `~`는 connected standby의 캐시라는 뜻. error/unknown/off/old 구분 및 7글자 폭 유지.
- 명시적인 절전 설정 파일 `config/dongle/peripheral-power.conf`를 양쪽 빌드에 포함.

## 바꾸지 않은 것
키맵과 부트키, GPIO, 배터리 ADC 분압, 트랙볼 원본 드라이버와 폴링 주기, 센서 축/속도, USB/Bluetooth 역할/페어링, 화면 모듈/맥 수정키/테마/배치.
0%만을 근거로 강제 종료하지 않습니다. 실제 충전 여부를 추측해 충전 아이콘을 만들지 않습니다.

## 검증
33개 Python unittest(내부 C 모의 테스트 포함), 구성 검사, 모의 HEX/UF2 패키징 검사 통과.
LED C 코드: 좌/우 각각 컴파일 후 active 연결/idle off/복귀/미연결 점멸/중복 PWM 쓰기 방지를 모의 확인.
센서 C 코드: 3.32/3.33V에서 원래 0% 유지, 샘플 실패와 0 구분, cached idle 읽기의 ADC 재요청 방지, 복귀/연결 해제/재시도 확인.
실제 ARM 툴체인/종속 소스를 확보하지 못하여 펌웨어 compile/link와 실물 전류/배터리 정확도/화면 테스트는 수행하지 못했습니다.
인터넷 패키지 다운로드도 이 실행 환경에서 DNS 해석 실패로 불가능했습니다. 모의 시험 성공을 실물 성공으로 해석하지 않습니다.

## 확인한 공식/원본 소스

- ZMK 배터리 환산: https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/module/drivers/sensor/battery/battery_common.c
- MODU-C 배터리/입력 설정: https://raw.githubusercontent.com/22sh22/modu-c-firmware/bee0bb4b812f63f279eb67e928accc89600b5904/modu-module/boards/shields/modu/modu.dtsi
- 원본 좌측 설정: https://raw.githubusercontent.com/22sh22/modu-c-firmware/bee0bb4b812f63f279eb67e928accc89600b5904/modu-module/boards/shields/modu/modu_left.conf
- PMW3610 polling 주의사항: https://raw.githubusercontent.com/22sh22/modu-c-firmware/bee0bb4b812f63f279eb67e928accc89600b5904/zmk-pmw3610-driver/Kconfig
- PMW3610 activity 처리: https://raw.githubusercontent.com/22sh22/modu-c-firmware/bee0bb4b812f63f279eb67e928accc89600b5904/zmk-pmw3610-driver/src/pmw3610.c
- ZMK low power: https://zmk.dev/docs/features/low-power-states
- ZMK power options: https://zmk.dev/docs/config/power
- ZMK built-in battery idle 처리: https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/battery.c
- 일반 LiPo 전압 설명: https://learn.adafruit.com/li-ion-and-lipoly-batteries/voltages
