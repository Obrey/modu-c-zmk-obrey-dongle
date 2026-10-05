from pathlib import Path

root = Path(__file__).resolve().parents[1]
conf = (root / 'config/dongle/peripheral-power.conf').read_text()
led = (root / 'local-modules/modu-dongle/src/peripheral_status_led.c').read_text()
universal = (root / 'local-modules/modu-dongle/src/universal_deep_sleep.c').read_text()
required = [
    'CONFIG_ZMK_SLEEP=y',
    'CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=600000',
    'CONFIG_MODU_UNIVERSAL_DEEP_SLEEP=y',
    'CONFIG_MODU_UNIVERSAL_DEEP_SLEEP_TIMEOUT_MS=120000',
    'CONFIG_ZMK_KSCAN_MATRIX_POLLING=n',
    'CONFIG_ZMK_KSCAN_DIRECT_POLLING=n',
]
for item in required:
    assert item in conf, item
assert 'BUILD_ASSERT(!IS_ENABLED(CONFIG_ZMK_SLEEP)' not in led
assert 'zmk_activity_get_state() != ZMK_ACTIVITY_IDLE' in universal
assert 'zmk_pm_suspend_devices()' in universal
assert 'sys_poweroff();' in universal
assert 'zmk_usb_is_powered' not in universal
print('PASS: v11 universal deep sleep config and USB-independent System OFF checks')
