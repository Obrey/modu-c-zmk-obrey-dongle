/* SPDX-License-Identifier: MIT -- real LED adapter, mock PWM and scheduler. */
#include "battery_runtime_mock.h"
#include "../local-modules/modu-dongle/src/peripheral_status_led.c"
int main(void) {
    assert(init_status_led()==0);scheduled_calls=0;
    update_led(NULL);assert(pwm_calls==3 && scheduled_calls==0);
    unsigned green=CONFIG_SHIELD_MODU_RIGHT ? 0 : 1;
    assert(last_pulses[green]==18800000); /* 6% light, inverted 94% duty. */
    update_led(NULL);assert(pwm_calls==3); /* No redundant writes while connected. */
    mock_activity=ZMK_ACTIVITY_IDLE;status_led_event(NULL);update_led(NULL);
    for(int i=0;i<3;i++) assert(last_pulses[i]==20000000);
    int n=pwm_calls;scheduled_calls=0;
    for(int i=0;i<50;i++) update_led(NULL);
    assert(pwm_calls==n && scheduled_calls==0);
    mock_activity=ZMK_ACTIVITY_ACTIVE;status_led_event(NULL);update_led(NULL);
    assert(last_pulses[green]==18800000);
    mock_link=false;mock_clock=0;scheduled_calls=0;update_led(NULL);
    unsigned blue=CONFIG_SHIELD_MODU_RIGHT ? 1 : 0;
    assert(last_pulses[blue]==18800000 && scheduled_calls==1 && scheduled_delay_ms==100);
    mock_clock=700;update_led(NULL);assert(last_pulses[blue]==20000000);
    mock_activity=ZMK_ACTIVITY_IDLE;scheduled_calls=0;update_led(NULL);assert(scheduled_calls==0);
    mock_link=true;status_led_event(NULL);update_led(NULL);
    for(int i=0;i<3;i++) assert(last_pulses[i]==20000000); /* A link alone is not activity. */
    puts("PASS: active steady LED, idle off, no polling while connected/idle, wake and search blink");return 0;
}
