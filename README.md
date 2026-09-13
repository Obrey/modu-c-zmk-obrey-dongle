# MODU-C + Obrey nice!nano v2 동글

**비공식 개인용 동글 전환 소스. 실제 펌웨어 컴파일·실물 검증 전입니다.**

이 저장소는 업로드된 MODU 하드웨어 소스와 Corne 동글의 OLED 배선을 바탕으로
구성했습니다. 예전 Corne 펌웨어를 MODU-C에 쓰는 방식이 아닙니다.

```
MODU-C 왼쪽 ── BLE ──┐
                    nice!nano v2 + SH1106 동글 ── USB ── PC
MODU-C 오른쪽 ─ BLE ─┘
```

## 이번 버전의 범위

* Base / Nav / Mouse / Game / FnMedia 다섯 레이어를 보존합니다.
  대화에 남아 있는 마지막 원본기반 rebuild 키맵과 335개 일반 바인딩이 같습니다.
  그 후 Keymap Editor에서 개인적으로 변경한 내용은 업로드되지 않았으므로 자동 반영되지 않습니다.
* `1+3+5`와 `6+8+0`은 실제 입력 출처에 부트로더 명령을 전달하는 기존 확장을 사용합니다.
  Base 전용, 80ms 동시 입력 범위, 직전 입력 휴식 300ms입니다.
  유지보수 레이어나 추가 실행키는 없습니다.
* MODU 양쪽은 주변 장치, 동글만 중앙 장치입니다. 동글을 빼면 키보드 USB 단독 입력은 안 됩니다.
* 왼쪽 트랙볼은 input-split ID 0, 오른쪽은 ID 1입니다.
  양쪽 센서의 원래 배선·600 CPI·방향 보정·P0.08 방향 선택은 바꾸지 않았습니다.
* OLED는 업로드된 배선(SDA P0.17, SCL P0.20, SH1106 0x3c)을 그대로 사용하고,
  **ZMK 기본 상태 화면**을 사용합니다. 기존 Corne의 맞춤 dongle-display 화면과 같지 않습니다.
* 예전 Corne의 cormoran v0.3, Studio/RPC, runtime-sensor, battery-history,
  prospector, nice_view 모듈은 가져오지 않았습니다. Keymap Editor 편집은 유지합니다.
  이 버전에는 ZMK Studio 지원을 추가하지 않았습니다.
* 기존 동글을 MODU 전용으로 다시 플래시합니다. Corne과 MODU를 같은 동글로
  자동 전환하거나 동시에 쓰는 구성은 아닙니다. Corne 동글 UF2를 먼저 백업하세요.

## 어느 파일에서 시작했나

| 소스 | 사용한 버전 |
|---|---|
| MODU 실제 하드웨어 | 업로드 `modu-c-firmware-main.zip`, archive commit `bffdf7e0bf6c1e7573b35492fb9f611a5c24fb5e` |
| MODU 설정 저장소 | 업로드 `modu-c-zmk-config-main.zip`의 레이아웃·ZMK 고정 버전·패키징 도구 |
| Corne 동글 | 업로드 ZIP의 nice!nano v2 선택과 SH1106 배선 |
| 개인 키맵 | `modu-c-obrey-rebuilt-from-original.zip`의 5개 레이어 + 좌우 부트 콤보 확장 |

이번 하드웨어 ZIP은 예전 설정의 고정 버전 `bee0bb4...`와 다릅니다.
두 버전을 혼합하지 않습니다. 실제 하드웨어 소스 **45개 파일을 수정 없이**
`vendor/modu-c-firmware/`에 넣고, 별도 어댑터에서 동글 전용 변경을 덧붙입니다.
ZMK 자체는 원본 설정의 `641514a97db345f499dd50b0360e594270f008fe`를 유지합니다.
따라서 GitHub Actions에서는 ZMK/Zephyr를 내려받지만 MODU 하드웨어 저장소를 또 내려받지 않습니다.

nice!nano v2의 새 빌드 이름은 `nice_nano@2.0.0//zmk`입니다.
Corne v0.3의 `nice_nano_v2` 문자열을 새 ZMK에 그대로 쓰지 않았습니다.

## 가장 안전한 업로드 방법

**지금 작동 중인 저장소는 그대로 두고 새 저장소에서 빌드하세요.**
추천 이름은 `Obrey/modu-c-zmk-obrey-dongle`입니다.
기존 `Obrey/zmk-corne-dongle`도 보존하세요.

GitHub에서 README/라이선스 추가 없이 빈 저장소를 생성한 다음,
이 ZIP을 풀어 나온 `modu-c-zmk-obrey-dongle` 폴더에서 PowerShell을 엽니다.
이 폴더에는 `.git`이 포함되어 있지 않습니다.

```powershell
python scripts/validate.py
git init -b main
git add .
git commit -m "Add MODU-C dongle configuration"
git remote add origin https://github.com/Obrey/modu-c-zmk-obrey-dongle.git
git push -u origin main
```

이미 `.git`이 있는 기존 폴더에서 위 초기화 명령을 실행하지 마세요.
이 구성은 파일 자동 삭제나 기존 원격 변경을 하지 않습니다.
GitHub 웹으로 업로드할 때도 **.github, local-modules, vendor 폴더까지 전부** 올려야 합니다.
`config/modu.keymap`만 올리면 동글/트랙볼/좌우 부트 확장이 함께 빌드되지 않습니다.

Actions의 **Build MODU-C Dongle Firmware**에서 전체 결과를 확인합니다.
성공하면 **modu-c-dongle-firmware** 아티팩트를 받습니다.
중간 아티팩트는 오류 분석을 위해 삭제하지 않습니다.

## 빌드 결과물

| 파일 | 대상 |
|---|---|
| `modu_left_dongle.uf2` | MODU 왼쪽의 정상 동글용 펌웨어 |
| `modu_right_dongle.uf2` | MODU 오른쪽의 정상 동글용 펌웨어 |
| `modu_dongle.uf2` | nice!nano v2 USB 동글의 정상 펌웨어 |
| `modu_settings_reset.uf2` | MODU 왼쪽·오른쪽 각각의 연결 정보 초기화용 |
| `dongle_settings_reset.uf2` | nice!nano 동글 연결 정보 초기화용 |

**이 소스 ZIP 안에는 아직 UF2 바이너리가 없습니다.** 위 파일들은 실제 Actions 빌드 성공 후 생성됩니다.
모든 UF2는 nRF52840 family와 앱 영역만 검사해서 패키징합니다.
동글용·좌우용을 섞어 넣지 마세요.

## 처음 전환하기

[처음 설치·초기화·복구 절차](docs/FLASH_KO.md)를 읽으세요.
처음에는 세 장치 모두 기존 연결 정보를 초기화하고 정상 펌웨어를 다시 넣어야 합니다.
`BT_CLR`는 호스트 프로필 삭제이므로 이 전환용 전체 초기화를 대신하지 못합니다.
초기화 전용 펌웨어는 일반 키보드가 아니며, 초기화한 뒤에는 키 콤보가 작동하지 않습니다.
**부트로더에 다시 들어갈 물리 리셋 방법을 확보한 뒤 시작하세요.**

## 다음 키맵 수정

연결 전환을 마친 후에는 일반 키배치만 바꿀 때 주로 동글의 `modu_dongle.uf2`를 갱신합니다.
GPIO·트랙볼·무선통신·보드 설정을 바꿀 때는 양쪽도 같은 빌드로 갱신하세요.
`135`/`680`은 키보드 좌우를 갱신하기 위한 콤보이며, 동글 자체 부트로더용은 아닙니다.
동글 자체는 그 장치의 기존 물리 리셋 방법으로 진입합니다.

키맵 에디터에서 `config/modu.keymap`을 편집할 수 있습니다.
검사기는 `mouse` 같은 개인 콤보 이름이나 게임 방향키 위치를 고정하지 않습니다.
단, 숫자 135/680을 이동하면 안전한 부트 대상 확인을 위해 해당 콤보/가드도 함께 조정해야 합니다.

## 배터리 0% 관련

실제 하드웨어 소스에는 `vbatt`가 있습니다. ADC 채널 6, 분압 값 1000/1000입니다.
이전의 "센서 설정 자체가 없다"는 결론은 실제 하드웨어 소스를 보기 전의 잘못된 추정입니다.
이번 작업에서는 측정 저항·ADC 설정을 추측해서 바꾸지 않았습니다.
USB 전원만 쓰는 동글 자체는 배터리 보고를 끄고, 양쪽 배터리 값은 수신 가능하게 설정했습니다.
**동글 전환이 양쪽의 실제 잔량 측정 오류까지 고쳤다는 뜻은 아닙니다.**
USB 없이 켜진다고 실제 잔량이 충분하다고 단정할 수도 없습니다.

## 확인 범위와 남은 것

`VALIDATION.json`에 실제 수행한 검사 결과가 있습니다.
소스 배열/역할/트랙볼 경로 검사, Keymap Editor 편집 허용 회귀시험,
합성 HEX/UF2 패키징 시험과 실제 boot C 코드의 호스트 모의시험을 실행했습니다.
외부 ZMK 소스 다운로드와 ARM 툴체인이 없어 **실제 ZMK/Zephyr 컴파일, BLE 전달,
OLED 화면 및 키보드 실물 시험은 하지 못했습니다.**
이 결과를 검증된 완성 펌웨어라고 보아서는 안 됩니다.

## 자료

* https://zmk.dev/docs/hardware-integration/dongle
* https://zmk.dev/docs/hardware-integration/pointing
* https://zmk.dev/docs/features/split-keyboards
* https://zmk.dev/docs/troubleshooting/connection-issues
* https://zmk.dev/docs/config/displays
* https://zmk.dev/docs/config/battery
* https://zmk.dev/blog/2025/12/09/zephyr-4-1

개인 비상업용 수정본입니다. 원저작자와 각 소스의 라이선스는 LICENSE/NOTICE/THIRD_PARTY_NOTICES에 보존했습니다.
