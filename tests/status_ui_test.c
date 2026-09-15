/* SPDX-License-Identifier: MIT -- compile actual UI C against native test doubles. */
#include "status_ui_mock.h"
#include "../local-modules/modu-dongle/src/dongle_status.c"
int peripheral_slot_index_for_conn(struct bt_conn *conn) { return conn->source; }
static void key_event(uint8_t source, uint32_t position) {
    zmk_event_t event = {.kind=2, .position={.source=source,.position=position,.state=true}};
    assert(status_event(&event) == ZMK_EV_EVENT_BUBBLE);
}
static void battery_event(uint8_t source, uint8_t percent) {
    zmk_event_t event = {.kind=1,.battery={.source=source,.state_of_charge=percent}};
    assert(status_event(&event) == ZMK_EV_EVENT_BUBBLE);
}
static int restore(void *arg, void *out, size_t len) {
    (void)arg; memcpy(out,saved_settings,len); return (int)len;
}
int main(void) {
    for (unsigned i=0;i<3;i++) {
        mock_conns[i].info=(struct bt_conn_info){.type=BT_CONN_TYPE_LE,.role=BT_CONN_ROLE_CENTRAL,.state=BT_CONN_STATE_CONNECTED};
        mock_conns[i].addr.type=1; mock_conns[i].addr.a.val[0]=(uint8_t)(10+i);
        mock_conns[i].source=(int)i;
    }
    mock_conns[2].info.role=BT_CONN_ROLE_PERIPHERAL; /* A HOST, not a third half. */
    mock_conns[2].source=-1;
    assert(zmk_display_status_screen()!=NULL);
    assert(strcmp(host_label->text,"USB ON")==0);
    assert(strcmp(half_labels[0]->text,"L: press a key")==0);
    assert(strcmp(half_labels[1]->text,"R: press a key")==0);
    /* source 0 is RIGHT, source 1 is LEFT. */
    key_event(0,10); key_event(1,1);
    battery_event(0,77); battery_event(1,83);
    refresh_screen(NULL);
    assert(strcmp(half_labels[0]->text,"L:  83%  ON")==0);
    assert(strcmp(half_labels[1]->text,"R:  77%  ON")==0);
    save_identities(NULL); assert(save_calls==1);
    memset(&identities,0,sizeof(identities));
    assert(modu_settings_set("peers", saved_len, restore, NULL)==0);
    refresh_screen(NULL); assert(strcmp(half_labels[0]->text,"L:  83%  ON")==0);
    assert(modu_settings_set("wrong",saved_len,restore,NULL)==-ENOENT);
    assert(modu_settings_set("peers",saved_len-1,restore,NULL)==-EINVAL);
    battery_event(1,0); refresh_screen(NULL);
    assert(strcmp(half_labels[0]->text,"L:   0%  ON")==0);
    mock_conns[1].info.state=0;
    modu_status_conn_callbacks.disconnected(&mock_conns[1],0);
    refresh_screen(NULL); assert(strcmp(half_labels[0]->text,"L:  --%  OFF")==0);
    mock_conns[1].info.state=BT_CONN_STATE_CONNECTED;
    refresh_screen(NULL); assert(strcmp(half_labels[0]->text,"L:  --%  ON")==0);
    battery_event(1,55); refresh_screen(NULL);
    assert(strcmp(half_labels[0]->text,"L:  55%  ON")==0);
    battery_event(1,101); refresh_screen(NULL);
    assert(strcmp(half_labels[0]->text,"L:  --%  ON")==0);
    /* Connecting a different peer must never re-use a previous peer's L label. */
    mock_conns[1].addr.a.val[0]=99; refresh_screen(NULL);
    assert(strcmp(half_labels[0]->text,"L:  --%  OFF")==0);
    key_event(1,1); battery_event(1,64); refresh_screen(NULL);
    assert(strcmp(half_labels[0]->text,"L:  64%  ON")==0);
    mock_layer=3; refresh_screen(NULL); assert(strcmp(layer_label->text,"3 Game")==0);
    mock_endpoint.transport=ZMK_TRANSPORT_BLE; mock_endpoint.ble.profile_index=2;
    refresh_screen(NULL); assert(strcmp(host_label->text,"BT3 ON")==0);
    mock_endpoint.transport=ZMK_TRANSPORT_NONE;
    refresh_screen(NULL); assert(strcmp(host_label->text,"NO HOST")==0);
    assert(strcmp(half_labels[0]->text,"L:  64%  ON")==0); /* links != host */
#if CONFIG_MODU_DONGLE_HAS_BATTERY
    assert(strcmp(power_label->text,"D:  --% USB")==0);
    zmk_event_t local = {.kind=3,.local={.state_of_charge=42}};
    status_event(&local); refresh_screen(NULL);
    assert(strcmp(power_label->text,"D:  42% USB")==0);
    mock_usb=false; refresh_screen(NULL); assert(strcmp(power_label->text,"D:  42% BAT")==0);
#else
    assert(strcmp(power_label->text,"D: USB POWER")==0);
    mock_usb=false; refresh_screen(NULL); assert(strcmp(power_label->text,"D: EXT POWER")==0);
#endif
    unsigned n=object_count;
    for (int i=0;i<1000;i++) refresh_screen(NULL);
    assert(object_count==n); /* refresh must not create additional objects */
    for (unsigned i=0;i<n;i++) {
        assert(mock_objects[i].x>=0 && mock_objects[i].y>=0);
        assert(mock_objects[i].x+mock_objects[i].w<=128);
        assert(mock_objects[i].y+mock_objects[i].h<=64);
    }
    puts("Native UI simulation passed (NOT an ARM/LVGL/Zephyr build or hardware test)");
    return 0;
}
