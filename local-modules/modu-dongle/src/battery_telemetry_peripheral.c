/* SPDX-License-Identifier: MIT
 * Read-only diagnostics on the EXISTING encrypted split BLE connection.
 * Original ZMK battery reporting remains intact. No new ADC pins, no ratio
 * guesses, no mock/fixed percentage, no changes to pairing or key processing.
 */
#include <errno.h>
#include <limits.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zmk/workqueue.h>
#include <zmk/event_manager.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#include "battery_telemetry_protocol.h"

LOG_MODULE_REGISTER(modu_battery_sensor, CONFIG_LOG_DEFAULT_LEVEL);
#if !DT_HAS_CHOSEN(zmk_battery)
#error "MODU battery telemetry requires the original zmk,battery chosen sensor"
#endif
#if IS_ENABLED(CONFIG_SHIELD_MODU_LEFT)
#define MODU_THIS_SIDE 0
#elif IS_ENABLED(CONFIG_SHIELD_MODU_RIGHT)
#define MODU_THIS_SIDE 1
#else
#error "Enable the peripheral battery service only for modu_left or modu_right"
#endif

static const struct device *const sensor = DEVICE_DT_GET(DT_CHOSEN(zmk_battery));
static struct k_spinlock detail_lock;
static struct modu_battery_detail detail = {
    .side = MODU_THIS_SIDE, .result = MODU_BAT_WAIT,
    .percent = 255, .age_seconds = UINT16_MAX,
};
static int64_t sampled_at = -1;
static void sample_work_fn(struct k_work *work);
K_WORK_DEFINE(modu_battery_sample_work, sample_work_fn);

static void queue_sample(void) {
    /* The built-in battery worker uses this same queue: do not race its ADC
     * fetch with another thread. Never do ADC I/O in the Bluetooth callback. */
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &modu_battery_sample_work);
}
static void sample_work_fn(struct k_work *work) {
    (void)work;
    struct modu_battery_detail next = {
        .side = MODU_THIS_SIDE, .result = MODU_BAT_NOT_READY, .percent = 255,
    };
    struct sensor_value voltage, charge;
    int rc = -ENODEV;
    if (!device_is_ready(sensor)) goto finished;
    rc = sensor_sample_fetch_chan(sensor, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE);
    if (rc) { next.result = MODU_BAT_FETCH_ERROR; goto finished; }
    rc = sensor_channel_get(sensor, SENSOR_CHAN_GAUGE_VOLTAGE, &voltage);
    if (rc) { next.result = MODU_BAT_VOLTAGE_ERROR; goto finished; }
    int64_t mv = (int64_t)voltage.val1 * 1000 + voltage.val2 / 1000;
    if (mv < 0 || mv > UINT16_MAX) {
        rc = -ERANGE; next.result = MODU_BAT_VALUE_ERROR; goto finished;
    }
    next.millivolts = (uint16_t)mv;
    rc = sensor_channel_get(sensor, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, &charge);
    if (rc) { next.result = MODU_BAT_PERCENT_ERROR; goto finished; }
    if (charge.val1 < 0 || charge.val1 > 100 || charge.val2 != 0) {
        rc = -ERANGE; next.result = MODU_BAT_VALUE_ERROR; goto finished;
    }
    next.percent = (uint8_t)charge.val1;
    next.result = MODU_BAT_OK;
finished:
    next.error = (int16_t)CLAMP(rc, INT16_MIN, INT16_MAX);
    k_spinlock_key_t key = k_spin_lock(&detail_lock);
    next.sequence = (uint16_t)(detail.sequence + 1);
    detail = next;
    sampled_at = k_uptime_get();
    k_spin_unlock(&detail_lock, key);
    LOG_DBG("hand=%u status=%u mv=%u pct=%u err=%d", next.side, next.result,
            next.millivolts, next.percent, next.error);
}
static void sample_timer_fn(struct k_timer *timer) {
    (void)timer;
    if (zmk_split_bt_peripheral_is_connected()) queue_sample();
}
K_TIMER_DEFINE(modu_battery_sample_timer, sample_timer_fn, NULL);

static ssize_t read_detail(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           void *buf, uint16_t length, uint16_t offset) {
    struct modu_battery_detail snapshot;
    k_spinlock_key_t key = k_spin_lock(&detail_lock);
    snapshot = detail;
    int64_t age = sampled_at < 0 ? UINT16_MAX : (k_uptime_get() - sampled_at) / 1000;
    snapshot.age_seconds = (uint16_t)MIN(age, UINT16_MAX);
    k_spin_unlock(&detail_lock, key);
    if (snapshot.result == MODU_BAT_WAIT || snapshot.age_seconds >= 30) queue_sample();
    uint8_t packet[MODU_BATTERY_PACKET_SIZE];
    modu_battery_encode(&snapshot, packet);
    return bt_gatt_attr_read(conn, attr, buf, length, offset, packet, sizeof(packet));
}
BT_GATT_SERVICE_DEFINE(modu_battery_telemetry_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_128(MODU_BATTERY_SERVICE_UUID)),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(MODU_BATTERY_VALUE_UUID),
        BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT, read_detail, NULL, NULL)
);
static int link_event(const zmk_event_t *event) {
    const struct zmk_split_peripheral_status_changed *link =
        as_zmk_split_peripheral_status_changed(event);
    if (link && link->connected) queue_sample();
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(modu_battery_telemetry_link, link_event);
ZMK_SUBSCRIPTION(modu_battery_telemetry_link, zmk_split_peripheral_status_changed);
static int telemetry_init(void) {
    k_timer_start(&modu_battery_sample_timer, K_SECONDS(2), K_SECONDS(30));
    return 0;
}
SYS_INIT(telemetry_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
