/**
 * @file ir_distance_width.c
 * @brief Universal Pulse Distance/Width Protocol Decoder Implementation
 *
 * Ported from Arduino-IRremote library to ESP-IDF with C language.
 * Uses ESP32 RMT peripheral instead of Arduino's interrupt-based timing.
 *
 * Based on Arduino-IRremote library
 * MIT License - Copyright (c) 2022-2025 Armin Joachimsmeyer
 */

#include "ir_distance_width.h"
#include "ir_timing.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "IR_DW";

/**
 * @brief Aggregate histogram counts into short and long duration bins
 *
 * Analyzes histogram array and groups consecutive counts (allowing 1-gap)
 * into average duration bins. Returns short and long bin indexes.
 *
 * This is the core algorithm from Arduino-IRremote that allows robust
 * decoding despite manufacturing variations in remote controls.
 *
 * @param array Histogram array (modified in-place)
 * @param max_index Maximum index to scan
 * @param short_index Output: index of short duration bin
 * @param long_index Output: index of long duration bin
 * @return true if successfully aggregated (max 2 bins found)
 *         false if more than 2 distinct durations found
 */
static bool aggregate_array_counts(uint8_t *array, uint8_t max_index,
                                    uint8_t *short_index, uint8_t *long_index) {
    uint8_t sum = 0;
    uint16_t weighted_sum = 0;
    uint8_t gap_count = 0;

    *short_index = 0;
    *long_index = 0;

    for (uint_fast8_t i = 0; i <= max_index; i++) {
        uint8_t current_count = array[i];

        if (current_count != 0) {
            // Add to sum and clear array entry
            sum += current_count;
            weighted_sum += (current_count * i);
            array[i] = 0;
            gap_count = 0;
        } else {
            gap_count++;
        }

        // Aggregate when we have a sum AND (reached end OR gap > 1)
        if (sum != 0 && (i == max_index || gap_count > 1)) {
            // Calculate weighted average with rounding
            uint8_t aggregate_index = (weighted_sum + (sum / 2)) / sum;
            array[aggregate_index] = sum;  // Store for reference

            // Assign to short or long bin
            if (*short_index == 0) {
                *short_index = aggregate_index;
            } else if (*long_index == 0) {
                *long_index = aggregate_index;
            } else {
                // Found 3 bins - this is likely not pulse distance/width protocol
                // (could be RC5/RC6 bi-phase or other complex encoding)
                ESP_LOGD(TAG, "Aggregation found 3+ bins - not pulse distance/width");
                return false;
            }

            // Reset for next aggregation
            sum = 0;
            weighted_sum = 0;
        }
    }

    return true;
}

/**
 * @brief Decode bits from RMT symbols using pulse distance or width encoding
 *
 * @param symbols RMT symbol array
 * @param start_index Symbol index to start decoding (skip header)
 * @param num_bits Number of bits to decode
 * @param long_duration_us Threshold for "1" bit (marks for pulse width, spaces for pulse distance)
 * @param is_pulse_width true for pulse width, false for pulse distance
 * @param is_msb_first true for MSB first, false for LSB first
 * @param decoded_data Output: decoded data value
 * @return ESP_OK on success, ESP_FAIL on error
 */
static esp_err_t decode_bits(const rmt_symbol_word_t *symbols,
                              size_t start_index,
                              uint8_t num_bits,
                              uint16_t long_duration_us,
                              bool is_pulse_width,
                              bool is_msb_first,
                              uint32_t *decoded_data) {
    uint32_t data = 0;

    for (uint_fast8_t bit = 0; bit < num_bits; bit++) {
        size_t symbol_index = start_index + bit;
        const rmt_symbol_word_t *sym = &symbols[symbol_index];

        bool bit_value;

        if (is_pulse_width) {
            // Pulse WIDTH: bit determined by mark duration
            uint16_t mark_us = ir_get_mark_us(sym);
            bit_value = (mark_us >= long_duration_us);
        } else {
            // Pulse DISTANCE: bit determined by space duration
            uint16_t space_us = ir_get_space_us(sym);
            bit_value = (space_us >= long_duration_us);
        }

        // Pack bit into data word
        if (is_msb_first) {
            data = (data << 1) | (bit_value ? 1 : 0);
        } else {
            // LSB first
            if (bit_value) {
                data |= (1UL << bit);
            }
        }
    }

    *decoded_data = data;
    return ESP_OK;
}

esp_err_t ir_decode_distance_width(const rmt_symbol_word_t *symbols,
                                    size_t num_symbols,
                                    ir_code_t *code) {
    if (symbols == NULL || code == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Require at least 7 data bits + header (2 symbols) + stop bit (1 symbol) = 18+ symbols minimum
    // Each bit = mark + space = 1 RMT symbol
    size_t min_symbols = (2 * IR_DW_MIN_BITS) + 4;
    if (num_symbols < min_symbols) {
        ESP_LOGD(TAG, "Too few symbols: %zu < %zu", num_symbols, min_symbols);
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize histogram arrays for mark and space durations
    uint8_t mark_histogram[IR_DW_DURATION_ARRAY_SIZE] = {0};
    uint8_t space_histogram[IR_DW_DURATION_ARRAY_SIZE] = {0};

    uint8_t mark_max_index = 0;
    uint8_t space_max_index = 0;

    /*
     * STEP 1: Build histograms of mark and space durations
     * Skip header (symbols 0-1) and stop bit (last symbol)
     */

    // Count MARK durations (level0) - skip header and stop bit
    for (size_t i = 1; i < num_symbols - 1; i++) {
        uint16_t mark_us = symbols[i].duration0;

        // Convert to bin index (50us bins)
        uint8_t bin_index = mark_us / IR_DW_DURATION_BIN_SIZE_US;

        if (bin_index < IR_DW_DURATION_ARRAY_SIZE) {
            mark_histogram[bin_index]++;
            if (bin_index > mark_max_index) {
                mark_max_index = bin_index;
            }
        } else {
            ESP_LOGD(TAG, "Mark %u us exceeds max %u us at symbol %zu",
                     mark_us, IR_DW_DURATION_ARRAY_SIZE * IR_DW_DURATION_BIN_SIZE_US, i);
            return ESP_FAIL;
        }
    }

    // Count SPACE durations (level1) - skip header and stop bit
    for (size_t i = 1; i < num_symbols - 1; i++) {
        uint16_t space_us = symbols[i].duration1;

        // Convert to bin index
        uint8_t bin_index = space_us / IR_DW_DURATION_BIN_SIZE_US;

        if (bin_index < IR_DW_DURATION_ARRAY_SIZE) {
            space_histogram[bin_index]++;
            if (bin_index > space_max_index) {
                space_max_index = bin_index;
            }
        } else {
            ESP_LOGD(TAG, "Space %u us exceeds max %u us at symbol %zu",
                     space_us, IR_DW_DURATION_ARRAY_SIZE * IR_DW_DURATION_BIN_SIZE_US, i);
            return ESP_FAIL;
        }
    }

    /*
     * STEP 2: Aggregate histograms to find short and long durations
     */

    uint8_t mark_short_idx = 0, mark_long_idx = 0;
    if (!aggregate_array_counts(mark_histogram, mark_max_index, &mark_short_idx, &mark_long_idx)) {
        ESP_LOGD(TAG, "Mark aggregation failed (3+ distinct mark durations)");
        return ESP_FAIL;
    }

    uint8_t space_short_idx = 0, space_long_idx = 0;
    if (!aggregate_array_counts(space_histogram, space_max_index, &space_short_idx, &space_long_idx)) {
        ESP_LOGD(TAG, "Space aggregation failed (3+ distinct space durations)");
        return ESP_FAIL;
    }

    // Convert bin indexes back to microseconds
    uint16_t mark_short_us = mark_short_idx * IR_DW_DURATION_BIN_SIZE_US;
    uint16_t mark_long_us = mark_long_idx * IR_DW_DURATION_BIN_SIZE_US;
    uint16_t space_short_us = space_short_idx * IR_DW_DURATION_BIN_SIZE_US;
    uint16_t space_long_us = space_long_idx * IR_DW_DURATION_BIN_SIZE_US;

    ESP_LOGI(TAG, "Timing: mark=%u/%uus, space=%u/%uus",
             mark_short_us, mark_long_us, space_short_us, space_long_us);

    /*
     * STEP 3: Classify protocol type
     *
     * PULSE_DISTANCE: marks constant, spaces vary (long space = "1")
     * PULSE_WIDTH: spaces constant, marks vary (long mark = "1")
     * PULSE_DISTANCE_WIDTH: both marks and spaces vary
     */

    // Check if we have enough timing variation to decode
    if (mark_long_idx == 0 && space_long_idx == 0) {
        ESP_LOGD(TAG, "Cannot decode: only one duration for both mark and space");
        return ESP_FAIL;
    }

    bool is_pulse_width = (mark_long_idx != 0 && space_short_idx == space_long_idx);
    bool is_pulse_distance = (mark_short_idx == mark_long_idx && space_long_idx != 0);

    // PULSE_DISTANCE_WIDTH can be decoded as pulse distance
    if (!is_pulse_width && !is_pulse_distance && space_long_idx != 0) {
        is_pulse_distance = true;  // Decode as pulse distance
        ESP_LOGD(TAG, "PULSE_DISTANCE_WIDTH detected, decoding as pulse distance");
    }

    /*
     * STEP 4: Decode bits
     */

    // Calculate number of data bits (exclude header symbol and stop bit)
    uint8_t num_bits = (num_symbols / 1) - 2;  // Each symbol is mark+space
    if (space_long_idx > 0) {
        num_bits--;  // Pulse distance has mandatory stop bit
    }

    if (num_bits == 0 || num_bits > 64) {
        ESP_LOGD(TAG, "Invalid bit count: %u", num_bits);
        return ESP_FAIL;
    }

    // Determine threshold for "1" bit
    uint16_t long_threshold_us;
    if (is_pulse_width) {
        // Pulse width: use midpoint between short and long marks
        long_threshold_us = (mark_short_us + mark_long_us) / 2;
    } else {
        // Pulse distance: use midpoint between short and long spaces
        long_threshold_us = (space_short_us + space_long_us) / 2;
    }

    uint32_t decoded_data = 0;
    esp_err_t ret = decode_bits(symbols,
                                 1,  // Start after header symbol
                                 num_bits,
                                 long_threshold_us,
                                 is_pulse_width,
                                 false,  // LSB first by default (can make configurable)
                                 &decoded_data);

    if (ret != ESP_OK) {
        return ret;
    }

    /*
     * STEP 5: Fill result structure
     */

    code->protocol = is_pulse_width ? IR_PROTOCOL_PULSE_WIDTH : IR_PROTOCOL_PULSE_DISTANCE;
    code->data = decoded_data;
    code->bits = num_bits;
    code->address = 0;  // Cannot extract without protocol knowledge
    code->command = 0;  // Cannot extract without protocol knowledge
    code->flags = 0;    // LSB first by default

    ESP_LOGI(TAG, "Decoded %s: %u bits, data=0x%08lX",
             is_pulse_width ? "PULSE_WIDTH" : "PULSE_DISTANCE",
             num_bits, decoded_data);

    ESP_LOGI(TAG, "Timing info: header=%u/%uus, 0=%u/%uus, 1=%u/%uus",
             (unsigned int)symbols[0].duration0, (unsigned int)symbols[0].duration1,
             mark_short_us, space_short_us,
             is_pulse_width ? mark_long_us : mark_short_us,
             is_pulse_width ? space_short_us : space_long_us);

    return ESP_OK;
}
