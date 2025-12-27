/**
 * @file ir_denon.h
 * @brief Denon/Sharp Protocol Decoder
 *
 * Denon and Sharp use the same protocol.
 * 15 bits with parity check.
 *
 * Based on Arduino-IRremote library
 * MIT License
 */

#ifndef IR_DENON_H
#define IR_DENON_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DENON_HEADER_MARK    275
#define DENON_HEADER_SPACE   775
#define DENON_BIT_MARK       275
#define DENON_ONE_SPACE      1900
#define DENON_ZERO_SPACE     775
#define DENON_BITS           15

esp_err_t ir_decode_denon(const rmt_symbol_word_t *symbols,
                           size_t num_symbols,
                           ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif /* IR_DENON_H */
