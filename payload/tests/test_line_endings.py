"""Regressions for Git line endings and keymap edits (no firmware/hardware tests)."""
from pathlib import Path
import hashlib
import json
import shutil
import sys
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'scripts'))
from validate import verify_vendor_content, Invalid, keymap_info, checks
from move_tab_column import relocate_tab, base_binding_spans

class VendorLineEndingTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)/'repo'
        shutil.copytree(ROOT/'vendor', self.root/'vendor')
        shutil.copytree(ROOT/'reference', self.root/'reference')
        self.vendor = self.root/'vendor/modu-c-firmware'
    def normalize_all(self, ending):
        for relative in json.loads((self.root/'reference/hardware-sha256.json').read_text()):
            p = self.vendor/relative
            content = p.read_bytes().replace(b'\r\n',b'\n')
            if ending == b'\r\n': content = content.replace(b'\n', b'\r\n')
            p.write_bytes(content)
    def test_archive_original_passes(self):
        self.assertEqual(verify_vendor_content(self.root)['vendor_content_verified_files'],45)
    def test_kconfig_lf_only_passes(self):
        p=self.vendor/'zmk-pmw3610-driver/Kconfig';p.write_bytes(p.read_bytes().replace(b'\r\n',b'\n'))
        self.assertEqual(verify_vendor_content(self.root)['vendor_content_verified_files'],45)
    def test_every_vendor_file_lf_passes(self):
        self.normalize_all(b'\n');self.assertEqual(verify_vendor_content(self.root)['vendor_content_verified_files'],45)
    def test_every_vendor_file_crlf_passes(self):
        self.normalize_all(b'\r\n');self.assertEqual(verify_vendor_content(self.root)['vendor_content_verified_files'],45)
    def test_originally_mixed_newline_file_lf_passes(self):
        p=self.vendor/'zmk-pmw3610-driver/src/pmw3610.c';p.write_bytes(p.read_bytes().replace(b'\r\n',b'\n'))
        self.assertEqual(verify_vendor_content(self.root)['vendor_content_verified_files'],45)
    def test_actual_code_change_fails_even_with_lf(self):
        self.normalize_all(b'\n');p=self.vendor/'zmk-pmw3610-driver/Kconfig';p.write_bytes(p.read_bytes()+b'\n# unexpected code change\n')
        with self.assertRaisesRegex(Invalid,'CONTENT changed'):verify_vendor_content(self.root)
    def test_final_newline_removed_fails(self):
        p=self.vendor/'zmk-pmw3610-driver/Kconfig';p.write_bytes(p.read_bytes().rstrip(b'\r\n'))
        with self.assertRaises(Invalid):verify_vendor_content(self.root)
    def test_bom_added_fails(self):
        p=self.vendor/'zmk-pmw3610-driver/Kconfig';p.write_bytes(b'\xef\xbb\xbf'+p.read_bytes())
        with self.assertRaises(Invalid):verify_vendor_content(self.root)
    def test_missing_vendor_file_fails(self):
        (self.vendor/'zmk-pmw3610-driver/Kconfig').unlink()
        with self.assertRaises(Invalid):verify_vendor_content(self.root)
    def test_manifests_cannot_be_silently_reanchored(self):
        p=self.root/'reference/hardware-normalized-sha256.json';d=json.loads(p.read_text());d['files']['zmk-pmw3610-driver/Kconfig']['source_sha256']='0'*64;p.write_text(json.dumps(d))
        with self.assertRaisesRegex(Invalid,'manifests disagree'):verify_vendor_content(self.root)

class TabMoveTests(unittest.TestCase):
    def setUp(self):
        self.text=(ROOT/'reference/validation-fixture.keymap').read_text(encoding='utf-8')
    def layers(self,text):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'test.keymap';p.write_text(text,encoding='utf-8');return keymap_info(p)[3]
    def test_four_base_changes_only(self):
        new,changes=relocate_tab(self.text)
        old_layers=self.layers(self.text);new_layers=self.layers(new)
        changed=[(li,ki) for li,((_,a),(_,b)) in enumerate(zip(old_layers,new_layers)) for ki,(x,y) in enumerate(zip(a,b)) if x!=y]
        self.assertEqual(changed,[(0,12),(0,24),(0,36),(0,48)])
        self.assertEqual([new_layers[0][1][p] for p in (12,24,36,48)],['&kp TAB','&mt LG(A) LG(Z)','&mt LG(X) LG(C)','&kp LG(V)'])
        self.assertEqual(len(changes),4)
    def test_user_added_combo_and_ime_preserved(self):
        extra='\n        my_space { key-positions = <27 28>; bindings = <&kp SPACE>; layers = <0>; };\n        my_ime { key-positions = <26 27>; bindings = <&kp RALT>; layers = <0>; };\n'
        text=self.text.replace('compatible = "zmk,combos";', 'compatible = "zmk,combos";'+extra).replace('mouse {','my_pointer {')
        new,_=relocate_tab(text);self.assertIn(extra,new);self.assertIn('my_pointer {',new)
    def test_user_changed_modifier_preserved(self):
        text=self.text.replace('&mt LG(A) LG(Z)','&mt LC(A) LC(Z)')
        new,_=relocate_tab(text);self.assertEqual(base_binding_spans(new)[24][2],'&mt LC(A) LC(Z)')
    def test_second_run_no_change(self):
        first,_=relocate_tab(self.text);second,changes=relocate_tab(first);self.assertEqual(first,second);self.assertEqual(changes,[])
    def test_editor_format_and_renamed_base(self):
        text=self.text.replace('default_layer {','typing {')
        text='\n'.join(line.strip() for line in text.splitlines())
        new,changes=relocate_tab(text);self.assertEqual(len(changes),4);self.assertEqual(base_binding_spans(new)[12][2],'&kp TAB')
    def test_refuse_non_tab_source(self):
        a,b,_=base_binding_spans(self.text)[48];text=self.text[:a]+'&kp ESC'+self.text[b:]
        with self.assertRaises(Invalid):relocate_tab(text)
    def test_bom_and_crlf_preserved(self):
        text='\ufeff'+self.text.replace('\n','\r\n');new,_=relocate_tab(text)
        self.assertTrue(new.startswith('\ufeff'));self.assertEqual(new.count('\r\n'),text.count('\r\n'))
    def test_comment_text_not_parsed_as_bindings(self):
        text=self.text.replace('/* 12..23 */','/* ignore &kp F10; keymap { bindings = <&none>; } */')
        new,changes=relocate_tab(text);self.assertEqual(len(changes),4)

if __name__=='__main__':unittest.main()
