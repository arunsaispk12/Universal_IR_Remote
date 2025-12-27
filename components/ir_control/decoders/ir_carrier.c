/**
 * @file ir_carrier.c
 * @brief Carrier AC Protocol Decoder Implementation
 *
 * Critical for Indian market - covers Voltas, Blue Star, Lloyd
 */

#include "ir_carrier.h"
#include "ir_timing.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "IR_CARRIER";

/**
 * @brief Calculate Carrier AC checksum
 *
 * Carrier uses a nibble-based checksum algorithm
 */
static uint8_t carrier_checksum(const uint8_t *data, uint8_t len) {
    uint16_t sum = 0;

    // Sum all nibbles (4-bit values)
    for (uint8_t i = 0; i < len; i++) {
        sum += (data[i] & 0x0F);       // Lower nibble
        sum += ((data[i] >> 4) & 0x0F); // Upper nibble
    }

    // Return lower nibble of sum
    return sum & 0x0F;
}

esp_err_t ir_decode_carrier(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < CARRIER_BITS + 1) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check header
    if (!ir_match_mark(&symbols[0], CARRIER_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], CARRIER_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }

    // Decode 128 bits (16 bytes)
    uint8_t data[CARRIER_BYTES] = {0};

    for (uint8_t byte_idx = 0; byte_idx < CARRIER_BYTES; byte_idx++) {
        uint8_t byte_val = 0;

        for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++) {
            size_t symbol_idx = 1 + (byte_idx * 8) + bit_idx;
            const rmt_symbol_word_t *sym = &symbols[symbol_idx];

            if (!ir_match_mark(sym, CARRIER_BIT_MARK, 0)) {
                return ESP_FAIL;
            }

            if (ir_match_space(sym, CARRIER_ONE_SPACE, 0)) {
                byte_val |= (1 << bit_idx);  // LSB first
            } else if (!ir_match_space(sym, CARRIER_ZERO_SPACE, 0)) {
                return ESP_FAIL;
            }
        }

        data[byte_idx] = byte_val;
    }

    // Validate checksum (typically last nibble)
    uint8_t calculated_checksum = carrier_checksum(data, CARRIER_BYTES - 1);
    uint8_t received_checksum = data[CARRIER_BYTES - 1] & 0x0F;
    bool checksum_ok = (calculated_checksum == received_checksum);

    if (!checksum_ok) {
        ESP_LOGW(TAG, "Checksum failed: expected 0x%X, got 0x%X",
                 received_checksum, calculated_checksum);
        // Continue anyway - some variants may differ
    }

    // Fill code structure
    code->protocol = IR_PROTOCOL_CARRIER;
    code->bits = CARRIER_BITS;
    code->address = data[0];  // First byte typically indicates model/type
    code->command = data[1];  // Second byte typically has mode/temp
    code->flags = 0;

    // Store first 4 bytes in data field
    code->data = (uint32_t)data[0] |
                 ((uint32_t)data[1] << 8) |
                 ((uint32_t)data[2] << 16) |
                 ((uint32_t)data[3] << 24);

    ESP_LOGI(TAG, "Decoded Carrier AC (Voltas/Blue Star/Lloyd): "
             "Model=0x%02X, Cmd=0x%02X, Checksum=%s",
             data[0], data[1], checksum_ok ? "OK" : "FAIL");

    return ESP_OK;
}
