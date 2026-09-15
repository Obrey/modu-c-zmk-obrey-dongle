/* SPDX-License-Identifier: MIT -- exercises real adapter C with mock APIs. */
#include "battery_runtime_mock.h"
#include "../local-modules/modu-dongle/src/battery_telemetry_peripheral.c"
int main(void) {
    assert(telemetry_init()==0);
    uint8_t packet[12];struct modu_battery_detail d;
    assert(read_detail(NULL,NULL,packet,12,0)==12);
    assert(modu_battery_decode(packet,12,&d));assert(d.result==MODU_BAT_WAIT && d.percent==255);
    assert(fetch_calls==0 && queued_samples==1); /* GATT callback never reads ADC. */
    mock_clock=1000;sample_work_fn(NULL);
    read_detail(NULL,NULL,packet,12,0);assert(modu_battery_decode(packet,12,&d));
    assert(d.side==0 && d.result==MODU_BAT_OK && d.millivolts==3900 && d.percent==61 && d.sequence==1);
    mock_clock+=35000;read_detail(NULL,NULL,packet,12,0);modu_battery_decode(packet,12,&d);
    assert(d.age_seconds==35); /* Age is since sample, not since read. */
    mock_percent=0;mock_mv=3300;sample_work_fn(NULL);read_detail(NULL,NULL,packet,12,0);modu_battery_decode(packet,12,&d);
    assert(d.result==MODU_BAT_OK && d.percent==0 && d.millivolts==3300);
    mock_fetch_error=-5;sample_work_fn(NULL);read_detail(NULL,NULL,packet,12,0);modu_battery_decode(packet,12,&d);
    assert(d.result==MODU_BAT_FETCH_ERROR && d.percent==255 && d.error==-5);
    mock_fetch_error=0;mock_voltage_error=-95;sample_work_fn(NULL);assert(detail.result==MODU_BAT_VOLTAGE_ERROR);
    mock_voltage_error=0;mock_percent_error=-5;sample_work_fn(NULL);assert(detail.result==MODU_BAT_PERCENT_ERROR);
    mock_percent_error=0;mock_percent=101;sample_work_fn(NULL);assert(detail.result==MODU_BAT_VALUE_ERROR);
    mock_device.ready=0;sample_work_fn(NULL);assert(detail.result==MODU_BAT_NOT_READY && detail.percent==255);
    zmk_event_t e={.link={.connected=true}};assert(link_event(&e)==0);
    mock_link=false;int n=queued_samples;sample_timer_fn(NULL);assert(queued_samples==n);
    mock_link=true;sample_timer_fn(NULL);assert(queued_samples==n+1);
    puts("PASS: peripheral sample/error/voltage/age logic (mock sensor and BLE)");return 0;
}
