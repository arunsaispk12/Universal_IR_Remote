/**
 * @file ir_daikin.c
 * @brief Daikin AC Protocol Decoder Implementation
 */

#include "ir_daikin.h"
#include "ir_timing.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "IR_DAIKIN";

/**
 * @brief Calculate Daikin checksum for a frame
 */
static uint8_t daikin_checksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum & 0xFF;
}

/**
 * @brief Decode a single Daikin frame
 */
static esp_err_t decode_daikin_frame(const rmt_symbol_word_t *symbols, size_t *idx,
                                     uint8_t *data, uint8_t num_bytes) {
    // Check header
    if (!ir_match_mark(&symbols[*idx], DAIKIN_HEADER_MARK, 0) ||
        !ir_match_space(&symbols[*idx], DAIKIN_HEADER_SPACE, 0)) {
        return ESP_FAIL;
    }
    (*idx)++;

    // Decode bytes
    for (uint8_t byte_idx = 0; byte_idx < num_bytes; byte_idx++) {
        uint8_t byte_val = 0;

        for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++) {
            const rmt_symbol_word_t *sym = &symbols[*idx];

            if (!ir_match_mark(sym, DAIKIN_BIT_MARK, 0)) {
                return ESP_FAIL;
            }

            if (ir_match_space(sym, DAIKIN_ONE_SPACE, 0)) {
                byte_val |= (1 << bit_idx);  // LSB first
            } else if (!ir_match_space(sym, DAIKIN_ZERO_SPACE, 0)) {
                return ESP_FAIL;
            }

            (*idx)++;
        }

        data[byte_idx] = byte_val;
    }

    return ESP_OK;
}

esp_err_t ir_decode_daikin(const rmt_symbol_word_t *symbols, size_t num_symbols, ir_code_t *code) {
    if (!symbols || !code || num_symbols < 250) {  // Minimum symbols for 2-frame Daikin
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t frame1[DAIKIN_FRAME1_BYTES] = {0};
    uint8_t frame2[DAIKIN_FRAME2_BYTES] = {0};
    size_t idx = 0;

    // Decode Frame 1 (8 bytes)
    if (decode_daikin_frame(symbols, &idx, frame1, DAIKIN_FRAME1_BYTES) != ESP_OK) {
        return ESP_FAIL;
    }

    // Validate Frame 1 checksum
    uint8_t checksum1 = daikin_checksum(frame1, DAIKIN_FRAME1_BYTES - 1);
    if (checksum1 != frame1[DAIKIN_FRAME1_BYTES - 1]) {
        ESP_LOGW(TAG, "Frame 1 checksum failed");
    }

    // Check for gap (29ms space)
    if (idx < num_symbols && ir_match_space(&symbols[idx], DAIKIN_GAP, 10)) {
        idx++;
    }

    // Decode Frame 2 (19 bytes)
    if (decode_daikin_frame(symbols, &idx, frame2, DAIKIN_FRAME2_BYTES) != ESP_OK) {
        return ESP_FAIL;
    }

    // Validate Frame 2 checksum
    uint8_t checksum2 = daikin_checksum(frame2, DAIKIN_FRAME2_BYTES - 1);
    if (checksum2 != frame2[DAIKIN_FRAME2_BYTES - 1]) {
        ESP_LOGW(TAG, "Frame 2 checksum failed");
    }

    // Fill code structure
    code->protocol = IR_PROTOCOL_DAIKIN;
    code->bits = DAIKIN_TOTAL_BITS;
    code->address = 0x11;  // Daikin manufacturer code
    code->command = frame2[5];  // Mode/temp byte
    code->flags = 0;

    // Store first 4 bytes of frame2 in data field
    code->data = (uint32_t)frame2[0] |
                 ((uint32_t)frame2[1] << 8) |
                 ((uint32_t)frame2[2] << 16) |
                 ((uint32_t)frame2[3] << 24);

    ESP_LOGI(TAG, "Decoded Daikin AC: Mode=0x%02X, Frame1_CS=%s, Frame2_CS=%s",
             frame2[5],
             (checksum1 == frame1[DAIKIN_FRAME1_BYTES - 1]) ? "OK" : "FAIL",
             (checksum2 == frame2[DAIKIN_FRAME2_BYTES - 1]) ? "OK" : "FAIL");

    return ESP_OK;
}
