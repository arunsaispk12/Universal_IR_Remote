/**
 * @file ir_ac_state.c
 * @brief Air Conditioner State-Based Control Implementation
 *
 * This implements the state-based AC control model where the firmware maintains
 * complete AC state and regenerates full IR frames on any parameter change.
 *
 * Copyright (c) 2025
 */

#include "ir_ac_state.h"
#include "ir_control.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "ir_ac_state";

/* NVS namespace for AC state */
#define NVS_NAMESPACE_AC        "ir_ac"
#define NVS_KEY_AC_STATE        "state"

/* Internal state */
static bool is_initialized = false;
static nvs_handle_t nvs_handle_ac = 0;
static ac_state_t current_state = {0};

/* Forward declarations for protocol encoders */
extern esp_err_t ir_ac_encode_daikin(const ac_state_t *state, ir_code_t *code);
extern esp_err_t ir_ac_encode_carrier(const ac_state_t *state, ir_code_t *code);
extern esp_err_t ir_ac_encode_hitachi(const ac_state_t *state, ir_code_t *code);
extern esp_err_t ir_ac_encode_mitsubishi(const ac_state_t *state, ir_code_t *code);
extern esp_err_t ir_ac_encode_fujitsu(const ac_state_t *state, ir_code_t *code);
extern esp_err_t ir_ac_encode_haier(const ac_state_t *state, ir_code_t *code);
extern esp_err_t ir_ac_encode_midea(const ac_state_t *state, ir_code_t *code);
extern esp_err_t ir_ac_encode_samsung48(const ac_state_t *state, ir_code_t *code);
extern esp_err_t ir_ac_encode_panasonic(const ac_state_t *state, ir_code_t *code);
extern esp_err_t ir_ac_encode_lg2(const ac_state_t *state, ir_code_t *code);

/* ============================================================================
 * NAME TABLES
 * ============================================================================ */

static const char* mode_names[] = {
    [AC_MODE_OFF]  = "Off",
    [AC_MODE_AUTO] = "Auto",
    [AC_MODE_COOL] = "Cool",
    [AC_MODE_HEAT] = "Heat",
    [AC_MODE_DRY]  = "Dry",
    [AC_MODE_FAN]  = "Fan",
};

static const char* fan_speed_names[] = {
    [AC_FAN_AUTO]   = "Auto",
    [AC_FAN_LOW]    = "Low",
    [AC_FAN_MEDIUM] = "Medium",
    [AC_FAN_HIGH]   = "High",
    [AC_FAN_QUIET]  = "Quiet",
    [AC_FAN_TURBO]  = "Turbo",
};

static const char* swing_names[] = {
    [AC_SWING_OFF]        = "Off",
    [AC_SWING_VERTICAL]   = "Vertical",
    [AC_SWING_HORIZONTAL] = "Horizontal",
    [AC_SWING_BOTH]       = "Both",
    [AC_SWING_AUTO]       = "Auto",
};

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

esp_err_t ir_ac_state_init(void)
{
    if (is_initialized) {
        ESP_LOGW(TAG, "AC state system already initialized");
        return ESP_OK;
    }

    /* Initialize NVS partition if needed (shared with ir_action) */
    esp_err_t err = nvs_flash_init_partition("ir_storage");
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "ir_storage partition needs to be erased, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase_partition("ir_storage"));
        err = nvs_flash_init_partition("ir_storage");
    }
    /* Ignore if already initialized by ir_action */

    /* Open NVS namespace for AC state from dedicated ir_storage partition */
    err = nvs_open_from_partition("ir_storage", NVS_NAMESPACE_AC, NVS_READWRITE, &nvs_handle_ac);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace from ir_storage partition: %s", esp_err_to_name(err));
        return err;
    }

    /* Initialize with default state */
    ir_ac_get_default_state(&current_state);

    /* Try to load saved state */
    err = ir_ac_load_state();
    if (err == ESP_ERR_NOT_FOUND) {
        ESP_LOGI(TAG, "No saved AC state, using defaults");
    } else if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load AC state: %s", esp_err_to_name(err));
    }

    is_initialized = true;
    ESP_LOGI(TAG, "AC state system initialized (Protocol: %s, Power: %s, Mode: %s, Temp: %d°C)",
             current_state.is_learned ? ir_get_protocol_name(current_state.protocol) : "Not configured",
             current_state.power ? "ON" : "OFF",
             ir_ac_get_mode_name(current_state.mode),
             current_state.temperature);

    return ESP_OK;
}

/* ============================================================================
 * STATE GETTERS
 * ============================================================================ */

const ac_state_t* ir_ac_state_get(void)
{
    return &current_state;
}

bool ir_ac_is_configured(void)
{
    return current_state.is_learned && (current_state.protocol != IR_PROTOCOL_UNKNOWN);
}

/* ============================================================================
 * STATE SETTERS (Individual Parameters)
 * ============================================================================ */

esp_err_t ir_ac_set_power(bool power)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (current_state.power == power) {
        ESP_LOGD(TAG, "Power already %s", power ? "ON" : "OFF");
        return ESP_OK;
    }

    current_state.power = power;
    ESP_LOGI(TAG, "AC Power: %s", power ? "ON" : "OFF");

    /* Transmit state */
    esp_err_t err = ir_ac_transmit_state();
    if (err == ESP_OK) {
        ir_ac_save_state(); // Auto-save on successful transmission
    }

    return err;
}

esp_err_t ir_ac_set_mode(ac_mode_t mode)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (mode < AC_MODE_OFF || mode >= AC_MODE_MAX) {
        ESP_LOGE(TAG, "Invalid mode: %d", mode);
        return ESP_ERR_INVALID_ARG;
    }

    if (current_state.mode == mode) {
        ESP_LOGD(TAG, "Mode already %s", ir_ac_get_mode_name(mode));
        return ESP_OK;
    }

    current_state.mode = mode;
    ESP_LOGI(TAG, "AC Mode: %s", ir_ac_get_mode_name(mode));

    /* Transmit state */
    esp_err_t err = ir_ac_transmit_state();
    if (err == ESP_OK) {
        ir_ac_save_state();
    }

    return err;
}

esp_err_t ir_ac_set_temperature(uint8_t temperature)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (temperature < AC_TEMP_MIN || temperature > AC_TEMP_MAX) {
        ESP_LOGE(TAG, "Temperature out of range: %d (valid: %d-%d)",
                 temperature, AC_TEMP_MIN, AC_TEMP_MAX);
        return ESP_ERR_INVALID_ARG;
    }

    if (current_state.temperature == temperature) {
        ESP_LOGD(TAG, "Temperature already %d°C", temperature);
        return ESP_OK;
    }

    current_state.temperature = temperature;
    ESP_LOGI(TAG, "AC Temperature: %d°C", temperature);

    /* Transmit state */
    esp_err_t err = ir_ac_transmit_state();
    if (err == ESP_OK) {
        ir_ac_save_state();
    }

    return err;
}

esp_err_t ir_ac_set_fan_speed(ac_fan_speed_t fan_speed)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (fan_speed < AC_FAN_AUTO || fan_speed >= AC_FAN_MAX) {
        ESP_LOGE(TAG, "Invalid fan speed: %d", fan_speed);
        return ESP_ERR_INVALID_ARG;
    }

    if (current_state.fan_speed == fan_speed) {
        ESP_LOGD(TAG, "Fan speed already %s", ir_ac_get_fan_speed_name(fan_speed));
        return ESP_OK;
    }

    current_state.fan_speed = fan_speed;
    ESP_LOGI(TAG, "AC Fan Speed: %s", ir_ac_get_fan_speed_name(fan_speed));

    /* Transmit state */
    esp_err_t err = ir_ac_transmit_state();
    if (err == ESP_OK) {
        ir_ac_save_state();
    }

    return err;
}

esp_err_t ir_ac_set_swing(ac_swing_t swing)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (swing < AC_SWING_OFF || swing >= AC_SWING_MAX) {
        ESP_LOGE(TAG, "Invalid swing mode: %d", swing);
        return ESP_ERR_INVALID_ARG;
    }

    if (current_state.swing == swing) {
        ESP_LOGD(TAG, "Swing already %s", ir_ac_get_swing_name(swing));
        return ESP_OK;
    }

    current_state.swing = swing;
    ESP_LOGI(TAG, "AC Swing: %s", ir_ac_get_swing_name(swing));

    /* Transmit state */
    esp_err_t err = ir_ac_transmit_state();
    if (err == ESP_OK) {
        ir_ac_save_state();
    }

    return err;
}

esp_err_t ir_ac_set_state(const ac_state_t *state)
{
    if (!is_initialized || !state) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Validate state */
    esp_err_t err = ir_ac_validate_state(state);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Invalid AC state");
        return err;
    }

    /* Update current state */
    memcpy(&current_state, state, sizeof(ac_state_t));

    ESP_LOGI(TAG, "AC State updated: Power=%s, Mode=%s, Temp=%d°C, Fan=%s, Swing=%s",
             current_state.power ? "ON" : "OFF",
             ir_ac_get_mode_name(current_state.mode),
             current_state.temperature,
             ir_ac_get_fan_speed_name(current_state.fan_speed),
             ir_ac_get_swing_name(current_state.swing));

    /* Transmit state */
    err = ir_ac_transmit_state();
    if (err == ESP_OK) {
        ir_ac_save_state();
    }

    return err;
}

esp_err_t ir_ac_set_protocol(ir_protocol_t protocol, uint8_t variant)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Verify it's an AC protocol */
    bool is_ac_protocol = (protocol == IR_PROTOCOL_DAIKIN ||
                            protocol == IR_PROTOCOL_CARRIER ||
                            protocol == IR_PROTOCOL_HITACHI ||
                            protocol == IR_PROTOCOL_MITSUBISHI ||
                            protocol == IR_PROTOCOL_FUJITSU ||
                            protocol == IR_PROTOCOL_HAIER ||
                            protocol == IR_PROTOCOL_MIDEA ||
                            protocol == IR_PROTOCOL_SAMSUNG48 ||
                            protocol == IR_PROTOCOL_PANASONIC ||
                            protocol == IR_PROTOCOL_LG2);

    if (!is_ac_protocol) {
        ESP_LOGE(TAG, "Protocol %d is not an AC protocol", protocol);
        return ESP_ERR_NOT_SUPPORTED;
    }

    current_state.protocol = protocol;
    current_state.protocol_variant = variant;
    current_state.is_learned = true;

    ESP_LOGI(TAG, "AC Protocol set to: %s (variant %d)",
             ir_get_protocol_name(protocol), variant);

    /* Save configuration */
    return ir_ac_save_state();
}

/* ============================================================================
 * PROTOCOL ENCODING/DECODING
 * ============================================================================ */

esp_err_t ir_ac_encode_state(const ac_state_t *state, ir_code_t *code)
{
    if (!state || !code) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!state->is_learned || state->protocol == IR_PROTOCOL_UNKNOWN) {
        ESP_LOGE(TAG, "AC protocol not configured. Please learn AC first.");
        return ESP_ERR_INVALID_STATE;
    }

    /* Validate state before encoding */
    esp_err_t err = ir_ac_validate_state(state);
    if (err != ESP_OK) {
        return err;
    }

    /* Clear code structure */
    memset(code, 0, sizeof(ir_code_t));

    /* Call protocol-specific encoder */
    switch (state->protocol) {
        case IR_PROTOCOL_DAIKIN:
            err = ir_ac_encode_daikin(state, code);
            break;

        case IR_PROTOCOL_CARRIER:
            err = ir_ac_encode_carrier(state, code);
            break;

        case IR_PROTOCOL_HITACHI:
            err = ir_ac_encode_hitachi(state, code);
            break;

        case IR_PROTOCOL_MITSUBISHI:
            err = ir_ac_encode_mitsubishi(state, code);
            break;

        case IR_PROTOCOL_FUJITSU:
            err = ir_ac_encode_fujitsu(state, code);
            break;

        case IR_PROTOCOL_HAIER:
            err = ir_ac_encode_haier(state, code);
            break;

        case IR_PROTOCOL_MIDEA:
            err = ir_ac_encode_midea(state, code);
            break;

        case IR_PROTOCOL_SAMSUNG48:
            err = ir_ac_encode_samsung48(state, code);
            break;

        case IR_PROTOCOL_PANASONIC:
        case IR_PROTOCOL_KASEIKYO:
            err = ir_ac_encode_panasonic(state, code);
            break;

        case IR_PROTOCOL_LG2:
            err = ir_ac_encode_lg2(state, code);
            break;

        default:
            ESP_LOGE(TAG, "Protocol encoder not implemented for: %s",
                     ir_get_protocol_name(state->protocol));
            return ESP_ERR_NOT_SUPPORTED;
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to encode AC state: %s", esp_err_to_name(err));
    } else {
        ESP_LOGD(TAG, "AC state encoded successfully (protocol: %s, bits: %d)",
                 ir_get_protocol_name(code->protocol), code->bits);
    }

    return err;
}

/**
 * @brief Decode RMT raw data to byte array (LSB-first)
 */
static esp_err_t decode_raw_to_bytes_lsb(const uint16_t *raw_data, size_t num_marks_spaces,
                                           uint16_t mark_us, uint16_t one_space_us, uint16_t zero_space_us,
                                           uint8_t *bytes, size_t num_bytes)
{
    if (!raw_data || !bytes || num_marks_spaces < num_bytes * 16) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t idx = 0;
    for (size_t byte_idx = 0; byte_idx < num_bytes; byte_idx++) {
        uint8_t byte = 0;
        for (uint8_t bit = 0; bit < 8; bit++) {
            /* Skip mark (should be consistent) */
            idx++;
            /* Check space duration */
            uint16_t space = raw_data[idx++];

            /* Tolerance: ±30% */
            if (space > (one_space_us * 0.7) && space < (one_space_us * 1.3)) {
                byte |= (1 << bit);  // LSB first
            }
            /* If not one, assume zero (already 0 in byte) */
        }
        bytes[byte_idx] = byte;
    }

    return ESP_OK;
}

esp_err_t ir_ac_decode_state(const ir_code_t *code, ac_state_t *state)
{
    if (!code || !state) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Initialize state to defaults */
    ir_ac_get_default_state(state);

    /* Check if protocol is an AC protocol */
    switch (code->protocol) {
        case IR_PROTOCOL_CARRIER: {
            /* Carrier/Voltas: 128-bit (16 bytes) */
            if (code->bits < 120) {
                ESP_LOGW(TAG, "Carrier frame too short: %d bits", code->bits);
                return ESP_ERR_INVALID_SIZE;
            }

            uint8_t data[16] = {0};
            if (decode_raw_to_bytes_lsb(code->raw_data, code->raw_length, 560, 1690, 560, data, 16) != ESP_OK) {
                return ESP_FAIL;
            }

            /* Extract power and mode from byte 3 */
            state->power = (data[3] & 0x01) ? true : false;
            uint8_t mode_bits = (data[3] >> 1) & 0x07;
            switch (mode_bits) {
                case 0: state->mode = AC_MODE_AUTO; break;
                case 1: state->mode = AC_MODE_COOL; break;
                case 2: state->mode = AC_MODE_DRY; break;
                case 3: state->mode = AC_MODE_FAN; break;
                case 4: state->mode = AC_MODE_HEAT; break;
                default: state->mode = AC_MODE_COOL; break;
            }

            /* Temperature (byte 4, offset encoding) */
            state->temperature = data[4] + 16;
            if (state->temperature < AC_TEMP_MIN) state->temperature = AC_TEMP_MIN;
            if (state->temperature > AC_TEMP_MAX) state->temperature = AC_TEMP_MAX;

            /* Fan speed (byte 5) */
            state->fan_speed = (ac_fan_speed_t)(data[5] & 0x03);

            /* Swing (byte 6) */
            state->swing = (data[6] & 0x01) ? AC_SWING_VERTICAL : AC_SWING_OFF;

            /* Extended features */
            state->turbo = (data[7] & 0x01) ? true : false;
            state->sleep = (data[8] & 0x01) ? true : false;
            state->econo = (data[9] & 0x01) ? true : false;

            state->protocol = IR_PROTOCOL_CARRIER;
            ESP_LOGI(TAG, "Carrier decoded: Power=%d, Mode=%d, Temp=%d°C",
                     state->power, state->mode, state->temperature);
            return ESP_OK;
        }

        case IR_PROTOCOL_DAIKIN: {
            /* Daikin: 312-bit (19 bytes main frame) */
            if (code->bits < 300) {
                ESP_LOGW(TAG, "Daikin frame too short: %d bits", code->bits);
                return ESP_ERR_INVALID_SIZE;
            }

            uint8_t data[19] = {0};
            if (decode_raw_to_bytes_lsb(code->raw_data, code->raw_length, 428, 1280, 428, data, 19) != ESP_OK) {
                return ESP_FAIL;
            }

            /* Extract power and mode from byte 5 */
            state->power = (data[5] & 0x01) ? true : false;
            uint8_t mode_bits = (data[5] >> 4) & 0x0F;
            switch (mode_bits) {
                case 0: state->mode = AC_MODE_FAN; break;
                case 2: state->mode = AC_MODE_DRY; break;
                case 3: state->mode = AC_MODE_COOL; break;
                case 4: state->mode = AC_MODE_HEAT; break;
                case 7: state->mode = AC_MODE_AUTO; break;
                default: state->mode = AC_MODE_COOL; break;
            }

            /* Temperature (byte 6, scaled by 2) */
            state->temperature = data[6] / 2;
            if (state->temperature < AC_TEMP_MIN) state->temperature = AC_TEMP_MIN;
            if (state->temperature > AC_TEMP_MAX) state->temperature = AC_TEMP_MAX;

            /* Fan speed (byte 8, upper nibble) */
            uint8_t fan_bits = (data[8] >> 4) & 0x0F;
            switch (fan_bits) {
                case 3: state->fan_speed = AC_FAN_AUTO; break;
                case 4: state->fan_speed = AC_FAN_LOW; break;
                case 5: state->fan_speed = AC_FAN_MEDIUM; break;
                case 6: state->fan_speed = AC_FAN_HIGH; break;
                case 7: state->fan_speed = AC_FAN_TURBO; break;
                default: state->fan_speed = AC_FAN_AUTO; break;
            }

            /* Swing (byte 9) */
            state->swing = (data[9] == 0xF1) ? AC_SWING_VERTICAL : AC_SWING_OFF;

            /* Advanced features (byte 13) */
            state->turbo = (data[13] & 0x01) ? true : false;
            state->quiet = (data[13] & 0x02) ? true : false;
            state->econo = (data[13] & 0x04) ? true : false;

            state->protocol = IR_PROTOCOL_DAIKIN;
            ESP_LOGI(TAG, "Daikin decoded: Power=%d, Mode=%d, Temp=%d°C",
                     state->power, state->mode, state->temperature);
            return ESP_OK;
        }

        case IR_PROTOCOL_MIDEA: {
            /* Midea: 48-bit (6 bytes) */
            if (code->bits < 40) {
                ESP_LOGW(TAG, "Midea frame too short: %d bits", code->bits);
                return ESP_ERR_INVALID_SIZE;
            }

            uint8_t data[6] = {0};
            if (decode_raw_to_bytes_lsb(code->raw_data, code->raw_length, 560, 1690, 560, data, 6) != ESP_OK) {
                return ESP_FAIL;
            }

            /* Basic decoding for Midea (simplified) */
            state->power = (data[1] & 0x20) ? true : false;
            state->mode = AC_MODE_COOL;  /* Default mode */
            state->temperature = ((data[1] & 0x0F) + 17);  /* Simplified temp extraction */
            state->protocol = IR_PROTOCOL_MIDEA;

            ESP_LOGI(TAG, "Midea decoded: Power=%d, Temp=%d°C", state->power, state->temperature);
            return ESP_OK;
        }

        case IR_PROTOCOL_LG2: {
            /* LG2: 28-bit (4 bytes) */
            if (code->bits < 24) {
                ESP_LOGW(TAG, "LG2 frame too short: %d bits", code->bits);
                return ESP_ERR_INVALID_SIZE;
            }

            uint8_t data[4] = {0};
            if (decode_raw_to_bytes_lsb(code->raw_data, code->raw_length, 560, 1690, 560, data, 4) != ESP_OK) {
                return ESP_FAIL;
            }

            /* Basic LG2 decoding (simplified) */
            state->power = true;  /* Assume ON when frame is sent */
            state->mode = AC_MODE_COOL;
            state->temperature = ((data[1] & 0x0F) + 18);  /* Simplified */
            state->protocol = IR_PROTOCOL_LG2;

            ESP_LOGI(TAG, "LG2 decoded: Temp=%d°C", state->temperature);
            return ESP_OK;
        }

        case IR_PROTOCOL_HITACHI:
        case IR_PROTOCOL_MITSUBISHI:
        case IR_PROTOCOL_HAIER:
        case IR_PROTOCOL_SAMSUNG48:
        case IR_PROTOCOL_PANASONIC:
        case IR_PROTOCOL_FUJITSU:
            /* These protocols are recognized but detailed decoding not implemented */
            ESP_LOGW(TAG, "AC decoder for %s not fully implemented - using defaults",
                     ir_get_protocol_name(code->protocol));
            state->protocol = code->protocol;
            state->power = true;  /* Assume ON */
            state->mode = AC_MODE_COOL;
            state->temperature = AC_TEMP_DEFAULT;
            return ESP_OK;

        default:
            ESP_LOGW(TAG, "Protocol %s is not an AC protocol", ir_get_protocol_name(code->protocol));
            return ESP_ERR_NOT_SUPPORTED;
    }
}

esp_err_t ir_ac_transmit_state(void)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!current_state.is_learned) {
        ESP_LOGE(TAG, "AC not configured. Please learn AC protocol first.");
        return ESP_ERR_INVALID_STATE;
    }

    /* Encode current state to IR code */
    ir_code_t code = {0};
    esp_err_t err = ir_ac_encode_state(&current_state, &code);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to encode AC state: %s", esp_err_to_name(err));
        return err;
    }

    /* Transmit IR code */
    ESP_LOGI(TAG, "Transmitting AC state: Power=%s, Mode=%s, Temp=%d°C",
             current_state.power ? "ON" : "OFF",
             ir_ac_get_mode_name(current_state.mode),
             current_state.temperature);

    err = ir_transmit(&code);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to transmit AC IR code: %s", esp_err_to_name(err));
    }

    return err;
}

/* ============================================================================
 * LEARNING MODE
 * ============================================================================ */

/**
 * @brief Identify AC protocol from captured IR code
 *
 * Analyzes the captured IR code characteristics (bit length, carrier frequency,
 * protocol type) to identify which AC protocol it matches.
 */
static ir_protocol_t identify_ac_protocol(const ir_code_t *code)
{
    if (!code) {
        return IR_PROTOCOL_UNKNOWN;
    }

    /* Check if already identified by decoder */
    if (code->protocol != IR_PROTOCOL_UNKNOWN && code->protocol != IR_PROTOCOL_RAW) {
        /* Verify it's an AC protocol */
        switch (code->protocol) {
            case IR_PROTOCOL_DAIKIN:
            case IR_PROTOCOL_CARRIER:
            case IR_PROTOCOL_HITACHI:
            case IR_PROTOCOL_MITSUBISHI:
            case IR_PROTOCOL_MIDEA:
            case IR_PROTOCOL_HAIER:
            case IR_PROTOCOL_SAMSUNG48:
            case IR_PROTOCOL_PANASONIC:
            case IR_PROTOCOL_FUJITSU:
            case IR_PROTOCOL_LG2:
                return code->protocol;
            default:
                break;
        }
    }

    /* Protocol not identified by decoder - try to identify by bit length */
    ESP_LOGI(TAG, "Analyzing captured code: bits=%d, carrier=%dHz", code->bits, code->carrier_freq_hz);

    /* Match by bit length (common AC protocol signatures) */
    switch (code->bits) {
        case 28:
            ESP_LOGI(TAG, "Detected 28-bit frame → LG2 AC protocol");
            return IR_PROTOCOL_LG2;

        case 48:
            /* Multiple AC protocols use 48-bit */
            ESP_LOGI(TAG, "Detected 48-bit frame → Could be Midea, Samsung48, or Panasonic");
            ESP_LOGI(TAG, "Defaulting to Midea (most common)");
            return IR_PROTOCOL_MIDEA;

        case 104:
            ESP_LOGI(TAG, "Detected 104-bit frame → Haier AC protocol");
            return IR_PROTOCOL_HAIER;

        case 128:
            ESP_LOGI(TAG, "Detected 128-bit frame → Carrier/Voltas AC protocol");
            return IR_PROTOCOL_CARRIER;

        case 152:
            ESP_LOGI(TAG, "Detected 152-bit frame → Mitsubishi AC protocol");
            return IR_PROTOCOL_MITSUBISHI;

        case 264:
            ESP_LOGI(TAG, "Detected 264-bit frame → Hitachi AC protocol");
            return IR_PROTOCOL_HITACHI;

        case 312:
            ESP_LOGI(TAG, "Detected 312-bit frame → Daikin AC protocol");
            return IR_PROTOCOL_DAIKIN;

        default:
            /* Variable length protocols */
            if (code->bits >= 100 && code->bits <= 150) {
                ESP_LOGI(TAG, "Detected variable length frame (%d bits) → Fujitsu AC protocol", code->bits);
                return IR_PROTOCOL_FUJITSU;
            }
            break;
    }

    ESP_LOGW(TAG, "Could not identify AC protocol from %d-bit frame", code->bits);
    return IR_PROTOCOL_UNKNOWN;
}

esp_err_t ir_ac_learn_protocol(uint32_t timeout_ms)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "AC Protocol Learning Started");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Please press a button on your AC remote");
    ESP_LOGI(TAG, "Recommended: Power ON + Cool 24°C + Auto Fan");
    ESP_LOGI(TAG, "Timeout: %d seconds", timeout_ms / 1000);

    /* Use IR learning system to capture a code */
    ir_code_t captured_code = {0};
    esp_err_t err = ir_learn_code(timeout_ms, &captured_code);

    if (err == ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "AC learning timeout - no remote signal detected");
        ESP_LOGI(TAG, "Please try again");
        return err;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "AC learning failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "IR code captured successfully!");
    ESP_LOGI(TAG, "  Protocol: %s", ir_get_protocol_name(captured_code.protocol));
    ESP_LOGI(TAG, "  Bits: %d", captured_code.bits);
    ESP_LOGI(TAG, "  Carrier: %d Hz", captured_code.carrier_freq_hz);

    /* Identify AC protocol from captured code */
    ir_protocol_t detected_protocol = identify_ac_protocol(&captured_code);

    if (detected_protocol == IR_PROTOCOL_UNKNOWN) {
        ESP_LOGE(TAG, "Failed to identify AC protocol");
        ESP_LOGE(TAG, "This may not be an AC remote, or protocol is not supported");
        ESP_LOGI(TAG, "Supported AC protocols:");
        ESP_LOGI(TAG, "  - Carrier/Voltas (128-bit)");
        ESP_LOGI(TAG, "  - Daikin (312-bit)");
        ESP_LOGI(TAG, "  - Hitachi (264-bit)");
        ESP_LOGI(TAG, "  - Mitsubishi (152-bit)");
        ESP_LOGI(TAG, "  - Midea (48-bit)");
        ESP_LOGI(TAG, "  - Haier (104-bit)");
        ESP_LOGI(TAG, "  - Samsung48 (48-bit)");
        ESP_LOGI(TAG, "  - Panasonic (48-bit)");
        ESP_LOGI(TAG, "  - Fujitsu (variable)");
        ESP_LOGI(TAG, "  - LG2 (28-bit)");
        return ESP_ERR_NOT_FOUND;
    }

    /* Set the detected protocol */
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "AC Protocol Detected: %s", ir_get_protocol_name(detected_protocol));
    ESP_LOGI(TAG, "========================================");

    err = ir_ac_set_protocol(detected_protocol, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set AC protocol: %s", esp_err_to_name(err));
        return err;
    }

    /* Mark AC as configured */
    current_state.is_learned = true;
    snprintf(current_state.brand, sizeof(current_state.brand), "%s",
             ir_get_protocol_name(detected_protocol));

    /* Try to decode initial state (optional - may not be fully implemented) */
    ac_state_t decoded_state = {0};
    if (ir_ac_decode_state(&captured_code, &decoded_state) == ESP_OK) {
        ESP_LOGI(TAG, "Initial AC state decoded:");
        ESP_LOGI(TAG, "  Power: %s", decoded_state.power ? "ON" : "OFF");
        ESP_LOGI(TAG, "  Mode: %s", ir_ac_get_mode_name(decoded_state.mode));
        ESP_LOGI(TAG, "  Temperature: %d°C", decoded_state.temperature);
        ESP_LOGI(TAG, "  Fan Speed: %s", ir_ac_get_fan_speed_name(decoded_state.fan_speed));

        /* Update current state with decoded values */
        current_state.power = decoded_state.power;
        current_state.mode = decoded_state.mode;
        current_state.temperature = decoded_state.temperature;
        current_state.fan_speed = decoded_state.fan_speed;
        current_state.swing = decoded_state.swing;
    } else {
        ESP_LOGW(TAG, "Could not decode initial state from captured frame");
        ESP_LOGI(TAG, "Using default state: Power=OFF, Mode=Cool, Temp=24°C");
        /* Keep default state values */
    }

    /* Save configuration to NVS */
    err = ir_ac_save_state();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to save AC configuration: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "AC configuration saved to NVS");
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "AC Learning Complete!");
    ESP_LOGI(TAG, "You can now control your AC via RainMaker");
    ESP_LOGI(TAG, "========================================");

    return ESP_OK;
}

/* ============================================================================
 * NVS STORAGE
 * ============================================================================ */

esp_err_t ir_ac_save_state(void)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Save AC state to NVS */
    esp_err_t err = nvs_set_blob(nvs_handle_ac, NVS_KEY_AC_STATE, &current_state, sizeof(ac_state_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save AC state: %s", esp_err_to_name(err));
        return err;
    }

    /* Commit to NVS */
    err = nvs_commit(nvs_handle_ac);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGD(TAG, "AC state saved to NVS");
    return ESP_OK;
}

esp_err_t ir_ac_load_state(void)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Load AC state from NVS */
    size_t required_size = sizeof(ac_state_t);
    esp_err_t err = nvs_get_blob(nvs_handle_ac, NVS_KEY_AC_STATE, &current_state, &required_size);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGD(TAG, "No saved AC state found");
        return ESP_ERR_NOT_FOUND;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load AC state: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "AC state loaded from NVS (Protocol: %s, Power: %s, Temp: %d°C)",
             ir_get_protocol_name(current_state.protocol),
             current_state.power ? "ON" : "OFF",
             current_state.temperature);

    return ESP_OK;
}

esp_err_t ir_ac_clear_state(void)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Erase from NVS */
    esp_err_t err = nvs_erase_key(nvs_handle_ac, NVS_KEY_AC_STATE);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to clear AC state: %s", esp_err_to_name(err));
        return err;
    }

    /* Commit */
    err = nvs_commit(nvs_handle_ac);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        return err;
    }

    /* Reset to default state */
    ir_ac_get_default_state(&current_state);

    ESP_LOGI(TAG, "AC configuration cleared (factory reset)");
    return ESP_OK;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

const char* ir_ac_get_mode_name(ac_mode_t mode)
{
    if (mode >= 0 && mode < AC_MODE_MAX) {
        return mode_names[mode];
    }
    return "Unknown";
}

const char* ir_ac_get_fan_speed_name(ac_fan_speed_t fan_speed)
{
    if (fan_speed >= 0 && fan_speed < AC_FAN_MAX) {
        return fan_speed_names[fan_speed];
    }
    return "Unknown";
}

const char* ir_ac_get_swing_name(ac_swing_t swing)
{
    if (swing >= 0 && swing < AC_SWING_MAX) {
        return swing_names[swing];
    }
    return "Unknown";
}

esp_err_t ir_ac_validate_state(const ac_state_t *state)
{
    if (!state) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Validate mode */
    if (state->mode < AC_MODE_OFF || state->mode >= AC_MODE_MAX) {
        ESP_LOGE(TAG, "Invalid mode: %d", state->mode);
        return ESP_ERR_INVALID_ARG;
    }

    /* Validate temperature */
    if (state->temperature < AC_TEMP_MIN || state->temperature > AC_TEMP_MAX) {
        ESP_LOGE(TAG, "Temperature out of range: %d (valid: %d-%d)",
                 state->temperature, AC_TEMP_MIN, AC_TEMP_MAX);
        return ESP_ERR_INVALID_ARG;
    }

    /* Validate fan speed */
    if (state->fan_speed < AC_FAN_AUTO || state->fan_speed >= AC_FAN_MAX) {
        ESP_LOGE(TAG, "Invalid fan speed: %d", state->fan_speed);
        return ESP_ERR_INVALID_ARG;
    }

    /* Validate swing */
    if (state->swing < AC_SWING_OFF || state->swing >= AC_SWING_MAX) {
        ESP_LOGE(TAG, "Invalid swing mode: %d", state->swing);
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

void ir_ac_get_default_state(ac_state_t *state)
{
    if (!state) {
        return;
    }

    memset(state, 0, sizeof(ac_state_t));

    /* Set safe defaults */
    state->power = false;
    state->mode = AC_MODE_COOL;
    state->temperature = AC_TEMP_DEFAULT;
    state->fan_speed = AC_FAN_AUTO;
    state->swing = AC_SWING_OFF;
    state->turbo = false;
    state->quiet = false;
    state->econo = false;
    state->clean = false;
    state->sleep = false;
    state->sleep_timer = 0;
    state->display = true;
    state->beep = true;
    state->filter = false;
    state->light = true;
    state->anti_fungal = false;
    state->auto_clean = false;
    state->comfort_mode = 0;
    state->protocol = IR_PROTOCOL_UNKNOWN;
    state->protocol_variant = 0;
    state->is_learned = false;
    strncpy(state->brand, "Unknown", sizeof(state->brand) - 1);
    strncpy(state->model, "", sizeof(state->model) - 1);
}
