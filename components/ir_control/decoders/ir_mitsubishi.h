/**
 * @file ir_mitsubishi.h
 * @brief Mitsubishi Electric AC Protocol Decoder
 *
 * Mitsubishi Electric uses a 152-bit protocol for air conditioners
 * - Very popular globally (Asia, Europe, North America)
 * - 152 bits = 19 bytes of data
 * - Includes checksum validation
 * - Based on Arduino-IRremote library, MIT License
 *
 * Protocol Structure:
 * - Header: 3400us mark + 1750us space
 * - Bit encoding: Pulse distance (560us mark, 420us/1300us space)
 * - Data: 19 bytes (address, mode, temp, fan, swing, timer, etc.)
 * - Checksum: Byte sum of data
 *
 * Reference: https://github.com/Arduino-IRremote/Arduino-IRremote
 */

#ifndef IR_MITSUBISHI_H
#define IR_MITSUBISHI_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// Mitsubishi Electric AC timing constants
#define MITSUBISHI_HEADER_MARK        3400
#define MITSUBISHI_HEADER_SPACE       1750
#define MITSUBISHI_BIT_MARK           450
#define MITSUBISHI_ONE_SPACE          1300
#define MITSUBISHI_ZERO_SPACE         420
#define MITSUBISHI_BITS               152     // 19 bytes
#define MITSUBISHI_BYTES              19

/**
 * @brief Decode Mitsubishi Electric IR protocol
 *
 * @param symbols RMT symbol array
 * @param num_symbols Number of symbols
 * @param code Output IR code structure
 * @return ESP_OK on success, ESP_FAIL on decode failure, ESP_ERR_INVALID_ARG on invalid input
 */
esp_err_t ir_decode_mitsubishi(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif // IR_MITSUBISHI_H
