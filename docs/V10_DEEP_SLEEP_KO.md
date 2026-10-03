# v10 Deep Sleep 변경 사항

## 적용 대상
- `modu_left.uf2`
- `modu_right.uf2`

동글 펌웨어는 v9와 동일한 전원 정책을 유지합니다. USB 전원 동글은 자동 Deep Sleep 대상이 아닙니다.

## 동작
- 30초 무입력: ZMK Idle. 상태 LED가 꺼지고 배터리/상태 작업 빈도가 줄어듭니다.
- 10분 무입력: ZMK Deep Sleep (`sys_poweroff`). BLE split 연결이 끊기고 소비전력이 크게 줄어듭니다.
- 깨우기: **키를 한 번 누릅니다.** 깨어난 뒤 동글에 BLE 재연결되므로 첫 키는 깨우기용으로 소비되거나 입력이 늦게 보일 수 있습니다.

## 왜 polling을 끄나
제작자 `modu_left.conf`는 원래 왼쪽이 central일 때 matrix/direct polling을 강제합니다. 오른쪽은 해당 polling 강제가 없습니다.
현재 구성은 동글이 central이고 좌/우 모두 peripheral이므로 v10은 양쪽에
`CONFIG_ZMK_KSCAN_MATRIX_POLLING=n`, `CONFIG_ZMK_KSCAN_DIRECT_POLLING=n`을 적용해
GPIO interrupt 기반 kscan으로 통일합니다. 원본 devicetree의 `kscan_matrix`,
`kscan_direct`, composite kscan에는 이미 `wakeup-source`가 선언돼 있습니다.

## 트랙볼 wake
PMW3610에는 실제 IRQ GPIO가 연결되어 있지만 현재 핀/드라이버 조합에서
`sys_poweroff`를 **트랙볼 움직임만으로** 깨우는 것은 이 빌드에서 보장하지 않습니다.
Deep Sleep 후에는 키를 한 번 눌러 깨운 다음 트랙볼을 사용하세요.

## 복구
실물에서 10분 후 키로 깨어나지 않으면 하드웨어 고장이 아닙니다.
부트로더로 들어가 v9 `modu_left.uf2` / `modu_right.uf2`를 다시 플래시하면 됩니다.
`settings_reset`은 필요 없습니다.
