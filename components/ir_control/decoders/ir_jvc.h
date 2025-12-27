/**
 * @file ir_jvc.h
 * @brief JVC Protocol Decoder
 *
 * JVC protocol decoder for AV receivers and equipment.
 * Special feature: Repeat frames have NO header (headerless repeats).
 *
 * Protocol characteristics:
 * - Carrier: 38kHz
 * - Encoding: Pulse distance (LSB first)
 * - Header: 8400us mark + 4200us space (first frame only)
 * - Data: 16 bits (8 address + 8 command)
 * - Bit mark: 525us, Space: 525us/"0" or 1575us/"1"
 * - Repeat: Headerless (data bits only)
 *
 * Based on Arduino-IRremote library
 * MIT License
 */

#ifndef IR_JVC_H
#define IR_JVC_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JVC_HEADER_MARK      8400
#define JVC_HEADER_SPACE     4200
#define JVC_BIT_MARK         525
#define JVC_ONE_SPACE        1575
#define JVC_ZERO_SPACE       525
#define JVC_BITS             16

esp_err_t ir_decode_jvc(const rmt_symbol_word_t *symbols,
                         size_t num_symbols,
                         ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif /* IR_JVC_H */
