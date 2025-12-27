/**
 * @file ir_magiquest.c
 * @brief MagiQuest Protocol Decoder Implementation
 */

#include "ir_magiquest.h"
#include "ir_timing.h"
#include "esp_log.h"

static const char *TAG = "IR_MAGIQUEST";

esp_err_t ir_decode_magiquest(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < MAGIQUEST_BITS) return ESP_ERR_INVALID_ARG;

    uint64_t decoded_data = 0;
    for (int i = MAGIQUEST_BITS - 1; i >= 0; i--) {  // MSB first
        const rmt_symbol_word_t *sym = &symbols[MAGIQUEST_BITS - 1 - i];
        if (!ir_match_mark(sym, MAGIQUEST_BIT_MARK, 0)) return ESP_FAIL;
        decoded_data = (decoded_data << 1) | (ir_match_space(sym, MAGIQUEST_ONE_SPACE, 0) ? 1 : 0);
    }

    code->protocol = IR_PROTOCOL_MAGIQUEST;
    code->data = (uint32_t)(decoded_data & 0xFFFFFFFF);
    code->bits = MAGIQUEST_BITS;
    code->flags = IR_FLAG_MSB_FIRST;

    ESP_LOGI(TAG, "Decoded MagiQuest: 56 bits");
    return ESP_OK;
}
