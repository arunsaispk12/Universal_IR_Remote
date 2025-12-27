/**
 * @file ir_midea.h
 * @brief Midea AC Protocol Decoder
 *
 * Midea air conditioners (also used by: Toshiba, Carrier, Electrolux, many OEM brands)
 * - Very popular budget AC segment globally
 * - 48 bits (6 bytes) typical
 * - Inverted bit encoding
 * - Based on Arduino-IRremote library, MIT License
 *
 * Protocol Structure:
 * - Header: 4500us mark + 4500us space
 * - Bit encoding: Pulse distance (560us mark, 560us/1680us space)
 * - Data: 6 bytes (3 bytes data + 3 bytes inverted)
 * - Validation: Inverted bytes must match
 *
 * Note: Midea protocol is used by many brands under OEM agreements
 *
 * Reference: https://github.com/Arduino-IRremote/Arduino-IRremote
 */

#ifndef IR_MIDEA_H
#define IR_MIDEA_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// Midea timing constants
#define MIDEA_HEADER_MARK        4500
#define MIDEA_HEADER_SPACE       4500
#define MIDEA_BIT_MARK           560
#define MIDEA_ONE_SPACE          1680
#define MIDEA_ZERO_SPACE         560
#define MIDEA_BITS               48
#define MIDEA_BYTES              6

/**
 * @brief Decode Midea AC IR protocol
 *
 * Validates inverted byte pairs for data integrity
 *
 * @param symbols RMT symbol array
 * @param num_symbols Number of symbols
 * @param code Output IR code structure
 * @return ESP_OK on success, ESP_FAIL on decode failure, ESP_ERR_INVALID_ARG on invalid input
 */
esp_err_t ir_decode_midea(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif // IR_MIDEA_H
