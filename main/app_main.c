/**
 * @file app_main.c
 * @brief Universal IR Remote Control with ESP RainMaker - Multi-Device Architecture (v3.1+)
 *
 * Features:
 * - Multi-device architecture (TV, AC, STB, Speaker, Fan, Custom)
 * - Logical action mapping (RainMaker params → IR codes)
 * - AC state-based control (full state regeneration)
 * - Custom device (12 programmable buttons for any IR appliance)
 * - BLE WiFi provisioning
 * - Cloud control via RainMaker app
 * - IR learning and transmission
 * - RGB LED status indication
 * - OTA updates
 * - Factory reset (10s button press)
 * - WiFi reset (3s button press)
 *
 * Architecture:
 * RainMaker Device → Logical Parameter → Action Mapping → IR Code → Transmission
 *
 * Example: TV.Volume parameter change → IR_ACTION_VOL_UP → Stored IR code → Transmit
 *
 * v3.1+ - Custom device support for generic IR appliances
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_console.h"
#include "driver/gpio.h"

/* ESP RainMaker */
#include "esp_rmaker_core.h"
#include "esp_rmaker_standard_types.h"
#include "esp_rmaker_standard_params.h"
#include "esp_rmaker_standard_devices.h"
#include "esp_rmaker_ota.h"
#include "esp_rmaker_schedule.h"
#include "esp_rmaker_console.h"

/* Application headers */
#include "app_config.h"
#include "app_wifi.h"
#include "ir_control.h"
#include "ir_action.h"
#include "ir_ac_state.h"
#include "rgb_led.h"

static const char *TAG = "app_main";

/* ============================================================================
 * RAINMAKER DEVICE HANDLES
 * ============================================================================ */

static esp_rmaker_device_t *tv_device = NULL;
static esp_rmaker_device_t *ac_device = NULL;
static esp_rmaker_device_t *stb_device = NULL;
static esp_rmaker_device_t *speaker_device = NULL;
static esp_rmaker_device_t *fan_device = NULL;
static esp_rmaker_device_t *custom_device = NULL;

/* Boot button handling */
static TimerHandle_t boot_button_timer = NULL;
static uint32_t button_press_start = 0;
static bool factory_reset_triggered = false;

/* Learning state */
typedef struct {
    ir_device_type_t device;
    ir_action_t action;
    bool is_active;
} learning_state_t;

static learning_state_t learning_state = {0};

/* ============================================================================
 * IR LEARNING CALLBACKS
 * ============================================================================ */

/**
 * @brief Callback when IR learning succeeds
 */
static void ir_learn_success_callback(ir_button_t button, ir_code_t *code, void *arg)
{
    if (!learning_state.is_active) {
        return;
    }

    ESP_LOGI(TAG, "IR learning successful for %s.%s (%s protocol)",
             ir_action_get_device_name(learning_state.device),
             ir_action_get_action_name(learning_state.action),
             ir_get_protocol_name(code->protocol));

    /* Save code to NVS using action mapping */
    if (ir_action_save(learning_state.device, learning_state.action, code) == ESP_OK) {
        ESP_LOGI(TAG, "IR code saved to NVS");
        rgb_led_set_status(LED_STATUS_LEARN_SUCCESS);
    } else {
        ESP_LOGE(TAG, "Failed to save IR code");
        rgb_led_set_status(LED_STATUS_ERROR);
    }

    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Reset learning state */
    learning_state.is_active = false;
    learning_state.device = IR_DEVICE_NONE;
    learning_state.action = IR_ACTION_NONE;

    /* Return to connected state */
    if (app_wifi_is_connected()) {
        rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);
    } else {
        rgb_led_set_status(LED_STATUS_IDLE);
    }
}

/**
 * @brief Callback when IR learning fails
 */
static void ir_learn_fail_callback(ir_button_t button, void *arg)
{
    ESP_LOGW(TAG, "IR learning failed (timeout)");

    rgb_led_set_status(LED_STATUS_LEARN_FAILED);
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Reset learning state */
    learning_state.is_active = false;
    learning_state.device = IR_DEVICE_NONE;
    learning_state.action = IR_ACTION_NONE;

    /* Return to connected state */
    if (app_wifi_is_connected()) {
        rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);
    } else {
        rgb_led_set_status(LED_STATUS_IDLE);
    }
}

/* ============================================================================
 * TV DEVICE CALLBACKS
 * ============================================================================ */

static esp_err_t tv_write_cb(const esp_rmaker_device_t *device,
                               const esp_rmaker_param_t *param,
                               const esp_rmaker_param_val_t val,
                               void *priv_data,
                               esp_rmaker_write_ctx_t *ctx)
{
    const char *param_name = esp_rmaker_param_get_name(param);
    ESP_LOGI(TAG, "TV parameter update: %s", param_name);

    /* Power */
    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        bool power = val.val.b;
        ESP_LOGI(TAG, "TV Power: %s", power ? "ON" : "OFF");
        return ir_action_execute(IR_DEVICE_TV, IR_ACTION_POWER);
    }
    /* Volume */
    else if (strcmp(param_name, "Volume") == 0) {
        int volume = val.val.i;
        ESP_LOGI(TAG, "TV Volume: %d (use Vol+/Vol- actions)", volume);
        /* Volume tracking not implemented - parameters are for UI only */
        return ESP_OK;
    }
    /* Mute */
    else if (strcmp(param_name, "Mute") == 0) {
        bool mute = val.val.b;
        ESP_LOGI(TAG, "TV Mute: %s", mute ? "ON" : "OFF");
        return ir_action_execute(IR_DEVICE_TV, IR_ACTION_MUTE);
    }
    /* Channel */
    else if (strcmp(param_name, "Channel") == 0) {
        int channel = val.val.i;
        ESP_LOGI(TAG, "TV Channel: %d (use Ch+/Ch- actions)", channel);
        /* Channel tracking not implemented - parameters are for UI only */
        return ESP_OK;
    }
    /* Input Source */
    else if (strcmp(param_name, "Input") == 0) {
        const char *input = val.val.s;
        ESP_LOGI(TAG, "TV Input: %s", input);

        if (strcmp(input, "HDMI1") == 0) {
            return ir_action_execute(IR_DEVICE_TV, IR_ACTION_TV_INPUT_HDMI1);
        } else if (strcmp(input, "HDMI2") == 0) {
            return ir_action_execute(IR_DEVICE_TV, IR_ACTION_TV_INPUT_HDMI2);
        } else if (strcmp(input, "HDMI3") == 0) {
            return ir_action_execute(IR_DEVICE_TV, IR_ACTION_TV_INPUT_HDMI3);
        } else if (strcmp(input, "AV") == 0) {
            return ir_action_execute(IR_DEVICE_TV, IR_ACTION_TV_INPUT_AV);
        } else {
            return ir_action_execute(IR_DEVICE_TV, IR_ACTION_TV_INPUT);
        }
    }
    /* Learn Mode */
    else if (strcmp(param_name, "Learn_Mode") == 0) {
        const char *action_name = val.val.s;
        ESP_LOGI(TAG, "TV Learn Mode: %s", action_name);

        /* Map action name to ir_action_t */
        ir_action_t action = IR_ACTION_NONE;
        if (strcmp(action_name, "Power") == 0) action = IR_ACTION_POWER;
        else if (strcmp(action_name, "VolumeUp") == 0) action = IR_ACTION_VOL_UP;
        else if (strcmp(action_name, "VolumeDown") == 0) action = IR_ACTION_VOL_DOWN;
        else if (strcmp(action_name, "Mute") == 0) action = IR_ACTION_MUTE;
        else if (strcmp(action_name, "ChannelUp") == 0) action = IR_ACTION_CH_UP;
        else if (strcmp(action_name, "ChannelDown") == 0) action = IR_ACTION_CH_DOWN;
        else if (strcmp(action_name, "Input") == 0) action = IR_ACTION_TV_INPUT;
        else if (strcmp(action_name, "Menu") == 0) action = IR_ACTION_MENU;
        else if (strcmp(action_name, "OK") == 0) action = IR_ACTION_NAV_OK;
        else if (strcmp(action_name, "Back") == 0) action = IR_ACTION_BACK;
        else {
            ESP_LOGW(TAG, "Unknown action: %s", action_name);
            return ESP_ERR_INVALID_ARG;
        }

        /* Start learning */
        learning_state.device = IR_DEVICE_TV;
        learning_state.action = action;
        learning_state.is_active = true;

        rgb_led_set_status(LED_STATUS_LEARNING);
        return ir_learn_start(IR_BTN_CUSTOM_1, IR_LEARNING_TIMEOUT_MS);
    }

    return ESP_OK;
}

/* ============================================================================
 * AC DEVICE CALLBACKS
 * ============================================================================ */

static esp_err_t ac_write_cb(const esp_rmaker_device_t *device,
                               const esp_rmaker_param_t *param,
                               const esp_rmaker_param_val_t val,
                               void *priv_data,
                               esp_rmaker_write_ctx_t *ctx)
{
    const char *param_name = esp_rmaker_param_get_name(param);
    ESP_LOGI(TAG, "AC parameter update: %s", param_name);

    /* AC Power */
    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        bool power = val.val.b;
        ESP_LOGI(TAG, "AC Power: %s", power ? "ON" : "OFF");
        return ir_ac_set_power(power);
    }
    /* AC Mode */
    else if (strcmp(param_name, "Mode") == 0) {
        const char *mode_str = val.val.s;
        ESP_LOGI(TAG, "AC Mode: %s", mode_str);

        ac_mode_t mode = AC_MODE_COOL;
        if (strcmp(mode_str, "Cool") == 0) mode = AC_MODE_COOL;
        else if (strcmp(mode_str, "Heat") == 0) mode = AC_MODE_HEAT;
        else if (strcmp(mode_str, "Auto") == 0) mode = AC_MODE_AUTO;
        else if (strcmp(mode_str, "Dry") == 0) mode = AC_MODE_DRY;
        else if (strcmp(mode_str, "Fan") == 0) mode = AC_MODE_FAN;

        return ir_ac_set_mode(mode);
    }
    /* AC Temperature */
    else if (strcmp(param_name, ESP_RMAKER_DEF_TEMPERATURE_NAME) == 0) {
        float temp = val.val.f;
        uint8_t temperature = (uint8_t)temp;
        ESP_LOGI(TAG, "AC Temperature: %d°C", temperature);
        return ir_ac_set_temperature(temperature);
    }
    /* AC Fan Speed */
    else if (strcmp(param_name, "Fan_Speed") == 0) {
        const char *fan_str = val.val.s;
        ESP_LOGI(TAG, "AC Fan Speed: %s", fan_str);

        ac_fan_speed_t fan_speed = AC_FAN_AUTO;
        if (strcmp(fan_str, "Auto") == 0) fan_speed = AC_FAN_AUTO;
        else if (strcmp(fan_str, "Low") == 0) fan_speed = AC_FAN_LOW;
        else if (strcmp(fan_str, "Medium") == 0) fan_speed = AC_FAN_MEDIUM;
        else if (strcmp(fan_str, "High") == 0) fan_speed = AC_FAN_HIGH;

        return ir_ac_set_fan_speed(fan_speed);
    }
    /* AC Swing */
    else if (strcmp(param_name, "Swing") == 0) {
        bool swing = val.val.b;
        ESP_LOGI(TAG, "AC Swing: %s", swing ? "ON" : "OFF");
        return ir_ac_set_swing(swing ? AC_SWING_VERTICAL : AC_SWING_OFF);
    }
    /* AC Learn Protocol */
    else if (strcmp(param_name, "Learn_Protocol") == 0) {
        const char *protocol_str = val.val.s;
        ESP_LOGI(TAG, "AC Learn Protocol: %s", protocol_str);

        /* Auto-detect mode - trigger AC learning */
        if (strcmp(protocol_str, "Auto-Detect") == 0) {
            ESP_LOGI(TAG, "Starting AC protocol auto-detection...");
            rgb_led_set_status(LED_STATUS_LEARNING);

            esp_err_t err = ir_ac_learn_protocol(IR_LEARNING_TIMEOUT_MS);

            if (err == ESP_OK) {
                ESP_LOGI(TAG, "AC protocol learned successfully!");
                rgb_led_set_status(LED_STATUS_LEARN_SUCCESS);
                vTaskDelay(pdMS_TO_TICKS(1500));
            } else {
                ESP_LOGE(TAG, "AC protocol learning failed: %s", esp_err_to_name(err));
                rgb_led_set_status(LED_STATUS_LEARN_FAILED);
                vTaskDelay(pdMS_TO_TICKS(1500));
            }

            /* Return to connected state */
            if (app_wifi_is_connected()) {
                rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);
            } else {
                rgb_led_set_status(LED_STATUS_IDLE);
            }

            return err;
        }

        /* Manual protocol selection */
        ir_protocol_t protocol = IR_PROTOCOL_UNKNOWN;
        if (strcmp(protocol_str, "Daikin") == 0) protocol = IR_PROTOCOL_DAIKIN;
        else if (strcmp(protocol_str, "Carrier") == 0 || strcmp(protocol_str, "Voltas") == 0) {
            protocol = IR_PROTOCOL_CARRIER;
        }
        else if (strcmp(protocol_str, "Hitachi") == 0) protocol = IR_PROTOCOL_HITACHI;
        else if (strcmp(protocol_str, "Mitsubishi") == 0) protocol = IR_PROTOCOL_MITSUBISHI;
        else if (strcmp(protocol_str, "Midea") == 0) protocol = IR_PROTOCOL_MIDEA;
        else if (strcmp(protocol_str, "Haier") == 0) protocol = IR_PROTOCOL_HAIER;
        else if (strcmp(protocol_str, "Samsung48") == 0) protocol = IR_PROTOCOL_SAMSUNG48;
        else if (strcmp(protocol_str, "Panasonic") == 0) protocol = IR_PROTOCOL_PANASONIC;
        else if (strcmp(protocol_str, "Fujitsu") == 0) protocol = IR_PROTOCOL_FUJITSU;
        else if (strcmp(protocol_str, "LG2") == 0) protocol = IR_PROTOCOL_LG2;

        if (protocol != IR_PROTOCOL_UNKNOWN) {
            esp_err_t err = ir_ac_set_protocol(protocol, 0);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "AC protocol manually set to: %s", protocol_str);
            }
            return err;
        }

        ESP_LOGW(TAG, "Unknown protocol: %s", protocol_str);
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

/* ============================================================================
 * SPEAKER DEVICE CALLBACKS
 * ============================================================================ */

static esp_err_t speaker_write_cb(const esp_rmaker_device_t *device,
                                    const esp_rmaker_param_t *param,
                                    const esp_rmaker_param_val_t val,
                                    void *priv_data,
                                    esp_rmaker_write_ctx_t *ctx)
{
    const char *param_name = esp_rmaker_param_get_name(param);
    ESP_LOGI(TAG, "Speaker parameter update: %s", param_name);

    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        return ir_action_execute(IR_DEVICE_SPEAKER, IR_ACTION_POWER);
    } else if (strcmp(param_name, "Volume") == 0) {
        int volume = val.val.i;
        ESP_LOGI(TAG, "Speaker Volume: %d (use Vol+/Vol- actions)", volume);
        /* Volume tracking not implemented */
        return ESP_OK;
    } else if (strcmp(param_name, "Mute") == 0) {
        return ir_action_execute(IR_DEVICE_SPEAKER, IR_ACTION_MUTE);
    }

    return ESP_OK;
}

/* ============================================================================
 * FAN DEVICE CALLBACKS
 * ============================================================================ */

static esp_err_t fan_write_cb(const esp_rmaker_device_t *device,
                                const esp_rmaker_param_t *param,
                                const esp_rmaker_param_val_t val,
                                void *priv_data,
                                esp_rmaker_write_ctx_t *ctx)
{
    const char *param_name = esp_rmaker_param_get_name(param);
    ESP_LOGI(TAG, "Fan parameter update: %s", param_name);

    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        return ir_action_execute(IR_DEVICE_FAN, IR_ACTION_POWER);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_SPEED_NAME) == 0) {
        int speed = val.val.i;
        ESP_LOGI(TAG, "Fan Speed: %d", speed);

        /* Map speed to discrete actions */
        ir_action_t action = IR_ACTION_FAN_SPEED_1;
        switch (speed) {
            case 1: action = IR_ACTION_FAN_SPEED_1; break;
            case 2: action = IR_ACTION_FAN_SPEED_2; break;
            case 3: action = IR_ACTION_FAN_SPEED_3; break;
            case 4: action = IR_ACTION_FAN_SPEED_4; break;
            case 5: action = IR_ACTION_FAN_SPEED_5; break;
            default: action = IR_ACTION_FAN_SPEED_3; break;
        }

        return ir_action_execute(IR_DEVICE_FAN, action);
    } else if (strcmp(param_name, "Swing") == 0) {
        return ir_action_execute(IR_DEVICE_FAN, IR_ACTION_FAN_SWING);
    }

    return ESP_OK;
}

/* ============================================================================
 * STB DEVICE CALLBACKS
 * ============================================================================ */

static esp_err_t stb_write_cb(const esp_rmaker_device_t *device,
                                const esp_rmaker_param_t *param,
                                const esp_rmaker_param_val_t val,
                                void *priv_data,
                                esp_rmaker_write_ctx_t *ctx)
{
    const char *param_name = esp_rmaker_param_get_name(param);
    ESP_LOGI(TAG, "STB parameter update: %s", param_name);

    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        return ir_action_execute(IR_DEVICE_STB, IR_ACTION_POWER);
    } else if (strcmp(param_name, "Channel") == 0) {
        int channel = val.val.i;
        ESP_LOGI(TAG, "STB Channel: %d (use Ch+/Ch- actions)", channel);
        /* Channel tracking not implemented */
        return ESP_OK;
    } else if (strcmp(param_name, "Play_Pause") == 0) {
        return ir_action_execute(IR_DEVICE_STB, IR_ACTION_STB_PLAY_PAUSE);
    } else if (strcmp(param_name, "Guide") == 0) {
        return ir_action_execute(IR_DEVICE_STB, IR_ACTION_STB_GUIDE);
    }

    return ESP_OK;
}

/* ============================================================================
 * CUSTOM DEVICE CALLBACKS
 * ============================================================================ */

static esp_err_t custom_write_cb(const esp_rmaker_device_t *device,
                                   const esp_rmaker_param_t *param,
                                   const esp_rmaker_param_val_t val,
                                   void *priv_data,
                                   esp_rmaker_write_ctx_t *ctx)
{
    const char *param_name = esp_rmaker_param_get_name(param);
    ESP_LOGI(TAG, "Custom device parameter update: %s", param_name);

    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_POWER);
    }
    /* Button 1-12 */
    else if (strcmp(param_name, "Button_1") == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_CUSTOM_1);
    }
    else if (strcmp(param_name, "Button_2") == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_CUSTOM_2);
    }
    else if (strcmp(param_name, "Button_3") == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_CUSTOM_3);
    }
    else if (strcmp(param_name, "Button_4") == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_CUSTOM_4);
    }
    else if (strcmp(param_name, "Button_5") == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_CUSTOM_5);
    }
    else if (strcmp(param_name, "Button_6") == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_CUSTOM_6);
    }
    else if (strcmp(param_name, "Button_7") == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_CUSTOM_7);
    }
    else if (strcmp(param_name, "Button_8") == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_CUSTOM_8);
    }
    else if (strcmp(param_name, "Button_9") == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_CUSTOM_9);
    }
    else if (strcmp(param_name, "Button_10") == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_CUSTOM_10);
    }
    else if (strcmp(param_name, "Button_11") == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_CUSTOM_11);
    }
    else if (strcmp(param_name, "Button_12") == 0) {
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_CUSTOM_12);
    }
    /* Learn Mode */
    else if (strcmp(param_name, "Learn_Mode") == 0) {
        const char *action_name = val.val.s;
        ESP_LOGI(TAG, "Custom Learn Mode: %s", action_name);

        /* Map action name to ir_action_t */
        ir_action_t action = IR_ACTION_NONE;
        if (strcmp(action_name, "Power") == 0) action = IR_ACTION_POWER;
        else if (strcmp(action_name, "Button1") == 0) action = IR_ACTION_CUSTOM_1;
        else if (strcmp(action_name, "Button2") == 0) action = IR_ACTION_CUSTOM_2;
        else if (strcmp(action_name, "Button3") == 0) action = IR_ACTION_CUSTOM_3;
        else if (strcmp(action_name, "Button4") == 0) action = IR_ACTION_CUSTOM_4;
        else if (strcmp(action_name, "Button5") == 0) action = IR_ACTION_CUSTOM_5;
        else if (strcmp(action_name, "Button6") == 0) action = IR_ACTION_CUSTOM_6;
        else if (strcmp(action_name, "Button7") == 0) action = IR_ACTION_CUSTOM_7;
        else if (strcmp(action_name, "Button8") == 0) action = IR_ACTION_CUSTOM_8;
        else if (strcmp(action_name, "Button9") == 0) action = IR_ACTION_CUSTOM_9;
        else if (strcmp(action_name, "Button10") == 0) action = IR_ACTION_CUSTOM_10;
        else if (strcmp(action_name, "Button11") == 0) action = IR_ACTION_CUSTOM_11;
        else if (strcmp(action_name, "Button12") == 0) action = IR_ACTION_CUSTOM_12;
        else {
            ESP_LOGW(TAG, "Unknown action: %s", action_name);
            return ESP_ERR_INVALID_ARG;
        }

        /* Start learning */
        learning_state.device = IR_DEVICE_CUSTOM;
        learning_state.action = action;
        learning_state.is_active = true;

        rgb_led_set_status(LED_STATUS_LEARNING);
        return ir_learn_start(IR_BTN_CUSTOM_1, IR_LEARNING_TIMEOUT_MS);
    }

    return ESP_OK;
}

/* ============================================================================
 * RAINMAKER DEVICE CREATION
 * ============================================================================ */

static esp_err_t create_tv_device(esp_rmaker_node_t *node)
{
    tv_device = esp_rmaker_tv_device_create("TV", NULL, true);
    if (!tv_device) {
        ESP_LOGE(TAG, "Failed to create TV device");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_cb(tv_device, tv_write_cb, NULL);

    /* Add TV-specific parameters */
    esp_rmaker_device_add_param(tv_device, esp_rmaker_name_param_create("Name", "TV"));

    esp_rmaker_param_t *volume = esp_rmaker_param_create("Volume", "esp.param.range",
                                                           esp_rmaker_int(50), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_bounds(volume, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
    esp_rmaker_device_add_param(tv_device, volume);

    esp_rmaker_param_t *mute = esp_rmaker_param_create("Mute", "esp.param.toggle",
                                                         esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_device_add_param(tv_device, mute);

    esp_rmaker_param_t *channel = esp_rmaker_param_create("Channel", "esp.param.range",
                                                            esp_rmaker_int(1), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_bounds(channel, esp_rmaker_int(1), esp_rmaker_int(999), esp_rmaker_int(1));
    esp_rmaker_device_add_param(tv_device, channel);

    esp_rmaker_param_t *input = esp_rmaker_param_create("Input", "esp.param.string",
                                                          esp_rmaker_str("HDMI1"), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_device_add_param(tv_device, input);

    /* Learning mode parameter */
    esp_rmaker_param_t *learn_mode = esp_rmaker_param_create("Learn_Mode", "esp.param.string",
                                                               esp_rmaker_str("None"), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(learn_mode, ESP_RMAKER_UI_DROPDOWN);
    esp_rmaker_device_add_param(tv_device, learn_mode);

    esp_rmaker_node_add_device(node, tv_device);
    ESP_LOGI(TAG, "TV device created");
    return ESP_OK;
}

static esp_err_t create_ac_device(esp_rmaker_node_t *node)
{
    ac_device = esp_rmaker_ac_device_create("AC", NULL, false);
    if (!ac_device) {
        ESP_LOGE(TAG, "Failed to create AC device");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_cb(ac_device, ac_write_cb, NULL);

    /* Get current AC state */
    const ac_state_t *state = ir_ac_state_get();

    /* Add AC-specific parameters */
    esp_rmaker_device_add_param(ac_device, esp_rmaker_name_param_create("Name", "AC"));

    /* Mode parameter */
    esp_rmaker_param_t *mode = esp_rmaker_param_create("Mode", "esp.param.mode",
                                                         esp_rmaker_str("Cool"), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(mode, ESP_RMAKER_UI_DROPDOWN);
    esp_rmaker_device_add_param(ac_device, mode);

    /* Temperature parameter */
    esp_rmaker_param_t *temp = esp_rmaker_temperature_param_create("Temperature",
                                                                     state->temperature);
    esp_rmaker_param_add_bounds(temp, esp_rmaker_float(AC_TEMP_MIN),
                                 esp_rmaker_float(AC_TEMP_MAX), esp_rmaker_float(1));
    esp_rmaker_device_add_param(ac_device, temp);

    /* Fan Speed parameter */
    esp_rmaker_param_t *fan_speed = esp_rmaker_param_create("Fan_Speed", "esp.param.mode",
                                                              esp_rmaker_str("Auto"), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(fan_speed, ESP_RMAKER_UI_DROPDOWN);
    esp_rmaker_device_add_param(ac_device, fan_speed);

    /* Swing parameter */
    esp_rmaker_param_t *swing = esp_rmaker_param_create("Swing", "esp.param.toggle",
                                                          esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_device_add_param(ac_device, swing);

    /* Protocol selection parameter */
    esp_rmaker_param_t *learn_protocol = esp_rmaker_param_create("Learn_Protocol", "esp.param.string",
                                                                   esp_rmaker_str("Daikin"), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(learn_protocol, ESP_RMAKER_UI_DROPDOWN);
    esp_rmaker_device_add_param(ac_device, learn_protocol);

    esp_rmaker_node_add_device(node, ac_device);
    ESP_LOGI(TAG, "AC device created (Protocol: %s)",
             state->is_learned ? ir_get_protocol_name(state->protocol) : "Not configured");
    return ESP_OK;
}

static esp_err_t create_speaker_device(esp_rmaker_node_t *node)
{
    speaker_device = esp_rmaker_speaker_device_create("Soundbar", NULL, true);
    if (!speaker_device) {
        ESP_LOGE(TAG, "Failed to create Speaker device");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_cb(speaker_device, speaker_write_cb, NULL);
    esp_rmaker_device_add_param(speaker_device, esp_rmaker_name_param_create("Name", "Soundbar"));

    esp_rmaker_node_add_device(node, speaker_device);
    ESP_LOGI(TAG, "Speaker device created");
    return ESP_OK;
}

static esp_err_t create_fan_device(esp_rmaker_node_t *node)
{
    fan_device = esp_rmaker_fan_device_create("Fan", NULL, false);
    if (!fan_device) {
        ESP_LOGE(TAG, "Failed to create Fan device");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_cb(fan_device, fan_write_cb, NULL);
    esp_rmaker_device_add_param(fan_device, esp_rmaker_name_param_create("Name", "Fan"));

    /* Fan speed */
    esp_rmaker_param_t *speed = esp_rmaker_speed_param_create("Speed", 3);
    esp_rmaker_param_add_bounds(speed, esp_rmaker_int(1), esp_rmaker_int(5), esp_rmaker_int(1));
    esp_rmaker_device_add_param(fan_device, speed);

    /* Swing */
    esp_rmaker_param_t *swing = esp_rmaker_param_create("Swing", "esp.param.toggle",
                                                          esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_device_add_param(fan_device, swing);

    esp_rmaker_node_add_device(node, fan_device);
    ESP_LOGI(TAG, "Fan device created");
    return ESP_OK;
}

static esp_err_t create_stb_device(esp_rmaker_node_t *node)
{
    stb_device = esp_rmaker_device_create("STB", ESP_RMAKER_DEVICE_OTHER, NULL);
    if (!stb_device) {
        ESP_LOGE(TAG, "Failed to create STB device");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_cb(stb_device, stb_write_cb, NULL);
    esp_rmaker_device_add_param(stb_device, esp_rmaker_name_param_create("Name", "Set-Top Box"));
    esp_rmaker_device_add_param(stb_device, esp_rmaker_power_param_create("Power", false));

    /* Channel */
    esp_rmaker_param_t *channel = esp_rmaker_param_create("Channel", "esp.param.range",
                                                            esp_rmaker_int(1), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_device_add_param(stb_device, channel);

    /* Play/Pause */
    esp_rmaker_param_t *play_pause = esp_rmaker_param_create("Play_Pause", "esp.param.toggle",
                                                               esp_rmaker_bool(false), PROP_FLAG_WRITE);
    esp_rmaker_device_add_param(stb_device, play_pause);

    /* Guide */
    esp_rmaker_param_t *guide = esp_rmaker_param_create("Guide", "esp.param.toggle",
                                                          esp_rmaker_bool(false), PROP_FLAG_WRITE);
    esp_rmaker_device_add_param(stb_device, guide);

    esp_rmaker_node_add_device(node, stb_device);
    ESP_LOGI(TAG, "STB device created");
    return ESP_OK;
}

static esp_err_t create_custom_device(esp_rmaker_node_t *node)
{
    custom_device = esp_rmaker_device_create("Custom", ESP_RMAKER_DEVICE_OTHER, NULL);
    if (!custom_device) {
        ESP_LOGE(TAG, "Failed to create Custom device");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_cb(custom_device, custom_write_cb, NULL);
    esp_rmaker_device_add_param(custom_device, esp_rmaker_name_param_create("Name", "Custom Device"));
    esp_rmaker_device_add_param(custom_device, esp_rmaker_power_param_create("Power", false));

    /* Button 1-12 */
    for (int i = 1; i <= 12; i++) {
        char button_name[16];
        snprintf(button_name, sizeof(button_name), "Button_%d", i);
        esp_rmaker_param_t *button = esp_rmaker_param_create(button_name, "esp.param.toggle",
                                                               esp_rmaker_bool(false), PROP_FLAG_WRITE);
        esp_rmaker_device_add_param(custom_device, button);
    }

    /* Learning mode parameter */
    esp_rmaker_param_t *learn_mode = esp_rmaker_param_create("Learn_Mode", "esp.param.string",
                                                               esp_rmaker_str("None"), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(learn_mode, ESP_RMAKER_UI_DROPDOWN);
    esp_rmaker_device_add_param(custom_device, learn_mode);

    esp_rmaker_node_add_device(node, custom_device);
    ESP_LOGI(TAG, "Custom device created");
    return ESP_OK;
}

/* ============================================================================
 * BOOT BUTTON HANDLER (WiFi Reset / Factory Reset)
 * ============================================================================ */

static void boot_button_timer_cb(TimerHandle_t timer)
{
    uint32_t press_duration = (esp_log_timestamp() - button_press_start);

    if (gpio_get_level(GPIO_BOOT_BUTTON) == 0) {
        if (press_duration >= BUTTON_FACTORY_RESET_MS && !factory_reset_triggered) {
            ESP_LOGW(TAG, "Factory reset triggered!");
            factory_reset_triggered = true;
            rgb_led_set_status(LED_STATUS_ERROR);

            /* Clear all IR codes */
            ir_action_clear_all();
            ir_ac_clear_state();

            /* Reset WiFi and restart */
            esp_rmaker_factory_reset(0, 2);
        }
    } else {
        if (press_duration >= BUTTON_WIFI_RESET_MS && press_duration < BUTTON_FACTORY_RESET_MS) {
            ESP_LOGI(TAG, "WiFi reset triggered");
            rgb_led_set_status(LED_STATUS_ERROR);
            app_wifi_reset();
        }

        xTimerStop(timer, 0);
        factory_reset_triggered = false;
    }
}

static void IRAM_ATTR boot_button_isr_handler(void *arg)
{
    if (gpio_get_level(GPIO_BOOT_BUTTON) == 0) {
        button_press_start = esp_log_timestamp();
        factory_reset_triggered = false;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xTimerStartFromISR(boot_button_timer, &xHigherPriorityTaskWoken);
    }
}

static void init_boot_button(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_BOOT_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf);

    boot_button_timer = xTimerCreate("boot_btn", pdMS_TO_TICKS(100), pdTRUE,
                                      NULL, boot_button_timer_cb);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_BOOT_BUTTON, boot_button_isr_handler, NULL);

    ESP_LOGI(TAG, "Boot button initialized (GPIO%d)", GPIO_BOOT_BUTTON);
    ESP_LOGI(TAG, "  - Press 3s: WiFi reset");
    ESP_LOGI(TAG, "  - Press 10s: Factory reset");
}

/* ============================================================================
 * MAIN APPLICATION
 * ============================================================================ */

void app_main(void)
{
    esp_err_t err = ESP_OK;

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Universal IR Remote Control v3.0");
    ESP_LOGI(TAG, "  Multi-Device Architecture");
    ESP_LOGI(TAG, "  Firmware: %s", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "========================================");

    /* Initialize NVS */
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* Initialize event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize RGB LED */
    rgb_led_config_t led_config = {
        .gpio_num = GPIO_RGB_LED,
        .led_count = RGB_LED_COUNT,
        .rmt_channel = RGB_LED_RMT_CHANNEL
    };
    ESP_ERROR_CHECK(rgb_led_init(&led_config));
    rgb_led_set_status(LED_STATUS_IDLE);

    /* Initialize IR control */
    ESP_LOGI(TAG, "Initializing IR control...");
    ESP_ERROR_CHECK(ir_control_init());

    /* Initialize action mapping system */
    ESP_LOGI(TAG, "Initializing action mapping system...");
    ESP_ERROR_CHECK(ir_action_init());

    /* Initialize AC state management */
    ESP_LOGI(TAG, "Initializing AC state management...");
    ESP_ERROR_CHECK(ir_ac_state_init());

    /* Register IR callbacks */
    ir_callbacks_t ir_callbacks = {
        .learn_success_cb = ir_learn_success_callback,
        .learn_fail_cb = ir_learn_fail_callback,
        .receive_cb = NULL,
        .user_arg = NULL
    };
    ESP_ERROR_CHECK(ir_register_callbacks(&ir_callbacks));

    /* Initialize boot button */
    init_boot_button();

    /* Initialize RainMaker */
    ESP_LOGI(TAG, "Initializing ESP RainMaker...");
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, DEVICE_NAME, DEVICE_TYPE);
    if (!node) {
        ESP_LOGE(TAG, "Failed to initialize RainMaker node");
        abort();
    }

    /* Create all devices */
    ESP_LOGI(TAG, "Creating RainMaker devices...");
    ESP_ERROR_CHECK(create_tv_device(node));
    ESP_ERROR_CHECK(create_ac_device(node));
    ESP_ERROR_CHECK(create_stb_device(node));
    ESP_ERROR_CHECK(create_speaker_device(node));
    ESP_ERROR_CHECK(create_fan_device(node));
    ESP_ERROR_CHECK(create_custom_device(node));

    /* Enable OTA */
    esp_rmaker_ota_enable_default();

    /* Enable scheduling */
    esp_rmaker_schedule_enable();

    /* Enable timezone service */
    esp_rmaker_timezone_service_enable();

    /* Start RainMaker */
    ESP_LOGI(TAG, "Starting ESP RainMaker...");
    ESP_ERROR_CHECK(esp_rmaker_start());

    /* Initialize WiFi with BLE provisioning */
    ESP_LOGI(TAG, "Initializing WiFi...");
    ESP_ERROR_CHECK(app_wifi_init("abcd1234")); // Default PoP

    /* Initialize console for debugging */
    esp_rmaker_console_init();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Universal IR Remote Ready!");
    ESP_LOGI(TAG, "  Devices: TV, AC, STB, Speaker, Fan, Custom");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Use RainMaker app to provision and control");
}
