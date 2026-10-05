/* SPDX-License-Identifier: MIT
 * MODU-C peripheral power-state helper.
 * Reads nRF52840 VBUS presence directly, without enabling USB HID on a split peripheral.
 * This is read-only: it does not alter the charger, regulator, USB pins, or battery path.
 */
#pragma once
#include <stdbool.h>
#if defined(CONFIG_SOC_NRF52840) || defined(CONFIG_SOC_NRF52840_QIAA)
#include <nrf.h>
#endif

static inline bool modu_peripheral_vbus_present(void) {
#if defined(MODU_POWER_STATE_TEST)
    extern bool mock_usb_powered;
    return mock_usb_powered;
#elif defined(NRF_POWER) && defined(POWER_USBREGSTATUS_VBUSDETECT_Msk)
    return (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0;
#else
    return false;
#endif
}
