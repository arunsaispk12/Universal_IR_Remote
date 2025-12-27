/**
 * @file ir_apple.h
 * @brief Apple Protocol Decoder (Apple remotes - NEC variant)
 * Based on Arduino-IRremote library, MIT License
 */

#ifndef IR_APPLE_H
#define IR_APPLE_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// Apple uses NEC timing
#define APPLE_HEADER_MARK        9000
#define APPLE_HEADER_SPACE       4500
#define APPLE_BIT_MARK           560
#define APPLE_ONE_SPACE          1690
#define APPLE_ZERO_SPACE         560
#define APPLE_BITS               32
#define APPLE_ADDRESS            0x77E1  // Apple-specific address

esp_err_t ir_decode_apple(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif
