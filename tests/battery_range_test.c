/* SPDX-License-Identifier: MIT -- display arithmetic, NOT cell calibration. */
#include <assert.h>
#include <string.h>
#include "../local-modules/modu-dongle/src/battery_range_display.h"
int main(void) {
    const struct modu_battery_range range = {1750, 2950};
    const struct modu_battery_range invalid = {2950, 1750}, equal = {2000, 2000};
    uint8_t pct = 255;
    assert(!modu_battery_range_percent(3000, NULL, &pct));
    assert(!modu_battery_range_percent(3000, &range, NULL));
    assert(!modu_battery_range_percent(3000, &invalid, &pct));
    assert(!modu_battery_range_percent(3000, &equal, &pct));
    static const unsigned examples[][2] = {
        {0,0}, {1749,0}, {1750,0}, {1760,1}, {1780,3}, {1870,10},
        {2000,21}, {2350,50}, {2640,74}, {2730,82}, {2800,88},
        {2950,100}, {3300,100}, {65535,100}
    };    for (size_t i=0; i<sizeof(examples)/sizeof(examples[0]); i++) {
        assert(modu_battery_range_percent(examples[i][0], &range, &pct));
        assert(pct == examples[i][1]);
    }
    unsigned previous=0;
    for (uint32_t mv=0; mv<=65535; mv++) {
        assert(modu_battery_range_percent((uint16_t)mv, &range, &pct));
        assert(pct <= 100 && pct >= previous); previous=pct;
    }
    struct modu_battery_detail d = {.side=0, .result=MODU_BAT_OK,
        .percent=0, .millivolts=2730, .age_seconds=0, .sequence=12};
    const struct modu_battery_detail original = d;
    char text[16];
    modu_battery_format_range(text, sizeof(text), 'L', &d, false, &range);
    assert(!strcmp(text, "L  82%"));
    assert(!memcmp(&d, &original, sizeof(d))); /* NEVER overwrites actual telemetry. */
    modu_battery_format_range(text, sizeof(text), 'L', &d, true, &range);
    assert(!strcmp(text, "L 2.73V"));
    d.flags=MODU_BATTERY_FLAG_IDLE;d.age_seconds=300;
    modu_battery_format_range(text, sizeof(text), 'R', &d, false, &range);
    assert(!strcmp(text, "R~ 82%"));
    modu_battery_format_range(text, sizeof(text), 'R', &d, true, &range);
    assert(!strcmp(text, "R~2.73V"));
    d.age_seconds=901;
    modu_battery_format_range(text, sizeof(text), 'R', &d, false, &range);
    assert(!strcmp(text, "R OLD"));
    d.flags=0;d.age_seconds=0;d.result=MODU_BAT_FETCH_ERROR;
    modu_battery_format_range(text, sizeof(text), 'L', &d, false, &range);
    assert(!strcmp(text, "L ERR"));
    modu_battery_format_range(text, sizeof(text), 'L', NULL, false, &range);
    assert(!strcmp(text, "L  --%"));
    d.result=MODU_BAT_WAIT;
    modu_battery_format_range(text, sizeof(text), 'L', &d, false, &range);
    assert(!strcmp(text, "L  --%"));
    d.result=MODU_BAT_OK;d.millivolts=1760;
    modu_battery_format_range(text, sizeof(text), 'L', &d, false, &range);
    assert(!strcmp(text, "L   1%"));
    d.millivolts=3300; d.flags=MODU_BATTERY_FLAG_USB_POWERED;
    modu_battery_format_range(text, sizeof(text), 'L', &d, false, &range);
    assert(!strcmp(text, "L CHG"));
    modu_battery_format_range(text, sizeof(text), 'L', &d, true, &range);
    assert(!strcmp(text, "L 3.30V"));
    d.flags=0;
    d.millivolts=3320;
    modu_battery_format_range(text, sizeof(text), 'L', &d, false, &invalid);
    assert(!strcmp(text, "L   0%")); /* Invalid profile fails closed to old path. */
    for (uint32_t mv=0; mv<=65535; mv++) {
        d.millivolts=(uint16_t)mv;
        for (unsigned idle=0; idle<2; idle++) {
            d.flags=idle ? MODU_BATTERY_FLAG_IDLE : 0;
            for (unsigned phase=0; phase<2; phase++) {
                modu_battery_format_range(text, sizeof(text), 'L', &d, phase, &range);
                assert(strlen(text)<=7);
            }
        }
    }
    char small[4];
    for (size_t cap=0; cap<=3; cap++) {
        memset(small,'X',sizeof(small));
        modu_battery_format_range(small,cap,'L',&original,false,&range);
        assert(small[cap]=='X');
        if (cap) assert(small[cap-1]=='\0');
    }
    puts("PASS: full uint16 range, observed examples, rounding, markers, raw immutability and label bounds");
    return 0;
}
