#!/usr/bin/env python3
"""Move the existing Base Tab above the left shortcut column; preserve other edits.

No layers or combos are added. Game, mouse, thumb keys and the home row are not
rewritten. This script only rotates Base binding positions 12,24,36,48.
"""
from __future__ import annotations
import argparse
from datetime import datetime
import difflib
from pathlib import Path
import re
import sys
from validate import Invalid, close_brace

POSITIONS = (12, 24, 36, 48)

def mask_comments(text: str) -> str:
    return re.sub(
        r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*',
        lambda m: m[0] if m[0].startswith('"') else ''.join('\n' if ch == '\n' else ' ' for ch in m[0]),
        text, flags=re.S,
    )

def base_binding_spans(text: str) -> list[tuple[int, int, str]]:
    masked = mask_comments(text)
    roots = list(re.finditer(r'\bkeymap\s*\{', masked))
    if len(roots) != 1:
        raise Invalid('Expected one keymap container; no file changed.')
    opening = masked.index('{', roots[0].start())
    closing = close_brace(masked, opening)
    m = re.search(r'\bbindings\s*=\s*<(.*?)>\s*;', masked[opening + 1:closing], re.S)
    if not m:
        raise Invalid('No Base bindings found; no file changed.')
    start = opening + 1 + m.start(1)
    content = m.group(1)
    items = list(re.finditer(r'&[A-Za-z_]\w*[^&]*', content))
    result = []
    for item in items:
        trimmed = item[0].rstrip()
        result.append((start + item.start(), start + item.start() + len(trimmed), ' '.join(trimmed.split())))
    if len(result) != 67:
        raise Invalid(f'Base has {len(result)} bindings, expected 67; no file changed.')
    return result

def relocate_tab(text: str) -> tuple[str, list[dict]]:
    spans = base_binding_spans(text)
    old = [spans[p][2] for p in POSITIONS]
    tab_keys = {'&kp TAB', '&kp TABULATOR'}
    if old[0] in tab_keys:
        return text, []  # Already at the top; never rotate twice.
    if old[3] not in tab_keys:
        raise Invalid('Base position 48 is not a plain Tab. Refusing to guess or overwrite your layout.')
    new = [old[3], old[0], old[1], old[2]]
    changes = [{'position': p, 'before': a, 'after': b} for p, a, b in zip(POSITIONS, old, new)]
    output = text
    for position, binding in reversed(list(zip(POSITIONS, new))):
        a, b, _ = spans[position]
        output = output[:a] + binding + output[b:]
    return output, changes

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--apply', action='store_true', help='Write after making an external backup; default is preview only.')
    args = parser.parse_args()
    path = args.root.resolve() / 'config/modu.keymap'
    try:
        raw = path.read_bytes()
        old = raw.decode('utf-8')  # Preserve BOM and line endings.
        new, changes = relocate_tab(old)
        if not changes:
            print('Tab is already at Base position 12. No changes.')
            return 0
        for c in changes:
            print(f"Base[{c['position']}]: {c['before']} -> {c['after']}")
        if not args.apply:
            print('Preview only. Use --apply to write these four changes.')
            return 0
        backup = args.root.resolve().parent / (args.root.resolve().name + '-keymap-backup-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f') + '.keymap')
        backup.write_bytes(raw)
        if path.read_bytes() != raw:
            raise Invalid('The keymap changed during the operation; no write performed.')
        path.write_bytes(new.encode('utf-8'))
        print('Backup:', backup)
        print('Updated Base positions 12,24,36,48 only.')
        return 0
    except (OSError, UnicodeError, Invalid) as exc:
        print('STOP:', exc, file=sys.stderr)
        return 1

if __name__ == '__main__':
    raise SystemExit(main())
