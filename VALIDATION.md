# MAC v3 validation record — 2026-09-15

## What was executed

- `python3 scripts/validate.py`: 67-position metadata, layer structure, source pins, build matrix, overlay/module paths and workflow/packaging contracts.
- `python3 scripts/selftest.py`: Intel HEX normalization/corruption checks and UF2 packaging using deliberately synthetic fixtures, never distributable device firmware.
- `python3 scripts/test_full_config.py`: 14 existing configuration/packaging regressions.
- `python3 scripts/test_status.py`: 9 additional host tests, including:
  - Pure C helper code compiled by native GCC and executed with assertions for key position -> side, address identity, persistence validation, LED phase/channel selection and unknown/0/100/invalid battery values.
  - The actual `dongle_status.c` compiled and executed against test doubles of the relevant Zephyr/ZMK/LVGL API calls, with dongle-battery support both disabled and enabled.
  - UI simulation covers reversed pairing order, separate host vs half connectivity, percentage values, disconnect/reconnect, stale/changed peers, layer changes, settings restoration, invalid settings, fixed object count across 1,000 updates, and widget bounding boxes in 128x64.
  - Source checks cover mono/custom screen settings, actual split connection LED functions, no LVGL calls in event callbacks, canonical keymap wrappers, no Studio keymap overrides, and the battery event dependency fix.
- YAML parsing of the manifest, module metadata, workflow and build matrix.
- Release source keymap compared to the supplied custom keymap: all seven original layer binding arrays match except the requested 5/6 holdtap changes, plus two added boot layers. `KEYMAP_PROVENANCE.json` records the hashes and exact binding changes.

The executed results are in `docs/TEST_RESULTS.txt`. The native test API doubles are in `tests/`.
They check C logic against the described API contracts; they are not the actual Zephyr or LVGL implementations.

## What was NOT executed

- A real Zephyr/ZMK ARM compilation or linker invocation with the nRF52840 toolchain.
- Generated Devicetree/Kconfig evaluation against the complete west dependency workspace.
- A firmware run on the user's dongle, OLED, trackballs, battery sensors or LEDs.
- A real Bluetooth battery event, persistent flash transaction, radio connection or host HID test.
- A physical test of OLED memory use, latency, three-second boot gestures or power consumption.

This working environment has no ZMK west workspace or ARM SDK. Its container network could not resolve
the dependency host during an attempted retrieval; primary source headers were inspected with the web tool.
There is no claim that the resulting six firmware targets already compile or work on hardware.
GitHub Actions performs the actual firmware build on the user's commit. Only a successful run provides
compiler/linker evidence; hardware behavior still requires a device test.

## Prior user observation, not a test performed here

The user reports that the previous headless dongle firmware accepts keyboard input. That narrows the
problem and motivates preserving the working radio/matrix/trackball configuration, not resetting bonds.
The earlier OLED failure and the reported different live keymap are not definitively diagnosed from
that observation alone. MAC v3 adds isolated rendering/config corrections and visible build/layer identity
rather than asserting an unverified single root cause.

## Rollback

Retain the previously working headless UF2. New build artifacts place the headless target under
`fallback/`. Do not use `reset/` during the ordinary update of an already connected setup.
