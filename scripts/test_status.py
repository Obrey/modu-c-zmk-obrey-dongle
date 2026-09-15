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

    def test_native_battery_widget_simulation(self):
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
                    'LV_COLOR_DEPTH_1=y', 'LV_Z_BITS_PER_PIXEL=1', 'LV_Z_VDB_SIZE=100',
                    'LV_Z_MEM_POOL_SIZE=32768', 'LV_USE_LABEL=y', 'LV_FONT_UNSCII_8=y',
                    'ZMK_DISPLAY_WORK_QUEUE_DEDICATED=y', 'ZMK_DISPLAY_DEDICATED_THREAD_STACK_SIZE=4096',
                    'MODU_DONGLE_HAS_BATTERY=y']:
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
        self.assertIn('int zmk_widget_dongle_battery_status_init(', text)
        self.assertNotIn('zmk_display_status_screen(', text)
        self.assertIn('<zmk/events/battery_state_changed.h>', text)
        self.assertIn('info.state != BT_CONN_STATE_CONNECTED', text)
        self.assertIn('info.role != BT_CONN_ROLE_CENTRAL', text)
        self.assertIn('peripheral_slot_index_for_conn(conn)', text)

    def test_no_lvgl_in_event_or_bluetooth_callbacks(self):
        text = (MODULE/'src/dongle_status.c').read_text()
        callbacks = text[text.index('static int status_event('):text.index('/* Fixed objects:')]
        self.assertNotRegex(callbacks, r'\blv_\w+\s*\(')
        self.assertIn('k_spin_lock(&state_lock)', callbacks)
        self.assertIn('return ZMK_EV_EVENT_BUBBLE;', callbacks)
        self.assertIn('lv_timer_create(refresh_batteries, 500, NULL)', text)

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
        self.assertIn('CONFIG_MODU_DONGLE_HAS_BATTERY', text)
        self.assertNotIn('make_mac_mark', text)
        self.assertNotIn('"MAC v3"', text)
        self.assertIn('"D  --%%"', text)
        self.assertIn('dongle_battery_valid', text)

    def test_restore_original_module_and_requested_options(self):
        manifest = (ROOT/'config/west.yml').read_text()
        self.assertIn('name: zmk-dongle-display', manifest)
        cfg = (SHIELD/'modu_dongle_oled.conf').read_text()
        for line in ['CONFIG_ZMK_DONGLE_DISPLAY_MAC_MODIFIERS=y',
                     'CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY=y',
                     'CONFIG_LV_USE_THEME_MONO=y', 'CONFIG_LV_Z_FULL_REFRESH=y']:
            self.assertIn(line, cfg)
        cm = (MODULE/'original_display.cmake').read_text()
        for file in ['custom_status_screen.c', 'modifiers.c', 'modifiers_sym.c',
                     'output_status.c', 'bongo_cat.c', 'layer_status.c']:
            self.assertIn(file, cm)
        self.assertIn('CONFIG_ZMK_BATTERY_REPORTING', cm)
        self.assertIn('ev ? ev->indicators : 0', cm)
        # No duplicate upstream battery implementation is linked.
        self.assertNotIn('zephyr_library_sources("${_dd}/widgets/battery_status.c")', cm)

    def test_original_display_cmake_adapter(self):
        # Minimal SOURCE FIXTURE to exercise CMake preparation, not an upstream
        # checkout and not a firmware compile. Test rejects LVGL 8 input.
        cmake = shutil.which('cmake')
        self.assertIsNotNone(cmake)
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            dd = tmp/'upstream/boards/shields/dongle_display'
            (dd/'widgets').mkdir(parents=True)
            (dd/'widgets/modifiers_sym.c').write_text('LV_COLOR_FORMAT_I1')
            (dd/'widgets/modifiers.c').write_text('CONFIG_ZMK_DONGLE_DISPLAY_MAC_MODIFIERS cmd_icon opt_icon')
            (dd/'custom_status_screen.c').write_text('#if IS_ENABLED(CONFIG_ZMK_BATTERY)\nzmk_widget_dongle_battery_status_init();\n#endif\n#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)\n/* keep existing spelling */\n#endif\n')
            (dd/'widgets/hid_indicators.c').write_text('.hid_indicators = ev->indicators,')
            script = tmp/'check.cmake'
            script.write_text(f"""
cmake_minimum_required(VERSION 3.20)
set(ZEPHYR_ZMK_DONGLE_DISPLAY_MODULE_DIR "{tmp/'upstream'}")
set(PROJECT_BINARY_DIR "{tmp/'build'}")
set(CONFIG_ZMK_HID_INDICATORS y)
set(CONFIG_ZMK_DONGLE_DISPLAY_MODIFIERS y)
set(CONFIG_ZMK_DONGLE_DISPLAY_LAYER y)
set(CONFIG_ZMK_DONGLE_DISPLAY_BONGO_CAT y)
function(zephyr_library_include_directories)
endfunction()
function(zephyr_library_sources)
endfunction()
include("{MODULE/'original_display.cmake'}")
""")
            proc = subprocess.run([cmake, '-P', str(script)], capture_output=True, text=True)
            self.assertEqual(proc.returncode, 0, proc.stdout+proc.stderr)
            generated = tmp/'build/modu-original-display'
            text = (generated/'custom_status_screen.c').read_text()
            self.assertNotIn('CONFIG_ZMK_BATTERY)', text)
            self.assertNotIn('REPORTING_REPORTING', text)
            self.assertEqual(text.count('CONFIG_ZMK_BATTERY_REPORTING'), 2)
            self.assertIn('ev ? ev->indicators : 0', (generated/'hid_indicators.c').read_text())
            (dd/'widgets/modifiers_sym.c').write_text('LV_IMG_CF_INDEXED_1BIT')
            proc = subprocess.run([cmake, '-P', str(script)], capture_output=True, text=True)
            self.assertNotEqual(proc.returncode, 0)
            self.assertIn('LVGL 9', proc.stderr)

if __name__ == '__main__':
    unittest.main(verbosity=2)
