# v8 validation scope

Ran in this session:

```sh
python3 scripts/validate.py
python3 scripts/selftest.py
python3 scripts/test_full_config.py
python3 scripts/test_status.py
python3 scripts/test_battery_telemetry.py
python3 scripts/test_power.py
python3 scripts/test_battery_range.py
```

40 unittest methods passed: 14 full configuration, 12 status, 3 telemetry,
4 power, 7 range display. validate.py and synthetic packaging selftest.py passed separately.
See docs/TEST_RESULTS.txt for actual output. Synthetic UF2s used in packaging tests are NOT firmware
and are not included in this archive.

The real display adapter C is compiled on the host with explicit Zephyr/BLE/LVGL test doubles.
Range conversion also tests every uint16 input for monotonicity/bounds and label length.
Compile-error tests reject reversed/equal per-hand ranges. Tests verify feature-off compatibility,
unchanged raw telemetry, unchanged D row, error states, independent hand profiles and existing layout size.

NOT run: ARM firmware compilation/link, on-device flashing, physical display rendering,
actual cell voltage/capacity calibration or hardware charge/power measurements.
No west or ARM Zephyr toolchain is installed in this runtime.
This is a source archive. GitHub Actions must build it before flashing.

The new number is visibly marked as an uncalibrated observed-range index, NOT verified state of charge.
Raw ADC readings, percentage conversion and BLE BAS reports remain unchanged.
config/modu.keymap, build matrix, hand overlays, power settings, peripheral status LED,
telemetry protocol/server/client, original-display adapter and screen styles are byte-identical to v7.
