/**
 * @file ir_rc6.c
 * @brief Philips RC6 Protocol Decoder Implementation
 *
 * RC6 uses bi-phase encoding similar to RC5 but with important differences:
 * - Has a leader pulse
 * - Has a "trailer bit" (toggle bit) with DOUBLE length
 * - More complex timing
 */

#include "ir_rc6.h"
#include "ir_timing.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "IR_RC6";

/**
 * @brief Decode standard RC6 bi-phase bit (444us periods)
 */
static esp_err_t decode_rc6_bit(const rmt_symbol_word_t *symbols, size_t *idx,
                                 size_t num_symbols, uint8_t *bit_value, bool is_trailer) {
    if (*idx >= num_symbols) {
        return ESP_FAIL;
    }

    const rmt_symbol_word_t *sym = &symbols[*idx];
    uint16_t duration0 = sym->duration0;
    uint16_t duration1 = sym->duration1;

    // Trailer bit is double length (889us each half instead of 444us)
    uint16_t expected_duration = is_trailer ? (RC6_UNIT * 2) : RC6_UNIT;

    if (ir_timing_matches_percent(duration0, expected_duration, 30) &&
        ir_timing_matches_percent(duration1, expected_duration, 30)) {

        // Bit value depends on transition direction
        if (sym->level0 == 0) {
            *bit_value = 0;  // Space-to-Mark = "0"
        } else {
            *bit_value = 1;  // Mark-to-Space = "1"
        }

        (*idx)++;
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t ir_decode_rc6(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < 20) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t idx = 0;

    // Check leader (long mark + space)
    if (!ir_match_mark(&symbols[idx], RC6_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[idx], RC6_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }
    idx++;

    // Decode start bit (should be 1)
    uint8_t start_bit = 0;
    if (decode_rc6_bit(symbols, &idx, num_symbols, &start_bit, false) != ESP_OK) {
        return ESP_FAIL;
    }

    if (start_bit != 1) {
        ESP_LOGD(TAG, "Invalid start bit: %d", start_bit);
        return ESP_FAIL;
    }

    // Decode mode (3 bits)
    uint8_t mode = 0;
    for (uint8_t i = 0; i < 3; i++) {
        uint8_t bit = 0;
        if (decode_rc6_bit(symbols, &idx, num_symbols, &bit, false) != ESP_OK) {
            return ESP_FAIL;
        }
        mode = (mode << 1) | bit;
    }

    // Decode trailer bit (toggle bit - DOUBLE LENGTH!)
    uint8_t toggle_bit = 0;
    if (decode_rc6_bit(symbols, &idx, num_symbols, &toggle_bit, true) != ESP_OK) {
        return ESP_FAIL;
    }

    // Decode address (8 bits for mode 0)
    uint8_t address = 0;
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = 0;
        if (decode_rc6_bit(symbols, &idx, num_symbols, &bit, false) != ESP_OK) {
            return ESP_FAIL;
        }
        address = (address << 1) | bit;
    }

    // Decode command (8 bits for mode 0)
    uint8_t command = 0;
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = 0;
        if (decode_rc6_bit(symbols, &idx, num_symbols, &bit, false) != ESP_OK) {
            return ESP_FAIL;
        }
        command = (command << 1) | bit;
    }

    // Fill code structure
    code->protocol = IR_PROTOCOL_RC6;
    code->bits = 20;  // Mode 0: 1 start + 3 mode + 1 toggle + 8 addr + 8 cmd
    code->address = address;
    code->command = command;
    code->data = ((uint32_t)mode << 17) |
                 ((uint32_t)toggle_bit << 16) |
                 ((uint32_t)address << 8) |
                 command;
    code->flags = toggle_bit ? IR_FLAG_TOGGLE_BIT : 0;

    ESP_LOGI(TAG, "Decoded RC6: Mode=%d, Addr=0x%02X, Cmd=0x%02X, Toggle=%d",
             mode, address, command, toggle_bit);

    return ESP_OK;
}
