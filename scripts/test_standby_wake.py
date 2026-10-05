#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
cfg=(root/'config/dongle/peripheral-power.conf').read_text()
assert 'CONFIG_ZMK_IDLE_TIMEOUT=600000' in cfg
assert 'CONFIG_ZMK_SLEEP=y' in cfg
assert 'CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=7200000' in cfg
assert 'CONFIG_ZMK_KSCAN_MATRIX_POLLING=y' in cfg
assert 'CONFIG_ZMK_KSCAN_DIRECT_POLLING=y' in cfg
# Keep vendor PMW3610 polling alive during the first-stage connected IDLE.
assert 'CONFIG_PMW3610_ALT_POLL_INTERVAL_MS=0' not in cfg
print('PASS: 10 min connected IDLE + 2 h Deep Sleep policy is applied')
