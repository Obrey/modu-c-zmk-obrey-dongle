# MODU-C dongle LED compile correction

## Exact failure

The supplied MODU-C source recognized only `CONFIG_SHIELD_MODU_LEFT` and
`CONFIG_SHIELD_MODU_RIGHT`. The dongle wrapper builds
`CONFIG_SHIELD_MODU_PERIPHERAL_LEFT` and `CONFIG_SHIELD_MODU_PERIPHERAL_RIGHT`.
Neither old flag is set in those builds. The C preprocessor therefore removed
`white_color` and `left_usb_connected()`, but selected the non-right body of
`breath_work_handler()`, which referenced both. This explains the user's
180/199-line diagnostics, for both dongle peripheral targets.

## Minimal source correction

Extend only the two `MODU_IS_*_SHIELD` macro expressions to recognize the
corresponding new shield names. Keep the old names supported. Do not change
`CONFIG_ZMK_SPLIT_ROLE_CENTRAL`, USB, GPIO, PWM channel ordering, timers, keymaps,
battery reporting, boot combos, or trackball code. Do not turn LEDs off to hide
this compile failure.

The canonical original and patched hashes are recorded in
`reference/hardware-normalized-sha256.json` under `approved_local_patches`.
The two hardware baseline manifests intentionally update only this C file's
expected digest. Unknown changes elsewhere are still rejected. This is a
recorded local change, not a claim that all vendor files remain byte-identical
to the uploaded upstream ZIP. All other 44 vendor files are unchanged by this
patch. The original copyright/license header remains in the file.

## Regression checks

`tests/test_led_shield_compat.py` compiles the actual vendored C file with a
host compiler and **mocked Zephyr/ZMK headers**, under original-left,
original-right, dongle-left and dongle-right defines. It also checks that the
right PWM palette is selected on the new right shield and that the left
peripheral, with USB disabled, does not call the USB API. This is a C declaration
and preprocessor check, **not** an ARM firmware build, transport/link test, or
physical LED test. Without a host C compiler the compile tests are skipped.
GitHub's Linux validation job normally provides a host compiler.

## Applying

Run the accompanying `APPLY_LED_FIX.cmd` outside the working repository and
select the repository folder. It recognizes the exact known original source
(or this already-patched source), backs up touched files beside the repository,
and preserves the current keymap and all previous non-LED patches. It does not
commit, push, flash, erase settings, or update a firmware pin.

Review the changed files, commit and push them. Confirm the **new commit's**
Actions build. A full five-target ZMK build and physical device testing are
still required. The prior battery-event link compatibility patch is separate
and is not overwritten or replaced here.
