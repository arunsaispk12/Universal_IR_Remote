/**
 * @file ir_daikin.h
 * @brief Daikin AC Protocol Decoder
 *
 * Daikin is one of the most complex IR protocols for air conditioners
 * - Market leader in Asia and Australia
 * - Multi-frame transmission (2-3 frames)
 * - 312 bits total (39 bytes) in main protocol variant
 * - Multiple checksums
 * - Based on Arduino-IRremote library, MIT License
 *
 * Protocol Structure (Daikin 1):
 * - Frame 1: 8 bytes (leader frame)
 * - Gap: 29ms
 * - Frame 2: 19 bytes (main data frame)
 * - Header: 3650us mark + 1623us space
 * - Bit encoding: Pulse distance (428us mark, 428us/1280us space)
 * - Multiple checksums (per frame)
 *
 * Reference: https://github.com/Arduino-IRremote/Arduino-IRremote
 */

#ifndef IR_DAIKIN_H
#define IR_DAIKIN_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// Daikin timing constants
#define DAIKIN_HEADER_MARK        3650
#define DAIKIN_HEADER_SPACE       1623
#define DAIKIN_BIT_MARK           428
#define DAIKIN_ONE_SPACE          1280
#define DAIKIN_ZERO_SPACE         428
#define DAIKIN_GAP                29000  // Inter-frame gap

// Daikin frame sizes (Daikin 1 protocol)
#define DAIKIN_FRAME1_BYTES       8
#define DAIKIN_FRAME2_BYTES       19
#define DAIKIN_TOTAL_BYTES        (DAIKIN_FRAME1_BYTES + DAIKIN_FRAME2_BYTES)
#define DAIKIN_TOTAL_BITS         (DAIKIN_TOTAL_BYTES * 8)

/**
 * @brief Decode Daikin AC IR protocol
 *
 * Handles multi-frame Daikin protocol with gap detection and checksum validation
 *
 * @param symbols RMT symbol array
 * @param num_symbols Number of symbols
 * @param code Output IR code structure
 * @return ESP_OK on success, ESP_FAIL on decode failure, ESP_ERR_INVALID_ARG on invalid input
 */
esp_err_t ir_decode_daikin(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif // IR_DAIKIN_H
