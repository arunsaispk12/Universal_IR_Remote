/**
 * @file ir_whynter.c
 * @brief Whynter Protocol Decoder Implementation
 */

#include "ir_whynter.h"
#include "ir_timing.h"
#include "esp_log.h"

static const char *TAG = "IR_WHYNTER";

esp_err_t ir_decode_whynter(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < 33) return ESP_ERR_INVALID_ARG;

    if (!ir_match_mark(&symbols[0], WHYNTER_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], WHYNTER_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }

    uint32_t decoded_data = 0;
    for (int i = WHYNTER_BITS - 1; i >= 0; i--) {  // MSB first
        const rmt_symbol_word_t *sym = &symbols[WHYNTER_BITS - i];
        decoded_data = (decoded_data << 1) | (ir_match_space(sym, WHYNTER_ONE_SPACE, 0) ? 1 : 0);
    }

    code->protocol = IR_PROTOCOL_WHYNTER;
    code->data = decoded_data;
    code->bits = WHYNTER_BITS;
    code->flags = IR_FLAG_MSB_FIRST;

    ESP_LOGI(TAG, "Decoded Whynter: 0x%08lX", decoded_data);
    return ESP_OK;
}
