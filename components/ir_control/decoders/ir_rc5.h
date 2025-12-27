/**
 * @file ir_rc5.h
 * @brief Philips RC5 Protocol Decoder
 *
 * RC5 is a bi-phase (Manchester) encoded protocol developed by Philips
 * - Very popular in India for Philips TVs and many STBs
 * - Uses bi-phase encoding (not pulse distance/width)
 * - Toggle bit for repeat detection
 * - 14 bits total (command, address, toggle)
 * - 36kHz carrier (not 38kHz!)
 * - Based on Arduino-IRremote library, MIT License
 *
 * Protocol Structure:
 * - Start bits: 2 bits (always 1,1 except for extended RC5)
 * - Toggle bit: 1 bit (inverts each button press)
 * - Address: 5 bits (device type)
 * - Command: 6 bits (button/function)
 * - Total: 14 bits
 * - Bi-phase encoding: Each bit is half mark + half space (or vice versa)
 *
 * Timing:
 * - Bit period: 1778us (889us mark + 889us space for "1")
 * - Carrier: 36kHz
 *
 * Reference: https://github.com/Arduino-IRremote/Arduino-IRremote
 */

#ifndef IR_RC5_H
#define IR_RC5_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// RC5 timing constants
#define RC5_UNIT                  889     // Base timing unit in microseconds
#define RC5_HEADER_MARK           (RC5_UNIT * 1)
#define RC5_HEADER_SPACE          (RC5_UNIT * 1)
#define RC5_BIT_MARK              (RC5_UNIT * 1)
#define RC5_BIT_SPACE             (RC5_UNIT * 1)
#define RC5_BITS                  14      // Total bits including start + toggle

/**
 * @brief Decode Philips RC5 IR protocol
 *
 * Handles bi-phase (Manchester) encoding with toggle bit detection
 *
 * @param symbols RMT symbol array
 * @param num_symbols Number of symbols
 * @param code Output IR code structure
 * @return ESP_OK on success, ESP_FAIL on decode failure, ESP_ERR_INVALID_ARG on invalid input
 */
esp_err_t ir_decode_rc5(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif // IR_RC5_H
