/* SPDX-License-Identifier: MIT -- real display adapter with mock labels. */
#define CONFIG_MODU_BATTERY_TELEMETRY_CENTRAL 1
#include "status_ui_mock.h"
static int64_t mock_clock;
static int64_t k_uptime_get(void) {return mock_clock;}
#include "../local-modules/modu-dongle/src/dongle_status.c"
int peripheral_slot_index_for_conn(struct bt_conn *conn) {return conn->source;}
static struct modu_battery_detail details[2];
static bool have_detail[2];
bool modu_battery_detail_for_peer(const uint8_t peer[7],struct modu_battery_detail *out) {
    for(int i=0;i<2;i++) if(have_detail[i] && peer[1]==mock_conns[i].addr.a.val[0]) {*out=details[i];return true;}
    return false;
}
int main(void) {
    (void)modu_settings_set;(void)save_identities;
    for(int i=0;i<3;i++) {
        mock_conns[i].info=(struct bt_conn_info){1,0,2};
        mock_conns[i].source=i;mock_conns[i].addr.type=1;mock_conns[i].addr.a.val[0]=(uint8_t)(30+i);
    }
    mock_conns[2].info.role=1;mock_conns[2].source=-1;
    struct zmk_widget_dongle_battery_status w={0};zmk_widget_dongle_battery_status_init(&w,NULL);
    /* Standard BAS zero without a successful detailed sample is UNKNOWN. */
    zmk_event_t e={.kind=1,.battery={.source=0,.state_of_charge=0}};status_event(&e);refresh_batteries(NULL);
    assert(!strcmp(half_labels[0]->text,"L  --%"));
    details[0]=(struct modu_battery_detail){.side=1,.result=MODU_BAT_OK,.percent=73,.millivolts=3970};
    details[1]=(struct modu_battery_detail){.side=0,.result=MODU_BAT_OK,.percent=85,.millivolts=4050};
    have_detail[0]=have_detail[1]=true;refresh_batteries(NULL);
    assert(!strcmp(half_labels[0]->text,"L  85%") && !strcmp(half_labels[1]->text,"R  73%"));
    /* Side identities came from firmware, without any key press. */
    details[1].result=MODU_BAT_FETCH_ERROR;refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L ERR"));
    details[1].result=MODU_BAT_OK;details[1].percent=0;details[1].millivolts=3300;
    refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L   0%"));
    mock_clock=3000;refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L 3.30V"));
    details[1].age_seconds=181;refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L OLD"));
    details[1].flags=MODU_BATTERY_FLAG_IDLE;details[1].age_seconds=300;
    refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L~3.30V"));
    mock_clock=0;refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L~  0%"));
    details[1].age_seconds=901;refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L OLD"));
    mock_conns[1].info.state=0;refresh_batteries(NULL);assert(!strcmp(half_labels[0]->text,"L OFF"));
    unsigned count=object_count;for(int i=0;i<1000;i++) refresh_batteries(NULL);assert(object_count==count);
    puts("PASS: v7 UI side identification and real-zero/error/stale/disconnected distinctions (mock labels)");return 0;
}
