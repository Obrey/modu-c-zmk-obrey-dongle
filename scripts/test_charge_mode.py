#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
tele=(root/'local-modules/modu-dongle/src/battery_telemetry_peripheral.c').read_text()
led=(root/'local-modules/modu-dongle/src/peripheral_status_led.c').read_text()
helper=(root/'local-modules/modu-dongle/src/peripheral_power_state.h').read_text()
kcfg=(root/'local-modules/modu-dongle/Kconfig').read_text()
conf=(root/'config/dongle/peripheral-power.conf').read_text()
ui=(root/'local-modules/modu-dongle/src/dongle_status.c').read_text()
keymap_path=root/'config/modu.keymap'
keymap=keymap_path.read_text()
assert 'POWER_USBREGSTATUS_VBUSDETECT_Msk' in helper
assert 'zmk_usb_is_powered()' not in tele
assert 'modu_peripheral_vbus_present()' in tele
assert 'CONFIG_MODU_BATTERY_CHARGE_INTERVAL' in tele
assert '!charging' in led and 'modu_peripheral_vbus_present()' in led
assert 'CONFIG_MODU_CHARGE_MODE=y' in conf
assert 'CONFIG_MODU_BATTERY_CHARGE_INTERVAL=600' in conf
assert 'CONFIG_ZMK_SLEEP=y' in conf
assert 'CONFIG_ZMK_KSCAN_MATRIX_POLLING=y' in conf
assert 'CONFIG_ZMK_KSCAN_DIRECT_POLLING=y' in conf
assert 'CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=7200000' in conf
assert 'false, &display_ranges[side]' in ui
assert '&blt5 L_BOOT_L N5' in keymap and '&blt6 L_BOOT_R N6' in keymap
assert 'config MODU_CHARGE_MODE' in kcfg
print('PASS: v14 charge mode + 10m idle + 2h deep sleep + custom keymap')
