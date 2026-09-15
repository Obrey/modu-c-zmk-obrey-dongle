# v6 validation record

This is a source-only feature/diagnostic addition, not a validated firmware binary.

## Actually executed in this session

| Check | Result | What it establishes |
|---|---|---|
| `python scripts/validate.py` | PASS | Key count, layout metadata, build target/module paths, pinned revisions |
| `python scripts/selftest.py` | PASS | Packaging/HEX/UF2 safeguards using temporary SYNTHETIC test data |
| `python scripts/test_full_config.py` | 14 tests PASS | Existing boot, split routing, board/output-name source checks and synthetic packaging |
| `python scripts/test_status.py` | 12 tests PASS | Existing status helpers, legacy battery UI mock, CMake fixture and theme/source checks |
| `python scripts/test_battery_telemetry.py` | 3 tests PASS | 4 native C test programs plus feature configuration checks |
| v5 byte comparison | 12 files identical | User keymaps, LED logic, trackball overlays, display pins/theme adapter and upstream manifest |

The four new C test programs cover packet length/version/side/error validation;
sensor not-ready, fetch failure, voltage/SOC read failure, valid zero and sample age;
central host filtering, pending-read parameter lifetime, cache invalidation and refs;
and detailed UI mapping including reversed connection order, unknown/zero/error/stale/off.
They compile the new adapter source against test doubles, not against the real Zephyr SDK.
The test doubles do not emulate ADC hardware, radio scheduling, actual OLED rendering or
all implementation details of the GATT stack.

## NOT executed / NOT established

- No full Zephyr/ZMK ARM firmware build or link.
- No genuine UF2 binary was generated; fixture UF2s lived only in temporary test directories.
- No testing on the user's dongle or keyboards.
- No validation of sensor pin wiring, resistor ratio, actual battery voltage, percentage accuracy,
  power consumption or radio/USB/OLED stability.
- No claim that the underlying cause of the previous 0% has been identified or calibrated away.

The ARM toolchain/dependency checkout is not installed in this runtime, and a direct attempt
to retrieve build dependencies failed because network name resolution was unavailable.
GitHub Actions remains the actual firmware build step. Keep the previously working firmware
for rollback. v6 reads and distinguishes measurements; it does not invent a corrected percentage.
