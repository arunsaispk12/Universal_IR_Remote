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

    // Reject NEC-like headers (9000us + 4500us) - these should be decoded by NEC decoder
    // This prevents false JVC matches on NEC signals
    if (num_symbols > 16) {
        uint16_t first_mark = symbols[0].duration0;
        uint16_t first_space = symbols[0].duration1;

        // Check if this looks like NEC header (9000us +/- 25%, 4500us +/- 25%)
        if (first_mark > 6750 && first_mark < 11250 &&  // 9000 +/- 25%
            first_space > 3375 && first_space < 5625) {  // 4500 +/- 25%
            ESP_LOGD(TAG, "Rejecting NEC-like header: %uus + %uus", first_mark, first_space);
            return ESP_ERR_NOT_SUPPORTED;
        }
    }

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

    // Reject if symbol count is too high (likely NEC or other protocol)
    // JVC should have exactly 16 (repeat) or 17 (with header) symbols
    if (num_symbols > expected_symbols + 2) {  // Allow 2 symbols tolerance for trailing pulses
        ESP_LOGD(TAG, "Too many symbols for JVC: %zu (expected %zu)", num_symbols, expected_symbols);
        return ESP_ERR_INVALID_ARG;
    }

    // Additional validation: If we don't have a header, require that this looks like a repeat
    // Reject signals without header that have too many symbols (likely not JVC repeat)
    if (!has_header && num_symbols > 18) {
        ESP_LOGD(TAG, "Headerless signal with too many symbols: %zu (likely not JVC repeat)", num_symbols);
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
