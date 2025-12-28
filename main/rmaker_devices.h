/**
 * @file rmaker_devices.h
 * @brief Custom ESP RainMaker device type helpers
 *
 * Helper functions to create standard device types not provided
 * by esp_rmaker_standard_devices.h
 */

#pragma once

#include "esp_rmaker_core.h"
#include "esp_rmaker_standard_types.h"
#include "esp_rmaker_standard_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a TV device
 */
esp_rmaker_device_t *esp_rmaker_tv_device_create(const char *dev_name,
        void *priv_data, bool primary);

/**
 * @brief Create an Air Conditioner device
 */
esp_rmaker_device_t *esp_rmaker_ac_device_create(const char *dev_name,
        void *priv_data, bool primary);

/**
 * @brief Create a Speaker device
 */
esp_rmaker_device_t *esp_rmaker_speaker_device_create(const char *dev_name,
        void *priv_data, bool primary);

#ifdef __cplusplus
}
#endif
