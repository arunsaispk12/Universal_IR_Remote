/*
 * WS2812B LED Strip RMT Encoder
 * Handles WS2812B timing protocol using ESP32 RMT peripheral
 */

#ifndef LED_STRIP_ENCODER_H
#define LED_STRIP_ENCODER_H

#include <stdint.h>
#include "driver/rmt_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WS2812B encoder configuration
 */
typedef struct {
    uint32_t resolution; // RMT tick resolution in Hz
} led_strip_encoder_config_t;

/**
 * @brief Create RMT encoder for WS2812B LED strip
 *
 * @param config Encoder configuration
 * @param ret_encoder Returned encoder handle
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rmt_new_led_strip_encoder(const led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

#ifdef __cplusplus
}
#endif

#endif // LED_STRIP_ENCODER_H
