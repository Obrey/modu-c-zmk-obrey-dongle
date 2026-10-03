from pathlib import Path

root = Path(__file__).resolve().parents[1]
conf = (root / 'config/dongle/peripheral-power.conf').read_text()
led = (root / 'local-modules/modu-dongle/src/peripheral_status_led.c').read_text()
required = [
    'CONFIG_ZMK_SLEEP=y',
    'CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=600000',
    'CONFIG_ZMK_KSCAN_MATRIX_POLLING=n',
    'CONFIG_ZMK_KSCAN_DIRECT_POLLING=n',
]
for item in required:
    assert item in conf, item
assert 'BUILD_ASSERT(!IS_ENABLED(CONFIG_ZMK_SLEEP)' not in led
print('PASS: v10 deep sleep config and LED compatibility checks')
