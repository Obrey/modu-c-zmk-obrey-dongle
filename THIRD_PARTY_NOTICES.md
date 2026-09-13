# Third-party notices

See LICENSE for MODU-specific code and NOTICE.md for modification attribution.

* `vendor/modu-c-firmware/`: source supplied by the user, archive commit
  bffdf7e0bf6c1e7573b35492fb9f611a5c24fb5e. Every original file is unchanged.
  Its own THIRD_PARTY_NOTICES.md, LICENSES/, and tools/uf2/LICENSE are authoritative.
* `local-modules/modu-dongle/`: derived MODU layout + newly written integration,
  EKS non-commercial terms; OLED-only wiring fragment retains its MIT attribution.
* `local-modules/obrey-combo-boot/`: MIT; separate LICENSE retained.
* ZMK: downloaded by west from the original config's pinned commit
  641514a97db345f499dd50b0360e594270f008fe; MIT text in LICENSES/ZMK-MIT.txt.
* UF2 conversion is Microsoft's converter in the uploaded MODU source.
* The Corne cormoran v0.3 branch, runtime RPC modules and dongle-display plugin
  are NOT mixed into this newer MODU build. The OLED uses ZMK's built-in screen.
