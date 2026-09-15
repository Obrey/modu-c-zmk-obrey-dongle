#!/usr/bin/env python3
"""Compile/run pure C helpers; check source contracts. NOT a ZMK/ARM build."""
from __future__ import annotations
import hashlib
import json
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from validate import _parse_build_entries
from test_full_config import layers

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / 'local-modules/modu-dongle'
SHIELD = MODULE / 'boards/shields/modu_dongle'

class StatusTests(unittest.TestCase):
    def test_native_status_helpers(self):
        cc = shutil.which('cc') or shutil.which('gcc') or shutil.which('clang')
        self.assertIsNotNone(cc, 'A native C compiler is required')
        with tempfile.TemporaryDirectory() as temporary:
            exe = str(Path(temporary) / 'status-tests')
            subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-Wno-format-truncation', str(ROOT/'tests/status_logic_test.c'), '-o', exe], check=True)
            subprocess.run([exe], check=True)

    def test_native_ui_simulation(self):
        cc = shutil.which('cc') or shutil.which('gcc')
        self.assertIsNotNone(cc)
        source = (MODULE/'src/dongle_status.c').read_text()
        includes = re.findall(r'^#include <([^>]+)>', source, re.M)
        with tempfile.TemporaryDirectory() as temporary:
            headers = Path(temporary)/'include'
            for include in includes:
                if not include.startswith(('zephyr/', 'zmk/')) and include != 'lvgl.h':
                    continue
                path = headers/include
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('#include "status_ui_mock.h"\n')
            for battery in [0,1]:
                exe = str(Path(temporary)/f'ui-{battery}')
                subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror',
                                f'-DCONFIG_MODU_DONGLE_HAS_BATTERY={battery}',
                                '-I'+str(headers), '-I'+str(ROOT/'tests'),
                                str(ROOT/'tests/status_ui_test.c'), '-o', exe], check=True)
                subprocess.run([exe], check=True)

    def test_oled_custom_mono_config(self):
        text = (SHIELD/'modu_dongle_oled.conf').read_text()
        for key in ['ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y', 'MODU_DONGLE_STATUS_SCREEN=y',
                    'LV_COLOR_DEPTH_1=y', 'LV_Z_BITS_PER_PIXEL=1', 'LV_Z_VDB_SIZE=64',
                    'LV_Z_MEM_POOL_SIZE=16384', 'LV_USE_LABEL=y', 'LV_FONT_MONTSERRAT_10=y',
                    'ZMK_DISPLAY_WORK_QUEUE_DEDICATED=y', 'ZMK_DISPLAY_DEDICATED_THREAD_STACK_SIZE=4096',
                    'MODU_DONGLE_HAS_BATTERY=n']:
            self.assertIn('CONFIG_'+key, text)
        self.assertNotIn('CONFIG_ZMK_DISPLAY_STATUS_SCREEN_BUILT_IN=y', text)

    def test_both_halves_use_actual_split_led(self):
        entries = _parse_build_entries((ROOT/'build.yaml').read_text())
        for e in entries[:2]:
            args = e['cmake']
            self.assertIn('local-modules/modu-dongle', args)
            for arg in ['-DCONFIG_MODU_LED_BREATH=n', '-DCONFIG_MODU_PERIPHERAL_STATUS_LED=y',
                        '-DCONFIG_ZMK_BATTERY_REPORTING=y', '-DCONFIG_BT_BAS=y']:
                self.assertIn(arg, args)
        text = (MODULE/'src/peripheral_status_led.c').read_text()
        self.assertIn('zmk_split_bt_peripheral_is_connected()', text)
        self.assertIn('zmk_split_bt_peripheral_is_bonded()', text)
        self.assertNotIn('zmk_ble_active_profile', text)
        self.assertEqual(text.count('PWM_DT_SPEC_GET(DT_ALIAS(led_status'), 3)

    def test_correct_pinned_api_signatures(self):
        text = (MODULE/'src/dongle_status.c').read_text()
        self.assertIn('zmk_endpoint_is_connected()', text)
        self.assertNotIn('zmk_endpoint_is_connected(endpoint)', text)
        self.assertIn('lv_obj_t *zmk_display_status_screen(void)', text)
        self.assertIn('<zmk/events/battery_state_changed.h>', text)
        self.assertIn('info.state != BT_CONN_STATE_CONNECTED', text)
        self.assertIn('info.role != BT_CONN_ROLE_CENTRAL', text)
        self.assertIn('peripheral_slot_index_for_conn(conn)', text)
        self.assertIn('zmk_keymap_layer_index_to_id(index)', text)

    def test_no_lvgl_in_event_or_bluetooth_callbacks(self):
        text = (MODULE/'src/dongle_status.c').read_text()
        callbacks = text[text.index('static int status_event('):text.index('/* Fixed objects:')]
        self.assertNotRegex(callbacks, r'\blv_\w+\s*\(')
        self.assertIn('k_spin_lock(&state_lock)', callbacks)
        self.assertIn('return ZMK_EV_EVENT_BUBBLE;', callbacks)
        self.assertIn('lv_timer_create(refresh_screen, 250, NULL)', text)

    def test_custom_keymap_provenance_and_fallback(self):
        text = (ROOT/'config/modu.keymap').read_text()
        provenance = json.loads((ROOT/'docs/KEYMAP_PROVENANCE.json').read_text())
        sha = hashlib.sha256((ROOT/'config/modu.keymap').read_bytes()).hexdigest()
        # The snapshot documents this release, but MUST NOT prevent later user edits.
        self.assertEqual(provenance['original_layers_preserved'], 7)
        if sha != provenance['canonical_sha256']:
            print('INFO: canonical keymap has been edited since the release snapshot; allowed.')
        for name in ['modu_dongle.keymap', 'modu_dongle_oled.keymap']:
            self.assertIn('#include "modu.keymap"', (ROOT/'config'/name).read_text())
        guard = (MODULE/'src/keymap_guard.c').read_text()
        for token in ['BUILD_ASSERT', 'DT_NODELABEL(blt5)', 'DT_NODELABEL(blt6)', 'DT_PROP_LEN(MODU_BASE, bindings) == 67']:
            self.assertIn(token, guard)

    def test_persistent_peer_id_not_slot_equals_side(self):
        text = (MODULE/'src/dongle_status.c').read_text()
        self.assertIn('modu_side_for_position(position->position)', text)
        self.assertIn('modu_side_for_peer(&ids, live[i].peer)', text)
        self.assertIn('settings_save_one("modu_status/peers"', text)
        self.assertIn('modu_valid_identities(&loaded)', text)
        self.assertIn('samples[i].valid = false', text)
        self.assertNotRegex(text, r'if\s*\([^)]*percent\s*>\s*0')

    def test_firmware_identification_and_no_fake_dongle_battery(self):
        for name in ['modu_dongle.conf', 'modu_dongle_oled.conf']:
            text = (SHIELD/name).read_text()
            self.assertIn('CONFIG_ZMK_KEYBOARD_NAME="MODU-C Mac v3"', text)
            self.assertIn('CONFIG_ZMK_STUDIO=n', text)
            self.assertIn('CONFIG_ZMK_KEYMAP_SETTINGS_STORAGE=n', text)
            self.assertIn('CONFIG_ZMK_BATTERY_REPORTING=y', text)
        text = (MODULE/'src/dongle_status.c').read_text()
        for token in ['"MAC v3"', '"USB POWER"', 'CONFIG_MODU_DONGLE_HAS_BATTERY',
                      'MODU_CONFIG_KEYMAP_SHA256', '"D:  --%% %s"']:
            self.assertIn(token, text)

if __name__ == '__main__':
    unittest.main(verbosity=2)
