/**
 * @file ir_bosewave.h
 * @brief BoseWave Protocol Decoder (Bose Wave radios)
 * Based on Arduino-IRremote library, MIT License
 */

#ifndef IR_BOSEWAVE_H
#define IR_BOSEWAVE_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BOSEWAVE_HEADER_MARK     1014
#define BOSEWAVE_HEADER_SPACE    1468
#define BOSEWAVE_BIT_MARK        428
#define BOSEWAVE_ONE_SPACE       896
#define BOSEWAVE_ZERO_SPACE      1492
#define BOSEWAVE_BITS            16

esp_err_t ir_decode_bosewave(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif
