/**
 * @file ir_timing.h
 * @brief IR Timing Matching Functions
 *
 * Provides timing comparison functions with percentage-based tolerance
 * for robust IR signal decoding across different manufacturers and conditions.
 *
 * Based on Arduino-IRremote library timing matching algorithms
 *
 * MIT License
 */

#ifndef IR_TIMING_H
#define IR_TIMING_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/rmt_rx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Default timing tolerance percentage
 *
 * Arduino-IRremote uses 25% tolerance which works well across most protocols
 * and accounts for remote control manufacturing variations.
 */
#define IR_TIMING_TOLERANCE_PERCENT  25

/**
 * @brief Check if measured timing matches expected value within tolerance
 *
 * Uses percentage-based tolerance rather than absolute microseconds.
 * Example: 25% tolerance on 1000us = accepts 750us to 1250us
 *
 * @param measured_us Measured timing in microseconds
 * @param expected_us Expected timing in microseconds
 * @param tolerance_percent Tolerance percentage (typically 25)
 * @return true if timing matches within tolerance
 */
bool ir_timing_matches_percent(uint16_t measured_us, uint16_t expected_us, uint8_t tolerance_percent);

/**
 * @brief Check if measured timing matches expected value (default 25% tolerance)
 *
 * Convenience wrapper using default IR_TIMING_TOLERANCE_PERCENT
 *
 * @param measured_us Measured timing in microseconds
 * @param expected_us Expected timing in microseconds
 * @return true if timing matches within default tolerance
 */
bool ir_timing_matches(uint16_t measured_us, uint16_t expected_us);

/**
 * @brief Check if RMT symbol's mark (level0) duration matches expected value
 *
 * For ESP32 RMT, level0 is typically the MARK (IR LED on)
 *
 * @param symbol Pointer to RMT symbol word
 * @param expected_us Expected mark duration in microseconds
 * @param tolerance_percent Tolerance percentage (0 = use default 25%)
 * @return true if mark duration matches
 */
bool ir_match_mark(const rmt_symbol_word_t *symbol, uint16_t expected_us, uint8_t tolerance_percent);

/**
 * @brief Check if RMT symbol's space (level1) duration matches expected value
 *
 * For ESP32 RMT, level1 is typically the SPACE (IR LED off)
 *
 * @param symbol Pointer to RMT symbol word
 * @param expected_us Expected space duration in microseconds
 * @param tolerance_percent Tolerance percentage (0 = use default 25%)
 * @return true if space duration matches
 */
bool ir_match_space(const rmt_symbol_word_t *symbol, uint16_t expected_us, uint8_t tolerance_percent);

/**
 * @brief Extract mark duration from RMT symbol (in microseconds)
 *
 * @param symbol Pointer to RMT symbol word
 * @return Mark duration in microseconds
 */
static inline uint16_t ir_get_mark_us(const rmt_symbol_word_t *symbol) {
    return symbol->duration0;
}

/**
 * @brief Extract space duration from RMT symbol (in microseconds)
 *
 * @param symbol Pointer to RMT symbol word
 * @return Space duration in microseconds
 */
static inline uint16_t ir_get_space_us(const rmt_symbol_word_t *symbol) {
    return symbol->duration1;
}

#ifdef __cplusplus
}
#endif

#endif /* IR_TIMING_H */
