/**
 * @file ir_lego.h
 * @brief Lego Power Functions Protocol Decoder
 * Based on Arduino-IRremote library, MIT License
 */

#ifndef IR_LEGO_H
#define IR_LEGO_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LEGO_HEADER_MARK         158
#define LEGO_HEADER_SPACE        1026
#define LEGO_BIT_MARK            158
#define LEGO_ONE_SPACE           553
#define LEGO_ZERO_SPACE          263
#define LEGO_BITS                16

esp_err_t ir_decode_lego(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif
