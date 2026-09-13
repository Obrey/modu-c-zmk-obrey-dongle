# Implementation boundaries

## Kept unchanged

45 uploaded MODU firmware files under vendor/; the board's flash partitions,
GPIO matrix, direct keys, polling drivers, battery node, boot-retention setup,
PMW3610 driver, CPI, orientation transforms and P0.08 selector.
Five personal key layers (335 bindings), keymap metadata and the source-aware
boot behavior from the earlier uploaded rebuild.

## Added

`local-modules/modu-dongle` defines 3 new shield IDs. Peripheral overlays include
the exact vendored original side overlay, then disable the old local HID
listeners and old input-split route. Both import two new split channels;
left enables only ID 0 with its local sensor, right enables only ID 1.
The dongle imports only the common transform, mock scanner, both receiver
channels/listeners, and OLED wiring. It does not import MODU GPIO or PMW hardware.

The same 67-position default_transform exists on every device; right retains
the original column offset 6. No Corne 42/48-position transform is used.
OLED I2C wiring and display dimensions are copied from the provided Corne overlay,
including width=129 and segment-offset=1. The display renderer is stock ZMK.

## Boot dispatch

Native ZMK combo recognition invokes the custom source-aware behavior.
The module observes physical events before they are captured by the native combo
listener, checks three fresh down events from the same source, and delegates the
normal source-specific bootloader behavior with the observed source restored.
This continues to work conceptually when both sources are peripherals rather than
one local side. Remote peer IDs are not guessed from left/right labels.
The observer deliberately uses target_sources(app ...), not a separate library;
listener/link ordering still requires confirmation against the actual ZMK build.
If the observer did not see all three source events, the operation is rejected,
not silently rerouted to the dongle. No fallback to LOCAL exists in the adapter.

Host tests exercise the real C file against mocked ZMK/Zephyr APIs. They do not
prove embedded ABI compatibility, event-listener order or Bluetooth delivery.

## Editor validation

Validation is semantic for the direct boot combos and structural for ordinary
keys. Ordinary node names, mouse combos and Game bindings are not hard-coded.
Physical placeholder slots 51..56 remain disabled. Additional neutral keys may
also use &none without the earlier erroneous "extra placeholder" failure.
Normal rows must have 67 entries. The default number keys used for maintenance
must remain consistent with the combo positions and the observer guard.

## Core compatibility

The MODU config's pinned post-HWMv2 ZMK commit is retained. The new nice!nano v2
identifier is nice_nano@2.0.0//zmk. The old Corne v0.3 fork and custom display/RPC
plugins are intentionally not imported, since MODU uses the newer board model
and boot-retention mechanism. No ZMK Studio behavior editing is enabled.

## Packaging

5 explicit artifact names: 3 normal firmware images and 2 board-specific reset
images. UF2/HEX ambiguity, wrong family, duplicate normal outputs, invalid block
structure and addresses outside [0x26000, 0xEC000) fail packaging. The settings
reset program itself lives in the application partition; it erases stored
settings when run, not by packaging a direct write into the storage partition.

Source provenance is in SOURCES.json and reference/hardware-sha256.json.
