#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[1]
conf = (root / 'config/dongle/peripheral-power.conf').read_text()
required = [
    'CONFIG_ZMK_IDLE_TIMEOUT=600000',
    'CONFIG_ZMK_SLEEP=y',
    'CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=7200000',
    'CONFIG_ZMK_KSCAN_MATRIX_POLLING=y',
    'CONFIG_ZMK_KSCAN_DIRECT_POLLING=y',
]
for item in required:
    assert item in conf, item
print('PASS: v14 10m-idle/2h-deep-sleep config')
