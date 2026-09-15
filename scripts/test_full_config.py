#!/usr/bin/env python3
"""Host-side/static regression tests. These do NOT execute or compile ZMK."""
from __future__ import annotations
import re
import tempfile
import unittest
from pathlib import Path
from package_firmware import TARGETS, package_firmware, PackageError
from verify_uf2 import validate_uf2
from selftest import write_uf2
from validate import EXPECTED_COORDINATES, _parse_build_entries

ROOT = Path(__file__).resolve().parents[1]
SHIELD = ROOT / 'local-modules/modu-dongle/boards/shields/modu_dongle'

def strip(text: str) -> str:
    return re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)

def layers(text: str) -> list[tuple[str, list[str]]]:
    text = strip(text)
    text = text[text.index('keymap {'):]
    result = []
    for m in re.finditer(r'(?m)^\s*(\w+)\s*\{[^{}]*?\bbindings\s*=\s*<(.*?)>\s*;', text, re.S):
        result.append((m.group(1), [' '.join(x.split()) for x in re.findall(r'&[^&]+', m.group(2))]))
    return result

class FullConfigTests(unittest.TestCase):
    def setUp(self):
        self.keymap = (ROOT / 'config/modu.keymap').read_text()
        self.layers = layers(self.keymap)

    def test_nine_layers_67_positions(self):
        self.assertEqual(len(self.layers), 9)
        for name, bindings in self.layers:
            self.assertEqual(len(bindings), 67, name)

    def test_boot_gate_and_confirm_positions(self):
        base = self.layers[0][1]
        self.assertEqual(base[5], '&blt5 L_BOOT_L N5')
        self.assertEqual(base[6], '&blt6 L_BOOT_R N6')
        self.assertEqual(base[1], '&kp N1')
        self.assertEqual(base[10], '&kp N0')
        boot = [(i, [j for j,b in enumerate(bb) if b == '&bootloader']) for i, (_, bb) in enumerate(self.layers)]
        self.assertEqual([b for b in boot if b[1]], [(7,[1]), (8,[10])])
        self.assertEqual(self.layers[7][1][6], '&kp N6')
        self.assertEqual(self.layers[8][1][5], '&kp N5')
        self.assertEqual(dict(self.layers)['game'][5:7], ['&kp N5', '&kp N6'])

    def test_hold_settings(self):
        text = strip(self.keymap)
        self.assertNotRegex(text, r'&(lt|mt)\s*\{')
        for position in [5,6]:
            body = re.search(r'blt'+str(position)+r':\s*\w+\s*\{(.*?)\};', text, re.S).group(1)
            for item in ['tapping-term-ms = <3000>;', 'flavor = "hold-preferred";',
                         f'hold-trigger-key-positions = <{position}>;',
                         'bindings = <&mo>, <&kp>;', 'quick-tap-ms = <200>;',
                         'require-prior-idle-ms = <300>;']:
                self.assertIn(item, body)
            for item in ['hold-while-undecided', 'hold-trigger-on-release', 'retro-tap']:
                self.assertNotIn(item, body)

    def test_no_custom_boot_module_or_old_mouse_include(self):
        self.assertFalse((ROOT / 'local-modules/obrey-combo-boot').exists())
        for file in [ROOT/'config/modu.keymap', ROOT/'build.yaml', ROOT/'config/west.yml']:
            text = strip(file.read_text())
            for token in ['obrey-combo-boot', '&boot135', '&boot680', '<dt-bindings/zmk/mouse.h>']:
                self.assertNotIn(token, text)

    def test_dongle_transform_matches_keyboard_metadata(self):
        text = strip((SHIELD / 'modu-dongle-common.dtsi').read_text())
        coordinates = [(int(a),int(b)) for a,b in re.findall(r'RC\((\d+),(\d+)\)', text)]
        self.assertEqual(coordinates, EXPECTED_COORDINATES)
        self.assertIn('default_transform: keymap_transform_0', text)
        self.assertIn('compatible = "zmk,kscan-mock";', text)
        for token in ['&trackball>','&spi0','orientation-gpios','row-gpios','col-offset']:
            self.assertNotIn(token, text)

    def test_left_forwards_trackball_on_id1_only(self):
        text = strip((ROOT/'config/dongle/modu_left.overlay').read_text())
        self.assertRegex(text, r'&trackball_split\s*\{\s*status = "disabled";')
        self.assertRegex(text, r'trackball_split_left@1\s*\{[^}]*reg = <1>;[^}]*device = <&trackball>;')
        for label in ['trackball_listener','peripheral_trackball_listener']:
            self.assertRegex(text, '&'+label+r'\s*\{\s*status = "disabled";')

    def test_right_forwards_trackball_on_id0_only(self):
        text = strip((ROOT/'config/dongle/modu_right.overlay').read_text())
        self.assertRegex(text, r'&trackball_split\s*\{[^}]*status = "okay";[^}]*device = <&trackball>;')
        self.assertRegex(text, r'trackball_split_left@1\s*\{[^}]*status = "disabled";')
        for label in ['trackball_listener','peripheral_trackball_listener']:
            self.assertRegex(text, '&'+label+r'\s*\{\s*status = "disabled";')

    def test_dongle_receives_both_trackballs(self):
        text = strip((SHIELD/'modu-dongle-common.dtsi').read_text())
        self.assertEqual(re.findall(r'\breg = <(\d+)>;', text), ['0','1'])
        self.assertRegex(text, r'trackball_listener:\s*trackball_listener\s*\{[^}]*device = <&trackball_split_left>;')
        self.assertRegex(text, r'peripheral_trackball_listener:\s*peripheral_trackball_listener\s*\{[^}]*device = <&trackball_split>;')

    def test_oled_keeps_original_chip_and_pins(self):
        text = (SHIELD/'modu_dongle_oled.overlay').read_text()
        for token in ['sinowealth,sh1106', 'NRF_PSEL(TWIM_SDA, 0, 17)', 'NRF_PSEL(TWIM_SCL, 0, 20)', 'reg = <0x3c>;', 'height = <64>;']:
            self.assertIn(token, text)
        self.assertNotIn('runtime-input-processor', text)

    def test_central_connections_and_no_studio_fork(self):
        for name in ['modu_dongle','modu_dongle_oled']:
            text = (SHIELD/f'{name}.conf').read_text()
            for token in ['CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y','CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2','CONFIG_BT_MAX_CONN=7','CONFIG_BT_MAX_PAIRED=7','CONFIG_ZMK_POINTING=y']:
                self.assertIn(token,text)
        west = (ROOT/'config/west.yml').read_text()
        self.assertIn('remote: zmkfirmware',west)
        self.assertNotIn('cormoran',west)

    def test_dongle_battery_event_dependency_for_pinned_zmk(self):
        # At ZMK 641514a, peripheral and local battery event implementations
        # share src/events/battery_state_changed.c. app/CMakeLists.txt only
        # includes that source under CONFIG_ZMK_BATTERY_REPORTING. Enabling
        # fetching while disabling reporting leaves central.c with an
        # undefined raise_zmk_peripheral_battery_state_changed reference.
        # This is a source-configuration regression check, not a ZMK build.
        for name in ['modu_dongle', 'modu_dongle_oled']:
            with self.subTest(shield=name):
                text = (SHIELD / f'{name}.conf').read_text()
                for symbol in [
                    'CONFIG_ZMK_BATTERY_REPORTING',
                    'CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING',
                ]:
                    values = re.findall(
                        rf'(?m)^\s*{re.escape(symbol)}\s*=\s*([^#\s]+)', text
                    )
                    self.assertEqual(values, ['y'],
                        f'{name}: {symbol} must be enabled exactly once')
                entry = next(e for e in _parse_build_entries(
                    (ROOT / 'build.yaml').read_text()) if e['shield'] == name)
                self.assertNotIn('-DCONFIG_ZMK_BATTERY_REPORTING=n', entry['cmake'])

    def test_packaging_names_equal_matrix_names(self):
        names=tuple(e['artifact'] for e in _parse_build_entries((ROOT/'build.yaml').read_text()))
        self.assertEqual(names, TARGETS)
        self.assertEqual(len(set(names)), 6)

    def test_all_six_native_uf2_outputs(self):
        # Synthetic fixtures exist only inside a temporary directory, never in the release.
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); inputs=root/'inputs'; output=root/'outputs'
            for index,name in enumerate(TARGETS):
                write_uf2(inputs/f'{name}.uf2', 13+index*19)
            results=package_firmware(inputs,root/'unused-converter.py',output)
            self.assertEqual({p.name for p in results},{n+'.uf2' for n in TARGETS})
            for p in results: validate_uf2(p)

    def test_missing_dongle_output_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); inputs=root/'inputs'
            for index,name in enumerate(TARGETS):
                if name != 'modu_dongle': write_uf2(inputs/f'{name}.uf2', 7+index*17)
            with self.assertRaises(PackageError):
                package_firmware(inputs,root/'unused.py',root/'outputs')

if __name__ == '__main__': unittest.main(verbosity=2)
