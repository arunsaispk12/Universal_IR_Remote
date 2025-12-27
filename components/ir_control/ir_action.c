/**
 * @file ir_action.c
 * @brief Logical Action Mapping Implementation
 *
 * Implements the action-to-IR-code mapping layer that abstracts RainMaker
 * device parameters from raw IR codes.
 *
 * Copyright (c) 2025
 */

#include "ir_action.h"
#include "ir_control.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "ir_action";

/* NVS namespace for action mappings */
#define NVS_NAMESPACE_ACTIONS   "ir_actions"

/* Maximum NVS key length */
#define MAX_NVS_KEY_LEN         15

/* Internal state */
static bool is_initialized = false;
static nvs_handle_t nvs_handle = 0;

/* Current learning state */
static ir_device_type_t learning_device = IR_DEVICE_NONE;
static ir_action_t learning_action = IR_ACTION_NONE;
static bool is_learning_active = false;

/* Forward declarations for internal functions */
static esp_err_t action_learning_success_cb(ir_button_t button, ir_code_t *code, void *arg);
static esp_err_t generate_nvs_key_internal(ir_device_type_t device, ir_action_t action,
                                             char *key_buffer, size_t buffer_size);

/* ============================================================================
 * DEVICE AND ACTION NAME TABLES
 * ============================================================================ */

static const char* device_names[] = {
    [IR_DEVICE_NONE]    = "None",
    [IR_DEVICE_TV]      = "TV",
    [IR_DEVICE_AC]      = "AC",
    [IR_DEVICE_STB]     = "STB",
    [IR_DEVICE_SPEAKER] = "Speaker",
    [IR_DEVICE_FAN]     = "Fan",
    [IR_DEVICE_CUSTOM]  = "Custom",
};

static const char* action_names[] = {
    [IR_ACTION_NONE]                = "None",

    /* Common actions */
    [IR_ACTION_POWER]               = "Power",
    [IR_ACTION_POWER_ON]            = "PowerOn",
    [IR_ACTION_POWER_OFF]           = "PowerOff",
    [IR_ACTION_VOL_UP]              = "VolumeUp",
    [IR_ACTION_VOL_DOWN]            = "VolumeDown",
    [IR_ACTION_MUTE]                = "Mute",
    [IR_ACTION_CH_UP]               = "ChannelUp",
    [IR_ACTION_CH_DOWN]             = "ChannelDown",
    [IR_ACTION_CH_PREV]             = "ChannelPrev",
    [IR_ACTION_NAV_UP]              = "NavUp",
    [IR_ACTION_NAV_DOWN]            = "NavDown",
    [IR_ACTION_NAV_LEFT]            = "NavLeft",
    [IR_ACTION_NAV_RIGHT]           = "NavRight",
    [IR_ACTION_NAV_OK]              = "NavOK",
    [IR_ACTION_MENU]                = "Menu",
    [IR_ACTION_HOME]                = "Home",
    [IR_ACTION_BACK]                = "Back",
    [IR_ACTION_EXIT]                = "Exit",
    [IR_ACTION_INFO]                = "Info",
    [IR_ACTION_NUM_0]               = "Num0",
    [IR_ACTION_NUM_1]               = "Num1",
    [IR_ACTION_NUM_2]               = "Num2",
    [IR_ACTION_NUM_3]               = "Num3",
    [IR_ACTION_NUM_4]               = "Num4",
    [IR_ACTION_NUM_5]               = "Num5",
    [IR_ACTION_NUM_6]               = "Num6",
    [IR_ACTION_NUM_7]               = "Num7",
    [IR_ACTION_NUM_8]               = "Num8",
    [IR_ACTION_NUM_9]               = "Num9",

    /* TV-specific */
    [IR_ACTION_TV_INPUT]            = "Input",
    [IR_ACTION_TV_INPUT_HDMI1]      = "HDMI1",
    [IR_ACTION_TV_INPUT_HDMI2]      = "HDMI2",
    [IR_ACTION_TV_INPUT_HDMI3]      = "HDMI3",
    [IR_ACTION_TV_INPUT_AV]         = "AV",
    [IR_ACTION_TV_INPUT_USB]        = "USB",
    [IR_ACTION_TV_PICTURE_MODE]     = "PictureMode",
    [IR_ACTION_TV_SOUND_MODE]       = "SoundMode",
    [IR_ACTION_TV_SLEEP_TIMER]      = "SleepTimer",

    /* STB-specific */
    [IR_ACTION_STB_GUIDE]           = "Guide",
    [IR_ACTION_STB_RECORD]          = "Record",
    [IR_ACTION_STB_PLAY_PAUSE]      = "PlayPause",
    [IR_ACTION_STB_STOP]            = "Stop",
    [IR_ACTION_STB_REWIND]          = "Rewind",
    [IR_ACTION_STB_FORWARD]         = "Forward",
    [IR_ACTION_STB_PREV_TRACK]      = "PrevTrack",
    [IR_ACTION_STB_NEXT_TRACK]      = "NextTrack",
    [IR_ACTION_STB_SUBTITLE]        = "Subtitle",
    [IR_ACTION_STB_AUDIO]           = "Audio",

    /* Speaker-specific */
    [IR_ACTION_SPEAKER_MODE]        = "SpeakerMode",
    [IR_ACTION_SPEAKER_BASS_UP]     = "BassUp",
    [IR_ACTION_SPEAKER_BASS_DOWN]   = "BassDown",
    [IR_ACTION_SPEAKER_TREBLE_UP]   = "TrebleUp",
    [IR_ACTION_SPEAKER_TREBLE_DOWN] = "TrebleDown",
    [IR_ACTION_SPEAKER_SURROUND]    = "Surround",
    [IR_ACTION_SPEAKER_BLUETOOTH]   = "Bluetooth",
    [IR_ACTION_SPEAKER_AUX]         = "AUX",
    [IR_ACTION_SPEAKER_OPTICAL]     = "Optical",
    [IR_ACTION_SPEAKER_SUBWOOFER]   = "Subwoofer",

    /* Fan-specific */
    [IR_ACTION_FAN_SPEED_UP]        = "FanSpeedUp",
    [IR_ACTION_FAN_SPEED_DOWN]      = "FanSpeedDown",
    [IR_ACTION_FAN_SPEED_1]         = "FanSpeed1",
    [IR_ACTION_FAN_SPEED_2]         = "FanSpeed2",
    [IR_ACTION_FAN_SPEED_3]         = "FanSpeed3",
    [IR_ACTION_FAN_SPEED_4]         = "FanSpeed4",
    [IR_ACTION_FAN_SPEED_5]         = "FanSpeed5",
    [IR_ACTION_FAN_SWING]           = "FanSwing",
    [IR_ACTION_FAN_TIMER]           = "FanTimer",
    [IR_ACTION_FAN_SLEEP_MODE]      = "FanSleepMode",
    [IR_ACTION_FAN_NATURAL_WIND]    = "NaturalWind",
    [IR_ACTION_FAN_IONIZER]         = "Ionizer",

    /* AC-specific (fallback actions) */
    [IR_ACTION_AC_TEMP_UP]          = "ACTempUp",
    [IR_ACTION_AC_TEMP_DOWN]        = "ACTempDown",
    [IR_ACTION_AC_MODE_COOL]        = "ACModeCool",
    [IR_ACTION_AC_MODE_HEAT]        = "ACModeHeat",
    [IR_ACTION_AC_MODE_FAN]         = "ACModeFan",
    [IR_ACTION_AC_MODE_DRY]         = "ACModeDry",
    [IR_ACTION_AC_MODE_AUTO]        = "ACModeAuto",
    [IR_ACTION_AC_FAN_SPEED_UP]     = "ACFanSpeedUp",
    [IR_ACTION_AC_FAN_SPEED_DOWN]   = "ACFanSpeedDown",
    [IR_ACTION_AC_SWING]            = "ACSwing",

    /* Custom Device Actions */
    [IR_ACTION_CUSTOM_1]            = "Custom1",
    [IR_ACTION_CUSTOM_2]            = "Custom2",
    [IR_ACTION_CUSTOM_3]            = "Custom3",
    [IR_ACTION_CUSTOM_4]            = "Custom4",
    [IR_ACTION_CUSTOM_5]            = "Custom5",
    [IR_ACTION_CUSTOM_6]            = "Custom6",
    [IR_ACTION_CUSTOM_7]            = "Custom7",
    [IR_ACTION_CUSTOM_8]            = "Custom8",
    [IR_ACTION_CUSTOM_9]            = "Custom9",
    [IR_ACTION_CUSTOM_10]           = "Custom10",
    [IR_ACTION_CUSTOM_11]           = "Custom11",
    [IR_ACTION_CUSTOM_12]           = "Custom12",
};

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

esp_err_t ir_action_init(void)
{
    if (is_initialized) {
        ESP_LOGW(TAG, "Action system already initialized");
        return ESP_OK;
    }

    /* Open NVS namespace for actions */
    esp_err_t err = nvs_open(NVS_NAMESPACE_ACTIONS, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return err;
    }

    is_initialized = true;
    ESP_LOGI(TAG, "Action mapping system initialized");
    return ESP_OK;
}

/* ============================================================================
 * LEARNING MODE
 * ============================================================================ */

esp_err_t ir_action_learn(ir_device_type_t device, ir_action_t action, uint32_t timeout_ms)
{
    if (!is_initialized) {
        ESP_LOGE(TAG, "Action system not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (device <= IR_DEVICE_NONE || device >= IR_DEVICE_MAX) {
        ESP_LOGE(TAG, "Invalid device type: %d", device);
        return ESP_ERR_INVALID_ARG;
    }

    if (action <= IR_ACTION_NONE || action >= IR_ACTION_MAX) {
        ESP_LOGE(TAG, "Invalid action: %d", action);
        return ESP_ERR_INVALID_ARG;
    }

    if (is_learning_active) {
        ESP_LOGW(TAG, "Learning already in progress");
        return ESP_ERR_INVALID_STATE;
    }

    /* Set learning state */
    learning_device = device;
    learning_action = action;
    is_learning_active = true;

    ESP_LOGI(TAG, "Starting learning for %s.%s",
             ir_action_get_device_name(device),
             ir_action_get_action_name(action));

    /* Start IR learning mode (using existing IR control API) */
    /* We use IR_BTN_CUSTOM_1 as a dummy button for learning */
    esp_err_t err = ir_learn_start(IR_BTN_CUSTOM_1, timeout_ms);
    if (err != ESP_OK) {
        is_learning_active = false;
        learning_device = IR_DEVICE_NONE;
        learning_action = IR_ACTION_NONE;
        return err;
    }

    return ESP_OK;
}

/* Internal callback when learning succeeds */
static esp_err_t action_learning_success_cb(ir_button_t button, ir_code_t *code, void *arg)
{
    if (!is_learning_active) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Learning succeeded for %s.%s (protocol: %s)",
             ir_action_get_device_name(learning_device),
             ir_action_get_action_name(learning_action),
             ir_get_protocol_name(code->protocol));

    /* Save the learned code to NVS */
    esp_err_t err = ir_action_save(learning_device, learning_action, code);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save action: %s", esp_err_to_name(err));
    }

    /* Reset learning state */
    is_learning_active = false;
    learning_device = IR_DEVICE_NONE;
    learning_action = IR_ACTION_NONE;

    return err;
}

/* ============================================================================
 * ACTION EXECUTION
 * ============================================================================ */

esp_err_t ir_action_execute(ir_device_type_t device, ir_action_t action)
{
    if (!is_initialized) {
        ESP_LOGE(TAG, "Action system not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    /* Load IR code for this action */
    ir_code_t code = {0};
    esp_err_t err = ir_action_load(device, action, &code);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Action %s.%s not learned",
                 ir_action_get_device_name(device),
                 ir_action_get_action_name(action));
        return ESP_ERR_NOT_FOUND;
    }

    /* Transmit the IR code */
    ESP_LOGI(TAG, "Executing action: %s.%s",
             ir_action_get_device_name(device),
             ir_action_get_action_name(action));

    err = ir_transmit(&code);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to transmit IR code: %s", esp_err_to_name(err));
    }

    return err;
}

esp_err_t ir_action_execute_repeat(ir_device_type_t device, ir_action_t action,
                                     uint8_t repeat_count, uint16_t repeat_interval_ms)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Load IR code */
    ir_code_t code = {0};
    esp_err_t err = ir_action_load(device, action, &code);
    if (err != ESP_OK) {
        return ESP_ERR_NOT_FOUND;
    }

    /* Use protocol-specific repeat interval if not specified */
    uint16_t interval = repeat_interval_ms;
    if (interval == 0) {
        interval = code.repeat_period_ms > 0 ? code.repeat_period_ms : 110; // Default NEC repeat
    }

    ESP_LOGI(TAG, "Executing repeat action: %s.%s (count=%d, interval=%dms)",
             ir_action_get_device_name(device),
             ir_action_get_action_name(action),
             repeat_count, interval);

    /* Transmit multiple times */
    for (uint8_t i = 0; i < repeat_count; i++) {
        err = ir_transmit(&code);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to transmit repeat %d: %s", i, esp_err_to_name(err));
            return err;
        }

        if (i < repeat_count - 1) {
            vTaskDelay(pdMS_TO_TICKS(interval));
        }
    }

    return ESP_OK;
}

/* ============================================================================
 * NVS STORAGE
 * ============================================================================ */

static esp_err_t generate_nvs_key_internal(ir_device_type_t device, ir_action_t action,
                                             char *key_buffer, size_t buffer_size)
{
    if (buffer_size < MAX_NVS_KEY_LEN + 1) {
        return ESP_ERR_INVALID_SIZE;
    }

    /* Format: "<device>_<action>" (e.g., "tv_power", "ac_cool") */
    const char *dev_prefix = "";
    switch (device) {
        case IR_DEVICE_TV:      dev_prefix = "tv"; break;
        case IR_DEVICE_AC:      dev_prefix = "ac"; break;
        case IR_DEVICE_STB:     dev_prefix = "stb"; break;
        case IR_DEVICE_SPEAKER: dev_prefix = "spk"; break;
        case IR_DEVICE_FAN:     dev_prefix = "fan"; break;
        default:                dev_prefix = "unk"; break;
    }

    snprintf(key_buffer, buffer_size, "%s_%d", dev_prefix, (int)action);
    return ESP_OK;
}

esp_err_t ir_action_save(ir_device_type_t device, ir_action_t action, const ir_code_t *code)
{
    if (!is_initialized || !code) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Generate NVS key */
    char nvs_key[MAX_NVS_KEY_LEN + 1];
    esp_err_t err = generate_nvs_key_internal(device, action, nvs_key, sizeof(nvs_key));
    if (err != ESP_OK) {
        return err;
    }

    /* Save IR code to NVS */
    /* Note: For RAW codes, we need to save raw_data separately */
    err = nvs_set_blob(nvs_handle, nvs_key, code, sizeof(ir_code_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save action %s: %s", nvs_key, esp_err_to_name(err));
        return err;
    }

    /* If RAW protocol, save raw data array */
    if (code->protocol == IR_PROTOCOL_RAW && code->raw_data && code->raw_length > 0) {
        char raw_key[MAX_NVS_KEY_LEN + 5];
        snprintf(raw_key, sizeof(raw_key), "%s_raw", nvs_key);

        err = nvs_set_blob(nvs_handle, raw_key, code->raw_data,
                            code->raw_length * sizeof(uint16_t));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to save RAW data for %s: %s", nvs_key, esp_err_to_name(err));
            return err;
        }
    }

    /* Commit to NVS */
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Saved action %s.%s to NVS (key: %s)",
             ir_action_get_device_name(device),
             ir_action_get_action_name(action),
             nvs_key);

    return ESP_OK;
}

esp_err_t ir_action_load(ir_device_type_t device, ir_action_t action, ir_code_t *code)
{
    if (!is_initialized || !code) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Generate NVS key */
    char nvs_key[MAX_NVS_KEY_LEN + 1];
    esp_err_t err = generate_nvs_key_internal(device, action, nvs_key, sizeof(nvs_key));
    if (err != ESP_OK) {
        return err;
    }

    /* Load IR code from NVS */
    size_t required_size = sizeof(ir_code_t);
    err = nvs_get_blob(nvs_handle, nvs_key, code, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_ERR_NOT_FOUND;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load action %s: %s", nvs_key, esp_err_to_name(err));
        return err;
    }

    /* If RAW protocol, load raw data array */
    if (code->protocol == IR_PROTOCOL_RAW && code->raw_length > 0) {
        char raw_key[MAX_NVS_KEY_LEN + 5];
        snprintf(raw_key, sizeof(raw_key), "%s_raw", nvs_key);

        /* Allocate memory for raw data */
        code->raw_data = (uint16_t*)malloc(code->raw_length * sizeof(uint16_t));
        if (!code->raw_data) {
            ESP_LOGE(TAG, "Failed to allocate memory for RAW data");
            return ESP_ERR_NO_MEM;
        }

        size_t raw_size = code->raw_length * sizeof(uint16_t);
        err = nvs_get_blob(nvs_handle, raw_key, code->raw_data, &raw_size);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to load RAW data for %s: %s", nvs_key, esp_err_to_name(err));
            free(code->raw_data);
            code->raw_data = NULL;
            return err;
        }
    }

    ESP_LOGD(TAG, "Loaded action %s.%s from NVS (protocol: %s)",
             ir_action_get_device_name(device),
             ir_action_get_action_name(action),
             ir_get_protocol_name(code->protocol));

    return ESP_OK;
}

bool ir_action_is_learned(ir_device_type_t device, ir_action_t action)
{
    ir_code_t code = {0};
    return (ir_action_load(device, action, &code) == ESP_OK);
}

esp_err_t ir_action_clear(ir_device_type_t device, ir_action_t action)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Generate NVS key */
    char nvs_key[MAX_NVS_KEY_LEN + 1];
    esp_err_t err = generate_nvs_key_internal(device, action, nvs_key, sizeof(nvs_key));
    if (err != ESP_OK) {
        return err;
    }

    /* Erase from NVS */
    err = nvs_erase_key(nvs_handle, nvs_key);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK; // Already cleared
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear action %s: %s", nvs_key, esp_err_to_name(err));
        return err;
    }

    /* Also erase RAW data if exists */
    char raw_key[MAX_NVS_KEY_LEN + 5];
    snprintf(raw_key, sizeof(raw_key), "%s_raw", nvs_key);
    nvs_erase_key(nvs_handle, raw_key); // Ignore errors for RAW key

    /* Commit */
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Cleared action %s.%s",
             ir_action_get_device_name(device),
             ir_action_get_action_name(action));

    return ESP_OK;
}

esp_err_t ir_action_clear_device(ir_device_type_t device)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Clearing all actions for device: %s", ir_action_get_device_name(device));

    /* Iterate through all possible actions and clear */
    for (int i = IR_ACTION_NONE + 1; i < IR_ACTION_MAX; i++) {
        ir_action_clear(device, (ir_action_t)i);
    }

    return ESP_OK;
}

esp_err_t ir_action_clear_all(void)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Clearing all action mappings (factory reset)");

    /* Erase entire namespace */
    esp_err_t err = nvs_erase_all(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase all actions: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "All action mappings cleared");
    return ESP_OK;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

const char* ir_action_get_device_name(ir_device_type_t device)
{
    if (device >= 0 && device < IR_DEVICE_MAX) {
        return device_names[device];
    }
    return "Unknown";
}

const char* ir_action_get_action_name(ir_action_t action)
{
    if (action >= 0 && action < IR_ACTION_MAX) {
        return action_names[action];
    }
    return "Unknown";
}

esp_err_t ir_action_generate_nvs_key(ir_device_type_t device, ir_action_t action,
                                       char *key_buffer, size_t buffer_size)
{
    return generate_nvs_key_internal(device, action, key_buffer, buffer_size);
}

esp_err_t ir_action_get_device_actions(ir_device_type_t device, ir_action_t *actions,
                                         size_t max_actions, size_t *action_count)
{
    if (!actions || !action_count) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t count = 0;

    /* Add common actions (applicable to all devices except AC) */
    if (device != IR_DEVICE_AC && count < max_actions) {
        actions[count++] = IR_ACTION_POWER;
        if (count < max_actions) actions[count++] = IR_ACTION_VOL_UP;
        if (count < max_actions) actions[count++] = IR_ACTION_VOL_DOWN;
        if (count < max_actions) actions[count++] = IR_ACTION_MUTE;
    }

    /* Add device-specific actions */
    switch (device) {
        case IR_DEVICE_TV:
            if (count < max_actions) actions[count++] = IR_ACTION_CH_UP;
            if (count < max_actions) actions[count++] = IR_ACTION_CH_DOWN;
            if (count < max_actions) actions[count++] = IR_ACTION_TV_INPUT;
            if (count < max_actions) actions[count++] = IR_ACTION_MENU;
            if (count < max_actions) actions[count++] = IR_ACTION_NAV_OK;
            /* Add more TV actions as needed */
            break;

        case IR_DEVICE_STB:
            if (count < max_actions) actions[count++] = IR_ACTION_CH_UP;
            if (count < max_actions) actions[count++] = IR_ACTION_CH_DOWN;
            if (count < max_actions) actions[count++] = IR_ACTION_STB_GUIDE;
            if (count < max_actions) actions[count++] = IR_ACTION_STB_PLAY_PAUSE;
            /* Add more STB actions */
            break;

        case IR_DEVICE_FAN:
            if (count < max_actions) actions[count++] = IR_ACTION_FAN_SPEED_UP;
            if (count < max_actions) actions[count++] = IR_ACTION_FAN_SPEED_DOWN;
            if (count < max_actions) actions[count++] = IR_ACTION_FAN_SWING;
            /* Add more fan actions */
            break;

        case IR_DEVICE_AC:
            /* AC uses state-based control, not individual actions */
            /* These are fallback actions for non-state-aware remotes */
            if (count < max_actions) actions[count++] = IR_ACTION_POWER;
            if (count < max_actions) actions[count++] = IR_ACTION_AC_TEMP_UP;
            if (count < max_actions) actions[count++] = IR_ACTION_AC_TEMP_DOWN;
            break;

        default:
            break;
    }

    *action_count = count;
    return ESP_OK;
}
