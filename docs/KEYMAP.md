# 전체 키 위치와 바인딩

이 표는 재구성본 파일에서 자동 추출했습니다. 원본 JSON의 논리 위치이며 PCB 실측표는 아닙니다.
51~56은 원본이 지정한 placeholder입니다. 66은 Base에서 중립으로 유지하며 실물 키 유무를 단정하지 않습니다.
Game의 방향키는 46/57/58/59입니다. 숫자 0은 위치 10이며 게임 전환키(위치 0)와 다릅니다.

| 위치 | 원본 행/열 | Base | Nav | Mouse | Game | FnMedia |
|---:|---|---|---|---|---|---|
| 0 | 0/0 | `&to L_GAME` | `&to L_BASE` | `&to L_BASE` | `&to L_BASE` | `&kp F11` |
| 1 | 0/1 | `&kp N1` | `&trans` | `&trans` | `&kp N1` | `&kp F1` |
| 2 | 0/2 | `&kp N2` | `&trans` | `&trans` | `&kp N2` | `&kp F2` |
| 3 | 0/3 | `&kp N3` | `&trans` | `&trans` | `&kp N3` | `&kp F3` |
| 4 | 0/4 | `&kp N4` | `&trans` | `&trans` | `&kp N4` | `&kp F4` |
| 5 | 0/5 | `&kp N5` | `&trans` | `&trans` | `&kp N5` | `&kp F5` |
| 6 | 0/6 | `&kp N6` | `&trans` | `&trans` | `&kp N6` | `&kp F6` |
| 7 | 0/7 | `&kp N7` | `&trans` | `&trans` | `&kp N7` | `&kp F7` |
| 8 | 0/8 | `&kp N8` | `&trans` | `&trans` | `&kp N8` | `&kp F8` |
| 9 | 0/9 | `&kp N9` | `&trans` | `&trans` | `&kp N9` | `&kp F9` |
| 10 | 0/10 | `&kp N0` | `&trans` | `&trans` | `&kp N0` | `&kp F10` |
| 11 | 0/11 | `&kp MINUS` | `&trans` | `&trans` | `&kp MINUS` | `&kp F12` |
| 12 | 1/0 | `&mt LG(A) LG(Z)` | `&trans` | `&trans` | `&kp TAB` | `&trans` |
| 13 | 1/1 | `&kp SLASH` | `&trans` | `&kp LG(MINUS)` | `&kp SLASH` | `&kp GRAVE` |
| 14 | 1/2 | `&kp W` | `&trans` | `&kp LG(EQUAL)` | `&kp W` | `&kp PSCRN` |
| 15 | 1/3 | `&kp E` | `&trans` | `&none` | `&kp E` | `&kp SLCK` |
| 16 | 1/4 | `&kp R` | `&trans` | `&kp LG(RBKT)` | `&kp R` | `&kp C_PAUSE` |
| 17 | 1/5 | `&kp T` | `&trans` | `&kp LG(LBKT)` | `&kp T` | `&kp CAPSLOCK` |
| 18 | 1/6 | `&kp Y` | `&trans` | `&kp LG(LBKT)` | `&kp Y` | `&bt BT_SEL 0` |
| 19 | 1/7 | `&kp U` | `&trans` | `&kp LG(RBKT)` | `&kp U` | `&bt BT_SEL 1` |
| 20 | 1/8 | `&kp I` | `&kp UP` | `&mmv MOVE_UP` | `&kp I` | `&bt BT_SEL 2` |
| 21 | 1/9 | `&kp O` | `&trans` | `&kp LG(EQUAL)` | `&kp O` | `&bt BT_SEL 3` |
| 22 | 1/10 | `&kp SEMICOLON` | `&trans` | `&kp LG(MINUS)` | `&kp SEMICOLON` | `&bt BT_SEL 4` |
| 23 | 1/11 | `&kp EQUAL` | `&trans` | `&trans` | `&kp EQUAL` | `&kp K_APP` |
| 24 | 2/0 | `&mt LG(X) LG(C)` | `&trans` | `&trans` | `&kp LCTRL` | `&trans` |
| 25 | 2/1 | `&hm250 LCTRL A` | `&trans` | `&none` | `&kp A` | `&trans` |
| 26 | 2/2 | `&hm200 LALT S` | `&kp LG(LC(LS(N4)))` | `&mkp MCLK` | `&kp S` | `&trans` |
| 27 | 2/3 | `&hm200 LEFT_GUI D` | `&kp LG(LS(N5))` | `&mkp RCLK` | `&kp D` | `&trans` |
| 28 | 2/4 | `&hm200 LSHFT F` | `&trans` | `&mkp LCLK` | `&kp F` | `&trans` |
| 29 | 2/5 | `&kp G` | `&trans` | `&none` | `&kp G` | `&trans` |
| 30 | 2/6 | `&kp H` | `&kp TAB` | `&kp LG(LBKT)` | `&kp H` | `&kp C_PREV` |
| 31 | 2/7 | `&hm200 LSHFT J` | `&kp LEFT` | `&mmv MOVE_LEFT` | `&kp J` | `&kp C_PP` |
| 32 | 2/8 | `&hm200 LEFT_GUI K` | `&kp DOWN` | `&mmv MOVE_DOWN` | `&kp K` | `&kp C_NEXT` |
| 33 | 2/9 | `&hm200 LALT L` | `&kp RIGHT` | `&mmv MOVE_RIGHT` | `&kp L` | `&kp C_VOL_DN` |
| 34 | 2/10 | `&hm250 LCTRL P` | `&kp HOME` | `&trans` | `&kp P` | `&kp C_VOL_UP` |
| 35 | 2/11 | `&kp APOS` | `&trans` | `&trans` | `&kp PERIOD` | `&kp C_MUTE` |
| 36 | 3/0 | `&kp LG(V)` | `&trans` | `&trans` | `&kp LSHFT` | `&trans` |
| 37 | 3/1 | `&kp Z` | `&trans` | `&msc SCRL_RIGHT` | `&kp Z` | `&kp LBKT` |
| 38 | 3/2 | `&kp X` | `&trans` | `&msc SCRL_LEFT` | `&kp X` | `&kp RBKT` |
| 39 | 3/3 | `&kp C` | `&trans` | `&msc SCRL_UP` | `&kp C` | `&mkp MCLK` |
| 40 | 3/4 | `&kp V` | `&kp LG(LC(LS(N3)))` | `&msc SCRL_DOWN` | `&kp V` | `&msc SCRL_UP` |
| 41 | 3/5 | `&kp Q` | `&trans` | `&trans` | `&kp Q` | `&msc SCRL_DOWN` |
| 42 | 3/6 | `&kp N` | `&kp INS` | `&kp LG(RBKT)` | `&kp N` | `&out OUT_USB` |
| 43 | 3/7 | `&kp M` | `&kp PG_UP` | `&msc SCRL_UP` | `&kp M` | `&out OUT_BLE` |
| 44 | 3/8 | `&kp B` | `&kp PG_DN` | `&msc SCRL_DOWN` | `&kp B` | `&out OUT_TOG` |
| 45 | 3/9 | `&kp COMMA` | `&kp END` | `&msc SCRL_LEFT` | `&kp COMMA` | `&kp APOS` |
| 46 | 3/10 | `&kp PERIOD` | `&kp RALT` | `&msc SCRL_RIGHT` | `&kp UP` | `&trans` |
| 47 | 3/11 | `&kp BSLH` | `&trans` | `&trans` | `&kp BSLH` | `&to L_MOUSE` |
| 48 | 4/0 | `&kp TAB` | `&trans` | `&kp TAB` | `&kp LEFT_ALT` | `&trans` |
| 49 | 4/1 | `&kp LBKT` | `&trans` | `&none` | `&mkp RCLK` | `&trans` |
| 50 | 4/2 | `&kp RBKT` | `&trans` | `&mo L_FN` | `&mo L_FN` | `&trans` |
| 51 | 4/3 | `&none` | `&none` | `&none` | `&none` | `&none` |
| 52 | 4/4 | `&none` | `&none` | `&none` | `&none` | `&none` |
| 53 | 4/5 | `&none` | `&none` | `&none` | `&none` | `&none` |
| 54 | 4/6 | `&none` | `&none` | `&none` | `&none` | `&none` |
| 55 | 4/7 | `&none` | `&none` | `&none` | `&none` | `&none` |
| 56 | 4/8 | `&none` | `&none` | `&none` | `&none` | `&none` |
| 57 | 4/9 | `&kp GRAVE` | `&trans` | `&mkp MCLK` | `&kp LEFT` | `&trans` |
| 58 | 4/10 | `&mkp LCLK` | `&trans` | `&mkp LCLK` | `&kp DOWN` | `&trans` |
| 59 | 4/11 | `&mkp RCLK` | `&trans` | `&mkp RCLK` | `&kp RIGHT` | `&trans` |
| 60 | 5/0 | `&kp ESC` | `&trans` | `&to L_BASE` | `&kp ESC` | `&trans` |
| 61 | 5/1 | `&kp SPACE` | `&trans` | `&trans` | `&kp SPACE` | `&trans` |
| 62 | 5/2 | `&lt L_NAV CAPSLOCK` | `&trans` | `&trans` | `&mkp LCLK` | `&trans` |
| 63 | 5/6 | `&kp ENTER` | `&trans` | `&mkp RCLK` | `&kp ENTER` | `&trans` |
| 64 | 5/7 | `&kp BACKSPACE` | `&trans` | `&mkp LCLK` | `&kp BACKSPACE` | `&trans` |
| 65 | 5/8 | `&lt L_FN DEL` | `&trans` | `&mkp MCLK` | `&kp DELETE` | `&trans` |
| 66 | 5/9 | `&trans` | `&none` | `&none` | `&none` | `&none` |
