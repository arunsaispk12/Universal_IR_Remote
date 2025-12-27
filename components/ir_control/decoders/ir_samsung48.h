/**
 * @file ir_samsung48.h
 * @brief Samsung 48-bit Protocol Decoder (for AC units)
 * Based on Arduino-IRremote library, MIT License
 */

#ifndef IR_SAMSUNG48_H
#define IR_SAMSUNG48_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SAMSUNG48_HEADER_MARK    4500
#define SAMSUNG48_HEADER_SPACE   4500
#define SAMSUNG48_BIT_MARK       560
#define SAMSUNG48_ONE_SPACE      1690
#define SAMSUNG48_ZERO_SPACE     560
#define SAMSUNG48_BITS           48

esp_err_t ir_decode_samsung48(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif
