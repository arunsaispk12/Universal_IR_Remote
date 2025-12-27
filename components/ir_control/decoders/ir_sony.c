/**
 * @file ir_sony.c
 * @brief Sony SIRC Protocol Decoder Implementation
 *
 * Decodes Sony SIRC (Sony Infrared Remote Control) protocol variants.
 * Sony is the only major protocol using pulse WIDTH encoding with 40kHz carrier.
 *
 * Based on Arduino-IRremote library
 * MIT License
 */

#include "ir_sony.h"
#include "ir_timing.h"
#include "esp_log.h"

static const char *TAG = "IR_SONY";

esp_err_t ir_decode_sony(const rmt_symbol_word_t *symbols,
                          size_t num_symbols,
                          ir_code_t *code) {
    if (symbols == NULL || code == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Determine Sony variant based on number of symbols
     * Sony format: header (1 symbol) + N data bits (N symbols)
     * - SIRC-12: 1 header + 12 data = 13 symbols
     * - SIRC-15: 1 header + 15 data = 16 symbols
     * - SIRC-20: 1 header + 20 data = 21 symbols
     */
    uint8_t num_bits;
    if (num_symbols == 13) {
        num_bits = SONY_BITS_12;
    } else if (num_symbols == 16) {
        num_bits = SONY_BITS_15;
    } else if (num_symbols == 21) {
        num_bits = SONY_BITS_20;
    } else {
        ESP_LOGD(TAG, "Invalid symbol count: %zu (expected 13, 16, or 21)", num_symbols);
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Validate header timing
     * Header mark: 2400us, Header space: 600us
     */
    if (!ir_match_mark(&symbols[0], SONY_HEADER_MARK, 0)) {
        ESP_LOGD(TAG, "Header mark mismatch: %u us (expected %u us)",
                 ir_get_mark_us(&symbols[0]), SONY_HEADER_MARK);
        return ESP_FAIL;
    }

    if (!ir_match_space(&symbols[0], SONY_HEADER_SPACE, 0)) {
        ESP_LOGD(TAG, "Header space mismatch: %u us (expected %u us)",
                 ir_get_space_us(&symbols[0]), SONY_HEADER_SPACE);
        return ESP_FAIL;
    }

    /*
     * Decode data bits (pulse WIDTH encoding)
     * Sony uses pulse width for bit encoding:
     * - "0" bit: 600us mark + 600us space
     * - "1" bit: 1200us mark + 600us space
     *
     * NOTE: Sony has NO stop bit (unique among major protocols)
     */
    uint32_t decoded_data = 0;

    for (uint_fast8_t i = 0; i < num_bits; i++) {
        const rmt_symbol_word_t *sym = &symbols[i + 1];  // Skip header

        // Validate space (should always be ~600us)
        if (!ir_match_space(sym, SONY_SPACE, 0)) {
            ESP_LOGD(TAG, "Space mismatch at bit %u: %u us (expected %u us)",
                     i, ir_get_space_us(sym), SONY_SPACE);
            return ESP_FAIL;
        }

        // Decode bit from mark duration (pulse WIDTH encoding)
        uint16_t mark_us = ir_get_mark_us(sym);

        bool bit_value;
        if (ir_timing_matches(mark_us, SONY_ONE_MARK)) {
            bit_value = true;   // 1200us = "1"
        } else if (ir_timing_matches(mark_us, SONY_ZERO_MARK)) {
            bit_value = false;  // 600us = "0"
        } else {
            ESP_LOGD(TAG, "Mark mismatch at bit %u: %u us (expected %u or %u us)",
                     i, mark_us, SONY_ZERO_MARK, SONY_ONE_MARK);
            return ESP_FAIL;
        }

        // Pack bit LSB first
        if (bit_value) {
            decoded_data |= (1UL << i);
        }
    }

    /*
     * Extract command and address from decoded data
     * Sony format (LSB first):
     * - Bits 0-6: Command (7 bits)
     * - Bits 7+:  Address (5, 8, or 13 bits depending on variant)
     */
    uint8_t command = decoded_data & 0x7F;          // First 7 bits
    uint16_t address = decoded_data >> 7;           // Remaining bits

    /*
     * Fill result structure
     */
    code->protocol = IR_PROTOCOL_SONY;
    code->data = decoded_data;
    code->bits = num_bits;
    code->command = command;
    code->address = address;
    code->flags = 0;  // LSB first by default
    code->raw_data = NULL;
    code->raw_length = 0;

    ESP_LOGI(TAG, "Decoded Sony-%u: Command=0x%02X, Address=0x%04X, Data=0x%08lX",
             num_bits, command, address, decoded_data);

    return ESP_OK;
}
