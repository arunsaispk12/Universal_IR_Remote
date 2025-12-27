/**
 * @file ir_hitachi.h
 * @brief Hitachi AC Protocol Decoder
 *
 * Hitachi air conditioner protocol - Important for Indian market
 * - Popular AC brand in India (~10% market share)
 * - Complex state-based protocol
 * - 264 bits (33 bytes) or 344 bits (43 bytes) depending on model
 * - Checksum validation
 * - Based on Arduino-IRremote library, MIT License
 *
 * Protocol Structure:
 * - Header: 3300us mark + 1700us space
 * - Bit encoding: Pulse distance (370us mark, 370us/1260us space)
 * - Data: 33 or 43 bytes (model dependent)
 * - Checksum: Byte sum modulo 256
 *
 * Hitachi has multiple protocol variants. This implements the most
 * common variant used in Indian market models.
 *
 * Reference: https://github.com/Arduino-IRremote/Arduino-IRremote
 */

#ifndef IR_HITACHI_H
#define IR_HITACHI_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// Hitachi AC timing constants
#define HITACHI_HEADER_MARK       3300
#define HITACHI_HEADER_SPACE      1700
#define HITACHI_BIT_MARK          370
#define HITACHI_ONE_SPACE         1260
#define HITACHI_ZERO_SPACE        370

// Hitachi frame sizes (variable)
#define HITACHI_MIN_BITS          264    // 33 bytes (common variant)
#define HITACHI_MAX_BITS          344    // 43 bytes (extended variant)
#define HITACHI_MIN_BYTES         33
#define HITACHI_MAX_BYTES         43

/**
 * @brief Decode Hitachi AC IR protocol
 *
 * Handles variable-length Hitachi AC protocol with checksum validation
 *
 * @param symbols RMT symbol array
 * @param num_symbols Number of symbols
 * @param code Output IR code structure
 * @return ESP_OK on success, ESP_FAIL on decode failure, ESP_ERR_INVALID_ARG on invalid input
 */
esp_err_t ir_decode_hitachi(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif // IR_HITACHI_H
