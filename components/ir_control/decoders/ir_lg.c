/**
 * @file ir_lg.c
 * @brief LG Protocol Decoder Implementation
 *
 * Based on Arduino-IRremote library
 * MIT License
 */

#include "ir_lg.h"
#include "ir_timing.h"
#include "esp_log.h"

static const char *TAG = "IR_LG";

esp_err_t ir_decode_lg(const rmt_symbol_word_t *symbols,
                        size_t num_symbols,
                        ir_code_t *code) {
    if (symbols == NULL || code == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Expect header + 28 data bits + stop = 30 symbols minimum
    if (num_symbols < 29) {
        ESP_LOGD(TAG, "Too few symbols: %zu", num_symbols);
        return ESP_ERR_INVALID_ARG;
    }

    // Validate header
    if (!ir_match_mark(&symbols[0], LG_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[0], LG_HEADER_SPACE, 0)) {
        ESP_LOGD(TAG, "Header mismatch");
        return ESP_FAIL;
    }

    // Decode 28 bits (LSB first, pulse distance)
    uint32_t decoded_data = 0;
    for (uint_fast8_t i = 0; i < LG_BITS; i++) {
        const rmt_symbol_word_t *sym = &symbols[i + 1];

        if (!ir_match_mark(sym, LG_BIT_MARK, 0)) {
            ESP_LOGD(TAG, "Mark mismatch at bit %u", i);
            return ESP_FAIL;
        }

        if (ir_match_space(sym, LG_ONE_SPACE, 0)) {
            decoded_data |= (1UL << i);
        } else if (!ir_match_space(sym, LG_ZERO_SPACE, 0)) {
            ESP_LOGD(TAG, "Space mismatch at bit %u", i);
            return ESP_FAIL;
        }
    }

    // Extract fields
    uint8_t address = decoded_data & 0xFF;
    uint16_t command = (decoded_data >> 8) & 0xFFFF;
    uint8_t checksum_received = (decoded_data >> 24) & 0x0F;

    // Validate checksum (sum of nibbles)
    uint8_t checksum_calc = 0;
    checksum_calc += (address & 0x0F);
    checksum_calc += (address >> 4);
    checksum_calc += (command & 0x0F);
    checksum_calc += ((command >> 4) & 0x0F);
    checksum_calc += ((command >> 8) & 0x0F);
    checksum_calc += ((command >> 12) & 0x0F);
    checksum_calc &= 0x0F;

    code->protocol = IR_PROTOCOL_LG;
    code->data = decoded_data;
    code->bits = LG_BITS;
    code->address = address;
    code->command = command;
    code->flags = (checksum_received != checksum_calc) ? IR_FLAG_PARITY_FAILED : 0;

    if (checksum_received != checksum_calc) {
        ESP_LOGW(TAG, "Checksum mismatch: got 0x%X, calc 0x%X",
                 checksum_received, checksum_calc);
    }

    ESP_LOGI(TAG, "Decoded LG: Addr=0x%02X, Cmd=0x%04X%s",
             address, command, (code->flags & IR_FLAG_PARITY_FAILED) ? " (BAD CHECKSUM)" : "");

    return ESP_OK;
}
