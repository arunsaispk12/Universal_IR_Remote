/**
 * @file ir_fast.h
 * @brief FAST Protocol Decoder (FAST brand remotes)
 * Based on Arduino-IRremote library, MIT License
 */

#ifndef IR_FAST_H
#define IR_FAST_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FAST_BIT_MARK            320
#define FAST_ONE_SPACE           640
#define FAST_ZERO_SPACE          320
#define FAST_BITS                8

esp_err_t ir_decode_fast(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif
