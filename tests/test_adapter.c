/* SPDX-License-Identifier: MIT
 * Host-only adapter tests with mocks. NOT firmware build / transport testing.
 * Compiles the real adapter source below, not a rewritten model of it.
 */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_ZMK_SPLIT 1
#define CONFIG_ZMK_SPLIT_ROLE_CENTRAL 1
#ifndef CONFIG_ZMK_BEHAVIOR_METADATA
#define CONFIG_ZMK_BEHAVIOR_METADATA 0
#endif
#define CONFIG_ZMK_LOG_LEVEL 3
#define CONFIG_KERNEL_INIT_PRIORITY_DEFAULT 50
#define POST_KERNEL 0
#define IS_ENABLED(x) (x)
#define DT_HAS_COMPAT_STATUS_OKAY(x) 1
#define LOG_MODULE_DECLARE(...)
#define LOG_INF(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define LOG_ERR(...) ((void)0)
#define ARG_UNUSED(x) ((void)(x))
#define BUILD_ASSERT(cond,msg) _Static_assert(cond,msg)
#define ZMK_BEHAVIOR_OPAQUE 0
#define ZMK_EV_EVENT_BUBBLE 0
#define BEHAVIOR_LOCALITY_CENTRAL 0

struct zmk_behavior_binding { const char *behavior_dev; uint32_t param1, param2; };
struct zmk_behavior_binding_event { int layer; uint32_t position; int64_t timestamp; uint8_t source; };
struct behavior_parameter_metadata { int unused; };
struct device { const char *name; const void *config; void *data; };
struct behavior_driver_api {
    int (*binding_pressed)(struct zmk_behavior_binding *,struct zmk_behavior_binding_event);
    int (*binding_released)(struct zmk_behavior_binding *,struct zmk_behavior_binding_event);
    int locality;
    int (*get_parameter_metadata)(const struct device *,struct behavior_parameter_metadata *);
};
struct zmk_position_state_changed { uint8_t source; uint32_t position; bool state; int64_t timestamp; };
typedef struct { bool position_event; struct zmk_position_state_changed data; } zmk_event_t;
static const struct zmk_position_state_changed *as_zmk_position_state_changed(const zmk_event_t *e) {
    return e->position_event ? &e->data : NULL;
}
static int64_t now_ms;
static unsigned active_layer;
static int64_t k_uptime_get(void) { return now_ms; }
static unsigned zmk_keymap_highest_layer_active(void) { return active_layer; }
static const struct device *zmk_behavior_get_binding(const char *name);
static int zmk_behavior_invoke_binding(const struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event,bool pressed);
#if CONFIG_ZMK_BEHAVIOR_METADATA
static int zmk_behavior_get_empty_param_metadata(const struct device *dev,
                                                struct behavior_parameter_metadata *meta) {
    (void)dev; (void)meta; return 0;
}
#endif

#define DT_INST_PROP_(n,p) PROP_##n##_##p
#define DT_INST_PROP(n,p) DT_INST_PROP_(n,p)
#define PROP_0_key_positions {1,3,5}
#define PROP_1_key_positions {6,8,10}
#define PROP_0_window_ms 80
#define PROP_1_window_ms 80
#define DT_INST_PROP_LEN(n,p) LEN_##p
#define LEN_key_positions 3
#define LEN_bindings 1
#define DT_INST_PROP_BY_IDX_(n,p,i) IDX_##n##_##i
#define DT_INST_PROP_BY_IDX(n,p,i) DT_INST_PROP_BY_IDX_(n,p,i)
#define IDX_0_0 1
#define IDX_0_1 3
#define IDX_0_2 5
#define IDX_1_0 6
#define IDX_1_1 8
#define IDX_1_2 10
#define DT_INST_PHANDLE_BY_IDX(n,p,i) 0
#define DEVICE_DT_NAME(n) "bootloader"
#define DT_INST_FOREACH_STATUS_OKAY(macro) macro(0) macro(1)
#define NAME_0 "boot135"
#define NAME_1 "boot680"
#define BEHAVIOR_DT_INST_DEFINE(n,init,pm,data,config,level,priority,api) \
    static const struct device device_##n = {NAME_##n,config,data};
#define ZMK_LISTENER(name,callback)
#define ZMK_SUBSCRIPTION(name,event)

#include "../local-modules/obrey-combo-boot/src/behavior_combo_boot.c"

static const struct device boot_device = {"bootloader",NULL,NULL};
static bool boot_missing;
static int dispatch_result;
static int calls;
static struct zmk_behavior_binding_event forwarded;
static const struct device *zmk_behavior_get_binding(const char *name) {
    if (!strcmp(name,"boot135")) return &device_0;
    if (!strcmp(name,"boot680")) return &device_1;
    if (!strcmp(name,"bootloader") && !boot_missing) return &boot_device;
    return NULL;
}
static int zmk_behavior_invoke_binding(const struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event,bool pressed) {
    assert(pressed);
    assert(!strcmp(binding->behavior_dev,"bootloader"));
    ++calls; forwarded=event; return dispatch_result;
}
static void reset_test(void) {
    memset(&obrey_boot_data_0,0,sizeof(obrey_boot_data_0));
    memset(&obrey_boot_data_1,0,sizeof(obrey_boot_data_1));
    now_ms=1050; active_layer=0; calls=0; dispatch_result=0; boot_missing=false;
}
static void physical(uint32_t key,uint8_t src,bool down,int64_t t) {
    zmk_event_t event={.position_event=true,.data={src,key,down,t}};
    assert(obrey_boot_listener(&event)==ZMK_EV_EVENT_BUBBLE);
}
static void chord(unsigned side,uint8_t src) {
    const uint32_t *p=side ? obrey_boot_config_1.positions : obrey_boot_config_0.positions;
    for (unsigned i=0;i<3;++i) physical(p[i],src,true,1000+20*i);
}
static int invoke(unsigned side) {
    struct zmk_behavior_binding b={side ? "boot680":"boot135",0,0};
    /* Native combos falsely label even the peripheral chord as LOCAL (255). */
    struct zmk_behavior_binding_event e={0,65000,1000,255};
    return obrey_boot_api.binding_pressed(&b,e);
}
int main(void) {
    unsigned tests=0;
    reset_test(); chord(0,255); assert(invoke(0)==0); assert(calls==1 && forwarded.source==255 && forwarded.position==1); ++tests;
    reset_test(); chord(1,0); assert(invoke(1)==0); assert(calls==1 && forwarded.source==0 && forwarded.position==6); ++tests;
    reset_test(); chord(0,2); assert(invoke(0)==0 && forwarded.source==2); ++tests;
    reset_test(); chord(1,255); assert(invoke(1)==0 && forwarded.source==255); ++tests;
    reset_test(); chord(1,0); physical(8,1,true,1021); assert(invoke(1)==-EPERM && calls==0); ++tests;
    reset_test(); physical(1,255,true,1000); physical(3,255,true,1020); assert(invoke(0)==-EPERM && calls==0); ++tests;
    reset_test(); chord(0,255); physical(3,255,false,1045); assert(invoke(0)==-EPERM && calls==0); ++tests;
    reset_test(); chord(0,255); active_layer=3; assert(invoke(0)==-EPERM && calls==0); ++tests;
    reset_test(); chord(1,0); boot_missing=true; assert(invoke(1)==-ENODEV && calls==0); ++tests;
    reset_test(); chord(1,0); dispatch_result=-ENOTCONN; assert(invoke(1)==-ENOTCONN && calls==1); assert(invoke(1)==-EPERM && calls==1); ++tests;
    reset_test(); physical(1,255,true,900); physical(3,255,true,1000); physical(5,255,true,1040); assert(invoke(0)==-EPERM && calls==0); ++tests;
    reset_test(); chord(0,255); now_ms=2050; assert(invoke(0)==-EPERM && calls==0); ++tests;
    reset_test(); chord(0,255); now_ms=1030; assert(invoke(0)==-EPERM && calls==0); ++tests;
    reset_test(); chord(0,255); assert(invoke(0)==0); struct zmk_behavior_binding b={"boot135",0,0}; struct zmk_behavior_binding_event e={0}; assert(obrey_boot_api.binding_released(&b,e)==0 && calls==1); ++tests;
    reset_test(); chord(1,0); assert(invoke(1)==0); chord(1,0); assert(invoke(1)==-EPERM && calls==1); ++tests;
    reset_test(); chord(0,255); physical(1,255,false,1045); physical(1,255,true,1000); assert(invoke(0)==-EPERM && calls==0); ++tests;
    reset_test(); chord(0,255); assert(invoke(0)==0); for(unsigned i=0;i<3;++i) physical(obrey_boot_config_0.positions[i],255,false,1100+i); for(unsigned i=0;i<3;++i) physical(obrey_boot_config_0.positions[i],255,true,1500+20*i); now_ms=1550; assert(invoke(0)==0 && calls==2); ++tests;
    reset_test(); for(unsigned i=0;i<3;++i) { physical(obrey_boot_config_0.positions[i],255,true,1000+20*i); physical(obrey_boot_config_0.positions[i],255,false,1010+20*i); } assert(invoke(0)==-EPERM && calls==0); ++tests;
    reset_test(); chord(0,255); physical(20,0,true,1040); assert(invoke(0)==0 && forwarded.source==255); ++tests;
    reset_test(); zmk_event_t unrelated={0}; assert(obrey_boot_listener(&unrelated)==ZMK_EV_EVENT_BUBBLE && calls==0); ++tests;
    /* Remote firmware restarted before it could send any release events. */
    reset_test(); chord(1,0); assert(invoke(1)==0);
    for(unsigned i=0;i<3;++i) physical(obrey_boot_config_1.positions[i],0,true,3000+20*i);
    now_ms=3050; assert(invoke(1)==0 && calls==2 && forwarded.source==0); ++tests;
    /* A delayed identical replay cannot cause a second reset. */
    reset_test(); chord(1,0); assert(invoke(1)==0); now_ms=3050; chord(1,0);
    assert(invoke(1)==-EPERM && calls==1); ++tests;
    /* Reconnect: observing only one of the three keys is insufficient. */
    reset_test(); chord(1,0); assert(invoke(1)==0); physical(6,0,true,3000); now_ms=3050;
    assert(invoke(1)==-EPERM && calls==1); ++tests;
    /* Fn and other non-Base layers may not execute a reset. */
    reset_test(); chord(1,0); active_layer=4; assert(invoke(1)==-EPERM && calls==0); ++tests;
    /* Invalid behavior parameters must never select a destination. */
    reset_test(); chord(0,255); struct zmk_behavior_binding bad={"boot135",1,0};
    assert(obrey_boot_api.binding_pressed(&bad,(struct zmk_behavior_binding_event){0})==-EINVAL && calls==0); ++tests;
    printf("PASS: %u host adapter scenarios; metadata=%d. ZMK/Zephyr and transport are mocked.\n",tests,CONFIG_ZMK_BEHAVIOR_METADATA);
    return 0;
}
