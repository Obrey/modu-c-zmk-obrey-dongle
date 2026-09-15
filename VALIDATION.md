# Validation status — full dongle configuration

Date: 2026-09-15. This is a source configuration, not a compiled firmware release.

## Battery-link fix in this revision

The user supplied two failed ARM link logs, both reporting an undefined
`raise_zmk_peripheral_battery_state_changed` reference from `src/split/central.c`.
The previous source ZIP set `CONFIG_ZMK_BATTERY_REPORTING=n` while enabling
`CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING=y` in both dongle variants.

At pinned ZMK `641514a97db345f499dd50b0360e594270f008fe`,
`app/CMakeLists.txt` includes `src/events/battery_state_changed.c` only when
`CONFIG_ZMK_BATTERY_REPORTING` is enabled. That C file implements BOTH local
and peripheral battery events. Enabling peripheral fetching without compiling
that C file accounts for the undefined reference in the supplied logs.

Both dongle `.conf` files now explicitly set `CONFIG_ZMK_BATTERY_REPORTING=y`.
Peripheral fetching remains enabled. No ZMK version update, custom event stub,
new C module, keymap change, or hardware-overlay change was made.

The added host-side regression test was first run against the unmodified
configurations: it failed for BOTH dongle variants, as expected. After the two
configuration edits, all 14 configuration tests passed. This validates the
configuration guard, not an actual ARM compile/link. The previous tests were
insufficient to catch this configuration dependency; their success did not
establish that the firmware built.

## Executed locally

- `python3 scripts/validate.py`: metadata consistency; 67 positions on all nine layers;
  six target names; pinned direct revisions; quoted module/overlay paths; workflow hooks.
- `python3 scripts/selftest.py`: original HEX normalization and UF2 structural validation,
  missing/duplicate outputs, bad family/address/record cases, original conversion fixtures.
- `python3 scripts/test_full_config.py`: 14 host-side/static tests for hold-tap settings,
  boot confirmation positions, unchanged Game number keys, trackball IDs/listener configuration,
  dongle matrix coordinates, OLED wiring, the pinned battery-event dependency,
  and all six output names/packaging fixtures.

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

The local environment has no west/ARM SDK. A direct HTTPS source fetch from the
container failed with a DNS-resolution error; the web tool was used for source inspection. The workflow performs the actual firmware builds in
ZMK's official build container when run on GitHub.

## Source checks consulted

- https://zmk.dev/docs/hardware-integration/dongle
- https://zmk.dev/docs/hardware-integration/pointing
- https://zmk.dev/docs/keymaps/behaviors/hold-tap
- https://zmk.dev/docs/keymaps/behaviors/reset
- ZMK commit `641514a97db345f499dd50b0360e594270f008fe`:
  `app/boards/nicekeyboards/nice_nano/board.yml`, `app/src/pointing/input_split.c`,
  `app/src/split/bluetooth/Kconfig`, `app/src/display/Kconfig`,
  `.github/workflows/build-user-config.yml`, `app/CMakeLists.txt`,
  `app/src/events/battery_state_changed.c`, `app/src/split/central.c`, `app/Kconfig`.
- MODU commit `bee0bb4b812f63f279eb67e928accc89600b5904`:
  `modu-module/boards/shields/modu/{modu.dtsi,modu_left.overlay,modu_right.overlay,modu_left.conf,modu_right.conf}`.

## Reproducibility boundary

The two direct project commits are pinned. ZMK's transitive Zephyr branch and the official
build container tag remain inherited from the supplied workflow/manifest and are not frozen
by digest here. Static consistency is not a substitute for a successful build or a hardware test.
