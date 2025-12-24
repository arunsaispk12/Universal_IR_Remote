/**
 * @file app_wifi.h
 * @brief WiFi and Provisioning Management for Universal IR Remote
 */

#ifndef APP_WIFI_H
#define APP_WIFI_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi connection state
 */
typedef enum {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_PROVISIONING,
} app_wifi_state_t;

/**
 * @brief Initialize WiFi and start BLE provisioning
 *
 * @param pop_pin Proof of Possession PIN for secure provisioning
 * @return ESP_OK on success
 */
esp_err_t app_wifi_init(const char *pop_pin);

/**
 * @brief Get current WiFi connection state
 *
 * @return Current WiFi state
 */
app_wifi_state_t app_wifi_get_state(void);

/**
 * @brief Check if WiFi is connected
 *
 * @return true if connected, false otherwise
 */
bool app_wifi_is_connected(void);

/**
 * @brief Reset WiFi credentials and restart provisioning
 *
 * @return ESP_OK on success
 */
esp_err_t app_wifi_reset(void);

/**
 * @brief Get WiFi RSSI (signal strength)
 *
 * @return RSSI value in dBm
 */
int8_t app_wifi_get_rssi(void);

#ifdef __cplusplus
}
#endif

#endif // APP_WIFI_H
