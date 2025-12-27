/**
 * @file ir_samsung48.c
 * @brief Samsung 48-bit Protocol Decoder Implementation
 */

#include "ir_samsung48.h"
#include "ir_timing.h"
#include "esp_log.h"

static const char *TAG = "IR_SAMSUNG48";

esp_err_t ir_decode_samsung48(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < 49) return ESP_ERR_INVALID_ARG;

    if (!ir_match_mark(&symbols[0], SAMSUNG48_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], SAMSUNG48_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }

    uint64_t decoded_data = 0;
    for (uint_fast8_t i = 0; i < SAMSUNG48_BITS; i++) {
        const rmt_symbol_word_t *sym = &symbols[i + 1];
        if (!ir_match_mark(sym, SAMSUNG48_BIT_MARK, 0)) return ESP_FAIL;

        if (ir_match_space(sym, SAMSUNG48_ONE_SPACE, 0)) {
            decoded_data |= (1ULL << i);
        } else if (!ir_match_space(sym, SAMSUNG48_ZERO_SPACE, 0)) {
            return ESP_FAIL;
        }
    }

    code->protocol = IR_PROTOCOL_SAMSUNG48;
    code->data = (uint32_t)(decoded_data & 0xFFFFFFFF);
    code->bits = SAMSUNG48_BITS;
    code->address = (decoded_data >> 32) & 0xFFFF;
    code->command = decoded_data & 0xFFFF;
    code->flags = 0;

    ESP_LOGI(TAG, "Decoded Samsung48");
    return ESP_OK;
}
