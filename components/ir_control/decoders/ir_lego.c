/**
 * @file ir_lego.c
 * @brief Lego Power Functions Protocol Decoder Implementation
 */

#include "ir_lego.h"
#include "ir_timing.h"
#include "esp_log.h"

static const char *TAG = "IR_LEGO";

esp_err_t ir_decode_lego(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < 17) return ESP_ERR_INVALID_ARG;

    if (!ir_match_mark(&symbols[0], LEGO_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], LEGO_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }

    uint16_t decoded_data = 0;
    for (int i = LEGO_BITS - 1; i >= 0; i--) {  // MSB first
        const rmt_symbol_word_t *sym = &symbols[LEGO_BITS - i];
        decoded_data = (decoded_data << 1) | (ir_match_space(sym, LEGO_ONE_SPACE, 0) ? 1 : 0);
    }

    code->protocol = IR_PROTOCOL_LEGO_PF;
    code->data = decoded_data;
    code->bits = LEGO_BITS;
    code->flags = IR_FLAG_MSB_FIRST;

    ESP_LOGI(TAG, "Decoded Lego: 0x%04X", decoded_data);
    return ESP_OK;
}
