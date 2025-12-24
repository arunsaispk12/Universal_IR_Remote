# Universal IR Remote - ESP32 RainMaker

**Version:** 1.0.0
**Platform:** ESP32 | ESP-IDF v5.5.1 | ESP RainMaker
**Release Date:** December 24, 2024

> **Complete Universal IR Remote Control** - Learn and control any IR device from your smartphone!

---

## 🎯 Overview

A standalone ESP32-based universal IR remote control that learns IR codes from any remote control (TV, AC, Set-top box, etc.) and lets you control them via the **ESP RainMaker smartphone app** from anywhere in the world.

### Key Features

✅ **32 Programmable Buttons** - Learn codes from any IR remote
✅ **Multi-Protocol Support** - NEC, Samsung, and RAW (unknown) protocols
✅ **Cloud Control** - Control from anywhere via ESP RainMaker app
✅ **Visual Feedback** - RGB LED shows learning/transmit status
✅ **BLE Provisioning** - Easy WiFi setup via smartphone
✅ **Persistent Storage** - Learned codes saved across reboots
✅ **OTA Updates** - Update firmware wirelessly

---

## 📱 What You Can Do

### Learn IR Codes
1. Open ESP RainMaker app
2. Select any of 32 buttons (Power, Volume, Channel, etc.)
3. Press "Learn" toggle
4. Point your original remote at the device
5. Press the button you want to copy
6. **Done!** Code is learned and saved

### Control Devices
1. Press "Transmit" for any learned button
2. IR code is sent to your device
3. Works from anywhere via internet!

### Supported Devices
- 📺 **TVs** - All brands (Samsung, LG, Sony, etc.)
- 🌡️ **Air Conditioners** - Most brands
- 📡 **Set-top Boxes** - Cable/Satellite boxes
- 🔊 **Audio Systems** - Receivers, soundbars
- 💿 **DVD/Blu-ray Players**
- 🎮 **Game Consoles** (IR-enabled)
- And any other IR-controlled device!

---

## 🛠️ Hardware Requirements

### Core Components
- **ESP32 Development Board** (any variant with GPIO 17, 18, 22)
- **IR Receiver Module** - IRM-3638T, VS1838B, or compatible (38kHz)
- **IR LED Transmitter** - TSAL6200 or 940nm IR LED
- **2N2222 NPN Transistor** (for IR LED driver)
- **WS2812B RGB LED** (status indicator)
- **Resistors:**
  - 1× 10kΩ (transistor base)
  - 1× 330Ω (IR LED current limiting)
  - 1× 470Ω (WS2812B data line)
- **Power Supply:** 5V/1A USB power adapter

### Optional Components
- 100nF capacitor (WS2812B stability)
- 1000µF capacitor (power supply smoothing)

---

## 🔌 Wiring Diagram

```
ESP32 Connections:
==================

IR Receiver (IRM-3638T):
  ESP32 GPIO18 ─────────> IR Receiver OUT
  ESP32 5V ─────────────> IR Receiver VCC
  ESP32 GND ────────────> IR Receiver GND

IR Transmitter (940nm LED):
  ESP32 GPIO17 ──[10kΩ]──┬──> 2N2222 Base
                          │
                     2N2222 NPN
                      (E) │ (C)
                      GND └──[330Ω]─── IR LED (-)
                                        IR LED (+) ─── 5V

RGB Status LED (WS2812B):
  ESP32 GPIO22 ──[470Ω]──> WS2812B DIN
  ESP32 5V ─────────────> WS2812B VCC
  ESP32 GND ────────────> WS2812B GND

Boot Button:
  ESP32 GPIO0 ──┬──> Button
                └──> GND
```

---

## 🚀 Quick Start

### 1. Hardware Setup
- Connect components according to wiring diagram above
- Use ESP32 development board with USB connection

### 2. Build and Flash

```bash
# Clone or navigate to project
cd C:\Users\JYOTH\Desktop\ESP_IDF\Project_SHA\Universal_IR_Remote

# Set up ESP-IDF environment (first time only)
# Windows:
%USERPROFILE%\esp\esp-idf\export.bat
# Linux/macOS:
. $HOME/esp/esp-idf/export.sh

# Build
idf.py build

# Flash
idf.py -p COM3 flash monitor
```

### 3. Provision WiFi

**Method 1: QR Code (Easiest)**
1. Watch serial monitor for QR code
2. Open ESP RainMaker app
3. Tap "Add Device"
4. Scan QR code
5. Follow app instructions

**Method 2: Manual BLE**
1. Open ESP RainMaker app
2. Tap "Add Device"
3. Select "I don't have a QR code"
4. Connect to BLE device "PROV_xxxxxx"
5. Enter PoP: `abcd1234`
6. Select WiFi network and password

### 4. Start Learning!
1. In RainMaker app, find "IR Remote" device
2. Select a button (e.g., "Power")
3. Toggle "Learn" to ON
4. **LED turns PURPLE (pulsing)** - learning mode active
5. Point original remote at IR receiver
6. Press button on original remote
7. **LED flashes GREEN 3×** - success! Code learned
   - OR **LED flashes RED 3×** - failed, try again
8. Toggle "Learn" back to OFF

### 5. Transmit Learned Code
1. Select same button (e.g., "Power")
2. Toggle "Transmit" to ON
3. **LED flashes CYAN** - code transmitted!
4. Your device should respond

---

## 📋 32 Button Layout

### Power & Navigation (6 buttons)
- **Power** - Main power toggle
- **Source** - Input source selection
- **Menu** - Menu/Settings
- **Home** - Home button
- **Back** - Back/Return
- **OK** - Enter/Select

### Volume Control (3 buttons)
- **Vol+** - Volume up
- **Vol-** - Volume down
- **Mute** - Mute toggle

### Channel Control (2 buttons)
- **Ch+** - Channel up
- **Ch-** - Channel down

### Number Pad (11 buttons)
- **0** through **9**

### D-Pad Navigation (4 buttons)
- **Up** - Navigate up
- **Down** - Navigate down
- **Left** - Navigate left
- **Right** - Navigate right

### Custom Buttons (6 buttons)
- **Custom1** through **Custom6** - Programmable for any function

**Total: 32 buttons**

---

## 🎨 LED Status Indicators

| LED Color | Pattern | Meaning |
|-----------|---------|---------|
| 🔵 Dim Blue | Solid | System idle, ready |
| 🟣 Purple | Pulsing (2s) | IR learning mode active |
| 🟢 Green | Flash 3× | IR learning success |
| 🔴 Red | Flash 3× | IR learning failed |
| 🔵 Cyan | Flash 1× | IR transmitting |
| 🟡 Yellow | Pulsing (2s) | WiFi connecting |
| 🟢 Green | Solid | WiFi connected |
| 🔴 Red | Blinking (1s) | Error state |
| ⚫ Off | Off | LED disabled |

---

## 🔧 Advanced Features

### Boot Button Functions

**Short Press (< 3 seconds):**
- Normal boot operation

**3-Second Press:**
- **WiFi Reset** - Clears WiFi credentials
- Device enters provisioning mode
- LED turns yellow (WiFi connecting)

**10-Second Press:**
- **Factory Reset** - Clears ALL data
  - WiFi credentials
  - All learned IR codes
  - RainMaker configuration
- Device reboots and enters provisioning mode

### Console Commands (UART Monitor)

Connect via serial monitor (`idf.py monitor`) and use:

```bash
# Learn IR code for button
learn <button_id>
# Example: learn 0    (learns Power button)

# Transmit learned code
transmit <button_id>
# Example: transmit 0 (transmits Power)

# List all learned buttons
list

# Clear specific button
clear <button_id>
# Example: clear 5    (clears Mute button)

# Clear all buttons
clear all
```

**Button IDs:**
- 0: Power, 1: Source, 2: Menu, 3: Home, 4: Back, 5: OK
- 6: Vol+, 7: Vol-, 8: Mute
- 9: Ch+, 10: Ch-
- 11-21: Numbers 0-9, Source
- 22-25: Up, Down, Left, Right
- 26-31: Custom1-6

---

## 📊 Technical Specifications

### IR Protocols Supported
- **NEC** - Most common (9ms + 4.5ms leader)
- **Samsung** - Samsung variant (4.5ms + 4.5ms leader)
- **RAW** - Fallback for unknown protocols (exact timing capture)

### IR Specifications
- **Carrier Frequency:** 38kHz
- **Receiver:** Active-LOW with automatic inversion
- **Transmitter:** 940nm, 38kHz modulated
- **Range:** Up to 10 meters (depending on LED power)
- **Learning Timeout:** 30 seconds
- **Storage:** Up to 32 codes in NVS flash

### WiFi & Cloud
- **WiFi:** 2.4GHz 802.11 b/g/n
- **Provisioning:** BLE-based (secure)
- **Cloud:** ESP RainMaker (MQTT-based)
- **Control:** Local and remote (via internet)
- **OTA:** Wireless firmware updates

### Performance
- **Firmware Size:** ~1.0MB
- **RAM Usage:** ~180KB
- **Flash Usage:** 4MB (with OTA partitions)
- **Boot Time:** ~5 seconds
- **Learning Time:** < 1 second per code
- **Transmission Time:** < 200ms

---

## 🐛 Troubleshooting

### IR Learning Fails

**Problem:** LED flashes red, code not learned

**Solutions:**
1. **Check battery** - Replace remote batteries (most common!)
2. **Move closer** - Keep remote 6-12 inches from receiver
3. **Point directly** - Aim remote straight at IR receiver
4. **Avoid sunlight** - Move away from bright lights/windows
5. **Hold button longer** - Press and hold for 2-3 seconds
6. **Try multiple times** - Success rate improves with practice

### IR Transmission Doesn't Work

**Problem:** Code learned but device doesn't respond

**Solutions:**
1. **Re-learn code** - Try learning 2-3 times
2. **Check line-of-sight** - Point transmitter at device
3. **Check distance** - Move closer (< 5 meters)
4. **Verify LED** - Use phone camera to see IR LED glow
5. **Check wiring** - Verify transistor circuit connections

### WiFi Connection Issues

**Problem:** Can't connect to WiFi, yellow LED pulsing

**Solutions:**
1. **Check WiFi band** - Use 2.4GHz only (not 5GHz)
2. **Verify password** - Re-enter WiFi credentials
3. **Move closer to router** - Improve signal strength
4. **Reset WiFi** - Hold boot button 3 seconds
5. **Factory reset** - Hold boot button 10 seconds

### Device Not in RainMaker App

**Problem:** Device doesn't appear after provisioning

**Solutions:**
1. **Wait 30 seconds** - Allow time for cloud connection
2. **Check WiFi** - Ensure device is connected (green LED)
3. **Check internet** - Router must have internet access
4. **Restart app** - Close and reopen RainMaker app
5. **Re-provision** - Factory reset and provision again

---

## 📚 RainMaker App Guide

### Device Parameters (per button)

Each of 32 buttons has 3 parameters:

1. **Learn (toggle)**
   - Turn ON: Start learning mode (LED purple)
   - Turn OFF: Stop learning mode
   - After learning, automatically returns to OFF

2. **Transmit (toggle)**
   - Turn ON: Transmit IR code (LED cyan flash)
   - Automatically returns to OFF after transmission
   - Only works if button has learned code

3. **Learned (indicator)**
   - Shows ✓ if button has learned code
   - Shows ✗ if button is empty
   - Read-only (cannot be changed manually)

### Typical Workflow

**Learning a new button:**
```
1. Select button → Tap "Learn" → ON
2. LED turns purple (pulsing)
3. Point original remote, press button
4. LED flashes green 3× (success)
5. "Learned" indicator shows ✓
6. "Learn" automatically returns to OFF
```

**Using a learned button:**
```
1. Select button with ✓ indicator
2. Tap "Transmit" → ON
3. LED flashes cyan (transmitting)
4. Device responds to command
5. "Transmit" automatically returns to OFF
```

---

## 🔐 Security

### Provisioning Security
- **BLE pairing** with Proof of Possession (PoP)
- Default PoP: `abcd1234` (change in code if needed)
- Encrypted credential storage in NVS

### WiFi Security
- Supports WPA2-PSK and WPA3
- Credentials encrypted in flash
- Factory reset clears all credentials

### Cloud Security
- TLS/SSL encrypted communication
- Device certificate-based authentication
- Secure MQTT connection to RainMaker

---

## 📄 File Structure

```
Universal_IR_Remote/
├── main/
│   ├── app_main.c              # Main application
│   ├── app_wifi.c/h            # WiFi & provisioning
│   ├── app_config.h            # Configuration
│   └── CMakeLists.txt          # Main build config
│
├── components/
│   ├── ir_control/             # IR learning & transmission
│   │   ├── include/ir_control.h
│   │   ├── ir_control.c
│   │   └── CMakeLists.txt
│   └── rgb_led/                # RGB LED status
│       ├── include/rgb_led.h
│       ├── rgb_led.c
│       ├── led_strip_encoder.c
│       └── CMakeLists.txt
│
├── docs/                       # Documentation
├── CMakeLists.txt              # Project build config
├── sdkconfig.defaults          # ESP-IDF configuration
├── partitions.csv              # Flash partition table
├── version.txt                 # Version (1.0.0)
└── README.md                   # This file
```

---

## 🔄 OTA Updates

### Via RainMaker App
1. Open ESP RainMaker app
2. Go to device settings
3. Tap "OTA Update"
4. Upload new firmware binary
5. Device updates and reboots

### Via Command Line
```bash
# Build new firmware
idf.py build

# Upload to RainMaker (requires rmaker CLI)
esp-rainmaker-cli ota push build/universal_ir_remote.bin
```

---

## 🤝 Contributing

### Reporting Issues
- Check troubleshooting section first
- Provide serial monitor logs
- Describe hardware setup
- Include steps to reproduce

### Feature Requests
- Describe use case
- Explain expected behavior
- Consider compatibility impact

---

## 📝 License

**MIT License**

Copyright (c) 2024 Sai Automations

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.

---

## 🙏 Credits

- **ESP-IDF**: Espressif IoT Development Framework
- **ESP RainMaker**: Espressif cloud platform
- **Community**: ESP32 community for support and contributions

---

## 📞 Support

**Project Location:**
```
C:\Users\JYOTH\Desktop\ESP_IDF\Project_SHA\Universal_IR_Remote
```

**Documentation:**
- See `docs/` folder for detailed guides
- Check component README files for API reference

**Getting Help:**
1. Read troubleshooting section
2. Check serial monitor logs
3. Review wiring connections
4. Test with console commands

---

**Made with ❤️ for ESP32 IoT | v1.0.0 Universal IR Remote**
