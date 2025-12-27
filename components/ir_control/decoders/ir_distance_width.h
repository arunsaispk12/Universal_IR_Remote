/**
 * @file ir_distance_width.h
 * @brief Universal Pulse Distance/Width Protocol Decoder
 *
 * Implements generic decoder for pulse distance and pulse width protocols.
 * Can decode unknown protocols by analyzing mark/space timing patterns.
 *
 * Algorithm:
 * 1. Build histograms of mark and space durations
 * 2. Aggregate to find short and long duration bins (max 2 per type)
 * 3. Classify as pulse distance, pulse width, or combined
 * 4. Decode bits based on discovered timing
 *
 * Based on Arduino-IRremote library ir_DistanceWidthProtocol.hpp
 * https://github.com/Arduino-IRremote/Arduino-IRremote
 *
 * MIT License - Copyright (c) 2022-2025 Armin Joachimsmeyer
 */

#ifndef IR_DISTANCE_WIDTH_H
#define IR_DISTANCE_WIDTH_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum duration array size for histogram analysis
 *
 * ESP32 RMT provides 1us resolution, so we use microseconds directly
 * instead of Arduino's 50us ticks. We limit to 10ms (10000us) max duration.
 *
 * Array size of 200 means we bin durations in 50us increments (10000/50=200)
 * to match Arduino's approach while using 1us precision for matching.
 */
#define IR_DW_DURATION_ARRAY_SIZE     200  // Covers 0-10000us in 50us bins
#define IR_DW_DURATION_BIN_SIZE_US    50   // Each bin represents 50us

/**
 * @brief Minimum number of bits required for valid protocol
 *
 * Only protocols with at least 7 data bits (+ header + stop) are accepted
 * This filters out noise and incomplete frames.
 */
#define IR_DW_MIN_BITS                7

/**
 * @brief Maximum gap for repeat frame detection (microseconds)
 */
#define IR_DW_MAX_REPEAT_GAP_US       100000  // 100ms

/**
 * @brief Decode pulse distance or pulse width protocol from RMT symbols
 *
 * This universal decoder analyzes the timing characteristics of an IR signal
 * and attempts to decode it without prior knowledge of the specific protocol.
 *
 * @param symbols Pointer to RMT symbol array
 * @param num_symbols Number of symbols in array
 * @param code Output structure to fill with decoded data
 * @return ESP_OK if successfully decoded
 *         ESP_ERR_INVALID_ARG if too few symbols or invalid data
 *         ESP_FAIL if protocol cannot be classified or decoded
 */
esp_err_t ir_decode_distance_width(const rmt_symbol_word_t *symbols,
                                    size_t num_symbols,
                                    ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif /* IR_DISTANCE_WIDTH_H */
