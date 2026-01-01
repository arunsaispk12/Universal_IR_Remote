/**
 * @file app_wifi.h
 * @brief WiFi Connection Handler for ESP RainMaker
 * 
 * This component handles WiFi initialization and provisioning
 * for the Smart Home Automation RainMaker project.
 */

#ifndef APP_WIFI_H
#define APP_WIFI_H

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WiFi Event Base */
ESP_EVENT_DECLARE_BASE(APP_WIFI_EVENT);

/* WiFi Events */
typedef enum {
    APP_WIFI_EVENT_STA_CONNECTED,
    APP_WIFI_EVENT_STA_DISCONNECTED,
} app_wifi_event_t;

/* Provisioning Types */
typedef enum {
    POP_TYPE_NONE,
    POP_TYPE_RANDOM,
    POP_TYPE_CUSTOM,
} pop_type_t;

/**
 * @brief Initialize WiFi stack
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_wifi_init(void);

/**
 * @brief Start WiFi provisioning
 * 
 * @param pop_type Type of Proof of Possession
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_wifi_start(pop_type_t pop_type);

/**
 * @brief Check if WiFi is connected
 * 
 * @return true if connected, false otherwise
 */
bool app_wifi_is_connected(void);

/**
 * @brief Get WiFi RSSI
 * 
 * @return RSSI value in dBm
 */
int8_t app_wifi_get_rssi(void);

#ifdef __cplusplus
}
#endif



/**
 * @brief Reset WiFi credentials and restart
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_wifi_reset(void);

#endif /* APP_WIFI_H */
