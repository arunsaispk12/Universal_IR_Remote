# IR Control Component

Universal IR Remote Control Component for ESP32

Based on SHA_RainMaker_Project v1.0.0 IR implementation

## Features

- **Multi-Protocol Support**
  - NEC protocol (standard TV/AC remotes)
  - Samsung protocol variant
  - RAW protocol (fallback for unknown protocols)

- **IR Learning Mode**
  - 30-second timeout (configurable)
  - Automatic protocol detection
  - Success/failure callbacks
  - NVS storage for learned codes

- **32 Button Support**
  - Power, Source, Menu, Home, Back, OK
  - Volume Up/Down, Mute
  - Channel Up/Down
  - Number buttons (0-9)
  - Navigation (Up, Down, Left, Right)
  - 6 Custom buttons

- **Thread-Safe Operation**
  - Mutex-protected code storage
  - FreeRTOS task-based receiver
  - ISR-safe callbacks

- **RMT-Based Implementation**
  - GPIO 17: IR Transmitter
  - GPIO 18: IR Receiver (active-LOW with inversion)
  - 38kHz carrier frequency
  - 1MHz resolution (1us precision)

## Directory Structure

```
ir_control/
├── include/
│   └── ir_control.h      # Public API header
├── ir_control.c          # Implementation
├── CMakeLists.txt        # Component build config
└── README.md             # This file
```

## Hardware Requirements

- **IR Transmitter**: IR LED connected to GPIO 17
  - Recommended: TSAL6200 or similar 940nm IR LED
  - Current limiting resistor required (typically 100-220 ohm)

- **IR Receiver**: IRM-3638T, VS1838B, or compatible
  - Connected to GPIO 18
  - Active-LOW output (component handles inversion)
  - VCC: 3.3V, GND: GND

## Usage Example

```c
#include "ir_control.h"

// Callback for successful learning
void learn_success_cb(ir_button_t button, ir_code_t *code, void *arg)
{
    printf("Learned button '%s' - Protocol: %s\n",
           ir_get_button_name(button),
           ir_get_protocol_name(code->protocol));
}

// Callback for learning failure
void learn_fail_cb(ir_button_t button, void *arg)
{
    printf("Failed to learn button '%s'\n", ir_get_button_name(button));
}

// Callback for received IR codes
void receive_cb(ir_code_t *code, void *arg)
{
    printf("Received IR code - Protocol: %s\n",
           ir_get_protocol_name(code->protocol));
}

void app_main(void)
{
    // Initialize NVS
    nvs_flash_init();

    // Initialize IR control
    ir_control_init();

    // Register callbacks
    ir_callbacks_t callbacks = {
        .learn_success_cb = learn_success_cb,
        .learn_fail_cb = learn_fail_cb,
        .receive_cb = receive_cb,
        .user_arg = NULL
    };
    ir_register_callbacks(&callbacks);

    // Start learning mode for POWER button (30 second timeout)
    ir_learn_start(IR_BTN_POWER, 0);

    // Wait for learning to complete...

    // Transmit learned POWER button
    if (ir_is_learned(IR_BTN_POWER)) {
        ir_transmit_button(IR_BTN_POWER);
    }

    // Save all learned codes to NVS
    ir_save_all_codes();

    // Clear a specific button
    ir_clear_code(IR_BTN_POWER);

    // Clear all buttons
    ir_clear_all_codes();
}
```

## API Reference

### Initialization

- `esp_err_t ir_control_init(void)` - Initialize IR control component

### Learning Mode

- `esp_err_t ir_learn_start(ir_button_t button, uint32_t timeout_ms)` - Start learning
- `esp_err_t ir_learn_stop(void)` - Stop learning
- `bool ir_is_learning(void)` - Check if learning active

### Transmission

- `esp_err_t ir_transmit(ir_code_t *code)` - Transmit IR code
- `esp_err_t ir_transmit_button(ir_button_t button)` - Transmit learned button

### NVS Storage

- `esp_err_t ir_save_code(ir_button_t button, ir_code_t *code)` - Save single code
- `esp_err_t ir_load_code(ir_button_t button, ir_code_t *code)` - Load single code
- `esp_err_t ir_save_all_codes(void)` - Save all codes
- `esp_err_t ir_load_all_codes(void)` - Load all codes
- `esp_err_t ir_clear_code(ir_button_t button)` - Clear single code
- `esp_err_t ir_clear_all_codes(void)` - Clear all codes

### Status & Queries

- `bool ir_is_learned(ir_button_t button)` - Check if button learned
- `const char* ir_get_button_name(ir_button_t button)` - Get button name
- `const char* ir_get_protocol_name(ir_protocol_t protocol)` - Get protocol name

### Callback Registration

- `esp_err_t ir_register_callbacks(const ir_callbacks_t *callbacks)` - Register callbacks

## Button Definitions

```c
typedef enum {
    IR_BTN_POWER,    IR_BTN_SOURCE,   IR_BTN_MENU,
    IR_BTN_HOME,     IR_BTN_BACK,     IR_BTN_OK,
    IR_BTN_VOL_UP,   IR_BTN_VOL_DN,   IR_BTN_MUTE,
    IR_BTN_CH_UP,    IR_BTN_CH_DN,
    IR_BTN_0,        IR_BTN_1,        IR_BTN_2,
    IR_BTN_3,        IR_BTN_4,        IR_BTN_5,
    IR_BTN_6,        IR_BTN_7,        IR_BTN_8,
    IR_BTN_9,
    IR_BTN_UP,       IR_BTN_DOWN,     IR_BTN_LEFT,
    IR_BTN_RIGHT,
    IR_BTN_CUSTOM_1, IR_BTN_CUSTOM_2, IR_BTN_CUSTOM_3,
    IR_BTN_CUSTOM_4, IR_BTN_CUSTOM_5, IR_BTN_CUSTOM_6,
    IR_BTN_MAX       // Total: 32 buttons
} ir_button_t;
```

## Protocol Support

### NEC Protocol
- Leading code: 9ms HIGH + 4.5ms LOW
- Bit encoding: 560us HIGH + (560us/1690us) LOW
- 32-bit data: address + ~address + command + ~command
- Checksum verification

### Samsung Protocol
- Leading code: 4.5ms HIGH + 4.5ms LOW
- Same bit encoding as NEC
- 32-bit data (no checksum)

### RAW Protocol
- Fallback for unknown protocols
- Stores raw RMT symbol timing
- Up to 256 symbols (512 pulses)
- Exact replay of captured signal

## Thread Safety

All public API functions are thread-safe through:
- Mutex-protected code storage access
- FreeRTOS queue for RX events
- ISR-safe callbacks

## Memory Usage

- **Static RAM**: ~4KB (code storage + buffers)
- **Stack**: 8KB (IR receive task)
- **Dynamic RAM**: Variable (RAW codes only)
  - NEC/Samsung: 0 bytes
  - RAW: ~2-4 bytes per symbol

## Error Handling

All API functions return `esp_err_t`:
- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid parameter
- `ESP_ERR_NOT_FOUND` - Button not learned
- `ESP_ERR_NO_MEM` - Memory allocation failed
- `ESP_FAIL` - General failure

## Configuration

Edit `ir_control.h` to customize:
- `IR_TX_GPIO` - Transmitter GPIO (default: 17)
- `IR_RX_GPIO` - Receiver GPIO (default: 18)
- `IR_MAX_CODE_LENGTH` - Max RAW symbols (default: 256)
- `IR_CARRIER_FREQ_HZ` - Carrier frequency (default: 38000)
- `IR_LEARN_TIMEOUT_MS` - Learning timeout (default: 30000)

## Troubleshooting

**IR receiver not detecting signals:**
- Check GPIO 18 connection
- Verify receiver is powered (3.3V)
- Ensure remote uses 38kHz carrier
- Increase `IR_TIMING_TOLERANCE_US` for non-standard remotes

**Transmission not working:**
- Check GPIO 17 IR LED connection
- Verify current limiting resistor
- Ensure carrier frequency matches receiver (38kHz)
- Check LED polarity

**Learning timeout:**
- Hold remote closer (< 30cm)
- Press button firmly
- Avoid ambient IR interference
- Check receiver sensitivity

**RAW codes not working:**
- Verify enough memory available
- Check symbol count (10-256 valid range)
- Ensure transmission waits for completion

## License

Based on SHA_RainMaker_Project v1.0.0
ESP-IDF components are licensed under Apache License 2.0

## Version

v1.0.0 - Initial release for Universal IR Remote project
