/**
 * @file ir_mitsubishi.c
 * @brief Mitsubishi Electric AC Protocol Decoder Implementation
 */

#include "ir_mitsubishi.h"
#include "ir_timing.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "IR_MITSUBISHI";

esp_err_t ir_decode_mitsubishi(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < MITSUBISHI_BITS + 1) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check header
    if (!ir_match_mark(&symbols[0], MITSUBISHI_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], MITSUBISHI_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }

    // Decode 152 bits (19 bytes)
    uint8_t data[MITSUBISHI_BYTES] = {0};

    for (uint_fast16_t byte_idx = 0; byte_idx < MITSUBISHI_BYTES; byte_idx++) {
        uint8_t byte_val = 0;

        for (uint_fast8_t bit_idx = 0; bit_idx < 8; bit_idx++) {
            uint16_t symbol_idx = 1 + (byte_idx * 8) + bit_idx;
            const rmt_symbol_word_t *sym = &symbols[symbol_idx];

            if (!ir_match_mark(sym, MITSUBISHI_BIT_MARK, 0)) {
                return ESP_FAIL;
            }

            if (ir_match_space(sym, MITSUBISHI_ONE_SPACE, 0)) {
                byte_val |= (1 << bit_idx);  // LSB first
            } else if (!ir_match_space(sym, MITSUBISHI_ZERO_SPACE, 0)) {
                return ESP_FAIL;
            }
        }

        data[byte_idx] = byte_val;
    }

    // Validate checksum (byte sum of first 18 bytes)
    uint8_t checksum = 0;
    for (uint_fast8_t i = 0; i < MITSUBISHI_BYTES - 1; i++) {
        checksum += data[i];
    }

    if (checksum != data[MITSUBISHI_BYTES - 1]) {
        ESP_LOGW(TAG, "Checksum failed: expected 0x%02X, got 0x%02X",
                 data[MITSUBISHI_BYTES - 1], checksum);
        // Continue anyway - some variants may have different checksum
    }

    // Fill code structure
    code->protocol = IR_PROTOCOL_MITSUBISHI;
    code->bits = MITSUBISHI_BITS;
    code->address = 0x23;  // Mitsubishi manufacturer code
    code->command = data[5];  // Command byte (mode/temp)
    code->flags = 0;

    // Store full data in data field (first 4 bytes for compatibility)
    code->data = (uint32_t)data[0] |
                 ((uint32_t)data[1] << 8) |
                 ((uint32_t)data[2] << 16) |
                 ((uint32_t)data[3] << 24);

    ESP_LOGI(TAG, "Decoded Mitsubishi AC: Mode=0x%02X, Checksum=%s",
             data[5], (checksum == data[MITSUBISHI_BYTES - 1]) ? "OK" : "FAIL");

    return ESP_OK;
}
