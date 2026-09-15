# 동글 두 버전의 배터리 이벤트 링크 오류 수정

## 발생한 오류

사용자가 제공한 두 빌드 로그가 모두 다음 함수의 미정의 참조로 중단되었습니다.

```text
undefined reference to `raise_zmk_peripheral_battery_state_changed'
```

이는 키맵 문법 오류나 PMW3610 트랙볼 드라이버 오류가 아니라, 최종 펌웨어를 연결하는
링크 단계에서 배터리 이벤트 구현 함수가 빠진 문제입니다. 로그 중 mbedTLS 컴파일 행은
실패 원인이 아닙니다. 제공한 이전 동글 설정의 배터리 옵션 조합이 원인이었습니다.

## 고친 파일

두 파일 모두 아래 경로에 있습니다.

```text
local-modules/modu-dongle/boards/shields/modu_dongle/modu_dongle.conf
local-modules/modu-dongle/boards/shields/modu_dongle/modu_dongle_oled.conf
```

두 파일의 `CONFIG_ZMK_BATTERY_REPORTING=n`을 다음으로 바꾸었습니다.
기존 `n` 행을 그대로 두고 `y` 행을 추가한 것이 아니라 기존 값을 교체했습니다.

```ini
CONFIG_ZMK_BATTERY_REPORTING=y
CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING=y
```

고정 ZMK 커밋 `641514a97db345f499dd50b0360e594270f008fe`의 빌드 파일에서
`src/events/battery_state_changed.c`는 `CONFIG_ZMK_BATTERY_REPORTING`이 켜질 때만
컴파일됩니다. 이 소스에는 동글 자체와 주변 키보드의 배터리 이벤트 구현이 함께 있습니다.
반면 `src/split/central.c`는 주변 키보드 배터리 수신을 켜면 이 함수를 호출합니다.
그래서 이전의 `REPORTING=n` + `FETCHING=y` 조합이 제공된 링크 오류를 설명합니다.

동글 자체에 배터리가 없다는 이유로 배터리 기능을 끄면서 이 의존성을 빠뜨렸습니다.
이번 수정은 표준 설정을 켜는 방식이며, 이벤트를 무시하는 가짜 함수나 별도 C 패치를
추가하지 않았습니다. 배터리 수신도 끄지 않았습니다.

배터리 기능을 켜면 보드가 지원하는 자체 배터리 읽기도 활성화될 수 있습니다.
기본 ZMK 배터리 위젯을 좌우 배터리 전용 화면으로 변경한 것은 아닙니다.
USB 전용 동글 자체의 배터리 수치와 좌우 키보드의 배터리 수치를 혼동하지 마십시오.

## 적용

이전 전체 ZIP의 파일들을 저장소 최상위에 올린 상태라면 이번 ZIP을 풀어 동일한 경로에
덮어쓰고 새 커밋으로 빌드하십시오. GitHub에 ZIP 자체만 올리지 마십시오.
실제 펌웨어 설정 변경은 위 두 `.conf` 파일뿐입니다. 키맵을 따로 수정해 사용 중이라면
이 두 파일만 교체해도 이 수정이 적용되므로, 개인 키맵을 덮어쓸 필요는 없습니다.

전체 ZIP에는 문서·검사 코드 수정도 들어 있습니다. 왼쪽 5를 3초 누른 채 1, 오른쪽 6을
3초 누른 채 0이라는 부트키와 좌우 입력/트랙볼 설정은 이전 전체 ZIP에서 그대로 유지했습니다.
새 저장소를 만들거나 블루투스 설정을 초기화해야 소스 수정이 적용되는 것은 아닙니다.
최초 동글 설치의 초기화 절차는 기존 `FLASHING_KO.md`를 따르십시오.

## 검증한 범위

- 새 정적 회귀 검사를 이전 설정에 실행해 두 동글 설정 모두에서 실패하는 것을 확인했습니다.
- 수정 후 구성 검사 14개 및 기존 메타데이터/HEX/UF2 패키징 검사를 통과했습니다.
- 이전 ZIP과 비교해 키맵, 좌우 overlay, 동글 overlay, 빌드 행렬이 바뀌지 않았는지 확인했습니다.
- 전체 ARM/Zephyr 컴파일·링크, GitHub Actions 실행, 실물 연결/부트 동작은 여기서 검증하지 못했습니다.

따라서 이 ZIP은 오류를 수정한 전체 사용자 설정 소스이며, 컴파일 성공을 보증하거나
검증 완료한 UF2 펌웨어를 제공하는 것은 아닙니다. 실제 빌드 결과는 새 커밋의 Actions에서
확인해야 합니다.

## 대조한 공식 소스

```text
https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/CMakeLists.txt
https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/events/battery_state_changed.c
https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/split/central.c
https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/Kconfig
https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/display/widgets/battery_status.c
```
