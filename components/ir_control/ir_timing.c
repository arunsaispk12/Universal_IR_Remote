/**
 * @file ir_timing.c
 * @brief IR Timing Matching Functions Implementation
 *
 * Provides robust timing comparison for IR signal decoding.
 * Uses percentage-based tolerance to handle manufacturing variations.
 *
 * Based on Arduino-IRremote library
 *
 * MIT License
 */

#include "ir_timing.h"
#include "esp_log.h"

static const char *TAG = "IR_TIMING";

bool ir_timing_matches_percent(uint16_t measured_us, uint16_t expected_us, uint8_t tolerance_percent) {
    // Calculate tolerance in microseconds
    // Example: 25% of 1000us = 250us, so range is 750-1250us
    uint16_t tolerance_us = (expected_us * tolerance_percent) / 100;

    // Check if measured value is within tolerance range
    uint16_t lower_bound = expected_us - tolerance_us;
    uint16_t upper_bound = expected_us + tolerance_us;

    bool matches = (measured_us >= lower_bound) && (measured_us <= upper_bound);

#if 0  // Enable for detailed timing debugging
    if (!matches) {
        ESP_LOGD(TAG, "Timing mismatch: measured=%u, expected=%u±%u%% (%u-%u)",
                 measured_us, expected_us, tolerance_percent, lower_bound, upper_bound);
    }
#endif

    return matches;
}

bool ir_timing_matches(uint16_t measured_us, uint16_t expected_us) {
    return ir_timing_matches_percent(measured_us, expected_us, IR_TIMING_TOLERANCE_PERCENT);
}

bool ir_match_mark(const rmt_symbol_word_t *symbol, uint16_t expected_us, uint8_t tolerance_percent) {
    if (symbol == NULL) {
        return false;
    }

    // Use default tolerance if 0 specified
    if (tolerance_percent == 0) {
        tolerance_percent = IR_TIMING_TOLERANCE_PERCENT;
    }

    uint16_t measured_us = symbol->duration0;  // Mark is level0 (IR LED on)
    return ir_timing_matches_percent(measured_us, expected_us, tolerance_percent);
}

bool ir_match_space(const rmt_symbol_word_t *symbol, uint16_t expected_us, uint8_t tolerance_percent) {
    if (symbol == NULL) {
        return false;
    }

    // Use default tolerance if 0 specified
    if (tolerance_percent == 0) {
        tolerance_percent = IR_TIMING_TOLERANCE_PERCENT;
    }

    uint16_t measured_us = symbol->duration1;  // Space is level1 (IR LED off)
    return ir_timing_matches_percent(measured_us, expected_us, tolerance_percent);
}
