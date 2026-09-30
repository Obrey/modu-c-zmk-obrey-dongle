# v8 설계와 검증 범위

## 바꾼 것

`battery_range_display.h`에 순수 정수 기반 표시 함수를 추가하고,
`dongle_status.c`에서 선택적으로 호출합니다. 옵션은 OLED 설정에서만 활성화합니다.
측정값 구조체는 const로 전달하며, result/percent/millivolts/flags를 수정하지 않습니다.
유효한 데이터의 표시만 다음으로 환산합니다.

```
relative_index = clamp(round((raw_mV - min_mV) * 100 / (max_mV - min_mV)), 0, 100)
```

기본 표시 구간은 사용자가 관측한 2900~3400mV입니다. 실제 셀의 SOC 곡선이나 분압비를 알아낸 것이 아닙니다.
전압 조절기 출력 등 배터리 용량과 직접 대응하지 않는 값을 읽고 있을 가능성도 남아 있습니다.
따라서 모든 상대 표시에 `*`를 붙이고 원래 전압을 계속 표시합니다.
이 모드가 배터리 측정 오류 자체를 해결한다고 주장하지 않습니다.

## 바꾸지 않은 것

ADC 핀/분압비/구동 타이밍, 원래 배터리 드라이버, v1/v2 BLE 텔레메트리,
BAS 이벤트 값, 페어링, 좌우 소스 식별, 배터리 D 행, OLED 위젯 배치,
키맵/부트키/LED/트랙볼/전력 설정은 유지했습니다.
0으로 보고된 표준 SOC가 업데이트되어 정확해진 것처럼 꾸미지 않습니다.

## 불확실성 처리

- 측정 오류, 대기 중 오래된 값, 비정상 전압은 상대 지수로 덮어쓰지 않습니다.
- 미수신 BAS 초깃값으로 임의의 백분율을 만들지 않습니다.
- 실제 완충이나 방전으로 확인되지 않은 관측 최대/최소를 자동 학습하지 않습니다.
- 샘플을 가공/평활해서 센서 응답을 감추지 않습니다.
- 좌우는 서로 다른 표시 구간을 지정할 수 있으나, 기본값은 둘 다 동일한 관측 구간입니다.
- 글자 수는 사용 중 6개, 대기 중 최대 7개로 기존 폭 56px에 맞춥니다.

## 실행한 테스트

`test_battery_range.py`: 새 산술/표시 함수, 모든 uint16 입력의 단조성/범위,
사용자가 보고한 전압 예시, 역전/동일 구간 컴파일 차단,
원시값 불변, 오류/대기/미수신/연결해제 처리,
좌우 연결 순서와 별개인 프로필 선택, D 행 불변,
옵션 비활성화 시 기존 UI 복귀, 글자 수/버퍼 경계.

기존 validate/selftest/boot-key/telemetry/status/power 테스트도 모두 재실행했습니다.
UI 테스트는 실제 어댑터 C 코드를 모의 Zephyr/BLE/LVGL API와 호스트 컴파일러로 실행합니다.
이는 실제 펌웨어 빌드나 OLED 화소 렌더링, 실제 배터리 용량 교정 시험이 아닙니다.

## 확인한 공식 소스

기준 ZMK는 3450mV 이하에서 0을 반환합니다.
https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/module/drivers/sensor/battery/battery_common.c

이 기판의 원본 설정은 ADC6, output-ohms=1000, full-ohms=1000입니다.
https://raw.githubusercontent.com/22sh22/modu-c-firmware/bee0bb4b812f63f279eb67e928accc89600b5904/modu-module/boards/shields/modu/modu.dtsi

드라이버는 설정된 비율로 환산한 뒤 ZMK 공식을 적용합니다.
https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/module/drivers/sensor/battery/battery_voltage_divider.c

이 소스들은 실물 회로의 정확한 비율이나 실제 SOC를 입증하지는 않습니다.
