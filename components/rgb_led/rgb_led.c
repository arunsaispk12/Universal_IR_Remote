/**
 * @file rgb_led.c
 * @brief WS2812B RGB LED Status Indicator implementation
 */

#include "rgb_led.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "RGB_LED";

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1µs

static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;
static uint8_t led_strip_pixels[RGB_LED_COUNT * 3];
static rgb_led_mode_t current_mode = LED_MODE_OFF;
static TaskHandle_t effect_task_handle = NULL;
static bool effect_running = false;
static uint8_t brightness = 100; // Default brightness percentage

/* Effect control variables */
static rgb_color_t effect_color = {0, 0, 0};
static uint32_t effect_on_time = 0;
static rgb_color_t base_color = {0, 255, 0}; // WiFi connected color (green) to return to
static uint32_t effect_off_time = 0;
static uint32_t effect_repeat = 0;
static uint32_t effect_pulse_period = 0;

/**
 * @brief Apply brightness to color
 */
static inline uint8_t apply_brightness(uint8_t color_value)
{
    return (color_value * brightness) / 100;
}

/**
 * @brief Update the LED strip
 */
static esp_err_t update_led_strip(void)
{
    if (led_chan == NULL || led_encoder == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no loop
    };

    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
    
    return ESP_OK;
}

/**
 * @brief Blink effect task
 */
/**
 * @brief Blink effect task
 */
static void blink_task(void *pvParameters)
{
    uint32_t count = 0;
    
    while (effect_running) {
        // On phase - show effect color
        rgb_led_set_color(apply_brightness(effect_color.red),
                         apply_brightness(effect_color.green),
                         apply_brightness(effect_color.blue));
        
        if (effect_on_time > 0) {
            vTaskDelay(effect_on_time / portTICK_PERIOD_MS);
        }
        
        // Off phase - return to base color instead of turning off
        rgb_led_set_color(apply_brightness(base_color.red),
                         apply_brightness(base_color.green),
                         apply_brightness(base_color.blue));
        
        if (effect_off_time > 0) {
            vTaskDelay(effect_off_time / portTICK_PERIOD_MS);
        }
        
        // Check repeat count
        if (effect_repeat > 0) {
            count++;
            if (count >= effect_repeat) {
                break;
            }
        }
    }
    
    // Return to base color when effect completes
    rgb_led_set_color(apply_brightness(base_color.red),
                     apply_brightness(base_color.green),
                     apply_brightness(base_color.blue));
    
    effect_running = false;
    effect_task_handle = NULL;
    vTaskDelete(NULL);
}


/**
 * @brief Pulse effect task
 */
static void pulse_task(void *pvParameters)
{
    const uint32_t steps = 50;
    uint32_t step_delay = effect_pulse_period / (steps * 2);
    
    while (effect_running) {
        // Fade in
        for (uint32_t i = 0; i <= steps && effect_running; i++) {
            uint8_t brightness_level = (i * brightness) / steps;
            rgb_led_set_color((effect_color.red * brightness_level) / 100,
                            (effect_color.green * brightness_level) / 100,
                            (effect_color.blue * brightness_level) / 100);
            vTaskDelay(step_delay / portTICK_PERIOD_MS);
        }
        
        // Fade out
        for (uint32_t i = steps; i > 0 && effect_running; i--) {
            uint8_t brightness_level = (i * brightness) / steps;
            rgb_led_set_color((effect_color.red * brightness_level) / 100,
                            (effect_color.green * brightness_level) / 100,
                            (effect_color.blue * brightness_level) / 100);
            vTaskDelay(step_delay / portTICK_PERIOD_MS);
        }
    }
    
    effect_running = false;
    effect_task_handle = NULL;
    vTaskDelete(NULL);
}

/**
 * @brief Initialize RGB LED
 */
esp_err_t rgb_led_init(uint8_t gpio_num)
{
    ESP_LOGI(TAG, "Initializing RGB LED on GPIO%d", gpio_num);
    
    // RMT TX channel configuration
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = gpio_num,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));
    
    // LED strip encoder configuration
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));
    
    // Enable RMT channel
    ESP_ERROR_CHECK(rmt_enable(led_chan));
    
    // Clear LED
    memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
    update_led_strip();
    
    ESP_LOGI(TAG, "RGB LED initialized successfully");
    return ESP_OK;
}

/**
 * @brief Set LED mode
 */
esp_err_t rgb_led_set_mode(rgb_led_mode_t mode)
{
    current_mode = mode;
    
    // Stop any running effects
    rgb_led_stop_effect();
    
    switch (mode) {
        case LED_MODE_OFF:
            rgb_led_off();
            break;
            
        case LED_MODE_WIFI_CONNECTING:
            // Blue blinking (500ms on, 500ms off)
            rgb_led_blink((rgb_color_t)RGB_COLOR_BLUE, 500, 500, 0);
            break;
            
        case LED_MODE_WIFI_CONNECTED:
            // Green solid - save as base color
            base_color = (rgb_color_t){0, 255, 0};
            rgb_led_set_color(0, apply_brightness(255), 0);
            break;
            
        case LED_MODE_WIFI_ERROR:
            // Red blinking (250ms on, 250ms off)
            rgb_led_blink((rgb_color_t)RGB_COLOR_RED, 250, 250, 0);
            break;
            
        case LED_MODE_PROVISIONING:
            // Blue fast blink (200ms on, 200ms off)
            rgb_led_blink((rgb_color_t)RGB_COLOR_BLUE, 200, 200, 0);
            break;
            
        case LED_MODE_OTA_PROGRESS:
            // Purple pulsing
            rgb_led_pulse((rgb_color_t)RGB_COLOR_PURPLE, 2000);
            break;
            
        case LED_MODE_OTA_SUCCESS:
            // Green flash 3 times
            rgb_led_blink((rgb_color_t)RGB_COLOR_GREEN, 200, 200, 3);
            break;
            
        case LED_MODE_OTA_ERROR:
            // Red solid
            rgb_led_set_color(apply_brightness(255), 0, 0);
            break;
            
        case LED_MODE_FACTORY_RESET:
            // Red fast blink
            rgb_led_blink((rgb_color_t)RGB_COLOR_RED, 100, 100, 0);
            break;

        // IR Learning modes
        case LED_MODE_IR_LEARNING:
            // Purple blinking (500ms on, 500ms off, infinite)
            rgb_led_blink((rgb_color_t){128, 0, 255}, 500, 500, 0);
            break;

        case LED_MODE_IR_LEARNING_SUCCESS:
            // Green flash 3 times (100ms on, 100ms off)
            rgb_led_blink((rgb_color_t)RGB_COLOR_GREEN, 100, 100, 3);
            break;

        case LED_MODE_IR_LEARNING_FAILED:
            // Red flash 2 times (100ms on, 100ms off)
            rgb_led_blink((rgb_color_t)RGB_COLOR_RED, 100, 100, 2);
            break;

        case LED_MODE_IR_TRANSMITTING:
            // Cyan pulse (200ms on, no off, 1 time)
            rgb_led_blink((rgb_color_t)RGB_COLOR_CYAN, 200, 0, 1);
            break;

        case LED_MODE_CUSTOM:
            // Custom mode - do nothing, user will set color
            break;

        default:
            rgb_led_off();
            break;
    }
    
    return ESP_OK;
}

/**
 * @brief Set custom color (deprecated - use rgb_led_set_color)
 */
esp_err_t rgb_led_set_rgb_color(rgb_color_t color)
{
    return rgb_led_set_color(apply_brightness(color.red),
                            apply_brightness(color.green),
                            apply_brightness(color.blue));
}

/**
 * @brief Set RGB values directly
 */
esp_err_t rgb_led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
    // WS2812B uses GRB format
    led_strip_pixels[0] = green;
    led_strip_pixels[1] = red;
    led_strip_pixels[2] = blue;
    
    return update_led_strip();
}

/**
 * @brief Turn off LED
 */
esp_err_t rgb_led_off(void)
{
    rgb_led_stop_effect();
    return rgb_led_set_color(0, 0, 0);
}

/**
 * @brief Set LED brightness
 */
esp_err_t rgb_led_set_brightness(uint8_t new_brightness)
{
    if (new_brightness > 100) {
        new_brightness = 100;
    }
    
    brightness = new_brightness;
    ESP_LOGI(TAG, "Brightness set to %d%%", brightness);
    
    return ESP_OK;
}

/**
 * @brief Blink LED with custom pattern
 */
esp_err_t rgb_led_blink(rgb_color_t color, uint32_t on_time_ms, uint32_t off_time_ms, uint32_t repeat)
{
    // Stop any running effect
    rgb_led_stop_effect();
    
    effect_color = color;
    effect_on_time = on_time_ms;
    effect_off_time = off_time_ms;
    effect_repeat = repeat;
    effect_running = true;
    
    BaseType_t ret = xTaskCreate(blink_task, "rgb_blink", 2048, NULL, 5, &effect_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create blink task");
        effect_running = false;
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
 * @brief Pulse LED (fade in/out)
 */
esp_err_t rgb_led_pulse(rgb_color_t color, uint32_t period_ms)
{
    // Stop any running effect
    rgb_led_stop_effect();
    
    effect_color = color;
    effect_pulse_period = period_ms;
    effect_running = true;
    
    BaseType_t ret = xTaskCreate(pulse_task, "rgb_pulse", 2048, NULL, 5, &effect_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create pulse task");
        effect_running = false;
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
 * @brief Stop any ongoing LED effect
 */
esp_err_t rgb_led_stop_effect(void)
{
    if (effect_running) {
        effect_running = false;
        
        // Wait for task to finish
        if (effect_task_handle != NULL) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
    
    return ESP_OK;
}
