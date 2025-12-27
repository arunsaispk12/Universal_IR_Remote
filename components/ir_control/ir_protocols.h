/**
 * @file ir_protocols.h
 * @brief IR Protocol Database - Timing constants for all supported protocols
 *
 * This file contains timing constants and configuration for 25+ IR protocols
 * ported from Arduino-IRremote library to ESP-IDF.
 *
 * Based on Arduino-IRremote library by Ken Shirriff, Armin Joachimsmeyer
 * https://github.com/Arduino-IRremote/Arduino-IRremote
 *
 * MIT License - Copyright (c) 2009-2025 Ken Shirriff, Armin Joachimsmeyer
 */

#ifndef IR_PROTOCOLS_H
#define IR_PROTOCOLS_H

#include <stdint.h>
#include <stdbool.h>
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Protocol encoding type flags
 */
#define PROTOCOL_IS_LSB_FIRST        0x00  // LSB transmitted first (default)
#define PROTOCOL_IS_MSB_FIRST        0x80  // MSB transmitted first
#define PROTOCOL_IS_PULSE_DISTANCE   0x00  // Pulse distance encoding (default)
#define PROTOCOL_IS_PULSE_WIDTH      0x10  // Pulse width encoding
#define PROTOCOL_HAS_STOP_BIT        0x00  // Stop bit present (default)
#define PROTOCOL_NO_STOP_BIT         0x20  // No stop bit
#define PROTOCOL_IS_BIPHASE          0x40  // Manchester/bi-phase encoding (RC5/RC6)

/**
 * @brief Protocol timing constants structure
 *
 * Contains all timing information needed to decode or encode a protocol
 */
typedef struct {
    ir_protocol_t protocol;          // Protocol identifier
    uint8_t carrier_khz;             // Carrier frequency in kHz (38, 40, 36, 455, etc.)
    uint16_t header_mark_us;         // Header mark duration (microseconds)
    uint16_t header_space_us;        // Header space duration (microseconds)
    uint16_t bit_mark_us;            // Bit mark duration (for pulse distance) or "0" mark (for pulse width)
    uint16_t one_space_us;           // Space for "1" bit (pulse distance) or mark for "1" (pulse width)
    uint16_t zero_space_us;          // Space for "0" bit (pulse distance) or mark for "0" (pulse width)
    uint8_t flags;                   // Protocol flags (MSB/LSB, pulse distance/width, etc.)
    uint16_t repeat_period_ms;       // Time between repeat frames (milliseconds)
    uint8_t bits;                    // Number of bits (0 = variable length)
} ir_protocol_constants_t;

/**
 * @brief Get protocol timing constants by protocol type
 *
 * @param protocol Protocol identifier
 * @return Pointer to protocol constants, or NULL if not found
 */
const ir_protocol_constants_t* ir_get_protocol_constants(ir_protocol_t protocol);

/**
 * @brief Get protocol name string
 *
 * @param protocol Protocol identifier
 * @return Pointer to protocol name string (constant)
 */
const char* ir_protocol_to_string(ir_protocol_t protocol);

#ifdef __cplusplus
}
#endif

#endif /* IR_PROTOCOLS_H */
