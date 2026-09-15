/* SPDX-License-Identifier: MIT
 * Periodically read each hand's actual sensor result over its existing bonded
 * BLE connection. A dedicated low-priority queue keeps GATT waits off the HID,
 * system and display queues. The original ZMK BAS subscription is untouched.
 */
#include <string.h>
#include <limits.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include "battery_telemetry.h"

LOG_MODULE_REGISTER(modu_battery_client, CONFIG_LOG_DEFAULT_LEVEL);
#define BAT_CLIENTS CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS
BUILD_ASSERT(BAT_CLIENTS == 2, "MODU battery client expects two keyboard halves");
/* Adapter already used by v3-v5; valid for ZMK 641514a97db3 only. */
extern int peripheral_slot_index_for_conn(struct bt_conn *conn);
static struct bt_uuid_128 detail_uuid = BT_UUID_INIT_128(MODU_BATTERY_VALUE_UUID);

struct battery_client {
    struct bt_gatt_read_params params;
    struct bt_conn *request_conn;
    uint8_t request_peer[MODU_BATTERY_PEER_BYTES];
    atomic_t state; /* 0 idle, 1 in-flight, 2 callback finished */
    int64_t request_at, next_due;
    int64_t completed_at; /* Published before state becomes 2. */
};
struct battery_cache {
    bool valid;
    uint8_t peer[MODU_BATTERY_PEER_BYTES];
    struct modu_battery_detail detail;
    int64_t received_at;
};
static struct battery_client clients[BAT_CLIENTS];
static struct battery_cache cache[BAT_CLIENTS];
static struct k_spinlock cache_lock;
static struct k_work_q client_queue;
K_THREAD_STACK_DEFINE(client_stack, 2048);

static bool serialize_peer(struct bt_conn *conn, uint8_t out[MODU_BATTERY_PEER_BYTES]) {
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    if (!addr) return false;
    out[0] = addr->type;
    memcpy(out + 1, addr->a.val, 6);
    return true;
}
bool modu_battery_detail_for_peer(const uint8_t peer[MODU_BATTERY_PEER_BYTES],
                                 struct modu_battery_detail *out) {
    bool found = false;
    k_spinlock_key_t key = k_spin_lock(&cache_lock);
    for (int i = 0; i < BAT_CLIENTS; i++) {
        if (!cache[i].valid || memcmp(cache[i].peer, peer, MODU_BATTERY_PEER_BYTES)) continue;
        *out = cache[i].detail;
        int64_t age = out->age_seconds + (k_uptime_get() - cache[i].received_at) / 1000;
        out->age_seconds = (uint16_t)MIN(age, UINT16_MAX);
        found = true;
        break;
    }
    k_spin_unlock(&cache_lock, key);
    return found;
}
static uint8_t read_reply(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
                          const void *data, uint16_t length) {
    struct battery_client *client = CONTAINER_OF(params, struct battery_client, params);
    const size_t index = (size_t)(client - clients);
    struct modu_battery_detail detail;
    struct bt_conn_info info;
    /* Do not accept a reply that arrives while an old link is being removed. */
    if (!err && bt_conn_get_info(conn, &info) == 0 && info.state == BT_CONN_STATE_CONNECTED &&
        modu_battery_decode(data, length, &detail)) {
        k_spinlock_key_t key = k_spin_lock(&cache_lock);
        cache[index].valid = true;
        cache[index].detail = detail;
        cache[index].received_at = k_uptime_get();
        memcpy(cache[index].peer, client->request_peer, MODU_BATTERY_PEER_BYTES);
        k_spin_unlock(&cache_lock, key);
        LOG_DBG("hand=%u mv=%u pct=%u result=%u err=%d age=%u", detail.side,
                detail.millivolts, detail.percent, detail.result, detail.error, detail.age_seconds);
    } else if (err) {
        LOG_DBG("Battery detail read source %u failed ATT=%u", (unsigned)index, err);
    }
    client->completed_at = k_uptime_get();
    atomic_set(&client->state, 2);
    return BT_GATT_ITER_STOP;
}
static void poll_one_connection(struct bt_conn *conn, void *unused) {
    (void)unused;
    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info) || info.type != BT_CONN_TYPE_LE ||
        info.state != BT_CONN_STATE_CONNECTED || info.role != BT_CONN_ROLE_CENTRAL ||
        bt_conn_get_security(conn) < BT_SECURITY_L2) return;
    const int index = peripheral_slot_index_for_conn(conn);
    if (index < 0 || index >= BAT_CLIENTS) return; /* Excludes host connections. */
    struct battery_client *client = &clients[index];
    if (atomic_get(&client->state) != 0 || k_uptime_get() < client->next_due) return;
    uint8_t peer[MODU_BATTERY_PEER_BYTES];
    if (!serialize_peer(conn, peer)) return;
    /* Invalidate a sample before reusing a source slot for a different peer. */
    k_spinlock_key_t key = k_spin_lock(&cache_lock);
    if (cache[index].valid && memcmp(cache[index].peer, peer, sizeof(peer)))
        cache[index].valid = false;
    k_spin_unlock(&cache_lock, key);
    memcpy(client->request_peer, peer, sizeof(peer));
    memset(&client->params, 0, sizeof(client->params));
    client->params.func = read_reply;
    client->params.handle_count = 0;
    client->params.by_uuid.start_handle = 1;
    client->params.by_uuid.end_handle = 0xffff;
    client->params.by_uuid.uuid = &detail_uuid.uuid;
    client->request_conn = bt_conn_ref(conn);
    client->request_at = k_uptime_get();
    client->next_due = client->request_at + 10000;
    atomic_set(&client->state, 1);
    int err = bt_gatt_read(conn, &client->params);
    if (err) { /* Rejected requests do not receive a callback. */
        bt_conn_unref(client->request_conn);
        client->request_conn = NULL;
        atomic_set(&client->state, 0);
    }
}
static void client_poll(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(client_poll_work, client_poll);
static void client_poll(struct k_work *work) {
    (void)work;
    for (int i = 0; i < BAT_CLIENTS; i++) {
        struct battery_client *client = &clients[i];
        /* Retain read parameters and connection until the callback is complete.
         * Never overwrite an in-flight request on disconnect or a timer tick.
         * ATT has its own timeout; there is no unsafe forced-slot reuse here. */
        if (atomic_get(&client->state) == 2 && k_uptime_get() - client->completed_at >= 1000) {
            bt_conn_unref(client->request_conn);
            client->request_conn = NULL;
            atomic_set(&client->state, 0);
        }
    }
    bt_conn_foreach(BT_CONN_TYPE_LE, poll_one_connection, NULL);
    k_work_reschedule_for_queue(&client_queue, &client_poll_work, K_SECONDS(2));
}
static void telemetry_disconnected(struct bt_conn *conn, uint8_t reason) {
    (void)reason;
    uint8_t peer[MODU_BATTERY_PEER_BYTES];
    if (!serialize_peer(conn, peer)) return;
    k_spinlock_key_t key = k_spin_lock(&cache_lock);
    for (int i = 0; i < BAT_CLIENTS; i++)
        if (!memcmp(cache[i].peer, peer, sizeof(peer))) cache[i].valid = false;
    k_spin_unlock(&cache_lock, key);
    /* In-flight read params live until their error callback, including across
     * disconnect. Zephyr calls the read callback when an ATT request fails. */
}
BT_CONN_CB_DEFINE(modu_battery_client_callbacks) = {.disconnected = telemetry_disconnected};
static int client_init(void) {
    k_work_queue_start(&client_queue, client_stack, K_THREAD_STACK_SIZEOF(client_stack),
                       K_LOWEST_APPLICATION_THREAD_PRIO, NULL);
    /* Allow the original key/trackball service discovery to run first. */
    k_work_reschedule_for_queue(&client_queue, &client_poll_work, K_SECONDS(8));
    return 0;
}
SYS_INIT(client_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
