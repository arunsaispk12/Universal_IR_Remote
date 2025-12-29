# Universal IR Remote - Developer Guide

**Version:** 3.3.0
**Platform:** ESP-IDF v5.5.1
**Last Updated:** December 29, 2025

This guide is for developers who want to build, modify, or contribute to the Universal IR Remote project.

---

## Table of Contents

1. [Development Environment Setup](#development-environment-setup)
2. [Project Architecture](#project-architecture)
3. [Building the Project](#building-the-project)
4. [Hardware Configuration](#hardware-configuration)
5. [Component Overview](#component-overview)
6. [Adding New Features](#adding-new-features)
7. [Testing and Debugging](#testing-and-debugging)
8. [Contributing](#contributing)

---

## Development Environment Setup

### Prerequisites

**Required Software:**
- ESP-IDF v5.5.1 (REQUIRED - other versions may not work)
- Python 3.8 or later
- Git
- CMake 3.16+
- Ninja build system

**Supported Operating Systems:**
- Windows 10/11
- Linux (Ubuntu 20.04+)
- macOS 10.15+

### Installing ESP-IDF v5.5.1

#### Windows
```bash
# Create ESP directory
mkdir %USERPROFILE%\esp
cd %USERPROFILE%\esp

# Clone ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.5.1
git submodule update --init --recursive

# Install for all supported targets
install.bat esp32,esp32s3,esp32c3,esp32s2

# Set up environment (run every time)
export.bat
```

#### Linux/macOS
```bash
# Create ESP directory
mkdir -p ~/esp
cd ~/esp

# Clone ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.5.1
git submodule update --init --recursive

# Install for all supported targets
./install.sh esp32,esp32s3,esp32c3,esp32s2

# Set up environment (run every time or add to .bashrc)
. ./export.sh
```

### Verify Installation

```bash
idf.py --version
# Expected output: ESP-IDF v5.5.1
```

---

## Project Architecture

### Directory Structure

```
Universal_IR_Remote/
├── main/                      # Main application
│   ├── app_main.c            # Application entry point
│   ├── app_wifi.c            # WiFi & provisioning
│   ├── app_config.h          # Hardware configuration
│   └── rmaker_devices.c      # RainMaker device definitions
│
├── components/               # Custom components
│   ├── ir_control/          # IR transmission & reception
│   │   ├── ir_control.c     # Core IR functionality
│   │   ├── ir_protocols.c   # Protocol encoders/decoders
│   │   ├── ir_action.c      # Logical action mapping
│   │   └── ir_ac_state.c    # AC state management
│   │
│   └── rgb_led/             # Status LED control
│       └── rgb_led.c        # WS2812B driver
│
├── docs/                    # Documentation
│   ├── user/               # User guides
│   ├── developer/          # Developer guides
│   └── git/                # Git workflow
│
├── partitions*.csv         # Partition tables (4MB/8MB/16MB)
├── sdkconfig.defaults      # Default configuration
├── CMakeLists.txt          # Build configuration
└── version.txt             # Current version
```

### Multi-Target Support

The project supports multiple ESP32 SoCs:

| Target | Flash | Status | Use Case |
|--------|-------|--------|----------|
| ESP32 | 4MB | ✅ Tested | Original, cost-effective |
| ESP32-S3 | 4/8/16MB | ✅ Tested | High-performance, more memory |
| ESP32-C3 | 4MB | ⚠️ Untested | RISC-V, compact |
| ESP32-S2 | 4MB | ⚠️ Untested | Cost-optimized |

---

## Building the Project

### Quick Build (4MB ESP32)

```bash
# Clone repository
git clone <repository-url>
cd Universal_IR_Remote

# Set target (ESP32 default)
idf.py set-target esp32

# Build
idf.py build

# Flash and monitor
idf.py flash monitor
```

### Building for ESP32-S3

```bash
# Set target
idf.py set-target esp32s3

# For 8MB flash
cp partitions_8MB.csv partitions.csv
idf.py menuconfig
# Navigate to: Serial flasher config → Flash size → 8MB

# Build and flash
idf.py build flash monitor
```

### Building for ESP32-C3/S2

```bash
# Set target
idf.py set-target esp32c3  # or esp32s2

# Build
idf.py build flash monitor
```

### Build Configurations

**Debug Build:**
```bash
idf.py menuconfig
# Component config → Log output → Default log verbosity → Debug
idf.py build
```

**Release Build (Optimized):**
```bash
idf.py menuconfig
# Compiler options → Optimization Level → Optimize for size (-Os)
idf.py build
```

---

## Hardware Configuration

### GPIO Pin Assignment

**Edit:** `main/app_config.h`

```c
/* ESP32 Configuration */
#define GPIO_IR_TX              17  // IR Transmitter
#define GPIO_IR_RX              18  // IR Receiver
#define GPIO_RGB_LED            22  // RGB LED

/* ESP32-S3 Configuration */
#define GPIO_IR_TX              17  // IR Transmitter
#define GPIO_IR_RX              18  // IR Receiver
#define GPIO_RGB_LED            11  // RGB LED (GPIO 22 reserved for flash)
```

### Partition Table Selection

See [PARTITION_TABLES.md](../../PARTITION_TABLES.md) for complete guide.

**Quick Selection:**
```bash
# 4MB flash
cp partitions_4MB.csv partitions.csv

# 8MB flash
cp partitions_8MB.csv partitions.csv

# 16MB flash
cp partitions_16MB.csv partitions.csv
```

---

## Component Overview

### 1. IR Control Component

**Location:** `components/ir_control/`

**Key Files:**
- `ir_control.c` - Core IR TX/RX, learning mode
- `ir_protocols.c` - Protocol encode/decode (NEC, Samsung, etc.)
- `ir_action.c` - Action-to-IR-code mapping
- `ir_ac_state.c` - AC state-based control

**Adding a New IR Protocol:**

1. Add protocol enum to `ir_protocols.h`:
```c
typedef enum {
    // ... existing protocols
    IR_PROTOCOL_MYNEW,
} ir_protocol_t;
```

2. Add encoder in `ir_protocols.c`:
```c
esp_err_t ir_encode_mynew(const ir_code_t *code, rmt_symbol_word_t *symbols, size_t *length)
{
    // Implementation
}
```

3. Register in protocol table:
```c
static const ir_protocol_entry_t protocol_table[] = {
    // ... existing
    {IR_PROTOCOL_MYNEW, "MYNEW", ir_encode_mynew, ir_decode_mynew},
};
```

### 2. RGB LED Component

**Location:** `components/rgb_led/`

**Features:**
- WS2812B driver using RMT
- Status indication (connecting, learning, transmitting)
- Color patterns and effects

**Changing LED Behavior:**

Edit `components/rgb_led/rgb_led.c`:
```c
// Change color for "learning" mode
void rgb_led_learning(void) {
    rgb_led_set_color(255, 255, 0); // Yellow
}
```

### 3. Main Application

**Location:** `main/`

**Key Files:**
- `app_main.c` - Application initialization, RainMaker setup
- `app_wifi.c` - WiFi provisioning, network management
- `rmaker_devices.c` - RainMaker device type definitions

---

## Adding New Features

### Example: Adding a New Device Type

**1. Add device type enum (if needed):**

`components/ir_control/include/ir_action.h`:
```c
typedef enum {
    IR_DEVICE_TV,
    IR_DEVICE_AC,
    // Add new:
    IR_DEVICE_PROJECTOR,
} ir_device_type_t;
```

**2. Add actions for new device:**

```c
typedef enum {
    // Existing actions...

    // Projector-specific
    IR_ACTION_PROJECTOR_LENS_ZOOM_IN,
    IR_ACTION_PROJECTOR_LENS_ZOOM_OUT,
    IR_ACTION_PROJECTOR_FOCUS,
} ir_action_t;
```

**3. Create RainMaker device:**

`main/rmaker_devices.c`:
```c
esp_rmaker_device_t *esp_rmaker_projector_device_create(
    const char *dev_name, void *priv_data, bool primary)
{
    esp_rmaker_device_t *device = esp_rmaker_device_create(
        dev_name, ESP_RMAKER_DEVICE_OTHER, priv_data);

    esp_rmaker_device_add_param(device,
        esp_rmaker_power_param_create("Power", false));
    esp_rmaker_device_add_param(device,
        esp_rmaker_param_create("Zoom", NULL,
            esp_rmaker_int(0), PROP_FLAG_READ | PROP_FLAG_WRITE));

    return device;
}
```

**4. Register callbacks in app_main.c**

---

## Testing and Debugging

### Serial Monitor

```bash
idf.py monitor

# Filter logs by component
idf.py monitor | grep "ir_control"

# Set log level
idf.py menuconfig
# Component config → Log output → Default log verbosity
```

### Common Debug Commands

```c
// Enable verbose logging for IR component
esp_log_level_set("ir_control", ESP_LOG_DEBUG);

// Dump partition table
esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
while (it != NULL) {
    const esp_partition_t *part = esp_partition_get(it);
    ESP_LOGI(TAG, "Partition: %s, Type: %d, Offset: 0x%x, Size: %d",
             part->label, part->type, part->address, part->size);
    it = esp_partition_next(it);
}
```

### GDB Debugging

```bash
# Build with debug symbols
idf.py build

# Start GDB
idf.py gdb

# Common GDB commands
(gdb) break app_main
(gdb) continue
(gdb) backtrace
(gdb) print variable_name
```

### Memory Analysis

```bash
# Check heap usage
idf.py size-components

# Monitor heap at runtime
esp_get_free_heap_size()
esp_get_minimum_free_heap_size()
```

---

## Contributing

### Code Style

**Follow ESP-IDF coding standards:**
- Use `snake_case` for functions and variables
- Use `UPPER_CASE` for macros and constants
- Add Doxygen comments for public APIs
- Keep lines under 120 characters

**Example:**
```c
/**
 * @brief Transmit an IR code
 *
 * @param[in] code Pointer to IR code structure
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if code is NULL
 *     - ESP_FAIL on transmission error
 */
esp_err_t ir_transmit(const ir_code_t *code);
```

### Git Workflow

See [Git Usage Guide](../git/GIT_USAGE.md) for detailed workflow.

**Quick Reference:**
```bash
# Create feature branch
git checkout -b feature/new-protocol

# Make changes and commit
git add .
git commit -m "feat: add XYZ protocol support"

# Push and create PR
git push origin feature/new-protocol
```

### Testing Requirements

Before submitting PR:
1. ✅ Code compiles without warnings
2. ✅ Tested on actual hardware
3. ✅ Documentation updated
4. ✅ CHANGELOG.md updated
5. ✅ No memory leaks (check with heap monitoring)

---

## API Reference

### IR Control API

**Initialize IR system:**
```c
esp_err_t ir_init(gpio_num_t tx_gpio, gpio_num_t rx_gpio);
```

**Transmit IR code:**
```c
esp_err_t ir_transmit(const ir_code_t *code);
```

**Learn IR code:**
```c
esp_err_t ir_learn_start(ir_button_t button, uint32_t timeout_ms);
```

**Action-based control:**
```c
esp_err_t ir_action_execute(ir_device_type_t device, ir_action_t action);
esp_err_t ir_action_learn(ir_device_type_t device, ir_action_t action, uint32_t timeout_ms);
```

**AC control:**
```c
esp_err_t ir_ac_set_state(const ir_ac_state_t *state);
esp_err_t ir_ac_transmit(void);
```

### RGB LED API

**Initialize LED:**
```c
esp_err_t rgb_led_init(const rgb_led_config_t *config);
```

**Set color:**
```c
esp_err_t rgb_led_set_color(uint8_t red, uint8_t green, uint8_t blue);
```

**Status patterns:**
```c
void rgb_led_connecting(void);
void rgb_led_connected(void);
void rgb_led_learning(void);
void rgb_led_transmitting(void);
void rgb_led_error(void);
```

---

## Performance Tuning

### Binary Size Optimization

```bash
# Enable size optimization
idf.py menuconfig
# Compiler options → Optimization Level → Optimize for size (-Os)

# Disable unused features
# Component config → Bluetooth → Disable if not using
```

### Memory Optimization

**Reduce stack sizes:**
```c
// app_config.h
#define MAIN_TASK_STACK_SIZE    4096  // Reduce if possible
#define IR_TASK_STACK_SIZE      6144  // Reduce if possible
```

**Use IRAM wisely:**
```c
// Place time-critical functions in IRAM
IRAM_ATTR void time_critical_function(void) {
    // ...
}
```

---

## Troubleshooting Build Issues

### Common Errors

**Error: Component 'esp_rainmaker' not found**
```bash
# Solution: Install component manager dependencies
idf.py reconfigure
```

**Error: Python dependencies missing**
```bash
# Solution: Reinstall ESP-IDF Python packages
cd $IDF_PATH
./install.sh esp32,esp32s3,esp32c3,esp32s2
```

**Error: Partition table doesn't fit**
```bash
# Solution: Use appropriate partition table for your flash size
cp partitions_8MB.csv partitions.csv  # For 8MB flash
```

---

## Resources

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/)
- [ESP RainMaker Documentation](https://rainmaker.espressif.com/)
- [Project GitHub](https://github.com/your-repo)
- [IR Protocol Database](http://www.hifi-remote.com/wiki/)

---

**Happy Coding!** 🚀
