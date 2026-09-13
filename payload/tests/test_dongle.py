"""Source/packaging regressions. These do not run embedded firmware."""
import hashlib
import json
import re
import shutil
import struct
import sys
import tempfile
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from validate import checks, Invalid, keymap_info, coordinates, EXPECTED, SHIELD, strip_comments, close_brace
from package_firmware import package_firmware, _single_input, PackageError, TARGETS
from verify_uf2 import validate_uf2, Uf2ValidationError

class RepoTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name)/'repo'
        shutil.copytree(ROOT,self.root,ignore=shutil.ignore_patterns('__pycache__'))
        # Mutate a fixed test fixture, NOT the user's editable keymap.
        shutil.copy2(ROOT/'reference/validation-fixture.keymap', self.root/'config/modu.keymap')
    def replace(self,path,a,b):
        p=self.root/path;s=p.read_text();self.assertIn(a,s);p.write_text(s.replace(a,b))
    def test_complete_repo(self): self.assertEqual(checks(self.root)['result'],'PASS')
    def test_editor_can_rename_mouse(self):
        self.replace('config/modu.keymap','mouse {','pointer_mode {');checks(self.root)
    def test_editor_can_remove_mouse_combo(self):
        p=self.root/'config/modu.keymap';t=p.read_text();s=t.index('        mouse {');e=close_brace(t,t.index('{',s))+2
        p.write_text(t[:s]+t[e:]);checks(self.root)
    def test_editor_can_move_game_keys(self):
        self.replace('config/modu.keymap','&kp LEFT                &kp DOWN                &kp RIGHT','&kp DOWN                &kp LEFT                &kp RIGHT');checks(self.root)
    def test_editor_can_rename_layers(self):
        self.replace('config/modu.keymap','default_layer {','typing {');self.replace('config/modu.keymap','lower_layer {','navigation {');checks(self.root)
    def test_editor_can_disable_optional_position_66(self):
        p=self.root/'config/modu.keymap';t=p.read_text();t=t.replace('&lt L_FN DEL            &trans','&lt L_FN DEL            &none',1);p.write_text(t);checks(self.root)
    def test_editor_can_rename_boot_nodes_consistently(self):
        self.replace('config/modu.keymap','boot135','my_boot_left');checks(self.root)
    def test_reject_stock_boot_combo(self):
        self.replace('config/modu.keymap','bindings = <&boot135>;','bindings = <&bootloader>;')
        with self.assertRaises(Invalid):checks(self.root)
    def test_reject_boot_in_game(self):
        self.replace('config/modu.keymap','layers = <L_BASE>;','layers = <L_BASE L_GAME>;')
        with self.assertRaises(Invalid):checks(self.root)
    def test_reject_wrong_digit(self):
        self.replace('config/modu.keymap','&kp N3','&kp N2')
        with self.assertRaises(Invalid):checks(self.root)
    def test_reject_missing_key(self):
        self.replace('config/modu.keymap','&kp N3','')
        with self.assertRaises(Invalid):checks(self.root)
    def test_reject_duplicate_trackball_channel(self):
        self.replace(SHIELD+'/modu-inputs.dtsi','reg = <1>;','reg = <0>;')
        with self.assertRaises(Invalid):checks(self.root)
    def test_reject_left_still_central(self):
        self.replace(SHIELD+'/modu_peripheral_left.conf','CONFIG_ZMK_SPLIT_ROLE_CENTRAL=n','CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y')
        with self.assertRaises(Invalid):checks(self.root)
    def test_reject_old_hid_route(self):
        self.replace(SHIELD+'/modu_peripheral_left.overlay','&trackball_listener { status = "disabled"; };','&trackball_listener { status = "okay"; };')
        with self.assertRaises(Invalid):checks(self.root)
    def test_reject_hardware_overwrite(self):
        self.replace('vendor/modu-c-firmware/modu-module/boards/shields/modu/modu.dtsi','cpi = <600>;','cpi = <1200>;')
        with self.assertRaises(Invalid):checks(self.root)
    def test_reject_missing_dongle(self):
        self.replace('build.yaml','artifact-name: modu_dongle','artifact-name: corne')
        with self.assertRaises(Invalid):checks(self.root)
    def test_reject_missing_local_module(self):
        self.replace('build.yaml','/local-modules/obrey-combo-boot','/local-modules/wrong')
        with self.assertRaises(Invalid):checks(self.root)
    def test_historical_fixture_matches_agreed(self):
        self.assertEqual(keymap_info(ROOT/'reference/previous-five-layer.keymap')[3],keymap_info(ROOT/'reference/validation-fixture.keymap')[3])
    def test_actual_user_keymap_has_valid_configuration(self):
        # Functional safety checks only: the user may edit ordinary bindings/combos.
        self.assertEqual(checks(ROOT)['result'], 'PASS')
    def test_peripheral_number_source_coordinates(self):
        self.assertEqual([EXPECTED[i] for i in [1,3,5]],[(0,1),(0,3),(0,5)])
        self.assertEqual([EXPECTED[i] for i in [6,8,10]],[(0,6),(0,8),(0,10)])
    def test_right_fourth_direct_is_real_input_definition(self):
        t=(ROOT/'vendor/modu-c-firmware/modu-module/boards/shields/modu/modu_right.overlay').read_text()
        self.assertIn('<&gpio1  8 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>',t)
        self.assertEqual(EXPECTED[66],(5,9))

# Simulate DTS textual overlay merge of status/reg/device properties.
# This validates our relative includes/route intent, not Zephyr bindings or SoC headers.
def flatten(path):
    t=path.read_text()
    return re.sub(r'#include\s+"([^"]+)"',lambda m:flatten((path.parent/m[1]).resolve()),t)
def input_states(path):
    text=strip_comments(flatten(path));nodes={}
    pat=r'(?P<label>\w+)\s*:\s*(?P<name>[\w,@-]+)\s*\{|&(?P<ref>\w+)\s*\{'
    for m in re.finditer(pat,text):
        op=text.index('{',m.start());body=text[op+1:close_brace(text,op)]
        key=m['label'] or m['ref']
        n=nodes.setdefault(key,{})
        for p in ['compatible','status','reg','device']:
            mm=re.search(rf'(?<![\w-]){p}\s*=\s*(<[^>]*>|"[^"]*")\s*;',body)
            if mm:n[p]=mm[1]
    return {k:v for k,v in nodes.items() if v.get('compatible')=='"zmk,input-split"' and v.get('status','"okay"')=='"okay"'}
class InputRouteTests(unittest.TestCase):
    def test_left_only_channel0(self):
        d=input_states(ROOT/SHIELD/'modu_peripheral_left.overlay')
        self.assertEqual(list(d),['modu_left_split']);self.assertEqual(d['modu_left_split']['reg'],'<0>');self.assertEqual(d['modu_left_split']['device'],'<&trackball>')
    def test_right_only_channel1(self):
        d=input_states(ROOT/SHIELD/'modu_peripheral_right.overlay')
        self.assertEqual(list(d),['modu_right_split']);self.assertEqual(d['modu_right_split']['reg'],'<1>');self.assertEqual(d['modu_right_split']['device'],'<&trackball>')
    def test_dongle_both_without_local_sensor(self):
        d=input_states(ROOT/SHIELD/'modu_dongle.overlay')
        self.assertEqual(set(d),{'modu_left_split','modu_right_split'});self.assertTrue(all('device' not in v for v in d.values()))
    def test_all_layouts_equal(self):
        for f in ['modu_dongle.overlay','modu_peripheral_left.overlay','modu_peripheral_right.overlay']:
            self.assertEqual(coordinates(flatten(ROOT/SHIELD/f)),EXPECTED)

def uf2(value,addr=0x26000):
    b=bytearray(512);struct.pack_into('<8I',b,0,0x0A324655,0x9E5D5157,0x2000,addr,256,0,1,0xADA52840)
    b[32:288]=bytes([value])*256;struct.pack_into('<I',b,508,0x0AB16F30);return bytes(b)
class PackagingTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.root=Path(self.tmp.name)
        self.inputs=self.root/'in';self.inputs.mkdir();self.output=self.root/'out'
        for i,t in enumerate(TARGETS): (self.inputs/(t+'.uf2')).write_bytes(uf2(i+1))
    def package(self):return package_firmware(self.inputs,ROOT/'vendor/modu-c-firmware/tools/uf2/uf2conv.py',self.output)
    def test_all_five(self):self.assertEqual(len(self.package()),5)
    def test_missing_fails(self):
        (self.inputs/'modu_dongle.uf2').unlink()
        with self.assertRaises(PackageError):self.package()
    def test_ambiguous_fails(self):
        d=self.inputs/'nested';d.mkdir();(d/'modu_dongle.uf2').write_bytes(uf2(8))
        with self.assertRaises(PackageError):self.package()
    def test_no_bootloader_overwrite(self):
        (self.inputs/'modu_dongle.uf2').write_bytes(uf2(3,0xf4000))
        with self.assertRaises(PackageError):self.package()
    def test_no_settings_overwrite(self):
        (self.inputs/'modu_dongle.uf2').write_bytes(uf2(3,0xec000))
        with self.assertRaises(PackageError):self.package()
    def test_wrong_family(self):
        p=self.inputs/'modu_dongle.uf2';b=bytearray(p.read_bytes());struct.pack_into('<I',b,28,0);p.write_bytes(b)
        with self.assertRaises(PackageError):self.package()
    def test_normal_images_not_same(self):
        (self.inputs/'modu_dongle.uf2').write_bytes((self.inputs/'modu_left_dongle.uf2').read_bytes())
        with self.assertRaises(PackageError):self.package()
    def test_bad_magic(self):
        (self.inputs/'modu_dongle.uf2').write_bytes(bytes(512))
        with self.assertRaises(PackageError):self.package()
    def test_hex_conversion_real_converter(self):
        p=self.inputs/'modu_dongle.uf2';p.unlink()
        # Two data bytes at 0x26000, with valid Intel-HEX checksums.
        def rec(addr,kind,data):
            b=bytes([len(data),addr>>8,addr&255,kind])+data
            return ':'+(b+bytes([(-sum(b))&255])).hex().upper()+'\n'
        (self.inputs/'modu_dongle.hex').write_text(rec(0,4,b'\x00\x02')+rec(0x6000,0,b'\x88\x77')+rec(0,1,b''))
        self.assertEqual(len(self.package()),5)

if __name__=='__main__':unittest.main()
