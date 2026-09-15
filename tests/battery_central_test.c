/* SPDX-License-Identifier: MIT -- no radio or Zephyr scheduler is emulated. */
#include "battery_runtime_mock.h"
#include "../local-modules/modu-dongle/src/battery_telemetry_central.c"
int peripheral_slot_index_for_conn(struct bt_conn *c) {return c->source;}
int main(void) {
    for(int i=0;i<3;i++) {
        mock_connections[i].source=i;mock_connections[i].addr.type=1;
        mock_connections[i].addr.a.val[0]=(uint8_t)(20+i);
        mock_connections[i].info=(struct bt_conn_info){1,2,0};mock_connections[i].security=2;
    }
    mock_connections[2].info.role=BT_CONN_ROLE_PERIPHERAL; /* Host, never a hand. */
    client_init();mock_clock=8000;client_poll(NULL);assert(read_calls==2);
    assert(clients[0].state==1 && clients[1].state==1);
    struct bt_gatt_read_params saved=clients[0].params;
    mock_clock=20000;client_poll(NULL);assert(read_calls==2);
    assert(memcmp(&saved,&clients[0].params,sizeof(saved))==0); /* Do not reuse in-flight params. */
    struct modu_battery_detail d={.side=1,.result=MODU_BAT_OK,.percent=77,.millivolts=4020,.age_seconds=2,.sequence=7};
    uint8_t packet[12];modu_battery_encode(&d,packet);
    assert(read_reply(&mock_connections[0],0,&clients[0].params,packet,12)==BT_GATT_ITER_STOP);
    uint8_t peer[7];serialize_peer(&mock_connections[0],peer);
    struct modu_battery_detail got;assert(modu_battery_detail_for_peer(peer,&got));
    assert(got.side==1 && got.percent==77 && got.millivolts==4020);
    mock_clock+=1000;assert(modu_battery_detail_for_peer(peer,&got));assert(got.age_seconds==3);
    /* Other hand error: no fabricated zero or valid record. */
    read_reply(&mock_connections[1],10,&clients[1].params,NULL,0);
    assert(!cache[1].valid); /* No service on an old firmware is not 0%. */
    telemetry_disconnected(&mock_connections[0],0);assert(!modu_battery_detail_for_peer(peer,&got));
    mock_connections[0].info.state=0;
    mock_clock+=2000;client_poll(NULL);assert(mock_connections[0].refs==0);
    /* Completion releases refs even if no connection remains to enumerate. */
    assert(mock_connections[1].refs==1); /* Has started a new periodic request. */
    read_reply(&mock_connections[1],0,&clients[1].params,packet,3);assert(!cache[1].valid);
    mock_connections[1].info.state=0;mock_clock+=2000;client_poll(NULL);
    assert(mock_connections[1].refs==0);
    puts("PASS: central filtering/read lifetime/cache/age/disconnect logic (mock BLE)");return 0;
}
