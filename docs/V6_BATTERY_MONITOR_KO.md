# v6 구현 범위와 아직 확정하지 않은 것

## 발견된 사실

v5에도 좌우 ZMK_BATTERY_REPORTING/BT_BAS, 동글 battery fetching 설정은 이미 켜져 있습니다.
따라서 단순히 같은 설정을 다시 y로 바꾸는 것만으로 0% 원인을 해결했다고 할 수 없습니다.
기존 화면은 표준 BAS가 전달한 0을 표시할 수 있지만, 그 패킷에는 센서 측정 성공 여부나
실제 측정 전압이 없습니다. 이 때문에 "0을 받았다"와 "실제 배터리가 비었다"는 다릅니다.
제작자 원본 vbatt는 ADC 6, full-ohms=1000, output-ohms=1000을 사용합니다.
이 회로/비율이 사용자 실물과 맞는지는 확인하지 못했습니다.

## 이번 추가 기능

외부 ZMK 포크나 원본 소스 패치 대신 로컬 Zephyr 모듈 두 소스를 추가합니다.
`battery_telemetry_peripheral.c`는 원래 chosen battery 장치를 원래 드라이버로 읽습니다.
기본 ZMK battery_work와 같은 lowprio 큐를 사용해 동시 ADC 호출을 피합니다.
30초 주기 및 연결 이벤트에서 측정하며, BLE 읽기 콜백에서는 ADC 작업을 하지 않습니다.
정상 BAS 보고를 끄거나 가짜 값으로 덮어쓰지 않습니다.

BLE 패킷은 12바이트: 버전, 실제 손 구분, 측정 결과, 퍼센트, 센서 mV, 샘플 나이,
오류 코드, 측정 순번입니다. 표준 BAS와 별도의 읽기 전용 특성입니다.
패킷에 ADC raw count는 포함하지 않으며, mV는 기존 센서 드라이버가 환산한 값입니다.
서비스를 읽으려면 암호화된 연결이 필요하고, 기존 동글-키보드 연결을 재사용합니다.

`battery_telemetry_central.c`는 기존 split source에 속한 연결만 조회합니다.
컴퓨터와의 BLE 호스트 연결은 제외하고 새 장치 연결을 만들지 않습니다.
일반 키/트랙볼 서비스 탐색이 먼저 실행되도록 처음 8초 뒤 시작합니다.
각 손 약 10초 간격으로 재조회하며 별도 낮은 우선순위 큐를 사용합니다.
진행 중인 요청의 파라미터/연결 참조는 완료 콜백 전 재사용하지 않습니다.
오류/연결 해제/잘못된 패킷을 검출하고, 캐시된 값은 나이를 누적합니다.

화면은 배터리 행 내용만 바꿉니다. D 행 및 위젯 좌표/폰트/배경/맥 기호는 유지합니다.
새 패킷에 손 정보가 있어 연결 순서에 관계없이 L/R을 구분할 수 있습니다.
정상 0%도 숨기지 않고, 원인 확인에 필요한 측정 전압과 교대로 표시합니다.
2.5V 미만 / 4.5V 초과는 확인이 필요한 전압으로 표시합니다. 이는 UI의 진단 경고이며
기존 배터리 퍼센트 곡선이나 회로의 보정을 의미하지 않습니다.

## 해결을 보장하지 않는 부분

센서 핀 연결, 분압회로, 기판 리비전, ADC 실제 입력, 배터리 유무/건강은 소스만으로
확정할 수 없습니다. 잘못된 mV를 소프트웨어가 읽으면 텔레메트리도 그 mV를 보여줍니다.
이 경우 표시 기능 자체와 별개로 제작자의 회로/설정 확인이 필요합니다.
이 v6는 실제 기판에서 정확한 퍼센트가 뜨는 것을 검증한 릴리스가 아닙니다.

## 참조한 원본

- https://zmk.dev/docs/config/battery
- https://raw.githubusercontent.com/22sh22/modu-c-firmware/bee0bb4b812f63f279eb67e928accc89600b5904/modu-module/boards/shields/modu/modu.dtsi
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/battery.c
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/module/drivers/sensor/battery/battery_common.c
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/module/drivers/sensor/battery/battery_voltage_divider.c
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/split/bluetooth/central.c
- https://raw.githubusercontent.com/zmkfirmware/zephyr/v4.1.0%2Bzmk-fixes/include/zephyr/bluetooth/gatt.h
