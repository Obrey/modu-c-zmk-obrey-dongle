/* SPDX-License-Identifier: MIT -- native tests, not a Zephyr hardware test. */
#include <assert.h>
#include "../local-modules/modu-dongle/src/status_logic.h"
int main(void) {
    for (unsigned p = 0; p < 60; p++)
        assert(modu_side_for_position(p) == ((p >= 51 && p <= 56) ? -1 : ((p % 12) < 6 ? 0 : 1)));
    for (unsigned p = 60; p <= 62; p++) assert(modu_side_for_position(p) == 0);
    for (unsigned p = 63; p <= 66; p++) assert(modu_side_for_position(p) == 1);
    assert(modu_side_for_position(67) == -1);
    assert(modu_side_for_position(UINT32_MAX) == -1);
    uint8_t left[7] = {1, 1, 2, 3, 4, 5, 6}, right[7] = {1, 6, 5, 4, 3, 2, 1};
    struct modu_identities ids = {.version = 1};
    assert(modu_valid_identities(&ids));
    /* First connection is right. Never guess that source 0 is left. */
    assert(modu_learn_peer(&ids, 1, right));
    assert(modu_learn_peer(&ids, 0, left));
    assert(modu_side_for_peer(&ids, right) == 1);
    assert(modu_side_for_peer(&ids, left) == 0);
    assert(!modu_learn_peer(&ids, 0, left));
    assert(!modu_learn_peer(&ids, -1, left));
    assert(!modu_learn_peer(&ids, 2, left));
    struct modu_identities restored;
    memcpy(&restored, &ids, sizeof(ids));
    assert(modu_valid_identities(&restored));
    assert(modu_side_for_peer(&restored, right) == 1);
    restored.version = 4; assert(!modu_valid_identities(&restored));
    restored = ids; restored.side[0].known = 2; assert(!modu_valid_identities(&restored));
    restored = ids; restored.side[1] = restored.side[0]; assert(!modu_valid_identities(&restored));
    assert(modu_learn_peer(&ids, 1, left));
    assert(!ids.side[0].known && modu_side_for_peer(&ids, left) == 1);
    assert(modu_valid_identities(&ids));
    char buf[32];
    modu_format_half(buf, sizeof(buf), 0, false, false, false, 0);
    assert(strcmp(buf, "L: press a key") == 0);
    modu_format_half(buf, sizeof(buf), 1, true, false, true, 83);
    assert(strcmp(buf, "R:  --%  OFF") == 0);
    modu_format_half(buf, sizeof(buf), 0, true, true, false, 0);
    assert(strcmp(buf, "L:  --%  ON") == 0);
    modu_format_half(buf, sizeof(buf), 0, true, true, true, 0);
    assert(strcmp(buf, "L:   0%  ON") == 0);
    modu_format_half(buf, sizeof(buf), 1, true, true, true, 100);
    assert(strcmp(buf, "R: 100%  ON") == 0);
    modu_format_half(buf, sizeof(buf), 1, true, true, true, 255);
    assert(strcmp(buf, "R:  --%  ON") == 0);
    char tiny[2] = {1, 1}; modu_format_half(tiny, sizeof(tiny), 0, true, true, true, 99);
    assert(tiny[1] == '\0');
    assert(modu_led_channel(false, true) == 1);
    assert(modu_led_channel(true, true) == 0);
    assert(modu_led_channel(false, false) == 0);
    assert(modu_led_channel(true, false) == 1);
    for (unsigned ms = 0; ms < 2000; ms++) assert(modu_led_visible(true, false, ms));
    assert(modu_led_visible(false, true, 100));
    assert(!modu_led_visible(false, true, 750));
    assert(modu_led_visible(false, false, 450));
    assert(!modu_led_visible(false, false, 250));
    puts("Native status helpers passed: L/R identity, battery validity/format, LED state");
    return 0;
}
