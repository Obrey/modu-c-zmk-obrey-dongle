/* SPDX-License-Identifier: MIT
 * MODU-C empirical discharge gauge.
 * The endpoints are based on observed wireless-use behavior, not cell-terminal
 * voltage. Raw telemetry, standard BAS reports, charging and protection behavior
 * are untouched. USB-powered halves show CHG instead of a misleading percentage.
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "battery_telemetry_protocol.h"

struct modu_battery_range {
    uint16_t min_mv; /* Empirical near-shutdown sensor reading. */
    uint16_t max_mv; /* Empirical unplugged/full-use sensor reading. */
};

static inline bool modu_battery_range_percent(uint16_t mv,
                                             const struct modu_battery_range *range,
                                             uint8_t *out) {
    if (!range || !out || range->max_mv <= range->min_mv) return false;
    if (mv <= range->min_mv) { *out = 0; return true; }
    if (mv >= range->max_mv) { *out = 100; return true; }
    const uint32_t span = (uint32_t)range->max_mv - range->min_mv;
    /* Nearest whole percent, with 32-bit arithmetic even on small targets. */
    *out = (uint8_t)((((uint32_t)mv - range->min_mv) * 100u + span / 2u) / span);
    return true;
}

static inline void modu_battery_format_range(char *text, size_t capacity, char hand,
                                             const struct modu_battery_detail *d,
                                             bool voltage_phase,
                                             const struct modu_battery_range *range) {
    /* Keep the original raw voltage, no-data, error and stale display paths.
     * Only a fresh, successful, plausible measurement may be normalized.
     * No BAS fallback: a BAS percentage alone has no voltage to normalize.
     */
    const bool idle = d && (d->flags & MODU_BATTERY_FLAG_IDLE);
    const unsigned stale_after = idle ? MODU_BATTERY_IDLE_STALE_SECONDS :
                                       MODU_BATTERY_STALE_SECONDS;
    uint8_t relative;
    if (!voltage_phase && d && d->result == MODU_BAT_OK &&
        d->age_seconds <= stale_after &&
        (d->flags & MODU_BATTERY_FLAG_USB_POWERED)) {
        snprintf(text, capacity, "%c CHG", hand);
        return;
    }
    if (!voltage_phase && d && d->result == MODU_BAT_OK &&
        d->age_seconds <= stale_after && d->millivolts < 10000 &&
        modu_battery_range_percent(d->millivolts, range, &relative)) {
        /* This is an empirical MODU-C gauge, not chemical state-of-charge.
         * '~' still marks an idle cached sample.
         */
        if (idle) snprintf(text, capacity, "%c~%3u%%", hand, (unsigned)relative);
        else snprintf(text, capacity, "%c %3u%%", hand, (unsigned)relative);
        return;
    }
    modu_battery_format(text, capacity, hand, d, voltage_phase);
}
