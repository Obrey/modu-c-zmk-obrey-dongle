/* SPDX-License-Identifier: MIT
 * Pure C state/format helpers shared by firmware and native regression tests.
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MODU_SIDES 2
#define MODU_PEER_BYTES 7
#define MODU_SIDE_UNKNOWN (-1)
struct modu_identity {
    uint8_t known;
    uint8_t peer[MODU_PEER_BYTES];
};
struct modu_identities {
    uint8_t version;
    struct modu_identity side[MODU_SIDES];
};
static inline int modu_side_for_position(uint32_t position) {
    /* Six missing matrix locations are not input keys. */
    if (position >= 51 && position <= 56) return MODU_SIDE_UNKNOWN;
    if (position < 60) return (position % 12) < 6 ? 0 : 1;
    if (position < 63) return 0;
    if (position < 67) return 1;
    return MODU_SIDE_UNKNOWN;
}
static inline int modu_side_for_peer(const struct modu_identities *ids,
                                     const uint8_t peer[MODU_PEER_BYTES]) {
    for (int side = 0; side < MODU_SIDES; side++) {
        if (ids->side[side].known &&
            memcmp(ids->side[side].peer, peer, MODU_PEER_BYTES) == 0) return side;
    }
    return MODU_SIDE_UNKNOWN;
}
static inline bool modu_learn_peer(struct modu_identities *ids, int side,
                                    const uint8_t peer[MODU_PEER_BYTES]) {
    if (side < 0 || side >= MODU_SIDES) return false;
    if (ids->side[side].known &&
        memcmp(ids->side[side].peer, peer, MODU_PEER_BYTES) == 0) return false;
    /* A peer can never represent both hands. */
    for (int other = 0; other < MODU_SIDES; other++) {
        if (other != side && ids->side[other].known &&
            memcmp(ids->side[other].peer, peer, MODU_PEER_BYTES) == 0)
            memset(&ids->side[other], 0, sizeof(ids->side[other]));
    }
    ids->version = 1;
    ids->side[side].known = 1;
    memcpy(ids->side[side].peer, peer, MODU_PEER_BYTES);
    return true;
}
static inline bool modu_valid_identities(const struct modu_identities *ids) {
    if (ids->version != 1) return false;
    for (int side = 0; side < MODU_SIDES; side++)
        if (ids->side[side].known > 1) return false;
    return !(ids->side[0].known && ids->side[1].known &&
             memcmp(ids->side[0].peer, ids->side[1].peer, MODU_PEER_BYTES) == 0);
}
static inline void modu_format_half(char *out, size_t size, int side, bool known,
                                     bool connected, bool valid, uint8_t pct) {
    char hand = side == 0 ? 'L' : 'R';
    if (!known) snprintf(out, size, "%c: press a key", hand);
    else if (!connected) snprintf(out, size, "%c:  --%%  OFF", hand);
    else if (!valid || pct > 100) snprintf(out, size, "%c:  --%%  ON", hand);
    else snprintf(out, size, "%c: %3u%%  ON", hand, (unsigned)pct);
}
static inline unsigned modu_led_channel(bool right, bool connected) {
    /* Vendor wiring: left = B,G,R; right = G,B,R. */
    return connected ? (right ? 0u : 1u) : (right ? 1u : 0u);
}
static inline bool modu_led_visible(bool connected, bool bonded, uint32_t now_ms) {
    if (connected) return true;
    uint32_t period = bonded ? 1000u : 400u;
    return now_ms % period < period / 2u;
}
