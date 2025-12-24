# RGB LED Status Component

A complete WS2812B RGB LED control component for ESP-IDF providing visual status feedback with non-blocking animations.

## Features

- **WS2812B Support**: Full RMT-based driver for WS2812B addressable LEDs
- **Non-blocking Operation**: FreeRTOS task handles all animations
- **Thread-safe**: Mutex-protected status updates
- **8 Status States**: Predefined visual patterns for common operations
- **Custom Colors**: Support for custom RGB color setting
- **Low Memory**: 2KB task stack, minimal heap usage
- **Configurable**: GPIO, LED count, and RMT channel

## Hardware Setup

**Default Configuration:**
- GPIO: 22
- LED Count: 1
- Protocol: WS2812B (800kHz)

**Wiring:**
```
ESP32 GPIO22 -----> WS2812B DIN
ESP32 5V ---------> WS2812B 5V
ESP32 GND --------> WS2812B GND
```

**Note:** Add a 470Ω resistor in series with the data line and a 1000µF capacitor across power for stability.

## LED Status Patterns

| Status | Color | Pattern | Use Case |
|--------|-------|---------|----------|
| `LED_STATUS_IDLE` | Dim Blue | Solid | System idle/ready |
| `LED_STATUS_LEARNING` | Purple | Pulsing | Learning IR signal |
| `LED_STATUS_LEARN_SUCCESS` | Green | Flash 3x | Learning succeeded |
| `LED_STATUS_LEARN_FAILED` | Red | Flash 3x | Learning failed |
| `LED_STATUS_TRANSMITTING` | Cyan | Flash 1x | Transmitting IR |
| `LED_STATUS_WIFI_CONNECTING` | Yellow | Pulsing | WiFi connecting |
| `LED_STATUS_WIFI_CONNECTED` | Green | Solid | WiFi connected |
| `LED_STATUS_ERROR` | Red | Blinking | Error state |
| `LED_STATUS_OFF` | Off | None | LED disabled |

## Quick Start

### 1. Include in Your Main Component

Add to your main component's `CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES rgb_led  # Add this line
)
```

### 2. Basic Usage

```c
#include "rgb_led.h"

void app_main(void)
{
    // Initialize with default settings (GPIO 22, 1 LED)
    rgb_led_init(NULL);

    // Set status to idle
    rgb_led_set_status(LED_STATUS_IDLE);

    // Your application code...

    // Indicate WiFi connecting
    rgb_led_set_status(LED_STATUS_WIFI_CONNECTING);

    // After WiFi connects
    rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);
}
```

### 3. Custom Configuration

```c
#include "rgb_led.h"

void app_main(void)
{
    // Custom configuration
    rgb_led_config_t config = {
        .gpio_num = 22,      // GPIO pin
        .led_count = 1,      // Number of LEDs
        .rmt_channel = 0     // RMT channel
    };

    rgb_led_init(&config);
    rgb_led_set_status(LED_STATUS_IDLE);
}
```

### 4. IR Learning Example

```c
void ir_learning_sequence(void)
{
    // Start learning
    rgb_led_set_status(LED_STATUS_LEARNING);

    // Wait for IR signal...
    bool success = learn_ir_signal();

    // Show result
    if (success) {
        rgb_led_set_status(LED_STATUS_LEARN_SUCCESS);
    } else {
        rgb_led_set_status(LED_STATUS_LEARN_FAILED);
    }

    // Status automatically returns to IDLE after flash sequence
}
```

### 5. Custom Color

```c
void set_custom_color(void)
{
    rgb_led_color_t orange = {
        .r = 255,
        .g = 165,
        .b = 0
    };

    rgb_led_set_color(&orange);
}
```

## API Reference

### Initialization

```c
esp_err_t rgb_led_init(const rgb_led_config_t *config);
```
Initialize the RGB LED component. Pass NULL for default configuration.

### Status Control

```c
esp_err_t rgb_led_set_status(rgb_led_status_t status);
```
Set LED status (non-blocking). Status change takes effect immediately.

```c
rgb_led_status_t rgb_led_get_status(void);
```
Get current LED status.

### Color Control

```c
esp_err_t rgb_led_set_color(const rgb_led_color_t *color);
```
Set custom RGB color. Overrides status-based animation.

```c
esp_err_t rgb_led_turn_off(void);
```
Turn off LED (sets status to `LED_STATUS_OFF`).

### Cleanup

```c
esp_err_t rgb_led_deinit(void);
```
Deinitialize component and free resources.

## Technical Details

### WS2812B Timing

- **Bit 0**: 0.4µs HIGH, 0.85µs LOW
- **Bit 1**: 0.8µs HIGH, 0.45µs LOW
- **Reset**: >50µs LOW
- **Frequency**: 800kHz

### Color Order

WS2812B uses **GRB** order internally. This component handles the conversion automatically - you always specify colors in RGB format.

### Animation Parameters

- **Update Rate**: 50ms (20Hz)
- **Pulse Period**: 2000ms (2 seconds)
- **Flash Duration**: 200ms ON, 200ms OFF
- **Blink Duration**: 500ms ON, 500ms OFF

### Memory Usage

- **Task Stack**: 2048 bytes
- **LED Buffer**: 3 bytes per LED
- **Overhead**: ~100 bytes (state structure)

### Thread Safety

All public API functions are thread-safe and can be called from any task. Internal mutex protects shared state.

## Configuration Options

### Adjusting GPIO Pin

Change the default GPIO in your initialization:

```c
rgb_led_config_t config = {
    .gpio_num = 25,  // Use GPIO 25 instead
    .led_count = 1,
    .rmt_channel = 0
};
rgb_led_init(&config);
```

### Multiple LEDs

Support for LED strips (all LEDs show the same color):

```c
rgb_led_config_t config = {
    .gpio_num = 22,
    .led_count = 8,  // 8 LEDs
    .rmt_channel = 0
};
rgb_led_init(&config);
```

### Custom Animation Timing

Edit these defines in `rgb_led.c`:

```c
#define PULSE_PERIOD_MS     2000  // Pulse cycle time
#define FLASH_ON_MS         200   // Flash ON duration
#define FLASH_OFF_MS        200   // Flash OFF duration
#define BLINK_ON_MS         500   // Blink ON duration
#define BLINK_OFF_MS        500   // Blink OFF duration
```

## Troubleshooting

### LED Not Lighting

1. Check power supply (WS2812B needs 5V, stable power)
2. Verify GPIO connection
3. Check if RMT channel is not used by other peripherals
4. Add 470Ω resistor on data line

### Colors Are Wrong

1. Some WS2812B variants use RGB order instead of GRB
2. Edit `set_pixel_color()` in `rgb_led.c` to swap color order

### LED Flickers

1. Add 1000µF capacitor across power supply
2. Use shorter wires (< 1 meter recommended)
3. Check power supply quality

### Task Not Starting

1. Check available heap memory: `esp_get_free_heap_size()`
2. Reduce `RGB_LED_TASK_STACK_SIZE` if needed
3. Check task priority conflicts

## Integration Examples

### With WiFi Manager

```c
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                       int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        rgb_led_set_status(LED_STATUS_WIFI_CONNECTING);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);
    }
}
```

### With Error Handling

```c
void error_handler(esp_err_t err)
{
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error occurred: %s", esp_err_to_name(err));
        rgb_led_set_status(LED_STATUS_ERROR);
    }
}
```

## Component Files

```
rgb_led/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── include/
│   ├── rgb_led.h              # Main API header
│   └── led_strip_encoder.h    # RMT encoder header
├── rgb_led.c                  # Main implementation
└── led_strip_encoder.c        # RMT encoder implementation
```

## License

This component is provided as-is for use in ESP-IDF projects.

## Version

- **Version**: 1.0.0
- **ESP-IDF**: Compatible with v5.0+
- **Tested on**: ESP32, ESP32-S3
