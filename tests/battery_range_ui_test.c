/* SPDX-License-Identifier: MIT -- real display C adapter, native fake hardware. */
#define CONFIG_MODU_BATTERY_TELEMETRY_CENTRAL 1
#define CONFIG_MODU_BATTERY_RANGE_DISPLAY 1
#define CONFIG_MODU_DONGLE_HAS_BATTERY 1
#ifndef CONFIG_MODU_BATTERY_RANGE_LEFT_MIN_MV
#define CONFIG_MODU_BATTERY_RANGE_LEFT_MIN_MV 1750
#endif
#ifndef CONFIG_MODU_BATTERY_RANGE_LEFT_MAX_MV
#define CONFIG_MODU_BATTERY_RANGE_LEFT_MAX_MV 2950
#endif
#ifndef CONFIG_MODU_BATTERY_RANGE_RIGHT_MIN_MV
#define CONFIG_MODU_BATTERY_RANGE_RIGHT_MIN_MV 1750
#endif
#ifndef CONFIG_MODU_BATTERY_RANGE_RIGHT_MAX_MV
#define CONFIG_MODU_BATTERY_RANGE_RIGHT_MAX_MV 2950
#endif
#include "status_ui_mock.h"
static int64_t mock_clock;
static int64_t k_uptime_get(void) { return mock_clock; }
#include "../local-modules/modu-dongle/src/dongle_status.c"
int peripheral_slot_index_for_conn(struct bt_conn *conn) { return conn->source; }
static struct modu_battery_detail details[2];
static bool have_detail[2];
bool modu_battery_detail_for_peer(const uint8_t peer[7], struct modu_battery_detail *out) {
    for (int i=0;i<2;i++)
        if (have_detail[i] && peer[1]==mock_conns[i].addr.a.val[0]) {
            *out=details[i]; return true;
        }
    return false;
}
int main(void) {
    for(int i=0;i<3;i++) {
        mock_conns[i].info=(struct bt_conn_info){1,0,2};
        mock_conns[i].source=i;mock_conns[i].addr.type=1;
        mock_conns[i].addr.a.val[0]=(uint8_t)(30+i);
    }
    mock_conns[2].info.role=1;mock_conns[2].source=-1;
    struct zmk_widget_dongle_battery_status widget={0};
    assert(zmk_widget_dongle_battery_status_init(&widget,NULL)==0);
    assert(!strcmp(half_labels[0]->text,"L  --%"));
    assert(!strcmp(power_label->text,"D  --%"));
    zmk_event_t event={.kind=3,.local={.state_of_charge=42}};
    status_event(&event);refresh_batteries(NULL);
    assert(!strcmp(power_label->text,"D  42%")); /* D is NOT range-normalized. */
    event=(zmk_event_t){.kind=1,.battery={.source=0,.state_of_charge=0}};
    status_event(&event);refresh_batteries(NULL);
    assert(!strcmp(half_labels[0]->text,"L  --%")); /* No fake guess from BAS 0. */
    /* Reversed connection slots: profiles must follow handedness, not slots. */
    details[0]=(struct modu_battery_detail){.side=1,.result=MODU_BAT_OK,.percent=0,.millivolts=2800};
    details[1]=(struct modu_battery_detail){.side=0,.result=MODU_BAT_OK,.percent=0,.millivolts=2730};
    have_detail[0]=have_detail[1]=true;refresh_batteries(NULL);
    assert(!strcmp(half_labels[0]->text,"L  82%"));
#if CONFIG_MODU_BATTERY_RANGE_RIGHT_MIN_MV == 3000
    assert(!strcmp(half_labels[1]->text,"R   0%"));
#else
    assert(!strcmp(half_labels[1]->text,"R  88%"));
#endif
    assert(details[0].percent==0 && details[1].percent==0);
    assert(details[1].millivolts==2730);
    assert(!strcmp(power_label->text,"D  42%"));
    mock_clock=3000;refresh_batteries(NULL);
    assert(!strcmp(half_labels[0]->text,"L  82%"));
    details[1].flags=MODU_BATTERY_FLAG_IDLE;details[1].age_seconds=300;
    refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L~ 82%"));
    mock_clock=0;refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L~ 82%"));
    details[1].age_seconds=901;refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L OLD"));
    details[1].flags=0;details[1].age_seconds=0;details[1].result=MODU_BAT_FETCH_ERROR;
    refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L ERR"));
    details[1].result=MODU_BAT_OK;details[1].millivolts=1760;
    refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L   1%"));
    details[1].millivolts=3300;details[1].flags=MODU_BATTERY_FLAG_USB_POWERED;
    refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L CHG"));
    details[1].flags=0;details[1].millivolts=1870;refresh_batteries(NULL);
    assert(!strcmp(half_labels[0]->text,"L  10%"));
    mock_conns[1].info.state=0;refresh_batteries(NULL);
    assert(!strcmp(half_labels[0]->text,"L OFF"));
    unsigned objects=object_count;
    for(unsigned i=0;i<1000;i++) refresh_batteries(NULL);
    assert(object_count==objects);
    assert(widget.obj->w==56 && widget.obj->h==30);
    assert(half_labels[0]->w==56 && half_labels[1]->w==56);
    puts("PASS: v8 real UI adapter; L/R profiles, observed-range markers, unchanged D/raw/errors/layout");
    return 0;
}
