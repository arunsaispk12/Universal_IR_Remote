/**
 * @file ir_haier.c
 * @brief Haier AC Protocol Decoder Implementation
 */

#include "ir_haier.h"
#include "ir_timing.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "IR_HAIER";

esp_err_t ir_decode_haier(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < HAIER_BITS + 1) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check header
    if (!ir_match_mark(&symbols[0], HAIER_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], HAIER_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }

    // Decode 104 bits (13 bytes)
    uint8_t data[HAIER_BYTES] = {0};

    for (uint8_t byte_idx = 0; byte_idx < HAIER_BYTES; byte_idx++) {
        uint8_t byte_val = 0;

        for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++) {
            size_t symbol_idx = 1 + (byte_idx * 8) + bit_idx;
            const rmt_symbol_word_t *sym = &symbols[symbol_idx];

            if (!ir_match_mark(sym, HAIER_BIT_MARK, 0)) {
                return ESP_FAIL;
            }

            if (ir_match_space(sym, HAIER_ONE_SPACE, 0)) {
                byte_val |= (1 << bit_idx);  // LSB first
            } else if (!ir_match_space(sym, HAIER_ZERO_SPACE, 0)) {
                return ESP_FAIL;
            }
        }

        data[byte_idx] = byte_val;
    }

    // Validate checksum (XOR of all bytes except last)
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < HAIER_BYTES - 1; i++) {
        checksum ^= data[i];
    }

    bool checksum_ok = (checksum == data[HAIER_BYTES - 1]);
    if (!checksum_ok) {
        ESP_LOGW(TAG, "Checksum failed: expected 0x%02X, got 0x%02X",
                 data[HAIER_BYTES - 1], checksum);
    }

    // Fill code structure
    code->protocol = IR_PROTOCOL_HAIER;
    code->bits = HAIER_BITS;
    code->address = 0xA0;  // Haier manufacturer code
    code->command = data[9];  // Command byte
    code->flags = 0;

    // Store first 4 bytes in data field
    code->data = (uint32_t)data[0] |
                 ((uint32_t)data[1] << 8) |
                 ((uint32_t)data[2] << 16) |
                 ((uint32_t)data[3] << 24);

    ESP_LOGI(TAG, "Decoded Haier AC: Cmd=0x%02X, Checksum=%s",
             data[9], checksum_ok ? "OK" : "FAIL");

    return ESP_OK;
}
