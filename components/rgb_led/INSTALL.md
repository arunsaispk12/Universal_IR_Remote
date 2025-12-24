# RGB LED Component - Installation Guide

## Component Installation

The RGB LED component is already installed in your project at:
```
C:\Users\JYOTH\Desktop\ESP_IDF\Project_SHA\Universal_IR_Remote\components\rgb_led\
```

## Integration Steps

### Step 1: Update Your Main Component CMakeLists.txt

Edit your main component's `CMakeLists.txt` file and add `rgb_led` to the REQUIRES list:

**Location**: `C:\Users\JYOTH\Desktop\ESP_IDF\Project_SHA\Universal_IR_Remote\main\CMakeLists.txt`

```cmake
idf_component_register(
    SRCS "main.c"
         # ... your other source files ...
    INCLUDE_DIRS "."
    REQUIRES rgb_led      # <-- Add this line
             # ... other required components ...
)
```

### Step 2: Include Header in Your Code

Add the include directive in your `main.c` or relevant source files:

```c
#include "rgb_led.h"
```

### Step 3: Initialize in app_main()

Add initialization code at the start of your `app_main()` function:

```c
void app_main(void)
{
    // Initialize RGB LED
    rgb_led_config_t led_config = {
        .gpio_num = 22,      // GPIO pin for WS2812B data
        .led_count = 1,      // Number of LEDs
        .rmt_channel = 0     // RMT channel (0-3)
    };

    esp_err_t ret = rgb_led_init(&led_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RGB LED: %s", esp_err_to_name(ret));
    }

    // Set initial status
    rgb_led_set_status(LED_STATUS_IDLE);

    // ... rest of your code ...
}
```

Or use default configuration:

```c
void app_main(void)
{
    // Initialize with defaults (GPIO 22, 1 LED, RMT channel 0)
    rgb_led_init(NULL);
    rgb_led_set_status(LED_STATUS_IDLE);

    // ... rest of your code ...
}
```

### Step 4: Hardware Connection

Connect your WS2812B LED to the ESP32:

```
┌─────────────┐         ┌─────────────┐
│   ESP32     │         │  WS2812B    │
│             │         │             │
│  GPIO 22 ───┼────┬────┼─── DIN      │
│             │    │    │             │
│  5V ────────┼────┴────┼─── VCC      │
│             │    └──┐ │             │
│  GND ───────┼───────┴─┼─── GND      │
└─────────────┘         └─────────────┘
              └─── 470Ω resistor (recommended)

Optional: 1000µF capacitor across VCC/GND
```

**Recommended Components:**
- 470Ω resistor on data line (reduces reflections)
- 1000µF electrolytic capacitor across power (reduces voltage spikes)

### Step 5: Build and Flash

```bash
cd C:\Users\JYOTH\Desktop\ESP_IDF\Project_SHA\Universal_IR_Remote
idf.py build
idf.py flash monitor
```

## Usage Examples

### Basic Status Updates

```c
// WiFi connecting
rgb_led_set_status(LED_STATUS_WIFI_CONNECTING);

// WiFi connected
rgb_led_set_status(LED_STATUS_WIFI_CONNECTED);

// Learning IR signal
rgb_led_set_status(LED_STATUS_LEARNING);

// Learning success (flashes 3 times, returns to idle)
rgb_led_set_status(LED_STATUS_LEARN_SUCCESS);

// Transmitting IR (flashes once, returns to idle)
rgb_led_set_status(LED_STATUS_TRANSMITTING);

// Error state (red blinking)
rgb_led_set_status(LED_STATUS_ERROR);
```

### Integration with IR Remote

```c
// In your IR learning callback
void ir_learn_callback(bool success)
{
    if (success) {
        rgb_led_set_status(LED_STATUS_LEARN_SUCCESS);
        ESP_LOGI(TAG, "IR learning succeeded");
    } else {
        rgb_led_set_status(LED_STATUS_LEARN_FAILED);
        ESP_LOGE(TAG, "IR learning failed");
    }
}

// In your IR transmit function
void ir_transmit(void)
{
    rgb_led_set_status(LED_STATUS_TRANSMITTING);
    // ... transmit IR signal ...
}

// In WiFi event handler
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

## Verification

After installation, you should see:

1. **Compile time**: No errors about missing headers or symbols
2. **Boot time**: Log message: `RGB LED initialized successfully`
3. **Runtime**: LED shows dim blue (IDLE status)

## Common Issues and Solutions

### Issue: Compilation Error - Header Not Found

**Error**: `fatal error: rgb_led.h: No such file or directory`

**Solution**: Add `rgb_led` to REQUIRES in your CMakeLists.txt

### Issue: LED Not Lighting

**Check**:
1. Power supply (WS2812B needs 5V, stable current)
2. GPIO connection (default is GPIO 22)
3. Correct wiring (DIN, VCC, GND)
4. Add 470Ω resistor on data line

**Test**:
```c
// Set a bright color to test
rgb_led_color_t test_color = {255, 255, 255};  // White
rgb_led_set_color(&test_color);
```

### Issue: RMT Channel Conflict

**Error**: `Failed to create RMT channel`

**Solution**: Change RMT channel in configuration:
```c
rgb_led_config_t config = {
    .gpio_num = 22,
    .led_count = 1,
    .rmt_channel = 1  // Try channel 1, 2, or 3
};
```

### Issue: Wrong GPIO Pin

If you need to use a different GPIO:

```c
rgb_led_config_t config = {
    .gpio_num = 25,  // Change to your GPIO
    .led_count = 1,
    .rmt_channel = 0
};
rgb_led_init(&config);
```

### Issue: Colors Are Wrong

Some WS2812B variants use RGB order instead of GRB. If colors appear wrong:

Edit `rgb_led.c`, function `set_pixel_color()`:
```c
// Change from:
pixel->r = color->r;
pixel->g = color->g;
pixel->b = color->b;

// To (swap as needed):
pixel->g = color->r;  // Swap R and G
pixel->r = color->g;
pixel->b = color->b;
```

## Component Files

Your component includes:

```
rgb_led/
├── include/
│   ├── rgb_led.h              - Main API header (USE THIS)
│   └── led_strip_encoder.h    - RMT encoder (internal)
├── rgb_led.c                  - Main implementation (403 lines)
├── led_strip_encoder.c        - WS2812B timing (148 lines)
├── example_usage.c            - Usage examples (257 lines)
├── CMakeLists.txt             - Build configuration
├── README.md                  - Full documentation
├── QUICK_REFERENCE.md         - Quick reference guide
└── INSTALL.md                 - This file
```

## Next Steps

1. See `QUICK_REFERENCE.md` for common usage patterns
2. See `example_usage.c` for complete examples
3. See `README.md` for detailed documentation

## Technical Specifications

- **Protocol**: WS2812B (NeoPixel compatible)
- **Timing**: 800kHz, precise RMT-based timing
- **Colors**: 24-bit RGB (16.7M colors)
- **Update rate**: 20Hz (50ms)
- **Memory**: ~2.2KB RAM, ~8KB Flash
- **Thread-safe**: Yes (mutex-protected)
- **Non-blocking**: Yes (FreeRTOS task-based)

## Support

For issues or questions:
1. Check `README.md` for troubleshooting
2. Review `example_usage.c` for integration examples
3. See ESP-IDF documentation for RMT peripheral

---

**Component Version**: 1.0.0
**ESP-IDF Version**: v5.0+
**Tested On**: ESP32, ESP32-S3
