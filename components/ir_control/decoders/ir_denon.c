/**
 * @file ir_denon.c
 * @brief Denon/Sharp Protocol Decoder Implementation
 *
 * Based on Arduino-IRremote library
 * MIT License
 */

#include "ir_denon.h"
#include "ir_timing.h"
#include "esp_log.h"

static const char *TAG = "IR_DENON";

esp_err_t ir_decode_denon(const rmt_symbol_word_t *symbols,
                           size_t num_symbols,
                           ir_code_t *code) {
    if (symbols == NULL || code == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (num_symbols < 16) {
        return ESP_ERR_INVALID_ARG;
    }

    // Validate header
    if (!ir_match_mark(&symbols[0], DENON_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], DENON_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }

    // Decode 15 bits
    uint16_t decoded_data = 0;
    for (uint_fast8_t i = 0; i < DENON_BITS; i++) {
        const rmt_symbol_word_t *sym = &symbols[i + 1];

        if (!ir_match_mark(sym, DENON_BIT_MARK, 0)) {
            return ESP_FAIL;
        }

        if (ir_match_space(sym, DENON_ONE_SPACE, 0)) {
            decoded_data |= (1U << i);
        } else if (!ir_match_space(sym, DENON_ZERO_SPACE, 0)) {
            return ESP_FAIL;
        }
    }

    code->protocol = IR_PROTOCOL_DENON;
    code->data = decoded_data;
    code->bits = DENON_BITS;
    code->address = (decoded_data & 0x1F);         // 5 bits
    code->command = (decoded_data >> 5) & 0xFF;    // 8 bits
    code->flags = 0;

    ESP_LOGI(TAG, "Decoded Denon: Addr=0x%02X, Cmd=0x%02X",
             code->address, code->command);

    return ESP_OK;
}
