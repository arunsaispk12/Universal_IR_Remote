/**
 * @file rgb_led.h
 * @brief WS2812B RGB LED Status Indicator header
 * 
 * This component controls WS2812B LED for status indication
 */

#ifndef RGB_LED_H
#define RGB_LED_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* RGB LED Configuration */
#define RGB_LED_GPIO        11      // RGB LED GPIO
#define RGB_LED_COUNT       1       // Number of LEDs

/* LED Status Modes */
typedef enum {
    LED_MODE_OFF,               // LED off
    LED_MODE_WIFI_CONNECTING,   // Blue blinking
    LED_MODE_WIFI_CONNECTED,    // Green solid
    LED_MODE_WIFI_ERROR,        // Red blinking
    LED_MODE_PROVISIONING,      // Blue fast blink
    LED_MODE_OTA_PROGRESS,      // Purple pulsing
    LED_MODE_OTA_SUCCESS,       // Green flash
    LED_MODE_OTA_ERROR,         // Red solid
    LED_MODE_FACTORY_RESET,     // Red fast blink

    // IR Learning modes
    LED_MODE_IR_LEARNING,           // Purple blinking - waiting for IR
    LED_MODE_IR_LEARNING_SUCCESS,   // Green flash - code learned
    LED_MODE_IR_LEARNING_FAILED,    // Red flash - learning failed
    LED_MODE_IR_TRANSMITTING,       // Cyan pulse - sending IR code

    LED_MODE_CUSTOM            // Custom color
} rgb_led_mode_t;

/* RGB Color Structure */
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_color_t;

/* Predefined Colors */
#define RGB_COLOR_OFF       {0, 0, 0}
#define RGB_COLOR_RED       {255, 0, 0}
#define RGB_COLOR_GREEN     {0, 255, 0}
#define RGB_COLOR_BLUE      {0, 0, 255}
#define RGB_COLOR_YELLOW    {255, 255, 0}
#define RGB_COLOR_CYAN      {0, 255, 255}
#define RGB_COLOR_MAGENTA   {255, 0, 255}
#define RGB_COLOR_WHITE     {255, 255, 255}
#define RGB_COLOR_ORANGE    {255, 165, 0}
#define RGB_COLOR_PURPLE    {128, 0, 128}

/**
 * @brief Initialize RGB LED
 * 
 * @param gpio_num GPIO pin connected to RGB LED
 * @return ESP_OK on success
 */
esp_err_t rgb_led_init(uint8_t gpio_num);

/**
 * @brief Set LED mode
 * 
 * @param mode LED mode
 * @return ESP_OK on success
 */
esp_err_t rgb_led_set_mode(rgb_led_mode_t mode);

/**
 * @brief Set custom color using rgb_color_t struct
 * 
 * @param color RGB color
 * @return ESP_OK on success
 */
esp_err_t rgb_led_set_rgb_color(rgb_color_t color);

/**
 * @brief Set RGB values directly
 * 
 * @param red Red value (0-255)
 * @param green Green value (0-255)
 * @param blue Blue value (0-255)
 * @return ESP_OK on success
 */
esp_err_t rgb_led_set_color(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Turn off LED
 * 
 * @return ESP_OK on success
 */
esp_err_t rgb_led_off(void);

/**
 * @brief Set LED brightness
 * 
 * @param brightness Brightness level (0-100)
 * @return ESP_OK on success
 */
esp_err_t rgb_led_set_brightness(uint8_t brightness);

/**
 * @brief Blink LED with custom pattern
 * 
 * @param color LED color
 * @param on_time_ms On time in milliseconds
 * @param off_time_ms Off time in milliseconds
 * @param repeat Number of times to repeat (0 = infinite)
 * @return ESP_OK on success
 */
esp_err_t rgb_led_blink(rgb_color_t color, uint32_t on_time_ms, uint32_t off_time_ms, uint32_t repeat);

/**
 * @brief Pulse LED (fade in/out)
 * 
 * @param color LED color
 * @param period_ms Pulse period in milliseconds
 * @return ESP_OK on success
 */
esp_err_t rgb_led_pulse(rgb_color_t color, uint32_t period_ms);

/**
 * @brief Stop any ongoing LED effect
 * 
 * @return ESP_OK on success
 */
esp_err_t rgb_led_stop_effect(void);

#ifdef __cplusplus
}
#endif

#endif /* RGB_LED_H */
