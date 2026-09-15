# MODU-C + 기존 Corne OLED 화면 — v4

## 이번 업데이트에서 넣을 펌웨어

이미 v3에서 양쪽 입력과 LED가 동작한다면 **동글에 `modu_dongle_oled.uf2` 하나만** 넣습니다.
왼쪽/오른쪽 펌웨어를 다시 넣거나 `settings_reset`을 실행할 필요가 없습니다.
이 ZIP은 GitHub에 올리는 **전체 설정 저장소 소스**이며, 컴파일된 UF2가 아닙니다.

## 바뀐 것

- v3의 새 `MAC v3` 대시보드와 사과 모양을 제거했습니다.
- 원래 Corne에서 사용한 `englmaxi/zmk-dongle-display` 화면 구성으로 돌아갑니다.
- 기존 수정키 기호를 Control ⌃ / Option ⌥ / Command ⌘ / Shift ⇧로 표시합니다.
  이것은 눌린 수정키의 화면 표시 변경이며, 실제 키의 Ctrl/Command를 바꾸지 않습니다.
- 기존 USB/Bluetooth 연결 표시, 레이어, Bongo Cat을 유지합니다.
- 우측 상단 배터리 영역에 `D`(동글), `L`(왼쪽), `R`(오른쪽)를 표시합니다.
- 배터리 데이터 수신 전에는 `--%`, 알고 있는 한쪽이 끊어지면 `OFF`를 표시합니다.
- v3의 단색 테마 비활성화와 부분 화면 버퍼 설정을 바꾸었습니다.

## 바꾸지 않은 것

`config/modu.keymap`, 좌우 역할/트랙볼 오버레이, 좌우 LED 드라이버,
화면 없는 동글 설정 및 `build.yaml`은 v3와 바이트 단위로 동일합니다.
왼쪽 `5`를 3초 누른 채 `1`, 오른쪽 `6`을 3초 누른 채 `0` 부트키도 그대로입니다.

## GitHub 적용

압축의 바깥 폴더 **안의 내용**을 기존 저장소의 같은 경로에 덮어쓰고 커밋합니다.
`.github`, `config`, `local-modules`, `scripts`, `tests` 등이 저장소 최상위에 와야 합니다.
`config/west.yml`도 반드시 올리세요. 원래 화면 모듈을 빌드 중 받는 설정이 추가되었습니다.
워크플로우 `Build MODU-C + Dongle`가 성공하면 `modu-c-dongle-firmware`를 받습니다.
그 안의 **`modu_dongle_oled.uf2`만 현재 동글에 적용**합니다.

## 검증 범위

정적 구성 검사, 호스트 C 배터리/좌우 식별 검사, CMake 호환처리 검사,
HEX/UF2 패키징 검사를 실행했습니다. **실제 ARM/Zephyr/LVGL 컴파일과 실물 OLED 검증은
수행하지 못했습니다.** 흰 화면의 정확한 원인은 런타임 로그 없이 확정되지 않았습니다.
이번 파일은 원래 UI를 복구하고 화면 설정을 교정한 수정 소스이지, 실물 검증 완료 UF2가 아닙니다.

## 동글 배터리

`D`는 보드의 실제 배터리 센서 이벤트로 갱신합니다. 배터리가 없는 USB 전용 동글에서는
센서 수치가 실제 잔량을 뜻하지 않을 수 있습니다. 그런 하드웨어는
`modu_dongle_oled.conf`의 `CONFIG_MODU_DONGLE_HAS_BATTERY=n`으로 바꾸면 `D USB`로 표시합니다.
이번 기본값은 요청대로 동글 잔량 읽기 활성화입니다. 수신 전에는 `D --%`입니다.

세부 설명: `docs/FLASHING_KO.md`, `docs/IMPLEMENTATION_NOTES.md`.
