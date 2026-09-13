#!/usr/bin/env python3
"""Apply the narrowly scoped MODU-C dongle EOL checker + Tab column fix.

Run from any folder. No network, flashing, Git commits or pushes are performed.
Modified files are backed up beside the target repository, not inside it.
"""
from __future__ import annotations
import argparse
from datetime import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
PAYLOAD = HERE/'payload'
sys.path.insert(0, str(PAYLOAD/'scripts'))
from move_tab_column import relocate_tab
from validate import Invalid

FILES = (
    'scripts/validate.py',
    'scripts/move_tab_column.py',
    'tests/test_dongle.py',
    'tests/test_line_endings.py',
    'reference/hardware-normalized-sha256.json',
    'reference/validation-fixture.keymap',
)
ATTR_BEGIN = '# BEGIN OBREY EOL FIX'
ATTR_END = '# END OBREY EOL FIX'
ATTR_BLOCK = '''# BEGIN OBREY EOL FIX
# Consistent text checkouts. Validator independently verifies upstream content.
/vendor/modu-c-firmware/** text=auto eol=lf
/scripts/*.py text eol=lf
/tests/*.py text eol=lf
/reference/*.json text eol=lf
/reference/*.keymap text eol=lf
/config/*.keymap text eol=lf
# END OBREY EOL FIX
'''

def preflight_vendor(root: Path) -> None:
    current = json.loads((root/'reference/hardware-sha256.json').read_text(encoding='utf-8-sig'))
    approved = json.loads((PAYLOAD/'reference/hardware-normalized-sha256.json').read_text(encoding='utf-8-sig'))['files']
    if set(current) != set(approved):
        raise Invalid('This patch is for the supplied 45-file MODU-C dongle vendor snapshot, not another repository.')
    for rel, digest in current.items():
        entry = approved[rel]
        if entry['source_sha256'] != digest:
            raise Invalid('Different original hardware snapshot: ' + rel)
        data = (root/'vendor/modu-c-firmware'/rel).read_bytes()
        if hashlib.sha256(data).hexdigest() == digest:
            continue
        data.decode('utf-8')
        if b'\x00' in data or hashlib.sha256(data.replace(b'\r\n', b'\n')).hexdigest() != entry['lf_sha256']:
            raise Invalid('REAL hardware content difference, not only line endings: ' + rel + '. Nothing was changed.')

def updated_attributes(old: bytes | None) -> bytes:
    text = old.decode('utf-8') if old is not None else ''
    if ATTR_BEGIN in text:
        if ATTR_END not in text:
            raise Invalid('Incomplete prior EOL marker in .gitattributes; refusing to overwrite it.')
        start = text.index(ATTR_BEGIN)
        end = text.index(ATTR_END, start) + len(ATTR_END)
        # Preserve unrelated rules; replace only our named block.
        while end < len(text) and text[end] in '\r\n':
            end += 1
        text = text[:start] + text[end:]
    text = text.rstrip('\r\n')
    return ((text + '\n\n' if text else '') + ATTR_BLOCK).encode('utf-8')

def apply(root: Path, yes: bool = False, keep_keymap: bool = False) -> int:
    root = root.expanduser().resolve()
    if not (root/'local-modules/modu-dongle/zephyr/module.yml').is_file():
        raise Invalid('Select the DONGLE repository folder containing config, local-modules, vendor and build.yaml.')
    if root == HERE or root in HERE.parents:
        # Allowed to keep the patch inside the repo, but never add it with git add .
        print('Note: add only the listed files to Git, not the patch folder itself.')
    preflight_vendor(root)
    keymap_path = root/'config/modu.keymap'
    keymap_bytes = keymap_path.read_bytes()
    if keep_keymap:
        new_keymap = keymap_bytes
        moves = []
    else:
        text, moves = relocate_tab(keymap_bytes.decode('utf-8'))
        new_keymap = text.encode('utf-8')
    proposed = {relative:(PAYLOAD/relative).read_bytes() for relative in FILES}
    attributes = root/'.gitattributes'
    proposed['.gitattributes'] = updated_attributes(attributes.read_bytes() if attributes.exists() else None)
    if new_keymap != keymap_bytes:
        proposed['config/modu.keymap'] = new_keymap
    previous: dict[str, bytes | None] = {}
    for rel in proposed:
        path = root/rel
        if path.is_symlink() or (path.exists() and not path.is_file()):
            raise Invalid('Refusing non-regular target: ' + rel)
        previous[rel] = path.read_bytes() if path.exists() else None
    changes = {rel:b for rel,b in proposed.items() if previous[rel] != b}
    print('Target:', root)
    print('Files to update:')
    for rel in changes: print('  ' + rel)
    for m in moves: print(f"  Base[{m['position']}]: {m['before']} -> {m['after']}")
    if not moves:
        print('Keymap: kept unchanged (already moved, or --keep-keymap).')
    if changes and not yes and input('Back up and apply? Type Y: ').strip().lower() != 'y':
        print('Cancelled. No files changed.')
        return 0
    backup = None
    if changes:
        backup = root.parent/(root.name + '-backup-eol-tab-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
        backup.mkdir()
        for rel in changes:
            old = previous[rel]
            if old is not None:
                dest = backup/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(old)
        (backup/'restore-info.json').write_text(json.dumps({
            'repository':str(root), 'files':list(changes),
            'new_files_to_remove_when_restoring':[r for r in changes if previous[r] is None],
        },ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    try:
        # A concurrent editor save must not be silently overwritten.
        for rel in changes:
            path = root/rel
            current = path.read_bytes() if path.exists() else None
            if current != previous[rel]: raise Invalid('File changed while preparing patch: ' + rel)
        for rel,data in changes.items():
            p=root/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        env = dict(os.environ, PYTHONIOENCODING='utf-8', PYTHONUTF8='1')
        commands = (
            [sys.executable,'-X','utf8','scripts/validate.py'],
            [sys.executable,'-X','utf8','-m','unittest','discover','-s','tests','-p','test_*.py'],
        )
        for command in commands:
            result = subprocess.run(command,cwd=root,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding='utf-8',errors='replace')
            if backup is not None:
                with (backup/'checks.log').open('a',encoding='utf-8') as log:
                    log.write('$ '+' '.join(command)+'\n'+result.stdout+'\n')
            if result.returncode:
                print(result.stdout)
                raise Invalid('Local check failed. Patched files will be restored from the backup.')
            print('PASS:', ' '.join(command[3:]))
    except BaseException:
        for rel in changes:
            p=root/rel
            # Do not overwrite a new edit made after our own write.
            if p.exists() and p.read_bytes() != changes[rel]:
                continue
            if previous[rel] is None:
                if p.exists():p.unlink()
            else:p.write_bytes(previous[rel])
        if backup: print('Backup / failure log:',backup)
        raise
    print('\nPatch checks passed. Firmware has NOT been built or flashed.')
    if backup: print('Backup:',backup)
    stage=list(FILES)+['.gitattributes']
    if not keep_keymap:stage.append('config/modu.keymap')
    print('\nIn the target repository, review and run:')
    print('git diff --stat')
    print('git add '+' '.join(stage))
    print('git commit -m "Fix vendor line-ending validation and move Tab above shortcuts"')
    print('git push')
    print('Then inspect the NEW commit in GitHub Actions.')
    return 0

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo',type=Path)
    ap.add_argument('--yes',action='store_true')
    ap.add_argument('--keep-keymap',action='store_true',help='Apply the checker fix only; do not move Tab.')
    args=ap.parse_args()
    try:
        if args.repo is None:
            raw=input('Paste the LOCAL DONGLE repository folder path (not a URL): ').strip().strip('"')
            if not raw:raise Invalid('No repository selected.')
            args.repo=Path(raw)
        return apply(args.repo,args.yes,args.keep_keymap)
    except (Invalid,OSError,ValueError,UnicodeError) as exc:
        print('STOP:',exc,file=sys.stderr);return 1
if __name__=='__main__':raise SystemExit(main())
