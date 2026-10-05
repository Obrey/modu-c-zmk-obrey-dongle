/* SPDX-License-Identifier: MIT
 * MODU-C universal inactivity sleep.
 *
 * Upstream ZMK intentionally skips automatic deep sleep while USB power is
 * present. The MODU-C left/right halves are BLE peripherals even when a USB
 * cable is attached, so v11 adds an inactivity guard that enters the same
 * suspend + System OFF sequence regardless of VBUS state.
 *
 * Normal ZMK activity tracking remains authoritative. We only arm this timer
 * after ZMK has entered IDLE and cancel it immediately when activity resumes.
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/logging/log.h>

#include <zmk/activity.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/pm.h>

LOG_MODULE_REGISTER(modu_universal_sleep, CONFIG_ZMK_LOG_LEVEL);

BUILD_ASSERT(CONFIG_MODU_UNIVERSAL_DEEP_SLEEP_TIMEOUT_MS > CONFIG_ZMK_IDLE_TIMEOUT,
             "Universal deep-sleep timeout must be longer than ZMK idle timeout");

#define MODU_SLEEP_AFTER_IDLE_MS \
    (CONFIG_MODU_UNIVERSAL_DEEP_SLEEP_TIMEOUT_MS - CONFIG_ZMK_IDLE_TIMEOUT)

static void universal_sleep_work_fn(struct k_work *work) {
    (void)work;

    /* An activity event may have raced with this work item. Never power off
     * unless ZMK still considers the keyboard idle. */
    if (zmk_activity_get_state() != ZMK_ACTIVITY_IDLE) {
        return;
    }

    LOG_INF("Universal inactivity timeout reached; entering System OFF");
    if (zmk_pm_suspend_devices() < 0) {
        LOG_ERR("Failed to suspend devices before universal System OFF");
        zmk_pm_resume_devices();
        return;
    }

    sys_poweroff();
}

K_WORK_DELAYABLE_DEFINE(modu_universal_sleep_work, universal_sleep_work_fn);

static int universal_sleep_activity_event(const zmk_event_t *event) {
    const struct zmk_activity_state_changed *changed = as_zmk_activity_state_changed(event);
    if (!changed) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (changed->state == ZMK_ACTIVITY_IDLE) {
        k_work_reschedule(&modu_universal_sleep_work, K_MSEC(MODU_SLEEP_AFTER_IDLE_MS));
    } else {
        k_work_cancel_delayable(&modu_universal_sleep_work);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(modu_universal_sleep_activity, universal_sleep_activity_event);
ZMK_SUBSCRIPTION(modu_universal_sleep_activity, zmk_activity_state_changed);
