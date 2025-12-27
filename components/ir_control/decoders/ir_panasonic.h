/**
 * @file ir_panasonic.h
 * @brief Panasonic/Kaseikyo Protocol Decoder
 *
 * 48-bit protocol used in air conditioners and advanced appliances.
 * Based on Arduino-IRremote library, MIT License
 */

#ifndef IR_PANASONIC_H
#define IR_PANASONIC_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PANASONIC_HEADER_MARK    3456
#define PANASONIC_HEADER_SPACE   1728
#define PANASONIC_BIT_MARK       432
#define PANASONIC_ONE_SPACE      1296
#define PANASONIC_ZERO_SPACE     432
#define PANASONIC_BITS           48

esp_err_t ir_decode_panasonic(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif
