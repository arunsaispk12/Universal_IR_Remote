/**
 * @file app_wifi.c
 * @brief WiFi and BLE Provisioning Management
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"
#include "esp_random.h"

#include "app_wifi.h"
#include "app_config.h"
#include "rgb_led.h"

static const char *TAG = "app_wifi";

/* WiFi state tracking */
static app_wifi_state_t s_wifi_state = WIFI_STATE_DISCONNECTED;
static bool s_is_connected = false;
static int8_t s_rssi = 0;

/* Event group for WiFi events */
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* Forward declarations */
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_prov_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/* Get a unique service name for BLE provisioning */
static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "PROV_%s_%02X%02X%02X",
             DEVICE_NAME, eth_mac[3], eth_mac[4], eth_mac[5]);
}

/**
 * @brief WiFi event handler
 */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected, retrying...");
        s_wifi_state = WIFI_STATE_CONNECTING;
        s_is_connected = false;
        rgb_led_set_status(LED_STATUS_WIFI_CONNECTING);
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected with IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_state = WIFI_STATE_CONNECTED;
        s_is_connected = true;
        rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        /* Get RSSI */
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            s_rssi = ap_info.rssi;
            ESP_LOGI(TAG, "WiFi RSSI: %d dBm", s_rssi);
        }
    }
}

/**
 * @brief Provisioning event handler
 */
static void wifi_prov_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data)
{
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            s_wifi_state = WIFI_STATE_PROVISIONING;
            rgb_led_set_status(LED_STATUS_WIFI_CONNECTING);
            break;
        case WIFI_PROV_CRED_RECV: {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received WiFi credentials"
                     "\n\tSSID     : %s\n\tPassword : %s",
                     (const char *)wifi_sta_cfg->ssid,
                     (const char *)wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL: {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                     "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                     "WiFi station authentication failed" : "WiFi access-point not found");
            rgb_led_set_status(LED_STATUS_ERROR);
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            break;
        case WIFI_PROV_END:
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            ESP_LOGI(TAG, "Provisioning ended");
            break;
        default:
            break;
        }
    }
}

esp_err_t app_wifi_init(const char *pop_pin)
{
    esp_err_t ret = ESP_OK;

    /* Create event group */
    s_wifi_event_group = xEventGroupCreate();

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize WiFi */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Register event handlers */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t prov_config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };

    /* Initialize provisioning manager */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_config));

    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (!provisioned) {
        ESP_LOGI(TAG, "Starting BLE provisioning");

        /* Generate service name */
        char service_name[32];
        get_device_service_name(service_name, sizeof(service_name));

        /* Generate random service key */
        char service_key[16];
        for (int i = 0; i < sizeof(service_key) - 1; i++) {
            service_key[i] = 'a' + (esp_random() % 26);
        }
        service_key[sizeof(service_key) - 1] = '\0';

        /* Use default POP if not provided */
        const char *pop = pop_pin ? pop_pin : "abcd1234";

        /* Register provisioning event handler */
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
                                                    &wifi_prov_event_handler, NULL));

        /* Set device name in BLE advertisement */
        uint8_t custom_service_uuid[] = {
            /* Custom UUID for IR Remote service */
            0x21, 0xad, 0x07, 0xe6, 0xdd, 0xe2, 0x46, 0x6a,
            0x95, 0x7f, 0x57, 0x6d, 0x69, 0x72, 0x72, 0x65
        };

        /* Start provisioning */
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
            WIFI_PROV_SECURITY_1,
            pop,
            service_name,
            service_key));

        /* Print BLE provisioning QR code (for RainMaker app) */
        ESP_LOGI(TAG, "Provisioning started with service name: %s", service_name);
        ESP_LOGI(TAG, "Scan QR code or use BLE to provision device");
        ESP_LOGI(TAG, "Proof of Possession (PoP): %s", pop);

        s_wifi_state = WIFI_STATE_PROVISIONING;
        rgb_led_set_status(LED_STATUS_WIFI_CONNECTING);
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting WiFi station");

        /* Release provisioning resources */
        wifi_prov_mgr_deinit();

        /* Start WiFi station */
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());

        s_wifi_state = WIFI_STATE_CONNECTING;
        rgb_led_set_status(LED_STATUS_WIFI_CONNECTING);
    }

    return ret;
}

app_wifi_state_t app_wifi_get_state(void)
{
    return s_wifi_state;
}

bool app_wifi_is_connected(void)
{
    return s_is_connected;
}

esp_err_t app_wifi_reset(void)
{
    ESP_LOGI(TAG, "Resetting WiFi credentials");

    /* Clear provisioning data */
    esp_err_t ret = wifi_prov_mgr_reset_provisioning();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset provisioning: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "WiFi credentials cleared. Restarting...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    return ESP_OK;
}

int8_t app_wifi_get_rssi(void)
{
    if (s_is_connected) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            s_rssi = ap_info.rssi;
        }
    }
    return s_rssi;
}
