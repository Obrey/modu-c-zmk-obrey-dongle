#pragma once

/**
 * @file pixart.h
 *
 * @brief Common header file for all optical motion sensor by PIXART
 */

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/* device data structure */
struct pixart_data {
    const struct device          *dev;
    bool                         sw_smart_flag; // for pmw3610 smart algorithm

    struct gpio_callback         irq_gpio_cb; // motion pin irq callback
    struct k_work                trigger_work; // realtrigger job
#if CONFIG_PMW3610_ALT_POLL_INTERVAL_MS > 0
    struct k_timer               poll_timer; // periodic motion poll (IRQ-less boards)
#endif

    struct k_work_delayable      init_work; // the work structure for delayable init steps
    int                          async_init_step;

    bool                         ready; // whether init is finished successfully
    int                          err; // error code during async init
    bool                         active_swap_xy;
    bool                         active_inv_x;
    bool                         active_inv_y;
    int16_t                      active_rotation_cos_milli;
    int16_t                      active_rotation_sin_milli;
};

// device config data structure
struct pixart_config {
	struct spi_dt_spec spi;
    struct gpio_dt_spec irq_gpio;
    struct gpio_dt_spec orientation_gpio;
    uint16_t cpi;
    bool swap_xy;
    bool inv_x;
    bool inv_y;
    int16_t rotation_cos_milli;
    int16_t rotation_sin_milli;
    int16_t alt_rotation_cos_milli;
    int16_t alt_rotation_sin_milli;
    bool alt_swap_xy;
    bool alt_inv_x;
    bool alt_inv_y;
    uint8_t evt_type;
    uint8_t x_input_code;
    uint8_t y_input_code;
    bool force_awake;
    bool force_awake_4ms_mode;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
