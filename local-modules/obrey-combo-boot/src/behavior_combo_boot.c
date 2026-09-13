/* SPDX-License-Identifier: MIT
 * A combo normally loses the physical source and supplies LOCAL instead.
 * Observe, but never capture, physical events before ZMK's combo listener;
 * forward the existing bootloader behavior using the observed source.
 * No hard-coded left=central, peripheral index, custom transport, or reset register.
 */
#define DT_DRV_COMPAT zmk_behavior_obrey_combo_boot

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>

#include "obrey_guard.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#error "Obrey combo boot must be compiled only on a ZMK split central"
#endif

struct obrey_boot_config {
    uint32_t positions[OBREY_CHORD_KEYS];
    uint32_t window_ms;
    struct zmk_behavior_binding action;
};

struct obrey_boot_data {
    struct obrey_chord_guard guard;
};

static int obrey_boot_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    if (dev == NULL || binding->param1 != 0 || binding->param2 != 0) {
        return -EINVAL;
    }
    const struct obrey_boot_config *config = dev->config;
    struct obrey_boot_data *data = dev->data;
    uint8_t source;
    const int64_t now = k_uptime_get();

    /* Defense in depth: even an accidental binding outside Base cannot boot. */
    if (zmk_keymap_highest_layer_active() != 0 ||
        !obrey_guard_target(&data->guard, config->window_ms, 1000, now, &source)) {
        LOG_WRN("Obrey boot rejected: not a fresh same-source Base chord");
        return -EPERM;
    }
    if (zmk_behavior_get_binding(config->action.behavior_dev) == NULL) {
        LOG_ERR("Obrey boot rejected: bootloader behavior is not present");
        return -ENODEV;
    }

    /* Latch before dispatch; never retry or silently fall back to LOCAL. */
    data->guard.fired = true;
    data->guard.consumed_at = now;
    event.source = source;
    event.position = config->positions[0];
    event.layer = 0;
    event.timestamp = now;
    LOG_INF("Obrey combo boot: key %u, actual source %u", event.position, source);
    int err = zmk_behavior_invoke_binding(&config->action, event, true);
    if (err != 0) {
        LOG_ERR("Obrey boot dispatch returned %d; release all keys before retry", err);
    }
    return err;
}

static int obrey_boot_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    /* Bootloader reset acts on press. Do not send another command on release. */
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api obrey_boot_api = {
    .binding_pressed = obrey_boot_pressed,
    .binding_released = obrey_boot_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define OBREY_BOOT_INSTANCE(n)                                                                 \
    BUILD_ASSERT(DT_INST_PROP_LEN(n, key_positions) == OBREY_CHORD_KEYS, "Need three keys");      \
    BUILD_ASSERT(DT_INST_PROP_LEN(n, bindings) == 1, "Need one bootloader binding");              \
    BUILD_ASSERT(DT_INST_PROP_BY_IDX(n, key_positions, 0) !=                                     \
                     DT_INST_PROP_BY_IDX(n, key_positions, 1) &&                               \
                 DT_INST_PROP_BY_IDX(n, key_positions, 0) !=                                   \
                     DT_INST_PROP_BY_IDX(n, key_positions, 2) &&                               \
                 DT_INST_PROP_BY_IDX(n, key_positions, 1) !=                                   \
                     DT_INST_PROP_BY_IDX(n, key_positions, 2), "Keys must be distinct");         \
    static const struct obrey_boot_config obrey_boot_config_##n = {                             \
        .positions = DT_INST_PROP(n, key_positions),                                            \
        .window_ms = DT_INST_PROP(n, window_ms),                                                 \
        .action = {                                                                             \
            .behavior_dev = DEVICE_DT_NAME(DT_INST_PHANDLE_BY_IDX(n, bindings, 0)),               \
            .param1 = 0, .param2 = 0,                                                            \
        },                                                                                      \
    };                                                                                          \
    static struct obrey_boot_data obrey_boot_data_##n;                                           \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &obrey_boot_data_##n, &obrey_boot_config_##n,             \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &obrey_boot_api);

DT_INST_FOREACH_STATUS_OKAY(OBREY_BOOT_INSTANCE)

#define OBREY_OBSERVE(n)                                                                        \
    obrey_guard_observe(&obrey_boot_data_##n.guard, obrey_boot_config_##n.positions,                \
                        changed->position, changed->source, changed->state, changed->timestamp);

static int obrey_boot_listener(const zmk_event_t *event) {
    const struct zmk_position_state_changed *changed = as_zmk_position_state_changed(event);
    if (changed != NULL) {
        DT_INST_FOREACH_STATUS_OKAY(OBREY_OBSERVE)
    }
    return ZMK_EV_EVENT_BUBBLE;
}

/* target_sources(app ...) in this module is deliberate: external app sources
 * precede ZMK's core listeners. If source observation fails, the guard refuses
 * to boot rather than sending the command to the central by default.
 */
ZMK_LISTENER(obrey_combo_boot, obrey_boot_listener);
ZMK_SUBSCRIPTION(obrey_combo_boot, zmk_position_state_changed);
#endif
