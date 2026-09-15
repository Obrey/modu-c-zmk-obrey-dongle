/* SPDX-License-Identifier: MIT -- exercises real adapter C with mock APIs. */
#include "battery_runtime_mock.h"
#include "../local-modules/modu-dongle/src/battery_telemetry_peripheral.c"
int main(void) {
    assert(telemetry_init()==0);
    uint8_t packet[MODU_BATTERY_PACKET_SIZE];struct modu_battery_detail d;
    assert(read_detail(NULL,NULL,packet,MODU_BATTERY_PACKET_SIZE,0)==MODU_BATTERY_PACKET_SIZE);
    assert(modu_battery_decode(packet,MODU_BATTERY_PACKET_SIZE,&d));assert(d.result==MODU_BAT_WAIT && d.percent==255);
    assert(fetch_calls==0 && queued_samples==1); /* GATT callback never reads ADC. */
    mock_clock=1000;sample_work_fn(NULL);
    read_detail(NULL,NULL,packet,MODU_BATTERY_PACKET_SIZE,0);assert(modu_battery_decode(packet,MODU_BATTERY_PACKET_SIZE,&d));
    assert(d.side==0 && d.result==MODU_BAT_OK && d.millivolts==3900 && d.percent==61 && d.sequence==1);
    mock_clock+=35000;read_detail(NULL,NULL,packet,MODU_BATTERY_PACKET_SIZE,0);modu_battery_decode(packet,MODU_BATTERY_PACKET_SIZE,&d);
    assert(d.age_seconds==35); /* Age is since sample, not since read. */
    mock_percent=0;mock_mv=3300;sample_work_fn(NULL);read_detail(NULL,NULL,packet,MODU_BATTERY_PACKET_SIZE,0);modu_battery_decode(packet,MODU_BATTERY_PACKET_SIZE,&d);
    assert(d.result==MODU_BAT_OK && d.percent==0 && d.millivolts==3300);
    mock_fetch_error=-5;sample_work_fn(NULL);read_detail(NULL,NULL,packet,MODU_BATTERY_PACKET_SIZE,0);modu_battery_decode(packet,MODU_BATTERY_PACKET_SIZE,&d);
    assert(d.result==MODU_BAT_FETCH_ERROR && d.percent==255 && d.error==-5);
    mock_fetch_error=0;mock_voltage_error=-95;sample_work_fn(NULL);assert(detail.result==MODU_BAT_VOLTAGE_ERROR);
    mock_voltage_error=0;mock_percent_error=-5;sample_work_fn(NULL);assert(detail.result==MODU_BAT_PERCENT_ERROR);
    mock_percent_error=0;mock_percent=101;sample_work_fn(NULL);assert(detail.result==MODU_BAT_VALUE_ERROR);
    mock_device.ready=0;sample_work_fn(NULL);assert(detail.result==MODU_BAT_NOT_READY && detail.percent==255);
    zmk_event_t e={.link={.connected=true}};assert(link_event(&e)==0);
    mock_link=false;int n=fetch_calls;sample_work_fn(NULL);assert(fetch_calls==n);
    mock_link=true;mock_device.ready=1;mock_percent=0;mock_mv=3320;
    mock_activity=ZMK_ACTIVITY_ACTIVE;mock_clock=100000;sample_work_fn(NULL);
    assert(scheduled_delay_ms==60000);
    mock_activity=ZMK_ACTIVITY_IDLE;activity_event(NULL);assert(scheduled_delay_ms==300000);
    n=queued_samples;mock_clock+=150000;
    for(int i=0;i<20;i++) read_detail(NULL,NULL,packet,MODU_BATTERY_PACKET_SIZE,0);
    assert(queued_samples==n); /* BLE polling does not defeat standby sampling. */
    modu_battery_decode(packet,MODU_BATTERY_PACKET_SIZE,&d);
    assert(d.flags==MODU_BATTERY_FLAG_IDLE && d.age_seconds==150 && d.millivolts==3320);
    sample_work_fn(NULL);assert(scheduled_delay_ms==300000);
    mock_activity=ZMK_ACTIVITY_ACTIVE;activity_event(NULL);assert(queued_samples==n+1);
    read_detail(NULL,NULL,packet,MODU_BATTERY_PACKET_SIZE,0);modu_battery_decode(packet,MODU_BATTERY_PACKET_SIZE,&d);
    assert(d.flags==0);
    mock_activity=ZMK_ACTIVITY_SLEEP;activity_event(NULL);
    n=fetch_calls;sample_work_fn(NULL);assert(fetch_calls==n && cancelled_work>0);
    puts("PASS: peripheral active/idle/wake/disconnect sample intervals and cached reads (mock sensor and BLE)");return 0;
}
