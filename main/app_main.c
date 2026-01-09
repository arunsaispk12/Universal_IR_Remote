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
#include "esp_rmaker_utils.h"
#include "rmaker_devices.h"

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
static esp_rmaker_device_t *custom_device_2 = NULL;
static esp_rmaker_device_t *custom_device_3 = NULL;

static esp_rmaker_node_t *rainmaker_node = NULL;
static bool devices_created = false;

/* Boot button handling */
static TimerHandle_t boot_button_timer = NULL;
static uint32_t button_press_start = 0;
static bool factory_reset_triggered = false;
static TaskHandle_t reset_handler_task_handle = NULL;

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
        rgb_led_set_mode(LED_MODE_IR_LEARNING_SUCCESS);
    } else {
        ESP_LOGE(TAG, "Failed to save IR code");
        rgb_led_set_mode(LED_MODE_WIFI_ERROR);
    }

    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Reset learning state */
    learning_state.is_active = false;
    learning_state.device = IR_DEVICE_NONE;
    learning_state.action = IR_ACTION_NONE;
    ir_action_cancel_learning();  // Reset state in ir_action module

    /* Return to connected state */
    if (app_wifi_is_connected()) {
        rgb_led_set_mode(LED_MODE_WIFI_CONNECTED);
    } else {
        rgb_led_set_mode(LED_MODE_OFF);
    }
}

/**
 * @brief Callback when IR learning fails
 */
static void ir_learn_fail_callback(ir_button_t button, void *arg)
{
    ESP_LOGW(TAG, "IR learning failed (timeout)");

    /* Cancel learning in action mapper */
    ir_action_cancel_learning();

    rgb_led_set_mode(LED_MODE_IR_LEARNING_FAILED);
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Reset learning state */
    learning_state.is_active = false;
    learning_state.device = IR_DEVICE_NONE;
    learning_state.action = IR_ACTION_NONE;

    /* Return to connected state */
    if (app_wifi_is_connected()) {
        rgb_led_set_mode(LED_MODE_WIFI_CONNECTED);
    } else {
        rgb_led_set_mode(LED_MODE_OFF);
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

    /* TRIGGER BUTTONS - Execute IR action immediately when pressed */
    if (strcmp(param_name, "Power") == 0) {
        ESP_LOGI(TAG, "TV Power button pressed");
        return ir_action_execute(IR_DEVICE_TV, IR_ACTION_POWER);
    }
    else if (strcmp(param_name, "Vol_Up") == 0) {
        ESP_LOGI(TAG, "TV Volume Up button pressed");
        return ir_action_execute(IR_DEVICE_TV, IR_ACTION_VOL_UP);
    }
    else if (strcmp(param_name, "Vol_Down") == 0) {
        ESP_LOGI(TAG, "TV Volume Down button pressed");
        return ir_action_execute(IR_DEVICE_TV, IR_ACTION_VOL_DOWN);
    }
    else if (strcmp(param_name, "Mute") == 0) {
        ESP_LOGI(TAG, "TV Mute button pressed");
        return ir_action_execute(IR_DEVICE_TV, IR_ACTION_MUTE);
    }
    else if (strcmp(param_name, "Ch_Up") == 0) {
        ESP_LOGI(TAG, "TV Channel Up button pressed");
        return ir_action_execute(IR_DEVICE_TV, IR_ACTION_CH_UP);
    }
    else if (strcmp(param_name, "Ch_Down") == 0) {
        ESP_LOGI(TAG, "TV Channel Down button pressed");
        return ir_action_execute(IR_DEVICE_TV, IR_ACTION_CH_DOWN);
    }
    else if (strcmp(param_name, "Input") == 0) {
        ESP_LOGI(TAG, "TV Input button pressed");
        return ir_action_execute(IR_DEVICE_TV, IR_ACTION_TV_INPUT);
    }
    else if (strcmp(param_name, "Menu") == 0) {
        ESP_LOGI(TAG, "TV Menu button pressed");
        return ir_action_execute(IR_DEVICE_TV, IR_ACTION_MENU);
    }
    else if (strcmp(param_name, "OK") == 0) {
        ESP_LOGI(TAG, "TV OK button pressed");
        return ir_action_execute(IR_DEVICE_TV, IR_ACTION_NAV_OK);
    }
    else if (strcmp(param_name, "Back") == 0) {
        ESP_LOGI(TAG, "TV Back button pressed");
        return ir_action_execute(IR_DEVICE_TV, IR_ACTION_BACK);
    }
    /* Learn Mode - Dropdown selection */
    else if (strcmp(param_name, "Learn_Mode") == 0) {
        const char *action_name = val.val.s;
        ESP_LOGI(TAG, "TV Learn Mode: %s", action_name);

        /* Map dropdown selection to ir_action_t */
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

        rgb_led_set_mode(LED_MODE_IR_LEARNING);
        return ir_action_learn(IR_DEVICE_TV, action, IR_LEARNING_TIMEOUT_MS);
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
            rgb_led_set_mode(LED_MODE_IR_LEARNING);

            esp_err_t err = ir_ac_learn_protocol(IR_LEARNING_TIMEOUT_MS);

            if (err == ESP_OK) {
                ESP_LOGI(TAG, "AC protocol learned successfully!");
                rgb_led_set_mode(LED_MODE_IR_LEARNING_SUCCESS);
                vTaskDelay(pdMS_TO_TICKS(1500));
            } else {
                ESP_LOGE(TAG, "AC protocol learning failed: %s", esp_err_to_name(err));
                rgb_led_set_mode(LED_MODE_IR_LEARNING_FAILED);
                vTaskDelay(pdMS_TO_TICKS(1500));
            }

            /* Return to connected state */
            if (app_wifi_is_connected()) {
                rgb_led_set_mode(LED_MODE_WIFI_CONNECTED);
            } else {
                rgb_led_set_mode(LED_MODE_OFF);
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
        else if (strcmp(protocol_str, "Samsung") == 0) protocol = IR_PROTOCOL_SAMSUNG48;
        else if (strcmp(protocol_str, "LG") == 0) protocol = IR_PROTOCOL_LG2;

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

    /* TRIGGER BUTTONS - Execute IR action immediately when pressed */
    if (strcmp(param_name, "Power") == 0) {
        ESP_LOGI(TAG, "Soundbar Power button pressed");
        return ir_action_execute(IR_DEVICE_SPEAKER, IR_ACTION_POWER);
    }
    else if (strcmp(param_name, "Vol_Up") == 0) {
        ESP_LOGI(TAG, "Soundbar Volume Up button pressed");
        return ir_action_execute(IR_DEVICE_SPEAKER, IR_ACTION_VOL_UP);
    }
    else if (strcmp(param_name, "Vol_Down") == 0) {
        ESP_LOGI(TAG, "Soundbar Volume Down button pressed");
        return ir_action_execute(IR_DEVICE_SPEAKER, IR_ACTION_VOL_DOWN);
    }
    else if (strcmp(param_name, "Mute") == 0) {
        ESP_LOGI(TAG, "Soundbar Mute button pressed");
        return ir_action_execute(IR_DEVICE_SPEAKER, IR_ACTION_MUTE);
    }
    /* Learn Mode - Dropdown selection */
    else if (strcmp(param_name, "Learn_Mode") == 0) {
        const char *action_name = val.val.s;
        ESP_LOGI(TAG, "Soundbar Learn Mode: %s", action_name);

        /* Map dropdown selection to ir_action_t */
        ir_action_t action = IR_ACTION_NONE;
        if (strcmp(action_name, "Power") == 0) action = IR_ACTION_POWER;
        else if (strcmp(action_name, "VolumeUp") == 0) action = IR_ACTION_VOL_UP;
        else if (strcmp(action_name, "VolumeDown") == 0) action = IR_ACTION_VOL_DOWN;
        else if (strcmp(action_name, "Mute") == 0) action = IR_ACTION_MUTE;
        else {
            ESP_LOGW(TAG, "Unknown action: %s", action_name);
            return ESP_ERR_INVALID_ARG;
        }

        /* Start learning */
        learning_state.device = IR_DEVICE_SPEAKER;
        learning_state.action = action;
        learning_state.is_active = true;

        rgb_led_set_mode(LED_MODE_IR_LEARNING);
        return ir_action_learn(IR_DEVICE_SPEAKER, action, IR_LEARNING_TIMEOUT_MS);
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

    /* TRIGGER BUTTONS - Execute IR action immediately when pressed */
    if (strcmp(param_name, "Power") == 0) {
        ESP_LOGI(TAG, "STB Power button pressed");
        return ir_action_execute(IR_DEVICE_STB, IR_ACTION_POWER);
    }
    else if (strcmp(param_name, "Ch_Up") == 0) {
        ESP_LOGI(TAG, "STB Channel Up button pressed");
        return ir_action_execute(IR_DEVICE_STB, IR_ACTION_CH_UP);
    }
    else if (strcmp(param_name, "Ch_Down") == 0) {
        ESP_LOGI(TAG, "STB Channel Down button pressed");
        return ir_action_execute(IR_DEVICE_STB, IR_ACTION_CH_DOWN);
    }
    else if (strcmp(param_name, "Play_Pause") == 0) {
        ESP_LOGI(TAG, "STB Play/Pause button pressed");
        return ir_action_execute(IR_DEVICE_STB, IR_ACTION_STB_PLAY_PAUSE);
    }
    else if (strcmp(param_name, "Guide") == 0) {
        ESP_LOGI(TAG, "STB Guide button pressed");
        return ir_action_execute(IR_DEVICE_STB, IR_ACTION_STB_GUIDE);
    }
    else if (strcmp(param_name, "Menu") == 0) {
        ESP_LOGI(TAG, "STB Menu button pressed");
        return ir_action_execute(IR_DEVICE_STB, IR_ACTION_MENU);
    }
    else if (strcmp(param_name, "OK") == 0) {
        ESP_LOGI(TAG, "STB OK button pressed");
        return ir_action_execute(IR_DEVICE_STB, IR_ACTION_NAV_OK);
    }
    else if (strcmp(param_name, "Back") == 0) {
        ESP_LOGI(TAG, "STB Back button pressed");
        return ir_action_execute(IR_DEVICE_STB, IR_ACTION_BACK);
    }
    /* Learn Mode - Dropdown selection */
    else if (strcmp(param_name, "Learn_Mode") == 0) {
        const char *action_name = val.val.s;
        ESP_LOGI(TAG, "STB Learn Mode: %s", action_name);

        /* Map dropdown selection to ir_action_t */
        ir_action_t action = IR_ACTION_NONE;
        if (strcmp(action_name, "Power") == 0) action = IR_ACTION_POWER;
        else if (strcmp(action_name, "ChannelUp") == 0) action = IR_ACTION_CH_UP;
        else if (strcmp(action_name, "ChannelDown") == 0) action = IR_ACTION_CH_DOWN;
        else if (strcmp(action_name, "PlayPause") == 0) action = IR_ACTION_STB_PLAY_PAUSE;
        else if (strcmp(action_name, "Guide") == 0) action = IR_ACTION_STB_GUIDE;
        else if (strcmp(action_name, "Menu") == 0) action = IR_ACTION_MENU;
        else if (strcmp(action_name, "OK") == 0) action = IR_ACTION_NAV_OK;
        else if (strcmp(action_name, "Back") == 0) action = IR_ACTION_BACK;
        else {
            ESP_LOGW(TAG, "Unknown action: %s", action_name);
            return ESP_ERR_INVALID_ARG;
        }

        /* Start learning */
        learning_state.device = IR_DEVICE_STB;
        learning_state.action = action;
        learning_state.is_active = true;

        rgb_led_set_mode(LED_MODE_IR_LEARNING);
        return ir_action_learn(IR_DEVICE_STB, action, IR_LEARNING_TIMEOUT_MS);
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

    /* TRIGGER BUTTONS - Execute IR action immediately when pressed */
    if (strcmp(param_name, "Power") == 0) {
        ESP_LOGI(TAG, "Custom Power button pressed");
        return ir_action_execute(IR_DEVICE_CUSTOM, IR_ACTION_POWER);
    }
    /* Button 1-12 */
    else if (strcmp(param_name, "Button_1") == 0) {
        ESP_LOGI(TAG, "Custom Button 1 pressed");
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

        rgb_led_set_mode(LED_MODE_IR_LEARNING);
        return ir_action_learn(IR_DEVICE_CUSTOM, action, IR_LEARNING_TIMEOUT_MS);
    }

    return ESP_OK;
}

/* ============================================================================
 * RAINMAKER DEVICE CREATION
 * ============================================================================ */

static esp_err_t create_tv_device(esp_rmaker_node_t *node)
{
    /* Create as generic device for full control over UI */
    tv_device = esp_rmaker_device_create("TV Remote", ESP_RMAKER_DEVICE_OTHER, NULL);
    if (!tv_device) {
        ESP_LOGE(TAG, "Failed to create TV device");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_cb(tv_device, tv_write_cb, NULL);
    esp_rmaker_device_add_param(tv_device, esp_rmaker_name_param_create("Name", "TV Remote"));

    /* Remote control buttons - all as TRIGGERS (momentary push buttons) */
    esp_rmaker_param_t *power_btn = esp_rmaker_param_create("Power", "esp.param.toggle",
                                                              esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(power_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(tv_device, power_btn);

    esp_rmaker_param_t *vol_up_btn = esp_rmaker_param_create("Vol_Up", "esp.param.toggle",
                                                               esp_rmaker_bool(false), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(vol_up_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(tv_device, vol_up_btn);

    esp_rmaker_param_t *vol_down_btn = esp_rmaker_param_create("Vol_Down", "esp.param.toggle",
                                                                 esp_rmaker_bool(false), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(vol_down_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(tv_device, vol_down_btn);

    esp_rmaker_param_t *mute_btn = esp_rmaker_param_create("Mute", "esp.param.toggle",
                                                             esp_rmaker_bool(false), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(mute_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(tv_device, mute_btn);

    esp_rmaker_param_t *ch_up_btn = esp_rmaker_param_create("Ch_Up", "esp.param.toggle",
                                                              esp_rmaker_bool(false), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(ch_up_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(tv_device, ch_up_btn);

    esp_rmaker_param_t *ch_down_btn = esp_rmaker_param_create("Ch_Down", "esp.param.toggle",
                                                                esp_rmaker_bool(false), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(ch_down_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(tv_device, ch_down_btn);

    esp_rmaker_param_t *input_btn = esp_rmaker_param_create("Input", "esp.param.toggle",
                                                              esp_rmaker_bool(false), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(input_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(tv_device, input_btn);

    esp_rmaker_param_t *menu_btn = esp_rmaker_param_create("Menu", "esp.param.toggle",
                                                             esp_rmaker_bool(false), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(menu_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(tv_device, menu_btn);

    esp_rmaker_param_t *ok_btn = esp_rmaker_param_create("OK", "esp.param.toggle",
                                                           esp_rmaker_bool(false), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(ok_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(tv_device, ok_btn);

    esp_rmaker_param_t *back_btn = esp_rmaker_param_create("Back", "esp.param.toggle",
                                                             esp_rmaker_bool(false), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(back_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(tv_device, back_btn);

    /* Learning mode dropdown */
    esp_rmaker_param_t *learn_mode = esp_rmaker_param_create("Learn_Mode", "esp.param.string",
                                                               esp_rmaker_str("None"), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(learn_mode, ESP_RMAKER_UI_DROPDOWN);
    static const char *learn_options[] = {"None", "Power", "VolumeUp", "VolumeDown", "Mute",
                                           "ChannelUp", "ChannelDown", "Input", "Menu", "OK", "Back"};
    esp_rmaker_param_add_valid_str_list(learn_mode, learn_options, 11);
    esp_rmaker_device_add_param(tv_device, learn_mode);

    esp_rmaker_node_add_device(node, tv_device);
    ESP_LOGI(TAG, "TV Remote device created with push button controls");
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
    static const char *mode_options[] = {"Off", "Cool", "Heat", "Dry", "Fan", "Auto"};
    esp_rmaker_param_add_valid_str_list(mode, mode_options, 6);
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
    static const char *fan_speed_options[] = {"Auto", "Low", "Medium", "High"};
    esp_rmaker_param_add_valid_str_list(fan_speed, fan_speed_options, 4);
    esp_rmaker_device_add_param(ac_device, fan_speed);

    /* Swing parameter */
    esp_rmaker_param_t *swing = esp_rmaker_param_create("Swing", "esp.param.toggle",
                                                          esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_device_add_param(ac_device, swing);

    /* Protocol selection parameter */
    esp_rmaker_param_t *learn_protocol = esp_rmaker_param_create("Learn_Protocol", "esp.param.string",
                                                                   esp_rmaker_str("Daikin"), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(learn_protocol, ESP_RMAKER_UI_DROPDOWN);
    static const char *protocol_options[] = {"Daikin", "Mitsubishi", "LG", "Samsung", "Panasonic", "Hitachi"};
    esp_rmaker_param_add_valid_str_list(learn_protocol, protocol_options, 6);
    esp_rmaker_device_add_param(ac_device, learn_protocol);

    esp_rmaker_node_add_device(node, ac_device);
    ESP_LOGI(TAG, "AC device created (Protocol: %s)",
             state->is_learned ? ir_get_protocol_name(state->protocol) : "Not configured");
    return ESP_OK;
}

static esp_err_t create_speaker_device(esp_rmaker_node_t *node)
{
    /* Create as generic device for full control over UI */
    speaker_device = esp_rmaker_device_create("Soundbar Remote", ESP_RMAKER_DEVICE_OTHER, NULL);
    if (!speaker_device) {
        ESP_LOGE(TAG, "Failed to create Speaker device");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_cb(speaker_device, speaker_write_cb, NULL);
    esp_rmaker_device_add_param(speaker_device, esp_rmaker_name_param_create("Name", "Soundbar Remote"));

    /* Remote control buttons - all as TRIGGERS (momentary push buttons) */
    esp_rmaker_param_t *power_btn = esp_rmaker_param_create("Power", "esp.param.toggle",
                                                              esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(power_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(speaker_device, power_btn);

    esp_rmaker_param_t *vol_up_btn = esp_rmaker_param_create("Vol_Up", "esp.param.toggle",
                                                               esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(vol_up_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(speaker_device, vol_up_btn);

    esp_rmaker_param_t *vol_down_btn = esp_rmaker_param_create("Vol_Down", "esp.param.toggle",
                                                                 esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(vol_down_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(speaker_device, vol_down_btn);

    esp_rmaker_param_t *mute_btn = esp_rmaker_param_create("Mute", "esp.param.toggle",
                                                             esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(mute_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(speaker_device, mute_btn);

    /* Learning mode dropdown */
    esp_rmaker_param_t *learn_mode = esp_rmaker_param_create("Learn_Mode", "esp.param.string",
                                                               esp_rmaker_str("None"), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(learn_mode, ESP_RMAKER_UI_DROPDOWN);
    static const char *soundbar_learn_options[] = {"None", "Power", "VolumeUp", "VolumeDown", "Mute"};
    esp_rmaker_param_add_valid_str_list(learn_mode, soundbar_learn_options, 5);
    esp_rmaker_device_add_param(speaker_device, learn_mode);

    esp_rmaker_node_add_device(node, speaker_device);
    ESP_LOGI(TAG, "Soundbar Remote device created");
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

    /* Note: Name parameter is automatically added by esp_rmaker_fan_device_create() */

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
    /* Create as generic device for full control over UI */
    stb_device = esp_rmaker_device_create("STB Remote", ESP_RMAKER_DEVICE_OTHER, NULL);
    if (!stb_device) {
        ESP_LOGE(TAG, "Failed to create STB device");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_cb(stb_device, stb_write_cb, NULL);
    esp_rmaker_device_add_param(stb_device, esp_rmaker_name_param_create("Name", "STB Remote"));

    /* Remote control buttons - all as TRIGGERS (momentary push buttons) */
    esp_rmaker_param_t *power_btn = esp_rmaker_param_create("Power", "esp.param.toggle",
                                                              esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(power_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(stb_device, power_btn);

    esp_rmaker_param_t *ch_up_btn = esp_rmaker_param_create("Ch_Up", "esp.param.toggle",
                                                              esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(ch_up_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(stb_device, ch_up_btn);

    esp_rmaker_param_t *ch_down_btn = esp_rmaker_param_create("Ch_Down", "esp.param.toggle",
                                                                esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(ch_down_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(stb_device, ch_down_btn);

    esp_rmaker_param_t *play_pause_btn = esp_rmaker_param_create("Play_Pause", "esp.param.toggle",
                                                                   esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(play_pause_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(stb_device, play_pause_btn);

    esp_rmaker_param_t *guide_btn = esp_rmaker_param_create("Guide", "esp.param.toggle",
                                                              esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(guide_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(stb_device, guide_btn);

    esp_rmaker_param_t *menu_btn = esp_rmaker_param_create("Menu", "esp.param.toggle",
                                                             esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(menu_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(stb_device, menu_btn);

    esp_rmaker_param_t *ok_btn = esp_rmaker_param_create("OK", "esp.param.toggle",
                                                           esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(ok_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(stb_device, ok_btn);

    esp_rmaker_param_t *back_btn = esp_rmaker_param_create("Back", "esp.param.toggle",
                                                             esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(back_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(stb_device, back_btn);

    /* Learning mode dropdown */
    esp_rmaker_param_t *learn_mode = esp_rmaker_param_create("Learn_Mode", "esp.param.string",
                                                               esp_rmaker_str("None"), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(learn_mode, ESP_RMAKER_UI_DROPDOWN);
    static const char *stb_learn_options[] = {"None", "Power", "ChannelUp", "ChannelDown",
                                               "PlayPause", "Guide", "Menu", "OK", "Back"};
    esp_rmaker_param_add_valid_str_list(learn_mode, stb_learn_options, 9);
    esp_rmaker_device_add_param(stb_device, learn_mode);

    esp_rmaker_node_add_device(node, stb_device);
    ESP_LOGI(TAG, "STB Remote device created");
    return ESP_OK;
}

static esp_err_t create_custom_device(esp_rmaker_node_t *node)
{
    /* Create as generic device for full control over UI */
    custom_device = esp_rmaker_device_create("Custom Remote", ESP_RMAKER_DEVICE_OTHER, NULL);
    if (!custom_device) {
        ESP_LOGE(TAG, "Failed to create Custom device");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_cb(custom_device, custom_write_cb, NULL);
    esp_rmaker_device_add_param(custom_device, esp_rmaker_name_param_create("Name", "Custom Remote"));

    /* Power button as TRIGGER */
    esp_rmaker_param_t *power_btn = esp_rmaker_param_create("Power", "esp.param.toggle",
                                                              esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(power_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(custom_device, power_btn);

    /* Button 1-12 as TRIGGERS (momentary push buttons) */
    static const char *button_names[] = {
        "Button_1", "Button_2", "Button_3", "Button_4",
        "Button_5", "Button_6", "Button_7", "Button_8",
        "Button_9", "Button_10", "Button_11", "Button_12"
    };
    for (int i = 0; i < 12; i++) {
        esp_rmaker_param_t *button = esp_rmaker_param_create(button_names[i], "esp.param.toggle",
                                                               esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_param_add_ui_type(button, ESP_RMAKER_UI_TRIGGER);
        esp_rmaker_device_add_param(custom_device, button);
    }

    /* Learning mode parameter */
    esp_rmaker_param_t *learn_mode = esp_rmaker_param_create("Learn_Mode", "esp.param.string",
                                                               esp_rmaker_str("None"), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(learn_mode, ESP_RMAKER_UI_DROPDOWN);
    static const char *custom_learn_options[] = {"None", "Power", "Button1", "Button2", "Button3", "Button4",
                                                   "Button5", "Button6", "Button7", "Button8", "Button9",
                                                   "Button10", "Button11", "Button12"};
    esp_rmaker_param_add_valid_str_list(learn_mode, custom_learn_options, 14);
    esp_rmaker_device_add_param(custom_device, learn_mode);

    esp_rmaker_node_add_device(node, custom_device);
    ESP_LOGI(TAG, "Custom Remote device created");
    return ESP_OK;
}

static esp_err_t create_custom_device_2(esp_rmaker_node_t *node)
{
    /* Create as generic device for full control over UI */
    custom_device_2 = esp_rmaker_device_create("Custom Remote 2", ESP_RMAKER_DEVICE_OTHER, NULL);
    if (!custom_device_2) {
        ESP_LOGE(TAG, "Failed to create Custom device 2");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_cb(custom_device_2, custom_write_cb, NULL);
    esp_rmaker_device_add_param(custom_device_2, esp_rmaker_name_param_create("Name", "Custom Remote 2"));

    /* Power button as TRIGGER */
    esp_rmaker_param_t *power_btn = esp_rmaker_param_create("Power", "esp.param.toggle",
                                                              esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(power_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(custom_device_2, power_btn);

    /* Button 1-12 as TRIGGERS (momentary push buttons) */
    static const char *button_names_2[] = {
        "Button_1", "Button_2", "Button_3", "Button_4",
        "Button_5", "Button_6", "Button_7", "Button_8",
        "Button_9", "Button_10", "Button_11", "Button_12"
    };
    for (int i = 0; i < 12; i++) {
        esp_rmaker_param_t *button = esp_rmaker_param_create(button_names_2[i], "esp.param.toggle",
                                                               esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_param_add_ui_type(button, ESP_RMAKER_UI_TRIGGER);
        esp_rmaker_device_add_param(custom_device_2, button);
    }

    /* Learning mode parameter */
    esp_rmaker_param_t *learn_mode = esp_rmaker_param_create("Learn_Mode", "esp.param.string",
                                                               esp_rmaker_str("None"), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(learn_mode, ESP_RMAKER_UI_DROPDOWN);
    static const char *custom2_learn_options[] = {"None", "Power", "Button1", "Button2", "Button3", "Button4",
                                                    "Button5", "Button6", "Button7", "Button8", "Button9",
                                                    "Button10", "Button11", "Button12"};
    esp_rmaker_param_add_valid_str_list(learn_mode, custom2_learn_options, 14);
    esp_rmaker_device_add_param(custom_device_2, learn_mode);

    esp_rmaker_node_add_device(node, custom_device_2);
    ESP_LOGI(TAG, "Custom Remote 2 device created");
    return ESP_OK;
}

static esp_err_t create_custom_device_3(esp_rmaker_node_t *node)
{
    /* Create as generic device for full control over UI */
    custom_device_3 = esp_rmaker_device_create("Custom Remote 3", ESP_RMAKER_DEVICE_OTHER, NULL);
    if (!custom_device_3) {
        ESP_LOGE(TAG, "Failed to create Custom device 3");
        return ESP_FAIL;
    }

    esp_rmaker_device_add_cb(custom_device_3, custom_write_cb, NULL);
    esp_rmaker_device_add_param(custom_device_3, esp_rmaker_name_param_create("Name", "Custom Remote 3"));

    /* Power button as TRIGGER */
    esp_rmaker_param_t *power_btn = esp_rmaker_param_create("Power", "esp.param.toggle",
                                                              esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(power_btn, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(custom_device_3, power_btn);

    /* Button 1-12 as TRIGGERS (momentary push buttons) */
    static const char *button_names_3[] = {
        "Button_1", "Button_2", "Button_3", "Button_4",
        "Button_5", "Button_6", "Button_7", "Button_8",
        "Button_9", "Button_10", "Button_11", "Button_12"
    };
    for (int i = 0; i < 12; i++) {
        esp_rmaker_param_t *button = esp_rmaker_param_create(button_names_3[i], "esp.param.toggle",
                                                               esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
        esp_rmaker_param_add_ui_type(button, ESP_RMAKER_UI_TRIGGER);
        esp_rmaker_device_add_param(custom_device_3, button);
    }

    /* Learning mode parameter */
    esp_rmaker_param_t *learn_mode = esp_rmaker_param_create("Learn_Mode", "esp.param.string",
                                                               esp_rmaker_str("None"), PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(learn_mode, ESP_RMAKER_UI_DROPDOWN);
    static const char *custom3_learn_options[] = {"None", "Power", "Button1", "Button2", "Button3", "Button4",
                                                    "Button5", "Button6", "Button7", "Button8", "Button9",
                                                    "Button10", "Button11", "Button12"};
    esp_rmaker_param_add_valid_str_list(learn_mode, custom3_learn_options, 14);
    esp_rmaker_device_add_param(custom_device_3, learn_mode);

    esp_rmaker_node_add_device(node, custom_device_3);
    ESP_LOGI(TAG, "Custom Remote 3 device created");
    return ESP_OK;
}

/* ============================================================================
 * IP EVENT HANDLER - Just for logging (devices created in app_main)
 * ============================================================================ */

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));

        /* Update LED status to connected */
        rgb_led_set_mode(LED_MODE_WIFI_CONNECTED);
    }
}

/* ============================================================================
 * RESET HANDLER TASK (Handles WiFi reset outside timer context)
 * ============================================================================ */

#define RESET_TASK_WIFI_RESET    (1 << 0)

static void reset_handler_task(void *arg)
{
    uint32_t notification_value;

    while (1) {
        /* Wait for notification from timer callback */
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &notification_value, portMAX_DELAY) == pdTRUE) {
            if (notification_value & RESET_TASK_WIFI_RESET) {
                ESP_LOGI(TAG, "Reset handler task: executing WiFi reset");
                app_wifi_reset();
            }
        }
    }
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
            rgb_led_set_mode(LED_MODE_WIFI_ERROR);

            /* Clear all IR codes */
            ir_action_clear_all();
            ir_ac_clear_state();

            /* Reset WiFi and restart */
            esp_rmaker_factory_reset(0, 2);
        }
    } else {
        if (press_duration >= BUTTON_WIFI_RESET_MS && press_duration < BUTTON_FACTORY_RESET_MS) {
            ESP_LOGI(TAG, "WiFi reset triggered");
            rgb_led_set_mode(LED_MODE_WIFI_ERROR);

            /* Notify reset handler task instead of calling app_wifi_reset() directly */
            if (reset_handler_task_handle != NULL) {
                xTaskNotify(reset_handler_task_handle, RESET_TASK_WIFI_RESET, eSetBits);
            }
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
    /* Create reset handler task first */
    BaseType_t ret = xTaskCreate(
        reset_handler_task,
        "reset_handler",
        4096,               /* Stack size */
        NULL,               /* Parameters */
        5,                  /* Priority */
        &reset_handler_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create reset handler task");
        return;
    }

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

    /* Initialize RGB LED */
    ESP_ERROR_CHECK(rgb_led_init(GPIO_RGB_LED));
    rgb_led_set_mode(LED_MODE_WIFI_CONNECTING);

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

    /* Initialize TCP/IP stack and event loop */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize WiFi */
    app_wifi_init();

    /* Register IP event handler */
    ESP_LOGI(TAG, "Registering IP event handler...");
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                &ip_event_handler, NULL));

    /* Initialize RainMaker node */
    ESP_LOGI(TAG, "Initializing ESP RainMaker node...");
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = true,  // RainMaker will handle SNTP automatically
    };
    rainmaker_node = esp_rmaker_node_init(&rainmaker_cfg, DEVICE_NAME, DEVICE_TYPE);
    if (!rainmaker_node) {
        ESP_LOGE(TAG, "Failed to initialize RainMaker node");
        abort();
    }

    /* Create ALL devices BEFORE starting RainMaker (OFFICIAL PATTERN) */
    ESP_LOGI(TAG, "Creating RainMaker devices...");
    ESP_ERROR_CHECK(create_tv_device(rainmaker_node));
    ESP_ERROR_CHECK(create_ac_device(rainmaker_node));
    ESP_ERROR_CHECK(create_stb_device(rainmaker_node));
    ESP_ERROR_CHECK(create_speaker_device(rainmaker_node));
    ESP_ERROR_CHECK(create_fan_device(rainmaker_node));
    ESP_ERROR_CHECK(create_custom_device(rainmaker_node));
    ESP_ERROR_CHECK(create_custom_device_2(rainmaker_node));
    ESP_ERROR_CHECK(create_custom_device_3(rainmaker_node));
    devices_created = true;
    ESP_LOGI(TAG, "All RainMaker devices created (8 devices total)");

    /* Enable OTA */
    esp_rmaker_ota_enable_default();

    /* Enable timezone service */
    esp_rmaker_timezone_service_enable();

    /* Enable scheduling */
    esp_rmaker_schedule_enable();

    /* Start RainMaker - BEFORE WiFi provisioning (OFFICIAL PATTERN) */
    ESP_LOGI(TAG, "Starting ESP RainMaker...");
    ESP_ERROR_CHECK(esp_rmaker_start());

    /* Local control is auto-enabled by RainMaker - do NOT call manually */
    ESP_LOGI(TAG, "Local control will be auto-enabled by RainMaker");

    /* Start WiFi provisioning - AFTER RainMaker is started */
    ESP_LOGI(TAG, "Starting WiFi provisioning...");
    app_wifi_start(POP_TYPE_RANDOM);

    /* Initialize console for debugging */
    esp_rmaker_console_init();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Universal IR Remote Ready!");
    ESP_LOGI(TAG, "  Use RainMaker app to provision and control");
    ESP_LOGI(TAG, "========================================");

    /* Main task idle loop to keep task alive */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
