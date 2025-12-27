# Air Conditioner State Machine Architecture

## Executive Summary

Air conditioner IR remotes operate fundamentally different from TV/STB remotes. While a TV remote sends simple commands ("Volume Up"), AC remotes send **complete state snapshots** every time (Power=ON, Mode=COOL, Temp=24°C, Fan=AUTO, Swing=ON, etc.). This document explains the current learn/replay approach and the future state-based architecture.

---

## 🔍 The Problem: Why AC Remotes Are Different

### TV Remote (Simple Command Model)
```
Button Press → Single Command → Device Updates State

Example:
[Volume Up] → NEC: 0x00FF629D → TV increases volume by 1

State is stored IN THE TV, not in the remote signal.
```

### AC Remote (State Broadcast Model)
```
Button Press → FULL STATE → Device Replaces Entire State

Example:
[Temp +1] → Daikin: [Power=ON, Mode=COOL, Temp=25, Fan=AUTO, Swing=ON, Timer=OFF, ...]
                     ↑ ALL parameters sent every time (216 bits!)

State is encoded IN THE IR SIGNAL, device is stateless.
```

**Key Difference**:
- **TV**: "Do this action" (relative change)
- **AC**: "Be in this state" (absolute state)

---

## 📊 Current Implementation (v2.3.0): Learn & Replay

### What Works Today ✅

The firmware can successfully:

1. **Decode AC protocols** (7 major brands):
   - Daikin (multi-frame, 312 bits)
   - Mitsubishi Electric (152 bits)
   - Carrier/Voltas (#1 in India, 128 bits)
   - Hitachi (variable 264/344 bits)
   - Fujitsu General (variable 64-128 bits)
   - Haier (104 bits)
   - Midea (48 bits, OEM for many brands)

2. **Extract state information** (where defined in decoders):
   - Protocol type
   - Raw bits
   - Checksums validated
   - Frame structure

3. **Store complete IR frames**:
   - Protocol-based storage (~200-500 bytes per AC command)
   - RAW fallback for unknown AC brands (~600-800 bytes)
   - Metadata: carrier frequency (38kHz), repeat period

4. **Replay exact learned codes**:
   - Bit-perfect reproduction
   - Correct carrier frequency
   - Proper timing

### How Users Use It Today

**Learn Mode Workflow**:
```
Step 1: Learn "Power On - Cool Mode - 24°C - Auto Fan"
Step 2: Learn "Power On - Cool Mode - 25°C - Auto Fan"
Step 3: Learn "Power On - Cool Mode - 26°C - Auto Fan"
Step 4: Learn "Power On - Heat Mode - 24°C - Auto Fan"
...
Step 50: Learn "Power Off"

Total: ~50-100 learned codes per AC unit
```

**Transmit Workflow**:
```
User selects learned code: "Cool 24°C"
Firmware replays exact IR frame learned earlier
AC receives state and updates to 24°C Cool mode
```

### Limitations ❌

1. **Cannot modify state programmatically**:
   ```c
   // What you CANNOT do today:
   ac_state_t state = learned_code.state;
   state.temperature = 25;  // Change temp from 24 to 25
   ir_transmit_ac_state(&state);  // Generate new IR frame
   ```

2. **Cannot generate new commands**:
   - If you learned "Cool 24°C" but not "Cool 25°C", you can't send 25°C
   - Must learn EVERY combination you might need
   - No "smart" temperature adjustment

3. **High storage cost**:
   - 13 temperatures × 4 modes × 4 fan speeds = 208 combinations
   - Each takes ~250 bytes → 52KB total (exceeds typical NVS size)
   - Must be selective about what to learn

4. **No smart home integration** (without extensive learning):
   - Voice control: "Set AC to 24°C" → requires pre-learned 24°C code
   - Schedules: "Set to 22°C at night" → requires all temps learned
   - Automation: Temperature-based adjustment impossible

---

## 🏗️ Future Architecture: State-Based Encoding

### Goal

Enable the firmware to **generate AC IR frames from state parameters** without requiring every combination to be learned.

### Architecture Components

#### 1. AC State Structure

```c
/**
 * @brief Universal AC state representation
 *
 * Covers common parameters across all AC brands
 */
typedef struct {
    // Power
    bool power;                     // ON/OFF

    // Operating mode
    ac_mode_t mode;                 // COOL, HEAT, DRY, FAN, AUTO

    // Temperature
    uint8_t temperature_c;          // 16-30°C (varies by brand)
    ac_temp_unit_t temp_unit;       // CELSIUS or FAHRENHEIT

    // Fan control
    ac_fan_speed_t fan_speed;       // AUTO, LOW, MEDIUM, HIGH, TURBO
    bool fan_turbo;                 // Turbo/Powerful mode
    bool fan_quiet;                 // Quiet/Silent mode

    // Airflow
    ac_swing_mode_t swing_vertical; // AUTO, TOP, MID_HIGH, MID, MID_LOW, BOTTOM, OFF
    ac_swing_mode_t swing_horizontal; // AUTO, LEFT, MID_LEFT, MID, MID_RIGHT, RIGHT, WIDE, OFF

    // Advanced features
    bool econo_mode;                // Economy/Power-saving
    bool sleep_mode;                // Sleep/Comfort mode
    bool light;                     // Display/LED on/off
    bool beep;                      // Beep on commands
    bool filter;                    // Filter indicator/clean reminder
    bool health_mode;               // Ion/Health/Pure mode (Daikin, Haier)

    // Timer
    bool timer_on_enabled;          // ON timer enabled
    uint16_t timer_on_minutes;      // Minutes until turn ON
    bool timer_off_enabled;         // OFF timer enabled
    uint16_t timer_off_minutes;     // Minutes until turn OFF

    // Brand-specific extensions
    union {
        daikin_extended_t daikin;
        mitsubishi_extended_t mitsubishi;
        carrier_extended_t carrier;
        hitachi_extended_t hitachi;
        // ... other brands
    } brand_specific;

} ac_state_t;
```

#### 2. Brand-Specific State Encoders

Each AC protocol needs a state-to-IR encoder function:

```c
/**
 * @brief Encode Daikin AC state to IR frame
 *
 * Generates a complete Daikin IR frame (312 bits, multi-frame)
 * from the AC state structure
 *
 * @param state AC state parameters
 * @param code Output IR code (will populate protocol, bits, raw_data)
 * @return ESP_OK on success
 */
esp_err_t ir_encode_daikin_ac(const ac_state_t *state, ir_code_t *code);

/**
 * @brief Encode Mitsubishi AC state to IR frame
 */
esp_err_t ir_encode_mitsubishi_ac(const ac_state_t *state, ir_code_t *code);

/**
 * @brief Encode Carrier/Voltas AC state to IR frame
 */
esp_err_t ir_encode_carrier_ac(const ac_state_t *state, ir_code_t *code);

// ... etc for each brand
```

#### 3. State Decoder Enhancement

Enhance existing decoders to extract state:

```c
/**
 * @brief Decode Daikin AC frame to state structure
 *
 * Already implemented: Protocol decode
 * NEW: Extract all state parameters from decoded frame
 *
 * @param code Decoded IR code
 * @param state Output AC state
 * @return ESP_OK on success
 */
esp_err_t ir_decode_daikin_ac_state(const ir_code_t *code, ac_state_t *state);
```

#### 4. High-Level AC Control API

```c
/**
 * @brief Set AC to specific state and transmit
 *
 * This is the main API for smart home integration
 *
 * @param ac_id AC unit identifier (which AC in multi-unit systems)
 * @param state Desired AC state
 * @return ESP_OK on success
 */
esp_err_t ac_set_state(uint8_t ac_id, const ac_state_t *state);

/**
 * @brief Get current AC state (from last transmission or learned default)
 */
esp_err_t ac_get_state(uint8_t ac_id, ac_state_t *state);

/**
 * @brief Adjust temperature (relative change)
 */
esp_err_t ac_adjust_temperature(uint8_t ac_id, int8_t delta_celsius);

/**
 * @brief Quick-set functions
 */
esp_err_t ac_power_on(uint8_t ac_id);
esp_err_t ac_power_off(uint8_t ac_id);
esp_err_t ac_set_mode(uint8_t ac_id, ac_mode_t mode);
esp_err_t ac_set_temperature(uint8_t ac_id, uint8_t temp_c);
```

---

## 🔬 Implementation Approach

### Phase 1: State Decoding (Estimated: 1 week)

**Goal**: Extract state from existing decoders

**Tasks**:
1. Define `ac_state_t` structure
2. Enhance Daikin decoder to extract state (reference implementation)
3. Enhance Mitsubishi decoder
4. Enhance Carrier decoder
5. Enhance Hitachi, Fujitsu, Haier, Midea decoders
6. Test: Learn AC code → decode → verify state is correct

**Deliverable**: Can decode AC codes to state structures

---

### Phase 2: State Encoding - Single Brand (Estimated: 3-4 days)

**Goal**: Prove encoding works for one brand

**Tasks**:
1. Implement `ir_encode_daikin_ac()` (Daikin chosen - well documented)
2. Study Daikin frame structure:
   - Frame 1: 8 bytes (header + basic settings)
   - Gap: 29ms
   - Frame 2: 19 bytes (full state + checksum)
3. Implement byte packing:
   - Temperature encoding (offset, scaling)
   - Mode bits
   - Fan speed bits
   - Swing bits
   - Feature flags
   - Checksum calculation
4. Test: Generate 24°C code → transmit → verify AC responds
5. Test: Compare generated vs learned (should be identical)

**Deliverable**: Working Daikin state encoder

**Code Example**:
```c
esp_err_t ir_encode_daikin_ac(const ac_state_t *state, ir_code_t *code) {
    uint8_t frame1[8] = {0x11, 0xDA, 0x27, 0x00, 0xC5, 0x00, 0x00, 0xD7};
    uint8_t frame2[19] = {0};

    // Header
    frame2[0] = 0x11;
    frame2[1] = 0xDA;
    frame2[2] = 0x27;

    // Power
    frame2[5] = state->power ? 0xC1 : 0xC0;

    // Mode (bits in byte 6)
    switch (state->mode) {
        case AC_MODE_COOL: frame2[6] |= 0x30; break;
        case AC_MODE_HEAT: frame2[6] |= 0x40; break;
        case AC_MODE_DRY:  frame2[6] |= 0x20; break;
        case AC_MODE_FAN:  frame2[6] |= 0x60; break;
        case AC_MODE_AUTO: frame2[6] |= 0x00; break;
    }

    // Temperature (byte 6, lower nibble + byte 7)
    // Daikin: temp = (byte_value / 2) where byte_value = (temp_c * 2)
    uint8_t temp_encoded = (state->temperature_c * 2);
    frame2[6] |= (temp_encoded >> 4);
    frame2[7] = (temp_encoded << 4);

    // Fan speed (byte 8)
    switch (state->fan_speed) {
        case AC_FAN_AUTO: frame2[8] = 0xA0; break;
        case AC_FAN_QUIET: frame2[8] = 0xB0; break;
        case AC_FAN_LOW: frame2[8] = 0x30; break;
        case AC_FAN_MEDIUM: frame2[8] = 0x40; break;
        case AC_FAN_HIGH: frame2[8] = 0x50; break;
        case AC_FAN_TURBO: frame2[8] = 0x60; break;
    }

    // Swing (byte 8, bits 0-3)
    if (state->swing_vertical == AC_SWING_AUTO) {
        frame2[8] |= 0x0F;
    }

    // Additional features (bytes 9-12)
    if (state->econo_mode) frame2[10] |= 0x04;
    if (state->fan_turbo) frame2[10] |= 0x01;

    // Checksum (byte 18 = sum of bytes 0-17)
    uint8_t checksum = 0;
    for (int i = 0; i < 18; i++) {
        checksum += frame2[i];
    }
    frame2[18] = checksum;

    // TODO: Convert to RMT symbols
    // TODO: Set carrier frequency = 38kHz
    // TODO: Populate code structure

    return ESP_OK;
}
```

---

### Phase 3: State Encoding - All Brands (Estimated: 1-2 weeks)

**Tasks**:
1. Implement encoders for remaining brands:
   - Mitsubishi (152 bits, single frame)
   - Carrier/Voltas (128 bits, nibble checksum)
   - Hitachi (264/344 bits variable)
   - Fujitsu (variable length)
   - Haier (104 bits, XOR checksum)
   - Midea (48 bits, inverted validation)

2. Handle brand-specific quirks:
   - Mitsubishi: Different checksum algorithm
   - Carrier: Nibble-based checksum
   - Hitachi: Variable length based on features enabled
   - Daikin: Multi-frame with gap timing

3. Testing matrix (per brand):
   - Test all modes (Cool, Heat, Dry, Fan, Auto)
   - Test temperature range (16-30°C)
   - Test all fan speeds
   - Test swing modes
   - Verify checksum correctness

**Deliverable**: All 7 AC brands have working encoders

---

### Phase 4: High-Level API & Storage (Estimated: 3-4 days)

**Tasks**:
1. Implement AC state storage in NVS
   ```c
   // Store current AC state (not the IR code, just the state!)
   esp_err_t ac_save_state(uint8_t ac_id, const ac_state_t *state);
   esp_err_t ac_load_state(uint8_t ac_id, ac_state_t *state);
   ```

2. Implement high-level API:
   - `ac_set_state()` - Main control function
   - `ac_get_state()` - Query current state
   - `ac_adjust_temperature()` - Relative control
   - `ac_power_on()`, `ac_power_off()` - Quick commands

3. Brand detection & configuration:
   ```c
   // User configures which AC brand they have
   esp_err_t ac_configure(uint8_t ac_id, ac_brand_t brand);
   ```

4. Smart defaults:
   - First-time setup wizard (learn one code, detect brand/state)
   - Sensible defaults for unconfigured parameters

**Deliverable**: Complete AC control API

---

### Phase 5: Smart Home Integration (Estimated: 2-3 days)

**Tasks**:
1. ESP RainMaker integration:
   ```c
   // Add AC device type to RainMaker
   esp_rmaker_device_t *ac_device = esp_rmaker_ac_device_create(...);

   // Expose AC state parameters
   esp_rmaker_device_add_param(ac_device, "Power", ...);
   esp_rmaker_device_add_param(ac_device, "Temperature", ...);
   esp_rmaker_device_add_param(ac_device, "Mode", ...);
   ```

2. Voice control mapping:
   - "Alexa, set AC to 24 degrees" → `ac_set_temperature(0, 24)`
   - "Alexa, turn on AC" → `ac_power_on(0)` then `ac_set_state(0, default_cool_state)`
   - "Alexa, make it cooler" → `ac_adjust_temperature(0, -2)`

3. Automation examples:
   ```c
   // Schedule: Bedtime cooling
   if (hour == 22) {
       ac_state_t night_mode = {
           .power = true,
           .mode = AC_MODE_COOL,
           .temperature_c = 22,
           .fan_speed = AC_FAN_QUIET,
           .swing_vertical = AC_SWING_AUTO
       };
       ac_set_state(0, &night_mode);
   }

   // Temperature-based: Auto cool
   if (room_temp > 28) {
       ac_power_on(0);
       ac_set_temperature(0, 24);
       ac_set_mode(0, AC_MODE_COOL);
   }
   ```

**Deliverable**: Full smart home AC control

---

## 📈 Benefits of State-Based Architecture

### Storage Savings
| Scenario | Learn & Replay | State-Based | Savings |
|----------|----------------|-------------|---------|
| Basic control (10 temps) | 10 × 250 bytes = 2.5KB | 1 × 60 bytes state = 60 bytes | **97%** |
| Full control (208 combos) | 208 × 250 = 52KB | 1 × 60 bytes state = 60 bytes | **99.9%** |

### Smart Home Features Unlocked
- ✅ Voice control: "Set AC to 24°C"
- ✅ Schedules: Different temps for day/night
- ✅ Automation: Temperature-based triggers
- ✅ Scenes: "Movie mode" (22°C, quiet fan, swing off)
- ✅ Remote control: Adjust from anywhere (cloud/app)

### User Experience
- ❌ Before: Learn 50+ codes per AC
- ✅ After: Learn 1 code (brand detection) or manual config

---

## ⚠️ Challenges & Risks

### Challenge 1: Brand Variations

**Problem**: Same brand (e.g., Daikin) may have different models with different protocols

**Mitigation**:
- Support multiple variants per brand (Daikin v1, v2, v3)
- Auto-detect variant during learn mode
- Fallback to learn/replay for unknown variants

### Challenge 2: Undocumented Protocols

**Problem**: Some AC brands don't have public protocol documentation

**Mitigation**:
- Reverse engineer from learned codes (decode multiple samples)
- Community-sourced protocol specs (GitHub, forums)
- Hybrid approach: State-based for known brands, learn/replay for others

### Challenge 3: Testing Coverage

**Problem**: Cannot test all AC brands/models (expensive!)

**Mitigation**:
- Start with popular brands (Daikin, Mitsubishi, Voltas)
- Community testing (beta testers with real AC units)
- Comparison testing (generated vs learned codes should match)

### Challenge 4: Checksum Algorithms

**Problem**: Some brands use complex checksums (CRC, proprietary)

**Mitigation**:
- Reference Arduino-IRremote (checksums already implemented)
- Validate against known-good samples
- Unit tests with test vectors

---

## 🧪 Testing Strategy

### Unit Tests
```c
// Test: State encoding matches learned code
void test_daikin_encoding() {
    // 1. Learn code from real Daikin remote (24°C Cool)
    ir_code_t learned_code = { /* from real remote */ };

    // 2. Extract state from learned code
    ac_state_t state;
    ir_decode_daikin_ac_state(&learned_code, &state);

    // 3. Encode state back to IR code
    ir_code_t generated_code;
    ir_encode_daikin_ac(&state, &generated_code);

    // 4. Compare (should be identical)
    assert(generated_code.bits == learned_code.bits);
    assert(memcmp(generated_code.data, learned_code.data, bytes) == 0);
}
```

### Integration Tests
```c
// Test: Full workflow
void test_ac_control_workflow() {
    // 1. Configure AC brand
    ac_configure(0, AC_BRAND_DAIKIN);

    // 2. Set state via API
    ac_state_t state = {
        .power = true,
        .mode = AC_MODE_COOL,
        .temperature_c = 24,
        .fan_speed = AC_FAN_AUTO
    };
    ac_set_state(0, &state);

    // 3. Verify IR code was transmitted with correct protocol
    // (check via logic analyzer or IR receiver)

    // 4. Adjust temperature
    ac_adjust_temperature(0, +2);  // 24 → 26°C

    // 5. Verify state was updated
    ac_state_t new_state;
    ac_get_state(0, &new_state);
    assert(new_state.temperature_c == 26);
}
```

### Hardware Tests (with real AC units)
1. Test on Daikin AC: Set 24°C, verify AC responds
2. Test on Mitsubishi AC: Set Heat mode 28°C
3. Test on Voltas AC (#1 in India): Full feature test
4. Verify range: 10+ meters
5. Verify reliability: 100 consecutive commands

---

## 📋 Implementation Checklist

### Phase 1: State Decoding
- [ ] Define `ac_state_t` structure
- [ ] Enhance Daikin decoder with state extraction
- [ ] Enhance Mitsubishi decoder with state extraction
- [ ] Enhance Carrier decoder with state extraction
- [ ] Enhance Hitachi decoder with state extraction
- [ ] Enhance Fujitsu decoder with state extraction
- [ ] Enhance Haier decoder with state extraction
- [ ] Enhance Midea decoder with state extraction
- [ ] Test: Decode learned codes to state

### Phase 2: State Encoding (Daikin Reference)
- [ ] Implement `ir_encode_daikin_ac()`
- [ ] Frame 1 generation (8 bytes)
- [ ] Frame 2 generation (19 bytes)
- [ ] Multi-frame timing (29ms gap)
- [ ] Checksum calculation
- [ ] RMT symbol generation
- [ ] Test: Generated code matches learned code

### Phase 3: State Encoding (All Brands)
- [ ] Implement `ir_encode_mitsubishi_ac()`
- [ ] Implement `ir_encode_carrier_ac()`
- [ ] Implement `ir_encode_hitachi_ac()`
- [ ] Implement `ir_encode_fujitsu_ac()`
- [ ] Implement `ir_encode_haier_ac()`
- [ ] Implement `ir_encode_midea_ac()`
- [ ] Test matrix: All brands × modes × temps

### Phase 4: High-Level API
- [ ] Implement `ac_set_state()`
- [ ] Implement `ac_get_state()`
- [ ] Implement `ac_adjust_temperature()`
- [ ] Implement quick commands (power, mode)
- [ ] NVS state storage
- [ ] Brand configuration

### Phase 5: Smart Home Integration
- [ ] ESP RainMaker AC device type
- [ ] Voice control mapping
- [ ] Automation examples
- [ ] Cloud sync

---

## 🎯 Estimated Effort

| Phase | Complexity | Time | Depends On |
|-------|-----------|------|------------|
| Phase 1: State Decoding | Medium | 1 week | Current decoders |
| Phase 2: Daikin Encoding | High | 3-4 days | Phase 1 |
| Phase 3: All Brand Encoding | High | 1-2 weeks | Phase 2 |
| Phase 4: High-Level API | Medium | 3-4 days | Phase 3 |
| Phase 5: Smart Home | Low | 2-3 days | Phase 4 |

**Total**: 3-4 weeks for complete implementation

---

## 📚 References

### Protocol Documentation
- Arduino-IRremote library: https://github.com/Arduino-IRremote/Arduino-IRremote
- Daikin protocol: https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/ir_Daikin.cpp
- Mitsubishi protocol: https://github.com/r45635/HVAC-IR-Control
- Reverse engineering guide: http://www.righto.com/2016/08/reverse-engineering-mitsubishi.html

### Similar Projects
- IRremoteESP8266 (Arduino): Full AC state support (reference implementation)
- Tasmota: IR AC control with state
- ESPHome: Climate component with IR

### ESP32 Resources
- ESP-IDF RMT documentation
- ESP RainMaker AC device type specification

---

## 📝 Current Status

**Version**: 2.3.0
**Status**: ❌ Not Implemented (Learn/Replay works, State encoding planned)
**Recommendation**:
- **For now**: Use learn/replay (works reliably)
- **Future**: State-based encoding (unlocks smart home features)

**Next Steps**:
1. Complete commercial-grade learn/replay features ✅ DONE
2. Test with real AC units (verify decode accuracy)
3. Begin Phase 1 (State Decoding) when smart home features required

---

**Last Updated**: December 27, 2025
**Architect**: AI Assistant
**Review Status**: Draft (requires stakeholder approval before implementation)
