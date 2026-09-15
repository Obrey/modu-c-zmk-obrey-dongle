# Instructions for editing this repository

This is a complete MODU-C user-config repo, not a fork of ZMK and not the old Corne firmware.
The requested boot gesture is Base: hold physical 5 alone for >=3000 ms then press 1 while
holding, and hold physical 6 alone for >=3000 ms then press 0 while holding. Do not replace it
with a combo that invokes bootloader directly: that changes source-device semantics.

Keep the user's seven existing normal layers and 67-position matrix order. Gate behavior is
isolated to blt5/blt6; do not globally change &lt or &mt. Game numbers remain plain &kp.
The left and right halves both use peripheral role; only the dongle is central. Physical
trackball ID 0 is right, ID 1 is left. Disable any unbound peripheral input-split node.
Do not transplant MODU hardware GPIO nodes into the nice!nano dongle.

ZMK/MODU direct commits are in config/west.yml. Read the pinned sources rather than copying
fork-only DYA settings. Keep manufacturer wiring, PMW3610 and alt-thumb modules.
No root zephyr/module.yml: the reusable workflow's dependency paths rely on this choice.
Dongle shield is a separate local module passed via ZMK_EXTRA_MODULES.

Before commits run:
python3 scripts/validate.py
python3 scripts/selftest.py
python3 scripts/test_full_config.py

These are STATIC/HOST tests, not firmware builds. Never label synthetic UF2 fixtures as
flashable firmware. Build success and physical verification must be stated separately.
Preserve all license notices. Review docs/FLASHING_KO.md before proposing a settings reset.
