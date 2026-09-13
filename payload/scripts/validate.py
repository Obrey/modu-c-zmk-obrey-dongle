#!/usr/bin/env python3
"""Local source checks, not a substitute for a Zephyr/ZMK build.

Ordinary layer/combination node names and non-boot key choices may be edited.
Only physical shape, direct boot routing and dongle-critical setup are checked.
No external Python package is required.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import re
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ZMK_REV = '641514a97db345f499dd50b0360e594270f008fe'
EXPECTED = [(r,c) for r in range(5) for c in range(12)] + [(5,0),(5,1),(5,2),(5,6),(5,7),(5,8),(5,9)]
TARGETS = (
    ('ms88sf3/nrf52840','modu_peripheral_left','modu_left_dongle'),
    ('ms88sf3/nrf52840','modu_peripheral_right','modu_right_dongle'),
    ('nice_nano@2.0.0//zmk','modu_dongle','modu_dongle'),
    ('ms88sf3/nrf52840','settings_reset','modu_settings_reset'),
    ('nice_nano@2.0.0//zmk','settings_reset','dongle_settings_reset'),
)
SHIELD = 'local-modules/modu-dongle/boards/shields/modu_dongle'

class Invalid(ValueError): pass

def need(ok: bool, message: str):
    if not ok: raise Invalid(message)

def strip_comments(text: str) -> str:
    # Keep strings intact, including URLs or comment-like text in display names.
    return re.sub(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*',
                  lambda m: m[0] if m[0].startswith('"') else ' ', text, flags=re.S)

def close_brace(text: str, start: int) -> int:
    depth=0; quoted=False; escape=False
    for i in range(start,len(text)):
        ch=text[i]
        if quoted:
            if escape: escape=False
            elif ch=='\\': escape=True
            elif ch=='"': quoted=False
            continue
        if ch=='"': quoted=True
        elif ch=='{': depth+=1
        elif ch=='}':
            depth-=1
            if depth==0: return i
    raise Invalid('Unbalanced DTS braces')

@dataclass
class Node:
    name: str
    label: str | None
    body: str

def children(body: str) -> list[Node]:
    pattern=re.compile(r'(?:([A-Za-z_]\w*)\s*:\s*)?([A-Za-z_][\w,@-]*|/)\s*\{')
    result=[]; pos=0
    while m:=pattern.search(body,pos):
        op=body.index('{',m.start()); cl=close_brace(body,op)
        result.append(Node(m[2],m[1],body[op+1:cl]));pos=cl+1
    return result

def all_nodes(text: str) -> list[Node]:
    result=[]
    for node in children(text):
        result.append(node);result.extend(all_nodes(node.body))
    return result

def cells(body: str, prop: str) -> str | None:
    m=re.search(rf'(?<![\w-]){re.escape(prop)}\s*=\s*<(.*?)>\s*;',body,re.S)
    return m[1] if m else None

def bindings(body: str) -> list[str]:
    value=cells(body,'bindings')
    if value is None:return []
    return [' '.join(m.group(0).split()) for m in re.finditer(r'&[A-Za-z_]\w*[^&]*',value)]

def integers(value: str | None, defines: dict[str,int]) -> list[int]:
    need(value is not None,'Missing required numeric property')
    result=[]
    for token in value.split():
        if token in defines: result.append(defines[token])
        else:
            try: result.append(int(token,0))
            except ValueError: raise Invalid(f'Cannot resolve integer {token!r}')
    return result

def keymap_info(path: Path):
    text=strip_comments(path.read_text(encoding='utf-8-sig'))
    defines={m[1]:int(m[2],0) for m in re.finditer(r'(?m)^\s*#define\s+(\w+)\s+(0[xX][0-9a-fA-F]+|\d+)\s*$',text)}
    nodes=all_nodes(text)
    maps=[n for n in nodes if re.search(r'compatible\s*=\s*"zmk,keymap"',n.body) and n.name=='keymap']
    need(len(maps)==1,'Expected exactly one keymap node')
    layers=[(n.name, bindings(n.body)) for n in children(maps[0].body) if cells(n.body,'bindings') is not None]
    return text,defines,nodes,layers

def coordinates(text: str):
    return [(int(a),int(b)) for a,b in re.findall(r'RC\(\s*(\d+)\s*,\s*(\d+)\s*\)',strip_comments(text))]

def final_status(overlay: str,label: str):
    values=re.findall(rf'&{label}\s*\{{\s*status\s*=\s*"([^"]+)"\s*;',overlay)
    return values[-1] if values else None

def verify_vendor_content(root: Path) -> dict[str, int]:
    """Check uploaded upstream content; accept only LF/CRLF representation differences.

    Expected LF digests are precomputed from the original uploaded archive. They
    are NEVER learned from or refreshed using the user's working directory.
    Spaces, tabs, a BOM, a removed final newline, and actual code edits still fail.
    """
    reference = root / 'reference'
    originals = json.loads((reference / 'hardware-sha256.json').read_text(encoding='utf-8-sig'))
    normalized = json.loads((reference / 'hardware-normalized-sha256.json').read_text(encoding='utf-8-sig'))
    need(normalized.get('schema_version') == 1, 'Invalid vendor normalization manifest version')
    need(normalized.get('normalization') == 'utf8-crlf-to-lf', 'Unsupported vendor normalization')
    records = normalized.get('files', {})
    need(set(records) == set(originals), 'Vendor manifests disagree on file list')
    exact = 0
    eol_only = 0
    for relative, expected_raw in originals.items():
        path = root / 'vendor/modu-c-firmware' / relative
        record = records[relative]
        need(record.get('source_sha256') == expected_raw,
             'Vendor manifests disagree on original hash: ' + relative)
        need(path.is_file(), 'Vendored hardware missing: ' + relative)
        data = path.read_bytes()
        if hashlib.sha256(data).hexdigest() == expected_raw:
            exact += 1
            continue
        try:
            data.decode('utf-8')
        except UnicodeDecodeError as exc:
            raise Invalid('Vendored hardware is no longer UTF-8 text: ' + relative) from exc
        canonical = data.replace(b'\r\n', b'\n')
        matches = b'\x00' not in data and hashlib.sha256(canonical).hexdigest() == record.get('lf_sha256')
        need(matches, 'Vendored hardware CONTENT changed (not just line endings): ' + relative)
        eol_only += 1
    return {'vendor_content_verified_files': exact + eol_only,
            'vendor_exact_byte_matches': exact,
            'vendor_line_ending_only_differences': eol_only}

def checks(root: Path=ROOT) -> dict:
    read=lambda p:(root/p).read_text(encoding='utf-8-sig')
    _,defs,nodes,layers=keymap_info(root/'config/modu.keymap')
    need(len(layers)>=5,'Expected the five existing modes (additional modes allowed)')
    need(all(len(b)==67 for _,b in layers),'Each layer must have 67 bindings')
    need(all(layers[0][1][i]=='&none' for i in range(51,57)),'Nonphysical matrix slots 51..56 must remain &none')
    for n in ('config/info.json','config/modu.json'):
        meta=json.loads(read(n));pts=meta['layouts']['default_transform']['layout']
        need([(p['row'],p['col']) for p in pts]==EXPECTED,f'{n}: wrong coordinate order')
    source=read('vendor/modu-c-firmware/modu-module/boards/shields/modu/modu.dtsi')
    layout=read(SHIELD+'/modu-layout.dtsi')
    need(coordinates(source)==EXPECTED,'Vendor transform differs from supported 67-slot map')
    need(coordinates(layout)==EXPECTED,'Dongle transform does not match both peripherals')
    # A user may rename any combo or behavior; match functional type and positions.
    labels={n.label:n for n in nodes if n.label}
    containers=[n for n in nodes if n.name=='combos']
    need(len(containers)==1,'Expected one combos container')
    combos=children(containers[0].body)
    for positions,digits in [([1,3,5],['N1','N3','N5']),([6,8,10],['N6','N8','N0'])]:
        found=[n for n in combos if cells(n.body,'key-positions') is not None and set(integers(cells(n.body,'key-positions'),defs))==set(positions)]
        need(len(found)==1,f'Expected one direct combo at {positions}')
        combo=found[0];action=bindings(combo.body)
        need(len(action)==1 and action[0].startswith('&'),'Missing boot combo behavior')
        ref=action[0][1:]
        need(ref in labels,f'Combo refers to undefined behavior {ref}')
        behavior=labels[ref]
        need('"zmk,behavior-obrey-combo-boot"' in behavior.body,'Boot combos must use source-aware behavior, not stock &bootloader')
        need(integers(cells(combo.body,'layers'),defs)==[0],'Boot combos must be Base-only')
        need(integers(cells(behavior.body,'key-positions'),defs)==positions,'Boot behavior guard positions mismatch')
        need(bindings(behavior.body)==['&bootloader'],'Guard must forward only the bootloader action')
        window=integers(cells(combo.body,'timeout-ms'),defs)[0]
        need(window==integers(cells(behavior.body,'window-ms'),defs)[0],'Combo and guard windows differ')
        need(30<=window<=200,'Unsafe/invalid direct combo time window')
        need(integers(cells(combo.body,'require-prior-idle-ms'),defs)[0]>=300,'Boot combos need at least 300 ms idle protection')
        for pos,digit in zip(positions,digits):
            aliases={f'&kp {digit}',f'&kp NUMBER_{digit[1:]}' }
            need(layers[0][1][pos] in aliases,f'Base position {pos} is no longer digit {digit}; update boot chord intentionally')
    # Do not enforce any ordinary combo name (e.g. mouse) or game key location.
    inp=read(SHIELD+'/modu-inputs.dtsi')
    for side,channel in [('left',0),('right',1)]:
        need(re.search(rf'modu_{side}_split@{channel}\s*\{{.*?reg\s*=\s*<{channel}>',inp,re.S) is not None,'Missing distinct trackball channel')
        ov=read(SHIELD+f'/modu_peripheral_{side}.overlay')
        for label in ['trackball_listener','peripheral_trackball_listener','trackball_split']:
            need(final_status(ov,label)=='disabled',f'{side}: old trackball route is still enabled')
        need(re.search(rf'&modu_{side}_split\s*\{{\s*status\s*=\s*"okay";\s*device\s*=\s*<&trackball>',ov,re.S) is not None,f'{side}: sensor not forwarded')
        for inc in re.findall(r'#include\s+"([^"]+)"',ov):
            need((root/SHIELD/inc).resolve().is_file(),f'Missing local include {inc}')
        conf=read(SHIELD+f'/modu_peripheral_{side}.conf')
        for value in ['CONFIG_ZMK_SPLIT_ROLE_CENTRAL=n','CONFIG_ZMK_USB=n','CONFIG_PMW3610_ALT=y','CONFIG_MODU_ALT_THUMB_KSCAN=y']:
            need(value in conf,f'{side}: missing {value}')
    dongle=read(SHIELD+'/modu_dongle.overlay')
    for label in ['modu_left_split','modu_right_split','modu_left_listener','modu_right_listener']:
        need(final_status(dongle,label)=='okay',f'Dongle receiver {label} is not enabled')
    need('zmk,kscan = &mock_kscan' in dongle,'Dongle must have mock scan')
    need('modu.dtsi' not in dongle,'Dongle must not import physical MODU GPIOs')
    conf=read(SHIELD+'/modu_dongle.conf')
    for flag in ['CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y','CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS=2','CONFIG_ZMK_POINTING=y','CONFIG_ZMK_DISPLAY=y','CONFIG_ZMK_BATTERY_REPORTING=n','CONFIG_ZMK_USB=y']:
        need(flag in conf,f'Missing dongle flag {flag}')
    build=read('build.yaml')
    rows=re.findall(r'(?m)^  - board:\s*(\S+)\n    shield:\s*(\S+)\n    cmake-args:\s*>-\n      ([^\n]+)\n    artifact-name:\s*(\S+)',build)
    need([(b,s,a) for b,s,_,a in rows]==list(TARGETS),'Wrong normal/reset build matrix')
    mods=['vendor/modu-c-firmware/modu-module','vendor/modu-c-firmware/zmk-pmw3610-driver','local-modules/modu-dongle','local-modules/obrey-combo-boot']
    for _,shield,args,_ in rows:
        for module in mods:
            need('${GITHUB_WORKSPACE}/'+module in args,'Missing local module in build target')
            need((root/module/'zephyr/module.yml').is_file(),'Missing Zephyr module manifest')
        need(('KEYMAP_FILE' in args)==(shield!='settings_reset'),'Reset image must not reuse normal keymap')
    need(ZMK_REV in read('config/west.yml') and ZMK_REV in read('.github/workflows/build.yml'),'ZMK manifest/workflow pins disagree')
    need('name: modu-c-firmware' not in read('config/west.yml'),'Do not mix remote hardware pin with vendored upload')
    vendor_result = verify_vendor_content(root)
    return {'result':'PASS','layer_count':len(layers),'binding_count':sum(len(b) for _,b in layers),
            'matching_transform_positions':67,'build_targets':5,'trackball_channels':[0,1],
            **vendor_result,'firmware_compiled':False,'hardware_tested':False}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--root',type=Path,default=ROOT);a=ap.parse_args()
    try: print(json.dumps(checks(a.root),ensure_ascii=False,indent=2))
    except (Invalid,OSError,KeyError,ValueError) as e: raise SystemExit(f'ERROR: {e}')
if __name__=='__main__':main()
