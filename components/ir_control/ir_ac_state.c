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
static nvs_handle_t nvs_handle = 0;
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

    /* Open NVS namespace for AC state */
    esp_err_t err = nvs_open(NVS_NAMESPACE_AC, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
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

esp_err_t ir_ac_decode_state(const ir_code_t *code, ac_state_t *state)
{
    if (!code || !state) {
        return ESP_ERR_INVALID_ARG;
    }

    /* TODO: Implement protocol-specific decoders */
    /* For now, return not supported */
    ESP_LOGW(TAG, "AC state decoding not yet implemented");
    return ESP_ERR_NOT_SUPPORTED;
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

esp_err_t ir_ac_learn_protocol(uint32_t timeout_ms)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting AC protocol learning...");
    ESP_LOGI(TAG, "Please press a button on your AC remote (e.g., Power ON + Cool 24°C)");

    /* TODO: Implement AC protocol learning */
    /* This requires:
     * 1. Capture IR frame from user's AC remote
     * 2. Decode protocol using existing decoders
     * 3. Extract state from decoded frame
     * 4. Set protocol and initial state
     */

    ESP_LOGW(TAG, "AC learning not yet fully implemented");
    ESP_LOGW(TAG, "For now, manually set protocol using ir_ac_set_protocol()");

    return ESP_ERR_NOT_SUPPORTED;
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
    esp_err_t err = nvs_set_blob(nvs_handle, NVS_KEY_AC_STATE, &current_state, sizeof(ac_state_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save AC state: %s", esp_err_to_name(err));
        return err;
    }

    /* Commit to NVS */
    err = nvs_commit(nvs_handle);
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
    esp_err_t err = nvs_get_blob(nvs_handle, NVS_KEY_AC_STATE, &current_state, &required_size);

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
    esp_err_t err = nvs_erase_key(nvs_handle, NVS_KEY_AC_STATE);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to clear AC state: %s", esp_err_to_name(err));
        return err;
    }

    /* Commit */
    err = nvs_commit(nvs_handle);
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
