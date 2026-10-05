# MODU-C + Dongle v14 — 10분 Idle + 2시간 Deep Sleep

사용자 커스텀 키맵 + 동글 OLED/배터리 + 충전 저부하 모드 + 2단계 절전.

충전 여부와 관계없이 **10분 무입력 시 connected IDLE**, **2시간 연속 무입력 시 Deep Sleep/System OFF**으로 들어갑니다. 10분 전까지는 정상 ACTIVE 상태를 유지하고, IDLE에서는 polling 경로를 유지해 키/트랙볼 입력으로 즉시 ACTIVE 복귀할 수 있게 합니다. 2시간 이후에는 장시간 배터리 절약을 우선합니다.

Deep Sleep 이후 MODU-C의 키/트랙볼 wake는 하드웨어/드라이버 특성상 보장하지 않으며, Reset 또는 전원 재인가가 확실한 복귀 방법입니다.

상세 변경은 `docs/V14_IDLE_10M_DEEP_2H_KO.md`를 참고하세요.
