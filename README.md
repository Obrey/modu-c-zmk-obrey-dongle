## V9 empirical battery gauge

- L/R wireless-use gauge defaults: 1.75V = 0%, 2.95V = 100%.
- A half connected to USB shows `CHG`; charging voltage is excluded from percentage.
- Raw sensor voltage remains available on the alternating display phase.
- See `docs/V9_EMPIRICAL_BATTERY_KO.md`.

# MODU-C + 동글 — v8 관측 전압 범위 표시

**전체 GitHub 저장소 소스입니다. 컴파일된 UF2는 들어 있지 않습니다.**
기준: `modu-c-dongle-v7-power.zip`.

## 이번에 적용할 것은 동글 파일 하나

폴더 **안의 내용 전체**를 기존 GitHub 저장소 최상위에 덮어씁니다. `.github` 포함.
`Build MODU-C + Dongle`의 **새 빌드가 성공하면** `modu-c-dongle-firmware` 결과물에서:

**`modu_dongle_oled.uf2` → 동글에만 적용합니다.**

현재 v6/v7 좌우 키보드는 그대로 둡니다. `settings_reset`은 넣지 않습니다.
기존에 동작하던 동글 UF2도 복구용으로 보관합니다. 빌드 실패 시 결과물을 사용하지 않습니다.

## 표시 방식과 한계 — 반드시 구분

이 버전은 **실제 배터리 용량의 교정 완료본이 아닙니다.**
사용자가 관측한 센서값 2.90~3.40V 구간 안에서 현재 값이 어디에 있는지 0~100으로 나타내는
**참고용 상대 지수**입니다. 이 관측 범위가 실제 배터리의 방전·완충 기준이라는 증거는 없습니다.
특히 USB 전원을 연결하거나 뺐을 때 이 지수가 움직인다고 실제 배터리 용량이 그만큼 변했다는 뜻은 아닙니다.

| 센서가 보고한 값 | 새 L 표시 | 의미 |
|---|---|---|
| 2.90V 이하 | `L*  0%` | 관측 구간의 하한 이하, 실제 방전 판정 아님 |
| 2.91V | `L*  2%` | 구간 기준 2, 실제 남은 용량 2%라는 뜻 아님 |
| 2.93V | `L*  6%` | 구간 기준 6 |
| 2.99V | `L* 18%` | 구간 기준 18 |
| 3.15V | `L* 50%` | 구간의 중간값 |
| 3.32V | `L* 84%` | 구간 기준 84 |
| 3.33V | `L* 86%` | 구간 기준 86 |
| 3.40V 이상 | `L*100%` | 관측 구간의 상한 이상, 실제 완충 판정 아님 |

`*`는 **검증되지 않은 관측 구간을 기준으로 환산한 지수**라는 표시입니다. 오른쪽도 같은 방식입니다.
정상적인 측정값인지 확인하지 못한 상태에서 실제 배터리 잔량이라고 숨겨서 표시하지 않습니다.
센서가 비정상 범위(2.50V 미만 또는 4.50V 초과)를 반환하면 기존 `ADC?` 표시를 우선합니다.

퍼센트 지수와 원래 전압은 기존처럼 3초마다 교대합니다. 전압은 곱하거나 덧셈 보정하지 않았습니다.
`L*~ 84%`의 `~`, `L~3.32V`의 `~`는 기존처럼 대기 중 마지막 측정값이라는 뜻입니다.
`--%`, `ERR`, `OLD`, `OFF`, `ADC?` 구분도 유지합니다. 오류·미수신을 지수로 변환하지 않습니다.

## 유지되는 부분

기존 키맵/좌우 부트키, 맥 수정키 아이콘, 고양이, 화면 배치/테마,
동글 D 배터리 행, 트랙볼, 좌우 LED, 연결 유지형 절전, 측정/조회 간격은 유지합니다.
센서 핀·전압 환산비·배터리 드라이버·BLE 배터리 보고값·페어링 정보는 변경하지 않았습니다.
따라서 컴퓨터가 받는 **표준 BLE 잔량은 여전히 원래 값**이며 이 참고 지수로 교체되지 않습니다.
이번 변화는 **동글 OLED의 좌우 숫자 표시**에만 적용됩니다.

## 설정 위치

`local-modules/modu-dongle/boards/shields/modu_dongle/modu_dongle_oled.conf`

```ini
CONFIG_MODU_BATTERY_RANGE_DISPLAY=y
CONFIG_MODU_BATTERY_RANGE_LEFT_MIN_MV=1750
CONFIG_MODU_BATTERY_RANGE_LEFT_MAX_MV=2950
CONFIG_MODU_BATTERY_RANGE_RIGHT_MIN_MV=1750
CONFIG_MODU_BATTERY_RANGE_RIGHT_MAX_MV=2950
```

원래 ZMK 잔량 표시로 돌아가려면 첫 설정을 `n`으로 바꾸고, 아래 MIN/MAX 네 줄은 지운 뒤 재빌드합니다.
MIN/MAX는 실제 셀의 안전 전압이나 충전 설정이 아니라 표시용 범위입니다.
범위가 역전되거나 같으면 컴파일 단계에서 차단합니다. 값을 바꾼다고 실제 측정 정확도가 교정되지는 않습니다.

## 검증

구성/패키징 검사, C 모의 배터리/화면/LED 테스트, 새 상대 지수 테스트를 실행했습니다.
40개 unittest 메서드와 별도 validate/selftest 검사 통과. 구체적 기록: `docs/TEST_RESULTS.txt`.
**실제 ARM 펌웨어 컴파일·링크, 기판 측정, 실물 OLED, 실제 배터리 잔량 정확도는 검증하지 못했습니다.**
새 코드가 계산/상태 테스트를 통과한 것과 실제 잔량이 맞는 것은 다른 문제입니다.
이전 v3~v7 문서는 과거 이력이며, 이번 설치는 위의 동글 하나 업데이트가 우선입니다.
