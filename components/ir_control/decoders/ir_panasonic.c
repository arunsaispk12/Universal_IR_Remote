/**
 * @file ir_panasonic.c
 * @brief Panasonic/Kaseikyo Protocol Decoder Implementation
 */

#include "ir_panasonic.h"
#include "ir_timing.h"
#include "esp_log.h"

static const char *TAG = "IR_PANASONIC";

esp_err_t ir_decode_panasonic(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < 49) return ESP_ERR_INVALID_ARG;

    if (!ir_match_mark(&symbols[0], PANASONIC_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], PANASONIC_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }

    uint64_t decoded_data = 0;
    for (uint_fast8_t i = 0; i < PANASONIC_BITS; i++) {
        const rmt_symbol_word_t *sym = &symbols[i + 1];
        if (!ir_match_mark(sym, PANASONIC_BIT_MARK, 0)) return ESP_FAIL;

        if (ir_match_space(sym, PANASONIC_ONE_SPACE, 0)) {
            decoded_data |= (1ULL << i);
        } else if (!ir_match_space(sym, PANASONIC_ZERO_SPACE, 0)) {
            return ESP_FAIL;
        }
    }

    code->protocol = IR_PROTOCOL_PANASONIC;
    code->data = (uint32_t)(decoded_data & 0xFFFFFFFF);
    code->bits = PANASONIC_BITS;
    code->address = (decoded_data >> 32) & 0xFFFF;
    code->command = decoded_data & 0xFFFF;
    code->flags = 0;

    ESP_LOGI(TAG, "Decoded Panasonic: 48-bit data");
    return ESP_OK;
}
