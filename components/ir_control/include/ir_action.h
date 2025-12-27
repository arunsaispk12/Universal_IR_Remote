/**
 * @file ir_action.h
 * @brief Logical Action Mapping for Universal IR Remote
 *
 * This module provides abstraction between RainMaker device parameters and IR codes.
 * Each appliance action (e.g., "Volume Up") is mapped to a stored IR code.
 * This allows re-learning IR signals without changing the RainMaker cloud schema.
 *
 * Architecture:
 * RainMaker Parameter Change → Logical Action → Stored IR Code → IR Transmission
 *
 * Example:
 * User changes TV.Volume parameter → IR_ACTION_VOL_UP → NVS lookup → Transmit stored code
 *
 * Copyright (c) 2025
 */

#ifndef IR_ACTION_H
#define IR_ACTION_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "ir_control.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IR Device Types
 *
 * Represents the type of appliance being controlled.
 * Maps to RainMaker device types for proper cloud representation.
 */
typedef enum {
    IR_DEVICE_NONE = 0,
    IR_DEVICE_TV,              // Television
    IR_DEVICE_AC,              // Air Conditioner (state-based)
    IR_DEVICE_STB,             // Set-Top Box (DTH/Cable)
    IR_DEVICE_SPEAKER,         // Soundbar/Home Theater
    IR_DEVICE_FAN,             // IR-controlled fan
    IR_DEVICE_MAX
} ir_device_type_t;

/**
 * @brief Logical IR Actions
 *
 * Complete list of IR actions across all device types.
 * Each action represents a user-facing function, not a raw IR code.
 *
 * Naming convention: IR_ACTION_<DEVICE>_<FUNCTION>
 * Exception: Common actions (POWER, VOL_UP, etc.) work across devices
 */
typedef enum {
    IR_ACTION_NONE = 0,

    /* ===== COMMON ACTIONS (TV, STB, Speaker) ===== */
    IR_ACTION_POWER,                // Power On/Off toggle
    IR_ACTION_POWER_ON,             // Discrete Power On
    IR_ACTION_POWER_OFF,            // Discrete Power Off

    /* Volume Control */
    IR_ACTION_VOL_UP,               // Volume Up
    IR_ACTION_VOL_DOWN,             // Volume Down
    IR_ACTION_MUTE,                 // Mute/Unmute toggle

    /* Channel Control (TV, STB) */
    IR_ACTION_CH_UP,                // Channel Up
    IR_ACTION_CH_DOWN,              // Channel Down
    IR_ACTION_CH_PREV,              // Previous Channel

    /* Navigation (TV, STB) */
    IR_ACTION_NAV_UP,               // Navigation Up
    IR_ACTION_NAV_DOWN,             // Navigation Down
    IR_ACTION_NAV_LEFT,             // Navigation Left
    IR_ACTION_NAV_RIGHT,            // Navigation Right
    IR_ACTION_NAV_OK,               // OK/Select/Enter

    /* Menu Navigation (TV, STB) */
    IR_ACTION_MENU,                 // Menu
    IR_ACTION_HOME,                 // Home
    IR_ACTION_BACK,                 // Back/Return
    IR_ACTION_EXIT,                 // Exit
    IR_ACTION_INFO,                 // Info/Display

    /* Number Keys (TV, STB) */
    IR_ACTION_NUM_0,                // Number 0
    IR_ACTION_NUM_1,                // Number 1
    IR_ACTION_NUM_2,                // Number 2
    IR_ACTION_NUM_3,                // Number 3
    IR_ACTION_NUM_4,                // Number 4
    IR_ACTION_NUM_5,                // Number 5
    IR_ACTION_NUM_6,                // Number 6
    IR_ACTION_NUM_7,                // Number 7
    IR_ACTION_NUM_8,                // Number 8
    IR_ACTION_NUM_9,                // Number 9

    /* ===== TV-SPECIFIC ACTIONS ===== */
    IR_ACTION_TV_INPUT,             // Input/Source selection
    IR_ACTION_TV_INPUT_HDMI1,       // Discrete HDMI1
    IR_ACTION_TV_INPUT_HDMI2,       // Discrete HDMI2
    IR_ACTION_TV_INPUT_HDMI3,       // Discrete HDMI3
    IR_ACTION_TV_INPUT_AV,          // Discrete AV/Composite
    IR_ACTION_TV_INPUT_USB,         // Discrete USB
    IR_ACTION_TV_PICTURE_MODE,      // Picture Mode (Standard, Vivid, Cinema, etc.)
    IR_ACTION_TV_SOUND_MODE,        // Sound Mode (Standard, Music, Movie, etc.)
    IR_ACTION_TV_SLEEP_TIMER,       // Sleep Timer

    /* ===== STB-SPECIFIC ACTIONS ===== */
    IR_ACTION_STB_GUIDE,            // EPG/Guide
    IR_ACTION_STB_RECORD,           // Record
    IR_ACTION_STB_PLAY_PAUSE,       // Play/Pause
    IR_ACTION_STB_STOP,             // Stop
    IR_ACTION_STB_REWIND,           // Rewind/Fast Backward
    IR_ACTION_STB_FORWARD,          // Fast Forward
    IR_ACTION_STB_PREV_TRACK,       // Previous Track/Chapter
    IR_ACTION_STB_NEXT_TRACK,       // Next Track/Chapter
    IR_ACTION_STB_SUBTITLE,         // Subtitle toggle
    IR_ACTION_STB_AUDIO,            // Audio language selection

    /* ===== SPEAKER/SOUNDBAR ACTIONS ===== */
    IR_ACTION_SPEAKER_MODE,         // Sound mode (Music, Movie, News, etc.)
    IR_ACTION_SPEAKER_BASS_UP,      // Bass increase
    IR_ACTION_SPEAKER_BASS_DOWN,    // Bass decrease
    IR_ACTION_SPEAKER_TREBLE_UP,    // Treble increase
    IR_ACTION_SPEAKER_TREBLE_DOWN,  // Treble decrease
    IR_ACTION_SPEAKER_SURROUND,     // Surround sound toggle
    IR_ACTION_SPEAKER_BLUETOOTH,    // Bluetooth mode
    IR_ACTION_SPEAKER_AUX,          // AUX input
    IR_ACTION_SPEAKER_OPTICAL,      // Optical input
    IR_ACTION_SPEAKER_SUBWOOFER,    // Subwoofer level

    /* ===== FAN ACTIONS ===== */
    IR_ACTION_FAN_SPEED_UP,         // Increase fan speed
    IR_ACTION_FAN_SPEED_DOWN,       // Decrease fan speed
    IR_ACTION_FAN_SPEED_1,          // Discrete Speed 1 (Low)
    IR_ACTION_FAN_SPEED_2,          // Discrete Speed 2
    IR_ACTION_FAN_SPEED_3,          // Discrete Speed 3
    IR_ACTION_FAN_SPEED_4,          // Discrete Speed 4
    IR_ACTION_FAN_SPEED_5,          // Discrete Speed 5 (High)
    IR_ACTION_FAN_SWING,            // Oscillation toggle
    IR_ACTION_FAN_TIMER,            // Timer
    IR_ACTION_FAN_SLEEP_MODE,       // Sleep mode
    IR_ACTION_FAN_NATURAL_WIND,     // Natural wind mode
    IR_ACTION_FAN_IONIZER,          // Ionizer/Air purifier

    /* ===== AC ACTIONS (State-based - See ir_ac_state.h) ===== */
    /* Note: AC control uses full state regeneration, not discrete actions */
    /* These are for backward compatibility with non-state-aware AC remotes */
    IR_ACTION_AC_TEMP_UP,           // Temperature increase (fallback)
    IR_ACTION_AC_TEMP_DOWN,         // Temperature decrease (fallback)
    IR_ACTION_AC_MODE_COOL,         // Discrete Cool mode
    IR_ACTION_AC_MODE_HEAT,         // Discrete Heat mode
    IR_ACTION_AC_MODE_FAN,          // Discrete Fan mode
    IR_ACTION_AC_MODE_DRY,          // Discrete Dry/Dehumidify mode
    IR_ACTION_AC_MODE_AUTO,         // Discrete Auto mode
    IR_ACTION_AC_FAN_SPEED_UP,      // Fan speed increase
    IR_ACTION_AC_FAN_SPEED_DOWN,    // Fan speed decrease
    IR_ACTION_AC_SWING,             // Swing/Oscillation toggle

    /* ===== TOTAL ACTIONS ===== */
    IR_ACTION_MAX                   // Total: 100+ actions
} ir_action_t;

/**
 * @brief Action-to-IR-Code Mapping Entry
 *
 * Maps a logical action to a stored IR code in NVS.
 * This structure is used internally for action resolution.
 */
typedef struct {
    ir_device_type_t device;        // Device type
    ir_action_t action;             // Logical action
    char nvs_key[16];               // NVS storage key (e.g., "tv_power", "ac_cool")
    bool is_learned;                // Whether IR code is learned
} ir_action_mapping_t;

/* ============================================================================
 * ACTION MANAGEMENT FUNCTIONS
 * ============================================================================ */

/**
 * @brief Initialize action mapping system
 *
 * Must be called after ir_control_init() and before using action APIs.
 *
 * @return ESP_OK on success
 */
esp_err_t ir_action_init(void);

/**
 * @brief Learn an IR code for a specific device action
 *
 * Enters learning mode and associates the captured IR code with the given action.
 * Uses multi-frame verification (2-3 frames) for reliability.
 *
 * @param device Device type (IR_DEVICE_TV, IR_DEVICE_AC, etc.)
 * @param action Logical action to learn
 * @param timeout_ms Learning timeout in milliseconds
 * @return ESP_OK on success, ESP_ERR_TIMEOUT on timeout, ESP_ERR_INVALID_ARG if invalid
 */
esp_err_t ir_action_learn(ir_device_type_t device, ir_action_t action, uint32_t timeout_ms);

/**
 * @brief Execute a learned action (transmit IR code)
 *
 * Looks up the stored IR code for the given device+action and transmits it.
 * For AC devices, this may regenerate full state frame (see ir_ac_state.h).
 *
 * @param device Device type
 * @param action Logical action to execute
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if action not learned
 */
esp_err_t ir_action_execute(ir_device_type_t device, ir_action_t action);

/**
 * @brief Execute action with long-press (auto-repeat)
 *
 * Transmits the action repeatedly with protocol-correct repeat frames.
 * Used for volume/channel control.
 *
 * @param device Device type
 * @param action Logical action to repeat
 * @param repeat_count Number of times to repeat (0 = until released)
 * @param repeat_interval_ms Time between repeats (0 = use protocol default)
 * @return ESP_OK on success
 */
esp_err_t ir_action_execute_repeat(ir_device_type_t device, ir_action_t action,
                                     uint8_t repeat_count, uint16_t repeat_interval_ms);

/**
 * @brief Check if an action has a learned IR code
 *
 * @param device Device type
 * @param action Logical action
 * @return true if learned, false otherwise
 */
bool ir_action_is_learned(ir_device_type_t device, ir_action_t action);

/**
 * @brief Save action mapping to NVS
 *
 * Associates an IR code with a device+action and saves to persistent storage.
 * Called automatically by ir_action_learn().
 *
 * @param device Device type
 * @param action Logical action
 * @param code IR code to save
 * @return ESP_OK on success
 */
esp_err_t ir_action_save(ir_device_type_t device, ir_action_t action, const ir_code_t *code);

/**
 * @brief Load action mapping from NVS
 *
 * Retrieves the stored IR code for a device+action.
 *
 * @param device Device type
 * @param action Logical action
 * @param code Output buffer for IR code
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if not learned
 */
esp_err_t ir_action_load(ir_device_type_t device, ir_action_t action, ir_code_t *code);

/**
 * @brief Clear a specific action mapping
 *
 * @param device Device type
 * @param action Logical action to clear
 * @return ESP_OK on success
 */
esp_err_t ir_action_clear(ir_device_type_t device, ir_action_t action);

/**
 * @brief Clear all action mappings for a device
 *
 * @param device Device type
 * @return ESP_OK on success
 */
esp_err_t ir_action_clear_device(ir_device_type_t device);

/**
 * @brief Clear all action mappings (factory reset)
 *
 * @return ESP_OK on success
 */
esp_err_t ir_action_clear_all(void);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * @brief Get device type name string
 *
 * @param device Device type
 * @return Pointer to device name string (constant)
 */
const char* ir_action_get_device_name(ir_device_type_t device);

/**
 * @brief Get action name string
 *
 * @param action Logical action
 * @return Pointer to action name string (constant)
 */
const char* ir_action_get_action_name(ir_action_t action);

/**
 * @brief Generate NVS key for device+action
 *
 * Creates a unique storage key (e.g., "tv_power", "ac_mode_cool").
 * Used internally but exposed for testing.
 *
 * @param device Device type
 * @param action Logical action
 * @param key_buffer Output buffer (min 16 bytes)
 * @param buffer_size Size of output buffer
 * @return ESP_OK on success, ESP_ERR_INVALID_SIZE if buffer too small
 */
esp_err_t ir_action_generate_nvs_key(ir_device_type_t device, ir_action_t action,
                                       char *key_buffer, size_t buffer_size);

/**
 * @brief Get all actions for a device type
 *
 * Returns a list of applicable actions for the given device.
 * Used by RainMaker to populate UI options.
 *
 * @param device Device type
 * @param actions Output array of actions
 * @param max_actions Size of output array
 * @param action_count Output: number of actions written
 * @return ESP_OK on success
 */
esp_err_t ir_action_get_device_actions(ir_device_type_t device, ir_action_t *actions,
                                         size_t max_actions, size_t *action_count);

#ifdef __cplusplus
}
#endif

#endif /* IR_ACTION_H */
