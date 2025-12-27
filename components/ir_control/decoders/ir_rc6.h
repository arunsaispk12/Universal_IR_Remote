/**
 * @file ir_rc6.h
 * @brief Philips RC6 Protocol Decoder
 *
 * RC6 is an improved bi-phase protocol by Philips, used in modern devices
 * - Used in Microsoft Media Center remotes, some modern TVs
 * - Bi-phase (Manchester) encoding like RC5 but more complex
 * - Has a "trailer bit" with double length for mode detection
 * - Toggle bit for repeat detection
 * - 36kHz carrier
 * - Based on Arduino-IRremote library, MIT License
 *
 * Protocol Structure (Mode 0 - most common):
 * - Leader: Long mark + space
 * - Start bit: 1 bit (always 1)
 * - Mode: 3 bits (000 for mode 0)
 * - Toggle bit: 1 bit (double-length trailer bit)
 * - Address: 8 bits
 * - Command: 8 bits
 * - Total: 21 bits (including leader and mode)
 *
 * Timing:
 * - Base unit: 444us
 * - Leader mark: 2666us (6 * 444us)
 * - Leader space: 889us (2 * 444us)
 * - Bit period: 889us (2 * 444us)
 * - Trailer bit: 1778us (4 * 444us) - double length!
 * - Carrier: 36kHz
 *
 * Reference: https://github.com/Arduino-IRremote/Arduino-IRremote
 */

#ifndef IR_RC6_H
#define IR_RC6_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// RC6 timing constants
#define RC6_UNIT                  444     // Base timing unit in microseconds
#define RC6_HEADER_MARK           (RC6_UNIT * 6)   // 2666us
#define RC6_HEADER_SPACE          (RC6_UNIT * 2)   // 889us
#define RC6_BIT_MARK              (RC6_UNIT * 1)   // 444us
#define RC6_BIT_SPACE             (RC6_UNIT * 1)   // 444us
#define RC6_TOGGLE_MARK           (RC6_UNIT * 2)   // 889us (double length)
#define RC6_BITS                  21      // Approximate (variable with mode)

/**
 * @brief Decode Philips RC6 IR protocol
 *
 * Handles bi-phase encoding with special trailer bit and mode detection
 *
 * @param symbols RMT symbol array
 * @param num_symbols Number of symbols
 * @param code Output IR code structure
 * @return ESP_OK on success, ESP_FAIL on decode failure, ESP_ERR_INVALID_ARG on invalid input
 */
esp_err_t ir_decode_rc6(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif // IR_RC6_H
