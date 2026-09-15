# Implementation notes — v5 (v4 integration retained)

## Preserve the working input configuration

ZMK remains 641514a97db345f499dd50b0360e594270f008fe and MODU hardware remains
bee0bb4b812f63f279eb67e928accc89600b5904. The keymap, build matrix, left/right
overlays, LED driver and headless dongle configuration are unchanged from v3.

## Original, not re-drawn, display UI

The supplied Corne repository names englmaxi/zmk-dongle-display. v4 fetches that
project via west and compiles its original custom_status_screen, output_status,
modifiers and modifier symbols, layer_status, Bongo Cat and HID indicators.
It does NOT compile the v3 standalone status screen or render an Apple mark.
The original screen coordinates and modifier animation logic are retained.
Only the small top-right battery widget implementation is replaced.

The old Corne's `v0.3` display branch is for LVGL 8. The existing working MODU ZMK
pin uses LVGL 9, so this revision uses the display project's `main` branch as
documented upstream. **This new display dependency is a floating ref**; the
exact commit SHA is logged at configure time and written into
`modu-display-revision.txt` in the build directory. The known core/board pins
are unchanged. To fully freeze future reproductions, replace this main with
the SHA from a successful build. No unverified commit SHA is invented here.

We do not add the `dongle_display` shield, which would compile duplicate widget
symbols. The existing modu_dongle_oled shield controls the upstream source list
through original_display.cmake. The upstream module is a board_root-only west
module, so being fetched does not enable its display on keyboard halves.

Narrow build-directory adaptations:
- CONFIG_ZMK_BATTERY -> CONFIG_ZMK_BATTERY_REPORTING, the name on the pinned ZMK.
- HID widget initialization handles the NULL event used by ZMK widget listeners.
- The upstream battery implementation is not linked; our implementation uses
  its unchanged `struct zmk_widget_dongle_battery_status` ABI.

No fetched project source is overwritten. Checks reject old LVGL-8 image
formats and missing Mac modifier/battery APIs rather than silently falling back.

## Battery identity and sensor limitations

The event/peer backend is retained from v3: source index is NOT assumed to mean
left or right. A physical key event identifies the side, mapped to the BLE peer
address; the map is saved as `modu_status/peers`. v3's stored format is retained.
The display uses three rows D, L, R (6 monospace characters maximum, width 56).
Unknown data is --%; an identified disconnected side is OFF; zero is a valid
percentage. A disconnected peer's cached value is invalidated.
Callbacks use no LVGL API. A display timer updates existing labels at 500 ms.

D reads the local ZMK battery event, not an invented capacity. It starts unknown
until an event arrives. An ADC on a USB-only board is not proof a cell exists;
MODU_DONGLE_HAS_BATTERY=n displays D USB or D EXT instead of a percentage.

## White-screen remediation, not a claimed physical diagnosis

v3 explicitly disabled the mono theme and rendered its own opposite-polarity
screen/marks. v4 returns to the original screen's white logical background,
black content, preserving the supplied panel inversion setting.
CORRECTION in v5: v4 also enabled the LVGL mono WIDGET THEME. That theme adds
borders and 4-pixel padding to generic icon containers, changing the original
layout. v5 disables all three built-in themes. The original custom screen
still sets its own background/text styles; 1-bit pixel/buffer settings stay enabled.
The SH1106 hardware configuration is otherwise retained at 128x64 logical pixels.
VDB=100 + FULL_REFRESH uses an entire page-aligned frame; the pinned Zephyr
allocates 1024 pixel bytes plus the 8-byte monochrome palette. A separate
conversion buffer is enabled. Display work stays on a dedicated queue.

These are source-level corrections. Neither an ARM build nor actual panel
rendering was possible in this environment. Input success alone does not identify
the reason a panel showed all white. Host tests do not validate I2C, LVGL rendering,
upstream widget compilation, or physical hardware.

## Sources consulted on 2026-09-15

- https://raw.githubusercontent.com/englmaxi/zmk-dongle-display/main/README.md
- https://raw.githubusercontent.com/englmaxi/zmk-dongle-display/main/boards/shields/dongle_display/custom_status_screen.c
- https://raw.githubusercontent.com/englmaxi/zmk-dongle-display/main/boards/shields/dongle_display/widgets/modifiers.c
- https://raw.githubusercontent.com/englmaxi/zmk-dongle-display/main/boards/shields/dongle_display/widgets/hid_indicators.c
- https://raw.githubusercontent.com/englmaxi/zmk-dongle-display/main/boards/shields/dongle_display/widgets/battery_status.h
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/Kconfig
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/display/main.c
- https://raw.githubusercontent.com/zmkfirmware/zephyr/v4.1.0%2Bzmk-fixes/modules/lvgl/Kconfig.memory

## v5 theme correction sources

- https://raw.githubusercontent.com/zmkfirmware/lvgl/f1db87ee98f1810328a8419572fa42a3b5f352ae/src/themes/mono/lv_theme_mono.c
- https://raw.githubusercontent.com/zmkfirmware/lvgl/f1db87ee98f1810328a8419572fa42a3b5f352ae/Kconfig
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/display/main.c

The local battery implementation is byte-identical to v4. See V5_BATTERY_FINDINGS_KO.md
for the distinction between missing data and a received zero, and what remains unknown.
