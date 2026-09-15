# Validation status — full dongle configuration

Date: 2026-09-15. This is a source configuration, not a compiled firmware release.

## Executed locally

- `python3 scripts/validate.py`: metadata consistency; 67 positions on all nine layers;
  six target names; pinned direct revisions; quoted module/overlay paths; workflow hooks.
- `python3 scripts/selftest.py`: original HEX normalization and UF2 structural validation,
  missing/duplicate outputs, bad family/address/record cases, original conversion fixtures.
- `python3 scripts/test_full_config.py`: 13 host-side/static tests for hold-tap settings,
  boot confirmation positions, unchanged Game number keys, trackball IDs/listener configuration,
  dongle matrix coordinates, OLED wiring, and all six output names/packaging fixtures.

These tests inspect configuration and exercise Python packaging code. They do not execute
ZMK hold-tap timing, Bluetooth behavior, bootloader entry, or display drivers.
Synthetic UF2 test fixtures are created in temporary directories only and are NOT included
in the delivered source archive. No compiled firmware has been supplied.

## Not executed

- Full Zephyr/ARM compilation and linking of any target.
- The GitHub Actions workflow on the user's repository.
- MODU-C left/right physical key scanning and orientation selection.
- Peripheral reconnection, dual-trackball movement, source-specific bootloader entry.
- OLED initialization, alignment, or runtime status screen rendering.

The local environment has no west/ARM SDK. Attempts to download the pinned dependency
archives from the container failed. The workflow performs the actual firmware builds in
ZMK's official build container when run on GitHub.

## Source checks consulted

- https://zmk.dev/docs/hardware-integration/dongle
- https://zmk.dev/docs/hardware-integration/pointing
- https://zmk.dev/docs/keymaps/behaviors/hold-tap
- https://zmk.dev/docs/keymaps/behaviors/reset
- ZMK commit `641514a97db345f499dd50b0360e594270f008fe`:
  `app/boards/nicekeyboards/nice_nano/board.yml`, `app/src/pointing/input_split.c`,
  `app/src/split/bluetooth/Kconfig`, `app/src/display/Kconfig`,
  `.github/workflows/build-user-config.yml`.
- MODU commit `bee0bb4b812f63f279eb67e928accc89600b5904`:
  `modu-module/boards/shields/modu/{modu.dtsi,modu_left.overlay,modu_right.overlay,modu_left.conf,modu_right.conf}`.

## Reproducibility boundary

The two direct project commits are pinned. ZMK's transitive Zephyr branch and the official
build container tag remain inherited from the supplied workflow/manifest and are not frozen
by digest here. Static consistency is not a substitute for a successful build or a hardware test.
