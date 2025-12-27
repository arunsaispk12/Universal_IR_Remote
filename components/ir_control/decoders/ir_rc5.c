/**
 * @file ir_rc5.c
 * @brief Philips RC5 Protocol Decoder Implementation
 *
 * RC5 uses bi-phase (Manchester) encoding where each bit is represented by a transition:
 * - "0": Space-to-Mark transition (low to high)
 * - "1": Mark-to-Space transition (high to low)
 */

#include "ir_rc5.h"
#include "ir_timing.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "IR_RC5";

/**
 * @brief Decode bi-phase (Manchester) encoded bit from RC5 symbol
 *
 * RC5 bi-phase encoding:
 * - Each bit period is 1778us (2 * 889us)
 * - "1": Mark(889us) then Space(889us)
 * - "0": Space(889us) then Mark(889us)
 */
static esp_err_t decode_rc5_bit(const rmt_symbol_word_t *symbols, size_t *idx,
                                 size_t num_symbols, uint8_t *bit_value) {
    if (*idx >= num_symbols) {
        return ESP_FAIL;
    }

    const rmt_symbol_word_t *sym = &symbols[*idx];
    uint16_t duration0 = sym->duration0;
    uint16_t duration1 = sym->duration1;

    // Check for Manchester encoding (both durations should be ~889us)
    if (ir_timing_matches_percent(duration0, RC5_UNIT, 25) &&
        ir_timing_matches_percent(duration1, RC5_UNIT, 25)) {

        // Bit value depends on which comes first (mark or space)
        // Note: IR receiver inverts, so we check level0
        if (sym->level0 == 0) {
            // Space then Mark = "0"
            *bit_value = 0;
        } else {
            // Mark then Space = "1"
            *bit_value = 1;
        }

        (*idx)++;
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t ir_decode_rc5(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < RC5_BITS) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t idx = 0;
    uint16_t decoded_value = 0;

    // Decode 14 bits (start bits + toggle + address + command)
    for (uint8_t i = 0; i < RC5_BITS; i++) {
        uint8_t bit_value = 0;

        if (decode_rc5_bit(symbols, &idx, num_symbols, &bit_value) != ESP_OK) {
            return ESP_FAIL;
        }

        // Build value MSB first
        decoded_value = (decoded_value << 1) | bit_value;
    }

    // Extract fields from 14-bit value
    // Format: SS T AAAAA CCCCCC
    // SS = Start bits (should be 11)
    // T = Toggle bit
    // A = Address (5 bits)
    // C = Command (6 bits)

    uint8_t start_bits = (decoded_value >> 12) & 0x03;
    uint8_t toggle_bit = (decoded_value >> 11) & 0x01;
    uint8_t address = (decoded_value >> 6) & 0x1F;
    uint8_t command = decoded_value & 0x3F;

    // Validate start bits (should be 0b11 for standard RC5)
    if (start_bits != 0x03) {
        ESP_LOGD(TAG, "Invalid start bits: 0x%X (expected 0x3)", start_bits);
        // Continue anyway - might be extended RC5
    }

    // Fill code structure
    code->protocol = IR_PROTOCOL_RC5;
    code->bits = RC5_BITS;
    code->address = address;
    code->command = command;
    code->data = decoded_value;
    code->flags = toggle_bit ? IR_FLAG_TOGGLE_BIT : 0;

    ESP_LOGI(TAG, "Decoded RC5: Addr=0x%02X, Cmd=0x%02X, Toggle=%d",
             address, command, toggle_bit);

    return ESP_OK;
}
