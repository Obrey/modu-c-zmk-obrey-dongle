/* SPDX-License-Identifier: MIT
 * MODU battery diagnostic payload v2 (decodes v1 too). Wire encoding is explicit, never a C struct.
 * No voltage-divider ratio or percentage calibration is changed here.
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MODU_BATTERY_PACKET_SIZE 14
#define MODU_BATTERY_V1_PACKET_SIZE 12
#define MODU_BATTERY_PACKET_VERSION 2
#define MODU_BATTERY_FLAG_IDLE 0x01
#define MODU_BATTERY_PEER_BYTES 7
#define MODU_BATTERY_STALE_SECONDS 180
#define MODU_BATTERY_IDLE_STALE_SECONDS 900
#define MODU_BATTERY_SERVICE_UUID \
    BT_UUID_128_ENCODE(0x9f1c6d70, 0x3f92, 0x4e42, 0xa536, 0x86d3d8e53001)
#define MODU_BATTERY_VALUE_UUID \
    BT_UUID_128_ENCODE(0x9f1c6d70, 0x3f92, 0x4e42, 0xa536, 0x86d3d8e53002)

enum modu_battery_result {
    MODU_BAT_WAIT = 0,
    MODU_BAT_OK = 1,
    MODU_BAT_NOT_READY = 2,
    MODU_BAT_FETCH_ERROR = 3,
    MODU_BAT_VOLTAGE_ERROR = 4,
    MODU_BAT_PERCENT_ERROR = 5,
    MODU_BAT_VALUE_ERROR = 6,
};
struct modu_battery_detail {
    uint8_t side;          /* 0 left, 1 right, from the actual shield build. */
    uint8_t result;
    uint8_t percent;       /* 255 means unavailable; never convert it to 0. */
    uint16_t millivolts;   /* As reported by the ORIGINAL sensor driver. */
    uint16_t age_seconds; /* Age of last sampling attempt, not last % change. */
    int16_t error;         /* Negative driver errno, for diagnosis. */
    uint16_t sequence;
    uint8_t flags;        /* IDLE: connected standby, NOT a sleeping/disconnected link. */
};
static inline uint16_t modu_bat_get16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline void modu_bat_put16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
}
static inline void modu_battery_encode(const struct modu_battery_detail *d,
                                      uint8_t out[MODU_BATTERY_PACKET_SIZE]) {
    out[0] = MODU_BATTERY_PACKET_VERSION; out[1] = d->side;
    out[2] = d->result; out[3] = d->percent;
    modu_bat_put16(out + 4, d->millivolts);
    modu_bat_put16(out + 6, d->age_seconds);
    modu_bat_put16(out + 8, (uint16_t)d->error);
    modu_bat_put16(out + 10, d->sequence);
    out[12] = d->flags & MODU_BATTERY_FLAG_IDLE; out[13] = 0;
}
static inline bool modu_battery_decode(const void *data, size_t length,
                                      struct modu_battery_detail *out) {
    if (!data || !out || (length != MODU_BATTERY_PACKET_SIZE &&
                         length != MODU_BATTERY_V1_PACKET_SIZE)) return false;
    const uint8_t *p = data;
    bool v1 = p[0] == 1 && length == MODU_BATTERY_V1_PACKET_SIZE;
    bool v2 = p[0] == MODU_BATTERY_PACKET_VERSION && length == MODU_BATTERY_PACKET_SIZE;
    if ((!v1 && !v2) || p[1] > 1 || p[2] > MODU_BAT_VALUE_ERROR) return false;
    if (v2 && ((p[12] & ~MODU_BATTERY_FLAG_IDLE) || p[13] != 0)) return false;
    if (p[2] == MODU_BAT_OK && p[3] > 100) return false;
    const uint16_t raw_error = modu_bat_get16(p + 8);
    *out = (struct modu_battery_detail){
        .side = p[1], .result = p[2], .percent = p[3],
        .millivolts = modu_bat_get16(p + 4), .age_seconds = modu_bat_get16(p + 6),
        .error = (int16_t)(raw_error <= 32767 ? (int32_t)raw_error : (int32_t)raw_error - 65536),
        .sequence = modu_bat_get16(p + 10),
        .flags = v2 ? p[12] : 0,
    };
    return true;
}
/* Seven 8px characters max: no changes to the original 56px battery layout.
 * All valid samples alternate percentage and measured voltage. No calibration
 * is invented: 0% at 3.32/3.33V remains 0% on ZMK's original <=3.45V curve.
 * ~ means a cached measurement from connected standby. It is not charging or
 * a more precise estimate. Error / absent / expired samples remain distinct.
 */
static inline void modu_battery_format(char *text, size_t capacity, char hand,
                                      const struct modu_battery_detail *d, bool voltage_phase) {
    bool idle = d && (d->flags & MODU_BATTERY_FLAG_IDLE);
    unsigned stale_after = idle ? MODU_BATTERY_IDLE_STALE_SECONDS : MODU_BATTERY_STALE_SECONDS;
    char separator = idle ? '~' : ' ';
    if (!d || d->result == MODU_BAT_WAIT) {
        snprintf(text, capacity, "%c  --%%", hand);
    } else if (d->age_seconds > stale_after) {
        snprintf(text, capacity, "%c OLD", hand);
    } else if (d->result != MODU_BAT_OK) {
        snprintf(text, capacity, "%c ERR", hand);
    } else if (d->millivolts < 2500 || d->millivolts > 4500) {
        if (voltage_phase && d->millivolts < 10000)
            snprintf(text, capacity, "%c %u.%02uV", hand,
                     (unsigned)d->millivolts / 1000, ((unsigned)d->millivolts % 1000) / 10);
        else snprintf(text, capacity, "%c ADC?", hand);
    } else if (voltage_phase) {
        snprintf(text, capacity, "%c%c%u.%02uV", hand, separator,
                 (unsigned)d->millivolts / 1000, ((unsigned)d->millivolts % 1000) / 10);
    } else {
        snprintf(text, capacity, "%c%c%3u%%", hand, separator, (unsigned)d->percent);
    }
}
