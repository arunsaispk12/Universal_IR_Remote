/**
 * @file rmaker_devices.c
 * @brief Custom ESP RainMaker device type helpers implementation
 */

#include "rmaker_devices.h"
#include "esp_rmaker_core.h"
#include "esp_rmaker_standard_types.h"
#include "esp_rmaker_standard_params.h"
#include <esp_log.h>

static const char __attribute__((unused)) *TAG = "rmaker_devices";

esp_rmaker_device_t *esp_rmaker_tv_device_create(const char *dev_name,
        void *priv_data, bool primary)
{
    esp_rmaker_device_t *device = esp_rmaker_device_create(dev_name, ESP_RMAKER_DEVICE_TV, priv_data);
    if (device) {
        esp_rmaker_device_add_cb(device, NULL, NULL);
        if (primary) {
            esp_rmaker_device_add_param(device, esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, false));
        }
    }
    return device;
}

esp_rmaker_device_t *esp_rmaker_ac_device_create(const char *dev_name,
        void *priv_data, bool primary)
{
    esp_rmaker_device_t *device = esp_rmaker_device_create(dev_name, ESP_RMAKER_DEVICE_AIR_CONDITIONER, priv_data);
    if (device) {
        esp_rmaker_device_add_cb(device, NULL, NULL);
        if (primary) {
            esp_rmaker_device_add_param(device, esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, false));
        }
    }
    return device;
}

esp_rmaker_device_t *esp_rmaker_speaker_device_create(const char *dev_name,
        void *priv_data, bool primary)
{
    esp_rmaker_device_t *device = esp_rmaker_device_create(dev_name, ESP_RMAKER_DEVICE_SPEAKER, priv_data);
    if (device) {
        esp_rmaker_device_add_cb(device, NULL, NULL);
        if (primary) {
            esp_rmaker_device_add_param(device, esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, false));
        }
    }
    return device;
}

