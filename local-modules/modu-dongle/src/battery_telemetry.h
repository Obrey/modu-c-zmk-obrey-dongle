/* SPDX-License-Identifier: MIT */
#pragma once
#include "battery_telemetry_protocol.h"
/* Central only. Returns false until a verified packet from this BLE peer has
 * arrived. Increments age by the time spent in the central cache. */
bool modu_battery_detail_for_peer(const uint8_t peer[MODU_BATTERY_PEER_BYTES],
                                 struct modu_battery_detail *out);
