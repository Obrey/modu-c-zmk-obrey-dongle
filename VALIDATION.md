# v7 validation scope

Executed successfully in this session:

```sh
python3 scripts/validate.py
python3 scripts/selftest.py
python3 scripts/test_full_config.py
python3 scripts/test_status.py
python3 scripts/test_battery_telemetry.py
python3 scripts/test_power.py
```

33 unittest methods across the four test_* scripts passed; validate.py and selftest.py passed separately.
The actual firmware C battery adapters / LED adapter are compiled with native GCC against explicitly mocked Zephyr/BLE/LVGL/PWM APIs.
This verifies the adapter logic exercised by those fixtures, NOT real Zephyr scheduling, radio, driver integration, or OLED rendering.
Native tests verify v1/v2 wire decoding, error/zero separation, idle sample periods and cached reads, stale link handling, two LED channel maps and idle/wake transitions.
See docs/TEST_RESULTS.txt for execution output. Synthetic UF2 fixtures there are NOT actual usable firmware.

Not run: actual ARM firmware compilation/link, on-device flashing, ADC calibration, charge current, power measurement, physical wake behavior.
No west/ARM toolchain is installed here. Container dependency fetching failed DNS resolution. Download-tool fallback failed as well.
User GitHub Actions must build the new source; retain the previous working UF2s for rollback.

config/modu.keymap, trackball overlays, original-display adapter and display layout/styles are byte-identical to v6.
Only explicit power settings and battery/LED telemetry code are changed as listed in docs/V7_CHANGE_MANIFEST.json.
