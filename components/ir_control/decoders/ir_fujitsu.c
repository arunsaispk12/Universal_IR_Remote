/**
 * @file ir_fujitsu.c
 * @brief Fujitsu General AC Protocol Decoder Implementation
 */

#include "ir_fujitsu.h"
#include "ir_timing.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "IR_FUJITSU";

/**
 * @brief Calculate Fujitsu checksum
 */
static uint8_t fujitsu_checksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    // Sum all bytes except last (checksum byte)
    for (uint8_t i = 0; i < len - 1; i++) {
        sum += data[i];
    }
    return (0x100 - sum) & 0xFF;  // Two's complement
}

esp_err_t ir_decode_fujitsu(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < FUJITSU_MIN_BITS + 1) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check header
    if (!ir_match_mark(&symbols[0], FUJITSU_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], FUJITSU_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }

    // Determine frame length by counting symbols
    // Typical: 64 bits (65 symbols) or 128 bits (129 symbols)
    size_t available_data_symbols = num_symbols - 1;  // Minus header
    uint8_t num_bytes = available_data_symbols / 8;

    if (num_bytes < FUJITSU_MIN_BYTES) {
        ESP_LOGW(TAG, "Frame too short: %d bytes", num_bytes);
        return ESP_FAIL;
    }

    // Cap at maximum
    if (num_bytes > FUJITSU_MAX_BYTES) {
        num_bytes = FUJITSU_MAX_BYTES;
    }

    // Decode data
    uint8_t data[FUJITSU_MAX_BYTES] = {0};

    for (uint8_t byte_idx = 0; byte_idx < num_bytes; byte_idx++) {
        uint8_t byte_val = 0;

        for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++) {
            size_t symbol_idx = 1 + (byte_idx * 8) + bit_idx;
            if (symbol_idx >= num_symbols) {
                break;  // Safety check
            }

            const rmt_symbol_word_t *sym = &symbols[symbol_idx];

            if (!ir_match_mark(sym, FUJITSU_BIT_MARK, 0)) {
                return ESP_FAIL;
            }

            if (ir_match_space(sym, FUJITSU_ONE_SPACE, 0)) {
                byte_val |= (1 << bit_idx);  // LSB first
            } else if (!ir_match_space(sym, FUJITSU_ZERO_SPACE, 0)) {
                return ESP_FAIL;
            }
        }

        data[byte_idx] = byte_val;
    }

    // Validate checksum
    uint8_t calculated_checksum = fujitsu_checksum(data, num_bytes);
    uint8_t received_checksum = data[num_bytes - 1];
    bool checksum_ok = (calculated_checksum == received_checksum);

    if (!checksum_ok) {
        ESP_LOGW(TAG, "Checksum failed: expected 0x%02X, got 0x%02X",
                 received_checksum, calculated_checksum);
        // Continue anyway - some variants differ
    }

    // Fill code structure
    code->protocol = IR_PROTOCOL_FUJITSU;
    code->bits = num_bytes * 8;
    code->address = 0x14;  // Fujitsu manufacturer code
    code->command = data[5 < num_bytes ? 5 : 0];  // Command/mode byte
    code->flags = 0;

    // Store first 4 bytes in data field
    code->data = (uint32_t)data[0] |
                 ((uint32_t)data[1] << 8) |
                 ((uint32_t)data[2] << 16) |
                 ((uint32_t)data[3] << 24);

    ESP_LOGI(TAG, "Decoded Fujitsu AC: Length=%d bytes, Checksum=%s",
             num_bytes, checksum_ok ? "OK" : "FAIL");

    return ESP_OK;
}
