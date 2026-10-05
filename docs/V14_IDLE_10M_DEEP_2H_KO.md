# MODU-C v14 — 10분 Idle + 2시간 Deep Sleep

v13에서 사용자가 요청한 대로 IDLE 진입 시간을 30초에서 10분으로 늘렸습니다. Deep Sleep 기준은 2시간으로 그대로 유지했습니다.

- 0~10분 무입력: ACTIVE 상태 유지
- 10분 무입력: ZMK IDLE. BLE/폴링 경로를 유지하여 키 또는 트랙볼 입력 시 ACTIVE로 즉시 복귀
- 2시간 연속 무입력: ZMK Deep Sleep/System OFF. 장시간 미사용 시 소비전력 최소화
- 충전 중/배터리 사용 중 모두 동일한 시간 정책
- Deep Sleep 이후 키/트랙볼 wake는 MODU-C 하드웨어/드라이버 특성상 보장하지 않으므로 Reset 또는 전원 재인가가 확실한 복귀 방법

키맵, OLED, 배터리 표시, 충전 저부하 처리, 부트키 동작은 v13에서 변경하지 않았습니다.
