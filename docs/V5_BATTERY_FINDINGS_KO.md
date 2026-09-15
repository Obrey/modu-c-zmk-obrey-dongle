# 좌우 0%: 확인된 범위와 남은 확인

## 화면 코드에서 확인한 것

현재 local-modules/modu-dongle/src/dongle_status.c는 수신 여부를 별도 valid 플래그로 보관합니다.
초깃값의 0을 그대로 화면에 찍는 방식이 아닙니다. 미수신/미식별은 --%, 연결 해제는 OFF입니다.
수신된 이벤트의 state_of_charge가 0이면 0%를 표시합니다. 0과 미수신을 구분하는 모의 테스트도 통과합니다.
연결 순서를 L/R로 단정하지 않고 실제 키 위치/저장된 BLE 주소로 손을 구분합니다.

## 펌웨어 설정에서 확인한 것

좌우 빌드에 ZMK_BATTERY_REPORTING=y와 BT_BAS=y가 이미 명시돼 있고,
동글에는 ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING=y가 이미 켜져 있습니다.
따라서 지금 소스에서 이 설정의 누락을 원인으로 설명할 근거는 없습니다.

키보드의 배터리 센서 배선/전압 환산 설정은 제작자 원본 핀을 그대로 따릅니다.
원본 modu.dtsi는 adc 6, output-ohms=1000, full-ohms=1000을 사용합니다.
이 값이 사용자 실물 기판의 회로와 맞는지 실측한 것은 아닙니다.
회로도나 전압을 확인하지 않고 full-ohms를 두 배로 바꾸는 등의 임의 보정은 하지 않았습니다.

## 왜 수신값 0만으로 실제 방전이라고 판단할 수 없는가

동글은 배터리 퍼센트 이벤트만 받고, 각 키보드의 ADC 원시값이나 밀리볼트, 측정 성공 여부를 받지 않습니다.
키 입력 성공은 무선 입력 경로가 작동한다는 뜻이지 배터리 센서의 정확성을 증명하지 않습니다.
따라서 현재 정보만으로 센서/환산/업데이트 문제와 실제 낮은 잔량을 구분하지 못합니다.

## 다음 진단에 유용한 정보

현재 화면 사진은 --%, OFF, 0% 및 실제 배치를 구분하는 데 도움이 됩니다.
이미 충전선을 빼고 양쪽을 무선으로 쓰는 중이라면 그 사용 상태를 알려주면 됩니다.
아직 연결된 상태라면 연결/페어링 초기화를 반복할 필요는 없습니다.
이 단계에서는 분해, 배터리 교체, ADC 배선 수정, 임의 잔량 보정 등을 권하지 않습니다.

보다 확실한 원인 규명에는 실물에서 좌우 센서 샘플의 ADC 원시값/밀리볼트/보고 퍼센트와
동글이 수신한 퍼센트를 비교해야 합니다. 이 ZIP은 해당 실물 진단을 수행했다고 주장하지 않습니다.

## 직접 확인한 소스

- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/battery.c
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/split/bluetooth/central.c
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/module/drivers/sensor/battery/battery_voltage_divider.c
- https://raw.githubusercontent.com/22sh22/modu-c-firmware/bee0bb4b812f63f279eb67e928accc89600b5904/modu-module/boards/shields/modu/modu.dtsi
