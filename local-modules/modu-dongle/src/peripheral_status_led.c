/*
 * Unofficial MODU-C peripheral LED replacement.
 * GPIO aliases, inverted PWM level, and L/R channel order follow the vendor
 * implementation. Original hardware material: (c) 2026 EKS Inc., Ryu.
 * SPDX-License-Identifier: LicenseRef-EKS-NonCommercial-1.0
 */
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zmk/activity.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#include "status_logic.h"
LOG_MODULE_REGISTER(modu_link_led, CONFIG_LOG_DEFAULT_LEVEL);
BUILD_ASSERT(!IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL), "This LED driver is peripheral-only");
BUILD_ASSERT(!IS_ENABLED(CONFIG_ZMK_SLEEP),
             "MODU-C polling hardware: validate wake sources before enabling system-off sleep");
static const struct pwm_dt_spec leds[] = {
    PWM_DT_SPEC_GET(DT_ALIAS(led_status1)),
    PWM_DT_SPEC_GET(DT_ALIAS(led_status2)),
    PWM_DT_SPEC_GET(DT_ALIAS(led_status3)),
};
static bool led_ready;
static int last_level[3] = {-1, -1, -1};
static void update_led(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(status_led_work, update_led);
static void update_led(struct k_work *work) {
    (void)work;
    if (!led_ready) return;
    const bool active = zmk_activity_get_state() == ZMK_ACTIVITY_ACTIVE;
    const bool connected = zmk_split_bt_peripheral_is_connected();
    const bool bonded = zmk_split_bt_peripheral_is_bonded();
    const bool right = IS_ENABLED(CONFIG_SHIELD_MODU_RIGHT);
    unsigned channel = modu_led_channel(right, connected);
    bool visible = active && modu_led_visible(connected, bonded, k_uptime_get_32());
    for (unsigned i = 0; i < ARRAY_SIZE(leds); i++) {
        uint32_t level = (visible && i == channel) ? CONFIG_MODU_STATUS_LED_BRIGHTNESS : 0;
        if (last_level[i] == (int)level) continue;
        /* Active-low wiring: 100% electrical duty = LED OFF. */
        uint32_t pulse = (uint32_t)((uint64_t)leds[i].period * (100u - level) / 100u);
        int err = pwm_set_pulse_dt(&leds[i], pulse);
        if (!err) last_level[i] = (int)level;
        else LOG_DBG("Status LED write failed: %d", err);
    }
    /* Connected steady indication and idle OFF require no periodic work.
     * Link/activity events schedule an update. Only an active search blinks. */
    if (active && !connected) k_work_schedule(&status_led_work, K_MSEC(100));
}
static int status_led_event(const zmk_event_t *event) {
    (void)event;
    if (led_ready) k_work_reschedule(&status_led_work, K_NO_WAIT);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(modu_led_power_events, status_led_event);
ZMK_SUBSCRIPTION(modu_led_power_events, zmk_activity_state_changed);
ZMK_SUBSCRIPTION(modu_led_power_events, zmk_split_peripheral_status_changed);
static int init_status_led(void) {
    for (unsigned i = 0; i < ARRAY_SIZE(leds); i++) {
        if (!device_is_ready(leds[i].dev)) {
            LOG_WRN("PWM not ready; status LED disabled, keyboard can still operate");
            return 0;
        }
    }
    led_ready = true;
    k_work_schedule(&status_led_work, K_MSEC(100));
    return 0;
}
SYS_INIT(init_status_led, APPLICATION, 99);
