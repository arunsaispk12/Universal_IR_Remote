# RGB LED Component - Quick Reference

## Installation

1. Component is already in `components/rgb_led/`
2. Add to your main component's `CMakeLists.txt`:
   ```cmake
   REQUIRES rgb_led
   ```
3. Include header in your code:
   ```c
   #include "rgb_led.h"
   ```

## Minimal Setup (3 lines)

```c
#include "rgb_led.h"

void app_main(void) {
    rgb_led_init(NULL);                      // Initialize with defaults
    rgb_led_set_status(LED_STATUS_IDLE);     // Set initial status
    // Your app code here...
}
```

## Common Status Codes

```c
// Visual Feedback States
rgb_led_set_status(LED_STATUS_IDLE);            // Dim blue - ready
rgb_led_set_status(LED_STATUS_LEARNING);        // Purple pulse - learning
rgb_led_set_status(LED_STATUS_LEARN_SUCCESS);   // Green flash 3x - success
rgb_led_set_status(LED_STATUS_LEARN_FAILED);    // Red flash 3x - failed
rgb_led_set_status(LED_STATUS_TRANSMITTING);    // Cyan flash - transmitting
rgb_led_set_status(LED_STATUS_WIFI_CONNECTING); // Yellow pulse - connecting
rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);  // Green solid - connected
rgb_led_set_status(LED_STATUS_ERROR);           // Red blink - error
rgb_led_set_status(LED_STATUS_OFF);             // Off
```

## Color Reference

| Status | RGB Values | Visual |
|--------|-----------|---------|
| IDLE | (0, 0, 30) | Dim Blue |
| LEARNING | (128, 0, 128) | Purple |
| SUCCESS | (0, 255, 0) | Green |
| FAILED | (255, 0, 0) | Red |
| TRANSMITTING | (0, 255, 255) | Cyan |
| WIFI_CONNECTING | (255, 255, 0) | Yellow |
| WIFI_CONNECTED | (0, 255, 0) | Green |
| ERROR | (255, 0, 0) | Red |

## Custom Configuration

```c
rgb_led_config_t config = {
    .gpio_num = 22,      // GPIO pin (default: 22)
    .led_count = 1,      // Number of LEDs (default: 1)
    .rmt_channel = 0     // RMT channel (default: 0)
};
rgb_led_init(&config);
```

## Custom Colors

```c
rgb_led_color_t my_color = {
    .r = 255,  // Red: 0-255
    .g = 128,  // Green: 0-255
    .b = 0     // Blue: 0-255
};
rgb_led_set_color(&my_color);
```

## API Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `rgb_led_init(config)` | Initialize component | `ESP_OK` or error |
| `rgb_led_set_status(status)` | Set status (non-blocking) | `ESP_OK` or error |
| `rgb_led_set_color(color)` | Set custom color | `ESP_OK` or error |
| `rgb_led_turn_off()` | Turn LED off | `ESP_OK` or error |
| `rgb_led_get_status()` | Get current status | `rgb_led_status_t` |
| `rgb_led_deinit()` | Cleanup and free resources | `ESP_OK` or error |

## Typical IR Remote Usage

```c
void app_main(void) {
    // 1. Initialize
    rgb_led_init(NULL);
    rgb_led_set_status(LED_STATUS_WIFI_CONNECTING);

    // 2. WiFi setup
    wifi_init();
    // ... when connected ...
    rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);

    // 3. In IR learning handler
    void on_learn_button_press(void) {
        rgb_led_set_status(LED_STATUS_LEARNING);
        bool success = learn_ir_code();
        rgb_led_set_status(success ? LED_STATUS_LEARN_SUCCESS : LED_STATUS_LEARN_FAILED);
    }

    // 4. In IR transmit handler
    void on_transmit_button_press(void) {
        rgb_led_set_status(LED_STATUS_TRANSMITTING);
        transmit_ir_code();
    }

    // 5. In error handler
    void on_error(void) {
        rgb_led_set_status(LED_STATUS_ERROR);
    }
}
```

## Hardware Connection

```
ESP32 Pin 22 (GPIO22) ──┬─── WS2812B DIN
                       └─── 470Ω resistor recommended

ESP32 5V ─────────────────── WS2812B VCC (5V)
ESP32 GND ────────────────── WS2812B GND

Optional: 1000µF capacitor across VCC/GND near LED
```

## Timing Behavior

- **Pulse animations**: 2 second cycle
- **Flash (success/fail)**: 3 flashes, auto-returns to IDLE
- **Transmit flash**: 1 flash, auto-returns to IDLE
- **Update rate**: 50ms (20 Hz)
- **All operations**: Non-blocking

## Thread Safety

- All API calls are thread-safe
- Can call from any FreeRTOS task
- Internal mutex protection
- Safe for ISR context (but not recommended - use task notification instead)

## Memory Usage

- **Flash**: ~8KB code
- **RAM**: 2KB stack + 3 bytes per LED
- **Heap**: ~200 bytes

## Troubleshooting

| Issue | Solution |
|-------|----------|
| LED not lighting | Check 5V power, GPIO connection, add 470Ω resistor |
| Wrong colors | Some WS2812B variants use different color order |
| Flickering | Add capacitor across power, shorten wires |
| Task won't start | Check heap memory, reduce stack size if needed |

## Common Patterns

### Temporary Status with Auto-Return
```c
// These auto-return to IDLE after animation:
rgb_led_set_status(LED_STATUS_LEARN_SUCCESS);  // Flashes then idle
rgb_led_set_status(LED_STATUS_TRANSMITTING);   // Flashes then idle
```

### Persistent Status
```c
// These stay until changed:
rgb_led_set_status(LED_STATUS_WIFI_CONNECTED); // Stays green
rgb_led_set_status(LED_STATUS_ERROR);          // Keeps blinking
rgb_led_set_status(LED_STATUS_IDLE);           // Stays dim blue
```

### Querying Status
```c
rgb_led_status_t current = rgb_led_get_status();
if (current == LED_STATUS_ERROR) {
    // Handle error state
}
```

## Color Mixing Examples

```c
// Common colors
rgb_led_color_t white   = {255, 255, 255};
rgb_led_color_t orange  = {255, 165, 0};
rgb_led_color_t pink    = {255, 105, 180};
rgb_led_color_t purple  = {128, 0, 128};
rgb_led_color_t lime    = {128, 255, 0};

rgb_led_set_color(&white);
```

## Build Requirements

- ESP-IDF v5.0 or later
- Components: `driver`, `esp_timer`
- Compatible: ESP32, ESP32-S2, ESP32-S3, ESP32-C3

## Files Overview

```
rgb_led/
├── include/
│   ├── rgb_led.h              - Main API (use this)
│   └── led_strip_encoder.h    - RMT encoder (internal)
├── rgb_led.c                  - Main implementation
├── led_strip_encoder.c        - WS2812B timing
├── CMakeLists.txt             - Build config
├── README.md                  - Full documentation
├── QUICK_REFERENCE.md         - This file
└── example_usage.c            - Example code
```

## Next Steps

1. Wire up your WS2812B LED to GPIO 22
2. Add `REQUIRES rgb_led` to your CMakeLists.txt
3. Call `rgb_led_init(NULL)` in app_main()
4. Use `rgb_led_set_status()` throughout your code
5. See `example_usage.c` for complete examples

## Support

- Full documentation: `README.md`
- Example code: `example_usage.c`
- ESP-IDF docs: https://docs.espressif.com/projects/esp-idf/

---

**Version**: 1.0.0 | **Last Updated**: 2024
