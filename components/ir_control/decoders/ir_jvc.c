/**
 * @file ir_jvc.c
 * @brief JVC Protocol Decoder Implementation
 *
 * Based on Arduino-IRremote library
 * MIT License
 */

#include "ir_jvc.h"
#include "ir_timing.h"
#include "esp_log.h"

static const char *TAG = "IR_JVC";

esp_err_t ir_decode_jvc(const rmt_symbol_word_t *symbols,
                         size_t num_symbols,
                         ir_code_t *code) {
    if (symbols == NULL || code == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    bool has_header = false;
    size_t data_start = 0;

    // Check for header (8400us mark + 4200us space)
    if (num_symbols >= 17) {
        if (ir_match_mark(&symbols[0], JVC_HEADER_MARK, 0) &&
            ir_match_space(&symbols[0], JVC_HEADER_SPACE, 0)) {
            has_header = true;
            data_start = 1;
        }
    }

    // Expect 16 data bits (with or without header)
    size_t expected_symbols = has_header ? 17 : 16;
    if (num_symbols < expected_symbols) {
        ESP_LOGD(TAG, "Invalid symbol count: %zu", num_symbols);
        return ESP_ERR_INVALID_ARG;
    }

    // Decode 16 bits (LSB first, pulse distance)
    uint16_t decoded_data = 0;
    for (uint_fast8_t i = 0; i < JVC_BITS; i++) {
        const rmt_symbol_word_t *sym = &symbols[data_start + i];

        if (!ir_match_mark(sym, JVC_BIT_MARK, 0)) {
            ESP_LOGD(TAG, "Mark mismatch at bit %u", i);
            return ESP_FAIL;
        }

        if (ir_match_space(sym, JVC_ONE_SPACE, 0)) {
            decoded_data |= (1U << i);
        } else if (!ir_match_space(sym, JVC_ZERO_SPACE, 0)) {
            ESP_LOGD(TAG, "Space mismatch at bit %u", i);
            return ESP_FAIL;
        }
    }

    code->protocol = IR_PROTOCOL_JVC;
    code->data = decoded_data;
    code->bits = JVC_BITS;
    code->address = decoded_data & 0xFF;
    code->command = (decoded_data >> 8) & 0xFF;
    code->flags = has_header ? 0 : IR_FLAG_REPEAT;

    ESP_LOGI(TAG, "Decoded JVC%s: Addr=0x%02X, Cmd=0x%02X",
             has_header ? "" : " (repeat)", code->address, code->command);

    return ESP_OK;
}
