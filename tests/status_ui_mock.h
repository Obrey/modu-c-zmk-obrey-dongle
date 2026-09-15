/* SPDX-License-Identifier: MIT
 * Native test doubles only. These are NOT Zephyr/LVGL implementations.
 * API signatures used below were checked against the pinned upstream headers.
 */
#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CONFIG_LOG_DEFAULT_LEVEL 1
#define CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS 2
#ifndef CONFIG_MODU_DONGLE_HAS_BATTERY
#define CONFIG_MODU_DONGLE_HAS_BATTERY 0
#endif
#define CONFIG_SETTINGS 1
#define CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY 1
#define MODU_CONFIG_KEYMAP_SHA256 "1690544cd320d7216727f1c5ca50aec82c572684b15e639293a656b11ecf9985"
#define IS_ENABLED(x) (x)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define BUILD_ASSERT(c, m) _Static_assert(c, m)
#define LOG_MODULE_REGISTER(...)
#define LOG_INF(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define LOG_ERR(...) ((void)0)
struct k_spinlock { int unused; };
typedef int k_spinlock_key_t;
static inline int k_spin_lock(struct k_spinlock *s) { (void)s; return 0; }
static inline void k_spin_unlock(struct k_spinlock *s, int k) { (void)s; (void)k; }
struct k_work { int unused; };
struct k_work_delayable { void (*callback)(struct k_work *); };
#define K_WORK_DELAYABLE_DEFINE(n, cb) struct k_work_delayable n = {.callback=cb}
#define K_MSEC(x) (x)
static inline int k_work_reschedule(struct k_work_delayable *w, int delay) { (void)w; (void)delay; return 0; }
typedef int (*settings_read_cb)(void *, void *, size_t);
static unsigned save_calls;
static unsigned char saved_settings[128];
static size_t saved_len;
static inline int settings_save_one(const char *k, const void *p, size_t n) {
    assert(strcmp(k,"modu_status/peers") == 0); assert(n <= sizeof(saved_settings));
    memcpy(saved_settings,p,n); saved_len=n; save_calls++; return 0;
}
#define SETTINGS_STATIC_HANDLER_DEFINE(...)
typedef struct { uint8_t val[6]; } bt_addr_t;
typedef struct { uint8_t type; bt_addr_t a; } bt_addr_le_t;
#define BT_CONN_TYPE_LE 1
#define BT_CONN_ROLE_CENTRAL 0
#define BT_CONN_ROLE_PERIPHERAL 1
#define BT_CONN_STATE_CONNECTED 2
struct bt_conn_info { int type; uint8_t role; int state; };
struct bt_conn { struct bt_conn_info info; bt_addr_le_t addr; int source; };
static struct bt_conn mock_conns[3];
static inline int bt_conn_get_info(const struct bt_conn *c, struct bt_conn_info *i) { *i=c->info; return 0; }
static inline const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *c) { return &c->addr; }
static inline void bt_conn_foreach(int type, void (*f)(struct bt_conn *, void *), void *p) {
    (void)type; for (size_t i=0;i<ARRAY_SIZE(mock_conns);i++) f(&mock_conns[i],p);
}
struct bt_conn_cb { void (*disconnected)(struct bt_conn *, uint8_t); };
#define BT_CONN_CB_DEFINE(n) struct bt_conn_cb n
struct zmk_peripheral_battery_state_changed { uint8_t source; uint8_t state_of_charge; };
struct zmk_battery_state_changed { uint8_t state_of_charge; };
struct zmk_position_state_changed { uint8_t source; uint32_t position; bool state; int64_t timestamp; };
typedef struct {
    int kind;
    struct zmk_peripheral_battery_state_changed battery;
    struct zmk_battery_state_changed local;
    struct zmk_position_state_changed position;
} zmk_event_t;
static inline const struct zmk_peripheral_battery_state_changed *as_zmk_peripheral_battery_state_changed(const zmk_event_t *e) { return e->kind==1 ? &e->battery : NULL; }
static inline const struct zmk_position_state_changed *as_zmk_position_state_changed(const zmk_event_t *e) { return e->kind==2 ? &e->position : NULL; }
static inline const struct zmk_battery_state_changed *as_zmk_battery_state_changed(const zmk_event_t *e) { return e->kind==3 ? &e->local : NULL; }
#define ZMK_EV_EVENT_BUBBLE 0
#define ZMK_LISTENER(...)
#define ZMK_SUBSCRIPTION(...)
enum zmk_transport { ZMK_TRANSPORT_NONE=0, ZMK_TRANSPORT_USB=1, ZMK_TRANSPORT_BLE=2 };
struct zmk_endpoint_instance { enum zmk_transport transport; struct { int profile_index; } ble; };
static struct zmk_endpoint_instance mock_endpoint = {.transport=ZMK_TRANSPORT_USB};
static bool mock_usb = true;
static inline struct zmk_endpoint_instance zmk_endpoint_get_selected(void) { return mock_endpoint; }
static inline bool zmk_endpoint_is_connected(void) { return mock_endpoint.transport != ZMK_TRANSPORT_NONE; }
static inline bool zmk_usb_is_powered(void) { return mock_usb; }
typedef uint8_t zmk_keymap_layer_index_t;
typedef uint8_t zmk_keymap_layer_id_t;
static uint8_t mock_layer;
static inline uint8_t zmk_keymap_highest_layer_active(void) { return mock_layer; }
static inline uint8_t zmk_keymap_layer_index_to_id(uint8_t i) { return i; }
static inline const char *zmk_keymap_layer_name(uint8_t i) { return i==3 ? "Game" : "Base"; }
typedef struct { char text[64]; int x,y,w,h; } lv_obj_t;
typedef struct { int unused; } lv_timer_t;
static lv_obj_t mock_objects[96];
static unsigned object_count;
static int lv_font_unscii_8;
#define LV_LABEL_LONG_CLIP 0
#define LV_OPA_COVER 255
#define LV_OBJ_FLAG_SCROLLABLE 1
static inline lv_obj_t *lv_obj_create(lv_obj_t *p) { (void)p; assert(object_count<ARRAY_SIZE(mock_objects)); return &mock_objects[object_count++]; }
static inline lv_obj_t *lv_label_create(lv_obj_t *p) { return lv_obj_create(p); }
static inline void lv_obj_remove_style_all(lv_obj_t *p) { (void)p; }
static inline void lv_obj_set_style_text_font(lv_obj_t *p, const int *f, int s) { (void)p;(void)f;(void)s; }
static inline int lv_color_white(void) { return 1; }
static inline int lv_color_black(void) { return 0; }
static inline void lv_obj_set_style_text_color(lv_obj_t *p,int c,int s) { (void)p;(void)c;(void)s; }
static inline void lv_label_set_long_mode(lv_obj_t *p,int m) { (void)p;(void)m; }
static inline void lv_obj_set_pos(lv_obj_t *p,int x,int y) { p->x=x;p->y=y; }
static inline void lv_obj_set_size(lv_obj_t *p,int w,int h) { p->w=w;p->h=h; }
static inline void lv_obj_set_style_bg_color(lv_obj_t *p,int c,int s) { (void)p;(void)c;(void)s; }
static inline void lv_obj_set_style_bg_opa(lv_obj_t *p,int o,int s) { (void)p;(void)o;(void)s; }
static inline void lv_obj_remove_flag(lv_obj_t *p,int f) { (void)p;(void)f; }
static inline void lv_label_set_text(lv_obj_t *p,const char *t) { snprintf(p->text,sizeof(p->text),"%s",t); }
static inline lv_timer_t *lv_timer_create(void (*f)(lv_timer_t *),uint32_t ms,void *data) { (void)f;(void)ms;(void)data; static lv_timer_t timer; return &timer; }

static inline void lv_obj_set_style_text_letter_space(lv_obj_t *p,int c,int s) { (void)p;(void)c;(void)s; }

#ifndef CONFIG_MODU_BATTERY_TELEMETRY_CENTRAL
#define CONFIG_MODU_BATTERY_TELEMETRY_CENTRAL 0
#endif
