/*
 * RGB LED Component - Usage Example
 *
 * This file demonstrates how to integrate the RGB LED component
 * into your Universal IR Remote project.
 */

#include "rgb_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "rgb_example";

/**
 * Example 1: Basic initialization and status updates
 */
void example_basic_usage(void)
{
    ESP_LOGI(TAG, "Example 1: Basic Usage");

    // Initialize with default settings (GPIO 22, 1 LED)
    esp_err_t ret = rgb_led_init(NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RGB LED: %s", esp_err_to_name(ret));
        return;
    }

    // Start with idle status
    rgb_led_set_status(LED_STATUS_IDLE);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Turn off
    rgb_led_turn_off();
}

/**
 * Example 2: Custom configuration
 */
void example_custom_config(void)
{
    ESP_LOGI(TAG, "Example 2: Custom Configuration");

    // Configure for different GPIO and multiple LEDs
    rgb_led_config_t config = {
        .gpio_num = 22,      // Your GPIO pin
        .led_count = 1,      // Number of WS2812B LEDs
        .rmt_channel = 0     // RMT channel (0-3)
    };

    esp_err_t ret = rgb_led_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RGB LED: %s", esp_err_to_name(ret));
        return;
    }

    rgb_led_set_status(LED_STATUS_IDLE);
}

/**
 * Example 3: IR Learning sequence
 */
void example_ir_learning(void)
{
    ESP_LOGI(TAG, "Example 3: IR Learning Sequence");

    // Indicate learning mode
    rgb_led_set_status(LED_STATUS_LEARNING);
    ESP_LOGI(TAG, "Waiting for IR signal... (purple pulsing)");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Simulate learning success
    ESP_LOGI(TAG, "Learning succeeded!");
    rgb_led_set_status(LED_STATUS_LEARN_SUCCESS);
    vTaskDelay(pdMS_TO_TICKS(2000)); // Flashes 3 times, then returns to idle

    // Simulate learning failure
    ESP_LOGI(TAG, "Learning failed!");
    rgb_led_set_status(LED_STATUS_LEARN_FAILED);
    vTaskDelay(pdMS_TO_TICKS(2000)); // Flashes 3 times, then returns to idle
}

/**
 * Example 4: IR Transmission
 */
void example_ir_transmission(void)
{
    ESP_LOGI(TAG, "Example 4: IR Transmission");

    // Show transmission
    rgb_led_set_status(LED_STATUS_TRANSMITTING);
    ESP_LOGI(TAG, "Transmitting IR signal... (cyan flash)");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Flashes once, then returns to idle
}

/**
 * Example 5: WiFi connection sequence
 */
void example_wifi_sequence(void)
{
    ESP_LOGI(TAG, "Example 5: WiFi Connection Sequence");

    // Connecting
    rgb_led_set_status(LED_STATUS_WIFI_CONNECTING);
    ESP_LOGI(TAG, "Connecting to WiFi... (yellow pulsing)");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Connected
    rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);
    ESP_LOGI(TAG, "WiFi connected! (green solid)");
    vTaskDelay(pdMS_TO_TICKS(3000));
}

/**
 * Example 6: Error indication
 */
void example_error_indication(void)
{
    ESP_LOGI(TAG, "Example 6: Error Indication");

    // Show error
    rgb_led_set_status(LED_STATUS_ERROR);
    ESP_LOGE(TAG, "Error occurred! (red blinking)");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Clear error
    rgb_led_set_status(LED_STATUS_IDLE);
}

/**
 * Example 7: Custom color
 */
void example_custom_color(void)
{
    ESP_LOGI(TAG, "Example 7: Custom Color");

    // Set orange color
    rgb_led_color_t orange = {
        .r = 255,
        .g = 165,
        .b = 0
    };
    rgb_led_set_color(&orange);
    ESP_LOGI(TAG, "Custom color: Orange");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Set pink color
    rgb_led_color_t pink = {
        .r = 255,
        .g = 105,
        .b = 180
    };
    rgb_led_set_color(&pink);
    ESP_LOGI(TAG, "Custom color: Pink");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Return to status-based control
    rgb_led_set_status(LED_STATUS_IDLE);
}

/**
 * Example 8: Integration with IR Remote application
 */
void app_main_example(void)
{
    ESP_LOGI(TAG, "=== RGB LED Component Examples ===");

    // Initialize RGB LED
    rgb_led_config_t config = {
        .gpio_num = 22,
        .led_count = 1,
        .rmt_channel = 0
    };

    if (rgb_led_init(&config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RGB LED");
        return;
    }

    // Run examples
    example_basic_usage();
    vTaskDelay(pdMS_TO_TICKS(1000));

    example_ir_learning();
    vTaskDelay(pdMS_TO_TICKS(1000));

    example_ir_transmission();
    vTaskDelay(pdMS_TO_TICKS(1000));

    example_wifi_sequence();
    vTaskDelay(pdMS_TO_TICKS(1000));

    example_error_indication();
    vTaskDelay(pdMS_TO_TICKS(1000));

    example_custom_color();
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Cleanup
    rgb_led_deinit();
    ESP_LOGI(TAG, "=== Examples Complete ===");
}

/**
 * Example 9: Typical IR Remote main application integration
 */
void typical_ir_remote_integration(void)
{
    // Initialize RGB LED early in app_main
    rgb_led_config_t led_config = {
        .gpio_num = 22,
        .led_count = 1,
        .rmt_channel = 0
    };
    rgb_led_init(&led_config);

    // Show WiFi connecting
    rgb_led_set_status(LED_STATUS_WIFI_CONNECTING);

    // ... WiFi initialization code ...

    // After WiFi connects
    rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);

    // In your IR learning function:
    void start_learning(void) {
        rgb_led_set_status(LED_STATUS_LEARNING);
        // ... learning logic ...
    }

    // In your IR transmit function:
    void transmit_signal(void) {
        rgb_led_set_status(LED_STATUS_TRANSMITTING);
        // ... transmit logic ...
    }

    // In error handler:
    void handle_error(void) {
        rgb_led_set_status(LED_STATUS_ERROR);
    }

    // Idle state when not active:
    rgb_led_set_status(LED_STATUS_IDLE);
}

/*
 * Integration Tips:
 *
 * 1. Call rgb_led_init() early in app_main()
 * 2. Update status at key application events
 * 3. Status changes are non-blocking - call from any task
 * 4. LEARN_SUCCESS and LEARN_FAILED auto-return to IDLE
 * 5. TRANSMITTING auto-returns to IDLE after flash
 * 6. Use WIFI_CONNECTED during normal operation
 * 7. Use ERROR for critical failures
 * 8. Call rgb_led_deinit() before app shutdown (optional)
 */
