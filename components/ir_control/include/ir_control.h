/**
 * @file ir_control.h
 * @brief Universal IR Remote Control Component
 *
 * This component handles IR learning and transmission for universal remote control
 * Supports NEC, Samsung, and RAW IR protocols with NVS storage
 *
 * Based on SHA_RainMaker_Project v1.0.0 IR implementation
 */

#ifndef IR_CONTROL_H
#define IR_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* IR GPIO Configuration */
#define IR_TX_GPIO          17      // IR Transmitter GPIO
#define IR_RX_GPIO          18      // IR Receiver GPIO (active-LOW with inversion)
#define IR_RMT_TX_CHANNEL   0       // RMT channel for TX
#define IR_RMT_RX_CHANNEL   1       // RMT channel for RX

/* IR Timing Configuration */
#define IR_MAX_CODE_LENGTH  256     // Maximum IR code length (raw pulses)
#define IR_CARRIER_FREQ_HZ  38000   // Standard IR carrier frequency
#define IR_TIMEOUT_MS       100     // Receive timeout
#define IR_LEARN_TIMEOUT_MS 30000   // Learning mode timeout (30 seconds)

/**
 * @brief IR Protocol Types
 */
typedef enum {
    IR_PROTOCOL_UNKNOWN = 0,    // Must be 0 for memset() initialization
    IR_PROTOCOL_NEC,            // NEC protocol (standard TV/AC remotes)
    IR_PROTOCOL_SAMSUNG,        // Samsung protocol variant
    IR_PROTOCOL_RAW             // Raw timing data (fallback for unknown protocols)
} ir_protocol_t;

/**
 * @brief IR Code Structure
 *
 * Contains protocol-specific data and raw timing information
 */
typedef struct {
    ir_protocol_t protocol;     // Protocol type
    uint32_t data;              // NEC/Samsung: 32-bit command+address
    uint16_t bits;              // Number of bits (typically 32)
    uint16_t *raw_data;         // RAW protocol: pulse/space timing array
    uint16_t raw_length;        // RAW protocol: number of timing elements
} ir_code_t;

/**
 * @brief Universal Remote Button Definitions (32 buttons)
 */
typedef enum {
    // Power and Source
    IR_BTN_POWER = 0,           // Power On/Off
    IR_BTN_SOURCE,              // Input Source
    IR_BTN_MENU,                // Menu
    IR_BTN_HOME,                // Home
    IR_BTN_BACK,                // Back/Return
    IR_BTN_OK,                  // OK/Enter

    // Volume Control
    IR_BTN_VOL_UP,              // Volume Up
    IR_BTN_VOL_DN,              // Volume Down
    IR_BTN_MUTE,                // Mute

    // Channel Control
    IR_BTN_CH_UP,               // Channel Up
    IR_BTN_CH_DN,               // Channel Down

    // Number Buttons
    IR_BTN_0,                   // Number 0
    IR_BTN_1,                   // Number 1
    IR_BTN_2,                   // Number 2
    IR_BTN_3,                   // Number 3
    IR_BTN_4,                   // Number 4
    IR_BTN_5,                   // Number 5
    IR_BTN_6,                   // Number 6
    IR_BTN_7,                   // Number 7
    IR_BTN_8,                   // Number 8
    IR_BTN_9,                   // Number 9

    // Navigation
    IR_BTN_UP,                  // Navigation Up
    IR_BTN_DOWN,                // Navigation Down
    IR_BTN_LEFT,                // Navigation Left
    IR_BTN_RIGHT,               // Navigation Right

    // Custom/Extended
    IR_BTN_CUSTOM_1,            // Custom button 1
    IR_BTN_CUSTOM_2,            // Custom button 2
    IR_BTN_CUSTOM_3,            // Custom button 3
    IR_BTN_CUSTOM_4,            // Custom button 4
    IR_BTN_CUSTOM_5,            // Custom button 5
    IR_BTN_CUSTOM_6,            // Custom button 6

    IR_BTN_MAX                  // Total: 32 buttons
} ir_button_t;

/**
 * @brief Callback function types
 */
typedef void (*ir_learn_success_cb_t)(ir_button_t button, ir_code_t *code, void *arg);
typedef void (*ir_learn_fail_cb_t)(ir_button_t button, void *arg);
typedef void (*ir_receive_cb_t)(ir_code_t *code, void *arg);

/**
 * @brief Callback registration structure
 */
typedef struct {
    ir_learn_success_cb_t learn_success_cb;
    ir_learn_fail_cb_t learn_fail_cb;
    ir_receive_cb_t receive_cb;
    void *user_arg;
} ir_callbacks_t;

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

/**
 * @brief Initialize IR control component
 *
 * Initializes RMT TX/RX channels, encoders, and NVS storage
 *
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t ir_control_init(void);

/* ============================================================================
 * LEARNING MODE
 * ============================================================================ */

/**
 * @brief Start IR learning mode for a specific button
 *
 * Enters learning mode and waits for IR signal from remote control.
 * Automatically exits after timeout or successful learning.
 *
 * @param button Button identifier to learn (IR_BTN_POWER, IR_BTN_VOL_UP, etc.)
 * @param timeout_ms Timeout in milliseconds (0 = use default IR_LEARN_TIMEOUT_MS)
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if button invalid
 */
esp_err_t ir_learn_start(ir_button_t button, uint32_t timeout_ms);

/**
 * @brief Stop IR learning mode
 *
 * Exits learning mode without saving the code
 *
 * @return ESP_OK on success
 */
esp_err_t ir_learn_stop(void);

/**
 * @brief Check if IR learning is in progress
 *
 * @return true if learning mode active, false otherwise
 */
bool ir_is_learning(void);

/* ============================================================================
 * TRANSMISSION
 * ============================================================================ */

/**
 * @brief Transmit an IR code
 *
 * Transmits IR code using appropriate encoder (NEC/Samsung/RAW)
 *
 * @param code Pointer to IR code structure
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if code is NULL
 */
esp_err_t ir_transmit(ir_code_t *code);

/**
 * @brief Transmit learned code for a button
 *
 * @param button Button identifier
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if button not learned
 */
esp_err_t ir_transmit_button(ir_button_t button);

/* ============================================================================
 * NVS STORAGE
 * ============================================================================ */

/**
 * @brief Save a learned IR code to NVS
 *
 * @param button Button identifier
 * @param code Pointer to IR code to save
 * @return ESP_OK on success
 */
esp_err_t ir_save_code(ir_button_t button, ir_code_t *code);

/**
 * @brief Load a learned IR code from NVS
 *
 * @param button Button identifier
 * @param code Pointer to store loaded code
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if not saved
 */
esp_err_t ir_load_code(ir_button_t button, ir_code_t *code);

/**
 * @brief Save all learned IR codes to NVS
 *
 * @return ESP_OK on success
 */
esp_err_t ir_save_all_codes(void);

/**
 * @brief Load all learned IR codes from NVS
 *
 * @return ESP_OK on success
 */
esp_err_t ir_load_all_codes(void);

/**
 * @brief Clear a specific learned IR code
 *
 * @param button Button identifier to clear
 * @return ESP_OK on success
 */
esp_err_t ir_clear_code(ir_button_t button);

/**
 * @brief Clear all learned IR codes
 *
 * Clears all codes from memory and NVS
 *
 * @return ESP_OK on success
 */
esp_err_t ir_clear_all_codes(void);

/* ============================================================================
 * STATUS & QUERIES
 * ============================================================================ */

/**
 * @brief Check if a button has a learned code
 *
 * @param button Button identifier
 * @return true if button has learned code, false otherwise
 */
bool ir_is_learned(ir_button_t button);

/**
 * @brief Get button name string
 *
 * @param button Button identifier
 * @return Pointer to button name string (constant)
 */
const char* ir_get_button_name(ir_button_t button);

/**
 * @brief Get protocol name string
 *
 * @param protocol Protocol type
 * @return Pointer to protocol name string (constant)
 */
const char* ir_get_protocol_name(ir_protocol_t protocol);

/* ============================================================================
 * CALLBACK REGISTRATION
 * ============================================================================ */

/**
 * @brief Register callbacks for IR events
 *
 * @param callbacks Pointer to callback structure
 * @return ESP_OK on success
 */
esp_err_t ir_register_callbacks(const ir_callbacks_t *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* IR_CONTROL_H */
