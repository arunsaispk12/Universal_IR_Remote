/**
 * @file ir_ac_state.h
 * @brief Air Conditioner State-Based Control Model
 *
 * This module implements state-based AC control, treating AC as a stateful device
 * rather than a collection of buttons. This is the CORRECT approach for AC remotes
 * because most AC IR protocols send the COMPLETE state in every transmission.
 *
 * Why AC is Different:
 * - TV remote: Each button sends discrete command (Vol+, Ch+, Power)
 * - AC remote: Every button press sends FULL state (Power=ON, Mode=COOL, Temp=24, Fan=AUTO, etc.)
 *
 * Architecture:
 * 1. Maintain local AC state in firmware
 * 2. When RainMaker parameter changes, update state
 * 3. Regenerate complete IR frame from state using protocol encoder
 * 4. Transmit full state frame
 *
 * This matches how real AC remotes work and enables proper state synchronization.
 *
 * Supported Protocols:
 * - Daikin (multi-frame, 312-bit)
 * - Carrier/Voltas (128-bit with checksum)
 * - Hitachi (variable 264/344-bit)
 * - Mitsubishi (152-bit)
 * - Fujitsu (variable length)
 * - Haier (104-bit)
 * - Midea (48-bit, many brands)
 * - Samsung48 (48-bit for AC)
 * - Panasonic/Kaseikyo (48-bit)
 * - LG2 (28-bit AC variant)
 *
 * Copyright (c) 2025
 */

#ifndef IR_AC_STATE_H
#define IR_AC_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AC Operating Mode
 */
typedef enum {
    AC_MODE_OFF = 0,        // Power off (some protocols use separate power bit)
    AC_MODE_AUTO,           // Automatic mode (selects cool/heat/fan based on temp)
    AC_MODE_COOL,           // Cooling mode
    AC_MODE_HEAT,           // Heating mode
    AC_MODE_DRY,            // Dehumidify/Dry mode
    AC_MODE_FAN,            // Fan only (no cooling/heating)
    AC_MODE_MAX
} ac_mode_t;

/**
 * @brief AC Fan Speed
 */
typedef enum {
    AC_FAN_AUTO = 0,        // Automatic fan speed
    AC_FAN_LOW,             // Low speed
    AC_FAN_MEDIUM,          // Medium speed
    AC_FAN_HIGH,            // High speed
    AC_FAN_QUIET,           // Quiet/Silent mode (if supported)
    AC_FAN_TURBO,           // Turbo/Powerful mode (if supported)
    AC_FAN_MAX
} ac_fan_speed_t;

/**
 * @brief AC Swing Mode (Vertical Louver)
 */
typedef enum {
    AC_SWING_OFF = 0,       // Swing off (fixed position)
    AC_SWING_VERTICAL,      // Vertical swing (up-down)
    AC_SWING_HORIZONTAL,    // Horizontal swing (left-right)
    AC_SWING_BOTH,          // Both vertical and horizontal
    AC_SWING_AUTO,          // Automatic swing
    AC_SWING_MAX
} ac_swing_t;

/**
 * @brief Complete AC State
 *
 * Represents the full state of an air conditioner.
 * This is the "single source of truth" for AC control.
 */
typedef struct {
    /* Core State */
    bool power;                     // Power on/off
    ac_mode_t mode;                 // Operating mode
    uint8_t temperature;            // Target temperature (°C, typically 16-30)
    ac_fan_speed_t fan_speed;       // Fan speed
    ac_swing_t swing;               // Swing/Oscillation mode

    /* Extended Features (protocol-dependent) */
    bool turbo;                     // Turbo/Powerful mode
    bool quiet;                     // Quiet/Silent mode
    bool econo;                     // Economy/Energy-saving mode
    bool clean;                     // Self-clean mode
    bool sleep;                     // Sleep mode
    uint8_t sleep_timer;            // Sleep timer (minutes, 0 = disabled)
    bool display;                   // Display on/off
    bool beep;                      // Beep on/off
    bool filter;                    // Filter indicator
    bool light;                     // LED/Light on/off

    /* Indian Market Features */
    bool anti_fungal;               // Anti-fungal mode (Voltas, Blue Star)
    bool auto_clean;                // Auto-clean after power off
    uint8_t comfort_mode;           // Comfort preset (0 = off, 1-3 = levels)

    /* Protocol Identification */
    ir_protocol_t protocol;         // Which AC protocol to use for encoding
    uint8_t protocol_variant;       // Variant within protocol (e.g., Daikin 64 vs 128)

    /* Metadata */
    bool is_learned;                // Whether this AC has been learned/configured
    char brand[16];                 // Brand name (e.g., "Daikin", "Voltas")
    char model[16];                 // Model identifier (optional)
} ac_state_t;

/**
 * @brief AC Temperature Limits
 */
#define AC_TEMP_MIN         16      // Minimum temperature (°C)
#define AC_TEMP_MAX         30      // Maximum temperature (°C)
#define AC_TEMP_DEFAULT     24      // Default temperature (°C)

/* ============================================================================
 * AC STATE MANAGEMENT FUNCTIONS
 * ============================================================================ */

/**
 * @brief Initialize AC state management system
 *
 * Must be called after ir_control_init().
 *
 * @return ESP_OK on success
 */
esp_err_t ir_ac_state_init(void);

/**
 * @brief Get current AC state
 *
 * Returns pointer to the internal AC state structure.
 * Do not modify directly; use setter functions.
 *
 * @return Pointer to current AC state (read-only)
 */
const ac_state_t* ir_ac_state_get(void);

/**
 * @brief Set AC power state
 *
 * Updates AC power and triggers IR transmission.
 *
 * @param power true = on, false = off
 * @return ESP_OK on success
 */
esp_err_t ir_ac_set_power(bool power);

/**
 * @brief Set AC operating mode
 *
 * @param mode Operating mode (AUTO, COOL, HEAT, DRY, FAN)
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if mode invalid
 */
esp_err_t ir_ac_set_mode(ac_mode_t mode);

/**
 * @brief Set AC target temperature
 *
 * @param temperature Temperature in °C (16-30)
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range
 */
esp_err_t ir_ac_set_temperature(uint8_t temperature);

/**
 * @brief Set AC fan speed
 *
 * @param fan_speed Fan speed (AUTO, LOW, MEDIUM, HIGH, QUIET, TURBO)
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if invalid
 */
esp_err_t ir_ac_set_fan_speed(ac_fan_speed_t fan_speed);

/**
 * @brief Set AC swing mode
 *
 * @param swing Swing mode (OFF, VERTICAL, HORIZONTAL, BOTH, AUTO)
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if invalid
 */
esp_err_t ir_ac_set_swing(ac_swing_t swing);

/**
 * @brief Set multiple AC parameters atomically
 *
 * Updates multiple state fields and transmits once.
 * More efficient than calling individual setters.
 *
 * @param state New AC state (will be validated and applied)
 * @return ESP_OK on success
 */
esp_err_t ir_ac_set_state(const ac_state_t *state);

/**
 * @brief Set AC protocol (for encoding)
 *
 * Specifies which AC protocol to use when encoding state to IR.
 * Must be called after learning or manual configuration.
 *
 * @param protocol AC protocol (IR_PROTOCOL_DAIKIN, IR_PROTOCOL_CARRIER, etc.)
 * @param variant Protocol variant (0 = default, protocol-specific)
 * @return ESP_OK on success, ESP_ERR_NOT_SUPPORTED if protocol not an AC type
 */
esp_err_t ir_ac_set_protocol(ir_protocol_t protocol, uint8_t variant);

/* ============================================================================
 * AC PROTOCOL ENCODING
 * ============================================================================ */

/**
 * @brief Encode AC state to IR code
 *
 * Generates a complete IR frame from the current AC state using the
 * configured protocol encoder. This is the CORE function for state-based AC control.
 *
 * Example:
 * - State: Power=ON, Mode=COOL, Temp=24, Fan=AUTO, Swing=OFF
 * - Protocol: IR_PROTOCOL_DAIKIN
 * - Output: 312-bit Daikin IR frame with all state fields encoded
 *
 * @param state AC state to encode
 * @param code Output IR code buffer
 * @return ESP_OK on success, ESP_ERR_NOT_SUPPORTED if protocol not implemented
 */
esp_err_t ir_ac_encode_state(const ac_state_t *state, ir_code_t *code);

/**
 * @brief Decode IR code to AC state
 *
 * Reverse operation: Decodes a captured IR frame into AC state.
 * Used during learning to extract initial state from user's AC remote.
 *
 * @param code IR code to decode
 * @param state Output AC state buffer
 * @return ESP_OK on success, ESP_ERR_NOT_SUPPORTED if not an AC protocol
 */
esp_err_t ir_ac_decode_state(const ir_code_t *code, ac_state_t *state);

/**
 * @brief Transmit current AC state
 *
 * Encodes current state and transmits IR frame.
 * This is called automatically by setter functions.
 *
 * @return ESP_OK on success
 */
esp_err_t ir_ac_transmit_state(void);

/* ============================================================================
 * AC LEARNING MODE
 * ============================================================================ */

/**
 * @brief Learn AC protocol by capturing remote signals
 *
 * Captures multiple IR frames from user's AC remote to:
 * 1. Identify AC protocol (Daikin, Carrier, Hitachi, etc.)
 * 2. Extract initial state values
 * 3. Configure protocol encoder for future transmissions
 *
 * Learning flow:
 * 1. User presses "Power ON + Cool 24°C" on their AC remote
 * 2. Firmware captures IR, decodes protocol
 * 3. Extracts state: Power=ON, Mode=COOL, Temp=24, etc.
 * 4. Sets protocol encoder
 * 5. Future changes regenerate frames with this protocol
 *
 * @param timeout_ms Learning timeout
 * @return ESP_OK on success
 */
esp_err_t ir_ac_learn_protocol(uint32_t timeout_ms);

/**
 * @brief Check if AC protocol is configured
 *
 * @return true if AC has been learned/configured, false otherwise
 */
bool ir_ac_is_configured(void);

/* ============================================================================
 * NVS STORAGE
 * ============================================================================ */

/**
 * @brief Save AC state to NVS
 *
 * Persists current AC state and protocol configuration.
 * Called automatically when state changes.
 *
 * @return ESP_OK on success
 */
esp_err_t ir_ac_save_state(void);

/**
 * @brief Load AC state from NVS
 *
 * Restores AC state and protocol from persistent storage.
 * Called during initialization.
 *
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if not saved
 */
esp_err_t ir_ac_load_state(void);

/**
 * @brief Clear AC configuration (factory reset for AC)
 *
 * @return ESP_OK on success
 */
esp_err_t ir_ac_clear_state(void);

/* ============================================================================
 * PROTOCOL-SPECIFIC ENCODERS
 * ============================================================================ */

/**
 * @brief Daikin AC protocol encoder
 *
 * Supports:
 * - Daikin 64-bit (older models)
 * - Daikin 128-bit (common)
 * - Daikin 312-bit (multi-frame, most models)
 *
 * @param state AC state
 * @param code Output IR code
 * @return ESP_OK on success
 */
esp_err_t ir_ac_encode_daikin(const ac_state_t *state, ir_code_t *code);

/**
 * @brief Carrier/Voltas AC protocol encoder
 *
 * Used by: Voltas, Blue Star, Lloyd (India market leaders)
 * Format: 128-bit with nibble checksum
 *
 * @param state AC state
 * @param code Output IR code
 * @return ESP_OK on success
 */
esp_err_t ir_ac_encode_carrier(const ac_state_t *state, ir_code_t *code);

/**
 * @brief Hitachi AC protocol encoder
 *
 * Variable length: 264-bit or 344-bit
 * Used by Hitachi ACs (~10% India market)
 *
 * @param state AC state
 * @param code Output IR code
 * @return ESP_OK on success
 */
esp_err_t ir_ac_encode_hitachi(const ac_state_t *state, ir_code_t *code);

/**
 * @brief Mitsubishi AC protocol encoder
 *
 * Format: 152-bit
 *
 * @param state AC state
 * @param code Output IR code
 * @return ESP_OK on success
 */
esp_err_t ir_ac_encode_mitsubishi(const ac_state_t *state, ir_code_t *code);

/**
 * @brief Fujitsu AC protocol encoder
 *
 * Variable length protocol
 *
 * @param state AC state
 * @param code Output IR code
 * @return ESP_OK on success
 */
esp_err_t ir_ac_encode_fujitsu(const ac_state_t *state, ir_code_t *code);

/**
 * @brief Haier AC protocol encoder
 *
 * Format: 104-bit
 *
 * @param state AC state
 * @param code Output IR code
 * @return ESP_OK on success
 */
esp_err_t ir_ac_encode_haier(const ac_state_t *state, ir_code_t *code);

/**
 * @brief Midea AC protocol encoder
 *
 * Format: 48-bit
 * Used by many brands (Midea, Electrolux, Qlima, etc.)
 *
 * @param state AC state
 * @param code Output IR code
 * @return ESP_OK on success
 */
esp_err_t ir_ac_encode_midea(const ac_state_t *state, ir_code_t *code);

/**
 * @brief Samsung48 AC protocol encoder
 *
 * Format: 48-bit (Samsung AC units)
 *
 * @param state AC state
 * @param code Output IR code
 * @return ESP_OK on success
 */
esp_err_t ir_ac_encode_samsung48(const ac_state_t *state, ir_code_t *code);

/**
 * @brief Panasonic/Kaseikyo AC protocol encoder
 *
 * Format: 48-bit
 *
 * @param state AC state
 * @param code Output IR code
 * @return ESP_OK on success
 */
esp_err_t ir_ac_encode_panasonic(const ac_state_t *state, ir_code_t *code);

/**
 * @brief LG2 AC protocol encoder
 *
 * Format: 28-bit (LG AC variant)
 *
 * @param state AC state
 * @param code Output IR code
 * @return ESP_OK on success
 */
esp_err_t ir_ac_encode_lg2(const ac_state_t *state, ir_code_t *code);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * @brief Get AC mode name string
 *
 * @param mode AC mode
 * @return Pointer to mode name string (constant)
 */
const char* ir_ac_get_mode_name(ac_mode_t mode);

/**
 * @brief Get AC fan speed name string
 *
 * @param fan_speed Fan speed
 * @return Pointer to fan speed name string (constant)
 */
const char* ir_ac_get_fan_speed_name(ac_fan_speed_t fan_speed);

/**
 * @brief Get AC swing mode name string
 *
 * @param swing Swing mode
 * @return Pointer to swing mode name string (constant)
 */
const char* ir_ac_get_swing_name(ac_swing_t swing);

/**
 * @brief Validate AC state
 *
 * Checks if state values are within valid ranges.
 *
 * @param state AC state to validate
 * @return ESP_OK if valid, ESP_ERR_INVALID_ARG if invalid
 */
esp_err_t ir_ac_validate_state(const ac_state_t *state);

/**
 * @brief Get default AC state
 *
 * Returns a safe default state (Power=OFF, Mode=COOL, Temp=24, Fan=AUTO, etc.)
 *
 * @param state Output buffer for default state
 */
void ir_ac_get_default_state(ac_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* IR_AC_STATE_H */
