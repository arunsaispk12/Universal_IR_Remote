/**
 * @file ir_fujitsu.h
 * @brief Fujitsu General AC Protocol Decoder
 *
 * Fujitsu General air conditioners use a variable-length protocol
 * - Popular in Japan and Asia
 * - Variable length: 64-128 bits depending on model
 * - Checksum validation
 * - Based on Arduino-IRremote library, MIT License
 *
 * Protocol Structure:
 * - Header: 3300us mark + 1650us space
 * - Bit encoding: Pulse distance (420us mark, 420us/1280us space)
 * - Data: 8-16 bytes (model dependent)
 * - Checksum: Custom algorithm
 *
 * Common lengths:
 * - 64 bits (8 bytes): Simple commands
 * - 128 bits (16 bytes): Full AC state
 *
 * Reference: https://github.com/Arduino-IRremote/Arduino-IRremote
 */

#ifndef IR_FUJITSU_H
#define IR_FUJITSU_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/rmt_rx.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// Fujitsu timing constants
#define FUJITSU_HEADER_MARK        3300
#define FUJITSU_HEADER_SPACE       1650
#define FUJITSU_BIT_MARK           420
#define FUJITSU_ONE_SPACE          1280
#define FUJITSU_ZERO_SPACE         420

// Fujitsu frame sizes (variable)
#define FUJITSU_MIN_BITS           64
#define FUJITSU_MAX_BITS           128
#define FUJITSU_MIN_BYTES          8
#define FUJITSU_MAX_BYTES          16

/**
 * @brief Decode Fujitsu AC IR protocol
 *
 * Handles variable-length Fujitsu protocol with checksum validation
 *
 * @param symbols RMT symbol array
 * @param num_symbols Number of symbols
 * @param code Output IR code structure
 * @return ESP_OK on success, ESP_FAIL on decode failure, ESP_ERR_INVALID_ARG on invalid input
 */
esp_err_t ir_decode_fujitsu(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code);

#ifdef __cplusplus
}
#endif

#endif // IR_FUJITSU_H
