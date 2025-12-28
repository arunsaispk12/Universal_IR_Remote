/*
 * RGB LED Status Component Implementation
 * Provides visual feedback for system status using WS2812B LED
 */

#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/rmt_tx.h"
#include "rgb_led.h"
#include "led_strip_encoder.h"

static const char *TAG = "rgb_led";

// Default configuration
#define RGB_LED_DEFAULT_GPIO        22
#define RGB_LED_DEFAULT_COUNT       1
#define RGB_LED_RMT_RESOLUTION_HZ   10000000  // 10MHz resolution, 1 tick = 0.1us
#define RGB_LED_TASK_STACK_SIZE     2048
#define RGB_LED_TASK_PRIORITY       5

// Animation timing (in milliseconds)
#define PULSE_PERIOD_MS             2000
#define FLASH_ON_MS                 200
#define FLASH_OFF_MS                200
#define BLINK_ON_MS                 500
#define BLINK_OFF_MS                500

// Color definitions (GRB order for WS2812B)
typedef struct {
    uint8_t g;
    uint8_t r;
    uint8_t b;
} led_pixel_t;

// LED component state
typedef struct {
    rmt_channel_handle_t led_chan;
    rmt_encoder_handle_t led_encoder;
    rmt_transmit_config_t tx_config;
    rgb_led_config_t config;
    rgb_led_status_t current_status;
    rgb_led_color_t custom_color;
    led_pixel_t *led_buffer;
    TaskHandle_t task_handle;
    SemaphoreHandle_t mutex;
    bool initialized;
    bool task_running;
} rgb_led_state_t;

static rgb_led_state_t s_led_state = {0};

// Predefined colors (R, G, B format)
static const rgb_led_color_t COLOR_OFF = {0, 0, 0};
static const rgb_led_color_t COLOR_DIM_BLUE = {0, 0, 30};
static const rgb_led_color_t COLOR_PURPLE = {128, 0, 128};
static const rgb_led_color_t COLOR_GREEN = {0, 255, 0};
static const rgb_led_color_t COLOR_RED = {255, 0, 0};
static const rgb_led_color_t COLOR_CYAN = {0, 255, 255};
static const rgb_led_color_t COLOR_YELLOW = {255, 255, 0};

/**
 * @brief Scale color brightness
 */
static void scale_color(const rgb_led_color_t *input, rgb_led_color_t *output, float scale)
{
    output->r = (uint8_t)(input->r * scale);
    output->g = (uint8_t)(input->g * scale);
    output->b = (uint8_t)(input->b * scale);
}

/**
 * @brief Calculate pulsing brightness (sine wave)
 */
static float calculate_pulse_brightness(uint32_t time_ms, uint32_t period_ms)
{
    float angle = (float)(time_ms % period_ms) / period_ms * 2.0f * 3.14159f;
    return (sinf(angle) + 1.0f) / 2.0f; // 0.0 to 1.0
}

/**
 * @brief Set LED pixel color
 */
static void set_pixel_color(led_pixel_t *pixel, const rgb_led_color_t *color)
{
    pixel->r = color->r;
    pixel->g = color->g;
    pixel->b = color->b;
}

/**
 * @brief Update LED strip
 */
static esp_err_t update_led_strip(void)
{
    if (!s_led_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return rmt_transmit(s_led_state.led_chan, s_led_state.led_encoder,
                       s_led_state.led_buffer,
                       s_led_state.config.led_count * sizeof(led_pixel_t),
                       &s_led_state.tx_config);
}

/**
 * @brief LED animation task
 */
static void led_animation_task(void *arg)
{
    uint32_t time_ms = 0;
    rgb_led_color_t current_color = COLOR_OFF;
    rgb_led_status_t last_status = LED_STATUS_OFF;
    int flash_count = 0;
    bool flash_state = false;

    while (s_led_state.task_running) {
        // Get current status safely
        if (xSemaphoreTake(s_led_state.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            rgb_led_status_t status = s_led_state.current_status;

            // Reset animation on status change
            if (status != last_status) {
                time_ms = 0;
                flash_count = 0;
                flash_state = false;
                last_status = status;
            }

            // Handle each status
            switch (status) {
                case LED_STATUS_IDLE:
                    current_color = COLOR_DIM_BLUE;
                    break;

                case LED_STATUS_LEARNING: {
                    float brightness = calculate_pulse_brightness(time_ms, PULSE_PERIOD_MS);
                    scale_color(&COLOR_PURPLE, &current_color, brightness);
                    break;
                }

                case LED_STATUS_LEARN_SUCCESS:
                case LED_STATUS_LEARN_FAILED: {
                    const rgb_led_color_t *flash_color = (status == LED_STATUS_LEARN_SUCCESS) ?
                                                         &COLOR_GREEN : &COLOR_RED;

                    if (flash_count < 3) {
                        uint32_t cycle_time = time_ms % (FLASH_ON_MS + FLASH_OFF_MS);
                        if (cycle_time < FLASH_ON_MS) {
                            if (!flash_state) {
                                flash_state = true;
                            }
                            current_color = *flash_color;
                        } else {
                            if (flash_state) {
                                flash_state = false;
                                flash_count++;
                            }
                            current_color = COLOR_OFF;
                        }
                    } else {
                        // Return to idle after flashing
                        current_color = COLOR_OFF;
                        s_led_state.current_status = LED_STATUS_IDLE;
                    }
                    break;
                }

                case LED_STATUS_TRANSMITTING: {
                    if (time_ms < FLASH_ON_MS) {
                        current_color = COLOR_CYAN;
                    } else {
                        current_color = COLOR_OFF;
                        s_led_state.current_status = LED_STATUS_IDLE;
                    }
                    break;
                }

                case LED_STATUS_WIFI_CONNECTING: {
                    float brightness = calculate_pulse_brightness(time_ms, PULSE_PERIOD_MS);
                    scale_color(&COLOR_YELLOW, &current_color, brightness);
                    break;
                }

                case LED_STATUS_WIFI_CONNECTED:
                    current_color = COLOR_GREEN;
                    break;

                case LED_STATUS_ERROR: {
                    uint32_t cycle_time = time_ms % (BLINK_ON_MS + BLINK_OFF_MS);
                    current_color = (cycle_time < BLINK_ON_MS) ? COLOR_RED : COLOR_OFF;
                    break;
                }

                case LED_STATUS_OFF:
                default:
                    current_color = COLOR_OFF;
                    break;
            }

            // Update LED
            set_pixel_color(&s_led_state.led_buffer[0], &current_color);
            update_led_strip();

            xSemaphoreGive(s_led_state.mutex);
        }

        // Update timing
        time_ms += 50;
        vTaskDelay(pdMS_TO_TICKS(50)); // 50ms update rate
    }

    vTaskDelete(NULL);
}

esp_err_t rgb_led_init(const rgb_led_config_t *config)
{
    esp_err_t ret = ESP_OK;

    if (s_led_state.initialized) {
        ESP_LOGW(TAG, "RGB LED already initialized");
        return ESP_OK;
    }

    // Use provided config or defaults
    if (config) {
        s_led_state.config = *config;
    } else {
        s_led_state.config.gpio_num = RGB_LED_DEFAULT_GPIO;
        s_led_state.config.led_count = RGB_LED_DEFAULT_COUNT;
        s_led_state.config.rmt_channel = 0;
    }

    ESP_LOGI(TAG, "Initializing RGB LED on GPIO %d, %d LED(s)",
             s_led_state.config.gpio_num, s_led_state.config.led_count);

    // Allocate LED buffer
    s_led_state.led_buffer = calloc(s_led_state.config.led_count, sizeof(led_pixel_t));
    ESP_GOTO_ON_FALSE(s_led_state.led_buffer, ESP_ERR_NO_MEM, err, TAG, "Failed to allocate LED buffer");

    // Create mutex
    s_led_state.mutex = xSemaphoreCreateMutex();
    ESP_GOTO_ON_FALSE(s_led_state.mutex, ESP_ERR_NO_MEM, err, TAG, "Failed to create mutex");

    // Configure RMT TX channel
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = s_led_state.config.gpio_num,
        .mem_block_symbols = 64,
        .resolution_hz = RGB_LED_RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
        .flags.invert_out = false,
        .flags.with_dma = false,
    };

    ESP_GOTO_ON_ERROR(rmt_new_tx_channel(&tx_chan_config, &s_led_state.led_chan), err, TAG, "Failed to create RMT channel");

    // Create LED strip encoder
    led_strip_encoder_config_t encoder_config = {
        .resolution = RGB_LED_RMT_RESOLUTION_HZ,
    };

    ESP_GOTO_ON_ERROR(rmt_new_led_strip_encoder(&encoder_config, &s_led_state.led_encoder), err, TAG, "Failed to create LED encoder");

    // Enable RMT channel
    ESP_GOTO_ON_ERROR(rmt_enable(s_led_state.led_chan), err, TAG, "Failed to enable RMT channel");

    // Configure transmit options
    s_led_state.tx_config.loop_count = 0; // No loop

    // Initialize to OFF
    s_led_state.current_status = LED_STATUS_OFF;
    memset(s_led_state.led_buffer, 0, s_led_state.config.led_count * sizeof(led_pixel_t));
    update_led_strip();

    // Start animation task
    s_led_state.task_running = true;
    BaseType_t task_created = xTaskCreate(
        led_animation_task,
        "rgb_led_task",
        RGB_LED_TASK_STACK_SIZE,
        NULL,
        RGB_LED_TASK_PRIORITY,
        &s_led_state.task_handle
    );

    ESP_GOTO_ON_FALSE(task_created == pdPASS, ESP_ERR_NO_MEM, err, TAG, "Failed to create LED task");

    s_led_state.initialized = true;
    ESP_LOGI(TAG, "RGB LED initialized successfully");
    return ESP_OK;

err:
    rgb_led_deinit();
    return ret;
}

esp_err_t rgb_led_set_status(rgb_led_status_t status)
{
    if (!s_led_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_led_state.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_led_state.current_status = status;
        ESP_LOGD(TAG, "LED status set to %d", status);
        xSemaphoreGive(s_led_state.mutex);
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t rgb_led_set_color(const rgb_led_color_t *color)
{
    if (!s_led_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!color) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_led_state.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_led_state.custom_color = *color;
        set_pixel_color(&s_led_state.led_buffer[0], color);
        update_led_strip();
        xSemaphoreGive(s_led_state.mutex);
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t rgb_led_turn_off(void)
{
    return rgb_led_set_status(LED_STATUS_OFF);
}

rgb_led_status_t rgb_led_get_status(void)
{
    rgb_led_status_t status = LED_STATUS_OFF;

    if (s_led_state.initialized && xSemaphoreTake(s_led_state.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        status = s_led_state.current_status;
        xSemaphoreGive(s_led_state.mutex);
    }

    return status;
}

esp_err_t rgb_led_deinit(void)
{
    if (!s_led_state.initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing RGB LED");

    // Stop task
    if (s_led_state.task_handle) {
        s_led_state.task_running = false;
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait for task to exit
        s_led_state.task_handle = NULL;
    }

    // Turn off LED
    if (s_led_state.led_buffer) {
        memset(s_led_state.led_buffer, 0, s_led_state.config.led_count * sizeof(led_pixel_t));
        update_led_strip();
    }

    // Cleanup RMT
    if (s_led_state.led_chan) {
        rmt_disable(s_led_state.led_chan);
        rmt_del_channel(s_led_state.led_chan);
        s_led_state.led_chan = NULL;
    }

    if (s_led_state.led_encoder) {
        rmt_del_encoder(s_led_state.led_encoder);
        s_led_state.led_encoder = NULL;
    }

    // Free resources
    if (s_led_state.led_buffer) {
        free(s_led_state.led_buffer);
        s_led_state.led_buffer = NULL;
    }

    if (s_led_state.mutex) {
        vSemaphoreDelete(s_led_state.mutex);
        s_led_state.mutex = NULL;
    }

    s_led_state.initialized = false;
    ESP_LOGI(TAG, "RGB LED deinitialized");

    return ESP_OK;
}
