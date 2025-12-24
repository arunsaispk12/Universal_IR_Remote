/*
 * RGB LED Status Component
 * WS2812B LED control with status indication
 */

#ifndef RGB_LED_H
#define RGB_LED_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED status enumeration
 */
typedef enum {
    LED_STATUS_IDLE,            // Dim blue - system idle
    LED_STATUS_LEARNING,        // Pulsing purple - learning IR signal
    LED_STATUS_LEARN_SUCCESS,   // Green flash (3 times) - learning succeeded
    LED_STATUS_LEARN_FAILED,    // Red flash (3 times) - learning failed
    LED_STATUS_TRANSMITTING,    // Cyan flash (once) - transmitting IR signal
    LED_STATUS_WIFI_CONNECTING, // Yellow pulsing - connecting to WiFi
    LED_STATUS_WIFI_CONNECTED,  // Green solid - WiFi connected
    LED_STATUS_ERROR,           // Red blinking - error state
    LED_STATUS_OFF              // LED off
} rgb_led_status_t;

/**
 * @brief RGB color structure
 */
typedef struct {
    uint8_t r;  // Red component (0-255)
    uint8_t g;  // Green component (0-255)
    uint8_t b;  // Blue component (0-255)
} rgb_led_color_t;

/**
 * @brief RGB LED configuration
 */
typedef struct {
    uint8_t gpio_num;      // GPIO pin for LED data
    uint16_t led_count;    // Number of LEDs in the strip
    uint8_t rmt_channel;   // RMT channel to use
} rgb_led_config_t;

/**
 * @brief Initialize RGB LED component
 *
 * @param config Pointer to LED configuration structure
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rgb_led_init(const rgb_led_config_t *config);

/**
 * @brief Set LED status (non-blocking)
 *
 * @param status Status to set
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rgb_led_set_status(rgb_led_status_t status);

/**
 * @brief Set custom LED color (non-blocking)
 *
 * @param color Pointer to color structure
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rgb_led_set_color(const rgb_led_color_t *color);

/**
 * @brief Turn off LED
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rgb_led_turn_off(void);

/**
 * @brief Deinitialize RGB LED component
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rgb_led_deinit(void);

/**
 * @brief Get current LED status
 *
 * @return rgb_led_status_t Current status
 */
rgb_led_status_t rgb_led_get_status(void);

#ifdef __cplusplus
}
#endif

#endif // RGB_LED_H
