/**
 * @file ir_control.c
 * @brief Universal IR Remote Control Implementation
 *
 * Multi-protocol IR transmitter and receiver with NVS storage
 * Supports 25+ IR protocols including NEC, Samsung, Sony, JVC, LG, and more
 *
 * Based on SHA_RainMaker_Project v1.0.0
 * Extended with Arduino-IRremote protocol support
 */

#include "ir_control.h"
#include "ir_protocols.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "driver/rmt_encoder.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

// Include all protocol decoders
#include "decoders/ir_distance_width.h"
#include "decoders/ir_sony.h"
#include "decoders/ir_jvc.h"
#include "decoders/ir_lg.h"
#include "decoders/ir_denon.h"
#include "decoders/ir_panasonic.h"
#include "decoders/ir_samsung48.h"
#include "decoders/ir_whynter.h"
#include "decoders/ir_lego.h"
#include "decoders/ir_magiquest.h"
#include "decoders/ir_bosewave.h"
#include "decoders/ir_fast.h"
#include "decoders/ir_apple.h"

static const char *TAG = "IR_CONTROL";

/* ============================================================================
 * RMT CONFIGURATION
 * ============================================================================ */

#define RMT_TICK_RESOLUTION_HZ  1000000  // 1MHz resolution, 1 tick = 1us

/* ============================================================================
 * IR PROTOCOL TIMING (in microseconds)
 * ============================================================================ */

// NEC Protocol
#define NEC_LEADING_CODE_HIGH_US    9000
#define NEC_LEADING_CODE_LOW_US     4500
#define NEC_PAYLOAD_ONE_HIGH_US     560
#define NEC_PAYLOAD_ONE_LOW_US      1690
#define NEC_PAYLOAD_ZERO_HIGH_US    560
#define NEC_PAYLOAD_ZERO_LOW_US     560
#define NEC_REPEAT_CODE_HIGH_US     9000
#define NEC_REPEAT_CODE_LOW_US      2250

// Samsung Protocol (variant)
#define SAMSUNG_LEADING_CODE_HIGH_US 4500
#define SAMSUNG_LEADING_CODE_LOW_US  4500

// Timing tolerance
#define IR_TIMING_TOLERANCE_US      300  // Increased for Samsung remotes

/* ============================================================================
 * NVS STORAGE
 * ============================================================================ */

#define IR_NVS_NAMESPACE   "ir_codes"
#define MAX_CODE_SIZE      512

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

// RMT channels and encoders
static rmt_channel_handle_t tx_channel = NULL;
static rmt_channel_handle_t rx_channel = NULL;
static rmt_encoder_handle_t nec_encoder = NULL;
static rmt_encoder_handle_t samsung_encoder = NULL;
static rmt_encoder_handle_t copy_encoder = NULL;

// RX buffers and queue
static rmt_symbol_word_t raw_symbols[IR_MAX_CODE_LENGTH];
static QueueHandle_t receive_queue = NULL;
static TaskHandle_t rx_task_handle = NULL;
static rmt_receive_config_t receive_config;

// Learned codes storage
static ir_code_t learned_codes[IR_BTN_MAX];
static SemaphoreHandle_t codes_mutex = NULL;

// Learning mode state
static bool learning_mode = false;
static ir_button_t current_learning_button = IR_BTN_MAX;
static esp_timer_handle_t learning_timer = NULL;

// Callbacks
static ir_callbacks_t callbacks = {0};

/* ============================================================================
 * BUTTON NAMES
 * ============================================================================ */

static const char* button_names[IR_BTN_MAX] = {
    "POWER", "SOURCE", "MENU", "HOME", "BACK", "OK",
    "VOL_UP", "VOL_DN", "MUTE",
    "CH_UP", "CH_DN",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "UP", "DOWN", "LEFT", "RIGHT",
    "CUSTOM_1", "CUSTOM_2", "CUSTOM_3", "CUSTOM_4", "CUSTOM_5", "CUSTOM_6"
};

static const char* protocol_names[] = {
    "UNKNOWN", "NEC", "SAMSUNG", "RAW"
};

/* ============================================================================
 * TIMING HELPER FUNCTIONS
 * ============================================================================ */

/**
 * @brief Check if timing matches expected value within tolerance
 */
static inline bool timing_matches(uint32_t actual, uint32_t expected, uint32_t tolerance)
{
    return (actual > (expected - tolerance)) && (actual < (expected + tolerance));
}

/* ============================================================================
 * NEC PROTOCOL DECODER
 * ============================================================================ */

/**
 * @brief Decode NEC protocol from RMT symbols
 */
static esp_err_t decode_nec_protocol(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code)
{
    if (num_symbols < 34) {  // Minimum: leading code (1) + 32 bits (32) + ending (1)
        ESP_LOGD(TAG, "NEC: Not enough symbols: %d", num_symbols);
        return ESP_ERR_INVALID_ARG;
    }

    // Check leading code (9ms HIGH + 4.5ms LOW)
    if (!timing_matches(symbols[0].duration0, NEC_LEADING_CODE_HIGH_US, IR_TIMING_TOLERANCE_US)) {
        ESP_LOGD(TAG, "NEC: Invalid leading HIGH: %d", symbols[0].duration0);
        return ESP_ERR_INVALID_ARG;
    }

    if (!timing_matches(symbols[0].duration1, NEC_LEADING_CODE_LOW_US, IR_TIMING_TOLERANCE_US)) {
        // Check for repeat code (9ms HIGH + 2.25ms LOW)
        if (timing_matches(symbols[0].duration1, NEC_REPEAT_CODE_LOW_US, IR_TIMING_TOLERANCE_US)) {
            ESP_LOGD(TAG, "NEC: Repeat code detected");
            return ESP_ERR_NOT_SUPPORTED;  // Repeat codes not handled
        }
        ESP_LOGD(TAG, "NEC: Invalid leading LOW: %d", symbols[0].duration1);
        return ESP_ERR_INVALID_ARG;
    }

    // Decode 32 bits (address + ~address + command + ~command)
    uint32_t decoded_data = 0;
    for (int i = 0; i < 32; i++) {
        int symbol_idx = i + 1;  // Skip leading code
        if (symbol_idx >= num_symbols) {
            ESP_LOGD(TAG, "NEC: Unexpected end of data at bit %d", i);
            return ESP_ERR_INVALID_ARG;
        }

        const rmt_symbol_word_t *symbol = &symbols[symbol_idx];

        // Check if it's a valid pulse (HIGH followed by LOW)
        if (symbol->level0 != 1 || symbol->level1 != 0) {
            ESP_LOGD(TAG, "NEC: Invalid pulse levels at bit %d", i);
            return ESP_ERR_INVALID_ARG;
        }

        // HIGH pulse should be ~560us for both 0 and 1
        if (!timing_matches(symbol->duration0, NEC_PAYLOAD_ZERO_HIGH_US, IR_TIMING_TOLERANCE_US)) {
            ESP_LOGD(TAG, "NEC: Invalid pulse HIGH at bit %d: %d", i, symbol->duration0);
            return ESP_ERR_INVALID_ARG;
        }

        // Determine bit value based on LOW pulse duration
        if (timing_matches(symbol->duration1, NEC_PAYLOAD_ONE_LOW_US, IR_TIMING_TOLERANCE_US)) {
            // Bit is '1' (~1690us LOW)
            decoded_data |= (1UL << i);
        } else if (!timing_matches(symbol->duration1, NEC_PAYLOAD_ZERO_LOW_US, IR_TIMING_TOLERANCE_US)) {
            // Invalid timing
            ESP_LOGD(TAG, "NEC: Invalid pulse LOW at bit %d: %d", i, symbol->duration1);
            return ESP_ERR_INVALID_ARG;
        }
        // Bit is '0' (~560us LOW) - no action needed
    }

    // Extract address and command
    uint8_t address = decoded_data & 0xFF;
    uint8_t address_inv = (decoded_data >> 8) & 0xFF;
    uint8_t command = (decoded_data >> 16) & 0xFF;
    uint8_t command_inv = (decoded_data >> 24) & 0xFF;

    // Verify inverted bits
    if ((address ^ address_inv) != 0xFF || (command ^ command_inv) != 0xFF) {
        ESP_LOGD(TAG, "NEC: Checksum failed: addr=0x%02X/0x%02X, cmd=0x%02X/0x%02X",
                 address, address_inv, command, command_inv);
        return ESP_ERR_INVALID_CRC;
    }

    // Fill in the code structure
    code->protocol = IR_PROTOCOL_NEC;
    code->data = decoded_data;
    code->bits = 32;
    code->raw_length = 0;
    code->raw_data = NULL;

    ESP_LOGI(TAG, "Decoded NEC: Addr=0x%02X, Cmd=0x%02X, Data=0x%08lX", address, command, decoded_data);

    return ESP_OK;
}

/* ============================================================================
 * SAMSUNG PROTOCOL DECODER
 * ============================================================================ */

/**
 * @brief Decode Samsung protocol from RMT symbols
 */
static esp_err_t decode_samsung_protocol(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code)
{
    if (num_symbols < 34) {
        ESP_LOGD(TAG, "Samsung: Not enough symbols: %d", num_symbols);
        return ESP_ERR_INVALID_ARG;
    }

    // Check Samsung leading code (4.5ms HIGH + 4.5ms LOW)
    if (!timing_matches(symbols[0].duration0, SAMSUNG_LEADING_CODE_HIGH_US, IR_TIMING_TOLERANCE_US)) {
        ESP_LOGD(TAG, "Samsung: Invalid leading HIGH: %d", symbols[0].duration0);
        return ESP_ERR_INVALID_ARG;
    }

    if (!timing_matches(symbols[0].duration1, SAMSUNG_LEADING_CODE_LOW_US, IR_TIMING_TOLERANCE_US)) {
        ESP_LOGD(TAG, "Samsung: Invalid leading LOW: %d", symbols[0].duration1);
        return ESP_ERR_INVALID_ARG;
    }

    // Decode 32 bits (same bit encoding as NEC)
    uint32_t decoded_data = 0;
    for (int i = 0; i < 32; i++) {
        int symbol_idx = i + 1;
        if (symbol_idx >= num_symbols) {
            return ESP_ERR_INVALID_ARG;
        }

        const rmt_symbol_word_t *symbol = &symbols[symbol_idx];

        if (symbol->level0 != 1 || symbol->level1 != 0) {
            return ESP_ERR_INVALID_ARG;
        }

        if (!timing_matches(symbol->duration0, NEC_PAYLOAD_ZERO_HIGH_US, IR_TIMING_TOLERANCE_US)) {
            return ESP_ERR_INVALID_ARG;
        }

        if (timing_matches(symbol->duration1, NEC_PAYLOAD_ONE_LOW_US, IR_TIMING_TOLERANCE_US)) {
            decoded_data |= (1UL << i);
        } else if (!timing_matches(symbol->duration1, NEC_PAYLOAD_ZERO_LOW_US, IR_TIMING_TOLERANCE_US)) {
            return ESP_ERR_INVALID_ARG;
        }
    }

    code->protocol = IR_PROTOCOL_SAMSUNG;
    code->data = decoded_data;
    code->bits = 32;
    code->raw_length = 0;
    code->raw_data = NULL;

    ESP_LOGI(TAG, "Decoded Samsung: Data=0x%08lX", decoded_data);

    return ESP_OK;
}

/* ============================================================================
 * LEARNING MODE TIMEOUT
 * ============================================================================ */

/**
 * @brief Learning mode timeout callback
 */
static void learning_timeout_callback(void *arg)
{
    ESP_LOGW(TAG, "Learning timeout for button '%s'", button_names[current_learning_button]);

    // Call failure callback
    if (callbacks.learn_fail_cb) {
        callbacks.learn_fail_cb(current_learning_button, callbacks.user_arg);
    }

    // Stop learning mode
    learning_mode = false;
    current_learning_button = IR_BTN_MAX;
}

/* ============================================================================
 * IR RECEIVE TASK
 * ============================================================================ */

/**
 * @brief IR receive task - processes incoming IR codes
 */
static void ir_receive_task(void *pvParameters)
{
    rmt_rx_done_event_data_t rx_data;
    ir_code_t received_code;

    ESP_LOGI(TAG, "IR receive task started");

    while (1) {
        // Wait for received data from ISR
        if (xQueueReceive(receive_queue, &rx_data, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Received %d RMT symbols", rx_data.num_symbols);

            memset(&received_code, 0, sizeof(ir_code_t));
            esp_err_t ret = ESP_FAIL;

            // ========== COMPREHENSIVE DECODER CHAIN ==========
            // Try protocols in priority order (most common first for performance)

            // TIER 1: Most common consumer protocols
            ret = decode_nec_protocol(rx_data.received_symbols, rx_data.num_symbols, &received_code);

            if (ret != ESP_OK) {
                ret = decode_samsung_protocol(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            if (ret != ESP_OK) {
                ret = ir_decode_sony(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            if (ret != ESP_OK) {
                ret = ir_decode_jvc(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            if (ret != ESP_OK) {
                ret = ir_decode_lg(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            // TIER 2: Extended consumer protocols
            if (ret != ESP_OK) {
                ret = ir_decode_denon(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            if (ret != ESP_OK) {
                ret = ir_decode_panasonic(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            if (ret != ESP_OK) {
                ret = ir_decode_samsung48(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            if (ret != ESP_OK) {
                ret = ir_decode_apple(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            // TIER 3: Exotic protocols
            if (ret != ESP_OK) {
                ret = ir_decode_whynter(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            if (ret != ESP_OK) {
                ret = ir_decode_lego(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            if (ret != ESP_OK) {
                ret = ir_decode_magiquest(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            if (ret != ESP_OK) {
                ret = ir_decode_bosewave(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            if (ret != ESP_OK) {
                ret = ir_decode_fast(rx_data.received_symbols, rx_data.num_symbols, &received_code);
            }

            // TIER 4: Universal decoder (fallback for unknown protocols)
            if (ret != ESP_OK) {
                ret = ir_decode_distance_width(rx_data.received_symbols, rx_data.num_symbols, &received_code);
                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "Universal decoder successfully decoded unknown protocol");
                }
            }

            if (ret == ESP_OK) {
                // Successfully decoded

                if (learning_mode && current_learning_button < IR_BTN_MAX) {
                    // Learning mode: Store the code
                    xSemaphoreTake(codes_mutex, portMAX_DELAY);
                    learned_codes[current_learning_button] = received_code;
                    xSemaphoreGive(codes_mutex);

                    ESP_LOGI(TAG, "Learned %s code for button '%s'",
                             protocol_names[received_code.protocol],
                             button_names[current_learning_button]);

                    // Auto-save
                    ir_save_code(current_learning_button, &received_code);

                    // Call success callback
                    if (callbacks.learn_success_cb) {
                        callbacks.learn_success_cb(current_learning_button, &received_code, callbacks.user_arg);
                    }

                    // Stop learning timer
                    if (learning_timer) {
                        esp_timer_stop(learning_timer);
                    }

                    // Exit learning mode
                    learning_mode = false;
                    current_learning_button = IR_BTN_MAX;
                } else {
                    // Normal mode: Call receive callback
                    if (callbacks.receive_cb) {
                        callbacks.receive_cb(&received_code, callbacks.user_arg);
                    }
                }
            } else if (ret == ESP_ERR_NOT_SUPPORTED) {
                ESP_LOGD(TAG, "Repeat code received (ignored)");
            } else {
                // Failed to decode - check if it's valid for RAW storage
                if (rx_data.num_symbols >= 10 && rx_data.num_symbols <= IR_MAX_CODE_LENGTH) {
                    ESP_LOGI(TAG, "Non-standard protocol detected (%d symbols)", rx_data.num_symbols);

                    if (learning_mode && current_learning_button < IR_BTN_MAX) {
                        // Store as RAW code
                        xSemaphoreTake(codes_mutex, portMAX_DELAY);

                        // Free any existing raw_data
                        if (learned_codes[current_learning_button].raw_data != NULL) {
                            free(learned_codes[current_learning_button].raw_data);
                        }

                        // Allocate memory for raw symbols
                        size_t raw_data_size = rx_data.num_symbols * sizeof(rmt_symbol_word_t);
                        rmt_symbol_word_t *raw_data = (rmt_symbol_word_t *)malloc(raw_data_size);

                        if (raw_data != NULL) {
                            memcpy(raw_data, rx_data.received_symbols, raw_data_size);

                            received_code.protocol = IR_PROTOCOL_RAW;
                            received_code.data = 0;
                            received_code.bits = 0;
                            received_code.raw_data = (uint16_t *)raw_data;
                            received_code.raw_length = rx_data.num_symbols;

                            learned_codes[current_learning_button] = received_code;

                            ESP_LOGI(TAG, "Learned RAW code for button '%s' (%d symbols)",
                                     button_names[current_learning_button], rx_data.num_symbols);

                            // Auto-save
                            ir_save_code(current_learning_button, &received_code);

                            // Call success callback
                            if (callbacks.learn_success_cb) {
                                callbacks.learn_success_cb(current_learning_button, &received_code, callbacks.user_arg);
                            }

                            // Stop learning timer
                            if (learning_timer) {
                                esp_timer_stop(learning_timer);
                            }

                            learning_mode = false;
                            current_learning_button = IR_BTN_MAX;
                        } else {
                            ESP_LOGE(TAG, "Failed to allocate memory for RAW data");

                            // Call failure callback
                            if (callbacks.learn_fail_cb) {
                                callbacks.learn_fail_cb(current_learning_button, callbacks.user_arg);
                            }

                            learning_mode = false;
                            current_learning_button = IR_BTN_MAX;
                        }

                        xSemaphoreGive(codes_mutex);
                    } else {
                        // Normal mode: Create temporary RAW code and call callback
                        size_t raw_data_size = rx_data.num_symbols * sizeof(rmt_symbol_word_t);
                        rmt_symbol_word_t *raw_data = (rmt_symbol_word_t *)malloc(raw_data_size);

                        if (raw_data != NULL) {
                            memcpy(raw_data, rx_data.received_symbols, raw_data_size);

                            received_code.protocol = IR_PROTOCOL_RAW;
                            received_code.data = 0;
                            received_code.bits = 0;
                            received_code.raw_data = (uint16_t *)raw_data;
                            received_code.raw_length = rx_data.num_symbols;

                            if (callbacks.receive_cb) {
                                callbacks.receive_cb(&received_code, callbacks.user_arg);
                            }

                            free(raw_data);
                        }
                    }
                } else if (learning_mode) {
                    ESP_LOGW(TAG, "Invalid IR signal: %d symbols (need 10-%d)",
                             rx_data.num_symbols, IR_MAX_CODE_LENGTH);

                    // Call failure callback
                    if (callbacks.learn_fail_cb) {
                        callbacks.learn_fail_cb(current_learning_button, callbacks.user_arg);
                    }
                }
            }

            // Restart receiving
            rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config);
        }
    }
}

/* ============================================================================
 * NEC ENCODER IMPLEMENTATION
 * ============================================================================ */

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *copy_encoder;
    rmt_encoder_t *bytes_encoder;
    rmt_symbol_word_t nec_leading_symbol;
    rmt_symbol_word_t nec_ending_symbol;
    int state;
} rmt_nec_encoder_t;

static size_t rmt_encode_nec(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                             const void *primary_data, size_t data_size,
                             rmt_encode_state_t *ret_state)
{
    rmt_nec_encoder_t *nec_encoder = __containerof(encoder, rmt_nec_encoder_t, base);
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;
    ir_code_t *code = (ir_code_t *)primary_data;

    switch (nec_encoder->state) {
    case 0: // Send leading code
        encoded_symbols += nec_encoder->copy_encoder->encode(nec_encoder->copy_encoder, channel,
                          &nec_encoder->nec_leading_symbol, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            nec_encoder->state = 1;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
        __attribute__((fallthrough));
    case 1: // Send data
        encoded_symbols += nec_encoder->bytes_encoder->encode(nec_encoder->bytes_encoder, channel,
                          &code->data, sizeof(uint32_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            nec_encoder->state = 2;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
        __attribute__((fallthrough));
    case 2: // Send ending code
        encoded_symbols += nec_encoder->copy_encoder->encode(nec_encoder->copy_encoder, channel,
                          &nec_encoder->nec_ending_symbol, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            nec_encoder->state = RMT_ENCODING_RESET;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_nec_encoder(rmt_encoder_t *encoder)
{
    rmt_nec_encoder_t *nec_encoder = __containerof(encoder, rmt_nec_encoder_t, base);
    rmt_del_encoder(nec_encoder->copy_encoder);
    rmt_del_encoder(nec_encoder->bytes_encoder);
    free(nec_encoder);
    return ESP_OK;
}

static esp_err_t rmt_nec_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_nec_encoder_t *nec_encoder = __containerof(encoder, rmt_nec_encoder_t, base);
    rmt_encoder_reset(nec_encoder->copy_encoder);
    rmt_encoder_reset(nec_encoder->bytes_encoder);
    nec_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

static esp_err_t rmt_new_nec_encoder(const rmt_encoder_t **ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_nec_encoder_t *nec_encoder = NULL;

    nec_encoder = calloc(1, sizeof(rmt_nec_encoder_t));
    if (nec_encoder == NULL) {
        return ESP_ERR_NO_MEM;
    }

    nec_encoder->base.encode = rmt_encode_nec;
    nec_encoder->base.del = rmt_del_nec_encoder;
    nec_encoder->base.reset = rmt_nec_encoder_reset;

    nec_encoder->nec_leading_symbol = (rmt_symbol_word_t) {
        .level0 = 1,
        .duration0 = NEC_LEADING_CODE_HIGH_US,
        .level1 = 0,
        .duration1 = NEC_LEADING_CODE_LOW_US,
    };

    nec_encoder->nec_ending_symbol = (rmt_symbol_word_t) {
        .level0 = 1,
        .duration0 = NEC_PAYLOAD_ZERO_HIGH_US,
        .level1 = 0,
        .duration1 = 0x7FFF,
    };

    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = NEC_PAYLOAD_ZERO_HIGH_US,
            .level1 = 0,
            .duration1 = NEC_PAYLOAD_ZERO_LOW_US,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = NEC_PAYLOAD_ONE_HIGH_US,
            .level1 = 0,
            .duration1 = NEC_PAYLOAD_ONE_LOW_US,
        },
        .flags.msb_first = 1,
    };

    ret = rmt_new_bytes_encoder(&bytes_encoder_config, &nec_encoder->bytes_encoder);
    if (ret != ESP_OK) {
        goto err;
    }

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ret = rmt_new_copy_encoder(&copy_encoder_config, &nec_encoder->copy_encoder);
    if (ret != ESP_OK) {
        goto err;
    }

    *ret_encoder = &nec_encoder->base;
    return ESP_OK;

err:
    if (nec_encoder) {
        if (nec_encoder->bytes_encoder) {
            rmt_del_encoder(nec_encoder->bytes_encoder);
        }
        if (nec_encoder->copy_encoder) {
            rmt_del_encoder(nec_encoder->copy_encoder);
        }
        free(nec_encoder);
    }
    return ret;
}

/* ============================================================================
 * SAMSUNG ENCODER (same as NEC but different leading code)
 * ============================================================================ */

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *copy_encoder;
    rmt_encoder_t *bytes_encoder;
    rmt_symbol_word_t samsung_leading_symbol;
    rmt_symbol_word_t samsung_ending_symbol;
    int state;
} rmt_samsung_encoder_t;

static size_t rmt_encode_samsung(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                                 const void *primary_data, size_t data_size,
                                 rmt_encode_state_t *ret_state)
{
    rmt_samsung_encoder_t *samsung_encoder = __containerof(encoder, rmt_samsung_encoder_t, base);
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;
    ir_code_t *code = (ir_code_t *)primary_data;

    switch (samsung_encoder->state) {
    case 0:
        encoded_symbols += samsung_encoder->copy_encoder->encode(samsung_encoder->copy_encoder, channel,
                          &samsung_encoder->samsung_leading_symbol, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            samsung_encoder->state = 1;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
        __attribute__((fallthrough));
    case 1:
        encoded_symbols += samsung_encoder->bytes_encoder->encode(samsung_encoder->bytes_encoder, channel,
                          &code->data, sizeof(uint32_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            samsung_encoder->state = 2;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
        __attribute__((fallthrough));
    case 2:
        encoded_symbols += samsung_encoder->copy_encoder->encode(samsung_encoder->copy_encoder, channel,
                          &samsung_encoder->samsung_ending_symbol, sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            samsung_encoder->state = RMT_ENCODING_RESET;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_samsung_encoder(rmt_encoder_t *encoder)
{
    rmt_samsung_encoder_t *samsung_encoder = __containerof(encoder, rmt_samsung_encoder_t, base);
    rmt_del_encoder(samsung_encoder->copy_encoder);
    rmt_del_encoder(samsung_encoder->bytes_encoder);
    free(samsung_encoder);
    return ESP_OK;
}

static esp_err_t rmt_samsung_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_samsung_encoder_t *samsung_encoder = __containerof(encoder, rmt_samsung_encoder_t, base);
    rmt_encoder_reset(samsung_encoder->copy_encoder);
    rmt_encoder_reset(samsung_encoder->bytes_encoder);
    samsung_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

static esp_err_t rmt_new_samsung_encoder(const rmt_encoder_t **ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_samsung_encoder_t *samsung_encoder = NULL;

    samsung_encoder = calloc(1, sizeof(rmt_samsung_encoder_t));
    if (samsung_encoder == NULL) {
        return ESP_ERR_NO_MEM;
    }

    samsung_encoder->base.encode = rmt_encode_samsung;
    samsung_encoder->base.del = rmt_del_samsung_encoder;
    samsung_encoder->base.reset = rmt_samsung_encoder_reset;

    samsung_encoder->samsung_leading_symbol = (rmt_symbol_word_t) {
        .level0 = 1,
        .duration0 = SAMSUNG_LEADING_CODE_HIGH_US,
        .level1 = 0,
        .duration1 = SAMSUNG_LEADING_CODE_LOW_US,
    };

    samsung_encoder->samsung_ending_symbol = (rmt_symbol_word_t) {
        .level0 = 1,
        .duration0 = NEC_PAYLOAD_ZERO_HIGH_US,
        .level1 = 0,
        .duration1 = 0x7FFF,
    };

    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = NEC_PAYLOAD_ZERO_HIGH_US,
            .level1 = 0,
            .duration1 = NEC_PAYLOAD_ZERO_LOW_US,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = NEC_PAYLOAD_ONE_HIGH_US,
            .level1 = 0,
            .duration1 = NEC_PAYLOAD_ONE_LOW_US,
        },
        .flags.msb_first = 1,
    };

    ret = rmt_new_bytes_encoder(&bytes_encoder_config, &samsung_encoder->bytes_encoder);
    if (ret != ESP_OK) {
        goto err;
    }

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ret = rmt_new_copy_encoder(&copy_encoder_config, &samsung_encoder->copy_encoder);
    if (ret != ESP_OK) {
        goto err;
    }

    *ret_encoder = &samsung_encoder->base;
    return ESP_OK;

err:
    if (samsung_encoder) {
        if (samsung_encoder->bytes_encoder) {
            rmt_del_encoder(samsung_encoder->bytes_encoder);
        }
        if (samsung_encoder->copy_encoder) {
            rmt_del_encoder(samsung_encoder->copy_encoder);
        }
        free(samsung_encoder);
    }
    return ret;
}

/* ============================================================================
 * RX CALLBACK (IRAM)
 * ============================================================================ */

static bool IRAM_ATTR rmt_rx_done_callback(rmt_channel_handle_t channel,
                                           const rmt_rx_done_event_data_t *edata,
                                           void *user_ctx)
{
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_ctx;
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

/* ============================================================================
 * PUBLIC API - INITIALIZATION
 * ============================================================================ */

esp_err_t ir_control_init(void)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Initializing IR control (TX: GPIO%d, RX: GPIO%d)", IR_TX_GPIO, IR_RX_GPIO);

    // Create mutex
    codes_mutex = xSemaphoreCreateMutex();
    if (codes_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize learned codes
    memset(learned_codes, 0, sizeof(learned_codes));

    // Create receive queue
    receive_queue = xQueueCreate(10, sizeof(rmt_rx_done_event_data_t));
    if (receive_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create receive queue");
        return ESP_ERR_NO_MEM;
    }

    // Configure TX channel
    rmt_tx_channel_config_t tx_config = {
        .gpio_num = IR_TX_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_TICK_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
        .flags.with_dma = false,
    };

    ret = rmt_new_tx_channel(&tx_config, &tx_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create TX channel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure RX channel
    rmt_rx_channel_config_t rx_config = {
        .gpio_num = IR_RX_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_TICK_RESOLUTION_HZ,
        .mem_block_symbols = 128,
        .intr_priority = 0,
        .flags.invert_in = true,   // IR receivers are active-LOW
        .flags.io_loop_back = false,
        .flags.with_dma = false,
    };

    ret = rmt_new_rx_channel(&rx_config, &rx_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RX channel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Create encoders
    ret = rmt_new_nec_encoder((const rmt_encoder_t **)&nec_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create NEC encoder: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = rmt_new_samsung_encoder((const rmt_encoder_t **)&samsung_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create Samsung encoder: %s", esp_err_to_name(ret));
        return ret;
    }

    rmt_copy_encoder_config_t copy_config = {};
    ret = rmt_new_copy_encoder(&copy_config, &copy_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create copy encoder: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register RX callback
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = rmt_rx_done_callback,
    };
    ret = rmt_rx_register_event_callbacks(rx_channel, &cbs, receive_queue);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register RX callback: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure carrier wave (38kHz)
    rmt_carrier_config_t carrier_cfg = {
        .frequency_hz = IR_CARRIER_FREQ_HZ,
        .duty_cycle = 0.33,
        .flags.polarity_active_low = false,
    };
    ret = rmt_apply_carrier(tx_channel, &carrier_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to apply carrier: %s", esp_err_to_name(ret));
        return ret;
    }

    // Enable channels
    ret = rmt_enable(tx_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable TX channel: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = rmt_enable(rx_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable RX channel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure receive parameters
    receive_config.signal_range_min_ns = 1250;
    receive_config.signal_range_max_ns = 10000000;
    receive_config.flags.en_partial_rx = false;

    // Start receiving
    ret = rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start receiving: %s", esp_err_to_name(ret));
        return ret;
    }

    // Create IR receive task
    BaseType_t task_ret = xTaskCreate(
        ir_receive_task,
        "ir_receive",
        8192,
        NULL,
        5,
        &rx_task_handle
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create IR receive task");
        return ESP_FAIL;
    }

    // Create learning timer (one-shot)
    const esp_timer_create_args_t learning_timer_args = {
        .callback = &learning_timeout_callback,
        .name = "ir_learn_timer"
    };
    ret = esp_timer_create(&learning_timer_args, &learning_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create learning timer: %s", esp_err_to_name(ret));
        return ret;
    }

    // Load saved codes from NVS
    ir_load_all_codes();

    ESP_LOGI(TAG, "IR control initialized successfully");
    return ESP_OK;
}

/* ============================================================================
 * PUBLIC API - LEARNING MODE
 * ============================================================================ */

esp_err_t ir_learn_start(ir_button_t button, uint32_t timeout_ms)
{
    if (button >= IR_BTN_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (timeout_ms == 0) {
        timeout_ms = IR_LEARN_TIMEOUT_MS;
    }

    ESP_LOGI(TAG, "Starting IR learn for button '%s' (timeout: %lu ms)",
             button_names[button], timeout_ms);

    learning_mode = true;
    current_learning_button = button;

    // Start timeout timer
    esp_timer_start_once(learning_timer, timeout_ms * 1000);

    return ESP_OK;
}

esp_err_t ir_learn_stop(void)
{
    if (!learning_mode) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping IR learn mode");

    // Stop timer
    esp_timer_stop(learning_timer);

    learning_mode = false;
    current_learning_button = IR_BTN_MAX;

    return ESP_OK;
}

bool ir_is_learning(void)
{
    return learning_mode;
}

/* ============================================================================
 * PUBLIC API - TRANSMISSION
 * ============================================================================ */

esp_err_t ir_transmit(ir_code_t *code)
{
    if (code == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // ========== MULTI-FREQUENCY CARRIER SUPPORT ==========
    const ir_protocol_constants_t *proto = ir_get_protocol_constants(code->protocol);
    uint32_t carrier_hz = proto ? (proto->carrier_khz * 1000) : 38000;

    rmt_carrier_config_t carrier_cfg = {
        .frequency_hz = carrier_hz,
        .duty_cycle = 0.33f,
        .flags.polarity_active_low = false,
    };

    esp_err_t ret = rmt_apply_carrier(tx_channel, &carrier_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set carrier to %lu Hz", carrier_hz);
        return ret;
    }

    ESP_LOGI(TAG, "Transmitting %s @ %lu Hz", ir_protocol_to_string(code->protocol), carrier_hz);

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };

    if (code->protocol == IR_PROTOCOL_NEC || code->protocol == IR_PROTOCOL_APPLE) {
        ret = rmt_transmit(tx_channel, nec_encoder, code, sizeof(ir_code_t), &tx_config);
        if (ret == ESP_OK) {
            ret = rmt_tx_wait_all_done(tx_channel, 1000);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Transmitted NEC/Apple code: 0x%08lX", code->data);
            } else {
                ESP_LOGE(TAG, "NEC transmission error: %s", esp_err_to_name(ret));
            }
        }
    } else if (code->protocol == IR_PROTOCOL_SAMSUNG || code->protocol == IR_PROTOCOL_SAMSUNG48) {
        ret = rmt_transmit(tx_channel, samsung_encoder, code, sizeof(ir_code_t), &tx_config);
        if (ret == ESP_OK) {
            ret = rmt_tx_wait_all_done(tx_channel, 1000);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Transmitted Samsung code: 0x%08lX", code->data);
            } else {
                ESP_LOGE(TAG, "Samsung transmission error: %s", esp_err_to_name(ret));
            }
        }
    } else if (code->protocol == IR_PROTOCOL_RAW && code->raw_data != NULL) {
        rmt_symbol_word_t *raw_symbols = (rmt_symbol_word_t *)code->raw_data;
        size_t num_symbols = code->raw_length;

        ESP_LOGI(TAG, "Transmitting RAW IR code (%d symbols)", num_symbols);

        ret = rmt_transmit(tx_channel, copy_encoder, raw_symbols,
                          num_symbols * sizeof(rmt_symbol_word_t), &tx_config);
        if (ret == ESP_OK) {
            ret = rmt_tx_wait_all_done(tx_channel, 1000);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Transmitted RAW code (%d symbols)", num_symbols);
            } else {
                ESP_LOGE(TAG, "RAW transmission error: %s", esp_err_to_name(ret));
            }
        }
    } else {
        // For all other decoded protocols, try NEC encoder (most compatible)
        ESP_LOGI(TAG, "Using NEC encoder for %s protocol", ir_protocol_to_string(code->protocol));
        ret = rmt_transmit(tx_channel, nec_encoder, code, sizeof(ir_code_t), &tx_config);
        if (ret == ESP_OK) {
            ret = rmt_tx_wait_all_done(tx_channel, 1000);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Transmitted %s code: 0x%08lX", ir_protocol_to_string(code->protocol), code->data);
            } else {
                ESP_LOGE(TAG, "Transmission error: %s", esp_err_to_name(ret));
            }
        }
    }

    return ret;
}

esp_err_t ir_transmit_button(ir_button_t button)
{
    if (button >= IR_BTN_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!ir_is_learned(button)) {
        ESP_LOGW(TAG, "Button '%s' not learned", button_names[button]);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Transmitting button '%s'", button_names[button]);

    xSemaphoreTake(codes_mutex, portMAX_DELAY);
    esp_err_t ret = ir_transmit(&learned_codes[button]);
    xSemaphoreGive(codes_mutex);

    return ret;
}

/* ============================================================================
 * PUBLIC API - NVS STORAGE
 * ============================================================================ */

esp_err_t ir_save_code(ir_button_t button, ir_code_t *code)
{
    if (button >= IR_BTN_MAX || code == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t ret;

    ret = nvs_open(IR_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    char key[20];
    snprintf(key, sizeof(key), "btn_%d", button);

    // Save basic ir_code_t (without raw_data pointer)
    ir_code_t code_to_save = *code;
    code_to_save.raw_data = NULL;

    ret = nvs_set_blob(nvs_handle, key, &code_to_save, sizeof(ir_code_t));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to save button %d metadata: %s", button, esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }

    // If RAW protocol, save raw_data separately
    if (code->protocol == IR_PROTOCOL_RAW && code->raw_data != NULL) {
        snprintf(key, sizeof(key), "raw_%d", button);
        size_t raw_size = code->raw_length * sizeof(rmt_symbol_word_t);

        ret = nvs_set_blob(nvs_handle, key, code->raw_data, raw_size);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to save button %d RAW data: %s", button, esp_err_to_name(ret));
        }
    }

    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "Saved code for button '%s'", button_names[button]);
    return ESP_OK;
}

esp_err_t ir_load_code(ir_button_t button, ir_code_t *code)
{
    if (button >= IR_BTN_MAX || code == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t ret;

    ret = nvs_open(IR_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return ESP_ERR_NOT_FOUND;
    }

    char key[20];
    size_t required_size = sizeof(ir_code_t);
    snprintf(key, sizeof(key), "btn_%d", button);

    ret = nvs_get_blob(nvs_handle, key, code, &required_size);
    if (ret != ESP_OK) {
        nvs_close(nvs_handle);
        return ESP_ERR_NOT_FOUND;
    }

    // If RAW protocol, load raw_data
    if (code->protocol == IR_PROTOCOL_RAW && code->raw_length > 0) {
        snprintf(key, sizeof(key), "raw_%d", button);

        size_t raw_size = 0;
        ret = nvs_get_blob(nvs_handle, key, NULL, &raw_size);
        if (ret != ESP_OK || raw_size == 0) {
            nvs_close(nvs_handle);
            return ESP_ERR_INVALID_STATE;
        }

        rmt_symbol_word_t *raw_data = (rmt_symbol_word_t *)malloc(raw_size);
        if (raw_data == NULL) {
            nvs_close(nvs_handle);
            return ESP_ERR_NO_MEM;
        }

        ret = nvs_get_blob(nvs_handle, key, raw_data, &raw_size);
        if (ret != ESP_OK) {
            free(raw_data);
            nvs_close(nvs_handle);
            return ESP_ERR_INVALID_STATE;
        }

        code->raw_data = (uint16_t *)raw_data;
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t ir_save_all_codes(void)
{
    xSemaphoreTake(codes_mutex, portMAX_DELAY);

    for (int i = 0; i < IR_BTN_MAX; i++) {
        if (learned_codes[i].protocol != IR_PROTOCOL_UNKNOWN) {
            ir_save_code((ir_button_t)i, &learned_codes[i]);
        }
    }

    xSemaphoreGive(codes_mutex);

    ESP_LOGI(TAG, "All IR codes saved");
    return ESP_OK;
}

esp_err_t ir_load_all_codes(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;

    ret = nvs_open(IR_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "No saved IR codes found");
        return ESP_OK;
    }

    xSemaphoreTake(codes_mutex, portMAX_DELAY);

    int loaded_count = 0;
    for (int i = 0; i < IR_BTN_MAX; i++) {
        char key[20];
        size_t required_size = sizeof(ir_code_t);
        snprintf(key, sizeof(key), "btn_%d", i);

        ret = nvs_get_blob(nvs_handle, key, &learned_codes[i], &required_size);
        if (ret != ESP_OK) {
            continue;
        }

        // Load RAW data if needed
        if (learned_codes[i].protocol == IR_PROTOCOL_RAW && learned_codes[i].raw_length > 0) {
            snprintf(key, sizeof(key), "raw_%d", i);

            size_t raw_size = 0;
            ret = nvs_get_blob(nvs_handle, key, NULL, &raw_size);
            if (ret != ESP_OK || raw_size == 0) {
                learned_codes[i].protocol = IR_PROTOCOL_UNKNOWN;
                continue;
            }

            rmt_symbol_word_t *raw_data = (rmt_symbol_word_t *)malloc(raw_size);
            if (raw_data == NULL) {
                learned_codes[i].protocol = IR_PROTOCOL_UNKNOWN;
                continue;
            }

            ret = nvs_get_blob(nvs_handle, key, raw_data, &raw_size);
            if (ret != ESP_OK) {
                free(raw_data);
                learned_codes[i].protocol = IR_PROTOCOL_UNKNOWN;
                continue;
            }

            learned_codes[i].raw_data = (uint16_t *)raw_data;
        }

        loaded_count++;
        ESP_LOGI(TAG, "Loaded %s code for '%s'",
                 protocol_names[learned_codes[i].protocol],
                 button_names[i]);
    }

    xSemaphoreGive(codes_mutex);
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "Loaded %d IR codes from NVS", loaded_count);
    return ESP_OK;
}

esp_err_t ir_clear_code(ir_button_t button)
{
    if (button >= IR_BTN_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(codes_mutex, portMAX_DELAY);

    // Free raw_data if allocated
    if (learned_codes[button].raw_data != NULL) {
        free(learned_codes[button].raw_data);
    }

    memset(&learned_codes[button], 0, sizeof(ir_code_t));

    xSemaphoreGive(codes_mutex);

    // Clear from NVS
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(IR_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        char key[20];
        snprintf(key, sizeof(key), "btn_%d", button);
        nvs_erase_key(nvs_handle, key);

        snprintf(key, sizeof(key), "raw_%d", button);
        nvs_erase_key(nvs_handle, key);

        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Cleared code for button '%s'", button_names[button]);
    return ESP_OK;
}

esp_err_t ir_clear_all_codes(void)
{
    xSemaphoreTake(codes_mutex, portMAX_DELAY);

    // Free all raw_data
    for (int i = 0; i < IR_BTN_MAX; i++) {
        if (learned_codes[i].raw_data != NULL) {
            free(learned_codes[i].raw_data);
        }
    }

    memset(learned_codes, 0, sizeof(learned_codes));

    xSemaphoreGive(codes_mutex);

    // Clear NVS
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(IR_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        nvs_erase_all(nvs_handle);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "All IR codes cleared");
    return ESP_OK;
}

/* ============================================================================
 * PUBLIC API - STATUS & QUERIES
 * ============================================================================ */

bool ir_is_learned(ir_button_t button)
{
    if (button >= IR_BTN_MAX) {
        return false;
    }

    xSemaphoreTake(codes_mutex, portMAX_DELAY);
    bool learned = (learned_codes[button].protocol != IR_PROTOCOL_UNKNOWN);
    xSemaphoreGive(codes_mutex);

    return learned;
}

const char* ir_get_button_name(ir_button_t button)
{
    if (button >= IR_BTN_MAX) {
        return "UNKNOWN";
    }
    return button_names[button];
}

const char* ir_get_protocol_name(ir_protocol_t protocol)
{
    if (protocol >= (sizeof(protocol_names) / sizeof(protocol_names[0]))) {
        return "INVALID";
    }
    return protocol_names[protocol];
}

/* ============================================================================
 * PUBLIC API - CALLBACK REGISTRATION
 * ============================================================================ */

esp_err_t ir_register_callbacks(const ir_callbacks_t *cbs)
{
    if (cbs == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    callbacks = *cbs;

    ESP_LOGI(TAG, "IR callbacks registered");
    return ESP_OK;
}
