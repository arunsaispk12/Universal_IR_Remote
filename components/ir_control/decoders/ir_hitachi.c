/**
 * @file ir_hitachi.c
 * @brief Hitachi AC Protocol Decoder Implementation
 *
 * Important for Indian market - popular AC brand
 */

#include "ir_hitachi.h"
#include "ir_timing.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "IR_HITACHI";

/**
 * @brief Calculate Hitachi AC checksum
 *
 * Hitachi uses byte sum modulo 256
 */
static uint8_t hitachi_checksum(const uint8_t *data, uint8_t len) {
    uint16_t sum = 0;

    // Sum all bytes except last (checksum byte)
    for (uint8_t i = 0; i < len - 1; i++) {
        sum += data[i];
    }

    return sum & 0xFF;  // Modulo 256
}

esp_err_t ir_decode_hitachi(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < HITACHI_MIN_BITS + 1) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check header
    if (!ir_match_mark(&symbols[0], HITACHI_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], HITACHI_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }

    // Determine frame length by counting available symbols
    size_t available_data_symbols = num_symbols - 1;  // Minus header
    uint8_t num_bytes = available_data_symbols / 8;

    if (num_bytes < HITACHI_MIN_BYTES) {
        ESP_LOGW(TAG, "Frame too short: %d bytes", num_bytes);
        return ESP_FAIL;
    }

    // Cap at maximum
    if (num_bytes > HITACHI_MAX_BYTES) {
        num_bytes = HITACHI_MAX_BYTES;
    }

    // Decode data
    uint8_t data[HITACHI_MAX_BYTES] = {0};

    for (uint8_t byte_idx = 0; byte_idx < num_bytes; byte_idx++) {
        uint8_t byte_val = 0;

        for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++) {
            size_t symbol_idx = 1 + (byte_idx * 8) + bit_idx;
            if (symbol_idx >= num_symbols) {
                break;  // Safety check
            }

            const rmt_symbol_word_t *sym = &symbols[symbol_idx];

            if (!ir_match_mark(sym, HITACHI_BIT_MARK, 0)) {
                return ESP_FAIL;
            }

            if (ir_match_space(sym, HITACHI_ONE_SPACE, 0)) {
                byte_val |= (1 << bit_idx);  // LSB first
            } else if (!ir_match_space(sym, HITACHI_ZERO_SPACE, 0)) {
                return ESP_FAIL;
            }
        }

        data[byte_idx] = byte_val;
    }

    // Validate checksum
    uint8_t calculated_checksum = hitachi_checksum(data, num_bytes);
    uint8_t received_checksum = data[num_bytes - 1];
    bool checksum_ok = (calculated_checksum == received_checksum);

    if (!checksum_ok) {
        ESP_LOGW(TAG, "Checksum failed: expected 0x%02X, got 0x%02X",
                 received_checksum, calculated_checksum);
        // Continue anyway - some variants may differ
    }

    // Fill code structure
    code->protocol = IR_PROTOCOL_HITACHI;
    code->bits = num_bytes * 8;
    code->address = data[0];  // First byte typically indicates model
    code->command = data[11 < num_bytes ? 11 : 1];  // Mode/temp byte
    code->flags = 0;

    // Store first 4 bytes in data field
    code->data = (uint32_t)data[0] |
                 ((uint32_t)data[1] << 8) |
                 ((uint32_t)data[2] << 16) |
                 ((uint32_t)data[3] << 24);

    ESP_LOGI(TAG, "Decoded Hitachi AC: Length=%d bytes, Checksum=%s",
             num_bytes, checksum_ok ? "OK" : "FAIL");

    return ESP_OK;
}
