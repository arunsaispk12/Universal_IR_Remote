/**
 * @file ir_protocols.c
 * @brief IR Protocol Database Implementation
 *
 * Contains timing constants for all supported IR protocols ported from Arduino-IRremote.
 * Timing values are in microseconds unless otherwise specified.
 *
 * Based on Arduino-IRremote library
 * References:
 * - https://github.com/Arduino-IRremote/Arduino-IRremote
 * - http://www.sbprojects.net/knowledge/ir/index.php
 *
 * MIT License
 */

#include "ir_protocols.h"
#include <stddef.h>

// Protocol name strings (for logging/debugging)
static const char* PROTOCOL_NAMES[] = {
    [IR_PROTOCOL_UNKNOWN] = "UNKNOWN",
    [IR_PROTOCOL_NEC] = "NEC",
    [IR_PROTOCOL_SAMSUNG] = "SAMSUNG",
    [IR_PROTOCOL_SONY] = "SONY",
    [IR_PROTOCOL_JVC] = "JVC",
    [IR_PROTOCOL_RC5] = "RC5",
    [IR_PROTOCOL_RC6] = "RC6",
    [IR_PROTOCOL_LG] = "LG",
    [IR_PROTOCOL_DENON] = "DENON",
    [IR_PROTOCOL_SHARP] = "SHARP",
    [IR_PROTOCOL_PANASONIC] = "PANASONIC",
    [IR_PROTOCOL_KASEIKYO] = "KASEIKYO",
    [IR_PROTOCOL_WHYNTER] = "WHYNTER",
    [IR_PROTOCOL_LEGO_PF] = "LEGO_PF",
    [IR_PROTOCOL_MAGIQUEST] = "MAGIQUEST",
    [IR_PROTOCOL_BOSEWAVE] = "BOSEWAVE",
    [IR_PROTOCOL_BANG_OLUFSEN] = "BANG_OLUFSEN",
    [IR_PROTOCOL_SAMSUNG48] = "SAMSUNG48",
    [IR_PROTOCOL_SAMSUNGLG] = "SAMSUNGLG",
    [IR_PROTOCOL_LG2] = "LG2",
    [IR_PROTOCOL_ONKYO] = "ONKYO",
    [IR_PROTOCOL_APPLE] = "APPLE",
    [IR_PROTOCOL_FAST] = "FAST",
    [IR_PROTOCOL_PULSE_DISTANCE] = "PULSE_DISTANCE",
    [IR_PROTOCOL_PULSE_WIDTH] = "PULSE_WIDTH",
    [IR_PROTOCOL_RAW] = "RAW",
};

/**
 * @brief Protocol timing constants database
 *
 * Format: {protocol, carrier_kHz, header_mark, header_space, bit_mark, one_space, zero_space, flags, repeat_period_ms, bits}
 *
 * Notes:
 * - For pulse distance protocols: bit_mark is constant, one_space != zero_space
 * - For pulse width protocols: one_space != bit_mark, zero_space == one_space
 * - For bi-phase protocols (RC5/RC6): special decoding required
 */
static const ir_protocol_constants_t PROTOCOL_DATABASE[] = {
    // NEC Protocol (most common - TV/AC remotes)
    // Reference: http://www.sbprojects.net/knowledge/ir/nec.php
    {
        .protocol = IR_PROTOCOL_NEC,
        .carrier_khz = 38,
        .header_mark_us = 9000,
        .header_space_us = 4500,
        .bit_mark_us = 560,
        .one_space_us = 1690,
        .zero_space_us = 560,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 110,
        .bits = 32
    },

    // Samsung Protocol (Samsung TVs/devices)
    // Similar to NEC but different header timing
    {
        .protocol = IR_PROTOCOL_SAMSUNG,
        .carrier_khz = 38,
        .header_mark_us = 4500,
        .header_space_us = 4500,
        .bit_mark_us = 560,
        .one_space_us = 1690,
        .zero_space_us = 560,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 108,
        .bits = 32
    },

    // Sony SIRC Protocol (Sony TVs, Blu-ray)
    // Reference: http://www.sbprojects.net/knowledge/ir/sirc.php
    // Uses 40kHz carrier (NOT 38kHz!)
    // Pulse WIDTH encoding (different from NEC)
    // Three variants: 12, 15, 20 bits
    {
        .protocol = IR_PROTOCOL_SONY,
        .carrier_khz = 40,  // Important: 40kHz not 38kHz!
        .header_mark_us = 2400,
        .header_space_us = 600,
        .bit_mark_us = 600,        // "0" mark
        .one_space_us = 1200,      // "1" mark (pulse width encoding)
        .zero_space_us = 600,      // Space after all bits
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_WIDTH | PROTOCOL_NO_STOP_BIT,
        .repeat_period_ms = 45,
        .bits = 0  // Variable: 12, 15, or 20 bits
    },

    // JVC Protocol (JVC AV receivers)
    // Reference: http://www.sbprojects.net/knowledge/ir/jvc.php
    // Special feature: Repeat frames have NO header
    {
        .protocol = IR_PROTOCOL_JVC,
        .carrier_khz = 38,
        .header_mark_us = 8400,
        .header_space_us = 4200,
        .bit_mark_us = 525,
        .one_space_us = 1575,
        .zero_space_us = 525,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 60,
        .bits = 16
    },

    // LG Protocol (LG TVs, air conditioners)
    // Reference: http://www.sbprojects.net/knowledge/ir/lg.php
    // 28 bits with checksum (4-bit sum of nibbles)
    {
        .protocol = IR_PROTOCOL_LG,
        .carrier_khz = 38,
        .header_mark_us = 9000,
        .header_space_us = 4500,
        .bit_mark_us = 560,
        .one_space_us = 1690,
        .zero_space_us = 560,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 110,
        .bits = 28
    },

    // RC5 Protocol (Philips, Marantz audio equipment)
    // Reference: http://www.sbprojects.net/knowledge/ir/rc5.php
    // Bi-phase (Manchester) encoding - special decoder needed!
    // Has toggle bit to distinguish repeated button presses
    {
        .protocol = IR_PROTOCOL_RC5,
        .carrier_khz = 36,  // 36kHz carrier
        .header_mark_us = 0,  // No header - bi-phase encoding
        .header_space_us = 0,
        .bit_mark_us = 889,  // Half-bit time
        .one_space_us = 889,
        .zero_space_us = 889,
        .flags = PROTOCOL_IS_MSB_FIRST | PROTOCOL_IS_BIPHASE,
        .repeat_period_ms = 114,
        .bits = 13
    },

    // RC6 Protocol (Microsoft Media Center remotes)
    // Reference: http://www.sbprojects.net/knowledge/ir/rc6.php
    // Also bi-phase encoding with toggle bit
    {
        .protocol = IR_PROTOCOL_RC6,
        .carrier_khz = 36,
        .header_mark_us = 2666,
        .header_space_us = 889,
        .bit_mark_us = 444,  // Half-bit time
        .one_space_us = 444,
        .zero_space_us = 444,
        .flags = PROTOCOL_IS_MSB_FIRST | PROTOCOL_IS_BIPHASE,
        .repeat_period_ms = 114,
        .bits = 20  // Mode 0: 20 bits (can be 21-31 for other modes)
    },

    // Denon/Sharp Protocol (same protocol, different name)
    // Reference: http://www.sbprojects.net/knowledge/ir/denon.php
    // 15 bits with parity (5 addr + 8 cmd + 2 parity)
    {
        .protocol = IR_PROTOCOL_DENON,
        .carrier_khz = 38,
        .header_mark_us = 275,
        .header_space_us = 775,
        .bit_mark_us = 275,
        .one_space_us = 1900,
        .zero_space_us = 775,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 45,
        .bits = 15
    },

    // Sharp Protocol (alias for Denon)
    {
        .protocol = IR_PROTOCOL_SHARP,
        .carrier_khz = 38,
        .header_mark_us = 275,
        .header_space_us = 775,
        .bit_mark_us = 275,
        .one_space_us = 1900,
        .zero_space_us = 775,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 45,
        .bits = 15
    },

    // Panasonic/Kaseikyo Protocol (Panasonic, some AC units)
    // Reference: http://www.sbprojects.net/knowledge/ir/kaseikyo.php
    // 48 bits with vendor ID and checksum
    {
        .protocol = IR_PROTOCOL_PANASONIC,
        .carrier_khz = 37,
        .header_mark_us = 3456,
        .header_space_us = 1728,
        .bit_mark_us = 432,
        .one_space_us = 1296,
        .zero_space_us = 432,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 130,
        .bits = 48
    },

    // Kaseikyo (generic - Panasonic variant)
    {
        .protocol = IR_PROTOCOL_KASEIKYO,
        .carrier_khz = 37,
        .header_mark_us = 3456,
        .header_space_us = 1728,
        .bit_mark_us = 432,
        .one_space_us = 1296,
        .zero_space_us = 432,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 130,
        .bits = 48
    },

    // Apple Protocol (Apple remotes)
    // Extension of NEC with different address
    {
        .protocol = IR_PROTOCOL_APPLE,
        .carrier_khz = 38,
        .header_mark_us = 9000,
        .header_space_us = 4500,
        .bit_mark_us = 560,
        .one_space_us = 1690,
        .zero_space_us = 560,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 110,
        .bits = 32
    },

    // Onkyo Protocol (Onkyo AV receivers)
    // NEC variant with different address scheme
    {
        .protocol = IR_PROTOCOL_ONKYO,
        .carrier_khz = 38,
        .header_mark_us = 9000,
        .header_space_us = 4500,
        .bit_mark_us = 560,
        .one_space_us = 1690,
        .zero_space_us = 560,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 110,
        .bits = 32
    },

    // Samsung48 Protocol (Samsung 48-bit variant for AC units)
    {
        .protocol = IR_PROTOCOL_SAMSUNG48,
        .carrier_khz = 38,
        .header_mark_us = 4500,
        .header_space_us = 4500,
        .bit_mark_us = 560,
        .one_space_us = 1690,
        .zero_space_us = 560,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 108,
        .bits = 48
    },

    // LG2 Protocol (LG variant for air conditioners)
    {
        .protocol = IR_PROTOCOL_LG2,
        .carrier_khz = 38,
        .header_mark_us = 3200,
        .header_space_us = 9900,
        .bit_mark_us = 560,
        .one_space_us = 1690,
        .zero_space_us = 560,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 110,
        .bits = 28
    },

    // ============ EXOTIC PROTOCOLS ============

    // Whynter Protocol (Whynter portable AC units)
    // Reference: Arduino-IRremote/src/ir_Whynter.hpp
    {
        .protocol = IR_PROTOCOL_WHYNTER,
        .carrier_khz = 38,
        .header_mark_us = 2850,
        .header_space_us = 2850,
        .bit_mark_us = 750,
        .one_space_us = 750,
        .zero_space_us = 750,
        .flags = PROTOCOL_IS_MSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 100,
        .bits = 32
    },

    // Lego Power Functions Protocol (Lego IR remotes)
    // Reference: Arduino-IRremote/src/ir_Lego.hpp
    // Uses 38kHz PWM with start bit + 4 nibbles
    {
        .protocol = IR_PROTOCOL_LEGO_PF,
        .carrier_khz = 38,
        .header_mark_us = 158,  // Start bit
        .header_space_us = 1026,
        .bit_mark_us = 158,
        .one_space_us = 553,
        .zero_space_us = 263,
        .flags = PROTOCOL_IS_MSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 0,  // No repeat (manual retransmit)
        .bits = 16
    },

    // MagiQuest Protocol (MagiQuest wands - theme park toys)
    // Reference: Arduino-IRremote/src/ir_MagiQuest.hpp
    {
        .protocol = IR_PROTOCOL_MAGIQUEST,
        .carrier_khz = 38,
        .header_mark_us = 0,  // No header
        .header_space_us = 0,
        .bit_mark_us = 288,
        .one_space_us = 864,
        .zero_space_us = 576,
        .flags = PROTOCOL_IS_MSB_FIRST | PROTOCOL_IS_PULSE_WIDTH,
        .repeat_period_ms = 0,
        .bits = 56  // 8 start + 48 data bits
    },

    // BoseWave Protocol (Bose Wave radios)
    // Reference: Arduino-IRremote/src/ir_BoseWave.hpp
    {
        .protocol = IR_PROTOCOL_BOSEWAVE,
        .carrier_khz = 38,
        .header_mark_us = 1014,
        .header_space_us = 1468,
        .bit_mark_us = 428,
        .one_space_us = 896,
        .zero_space_us = 1492,
        .flags = PROTOCOL_IS_MSB_FIRST | PROTOCOL_IS_PULSE_WIDTH,
        .repeat_period_ms = 50,
        .bits = 16
    },

    // Bang & Olufsen Protocol (B&O high-end audio/video)
    // Reference: Arduino-IRremote/src/ir_BangOlufsen.hpp
    // WARNING: Uses 455kHz carrier - VERY different from standard 38kHz!
    // NOTE: Cannot be received with standard 38kHz receivers
    {
        .protocol = IR_PROTOCOL_BANG_OLUFSEN,
        .carrier_khz = 455,  // 455kHz! Special receiver needed
        .header_mark_us = 3125,
        .header_space_us = 3125,
        .bit_mark_us = 625,
        .one_space_us = 625,
        .zero_space_us = 1250,
        .flags = PROTOCOL_IS_MSB_FIRST | PROTOCOL_IS_PULSE_WIDTH,
        .repeat_period_ms = 100,
        .bits = 16
    },

    // FAST Protocol (FAST brand remotes - rare)
    // Reference: Arduino-IRremote/src/ir_FAST.hpp
    {
        .protocol = IR_PROTOCOL_FAST,
        .carrier_khz = 38,
        .header_mark_us = 0,  // No header
        .header_space_us = 0,
        .bit_mark_us = 320,
        .one_space_us = 640,
        .zero_space_us = 320,
        .flags = PROTOCOL_IS_LSB_FIRST | PROTOCOL_IS_PULSE_DISTANCE,
        .repeat_period_ms = 0,
        .bits = 8
    },
};

#define PROTOCOL_DATABASE_SIZE (sizeof(PROTOCOL_DATABASE) / sizeof(PROTOCOL_DATABASE[0]))

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

const ir_protocol_constants_t* ir_get_protocol_constants(ir_protocol_t protocol) {
    // Linear search through database (small enough for acceptable performance)
    for (size_t i = 0; i < PROTOCOL_DATABASE_SIZE; i++) {
        if (PROTOCOL_DATABASE[i].protocol == protocol) {
            return &PROTOCOL_DATABASE[i];
        }
    }
    return NULL;  // Protocol not found
}

const char* ir_protocol_to_string(ir_protocol_t protocol) {
    if (protocol < sizeof(PROTOCOL_NAMES) / sizeof(PROTOCOL_NAMES[0])) {
        return PROTOCOL_NAMES[protocol];
    }
    return "INVALID";
}
