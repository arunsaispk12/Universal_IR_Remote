# Universal IR Remote - User Guide

**Version:** 3.3.0
**Last Updated:** December 29, 2025

Welcome to the Universal IR Remote! This guide will help you set up and use your ESP32-based universal remote control.

---

## Table of Contents

1. [What is Universal IR Remote?](#what-is-universal-ir-remote)
2. [Getting Started](#getting-started)
3. [Initial Setup](#initial-setup)
4. [Learning IR Codes](#learning-ir-codes)
5. [Controlling Devices](#controlling-devices)
6. [Troubleshooting](#troubleshooting)
7. [FAQ](#faq)

---

## What is Universal IR Remote?

The Universal IR Remote is a smart, cloud-connected infrared remote control that can:

- ✅ **Replace all your remotes** - Control TV, AC, Set-Top Box, Soundbar, and Fan
- ✅ **Control from anywhere** - Use your smartphone via ESP RainMaker app
- ✅ **Learn any IR code** - Compatible with virtually any IR device
- ✅ **Smart AC control** - Set temperature, mode, fan speed all at once
- ✅ **Voice control ready** - Works with Alexa/Google Home via RainMaker

---

## Getting Started

### What You Need

1. **Hardware:**
   - Universal IR Remote device (ESP32/ESP32-S3 based)
   - Power supply (USB 5V)
   - Your original remote controls (for learning)

2. **Software:**
   - ESP RainMaker smartphone app (iOS/Android)
   - WiFi connection (2.4GHz)

### Download ESP RainMaker App

- **iOS:** [App Store Link](https://apps.apple.com/app/esp-rainmaker/id1497491540)
- **Android:** [Play Store Link](https://play.google.com/store/apps/details?id=com.espressif.rainmaker)

---

## Initial Setup

### Step 1: Power On

1. Connect your Universal IR Remote to USB power
2. Wait for RGB LED to turn **BLUE** (ready for provisioning)

### Step 2: Add Device to RainMaker

1. Open **ESP RainMaker** app
2. Tap **"Add Device"**
3. Scan the QR code (on device or included card)
4. Follow on-screen instructions to:
   - Connect to device's Bluetooth
   - Enter your WiFi credentials
   - Wait for device to connect to cloud

### Step 3: Verify Connection

- RGB LED should turn **GREEN** = Connected to WiFi and cloud
- RGB LED **BLINKING GREEN** = Trying to connect
- RGB LED **RED** = Error (see troubleshooting)

---

## Learning IR Codes

The Universal IR Remote needs to "learn" the IR signals from your original remotes before it can control devices.

### For TV, Set-Top Box, Speaker, Fan

1. **Open RainMaker App** → Select your device
2. **Choose device type** (e.g., "TV")
3. **Tap "Learn_Mode"** parameter
4. **Select action** to learn (e.g., "Power", "Volume Up")
5. **Point your original remote** at the IR receiver
6. **Press the button** on your original remote
7. **Wait for confirmation** - LED will flash to confirm
8. **Repeat** for other buttons

**Example - Learning TV Power:**
```
1. Open "TV" device in app
2. Tap "Learn_Mode"
3. Select "Power"
4. Point TV remote at device
5. Press Power button on TV remote
6. Done! Power command learned
```

### For Air Conditioner (Special)

Air conditioners are smarter - you don't need to learn every button!

**Option 1: Auto-Detect Protocol (Recommended)**
1. Open "AC" device in app
2. Tap "Learn_Protocol"
3. Select "Auto-Detect"
4. Set your AC to a known state (e.g., ON, Cool, 24°C)
5. Point AC remote at device
6. Press **any** button on AC remote
7. System auto-detects brand and protocol
8. Now you can control AC from app!

**Option 2: Manual Protocol Selection**
1. Open "AC" device in app
2. Tap "Learn_Protocol"
3. Select your AC brand:
   - Daikin
   - Voltas/Carrier
   - Hitachi
   - Blue Star
   - LG
   - Samsung
   - (and more...)
4. Done! AC ready to control

---

## Controlling Devices

### From RainMaker App

**TV/STB/Speaker/Fan:**
- Simply tap any parameter (Power, Volume, Channel, etc.)
- Device transmits learned IR code immediately
- Works from anywhere with internet!

**Air Conditioner:**
1. Adjust any parameters:
   - Power: ON/OFF
   - Mode: Cool/Heat/Auto/Dry/Fan
   - Temperature: 16-30°C
   - Fan Speed: Auto/Low/Med/High
   - Swing: On/Off
2. Tap "Send Command"
3. AC receives complete state in one transmission

### From Voice Assistants (Alexa/Google Home)

1. **Link RainMaker account** in Alexa/Google Home app
2. **Discover devices**
3. **Use voice commands:**
   - "Alexa, turn on the TV"
   - "Hey Google, set AC to 24 degrees"
   - "Alexa, increase volume"

### Local Control (Optional)

Press the **Boot Button** on the device to:
- **Short press:** Test IR transmission (last used code)
- **Long press (3s):** WiFi reset
- **Very long press (10s):** Factory reset

---

## Troubleshooting

### Device Won't Connect to WiFi

**Problem:** LED stays blinking blue or turns red

**Solutions:**
1. Ensure WiFi is 2.4GHz (not 5GHz)
2. Check WiFi password is correct
3. Restart device (unplug and plug back)
4. Reset WiFi: Hold boot button for 3 seconds
5. Try provisioning again

### IR Code Not Working

**Problem:** Device doesn't respond to IR commands

**Solutions:**
1. **Check line of sight:** IR requires direct line of sight
2. **Re-learn code:** Sometimes learning fails, try again
3. **Distance:** Keep device within 10 meters of target
4. **Batteries:** Ensure your original remote has good batteries during learning
5. **Multiple attempts:** Try learning the same code 2-3 times

### AC Not Responding

**Problem:** AC doesn't respond or behaves unexpectedly

**Solutions:**
1. **Try different protocol:** Your AC might use a different protocol
2. **Auto-detect:** Use auto-detect feature
3. **Manual state:** Set AC to known state before learning
4. **Re-learn:** Clear and re-learn the protocol

### LED Meaning Reference

| LED Color | Status | Meaning |
|-----------|--------|---------|
| Blue (solid) | Provisioning | Ready to add to app |
| Green (solid) | Connected | WiFi and cloud connected |
| Green (blinking) | Connecting | Trying to connect |
| Purple | Transmitting | Sending IR signal |
| Yellow | Learning | Waiting for IR signal |
| Red | Error | Check WiFi/cloud connection |
| Off | Standby | Normal operation |

---

## FAQ

### How many devices can I control?

The Universal IR Remote can control:
- 1x TV
- 1x Air Conditioner
- 1x Set-Top Box
- 1x Speaker/Soundbar
- 1x Fan
- Multiple custom devices

Each device can have dozens of learned commands (up to **1000 total IR codes**).

### Does it work without internet?

**Learning:** Works offline (uses Bluetooth for app connection)
**Control:** Requires internet for remote control via app
**Local control:** Boot button works offline

### Will I lose my learned codes during updates?

No! IR codes are stored in a dedicated partition that survives:
- Firmware updates (OTA)
- WiFi resets
- App reinstallation

Only **Factory Reset** (10s button hold) erases learned codes.

### Can I backup my learned codes?

Currently, codes are stored on the device only. A future update will add:
- Cloud backup
- Export to file (on devices with SPIFFS)

### What if my device brand isn't listed?

**For TV/STB/Speaker/Fan:** Use learning mode - it works with ANY IR remote!

**For AC:** Try auto-detect, or use "Generic" protocol and manually learn the most common commands.

### Range and Angle

**Optimal:** 1-5 meters, direct line of sight
**Maximum:** 10 meters (may vary by device)
**Angle:** 30° cone from IR LED

### Power Consumption

**Active (WiFi connected):** ~150mA @ 5V
**Sleep mode:** ~50mA @ 5V
**24/7 operation:** Safe and designed for continuous use

---

## Support

For issues or questions:
- Check [Troubleshooting](#troubleshooting) section
- GitHub Issues: [Report a bug](https://github.com/your-repo/issues)
- Email: support@example.com

---

## Appendix: Supported AC Brands

### Fully Supported (Auto-Detect + State Control)
- Daikin
- Voltas (Carrier)
- Hitachi
- Blue Star
- LG
- Samsung
- Mitsubishi
- Fujitsu
- Haier
- Midea
- Panasonic
- Gree

### Partial Support (Manual Learning)
- Most other brands via Generic protocol

---

**Enjoy your Universal IR Remote!** 🎉
