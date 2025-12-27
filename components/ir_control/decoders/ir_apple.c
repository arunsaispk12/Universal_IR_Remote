/**
 * @file ir_apple.c
 * @brief Apple Protocol Decoder Implementation (NEC variant)
 */

#include "ir_apple.h"
#include "ir_timing.h"
#include "esp_log.h"

static const char *TAG = "IR_APPLE";

esp_err_t ir_decode_apple(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < 33) return ESP_ERR_INVALID_ARG;

    if (!ir_match_mark(&symbols[0], APPLE_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], APPLE_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }

    uint32_t decoded_data = 0;
    for (uint_fast8_t i = 0; i < APPLE_BITS; i++) {
        const rmt_symbol_word_t *sym = &symbols[i + 1];
        if (!ir_match_mark(sym, APPLE_BIT_MARK, 0)) return ESP_FAIL;

        if (ir_match_space(sym, APPLE_ONE_SPACE, 0)) {
            decoded_data |= (1UL << i);
        } else if (!ir_match_space(sym, APPLE_ZERO_SPACE, 0)) {
            return ESP_FAIL;
        }
    }

    uint16_t address = decoded_data & 0xFFFF;
    if (address != APPLE_ADDRESS) {
        return ESP_FAIL;  // Not Apple protocol
    }

    code->protocol = IR_PROTOCOL_APPLE;
    code->data = decoded_data;
    code->bits = APPLE_BITS;
    code->address = address;
    code->command = (decoded_data >> 16) & 0xFF;
    code->flags = 0;

    ESP_LOGI(TAG, "Decoded Apple: Cmd=0x%02X", code->command);
    return ESP_OK;
}
