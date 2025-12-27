/**
 * @file ir_carrier.h
 * @brief Carrier AC Protocol Decoder
 *
 * Carrier air conditioner protocol - CRITICAL for Indian market
 * - Used by: Voltas (#1 AC brand in India), Blue Star, Lloyd
 * - Complex state-based protocol
 * - Variable length frames
 * - Multiple checksums
 * - Based on Arduino-IRremote library, MIT License
 *
 * Protocol Structure:
 * - Header: 8820us mark + 4410us space
 * - Bit encoding: Pulse distance (420us mark, 420us/1260us space)
 * - Data: Variable length (typically 128 bits / 16 bytes)
 * - Checksums: Multiple checksums per frame
 *
 * Note: Carrier protocol has many variants. This implements the most
 * common variant used by Voltas, Blue Star, and Lloyd in India.
 *
 * Reference: https://github.com/Arduino-IRremote/Arduino-IRremote
 */

#ifndef IR_CARRIER_H
#define IR_CARRIER_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// Carrier AC timing constants
#define CARRIER_HEADER_MARK       8820
#define CARRIER_HEADER_SPACE      4410
#define CARRIER_BIT_MARK          420
#define CARRIER_ONE_SPACE         1260
#define CARRIER_ZERO_SPACE        420
#define CARRIER_BITS              128    // 16 bytes (common variant)
#define CARRIER_BYTES             16

/**
 * @brief Decode Carrier AC IR protocol
 *
 * Handles Carrier AC protocol used by Voltas, Blue Star, and Lloyd
 * with checksum validation
 *
 * @param symbols RMT symbol array
 * @param num_symbols Number of symbols
 * @param code Output IR code structure
 * @return ESP_OK on success, ESP_FAIL on decode failure, ESP_ERR_INVALID_ARG on invalid input
 */
esp_err_t ir_decode_carrier(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif // IR_CARRIER_H
