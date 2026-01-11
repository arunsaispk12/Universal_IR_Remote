# IR Transmitter Optimization Guide

## Quick Reference for Maximum Range

### TL;DR - Maximum Range Configuration

```
Hardware:
• IR LED: TSAL6200 or VSLY5940 (940nm, high-power)
• Driver: 2N2222 NPN transistor or 2N7000 MOSFET
• Current limiting resistor: 22Ω (for ~100mA @ 3.3V) or 15Ω (for ~130mA @ 3.3V)
• Power supply: 3.3V or 5V (higher voltage = more range)
• Multiple LEDs: 2-3 LEDs in parallel for 360° coverage

Expected Range:
• Standard (100mA, single LED): 5-8 meters
• High-Power (150mA, single LED): 8-12 meters
• Multi-LED (3× LEDs): 10-15 meters with full room coverage
```

---

## Table of Contents

1. [Hardware Optimization](#hardware-optimization)
2. [Wiring for Long Range](#wiring-for-long-range)
3. [Power Optimization](#power-optimization)
4. [Multiple LED Configuration](#multiple-led-configuration)
5. [Troubleshooting Range Issues](#troubleshooting-range-issues)
6. [Testing IR Transmission](#testing-ir-transmission)

---

## Hardware Optimization

### 1. IR LED Selection

**Recommended LEDs for Maximum Range:**

| Model | Power | Viewing Angle | Range | Notes |
|-------|-------|---------------|-------|-------|
| TSAL6200 | 230mW | 20° (narrow) | 12-15m | Best for direct targeting |
| VSLY5940 | 150mW | 30° (medium) | 8-12m | Good balance |
| TSAL6100 | 100mW | 60° (wide) | 5-8m | Best for room coverage |
| IR333C | 60mW | 40° | 3-5m | Budget option |

**Key Specifications:**
```
Wavelength: 940nm (standard IR remote frequency)
Forward Voltage: 1.2-1.5V @ 100mA
Peak Current: 1A (pulsed, 38kHz carrier = 33% duty cycle)
Continuous Current: 100-150mA maximum
Viewing Angle: Narrower = longer range, Wider = better coverage
```

### 2. Driver Circuit Options

#### Option A: NPN Transistor (Simple, Recommended)

```
ESP32 GPIO17 ──┬── 1kΩ ──┬── Base (2N2222)
               │          │
               │      Collector ──┬── IR LED Anode (+)
               │                  │
               │                22Ω resistor (100mA)
               │               OR 15Ω (130mA for more range)
               │                  │
               │                  └── 3.3V or 5V
               │
               │      Emitter ────────── GND
               │
               └── 10kΩ pull-down to GND (optional, prevents glitches)

Component Values for Different Currents:
• 80mA (safe):   Use 33Ω @ 3.3V or 47Ω @ 5V
• 100mA (standard): Use 22Ω @ 3.3V or 33Ω @ 5V
• 130mA (high-power): Use 15Ω @ 3.3V or 27Ω @ 5V
• 150mA (maximum): Use 12Ω @ 3.3V or 22Ω @ 5V

Note: Higher current = more range but also more heat!
Add heat sink to IR LED if running >120mA continuously.
```

**Calculate Resistor Value:**
```c
// Formula: R = (V_supply - V_led_drop) / I_desired
// Example for 100mA @ 3.3V:
R = (3.3V - 1.4V) / 0.100A = 1.9V / 0.100A = 19Ω
// Use nearest standard value: 22Ω (results in ~86mA, safe)

// Example for 100mA @ 5V:
R = (5.0V - 1.4V) / 0.100A = 3.6V / 0.100A = 36Ω
// Use nearest standard value: 33Ω (results in ~109mA)
```

#### Option B: MOSFET (Better for High-Frequency PWM)

```
ESP32 GPIO17 ──┬── 100Ω ──┬── Gate (2N7000)
               │           │
               │       Drain ──┬── IR LED Anode (+)
               │               │
               │             22Ω resistor
               │               │
               │               └── 3.3V or 5V
               │
               │      Source ────────── GND
               │
               └── 10kΩ pull-down to GND (MANDATORY!)

Advantages:
• Faster switching (better for 38kHz carrier)
• Lower voltage drop (RDS(on) ≈ 5Ω vs VCE(sat) ≈ 0.2V)
• Lower gate current (microamps vs milliamps)
• Can handle higher currents (500mA+ with proper MOSFETs)
```

### 3. Power Supply Optimization

**Voltage vs Range:**
```
3.3V Supply:
• LED forward current: ~86mA (with 22Ω)
• LED power: ~120mW
• Range: 5-8 meters (standard)

5.0V Supply:
• LED forward current: ~109mA (with 33Ω)
• LED power: ~150mW
• Range: 8-12 meters (+50% vs 3.3V)

Recommendation: Use 5V for IR LED power if available
• ESP32 logic still uses 3.3V
• Only IR LED driver powered from 5V
• Requires separate 5V supply or DC-DC boost converter
```

**Circuit with 5V Boost:**
```
┌──────────────────────────────────────┐
│ ESP32 Module (3.3V logic)            │
│                                      │
│ GPIO17 ──┬── 1kΩ ───┬── NPN Base    │
│          │           │               │
└──────────┼───────────┼───────────────┘
           │           │
           │    Collector ──┬── IR LED (+)
           │                │
           │              33Ω (for 109mA @ 5V)
           │                │
           │         ┌──────┴──────┐
           │         │ 5V Boost    │ (MT3608 or XL6009)
           │         │ Converter   │  Input: 3.3V
           │         │             │  Output: 5.0V @ 200mA
           │         └─────────────┘
           │                │
           │         Emitter ───┘
           │                │
           └────────────────┴──────── GND (common)
```

---

## Wiring for Long Range

### Short Distance (<1m) - Direct Connection

```
ESP32 Board ── 20cm wire ── IR TX LED

Wire: 22-26 AWG hookup wire
No special requirements
```

### Medium Distance (1-5m) - Buffered Connection

```
ESP32 Module              Long Cable            Remote IR Module
┌────────────┐            ┌──────────┐          ┌────────────────┐
│ GPIO17  ───┼─► 74HC125 ─┤ Shielded ├──────────┤ IR TX Driver   │
│            │   Buffer   │ 4-wire   │          │ (2N2222 + LED) │
│ 3.3V    ───┼────────────┤ Cable    ├──────────┤ 3.3V or 5V     │
│ GND     ───┼────────────┤          ├──────────┤ GND            │
└────────────┘            └──────────┘          └────────────────┘

Wire Specs:
• Type: 4-conductor shielded cable
• Gauge: 22 AWG for signals, 20 AWG for power
• Shield: Foil or braid, connected to GND at ESP32 side ONLY
• Max Length: 5 meters with buffer, 10 meters with differential signaling
```

**See `HARDWARE_WIRING_GUIDE.md` for detailed long-distance wiring (5-20m).**

---

## Power Optimization

### Carrier Duty Cycle

The IR carrier duty cycle determines how much power is transmitted:

```c
// Current firmware setting (components/ir_control/ir_control.c:1536)
.duty_cycle = 0.33f,  // 33% duty cycle (standard, power-efficient)

// For maximum range:
.duty_cycle = 0.50f,  // 50% duty cycle (+20% power, +15% range)

Trade-offs:
• 33% duty cycle: Lower power, cooler LED, longer LED life, good range
• 50% duty cycle: Higher power, warmer LED, maximum range
```

**When to Increase Duty Cycle:**
- Large room (>8m distance to devices)
- Multiple target devices in different directions
- Devices with poor IR sensitivity
- Through obstacles (glass cabinet doors, etc.)

**When to Keep 33% Duty Cycle:**
- Small to medium room (<8m)
- Single target device
- Power-sensitive application (battery powered)
- Thermal concerns (enclosed spaces)

### Pulse Width and Timing

The firmware uses standard NEC timing:
```c
// NEC Protocol (most common)
Leading pulse: 9000µs mark + 4500µs space
Data bit '0': 560µs mark + 560µs space
Data bit '1': 560µs mark + 1690µs space
```

These timings are optimized for compatibility and should **not** be changed unless you have a specific reason.

---

## Multiple LED Configuration

### Why Use Multiple LEDs?

**Single LED:**
- Narrow beam (20-30°)
- Must point at device
- Range: 8-12m in one direction

**Multiple LEDs:**
- 360° coverage (if positioned correctly)
- No aiming needed
- Same range in all directions

### Parallel LED Configuration (Recommended)

```
            LED 1 (points forward) ──┬─── 22Ω ──┬──── 3.3V or 5V
            LED 2 (points left)    ──┤          │
            LED 3 (points right)   ──┘          │
                                                 │
                                         2N2222 Collector
                                                 │
                                         ESP32 GPIO17 (via 1kΩ base resistor)
                                                 │
                                         2N2222 Emitter ──── GND

LED Positioning (Ceiling Mount):
• LED 1: Points forward (0°)
• LED 2: Points 120° left
• LED 3: Points 120° right
• Total coverage: 360° horizontal

Current:
• Each LED: ~86mA (with 22Ω @ 3.3V)
• Total: 260mA (within 2N2222 limit of 800mA)
• Power supply: Needs 300mA minimum for IR TX

For 5V supply:
• Use 33Ω resistor per LED → ~109mA each
• Total current: 330mA
```

### LED Array for Very Long Range (10-15m)

```
            LED 1 ──┬── 15Ω ──┬──── 5V
            LED 2 ──┤         │
            LED 3 ──┘         │
                              │
                      TIP120 Darlington Transistor (can handle 5A)
                              │
                      ESP32 GPIO17 (via 2.2kΩ base resistor)
                              │
                      TIP120 Emitter ──── GND

Current per LED: ~140mA (with 15Ω @ 5V)
Total current: 420mA
Range: 12-15 meters

Warning: Requires heatsink on LEDs and transistor!
```

---

## Troubleshooting Range Issues

### Problem: Range is only 1-2 meters

**Checklist:**
1. ✅ Measure LED current
   ```
   Measure voltage across current-limiting resistor
   V_resistor = I_led × R_resistor
   Example: 1.9V across 22Ω = 86mA (good)
   If <1.5V: Current too low, reduce resistor value
   ```

2. ✅ Check LED orientation
   ```
   IR LED has polarity: Anode (+) to resistor, Cathode (-) to transistor collector
   Flat side of LED = Cathode (shorter leg)
   Rounded side = Anode (longer leg)
   ```

3. ✅ Verify transistor saturation
   ```
   Measure collector-emitter voltage during transmission
   Should be <0.3V (saturated)
   If >0.5V: Transistor not saturated, reduce base resistor (1kΩ → 470Ω)
   ```

4. ✅ Confirm correct wavelength
   ```
   IR LED must be 940nm (standard for remotes)
   Some LEDs are 850nm (security cameras) - won't work!
   Check LED datasheet
   ```

5. ✅ Check power supply voltage
   ```
   Measure VCC during transmission (should stay >3.0V)
   If drops below 3.0V: Add 100µF capacitor near IR LED
   ```

### Problem: Range varies by device

**Possible Causes:**
- Different devices have different IR receiver sensitivity
- Some devices need higher carrier frequency (40kHz vs 38kHz)
- Some protocols require specific timing

**Solutions:**
- Verify carrier frequency matches device (check protocol database)
- Increase LED current for less sensitive devices
- Use RAW learning mode to capture exact timing

### Problem: Works close but not far

**Likely Cause: Insufficient current to LED**

**Test Procedure:**
```bash
# Use smartphone camera to visualize IR LED
# Point IR LED at camera while transmitting
# Should see bright purple/white flashing

# If dim or not visible:
# 1. Reduce current-limiting resistor (22Ω → 15Ω)
# 2. Upgrade to higher power LED (TSAL6200)
# 3. Add second LED in parallel
```

---

## Testing IR Transmission

### Visual Test (Smartphone Camera)

```
1. Open smartphone camera app
2. Point IR LED at camera lens
3. Press button on ESP32 to transmit
4. Observe IR LED through camera viewfinder

Expected: Bright purple/white flashing visible on screen
(38kHz carrier appears solid due to camera shutter speed)

If not visible: LED not transmitting or very weak
```

### Oscilloscope Test (If Available)

```
Probe 1: ESP32 GPIO17 (RMT output)
Expected: Rectangular pulses at RMT symbol rate
  - NEC: 560µs mark/space pulses
  - Should see 38kHz carrier modulation

Probe 2: IR LED anode (or transistor collector)
Expected: 38kHz bursts during mark periods
  - Amplitude: ~2V peak @ 3.3V supply, ~3.5V peak @ 5V supply
  - Duty cycle: 33% (12.3µs high, 13.8µs low per carrier cycle)
```

### Range Test Procedure

```
1. Set up target device (TV, AC, etc.) at known distance
2. Transmit learned IR code from ESP32
3. Observe if device responds
4. Move device farther away and repeat
5. Record maximum working distance

Expected Results:
• Standard config (100mA, single LED): 5-8m
• High-power config (150mA, single LED): 8-12m
• Multi-LED config (3× 100mA): 10-15m with 360° coverage

If below expected:
• Check LED current (increase if possible)
• Verify LED wavelength (must be 940nm)
• Check power supply (add bulk capacitor)
• Try 5V supply instead of 3.3V
```

---

## Quick Reference: Resistor Values

### For 3.3V Supply

| Target Current | Resistor Value | Actual Current | Range | Notes |
|----------------|----------------|----------------|-------|-------|
| 60mA (safe) | 33Ω | ~58mA | 3-5m | Very safe, cool LED |
| 80mA (standard) | 22Ω | ~86mA | 5-8m | Recommended default |
| 100mA (moderate) | 18Ω | ~106mA | 6-10m | Good balance |
| 130mA (high) | 15Ω | ~127mA | 8-12m | Warm LED, needs ventilation |
| 150mA (maximum) | 12Ω | ~158mA | 10-15m | Hot LED, add heatsink! |

### For 5V Supply

| Target Current | Resistor Value | Actual Current | Range | Notes |
|----------------|----------------|----------------|-------|-------|
| 60mA (safe) | 56Ω | ~64mA | 3-5m | Very safe, cool LED |
| 80mA (standard) | 47Ω | ~77mA | 5-8m | Recommended default |
| 100mA (moderate) | 33Ω | ~109mA | 6-10m | Good balance |
| 130mA (high) | 27Ω | ~133mA | 8-12m | Warm LED, needs ventilation |
| 150mA (maximum) | 22Ω | ~164mA | 10-15m | Hot LED, add heatsink! |

**Formula:**
```
R = (V_supply - 1.4V) / I_target

Where:
• V_supply = 3.3V or 5.0V
• I_target = desired current in amps (e.g., 0.100 for 100mA)
• 1.4V = typical IR LED forward voltage @ 100mA
```

---

## Safety Notes

**⚠️ Important Safety Warnings:**

1. **Heat Management**
   - IR LEDs running >100mA will get warm
   - At 150mA, LEDs can reach 50-70°C (hot to touch)
   - Add heatsink or use thermal epoxy to PCB ground plane
   - Ensure adequate ventilation

2. **Eye Safety**
   - 940nm IR is invisible to human eye
   - High-power IR can cause eye damage if stared at directly
   - Keep transmission time brief (<5 seconds)
   - Do not point IR LED at eyes at close range (<10cm)

3. **Power Consumption**
   - 3× 150mA LEDs = 450mA peak during transmission
   - Ensure power supply can handle peak current
   - Add 100µF bulk capacitor near IR LEDs
   - Check ESP32 regulator rating (must handle total current)

4. **Continuous Operation**
   - IR remotes transmit in short bursts (typically <1 second)
   - Firmware limits transmission to short pulses
   - Do not modify firmware to enable continuous transmission
   - Continuous high-current transmission will overheat LED

---

## Recommended Configurations

### Configuration 1: Standard Home Use
```
LED: VSLY5940 (single LED)
Current: 100mA
Supply: 3.3V
Resistor: 22Ω
Driver: 2N2222
Range: 5-8 meters
Cost: ~$0.50
```

### Configuration 2: Large Room
```
LED: TSAL6200 (single LED)
Current: 130mA
Supply: 5V (with boost converter)
Resistor: 27Ω
Driver: 2N2222
Range: 8-12 meters
Cost: ~$2.00
```

### Configuration 3: Full Room Coverage
```
LED: 3× VSLY5940 (120° spacing)
Current: 100mA per LED
Supply: 5V (with boost converter)
Resistor: 33Ω per LED
Driver: 2N2222
Range: 10-15 meters (360° coverage)
Cost: ~$3.50
```

### Configuration 4: Maximum Range
```
LED: 3× TSAL6200 (focused forward)
Current: 150mA per LED
Supply: 5V
Resistor: 22Ω per LED
Driver: TIP120 Darlington
Range: 15-20 meters (directional)
Cost: ~$5.00
Requires: Heatsinks on LEDs and transistor
```

---

**Document Version**: 1.0
**Last Updated**: January 11, 2026
**Firmware Version**: v2.3.0+
**Related Documents**:
- `HARDWARE_WIRING_GUIDE.md` - Complete wiring guide for all distances
- `README.md` - User guide and setup instructions
- `docs/hardware/README.md` - Hardware specifications
