# Implementation notes / upstream contracts

Pinned ZMK: `641514a97db345f499dd50b0360e594270f008fe`.
Pinned MODU-C firmware: `bee0bb4b812f63f279eb67e928accc89600b5904`.
Do not update these pins without reviewing the adapter and rebuilding all targets.

## UI

The local Zephyr module defines `zmk_display_status_screen()` with LVGL 9 APIs and custom-screen Kconfig.
All LVGL calls execute in the display work queue / LVGL timer context. Bluetooth callbacks and ZMK event
listeners only inspect links, update short spinlock-protected records, or schedule settings writes.
An OLED/I2C stall is separated from the system work queue, but hardware/driver failures are not impossible.
The Mac mark is a visual preference, not host OS detection or a new modifier remapping layer.

`peripheral_slot_index_for_conn()` is a GLOBAL function in the pinned ZMK
`app/src/split/bluetooth/central.c`. It is not declared in a public header.
This module declares that exact function to map real BLE connections to split source IDs.
It neither patches upstream nor changes the split wire protocol. This is a source-version-specific adapter.
`bt_conn_foreach()`/`bt_conn_get_info()` exclude host-role connections and disconnected entries.
A radio connection indication is not a guarantee of every subsequent GATT/HID operation.

Left/right labels are learned from MODU matrix positions and stored against the peer identity address,
not the first/second pairing slot. The first normal key press on each half identifies it; address changes
or settings reset require learning again. A saved mapping by itself does not imply an active connection.

Remote battery updates subscribe to `zmk_peripheral_battery_state_changed`, whose declaration is in
`zmk/events/battery_state_changed.h`. Both variants KEEP `ZMK_BATTERY_REPORTING=y`: the pinned ZMK
CMake uses that flag to include battery event implementations, including the peripheral event.
Never infer connection state from percentage > 0. Missing, invalid, disconnected and measured 0% differ.
Local dongle battery display is opt-in because a board battery ADC alone does not prove battery presence.

## LEDs

The original vendor left branch chooses host BLE profile status using SHIELD_MODU_LEFT, even if the
left is converted to a peripheral. Disable that implementation for both halves. The replacement reads
`zmk_split_bt_peripheral_is_connected/is_bonded` and preserves vendor inverted PWM and BGR/GBR wiring.
This is not a charger-IC LED control or proof of the dongle's host link.

## Keymap and build

The canonical file is `config/modu.keymap`. Explicit KEYMAP_FILE and include-only shield wrappers all
point there. CMake hashes that file, and compilation asserts the generated Devicetree has the expected
67 logical slots and side-specific boot holdtaps. The version tag and hash distinguish actual firmware.
The hash is not cryptographic attestation of a device; it is a build identification aid. Provenance JSON
is a release snapshot, not a lock against future keymap editing. Keymap settings storage is disabled
without erasing Bluetooth settings.

## Primary sources reviewed

- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/display/main.c
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/display/Kconfig
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/split/bluetooth/central.c
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/src/split/bluetooth/peripheral.c
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/include/zmk/events/battery_state_changed.h
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/include/zmk/endpoints.h
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/include/zmk/usb.h
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/include/zmk/keymap.h
- https://raw.githubusercontent.com/zmkfirmware/zmk/641514a97db345f499dd50b0360e594270f008fe/app/boards/shields/corne/Kconfig.defconfig
- https://raw.githubusercontent.com/22sh22/modu-c-firmware/bee0bb4b812f63f279eb67e928accc89600b5904/modu-module/src/led_breath/led_breath.c
- https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/v4.1.0/include/zephyr/bluetooth/conn.h
- https://raw.githubusercontent.com/zmkfirmware/lvgl/f1db87ee98f1810328a8419572fa42a3b5f352ae/lv_version.h
