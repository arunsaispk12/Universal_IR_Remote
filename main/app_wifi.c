/**
 * @file app_wifi.c
 * @brief WiFi Connection Handler Implementation for ESP RainMaker
 */

#include "app_wifi.h"
#include <string.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_random.h>
#include <nvs_flash.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include <qrcode.h>

static const char *TAG = "APP_WIFI";

ESP_EVENT_DEFINE_BASE(APP_WIFI_EVENT);

/* WiFi state */
static bool wifi_connected = false;
static EventGroupHandle_t wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;

/**
 * @brief WiFi event handler
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                break;
            case WIFI_PROV_END:
                wifi_prov_mgr_deinit();
                ESP_LOGI(TAG, "Provisioning end");
                break;
            default:
                break;
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                ESP_LOGI(TAG, "WiFi station started, connecting...");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "WiFi disconnected, retrying...");
                wifi_connected = false;
                xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
                esp_event_post(APP_WIFI_EVENT, APP_WIFI_EVENT_STA_DISCONNECTED, NULL, 0, portMAX_DELAY);
                esp_wifi_connect();
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI(TAG, "WiFi connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
                wifi_connected = true;
                xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
                esp_event_post(APP_WIFI_EVENT, APP_WIFI_EVENT_STA_CONNECTED, NULL, 0, portMAX_DELAY);
                break;
            }
            default:
                break;
        }
    }
}

/**
 * @brief Initialize WiFi
 */
esp_err_t app_wifi_init(void)
{
    ESP_LOGI(TAG, "Initializing WiFi");
    
    /* Create event group */
    wifi_event_group = xEventGroupCreate();
    if (wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }
    
    /* Initialize WiFi */
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi STA interface");
        return ESP_FAIL;
    }
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    /* Register event handlers */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, 
                                                &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                                &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                                &wifi_event_handler, NULL));
    
    /* Set WiFi mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi initialized");
    return ESP_OK;
}

/**
 * @brief Start WiFi provisioning
 */
esp_err_t app_wifi_start(pop_type_t pop_type)
{
    ESP_LOGI(TAG, "Starting WiFi provisioning");
    
    /* Configuration for provisioning manager - CHANGED TO BLE */
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };
    
    /* Initialize provisioning manager */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    
    bool provisioned = false;
    
    /* Check if already provisioned */
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    
    if (!provisioned) {
        ESP_LOGI(TAG, "Starting provisioning (not provisioned yet)");
        
        /* Generate proof of possession */
        char pop[9] = {0};
        if (pop_type == POP_TYPE_RANDOM) {
            snprintf(pop, sizeof(pop), "%08" PRIX32, esp_random());
        }
        
        /* Generate service name */
        char service_name[32];
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, mac);
        snprintf(service_name, sizeof(service_name), "PROV_SHA_%02X%02X%02X", 
                 mac[3], mac[4], mac[5]);
        
        const char *service_key = NULL;
        
        /* Start provisioning service - Using BLE instead of SoftAP */
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, 
                                                          pop_type == POP_TYPE_RANDOM ? pop : NULL, 
                                                          service_name, 
                                                          service_key));
        
        ESP_LOGI(TAG, "===============================================");
        ESP_LOGI(TAG, "Provisioning started (BLE)");
        ESP_LOGI(TAG, "Service Name: %s", service_name);
        if (pop_type == POP_TYPE_RANDOM) {
            ESP_LOGI(TAG, "Proof of Possession (PoP): %s", pop);
        }
        ESP_LOGI(TAG, "Use the ESP RainMaker app to provision");
        ESP_LOGI(TAG, "===============================================");

        /* Generate and display QR code for provisioning */
        char payload[150] = {0};
        snprintf(payload, sizeof(payload), "{\"ver\":\"v1\",\"name\":\"%s\"", service_name);

        if (pop_type == POP_TYPE_RANDOM) {
            snprintf(payload + strlen(payload), sizeof(payload) - strlen(payload),
                    ",\"pop\":\"%s\",\"transport\":\"ble\"}", pop);
        } else {
            snprintf(payload + strlen(payload), sizeof(payload) - strlen(payload),
                    ",\"transport\":\"ble\"}");
        }

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "Scan this QR code from the ESP RainMaker phone app:");

        // Generate and display QR code
        esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
        cfg.display_func = esp_qrcode_print_console;
        cfg.max_qrcode_version = 10;
        esp_qrcode_generate(&cfg, payload);

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "If QR code is not visible, use provisioning manually:");
        ESP_LOGI(TAG, "  Service Name: %s", service_name);
        if (pop_type == POP_TYPE_RANDOM) {
            ESP_LOGI(TAG, "  Proof of Possession: %s", pop);
        }
        ESP_LOGI(TAG, "");
        
    } else {
        ESP_LOGI(TAG, "Already provisioned, connecting to WiFi");
        wifi_prov_mgr_deinit();
        
        /* Wait for connection */
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, 
                           false, true, portMAX_DELAY);
    }
    
    return ESP_OK;
}

/**
 * @brief Check if WiFi is connected
 */
bool app_wifi_is_connected(void)
{
    return wifi_connected;
}

/**
 * @brief Get WiFi RSSI
 */
int8_t app_wifi_get_rssi(void)
{
    wifi_ap_record_t ap_info;
    
    if (!wifi_connected) {
        return -100;
    }
    
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    
    return -100;
}

/**
 * @brief Reset WiFi credentials and restart
 */
esp_err_t app_wifi_reset(void)
{
    ESP_LOGI(TAG, "Resetting WiFi credentials");

    /* Reset provisioning */
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
