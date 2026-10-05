# MODU-C v12 안정 깨우기 절전

## 목표

충전 중이든 배터리 사용 중이든 동일하게 동작합니다.

- 입력 중: ACTIVE
- 30초 무입력: IDLE(저전력 대기)
- 키 입력 또는 트랙볼 움직임: 즉시 ACTIVE 복귀
- BLE split 연결 유지
- 충전 중에도 키/트랙볼 성능 제한 없음

## 왜 Deep Sleep을 제거했나

MODU-C가 사용하는 PMW3610 ALT 드라이버의 Kconfig는 이 하드웨어에서 nRF GPIO SENSE/PORT level interrupt가 동작하지 않는 경우 polling이 필요하며, 이 polling은 deep sleep과 양립하지 않는다고 명시합니다. 실제 v11 실물 테스트에서도 deep sleep 후 키 입력으로 깨어나지 않았습니다.

따라서 v12는 `CONFIG_ZMK_SLEEP=n`으로 System OFF/Deep Sleep을 사용하지 않고, 제작자 호환 polling을 유지합니다. 이것은 완전 전원 OFF 수준의 절전은 아니지만, LED OFF, PMW3610의 ZMK IDLE 성능 저하 모드, 느린 배터리 텔레메트리 갱신과 함께 안정적으로 깨어나는 절전 대기입니다.

## 충전 모드

충전 중에도 ACTIVE/IDLE 전환 규칙은 배터리 사용 시와 동일합니다. VBUS가 있으면 상태 LED를 끄고 커스텀 배터리 진단 갱신만 10분으로 줄입니다. 키, 트랙볼, BLE split 입력은 제한하지 않습니다.

## 키맵

`config/modu.keymap`은 v11에서 사용하던 사용자 커스텀 키맵과 바이트 단위로 동일합니다. 원본 Obrey 키맵을 기반으로 하며, 사용자가 요청한 5/6 홀드 부트 동작이 포함되어 있습니다.
