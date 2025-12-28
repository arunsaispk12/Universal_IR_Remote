# Build Instructions - Universal IR Remote v3.3.0

**Complete guide to building and flashing the firmware**

---

## 📋 Prerequisites

### Required Software
- **ESP-IDF v5.5.1** (REQUIRED)
- **Python 3.8+**
- **Git**
- **USB-to-UART driver**

### Required Hardware
- ESP32, ESP32-S3, ESP32-C3, or ESP32-S2 development board
- USB cable (data-capable)
- IR receiver module (IRM-3638T or VS1838B)
- IR LED transmitter (940nm)
- WS2812B RGB LED
- Supporting components (transistor, resistors)

---

## 🛠️ Step-by-Step Build Instructions

### Step 1: Install ESP-IDF v5.5.1

#### Windows
```bash
# Download and install ESP-IDF
mkdir %USERPROFILE%\esp
cd %USERPROFILE%\esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.5.1
git submodule update --init --recursive
install.bat esp32
```

#### Linux/macOS
```bash
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.5.1
git submodule update --init --recursive
./install.sh esp32,esp32s3,esp32c3,esp32s2
```

### Step 2: Set Up Environment

#### Windows (Command Prompt)
```bash
cd %USERPROFILE%\esp\esp-idf
export.bat
```

#### Windows (PowerShell)
```powershell
cd $env:USERPROFILE\esp\esp-idf
.\export.ps1
```

#### Linux/macOS
```bash
cd ~/esp/esp-idf
. ./export.sh
```

**Verify installation:**
```bash
idf.py --version
# Expected: ESP-IDF v5.5.1
```

### Step 3: Navigate to Project

```bash
cd C:\Users\JYOTH\Desktop\ESP_IDF\Project_SHA\Universal_IR_Remote
```

### Step 4: Configure (Optional)

```bash
idf.py menuconfig
```

**Recommended settings (already configured in sdkconfig.defaults):**
- Flash size: 4MB
- Partition table: Custom (partitions.csv)
- WiFi: 2.4GHz enabled
- Bluetooth: BLE enabled
- ESP RainMaker: Self-claim enabled

### Step 5: Build

```bash
idf.py build
```

**Expected output:**
```
Project build complete.
To flash, run: idf.py -p (PORT) flash
```

**Build artifacts:**
- `build/universal_ir_remote.bin` - Main firmware
- `build/bootloader/bootloader.bin` - Bootloader
- `build/partition_table/partition-table.bin` - Partition table

---

## 📡 Flashing Instructions

### Method 1: Full Flash (First Time)

```bash
# Connect ESP32 via USB
# Find COM port: Windows (COM3), Linux (/dev/ttyUSB0), macOS (/dev/cu.usbserial-xxx)

# Flash everything
idf.py -p COM3 flash monitor

# -p COM3     : Specify port (change to your port)
# flash       : Flash firmware
# monitor     : Open serial monitor
```

### Method 2: App Only (Updates)

```bash
# Flash only application (faster)
idf.py -p COM3 app-flash monitor
```

### Method 3: Manual esptool

```bash
esptool.py --chip esp32 --port COM3 --baud 460800 \
  --before default_reset --after hard_reset write_flash \
  0x1000 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/universal_ir_remote.bin
```

---

## 🔍 Verification

### Check Boot Messages

After flashing, monitor serial output:

**Expected boot sequence:**
```
I (xxx) UIR_MAIN: Starting Universal IR Remote v1.0.0
I (xxx) IR_CTRL: IR control initialized
I (xxx) RGB_LED: RGB LED initialized
I (xxx) WIFI: Starting WiFi provisioning
I (xxx) BLE_PROV: BLE provisioning started: PROV_xxxxxx
I (xxx) UIR_MAIN: Scan QR code:
  [ASCII QR Code displayed here]
I (xxx) RGB_LED: Status: WIFI_CONNECTING (Yellow pulsing)
```

### LED Status Check

| Stage | LED Color | Meaning |
|-------|-----------|---------|
| Boot | Dim Blue | System initializing |
| Provisioning | Yellow Pulsing | Waiting for WiFi setup |
| Connected | Green Solid | WiFi connected, ready |

### Test IR Learning

```bash
# In serial monitor, type:
learn 0

# Expected output:
I (xxx) UIR_MAIN: Starting IR learning for button 0 (Power)
I (xxx) RGB_LED: Status: LEARNING (Purple pulsing)

# Point remote and press button
I (xxx) IR_CTRL: IR code learned: Protocol=NEC, Data=0x12345678
I (xxx) RGB_LED: Status: LEARN_SUCCESS (Green flash 3x)
I (xxx) UIR_MAIN: Code saved successfully
```

---

## 🐛 Troubleshooting Build Issues

### Issue 1: "idf.py: command not found"

**Solution:**
```bash
# Environment not set up
# Windows:
cd %USERPROFILE%\esp\esp-idf
export.bat

# Linux/macOS:
cd ~/esp/esp-idf
. ./export.sh
```

### Issue 2: "No module named 'esp_idf_size'"

**Solution:**
```bash
# Python dependencies missing
cd ~/esp/esp-idf
./install.sh esp32  # or install.bat on Windows
```

### Issue 3: "fatal error: esp_rainmaker.h: No such file or directory"

**Solution:**
```bash
# ESP RainMaker component missing
idf.py add-dependency espressif/esp_rainmaker^1.2.0
idf.py reconfigure
idf.py build
```

### Issue 4: Build fails with "region overflowed"

**Solution:**
```bash
# Reduce code size
idf.py menuconfig
# → Compiler options → Optimization → Optimize for size (-Os)
idf.py fullclean
idf.py build
```

### Issue 5: Flash fails "Failed to connect"

**Solutions:**
1. **Check USB cable** - Use data cable, not charge-only
2. **Hold BOOT button** - While connecting power
3. **Try different port** - USB 2.0 vs USB 3.0
4. **Update drivers** - Install latest USB-to-UART driver
5. **Reduce baud rate:**
   ```bash
   idf.py -p COM3 -b 115200 flash
   ```

### Issue 6: "Brownout detector was triggered"

**Solution:**
```
1. Use better power supply (5V/1A minimum)
2. Add bulk capacitor (1000µF) on power rail
3. Reduce peak current (disconnect LEDs temporarily)
```

---

## 📊 Build Configuration

### Flash Partition Layout

```
Address   Size     Name              Type
0x9000    24KB     nvs               Data (NVS)
0xF000    8KB      otadata           Data (OTA)
0x11000   4KB      phy_init          Data
0x12000   24KB     fctry             Data (Factory)
0x20000   1536KB   ota_0             App (OTA partition 1)
0x1A0000  1536KB   ota_1             App (OTA partition 2)
0x320000  4KB      nvs_key           Data (NVS keys)
0x321000  28KB     rmaker_storage    Data (RainMaker)
0x328000  64KB     ir_storage        Data (IR codes)
```

**Total flash:** 4MB

### Component Dependencies

```
main → ir_control
    → rgb_led
    → esp_rainmaker
    → esp_wifi
    → nvs_flash
    → bt (Bluetooth)
    → driver
    → esp_timer
    → app_update (OTA)

ir_control → driver (RMT)
          → esp_timer
          → nvs_flash

rgb_led → driver (RMT)
       → esp_timer
```

---

## 🚀 Quick Build Commands

```bash
# Full clean build
idf.py fullclean
idf.py build

# Build specific component
idf.py build component-ir_control-build

# Flash and monitor
idf.py -p COM3 flash monitor

# Monitor only (after flash)
idf.py -p COM3 monitor

# Exit monitor: Ctrl+]

# Erase flash (factory reset)
idf.py -p COM3 erase-flash

# Check binary sizes
idf.py size
idf.py size-components

# Generate compile_commands.json (for IDEs)
idf.py reconfigure
```

---

## 📈 Build Optimization

### Reduce Firmware Size

```bash
idf.py menuconfig

# Navigate to:
# → Component config → Log output → Default log verbosity → Error
# → Compiler options → Optimization Level → Optimize for size (-Os)
# → Component config → Bluetooth → Disable Bluetooth (if not using BLE)

idf.py build
```

### Increase Build Speed

```bash
# Enable ccache (if installed)
export IDF_CCACHE_ENABLE=1

# Parallel build (adjust -j based on CPU cores)
idf.py build -j8
```

### Debug Build (for development)

```bash
idf.py menuconfig

# → Component config → Log output → Default log verbosity → Debug
# → Compiler options → Optimization Level → Debug (-Og)

idf.py build
```

---

## 🎯 Post-Flash Setup

### 1. Serial Monitor

```bash
idf.py -p COM3 monitor

# Expected output:
# - Boot messages
# - QR code for provisioning
# - WiFi status
# - RainMaker connection status
```

### 2. QR Code Provisioning

1. **Find QR code** in serial output
2. **Open ESP RainMaker app** (iOS/Android)
3. **Tap "Add Device"**
4. **Scan QR code**
5. **Follow app instructions**
   - Select WiFi network
   - Enter WiFi password
   - Wait for connection

### 3. Verify Device in App

1. **Check device appears** as "IR Remote"
2. **Verify 32 buttons** are listed
3. **Test learning:**
   - Select "Power" button
   - Toggle "Learn" ON
   - Point remote, press button
   - LED should flash green
   - "Learned" indicator shows ✓

### 4. Test Transmission

1. **Select learned button**
2. **Toggle "Transmit" ON**
3. **LED flashes cyan**
4. **Device responds**

---

## 📝 Build Artifacts

### Binary Sizes (Typical)

```
Bootloader:          ~28KB
Partition Table:     ~3KB
Application:         ~950KB
Total Used:          ~981KB

Flash Layout:
- OTA_0 (1536KB)     : Active firmware
- OTA_1 (1536KB)     : Backup/update firmware
- Data partitions    : ~156KB (NVS, RainMaker, IR storage)
```

### RAM Usage (Runtime)

```
Static RAM:          ~100KB
Heap available:      ~200KB
Task stacks:
  - Main task:       6KB
  - IR task:         8KB
  - LED task:        2KB
  - WiFi/BT:         ~40KB
  - RainMaker:       ~30KB

Total RAM used:      ~186KB / ~320KB available
```

---

## 🔗 Additional Resources

- **ESP-IDF Documentation:** https://docs.espressif.com/projects/esp-idf/en/v5.5.1/
- **ESP RainMaker:** https://rainmaker.espressif.com/docs/
- **ESP32 Datasheet:** https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
- **Project Documentation:** See `docs/` folder

---

## ✅ Verify Successful Build

After `idf.py build`, check:

```bash
# Windows
dir build\universal_ir_remote.bin
dir build\bootloader\bootloader.bin
dir build\partition_table\partition-table.bin

# Linux/macOS
ls -lh build/universal_ir_remote.bin
ls -lh build/bootloader/bootloader.bin
ls -lh build/partition_table/partition-table.bin
```

**All files should exist with reasonable sizes:**
- `universal_ir_remote.bin` - ~900KB-1.0MB
- `bootloader.bin` - ~27-30KB
- `partition-table.bin` - ~3KB

---

**Build Guide Version:** 1.0.0
**Last Updated:** December 24, 2024

**Ready to build? Run `idf.py build` and get started! 🚀**
