/**
 * @file ir_haier.h
 * @brief Haier AC Protocol Decoder
 *
 * Haier air conditioners (Chinese brand, global presence)
 * - Popular in China and emerging markets
 * - 104 bits (13 bytes) typical
 * - Checksum validation
 * - Based on Arduino-IRremote library, MIT License
 *
 * Protocol Structure:
 * - Header: 3000us mark + 3000us space
 * - Bit encoding: Pulse distance (520us mark, 650us/1650us space)
 * - Data: 13 bytes
 * - Checksum: XOR of all bytes
 *
 * Reference: https://github.com/Arduino-IRremote/Arduino-IRremote
 */

#ifndef IR_HAIER_H
#define IR_HAIER_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// Haier timing constants
#define HAIER_HEADER_MARK        3000
#define HAIER_HEADER_SPACE       3000
#define HAIER_BIT_MARK           520
#define HAIER_ONE_SPACE          1650
#define HAIER_ZERO_SPACE         650
#define HAIER_BITS               104
#define HAIER_BYTES              13

/**
 * @brief Decode Haier AC IR protocol
 *
 * @param symbols RMT symbol array
 * @param num_symbols Number of symbols
 * @param code Output IR code structure
 * @return ESP_OK on success, ESP_FAIL on decode failure, ESP_ERR_INVALID_ARG on invalid input
 */
esp_err_t ir_decode_haier(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif // IR_HAIER_H
