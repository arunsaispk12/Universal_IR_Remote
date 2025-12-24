/**
 * @file app_main.c
 * @brief Universal IR Remote Control with ESP RainMaker
 *
 * Features:
 * - 32 programmable IR buttons
 * - BLE WiFi provisioning
 * - Cloud control via RainMaker app
 * - IR learning and transmission
 * - RGB LED status indication
 * - OTA updates
 * - Factory reset (10s button press)
 * - WiFi reset (3s button press)
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
#include "rgb_led.h"

static const char *TAG = "app_main";

/* RainMaker handles */
static esp_rmaker_device_t *ir_remote_device = NULL;
static esp_rmaker_param_t *button_params[IR_BTN_MAX * 3]; // Learn, Transmit, Learned for each button

/* Button press tracking */
static TimerHandle_t boot_button_timer = NULL;
static uint32_t button_press_start = 0;
static bool factory_reset_triggered = false;

/* IR learning state */
static ir_button_t current_learning_button = IR_BTN_MAX;

/* Button parameter names for each IR button */
static const char *button_param_names[IR_BTN_MAX] = {
    "Power", "Source", "Menu", "Home", "Back", "OK",
    "Vol+", "Vol-", "Mute",
    "Ch+", "Ch-",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "Up", "Down", "Left", "Right",
    "Custom1", "Custom2", "Custom3", "Custom4", "Custom5", "Custom6"
};

/* ============================================================================
 * IR LEARNING CALLBACKS
 * ============================================================================ */

/**
 * @brief Callback when IR learning succeeds
 */
static void ir_learn_success_callback(ir_button_t button, ir_code_t *code, void *arg)
{
    ESP_LOGI(TAG, "IR learning successful for button: %s (%s protocol)",
             ir_get_button_name(button),
             ir_get_protocol_name(code->protocol));

    /* Update LED status */
    rgb_led_set_status(LED_STATUS_LEARN_SUCCESS);
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Save code to NVS */
    if (ir_save_code(button, code) == ESP_OK) {
        ESP_LOGI(TAG, "IR code saved to NVS");
    }

    /* Update RainMaker "Learned" status parameter */
    if (button < IR_BTN_MAX && button_params[button * 3 + 2]) {
        esp_rmaker_param_update_and_report(button_params[button * 3 + 2], esp_rmaker_bool(true));
    }

    /* Reset learning state */
    current_learning_button = IR_BTN_MAX;

    /* Return to idle or connected state */
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
    ESP_LOGW(TAG, "IR learning failed for button: %s (timeout)", ir_get_button_name(button));

    /* Update LED status */
    rgb_led_set_status(LED_STATUS_LEARN_FAILED);
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Reset learning state */
    current_learning_button = IR_BTN_MAX;

    /* Return to idle or connected state */
    if (app_wifi_is_connected()) {
        rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);
    } else {
        rgb_led_set_status(LED_STATUS_IDLE);
    }
}

/* ============================================================================
 * RAINMAKER WRITE CALLBACKS
 * ============================================================================ */

/**
 * @brief RainMaker write callback for IR button parameters
 */
static esp_err_t ir_button_write_cb(const esp_rmaker_device_t *device,
                                     const esp_rmaker_param_t *param,
                                     const esp_rmaker_param_val_t val,
                                     void *priv_data,
                                     esp_rmaker_write_ctx_t *ctx)
{
    if (!device || !param) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *param_name = esp_rmaker_param_get_name(param);
    ESP_LOGI(TAG, "Parameter update: %s = %s", param_name, val.val.b ? "true" : "false");

    /* Only process 'true' values (button press) */
    if (!val.val.b) {
        return ESP_OK;
    }

    /* Find which button and action */
    for (int i = 0; i < IR_BTN_MAX; i++) {
        char learn_name[32], transmit_name[32];
        snprintf(learn_name, sizeof(learn_name), "%s_Learn", button_param_names[i]);
        snprintf(transmit_name, sizeof(transmit_name), "%s_Transmit", button_param_names[i]);

        if (strcmp(param_name, learn_name) == 0) {
            /* Learn button pressed */
            ESP_LOGI(TAG, "Starting IR learning for button: %s", button_param_names[i]);
            rgb_led_set_status(LED_STATUS_LEARNING);
            current_learning_button = (ir_button_t)i;

            esp_err_t err = ir_learn_start((ir_button_t)i, IR_LEARNING_TIMEOUT_MS);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to start IR learning: %s", esp_err_to_name(err));
                rgb_led_set_status(LED_STATUS_ERROR);
                current_learning_button = IR_BTN_MAX;
            }

            /* Reset parameter to false */
            esp_rmaker_param_update_and_report(param, esp_rmaker_bool(false));
            return ESP_OK;

        } else if (strcmp(param_name, transmit_name) == 0) {
            /* Transmit button pressed */
            ESP_LOGI(TAG, "Transmitting IR code for button: %s", button_param_names[i]);
            rgb_led_set_status(LED_STATUS_TRANSMITTING);

            esp_err_t err = ir_transmit_button((ir_button_t)i);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "IR code transmitted successfully");
                vTaskDelay(pdMS_TO_TICKS(200));
            } else if (err == ESP_ERR_NOT_FOUND) {
                ESP_LOGW(TAG, "No learned code for button: %s", button_param_names[i]);
                rgb_led_set_status(LED_STATUS_ERROR);
                vTaskDelay(pdMS_TO_TICKS(500));
            } else {
                ESP_LOGE(TAG, "Failed to transmit IR code: %s", esp_err_to_name(err));
                rgb_led_set_status(LED_STATUS_ERROR);
                vTaskDelay(pdMS_TO_TICKS(500));
            }

            /* Return to connected state */
            if (app_wifi_is_connected()) {
                rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);
            } else {
                rgb_led_set_status(LED_STATUS_IDLE);
            }

            /* Reset parameter to false */
            esp_rmaker_param_update_and_report(param, esp_rmaker_bool(false));
            return ESP_OK;
        }
    }

    return ESP_OK;
}

/* ============================================================================
 * RAINMAKER DEVICE CREATION
 * ============================================================================ */

/**
 * @brief Create RainMaker device with all 32 IR buttons
 */
static esp_err_t create_ir_remote_device(esp_rmaker_node_t *node)
{
    /* Create IR Remote device */
    ir_remote_device = esp_rmaker_device_create("IR Remote", ESP_RMAKER_DEVICE_OTHER, NULL);
    if (!ir_remote_device) {
        ESP_LOGE(TAG, "Failed to create IR Remote device");
        return ESP_FAIL;
    }

    /* Add device to node */
    esp_rmaker_node_add_device(node, ir_remote_device);

    /* Create parameters for each button (Learn, Transmit, Learned status) */
    for (int i = 0; i < IR_BTN_MAX; i++) {
        char learn_name[32], transmit_name[32], learned_name[32];

        snprintf(learn_name, sizeof(learn_name), "%s_Learn", button_param_names[i]);
        snprintf(transmit_name, sizeof(transmit_name), "%s_Transmit", button_param_names[i]);
        snprintf(learned_name, sizeof(learned_name), "%s_Learned", button_param_names[i]);

        /* Learn parameter (write-only) */
        button_params[i * 3] = esp_rmaker_param_create(learn_name, ESP_RMAKER_PARAM_TOGGLE,
                                                        esp_rmaker_bool(false), PROP_FLAG_WRITE);
        if (button_params[i * 3]) {
            esp_rmaker_param_add_ui_type(button_params[i * 3], ESP_RMAKER_UI_TOGGLE);
            esp_rmaker_device_add_param(ir_remote_device, button_params[i * 3]);
            esp_rmaker_device_assign_param_callback(ir_remote_device, ir_button_write_cb, NULL);
        }

        /* Transmit parameter (write-only) */
        button_params[i * 3 + 1] = esp_rmaker_param_create(transmit_name, ESP_RMAKER_PARAM_TOGGLE,
                                                            esp_rmaker_bool(false), PROP_FLAG_WRITE);
        if (button_params[i * 3 + 1]) {
            esp_rmaker_param_add_ui_type(button_params[i * 3 + 1], ESP_RMAKER_UI_TOGGLE);
            esp_rmaker_device_add_param(ir_remote_device, button_params[i * 3 + 1]);
        }

        /* Learned status parameter (read-only) */
        bool is_learned = ir_is_learned((ir_button_t)i);
        button_params[i * 3 + 2] = esp_rmaker_param_create(learned_name, ESP_RMAKER_PARAM_TOGGLE,
                                                            esp_rmaker_bool(is_learned), PROP_FLAG_READ);
        if (button_params[i * 3 + 2]) {
            esp_rmaker_param_add_ui_type(button_params[i * 3 + 2], ESP_RMAKER_UI_TOGGLE);
            esp_rmaker_device_add_param(ir_remote_device, button_params[i * 3 + 2]);
        }
    }

    ESP_LOGI(TAG, "IR Remote device created with %d buttons", IR_BTN_MAX);
    return ESP_OK;
}

/* ============================================================================
 * BOOT BUTTON HANDLER (WiFi Reset / Factory Reset)
 * ============================================================================ */

/**
 * @brief Boot button timer callback
 */
static void boot_button_timer_cb(TimerHandle_t timer)
{
    uint32_t press_duration = (esp_log_timestamp() - button_press_start);

    if (gpio_get_level(GPIO_BOOT_BUTTON) == 0) {
        /* Button still pressed */
        if (press_duration >= BUTTON_FACTORY_RESET_MS && !factory_reset_triggered) {
            /* Factory reset */
            ESP_LOGW(TAG, "Factory reset triggered!");
            factory_reset_triggered = true;
            rgb_led_set_status(LED_STATUS_ERROR);

            /* Clear all IR codes */
            ir_clear_all_codes();

            /* Reset WiFi and restart */
            esp_rmaker_factory_reset(0, 2);
        }
    } else {
        /* Button released */
        if (press_duration >= BUTTON_WIFI_RESET_MS && press_duration < BUTTON_FACTORY_RESET_MS) {
            /* WiFi reset */
            ESP_LOGI(TAG, "WiFi reset triggered");
            rgb_led_set_status(LED_STATUS_ERROR);
            app_wifi_reset();
        }

        /* Stop timer */
        xTimerStop(timer, 0);
        factory_reset_triggered = false;
    }
}

/**
 * @brief Boot button interrupt handler
 */
static void IRAM_ATTR boot_button_isr_handler(void *arg)
{
    if (gpio_get_level(GPIO_BOOT_BUTTON) == 0) {
        /* Button pressed */
        button_press_start = esp_log_timestamp();
        factory_reset_triggered = false;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xTimerStartFromISR(boot_button_timer, &xHigherPriorityTaskWoken);
    }
}

/**
 * @brief Initialize boot button handler
 */
static void init_boot_button(void)
{
    /* Configure boot button GPIO */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_BOOT_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf);

    /* Create timer */
    boot_button_timer = xTimerCreate("boot_btn", pdMS_TO_TICKS(100), pdTRUE,
                                      NULL, boot_button_timer_cb);

    /* Install ISR service and add handler */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_BOOT_BUTTON, boot_button_isr_handler, NULL);

    ESP_LOGI(TAG, "Boot button initialized (GPIO%d)", GPIO_BOOT_BUTTON);
    ESP_LOGI(TAG, "  - Press 3s: WiFi reset");
    ESP_LOGI(TAG, "  - Press 10s: Factory reset");
}

/* ============================================================================
 * CONSOLE COMMANDS (for testing)
 * ============================================================================ */

/**
 * @brief Console command: learn <button_id>
 */
static int cmd_learn(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: learn <button_id>\n");
        printf("Example: learn 0 (Power button)\n");
        return 1;
    }

    int button_id = atoi(argv[1]);
    if (button_id < 0 || button_id >= IR_BTN_MAX) {
        printf("Error: button_id must be 0-%d\n", IR_BTN_MAX - 1);
        return 1;
    }

    printf("Starting IR learning for button %d (%s)...\n",
           button_id, ir_get_button_name((ir_button_t)button_id));

    rgb_led_set_status(LED_STATUS_LEARNING);
    current_learning_button = (ir_button_t)button_id;

    esp_err_t err = ir_learn_start((ir_button_t)button_id, IR_LEARNING_TIMEOUT_MS);
    if (err != ESP_OK) {
        printf("Error: Failed to start learning: %s\n", esp_err_to_name(err));
        return 1;
    }

    printf("Point remote at device and press button...\n");
    return 0;
}

/**
 * @brief Console command: transmit <button_id>
 */
static int cmd_transmit(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: transmit <button_id>\n");
        printf("Example: transmit 0 (Power button)\n");
        return 1;
    }

    int button_id = atoi(argv[1]);
    if (button_id < 0 || button_id >= IR_BTN_MAX) {
        printf("Error: button_id must be 0-%d\n", IR_BTN_MAX - 1);
        return 1;
    }

    printf("Transmitting IR code for button %d (%s)...\n",
           button_id, ir_get_button_name((ir_button_t)button_id));

    rgb_led_set_status(LED_STATUS_TRANSMITTING);
    esp_err_t err = ir_transmit_button((ir_button_t)button_id);

    if (err == ESP_OK) {
        printf("IR code transmitted successfully\n");
    } else if (err == ESP_ERR_NOT_FOUND) {
        printf("Error: No learned code for this button\n");
    } else {
        printf("Error: Failed to transmit: %s\n", esp_err_to_name(err));
    }

    vTaskDelay(pdMS_TO_TICKS(200));
    if (app_wifi_is_connected()) {
        rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);
    } else {
        rgb_led_set_status(LED_STATUS_IDLE);
    }

    return (err == ESP_OK) ? 0 : 1;
}

/**
 * @brief Console command: list (show all learned buttons)
 */
static int cmd_list(int argc, char **argv)
{
    printf("\n=== Learned IR Buttons ===\n");
    int count = 0;

    for (int i = 0; i < IR_BTN_MAX; i++) {
        if (ir_is_learned((ir_button_t)i)) {
            printf("  [%2d] %s\n", i, ir_get_button_name((ir_button_t)i));
            count++;
        }
    }

    printf("\nTotal learned buttons: %d / %d\n\n", count, IR_BTN_MAX);
    return 0;
}

/**
 * @brief Console command: clear <button_id|all>
 */
static int cmd_clear(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: clear <button_id|all>\n");
        printf("Example: clear 0 (clear Power button)\n");
        printf("Example: clear all (clear all buttons)\n");
        return 1;
    }

    if (strcmp(argv[1], "all") == 0) {
        printf("Clearing all learned IR codes...\n");
        esp_err_t err = ir_clear_all_codes();
        if (err == ESP_OK) {
            printf("All IR codes cleared successfully\n");

            /* Update all "Learned" status parameters */
            for (int i = 0; i < IR_BTN_MAX; i++) {
                if (button_params[i * 3 + 2]) {
                    esp_rmaker_param_update_and_report(button_params[i * 3 + 2], esp_rmaker_bool(false));
                }
            }
        } else {
            printf("Error: Failed to clear codes: %s\n", esp_err_to_name(err));
            return 1;
        }
    } else {
        int button_id = atoi(argv[1]);
        if (button_id < 0 || button_id >= IR_BTN_MAX) {
            printf("Error: button_id must be 0-%d or 'all'\n", IR_BTN_MAX - 1);
            return 1;
        }

        printf("Clearing IR code for button %d (%s)...\n",
               button_id, ir_get_button_name((ir_button_t)button_id));

        esp_err_t err = ir_clear_code((ir_button_t)button_id);
        if (err == ESP_OK) {
            printf("IR code cleared successfully\n");

            /* Update "Learned" status parameter */
            if (button_params[button_id * 3 + 2]) {
                esp_rmaker_param_update_and_report(button_params[button_id * 3 + 2], esp_rmaker_bool(false));
            }
        } else {
            printf("Error: Failed to clear code: %s\n", esp_err_to_name(err));
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Register console commands
 */
static void register_console_commands(void)
{
    const esp_console_cmd_t learn_cmd = {
        .command = "learn",
        .help = "Start IR learning for a button",
        .hint = "<button_id>",
        .func = &cmd_learn,
    };
    esp_console_cmd_register(&learn_cmd);

    const esp_console_cmd_t transmit_cmd = {
        .command = "transmit",
        .help = "Transmit learned IR code",
        .hint = "<button_id>",
        .func = &cmd_transmit,
    };
    esp_console_cmd_register(&transmit_cmd);

    const esp_console_cmd_t list_cmd = {
        .command = "list",
        .help = "List all learned IR buttons",
        .hint = NULL,
        .func = &cmd_list,
    };
    esp_console_cmd_register(&list_cmd);

    const esp_console_cmd_t clear_cmd = {
        .command = "clear",
        .help = "Clear learned IR code(s)",
        .hint = "<button_id|all>",
        .func = &cmd_clear,
    };
    esp_console_cmd_register(&clear_cmd);

    ESP_LOGI(TAG, "Console commands registered");
}

/* ============================================================================
 * MAIN APPLICATION
 * ============================================================================ */

void app_main(void)
{
    esp_err_t err = ESP_OK;

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Universal IR Remote Control");
    ESP_LOGI(TAG, "  Firmware: %s", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "  Model: %s", MODEL);
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

    /* Register IR callbacks */
    ir_callbacks_t ir_callbacks = {
        .learn_success_cb = ir_learn_success_callback,
        .learn_fail_cb = ir_learn_fail_callback,
        .receive_cb = NULL,
        .user_arg = NULL
    };
    ESP_ERROR_CHECK(ir_register_callbacks(&ir_callbacks));

    /* Load saved IR codes from NVS */
    ESP_LOGI(TAG, "Loading saved IR codes...");
    ir_load_all_codes();

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

    /* Create IR Remote device with all buttons */
    ESP_ERROR_CHECK(create_ir_remote_device(node));

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
    register_console_commands();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Universal IR Remote Ready!");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Use RainMaker app to provision and control");
    ESP_LOGI(TAG, "Console commands: learn, transmit, list, clear");
}
