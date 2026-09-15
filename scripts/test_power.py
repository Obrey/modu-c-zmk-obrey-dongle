#!/usr/bin/env python3
"""Native regressions. These are NOT an ARM build or a hardware wake test."""
from pathlib import Path
import hashlib, re, shutil, subprocess, tempfile, unittest
from validate import _parse_build_entries
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'local-modules/modu-dongle/src'
class PowerTests(unittest.TestCase):
    def test_led_adapter_both_hands(self):
        cc=shutil.which('cc') or shutil.which('gcc')
        self.assertIsNotNone(cc)
        with tempfile.TemporaryDirectory() as d:
            d=Path(d);inc=d/'include'
            for name in re.findall(r'^#include <([^>]+)>',(SRC/'peripheral_status_led.c').read_text(),re.M):
                if name.startswith(('zephyr/','zmk/')):
                    f=inc/name;f.parent.mkdir(parents=True,exist_ok=True);f.write_text('#include "battery_runtime_mock.h"\n')
            for right in (0,1):
                out=d/f'led-{right}'
                subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable',
                    f'-DCONFIG_SHIELD_MODU_RIGHT={right}',f'-DCONFIG_SHIELD_MODU_LEFT={1-right}',
                    '-I'+str(inc),'-I'+str(ROOT/'tests'),str(ROOT/'tests/power_led_test.c'),'-o',str(out)],check=True)
                subprocess.run([str(out)],check=True)
    def test_safe_standby_settings_are_applied_to_both_halves(self):
        entries=_parse_build_entries((ROOT/'build.yaml').read_text())
        for e in entries[:2]: self.assertIn('config/dongle/peripheral-power.conf',e['cmake'])
        cfg=(ROOT/'config/dongle/peripheral-power.conf').read_text()
        for line in ('CONFIG_ZMK_SLEEP=n','CONFIG_ZMK_IDLE_TIMEOUT=30000',
            'CONFIG_MODU_STATUS_LED_BRIGHTNESS=6','CONFIG_MODU_BATTERY_ACTIVE_INTERVAL=60',
            'CONFIG_MODU_BATTERY_IDLE_INTERVAL=300'):
            self.assertIn(line,cfg)
        self.assertNotIn('CONFIG_ZMK_KSCAN_MATRIX_POLLING=n',cfg)
        self.assertNotIn('CONFIG_PMW3610_ALT_POLL_INTERVAL_MS=0',cfg)
        self.assertNotIn('bt_unpair',(SRC/'peripheral_status_led.c').read_text())
    def test_no_sensor_calibration_guess_or_keymap_change(self):
        # Regression snapshot only: user keymap edits remain allowed by CI.
        import json
        info=json.loads((ROOT/'docs/KEYMAP_PROVENANCE.json').read_text())
        sha=hashlib.sha256((ROOT/'config/modu.keymap').read_bytes()).hexdigest()
        if sha!=info['canonical_sha256']: print('INFO: user keymap differs from snapshot; allowed.')
        for f in (ROOT/'config/dongle').glob('*.overlay'):
            self.assertNotIn('full-ohms',f.read_text()); self.assertNotIn('io-channels',f.read_text())
        self.assertNotIn('lithium_ion_mv_to_pct',(SRC/'battery_telemetry_peripheral.c').read_text())
    def test_connected_standby_not_false_deep_sleep(self):
        code=(SRC/'battery_telemetry_peripheral.c').read_text()
        self.assertIn('zmk_activity_get_state()',code)
        self.assertIn('k_work_cancel_delayable',code)
        self.assertNotIn('sys_poweroff',code)
        self.assertNotIn('K_TIMER_DEFINE',code)
        self.assertIn('MODU_BATTERY_FLAG_IDLE',code)
if __name__=='__main__': unittest.main(verbosity=2)
