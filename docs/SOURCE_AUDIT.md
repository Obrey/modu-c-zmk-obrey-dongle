# 업로드 소스에서 직접 확인한 항목

이 메모는 웹에서 추정한 사양이 아니라 ZIP 안의 텍스트 설정을 읽은 결과입니다.
실제 PCB의 전기적 상태나 장치가 어느 펌웨어를 실행 중인지 확인한 결과는 아닙니다.

## MODU-C 실제 하드웨어

업로드: `modu-c-firmware-main.zip`
ZIP archive comment: `bffdf7e0bf6c1e7573b35492fb9f611a5c24fb5e`

* `modu-module/boards/shields/modu/Kconfig.defconfig`: SHIELD_MODU_LEFT에서
  `ZMK_SPLIT_ROLE_CENTRAL` 기본값 y. 원래의 본체는 왼쪽입니다.
* `modu.dtsi`: 두 쪽 공통 PMW3610(`pixart,pmw3610-alt`), 600 CPI,
  SPI0 CS P0.21, IRQ P0.17. 주변 장치용 input-split@0도 있습니다.
* `modu_left.overlay`: 왼쪽 로컬 트랙볼과 오른쪽 proxy의 HID listener 두 개 활성화.
* `modu_right.overlay`: col-offset 6; 로컬 트랙볼을 trackball_split으로 전달.
* `modu.dtsi`: vbatt sensor는 ADC channel 6, output-ohms=1000, full-ohms=1000.
* `modu_right.overlay`: 오른쪽 직접 입력은 P0.26/P0.04/P0.05 외에 P1.08을 포함합니다.
  따라서 논리 66번(R5C9)은 이 소스에서 별도 GPIO 입력이 정의된 칸입니다.
  화면에 빈 칸처럼 보인다는 이유만으로 없는 키라고 단정할 수 없습니다.
* board 파일: `ms88sf3/nrf52840`, app 0x26000..0xEC000, settings 0xEC000..0xF4000.
  UF2 boot-retention 설정이 있습니다.

위 원본 45개 파일은 `vendor/modu-c-firmware/`에서 그대로 확인할 수 있습니다.
그 파일 해시는 `reference/hardware-sha256.json`에 기록했고 검사합니다.

## 기존 Corne 동글

업로드: `zmk-corne-dongle-main.zip`
ZIP archive comment: `717d5d7abb7145f01c93384b03ee5a699d72054e`

* `build.yaml`: 실제 활성 동글 행의 board `nice_nano_v2`,
  shield `eyeslash_corne_central_dongle dongle_display`.
* `eyeslash_corne_central_dongle.overlay`: SH1106 @0x3c,
  SDA=P0.17, SCL=P0.20, width=129, height=64, segment-offset=1.
* `config/west.yml`: cormoran ZMK v0.3 fork와 여러 RPC/display 모듈 의존.

배선 블록을 새 어댑터에 복사했습니다. 새 버전 기본 OLED 화면을 사용하고
old v0.3 display/RPC 구현은 복사하지 않았습니다.
I2C `pinctrl-names`를 명시했습니다. 나머지 OLED 배선/패널 값은 보존했습니다.

## 신규 동글 라우팅

* 왼쪽: 원본 left overlay + 이전 HID 경로 disabled + 채널 0 송신.
* 오른쪽: 원본 right overlay + 이전 proxy disabled + 채널 1 송신.
* 동글: MODU GPIO가 없는 mock scan + 동일 67-position transform + 두 채널 수신.
* 세 정상 펌웨어가 모두 같은 config/modu.keymap을 빌드합니다.
* 커스텀 부트 동작은 동글에서만 컴파일되고, 실제 source를 통해 좌우에 전달합니다.

펌웨어 전체 컴파일과 실제 트랙볼/화면/부트 전달 시험은 미실행입니다.


<!-- MODU-LED-SHIELD-COMPAT-v1 -->
## 정정: 동글 LED 호환 패치 적용 후

위의 "원본 45개 파일 그대로" 설명은 이 패치 적용 전 상태입니다.
현재는 `modu-module/src/led_breath/led_breath.c`의 좌우 판별 매크로 두 개에
`SHIELD_MODU_PERIPHERAL_LEFT/RIGHT` 이름을 추가했습니다. 나머지 하드웨어
44개 파일은 이 패치가 변경하지 않습니다. 변경 전후 SHA-256은
`reference/hardware-normalized-sha256.json`의 `approved_local_patches`에,
원인과 검증 범위는 `docs/LED_SHIELD_COMPAT.md`에 기록했습니다.
