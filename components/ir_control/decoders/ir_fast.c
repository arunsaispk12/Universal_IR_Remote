/**
 * @file ir_fast.c
 * @brief FAST Protocol Decoder Implementation
 */

#include "ir_fast.h"
#include "ir_timing.h"
#include "esp_log.h"

static const char *TAG = "IR_FAST";

esp_err_t ir_decode_fast(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < FAST_BITS) return ESP_ERR_INVALID_ARG;

    uint8_t decoded_data = 0;
    for (uint_fast8_t i = 0; i < FAST_BITS; i++) {
        const rmt_symbol_word_t *sym = &symbols[i];
        if (!ir_match_mark(sym, FAST_BIT_MARK, 0)) return ESP_FAIL;

        if (ir_match_space(sym, FAST_ONE_SPACE, 0)) {
            decoded_data |= (1U << i);
        } else if (!ir_match_space(sym, FAST_ZERO_SPACE, 0)) {
            return ESP_FAIL;
        }
    }

    code->protocol = IR_PROTOCOL_FAST;
    code->data = decoded_data;
    code->bits = FAST_BITS;
    code->flags = 0;

    ESP_LOGI(TAG, "Decoded FAST: 0x%02X", decoded_data);
    return ESP_OK;
}
