/* SPDX-License-Identifier: MIT
 * Native test double for the two-field upstream widget ABI; NOT a firmware header.
 */
#pragma once
#include "status_ui_mock.h"
typedef struct { void *next; } sys_snode_t;
struct zmk_widget_dongle_battery_status { sys_snode_t node; lv_obj_t *obj; };
int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *, lv_obj_t *);
lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *);
