/*
 * SPDX-License-Identifier: MIT
 * MODU-C v4: D/L/R battery widget inside the ORIGINAL Corne dongle UI.
 * No LVGL calls run in Bluetooth/event callbacks; the display queue owns UI.
 * Original layout, modifier symbols and connection widgets live upstream.
 */
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/usb.h>
#include "battery_status.h"
#include "status_logic.h"
#include "battery_telemetry.h"

LOG_MODULE_REGISTER(modu_status, CONFIG_LOG_DEFAULT_LEVEL);
#define SOURCE_COUNT CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS
BUILD_ASSERT(SOURCE_COUNT == 2, "This screen is for exactly two MODU-C halves");
/* This global function exists in the PINNED upstream central.c. Upstream has
 * not exposed it in a public header. No upstream source patch is applied.
 * Recheck this adapter before updating the ZMK revision in west.yml. */
extern int peripheral_slot_index_for_conn(struct bt_conn *conn);

struct live_peer {
    bool connected;
    uint8_t peer[MODU_PEER_BYTES];
};
struct battery_sample {
    bool valid;
    uint8_t percent;
    uint8_t peer[MODU_PEER_BYTES];
};
static struct k_spinlock state_lock;
static struct modu_identities identities = {.version = 1};
static struct battery_sample samples[SOURCE_COUNT];
#if IS_ENABLED(CONFIG_MODU_DONGLE_HAS_BATTERY)
static bool dongle_battery_valid;
static uint8_t dongle_battery_percent;
#endif

static void peer_bytes(const bt_addr_le_t *addr, uint8_t peer[MODU_PEER_BYTES]) {
    /* Explicit stable serialization: no compiler padding is persisted. */
    peer[0] = addr->type;
    memcpy(&peer[1], addr->a.val, 6);
}
static void collect_one_peer(struct bt_conn *conn, void *context) {
    struct live_peer *live = context;
    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info) != 0 || info.type != BT_CONN_TYPE_LE ||
        info.state != BT_CONN_STATE_CONNECTED || info.role != BT_CONN_ROLE_CENTRAL)
        return; /* Exclude the dongle's BLE HOST connection. */
    int source = peripheral_slot_index_for_conn(conn);
    if (source < 0 || source >= SOURCE_COUNT) return;
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    if (!addr) return;
    live[source].connected = true;
    peer_bytes(addr, live[source].peer);
}
static void collect_peers(struct live_peer live[SOURCE_COUNT]) {
    memset(live, 0, sizeof(struct live_peer) * SOURCE_COUNT);
    bt_conn_foreach(BT_CONN_TYPE_LE, collect_one_peer, live);
}

#if IS_ENABLED(CONFIG_SETTINGS)
static int modu_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    struct modu_identities loaded;
    if (strcmp(name, "peers") != 0) return -ENOENT;
    if (len != sizeof(loaded)) return -EINVAL;
    int got = read_cb(cb_arg, &loaded, sizeof(loaded));
    if (got < 0) return got;
    if ((size_t)got != sizeof(loaded) || !modu_valid_identities(&loaded)) return -EINVAL;
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    identities = loaded;
    k_spin_unlock(&state_lock, key);
    return 0;
}
SETTINGS_STATIC_HANDLER_DEFINE(modu_status_settings, "modu_status", NULL,
                               modu_settings_set, NULL, NULL);
static void save_identities(struct k_work *work) {
    (void)work;
    struct modu_identities snapshot;
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    snapshot = identities;
    k_spin_unlock(&state_lock, key);
    int err = settings_save_one("modu_status/peers", &snapshot, sizeof(snapshot));
    if (err) LOG_WRN("Could not save L/R identity (%d); press a key on each side next boot", err);
}
K_WORK_DELAYABLE_DEFINE(save_identities_work, save_identities);
#endif

static int status_event(const zmk_event_t *event) {
    const struct zmk_peripheral_battery_state_changed *battery =
        as_zmk_peripheral_battery_state_changed(event);
    if (battery && battery->source < SOURCE_COUNT) {
        struct live_peer live[SOURCE_COUNT];
        collect_peers(live);
        unsigned source = battery->source;
        k_spinlock_key_t key = k_spin_lock(&state_lock);
        samples[source].valid = live[source].connected && battery->state_of_charge <= 100;
        if (samples[source].valid) {
            samples[source].percent = battery->state_of_charge;
            memcpy(samples[source].peer, live[source].peer, MODU_PEER_BYTES);
        }
        k_spin_unlock(&state_lock, key);
        return ZMK_EV_EVENT_BUBBLE;
    }
    const struct zmk_position_state_changed *position = as_zmk_position_state_changed(event);
    if (position && position->state && position->source < SOURCE_COUNT) {
        int side = modu_side_for_position(position->position);
        if (side == MODU_SIDE_UNKNOWN) return ZMK_EV_EVENT_BUBBLE;
        struct live_peer live[SOURCE_COUNT];
        collect_peers(live);
        if (live[position->source].connected) {
            k_spinlock_key_t key = k_spin_lock(&state_lock);
            bool changed = modu_learn_peer(&identities, side, live[position->source].peer);
            k_spin_unlock(&state_lock, key);
            if (changed) {
                LOG_INF("Identified %s hand at split source %u", side == 0 ? "LEFT" : "RIGHT",
                        (unsigned)position->source);
#if IS_ENABLED(CONFIG_SETTINGS)
                k_work_reschedule(&save_identities_work, K_MSEC(500));
#endif
            }
        }
    }
#if IS_ENABLED(CONFIG_MODU_DONGLE_HAS_BATTERY)
    const struct zmk_battery_state_changed *local = as_zmk_battery_state_changed(event);
    if (local) {
        k_spinlock_key_t key = k_spin_lock(&state_lock);
        dongle_battery_valid = local->state_of_charge <= 100;
        dongle_battery_percent = local->state_of_charge;
        k_spin_unlock(&state_lock, key);
    }
#endif
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(modu_status_events, status_event);
ZMK_SUBSCRIPTION(modu_status_events, zmk_peripheral_battery_state_changed);
ZMK_SUBSCRIPTION(modu_status_events, zmk_position_state_changed);
#if IS_ENABLED(CONFIG_MODU_DONGLE_HAS_BATTERY)
ZMK_SUBSCRIPTION(modu_status_events, zmk_battery_state_changed);
#endif

static void peer_disconnected(struct bt_conn *conn, uint8_t reason) {
    (void)reason;
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    if (!addr) return;
    uint8_t peer[MODU_PEER_BYTES];
    peer_bytes(addr, peer);
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    for (int i = 0; i < SOURCE_COUNT; i++)
        if (memcmp(samples[i].peer, peer, MODU_PEER_BYTES) == 0) samples[i].valid = false;
    k_spin_unlock(&state_lock, key);
}
BT_CONN_CB_DEFINE(modu_status_conn_callbacks) = {.disconnected = peer_disconnected};

/* Fixed objects: only the existing top-right battery area is replaced. */
static lv_obj_t *half_labels[MODU_SIDES], *power_label;
static bool widget_initialized;
static lv_obj_t *make_battery_label(lv_obj_t *parent, int y) {
    lv_obj_t *label = lv_label_create(parent);
    if (!label) return NULL;
    lv_obj_remove_style_all(label);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    lv_obj_set_style_text_letter_space(label, 0, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(label, 0, y);
    lv_obj_set_size(label, 56, 9);
    return label;
}
static void refresh_batteries(lv_timer_t *timer) {
    (void)timer;
    struct live_peer live[SOURCE_COUNT];
#if !IS_ENABLED(CONFIG_MODU_BATTERY_TELEMETRY_CENTRAL)
    struct battery_sample values[SOURCE_COUNT];
#endif
    struct modu_identities ids;
    collect_peers(live);
#if IS_ENABLED(CONFIG_MODU_BATTERY_TELEMETRY_CENTRAL)
    bool identity_changed = false;
    for (int i = 0; i < SOURCE_COUNT; i++) {
        struct modu_battery_detail d;
        if (live[i].connected && modu_battery_detail_for_peer(live[i].peer, &d)) {
            k_spinlock_key_t id_key = k_spin_lock(&state_lock);
            identity_changed |= modu_learn_peer(&identities, d.side, live[i].peer);
            k_spin_unlock(&state_lock, id_key);
        }
    }
#if IS_ENABLED(CONFIG_SETTINGS)
    if (identity_changed) k_work_reschedule(&save_identities_work, K_MSEC(500));
#else
    (void)identity_changed;
#endif
#endif
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    ids = identities;
    for (int i = 0; i < SOURCE_COUNT; i++) {
        if (!live[i].connected ||
            memcmp(samples[i].peer, live[i].peer, MODU_PEER_BYTES) != 0) samples[i].valid = false;
#if !IS_ENABLED(CONFIG_MODU_BATTERY_TELEMETRY_CENTRAL)
        values[i] = samples[i];
#endif
    }
#if IS_ENABLED(CONFIG_MODU_DONGLE_HAS_BATTERY)
    bool local_valid = dongle_battery_valid;
    uint8_t local_percent = dongle_battery_percent;
#endif
    k_spin_unlock(&state_lock, key);
    char text[16];
    if (power_label) {
#if IS_ENABLED(CONFIG_MODU_DONGLE_HAS_BATTERY)
        if (local_valid) snprintf(text, sizeof(text), "D %3u%%", (unsigned)local_percent);
        else snprintf(text, sizeof(text), "D  --%%");
#else
        snprintf(text, sizeof(text), "D  %s", zmk_usb_is_powered() ? "USB" : "EXT");
#endif
        lv_label_set_text(power_label, text);
    }
    for (int side = 0; side < MODU_SIDES; side++) {
        int source = -1;
        for (int i = 0; i < SOURCE_COUNT; i++) {
            if (live[i].connected && modu_side_for_peer(&ids, live[i].peer) == side) {
                source = i;
                break;
            }
        }
        char hand = side == 0 ? 'L' : 'R';
        if (source < 0 && ids.side[side].known)
            snprintf(text, sizeof(text), "%c OFF", hand);
#if IS_ENABLED(CONFIG_MODU_BATTERY_TELEMETRY_CENTRAL)
        else {
            struct modu_battery_detail d;
            bool have_detail = source >= 0 && modu_battery_detail_for_peer(live[source].peer, &d);
            modu_battery_format(text, sizeof(text), hand, have_detail ? &d : NULL,
                                ((k_uptime_get() / 3000) % 2) != 0);
        }
#else
        else if (source < 0 || !values[source].valid)
            snprintf(text, sizeof(text), "%c  --%%", hand);
        else
            snprintf(text, sizeof(text), "%c %3u%%", hand, (unsigned)values[source].percent);
#endif
        if (half_labels[side]) lv_label_set_text(half_labels[side], text);
    }
}
int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *widget,
                                         lv_obj_t *parent) {
    if (widget_initialized) return -EALREADY;
    widget->obj = lv_obj_create(parent);
    if (!widget->obj) return -ENOMEM;
    lv_obj_remove_style_all(widget->obj);
    lv_obj_remove_flag(widget->obj, LV_OBJ_FLAG_SCROLLABLE);
    /* The original status screen aligns this object TOP_RIGHT. */
#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY)
    const int offset = 1;
    power_label = make_battery_label(widget->obj, 0);
    if (!power_label) return -ENOMEM;
#else
    const int offset = 0;
#endif
    lv_obj_set_size(widget->obj, 56, (MODU_SIDES + offset) * 10);
    for (int side = 0; side < MODU_SIDES; side++) {
        half_labels[side] = make_battery_label(widget->obj, (side + offset) * 10);
        if (!half_labels[side]) return -ENOMEM;
    }
    widget_initialized = true;
    refresh_batteries(NULL);
    if (!lv_timer_create(refresh_batteries, 500, NULL)) {
        LOG_ERR("Battery refresh timer allocation failed");
        return -ENOMEM;
    }
    return 0;
}
lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}
