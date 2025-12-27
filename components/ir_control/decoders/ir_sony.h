/**
 * @file ir_sony.h
 * @brief Sony SIRC Protocol Decoder
 *
 * Sony SIRC (Sony Infrared Remote Control) protocol decoder
 * Supports 12-bit (SIRC-12), 15-bit (SIRC-15), and 20-bit (SIRC-20) variants
 *
 * Protocol characteristics:
 * - Carrier frequency: 40kHz (NOT standard 38kHz!)
 * - Encoding: Pulse WIDTH (not pulse distance)
 * - Bit order: LSB first
 * - Header: 2400us mark + 600us space
 * - Data bits: 1200us mark = "1", 600us mark = "0"
 * - Space: Always 600us
 * - NO stop bit (unique to Sony)
 * - Repeat period: 45ms
 *
 * Variants:
 * - SIRC-12: 7 command bits + 5 address bits
 * - SIRC-15: 7 command bits + 8 address bits
 * - SIRC-20: 7 command bits + 13 address bits (5 device + 8 extended)
 *
 * Reference: https://www.sbprojects.net/knowledge/ir/sirc.php
 *
 * Based on Arduino-IRremote library
 * MIT License
 */

#ifndef IR_SONY_H
#define IR_SONY_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// Sony protocol timing constants (microseconds)
#define SONY_UNIT                600   // Base unit (24 periods of 40kHz)
#define SONY_HEADER_MARK         2400  // Start bit: 4 * 600us
#define SONY_HEADER_SPACE        600   // Start space: 1 * 600us
#define SONY_ONE_MARK            1200  // "1" bit: 2 * 600us
#define SONY_ZERO_MARK           600   // "0" bit: 1 * 600us
#define SONY_SPACE               600   // All spaces: 1 * 600us

// Sony bit lengths for different variants
#define SONY_BITS_12             12    // Standard: 7 cmd + 5 addr
#define SONY_BITS_15             15    // Extended: 7 cmd + 8 addr
#define SONY_BITS_20             20    // Full: 7 cmd + 5 dev + 8 extended

// Sony carrier frequency
#define SONY_CARRIER_KHZ         40    // 40kHz (NOT 38kHz!)

/**
 * @brief Decode Sony SIRC protocol from RMT symbols
 *
 * Decodes Sony SIRC IR signals in 12-bit, 15-bit, or 20-bit formats.
 * Automatically determines variant based on number of symbols received.
 *
 * @param symbols Pointer to RMT symbol array
 * @param num_symbols Number of symbols in array
 * @param code Output structure to fill with decoded data
 * @return ESP_OK if successfully decoded as Sony protocol
 *         ESP_ERR_INVALID_ARG if num_symbols doesn't match Sony variants
 *         ESP_FAIL if timing doesn't match Sony protocol
 */
esp_err_t ir_decode_sony(const rmt_symbol_word_t *symbols,
                          size_t num_symbols,
                          ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif /* IR_SONY_H */
