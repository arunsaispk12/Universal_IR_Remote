/**
 * @file app_config.h
 * @brief Universal IR Remote - Hardware Configuration
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * GPIO PIN ASSIGNMENTS
 * ============================================================================ */
// IR Hardware
#define GPIO_IR_TX              17  // IR Transmitter (940nm LED)
#define GPIO_IR_RX              18  // IR Receiver (IRM-3638T, active-LOW)

// Status LED
#define GPIO_RGB_LED            11  // WS2812B RGB LED (ESP32-S3 compatible)

// Boot Button (for factory reset)
#define GPIO_BOOT_BUTTON        0   // Boot button (active-LOW)

/* ============================================================================
 * DEVICE INFORMATION
 * ============================================================================ */
#define DEVICE_NAME             "Universal IR Remote"
#define DEVICE_TYPE             "esp.device.other"
#define MANUFACTURER            "Sai Automations"
#define MODEL                   "UIR-V1.0"
#define FIRMWARE_VERSION        "1.0.0"

/* ============================================================================
 * RAINMAKER CONFIGURATION
 * ============================================================================ */
#define RMAKER_QR_CODE_SIZE     256

/* ============================================================================
 * IR CONFIGURATION
 * ============================================================================ */
#define IR_LEARNING_TIMEOUT_MS  30000   // 30 seconds
#define IR_TX_RMT_CHANNEL       1       // RMT channel for TX
#define IR_RX_RMT_CHANNEL       2       // RMT channel for RX

/* ============================================================================
 * RGB LED CONFIGURATION
 * ============================================================================ */
#define RGB_LED_COUNT           1
#define RGB_LED_RMT_CHANNEL     0

/* ============================================================================
 * NVS STORAGE KEYS
 * ============================================================================ */
#define NVS_NAMESPACE           "uir_config"
#define NVS_KEY_IR_CODES        "ir_codes"

/* ============================================================================
 * TASK PRIORITIES AND STACK SIZES
 * ============================================================================ */
#define MAIN_TASK_PRIORITY      5
#define MAIN_TASK_STACK_SIZE    6144

#define IR_TASK_PRIORITY        6
#define IR_TASK_STACK_SIZE      8192

#define LED_TASK_PRIORITY       3
#define LED_TASK_STACK_SIZE     4096

/* ============================================================================
 * BUTTON CONFIGURATION
 * ============================================================================ */
#define BUTTON_DEBOUNCE_MS      50
#define BUTTON_WIFI_RESET_MS    3000    // 3 seconds
#define BUTTON_FACTORY_RESET_MS 10000   // 10 seconds

#endif // APP_CONFIG_H
