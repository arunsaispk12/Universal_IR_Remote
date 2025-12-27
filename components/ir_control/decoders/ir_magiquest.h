/**
 * @file ir_magiquest.h
 * @brief MagiQuest Protocol Decoder (theme park wands)
 * Based on Arduino-IRremote library, MIT License
 */

#ifndef IR_MAGIQUEST_H
#define IR_MAGIQUEST_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAGIQUEST_BIT_MARK           288
#define MAGIQUEST_ONE_SPACE          864
#define MAGIQUEST_ZERO_SPACE         576
#define MAGIQUEST_BITS               56

esp_err_t ir_decode_magiquest(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif
