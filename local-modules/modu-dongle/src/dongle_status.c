/*
 * SPDX-License-Identifier: MIT
 * MODU-C MAC v3: split-aware SH1106 status screen for the pinned ZMK/LVGL 9.
 * No LVGL calls run in Bluetooth/event callbacks; the display queue owns UI.
 * The MAC mark is a fixed user-selected label, NOT automatic OS detection.
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
#include <zmk/endpoints.h>
#include <zmk/usb.h>
#include <zmk/keymap.h>
#include <zmk/display/status_screen.h>
#include "status_logic.h"

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

/* Fixed objects: refresh changes text only, without allocating new widgets. */
static lv_obj_t *host_label, *half_labels[MODU_SIDES], *power_label, *layer_label;
static lv_obj_t *make_label(lv_obj_t *parent, int x, int y, int width) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_remove_style_all(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, width, 12);
    return label;
}
static void rect(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *object = lv_obj_create(parent);
    lv_obj_remove_style_all(object);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, w, h);
    lv_obj_set_style_bg_color(object, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
}
static void make_mac_mark(lv_obj_t *parent) {
    /* Original tiny monochrome bitten-fruit mark: no private-use font glyph
     * or external icon/font file dependency. Each row is a 12-bit bitmap. */
    static const uint16_t rows[] = {
        0x030, 0x060, 0x040, 0x3BC, 0x7FE, 0xFFC,
        0xFF8, 0xFF8, 0xFFC, 0x7FE, 0x7FE, 0x3FC, 0x198,
    };
    for (unsigned y = 0; y < ARRAY_SIZE(rows); y++) {
        for (int x = 0; x < 12;) {
            if (!(rows[y] & (1u << (11 - x)))) { x++; continue; }
            int start = x;
            while (x < 12 && (rows[y] & (1u << (11 - x)))) x++;
            rect(parent, start, (int)y, x - start, 1);
        }
    }
}
static void refresh_screen(lv_timer_t *timer) {
    (void)timer;
    struct live_peer live[SOURCE_COUNT];
    struct battery_sample values[SOURCE_COUNT];
    struct modu_identities ids;
    collect_peers(live);
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    ids = identities;
    for (int i = 0; i < SOURCE_COUNT; i++) {
        if (!live[i].connected ||
            memcmp(samples[i].peer, live[i].peer, MODU_PEER_BYTES) != 0) samples[i].valid = false;
        values[i] = samples[i];
    }
#if IS_ENABLED(CONFIG_MODU_DONGLE_HAS_BATTERY)
    bool local_valid = dongle_battery_valid;
    uint8_t local_percent = dongle_battery_percent;
#endif
    k_spin_unlock(&state_lock, key);
    char text[40];
    struct zmk_endpoint_instance endpoint = zmk_endpoint_get_selected();
    bool host_connected = zmk_endpoint_is_connected();
    switch (endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        snprintf(text, sizeof(text), "USB %s", host_connected ? "ON" : "WAIT");
        break;
    case ZMK_TRANSPORT_BLE:
        snprintf(text, sizeof(text), "BT%d %s", endpoint.ble.profile_index + 1,
                 host_connected ? "ON" : "WAIT");
        break;
    default:
        snprintf(text, sizeof(text), "NO HOST");
        break;
    }
    lv_label_set_text(host_label, text);
    for (int side = 0; side < MODU_SIDES; side++) {
        int source = -1;
        for (int i = 0; i < SOURCE_COUNT; i++) {
            if (live[i].connected && modu_side_for_peer(&ids, live[i].peer) == side) {
                source = i;
                break;
            }
        }
        bool connected = source >= 0;
        bool valid = connected && values[source].valid;
        uint8_t percent = valid ? values[source].percent : 0;
        modu_format_half(text, sizeof(text), side, ids.side[side].known, connected, valid, percent);
        lv_label_set_text(half_labels[side], text);
    }
#if IS_ENABLED(CONFIG_MODU_DONGLE_HAS_BATTERY)
    if (local_valid) snprintf(text, sizeof(text), "D: %3u%% %s", (unsigned)local_percent,
                               zmk_usb_is_powered() ? "USB" : "BAT");
    else snprintf(text, sizeof(text), "D:  --%% %s", zmk_usb_is_powered() ? "USB" : "BAT");
#else
    snprintf(text, sizeof(text), "D: %s", zmk_usb_is_powered() ? "USB POWER" : "EXT POWER");
#endif
    lv_label_set_text(power_label, text);
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();
    zmk_keymap_layer_id_t id = zmk_keymap_layer_index_to_id(index);
    const char *name = zmk_keymap_layer_name(id);
    snprintf(text, sizeof(text), "%u %.11s", (unsigned)index, name ? name : "Layer");
    lv_label_set_text(layer_label, text);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, 128, 64);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    make_mac_mark(screen);
    lv_obj_t *title = make_label(screen, 16, 1, 46);
    lv_label_set_text(title, "MAC v3");
    host_label = make_label(screen, 70, 1, 58);
    rect(screen, 16, 13, 112, 1);
    half_labels[0] = make_label(screen, 0, 16, 128);
    half_labels[1] = make_label(screen, 0, 28, 128);
    power_label = make_label(screen, 0, 40, 128);
    layer_label = make_label(screen, 0, 52, 82);
    lv_obj_t *fingerprint = make_label(screen, 87, 52, 41);
    char short_hash[7];
    snprintf(short_hash, sizeof(short_hash), "%.6s", MODU_CONFIG_KEYMAP_SHA256);
    lv_label_set_text(fingerprint, short_hash);
    refresh_screen(NULL);
    lv_timer_t *refresh_timer = lv_timer_create(refresh_screen, 250, NULL);
    if (!refresh_timer) LOG_ERR("Status refresh timer allocation failed");
    return screen;
}
