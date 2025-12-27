/**
 * @file ir_lg.h
 * @brief LG Protocol Decoder
 *
 * LG protocol decoder for TVs and air conditioners.
 * 28 bits with 4-bit checksum validation.
 *
 * Protocol characteristics:
 * - Carrier: 38kHz
 * - Encoding: Pulse distance (LSB first)
 * - Header: 9000us mark + 4500us space
 * - Data: 28 bits (8 address + 16 command + 4 checksum)
 * - Checksum: Sum of nibbles modulo 16
 *
 * Based on Arduino-IRremote library
 * MIT License
 */

#ifndef IR_LG_H
#define IR_LG_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LG_HEADER_MARK       9000
#define LG_HEADER_SPACE      4500
#define LG_BIT_MARK          560
#define LG_ONE_SPACE         1690
#define LG_ZERO_SPACE        560
#define LG_BITS              28

esp_err_t ir_decode_lg(const rmt_symbol_word_t *symbols,
                        size_t num_symbols,
                        ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif /* IR_LG_H */
