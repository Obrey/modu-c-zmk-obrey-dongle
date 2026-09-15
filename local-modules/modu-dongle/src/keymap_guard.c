/* SPDX-License-Identifier: MIT
 * Refuse to silently build a stock/different-structure keymap on the dongle.
 * This guard is evaluated against the GENERATED devicetree, not the ZIP text.
 */
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modu_keymap_v3, CONFIG_LOG_DEFAULT_LEVEL);

#define MODU_BASE DT_PATH(keymap, default_layer)
#define MODU_BOOT_L DT_PATH(keymap, boot_left_layer)
#define MODU_BOOT_R DT_PATH(keymap, boot_right_layer)
BUILD_ASSERT(DT_NODE_EXISTS(MODU_BASE), "Custom Base layer is missing");
BUILD_ASSERT(DT_NODE_EXISTS(MODU_BOOT_L), "Custom left boot layer is missing");
BUILD_ASSERT(DT_NODE_EXISTS(MODU_BOOT_R), "Custom right boot layer is missing");
BUILD_ASSERT(DT_PROP_LEN(MODU_BASE, bindings) == 67, "MODU-C needs 67 logical positions");
BUILD_ASSERT(DT_SAME_NODE(DT_PHANDLE_BY_IDX(MODU_BASE, bindings, 5), DT_NODELABEL(blt5)),
             "The custom 5 long-hold binding was not compiled");
BUILD_ASSERT(DT_SAME_NODE(DT_PHANDLE_BY_IDX(MODU_BASE, bindings, 6), DT_NODELABEL(blt6)),
             "The custom 6 long-hold binding was not compiled");
static int modu_keymap_version(void) {
    LOG_INF("MODU-C MAC v3; keymap SHA256=" MODU_CONFIG_KEYMAP_SHA256);
    return 0;
}
SYS_INIT(modu_keymap_version, APPLICATION, 99);
