#!/usr/bin/env python3
"""Host C adapter tests and source checks. Not an ARM build or a BLE/ADC test."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
from validate import _parse_build_entries

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'local-modules/modu-dongle/src'

class BatteryTelemetryTests(unittest.TestCase):
    def test_native_adapters_and_ui(self):
        cc = shutil.which('cc') or shutil.which('gcc')
        self.assertIsNotNone(cc)
        for fixture, source, mock in [
            ('battery_protocol_test.c', None, None),
            ('battery_peripheral_test.c', 'battery_telemetry_peripheral.c', 'battery_runtime_mock.h'),
            ('battery_central_test.c', 'battery_telemetry_central.c', 'battery_runtime_mock.h'),
            ('battery_ui_test.c', 'dongle_status.c', 'status_ui_mock.h'),
        ]:
            with self.subTest(fixture=fixture), tempfile.TemporaryDirectory() as tmp:
                include = Path(tmp)/'include'
                if source:
                    for name in re.findall(r'^#include <([^>]+)>', (SRC/source).read_text(), re.M):
                        if name.startswith(('zephyr/', 'zmk/')) or name == 'lvgl.h':
                            p=include/name;p.parent.mkdir(parents=True, exist_ok=True)
                            p.write_text(f'#include "{mock}"\n')
                exe=Path(tmp)/'test'
                subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror',
                                '-Wno-unused-function','-Wno-unused-variable',
                                '-I'+str(include),'-I'+str(ROOT/'tests'),
                                str(ROOT/'tests'/fixture),'-o',str(exe)],check=True)
                subprocess.run([str(exe)],check=True)

    def test_all_three_active_firmwares_enable_feature(self):
        entries=_parse_build_entries((ROOT/'build.yaml').read_text())
        for e in entries[:2]:
            self.assertIn('-DCONFIG_MODU_BATTERY_TELEMETRY_PERIPHERAL=y',e['cmake'])
            self.assertIn('-DCONFIG_ZMK_BATTERY_REPORTING=y',e['cmake'])
            self.assertIn('-DCONFIG_BT_BAS=y',e['cmake'])
        cfg=(ROOT/'local-modules/modu-dongle/boards/shields/modu_dongle/modu_dongle_oled.conf').read_text()
        self.assertIn('CONFIG_MODU_BATTERY_TELEMETRY_CENTRAL=y',cfg)

    def test_sensor_config_not_guessed_and_existing_paths_not_replaced(self):
        for name in ['modu_left.overlay','modu_right.overlay']:
            text=(ROOT/'config/dongle'/name).read_text()
            self.assertNotIn('full-ohms',text)
            self.assertNotIn('io-channels',text)
        server=(SRC/'battery_telemetry_peripheral.c').read_text()
        self.assertIn('DEVICE_DT_GET(DT_CHOSEN(zmk_battery))',server)
        self.assertIn('BT_GATT_PERM_READ_ENCRYPT',server)
        self.assertIn('zmk_workqueue_lowprio_work_q()',server)
        self.assertNotIn('bt_unpair',server)
        self.assertNotIn('bt_bas_set_battery_level',server)
        client=(SRC/'battery_telemetry_central.c').read_text()
        self.assertIn('info.role != BT_CONN_ROLE_CENTRAL',client)
        self.assertIn('bt_conn_get_security(conn) < BT_SECURITY_L2',client)
        self.assertNotIn('bt_conn_le_create',client)
        self.assertNotIn('lv_',client)

if __name__=='__main__': unittest.main(verbosity=2)
