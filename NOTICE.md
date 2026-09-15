# Unofficial modification notice

This repository is an unofficial, modified, non-commercial ZMK user-config wrapper initially created on 2026-09-01 and audited on 2026-09-02.

It is **not** an official EKS Inc. or MODU release.

## Original work

- Project: `22sh22/modu-c-firmware`
- Original firmware copyright: Copyright (c) 2026 EKS Inc.
- Original firmware creator: Ryu
- Pinned upstream revision: `bee0bb4b812f63f279eb67e928accc89600b5904`
- Original license: EKS NON-COMMERCIAL SOURCE LICENSE 1.0

## Modifications in this wrapper

- Copied the original `modu.keymap` into the conventional user-config path `config/modu.keymap`, then customized it through Keymap Editor.
- Added Keymap Editor layout metadata for the 67-position `default_transform`.
- Added a pinned west manifest that fetches the original MODU-C board, shield, custom scanning code, and PMW3610 driver.
- Added GitHub Actions automation for left/right builds and the same nRF52840 HEX-to-UF2 family used by the original build scripts.
- Added exact output selection, Intel HEX validation/normalization, UF2 structural verification, and dependency-free self-tests.
- Added static consistency checks, license/notice packaging, and Korean setup documentation.

No trademark rights, patent rights, warranty, or endorsement are provided.

## 2026-09-15 full dongle adaptation

This distribution additionally retains the supplied user-customized keymap, adds ordinary
ZMK hold-tap boot-gate layers, replaces the old custom boot-combo dependency, and adds
MODU-C peripheral overlays, a dongle shield module, two dongle variants,
board-specific reset builds, and updated packaging/host checks. It is not an EKS release.
OLED wiring is adapted from the supplied Corne overlay, which carries
Copyright (c) 2021 Darryl deHaan / SPDX-License-Identifier: MIT.
The MODU matrix coordinates and original MODU material retain the EKS notice and license.
No source or hardware test success beyond VALIDATION.md is claimed.

## 2026-09-15 MAC v3 status additions

The local module now adds a split-aware custom LVGL status screen, persistent
left/right peer identification, generated-devicetree keymap guards, and a
peripheral-role connection LED replacement. MAC is a fixed visual preference,
not OS detection. The replacement LED file retains the EKS notice and license
for vendor hardware mappings and polarity. Independent UI/helper/test code is
marked MIT. Original notices remain in force. Native simulation is not a
Zephyr/ARM build, physical display test, or manufacturer endorsement.
