# Third-party notices
## ZMK PMW3610 driver

The `zmk-pmw3610-driver` directory is based on work by the ZMK contributors, ufan, inorichi, badjeff, and the Zephyr PMW3610 driver. Its source files retain their original copyright and SPDX license identifiers.

- Source: https://github.com/badjeff/zmk-pmw3610-driver
- License: MIT, except files that state another license

The applicable MIT License text is included in `LICENSES/MIT.txt`.

`zmk-pmw3610-driver/Kconfig` is licensed under `LicenseRef-Nordic-5-Clause`:
Copyright 2024, Nordic Semiconductor ASA

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form, except as embedded into a Nordic
   Semiconductor ASA integrated circuit in a product or a software update for
   such product, must reproduce the above copyright notice, this list of
   conditions and the following disclaimer in the documentation and/or other
   materials provided with the distribution.
3. Neither the name of Nordic Semiconductor ASA nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.
4. This software, with or without modification, must only be used with a Nordic
   Semiconductor ASA integrated circuit.
5. Any software provided in binary form under this license must not be reverse
   engineered, decompiled, modified and/or disassembled.
THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
## Microsoft UF2 converter

The packaging workflow fetches `tools/uf2/uf2conv.py` and `tools/uf2/uf2families.json` from the pinned MODU-C upstream revision. Those files are derived from Microsoft UF2.

- Source: https://github.com/microsoft/uf2
- License: MIT
- Copyright: Microsoft Corporation

The applicable MIT License text is included in `LICENSES/MICROSOFT-UF2-MIT.txt`.

## ZMK Firmware and reusable build workflow

ZMK Firmware and the reusable user-config build workflow are fetched from the
pinned ZMK revision listed in `config/west.yml` and `.github/workflows/build.yml`.

- License: MIT
- Copyright: Copyright (c) 2020 The ZMK Contributors

The applicable license text is included in `LICENSES/ZMK-MIT.txt`.

## Notice retention in firmware artifacts

The GitHub Actions packaging job includes this file, the repository `LICENSE`,
`NOTICE.md`, and the `LICENSES/` directory alongside the generated UF2 files.

## Supplied Corne dongle overlay wiring

The user's supplied `zmk-corne-dongle-main(1).zip` includes an OLED overlay carrying
Copyright (c) 2021 Darryl deHaan and SPDX-License-Identifier: MIT. The OLED hardware
wiring and panel settings were adapted into the new `modu_dongle_oled.overlay`; the
original attribution is retained there. MIT text is in `LICENSES/MIT.txt`.
The old Corne repository's DYA/runtime/settings/battery-history/display modules are
not dependencies of this configuration.


## MAC v3 custom status screen

The new UI uses the pinned ZMK and LVGL APIs; a small monochrome Mac-style mark
is encoded as original pixel rows and does not require redistributed font files.
The MAC label is a user-selected visual label, not an Apple/EKS endorsement.
The peripheral LED hardware channel order and inverted PWM follow the vendor
implementation; its EKS attribution and non-commercial license are retained in
the replacement source. Upstream Zephyr and LVGL are obtained by the pinned
west manifest and keep their own notices. New host test doubles are not copies
of the Zephyr or LVGL implementation and do not emulate physical hardware.


## v4 original dongle UI dependency

englmaxi/zmk-dongle-display, https://github.com/englmaxi/zmk-dongle-display,
MIT (Copyright (c) 2024 The ZMK Contributors in the referenced widget files).
Fetched through west; source assets and LICENSE remain with that upstream project.
This archive does not redistribute its fonts or image assets. v4 builds the original
layout/widgets with narrow generated-source compatibility adaptations. The local
D/L/R battery widget retains v3's original peer-mapping logic.
