/* SPDX-License-Identifier: MIT -- host test doubles, NOT the Zephyr runtime. */
#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#define CONFIG_LOG_DEFAULT_LEVEL 1
#define CONFIG_APPLICATION_INIT_PRIORITY 90
#define CONFIG_SHIELD_MODU_LEFT 1
#define CONFIG_SHIELD_MODU_RIGHT 0
#define CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS 2
#define IS_ENABLED(x) (x)
#define BUILD_ASSERT(c,m) _Static_assert(c,m)
#define MIN(a,b) ((a)<(b)?(a):(b))
#define CLAMP(v,l,h) ((v)<(l)?(l):((v)>(h)?(h):(v)))
#define CONTAINER_OF(ptr,type,member) ((type *)((char *)(ptr)-offsetof(type,member)))
#define LOG_MODULE_REGISTER(...)
#define LOG_DBG(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define LOG_INF(...) ((void)0)
#define LOG_ERR(...) ((void)0)
#define SYS_INIT(...)
#define K_SECONDS(x) ((x)*1000)
#define K_LOWEST_APPLICATION_THREAD_PRIO 14
struct k_spinlock {int dummy;};
typedef int k_spinlock_key_t;
static int k_spin_lock(struct k_spinlock *p) {(void)p; return 0;}
static void k_spin_unlock(struct k_spinlock *p,int k) {(void)p; (void)k;}
static int64_t mock_clock;
static int64_t k_uptime_get(void) {return mock_clock;}
struct k_work {void (*callback)(struct k_work *);};
struct k_work_delayable {struct k_work work;};
struct k_work_q {int dummy;};
struct k_timer {int dummy;};
#define K_WORK_DEFINE(n,fn) struct k_work n={.callback=(fn)}
#define K_WORK_DELAYABLE_DEFINE(n,fn) struct k_work_delayable n={.work={.callback=(fn)}}
#define K_TIMER_DEFINE(n,fn,stop) struct k_timer n
#define K_THREAD_STACK_DEFINE(n,size) unsigned char n[size]
#define K_THREAD_STACK_SIZEOF(n) sizeof(n)
static int queued_samples;
static struct k_work_q lowprio;
static struct k_work_q *zmk_workqueue_lowprio_work_q(void) {return &lowprio;}
static int k_work_submit_to_queue(struct k_work_q *q,struct k_work *w) {
    assert(q==&lowprio); (void)w; queued_samples++; return 0;
}
static void k_timer_start(struct k_timer *t,int delay,int period) {(void)t;(void)delay;(void)period;}
static void k_work_queue_start(struct k_work_q *q,void *s,size_t n,int p,void *c) {
    (void)q;(void)s;(void)n;(void)p;(void)c;
}
static int k_work_reschedule_for_queue(struct k_work_q *q,struct k_work_delayable *w,int t) {
    (void)q;(void)w;(void)t; return 0;
}
typedef int atomic_t;
static int atomic_get(atomic_t *p) {return *p;}
static void atomic_set(atomic_t *p,int v) {*p=v;}
struct device {int ready;};
static struct device mock_device={.ready=1};
#define DT_HAS_CHOSEN(x) 1
#define DT_CHOSEN(x) 0
#define DEVICE_DT_GET(x) (&mock_device)
static bool device_is_ready(const struct device *d) {return d->ready;}
enum sensor_channel {SENSOR_CHAN_GAUGE_STATE_OF_CHARGE,SENSOR_CHAN_GAUGE_VOLTAGE};
struct sensor_value {int32_t val1,val2;};
static int mock_fetch_error,mock_voltage_error,mock_percent_error;
static int mock_mv=3900,mock_percent=61,fetch_calls;
static int sensor_sample_fetch_chan(const struct device *d,enum sensor_channel c) {
    assert(d==&mock_device);assert(c==SENSOR_CHAN_GAUGE_STATE_OF_CHARGE);fetch_calls++;
    return mock_fetch_error;
}
static int sensor_channel_get(const struct device *d,enum sensor_channel c,struct sensor_value *v) {
    assert(d==&mock_device);
    if(c==SENSOR_CHAN_GAUGE_VOLTAGE) {*v=(struct sensor_value){mock_mv/1000,(mock_mv%1000)*1000};return mock_voltage_error;}
    *v=(struct sensor_value){mock_percent,0};return mock_percent_error;
}
struct zmk_split_peripheral_status_changed {bool connected;};
typedef struct {struct zmk_split_peripheral_status_changed link;} zmk_event_t;
static const struct zmk_split_peripheral_status_changed *as_zmk_split_peripheral_status_changed(const zmk_event_t *e) {return &e->link;}
static bool mock_link=true;
static bool zmk_split_bt_peripheral_is_connected(void) {return mock_link;}
#define ZMK_EV_EVENT_BUBBLE 0
#define ZMK_LISTENER(...)
#define ZMK_SUBSCRIPTION(...)

typedef struct {uint8_t val[6];} bt_addr_t;
typedef struct {uint8_t type;bt_addr_t a;} bt_addr_le_t;
#define BT_CONN_TYPE_LE 1
#define BT_CONN_STATE_CONNECTED 2
#define BT_CONN_ROLE_CENTRAL 0
#define BT_CONN_ROLE_PERIPHERAL 1
#define BT_SECURITY_L2 2
struct bt_conn_info {int type,state,role;};
struct bt_conn {struct bt_conn_info info;bt_addr_le_t addr;int source,refs,security;};
static struct bt_conn mock_connections[3];
static int bt_conn_get_info(struct bt_conn *c,struct bt_conn_info *i) {*i=c->info;return 0;}
static const bt_addr_le_t *bt_conn_get_dst(struct bt_conn *c) {return &c->addr;}
static int bt_conn_get_security(struct bt_conn *c) {return c->security;}
static struct bt_conn *bt_conn_ref(struct bt_conn *c) {c->refs++;return c;}
static void bt_conn_unref(struct bt_conn *c) {assert(c && c->refs>0);c->refs--;}
static void bt_conn_foreach(int type,void (*cb)(struct bt_conn *,void *),void *p) {
    (void)type;for(int i=0;i<3;i++)cb(&mock_connections[i],p);
}
struct bt_conn_cb {void (*disconnected)(struct bt_conn *,uint8_t);};
#define BT_CONN_CB_DEFINE(n) struct bt_conn_cb n
struct bt_uuid {int dummy;};
struct bt_uuid_128 {struct bt_uuid uuid;};
#define BT_UUID_128_ENCODE(...) 0
#define BT_UUID_INIT_128(...) {.uuid={0}}
static struct bt_uuid mock_uuid;
#define BT_UUID_DECLARE_128(...) (&mock_uuid)
struct bt_gatt_attr {int dummy;};
#define BT_GATT_PRIMARY_SERVICE(x) 0
#define BT_GATT_CHARACTERISTIC(...) 0
#define BT_GATT_SERVICE_DEFINE(n,...) static int n[]={__VA_ARGS__}
#define BT_GATT_CHRC_READ 2
#define BT_GATT_PERM_READ_ENCRYPT 4
static ssize_t bt_gatt_attr_read(struct bt_conn *c,const struct bt_gatt_attr *a,void *b,
                                 uint16_t len,uint16_t off,const void *v,uint16_t size) {
    (void)c;(void)a;if(off>size)return -7;
    size_t n=MIN(len,size-off);memcpy(b,(const uint8_t *)v+off,n);return (ssize_t)n;
}
#define BT_GATT_ITER_STOP 0
struct bt_gatt_read_params {
    uint8_t (*func)(struct bt_conn *,uint8_t,struct bt_gatt_read_params *,const void *,uint16_t);
    size_t handle_count;
    union {struct {uint16_t handle,offset;} single;
           struct {uint16_t start_handle,end_handle;const struct bt_uuid *uuid;} by_uuid;};
};
static int mock_read_rc,read_calls;
static int bt_gatt_read(struct bt_conn *c,struct bt_gatt_read_params *p) {
    assert(c->info.role==BT_CONN_ROLE_CENTRAL);
    assert(p->handle_count==0 && p->by_uuid.start_handle==1 && p->by_uuid.end_handle==0xffff);
    read_calls++;return mock_read_rc;
}
