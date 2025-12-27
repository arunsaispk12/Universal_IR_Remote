/**
 * @file ir_whynter.h
 * @brief Whynter Protocol Decoder (portable AC units)
 * Based on Arduino-IRremote library, MIT License
 */

#ifndef IR_WHYNTER_H
#define IR_WHYNTER_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WHYNTER_HEADER_MARK      2850
#define WHYNTER_HEADER_SPACE     2850
#define WHYNTER_BIT_MARK         750
#define WHYNTER_ONE_SPACE        750
#define WHYNTER_ZERO_SPACE       750
#define WHYNTER_BITS             32

esp_err_t ir_decode_whynter(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif
