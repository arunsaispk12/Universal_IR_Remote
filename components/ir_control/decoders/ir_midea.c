/**
 * @file ir_midea.c
 * @brief Midea AC Protocol Decoder Implementation
 */

#include "ir_midea.h"
#include "ir_timing.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "IR_MIDEA";

esp_err_t ir_decode_midea(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < MIDEA_BITS + 1) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check header
    if (!ir_match_mark(&symbols[0], MIDEA_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], MIDEA_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }

    // Decode 48 bits (6 bytes)
    uint8_t data[MIDEA_BYTES] = {0};

    for (uint8_t byte_idx = 0; byte_idx < MIDEA_BYTES; byte_idx++) {
        uint8_t byte_val = 0;

        for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++) {
            size_t symbol_idx = 1 + (byte_idx * 8) + bit_idx;
            const rmt_symbol_word_t *sym = &symbols[symbol_idx];

            if (!ir_match_mark(sym, MIDEA_BIT_MARK, 0)) {
                return ESP_FAIL;
            }

            if (ir_match_space(sym, MIDEA_ONE_SPACE, 0)) {
                byte_val |= (1 << bit_idx);  // LSB first
            } else if (!ir_match_space(sym, MIDEA_ZERO_SPACE, 0)) {
                return ESP_FAIL;
            }
        }

        data[byte_idx] = byte_val;
    }

    // Validate inverted bytes (bytes 3-5 should be inverse of bytes 0-2)
    bool validation_ok = true;
    for (uint8_t i = 0; i < 3; i++) {
        if (data[i] != (uint8_t)~data[i + 3]) {
            ESP_LOGW(TAG, "Byte %d: data=0x%02X, inverted=0x%02X (expected 0x%02X)",
                     i, data[i], data[i + 3], (uint8_t)~data[i]);
            validation_ok = false;
        }
    }

    // Fill code structure
    code->protocol = IR_PROTOCOL_MIDEA;
    code->bits = MIDEA_BITS;
    code->address = data[0];  // First byte is typically address/mode
    code->command = data[1];  // Second byte is command/temp
    code->flags = 0;

    // Store first 3 bytes (actual data, not inverted) in data field
    code->data = (uint32_t)data[0] |
                 ((uint32_t)data[1] << 8) |
                 ((uint32_t)data[2] << 16);

    ESP_LOGI(TAG, "Decoded Midea AC: Addr=0x%02X, Cmd=0x%02X, Validation=%s",
             data[0], data[1], validation_ok ? "OK" : "FAIL");

    return ESP_OK;
}
