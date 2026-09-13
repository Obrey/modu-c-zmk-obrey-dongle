/* SPDX-License-Identifier: MIT
 * Small, platform-independent guard. It does not recognize ZMK combos itself.
 * The native ZMK combo engine invokes the behavior after recognizing a chord.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define OBREY_CHORD_KEYS 3

struct obrey_key_sample {
    bool seen;
    bool down;
    uint8_t source;
    int64_t changed_at;
    int64_t pressed_at;
};

struct obrey_chord_guard {
    struct obrey_key_sample key[OBREY_CHORD_KEYS];
    bool fired;
    int64_t consumed_at;
};

/* Returns false for an unrelated or older replayed event. */
static inline bool obrey_guard_observe(struct obrey_chord_guard *guard,
                                      const uint32_t positions[OBREY_CHORD_KEYS],
                                      uint32_t position, uint8_t source,
                                      bool down, int64_t timestamp) {
    for (size_t i = 0; i < OBREY_CHORD_KEYS; ++i) {
        if (positions[i] != position) {
            continue;
        }
        /* A remote boot can prevent key-up packets from reaching us. A fresh
         * down event at least 1 s after dispatch starts a new observation set.
         * Old/identical event replays cannot meet this condition. Native ZMK
         * still has to recognize a NEW complete combo before we can execute.
         */
        if (guard->fired && down && timestamp >= guard->consumed_at &&
            timestamp - guard->consumed_at >= 1000) {
            *guard = (struct obrey_chord_guard){0};
        }
        struct obrey_key_sample *sample = &guard->key[i];
        if (sample->seen && timestamp < sample->changed_at) {
            return false;
        }
        /* Native combo/hold-tap code may replay an identical event. */
        if (sample->seen && timestamp == sample->changed_at &&
            sample->down == down && sample->source == source) {
            return false;
        }
        if (down && (!sample->seen || !sample->down || sample->source != source)) {
            sample->pressed_at = timestamp;
        }
        sample->seen = true;
        sample->down = down;
        sample->source = source;
        sample->changed_at = timestamp;
        if (!guard->key[0].down && !guard->key[1].down && !guard->key[2].down) {
            guard->fired = false;
        }
        return true;
    }
    return false;
}

/* No default target: missing, stale or mixed-source input is rejected. */
static inline bool obrey_guard_target(const struct obrey_chord_guard *guard,
                                     uint32_t window_ms, uint32_t max_age_ms,
                                     int64_t now, uint8_t *source_out) {
    if (guard->fired || source_out == NULL) {
        return false;
    }
    int64_t first = guard->key[0].pressed_at;
    int64_t last = first;
    for (size_t i = 0; i < OBREY_CHORD_KEYS; ++i) {
        const struct obrey_key_sample *sample = &guard->key[i];
        if (!sample->seen || !sample->down ||
            sample->source != guard->key[0].source) {
            return false;
        }
        if (sample->pressed_at < first) {
            first = sample->pressed_at;
        }
        if (sample->pressed_at > last) {
            last = sample->pressed_at;
        }
    }
    if (first < 0 || last - first > window_ms || now < last || now - last > max_age_ms) {
        return false;
    }
    *source_out = guard->key[0].source;
    return true;
}
