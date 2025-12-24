# Universal IR Remote - Complete Hardware Design Guide
## Multi-Directional IR Transmitter System

**Version:** 1.0.0
**Date:** December 24, 2024
**Project:** Universal IR Remote with 360° Coverage

---

## 📋 Table of Contents

1. [Overview](#overview)
2. [Design Options](#design-options)
3. [Component Selection](#component-selection)
4. [Circuit Designs](#circuit-designs)
5. [PCB Layout Guidelines](#pcb-layout-guidelines)
6. [Assembly Instructions](#assembly-instructions)
7. [Testing & Calibration](#testing--calibration)
8. [Enclosure Design](#enclosure-design)
9. [Bill of Materials](#bill-of-materials)
10. [Troubleshooting](#troubleshooting)

---

## 🎯 Overview

### Project Goals

Create a universal IR remote with **360° coverage** to control devices in any direction without aiming, using multiple IR transmitters positioned strategically.

### Key Features

✅ **360° IR Coverage** - Transmit in all directions simultaneously
✅ **Long Range** - Up to 15 meters effective range
✅ **Single Receiver** - One IR receiver for learning
✅ **Multiple Transmitters** - 4-8 IR LEDs for omnidirectional coverage
✅ **High Power** - Sufficient IR power for reliable transmission
✅ **Low Cost** - ~$10-15 total hardware cost

---

## 🔧 Design Options

### Option 1: 4-LED Square Configuration (Recommended)

**Coverage:** 4 directions (North, East, South, West)
**Best For:** Room-corner placement, wall mounting
**Cost:** Low (~$10)
**Complexity:** Simple

```
Top View:
         N
         │
    ┌────┼────┐
    │    ↑    │
W ──┤  ← ESP32→ ├── E
    │    ↓    │
    └────┼────┘
         │
         S

Legend:
↑ ↓ ← → = IR LED directions
ESP32 = Controller board
```

**Pros:**
- Simple to build and test
- Good coverage for most rooms
- Easy PCB layout
- Lower power consumption

**Cons:**
- Coverage gaps at 45° angles
- May need aiming for corner devices

---

### Option 2: 6-LED Hexagonal Configuration

**Coverage:** 6 directions (60° spacing)
**Best For:** Center of room, ceiling mount
**Cost:** Medium (~$12)
**Complexity:** Moderate

```
Top View:
       60°
    ↗  │  ↖
      \│/
  ←─────────→
      /│\
    ↙  │  ↘

Legend:
6 IR LEDs at 60° intervals
360° / 6 = 60° per sector
```

**Pros:**
- Better angular coverage (60° spacing)
- More uniform power distribution
- Fewer blind spots

**Cons:**
- More complex PCB layout
- Higher component count
- Slightly higher power consumption

---

### Option 3: 8-LED Octagonal Configuration (Maximum Coverage)

**Coverage:** 8 directions (45° spacing)
**Best For:** Large rooms, commercial applications
**Cost:** Higher (~$15)
**Complexity:** Advanced

```
Top View:
    ↑  ↗  →  ↘
    ↓  ↙  ←  ↖

Legend:
8 IR LEDs at 45° intervals
360° / 8 = 45° per sector
Near-perfect omnidirectional
```

**Pros:**
- Excellent omnidirectional coverage
- 45° spacing eliminates most blind spots
- Professional-grade performance

**Cons:**
- More components = higher cost
- Complex driver circuitry
- Higher power requirements

---

### Option 4: Vertical + Horizontal Configuration

**Coverage:** 3D coverage (ceiling, walls, floor)
**Best For:** Multi-floor rooms, loft apartments
**Cost:** Medium (~$13)
**Complexity:** Moderate

```
Side View:              Top View:
    ↑                      N
    │                      ↑
    LED                    │
    │                  W ←─┼─→ E
ESP32 ─→ LED               │
    │                      ↓
    LED                    S
    │
    ↓

4 horizontal + 2 vertical = 6 LEDs total
```

**Pros:**
- 3D coverage for multi-level control
- Good for ceiling/floor-mounted devices
- Covers both horizontal and vertical planes

**Cons:**
- Requires 3D enclosure design
- More complex assembly
- May waste power on unused directions

---

## 🔌 Component Selection

### 1. IR LED Transmitters

#### Recommended: TSAL6200 (High Power)

**Specifications:**
- **Wavelength:** 940nm (invisible to human eye)
- **Viewing Angle:** 20° (focused beam)
- **Peak Power:** 200mW
- **Forward Voltage:** 1.35V typical, 1.5V max
- **Forward Current:** 100mA continuous, 1A peak
- **Range:** Up to 15 meters at 100mA
- **Package:** T-1 3/4 (5mm dome)

**Why TSAL6200:**
✅ High radiant intensity (120 mW/sr)
✅ Fast switching (Rise/Fall: 100ns)
✅ Excellent 38kHz modulation response
✅ Wide availability and low cost (~$0.50 each)

**Alternatives:**
- **TSAL6100** - Lower power (50mW), shorter range, cheaper
- **TSUS5400** - Similar specs to TSAL6200
- **VSLY5940** - Vishay equivalent

#### Budget Option: Standard 5mm 940nm IR LED

**Specifications:**
- **Wavelength:** 940nm
- **Viewing Angle:** 15-25°
- **Power:** 20-50mW
- **Forward Voltage:** 1.2-1.4V
- **Forward Current:** 20mA continuous, 50mA peak
- **Range:** 5-8 meters

**When to Use:**
- Small rooms (<5m)
- Budget constraints
- Low power requirements

---

### 2. IR Receiver Module

#### Recommended: IRM-3638T (38kHz)

**Specifications:**
- **Carrier Frequency:** 38kHz (most common)
- **Operating Voltage:** 2.7V - 5.5V
- **Output:** Active LOW (0V when signal detected)
- **Sensitivity:** High (up to 25m reception)
- **Supply Current:** <1mA
- **Package:** 3-pin inline

**Pin Configuration:**
```
   ┌─────┐
   │  ○  │  ← IR Sensor
   └──┬──┘
   1  2  3
   │  │  │
   │  │  └── GND
   │  └───── VCC (5V)
   └──────── OUT (to ESP32 GPIO 18)
```

**Alternatives:**
- **VS1838B** - Similar specs, slightly different package
- **TSOP38238** - Vishay, very popular
- **IRM-3638N** - 38kHz, different sensitivity

**Why 38kHz:**
- Most TV/AC remotes use 38kHz carrier
- Good balance of range and power
- Wide component availability

---

### 3. Transistor Driver (NPN)

#### Recommended: 2N2222A (General Purpose)

**Specifications:**
- **Type:** NPN switching transistor
- **Max Collector Current:** 800mA (continuous)
- **Max Collector-Emitter Voltage:** 40V
- **DC Current Gain (hFE):** 100-300 (typical 200)
- **Package:** TO-92 (3-pin through-hole)

**Pin Configuration:**
```
   Flat side facing you:
   ┌───────┐
   │ 2N2222│
   └─┬─┬─┬─┘
     1 2 3
     │ │ │
     │ │ └── Collector (to IR LED cathode)
     │ └──── Base (to ESP32 via resistor)
     └────── Emitter (to GND)
```

**Alternatives:**
- **BC547** - Lower current (100mA), smaller package
- **2N3904** - Similar to 2N2222, slightly lower gain
- **PN2222** - Plastic package version of 2N2222

**For High Power (>200mA per LED):**
- **TIP120** - Darlington, up to 5A
- **BD139** - Up to 1.5A
- **2N3055** - Up to 15A (overkill for this application)

---

### 4. Resistors

#### IR LED Current Limiting Resistor

**Calculation:**
```
For TSAL6200 at 100mA:
VCC = 5V
V_LED = 1.35V (typical forward voltage)
V_CE(sat) = 0.2V (transistor saturation voltage)
I_LED = 100mA (desired current)

R = (VCC - V_LED - V_CE(sat)) / I_LED
R = (5 - 1.35 - 0.2) / 0.1
R = 3.45 / 0.1
R = 34.5Ω

Use: 33Ω (nearest standard value)
Power: P = I² × R = 0.1² × 33 = 0.33W
Use: 0.5W or 1W resistor
```

**Recommended Values:**
- **33Ω, 1W** - For 100mA (maximum power, 15m range)
- **47Ω, 0.5W** - For 70mA (good power, 12m range)
- **100Ω, 0.25W** - For 35mA (medium power, 8m range)

#### Base Resistor (for Transistor)

**Calculation:**
```
For 2N2222 driving 100mA load:
V_GPIO = 3.3V (ESP32 output)
V_BE = 0.7V (base-emitter voltage)
I_C = 100mA (collector current)
hFE = 100 (minimum gain from datasheet)

I_B = I_C / hFE = 100mA / 100 = 1mA (minimum)
Use: I_B = 5mA (5× overdrive for saturation)

R_base = (V_GPIO - V_BE) / I_B
R_base = (3.3 - 0.7) / 0.005
R_base = 2.6 / 0.005
R_base = 520Ω

Use: 470Ω or 560Ω (nearest standard values)
Power: P = I² × R = 0.005² × 470 = 0.01W
Use: 0.25W (1/4W) resistor
```

**Recommended Values:**
- **470Ω, 0.25W** - For 5.5mA base current (good saturation)
- **1kΩ, 0.25W** - For 2.6mA base current (minimum saturation)

---

### 5. WS2812B RGB LED (Status Indicator)

**Specifications:**
- **Type:** Addressable RGB LED
- **Voltage:** 5V (with level shifter from ESP32 3.3V)
- **Current:** 60mA max (20mA per color)
- **Protocol:** Single-wire control (800kHz)
- **Package:** 5050 SMD or 8mm through-hole

**Required Components:**
- **470Ω resistor** - Data line protection
- **100nF capacitor** - Power supply decoupling (optional)
- **1000µF capacitor** - Bulk power supply filtering (optional)

---

### 6. Capacitors (Optional but Recommended)

#### Power Supply Filtering

**For Stable Operation:**
- **1000µF, 16V electrolytic** - Bulk power supply smoothing (near power input)
- **100nF, 50V ceramic** - High-frequency decoupling (near ESP32 VCC)
- **100nF, 50V ceramic** - IR receiver decoupling (near receiver VCC)
- **100nF, 50V ceramic** - WS2812B decoupling (near LED VCC)

**When to Use:**
- Long wiring runs (>15cm)
- Noisy power supply
- Multiple IR LEDs switching simultaneously
- Interference issues

---

## ⚡ Circuit Designs

### Design A: Single IR LED Circuit (Basic)

**For:** Testing, prototyping, single-direction applications

```
ESP32 GPIO 17 ──[470Ω]──┬──> 2N2222 Base
                         │
                    2N2222 NPN
                    │ B C E │
                    │   │   │
                    │   │   GND
                    │   │
                    │   └──[33Ω, 1W]──┐
                    │                  │
                    │              IR LED
                    │              (TSAL6200)
                    │              Cathode (-)
                    │                  │
                    │              Anode (+)
                    │                  │
                    └──────────────── 5V
```

**Component List:**
- 1× ESP32 GPIO 17
- 1× 470Ω, 0.25W resistor (base)
- 1× 33Ω, 1W resistor (LED current limiting)
- 1× 2N2222 NPN transistor
- 1× TSAL6200 IR LED

**Notes:**
- Simple and easy to test
- ~100mA current draw
- 15m range in direct line

---

### Design B: 4-LED Square Configuration (Recommended)

**For:** 360° horizontal coverage, standard rooms

```
                    5V Rail
                     │
        ┌────────────┼────────────┐
        │            │            │
     [33Ω,1W]    [33Ω,1W]    [33Ω,1W]    [33Ω,1W]
        │            │            │            │
      LED_N        LED_E        LED_S        LED_W
        │            │            │            │
        C            C            C            C
        │            │            │            │
    ┌───Q1───┐  ┌───Q2───┐  ┌───Q3───┐  ┌───Q4───┐
    │2N2222  │  │2N2222  │  │2N2222  │  │2N2222  │
    B        E  B        E  B        E  B        E
    │        │  │        │  │        │  │        │
    │        GND │        GND │        GND │        GND
    │            │            │            │
   [470Ω]      [470Ω]      [470Ω]      [470Ω]
    │            │            │            │
    │            │            │            │
GPIO17_1    GPIO17_2    GPIO17_3    GPIO17_4
    │            │            │            │
    └────────────┴────────────┴────────────┘
              │
         ESP32 GPIO 17
```

**Connection Strategy:**

**Option 1: Parallel Connection (All LEDs Together)**
- Connect all bases together to single GPIO 17
- All LEDs fire simultaneously
- Simpler wiring
- Higher current draw (4 × 100mA = 400mA)

```c
// Code: Simple, all LEDs on GPIO 17
#define GPIO_IR_TX  17
// All 4 LEDs fire together
```

**Option 2: Individual GPIO Control**
- Each LED on separate GPIO
- Selective LED firing (save power)
- More complex wiring
- Lower peak current (100mA per LED)

```c
// Code: Individual control
#define GPIO_IR_TX_N  17
#define GPIO_IR_TX_E  18
#define GPIO_IR_TX_S  19
#define GPIO_IR_TX_W  21

// Fire specific direction only
```

**Recommended: Option 1 (Parallel)** for simplicity and reliability.

**Component List:**
- 4× TSAL6200 IR LEDs
- 4× 2N2222 NPN transistors
- 4× 33Ω, 1W resistors (LED current limiting)
- 4× 470Ω, 0.25W resistors (base)
- 1× ESP32 GPIO (parallel) or 4× ESP32 GPIOs (individual)

**Power Requirements:**
- Peak: 4 × 100mA = 400mA (during IR transmission)
- Average: ~40mA (10% duty cycle at 38kHz)

---

### Design C: 6-LED Hexagonal Configuration

**For:** Better omnidirectional coverage, ceiling mount

```
                    5V Rail
                     │
    ┌────────┬───────┼───────┬────────┬────────┬────────┐
    │        │       │       │        │        │        │
  [33Ω]   [33Ω]   [33Ω]   [33Ω]    [33Ω]    [33Ω]
    │        │       │       │        │        │
  LED_0°   LED_60° LED_120° LED_180° LED_240° LED_300°
    │        │       │       │        │        │
    C        C       C       C        C        C
    │        │       │       │        │        │
   2N2222  2N2222  2N2222  2N2222   2N2222   2N2222
    │        │       │       │        │        │
    B        B       B       B        B        B
    │        │       │       │        │        │
  [470Ω]  [470Ω]  [470Ω]  [470Ω]   [470Ω]   [470Ω]
    │        │       │       │        │        │
    └────────┴───────┴───────┴────────┴────────┴────────┘
                             │
                        ESP32 GPIO 17
```

**LED Positioning (Top View):**
```
          0° (North)
            ↑
    300° ↗     ↖ 60°

    240° ↙     ↘ 120°
            ↓
          180° (South)
```

**Component List:**
- 6× TSAL6200 IR LEDs (positioned at 60° intervals)
- 6× 2N2222 NPN transistors
- 6× 33Ω, 1W resistors
- 6× 470Ω, 0.25W resistors

**Power Requirements:**
- Peak: 6 × 100mA = 600mA
- Average: ~60mA (10% duty cycle)

---

### Design D: 8-LED Octagonal Configuration (Maximum Coverage)

**For:** Large rooms, commercial applications, perfect 360°

```
Top View LED Arrangement:

        N (0°)
        ↑
   NW ↗   ↖ NE
      \   /
W ←────┼────→ E
      /   \
   SW ↙   ↘ SE
        ↓
        S (180°)

8 LEDs at 45° intervals:
0° (N), 45° (NE), 90° (E), 135° (SE),
180° (S), 225° (SW), 270° (W), 315° (NW)
```

**Circuit (same as 6-LED, but with 8 channels):**

**Component List:**
- 8× TSAL6200 IR LEDs
- 8× 2N2222 NPN transistors
- 8× 33Ω, 1W resistors
- 8× 470Ω, 0.25W resistors

**Power Requirements:**
- Peak: 8 × 100mA = 800mA
- Average: ~80mA (10% duty cycle)

**Power Supply Consideration:**
- ESP32 USB: 500mA max (may not be enough)
- Use external 5V/1.5A power supply
- Share GND between ESP32 and power supply

---

### Design E: High-Power with ULN2803 Darlington Array

**For:** Simplified wiring, higher power, 4-8 LEDs

**Why ULN2803:**
- 8 Darlington transistors in one IC
- No base resistors needed (internal)
- Up to 500mA per channel
- Built-in flyback diodes
- Simple parallel connection

```
         ULN2803A IC
         ┌─────────┐
GPIO17 ──┤1B     1C├──[33Ω]──LED1──5V
GPIO18 ──┤2B     2C├──[33Ω]──LED2──5V
GPIO19 ──┤3B     3C├──[33Ω]──LED3──5V
GPIO21 ──┤4B     4C├──[33Ω]──LED4──5V
GPIO22 ──┤5B     5C├──[33Ω]──LED5──5V
GPIO23 ──┤6B     6C├──[33Ω]──LED6──5V
GPIO25 ──┤7B     7C├──[33Ω]──LED7──5V
GPIO26 ──┤8B     8C├──[33Ω]──LED8──5V
    GND ─┤9  GND   │
         └─────────┘

OR (All LEDs on single GPIO):

         ULN2803A IC
         ┌─────────┐
GPIO17 ──┼─1B    1C├──[33Ω]──LED1──5V
       │ ├─2B    2C├──[33Ω]──LED2──5V
       │ ├─3B    3C├──[33Ω]──LED3──5V
       │ ├─4B    4C├──[33Ω]──LED4──5V
       └─┼─All tied together
    GND ─┤9  GND   │
         └─────────┘
```

**Component List:**
- 1× ULN2803A Darlington array IC (~$0.50)
- 4-8× TSAL6200 IR LEDs
- 4-8× 33Ω, 1W resistors
- No base resistors needed!

**Advantages:**
- Simplified circuit (no discrete transistors)
- Built-in protection
- Cleaner PCB layout
- Lower component count

**Disadvantages:**
- Slightly higher cost (~$0.50 vs ~$0.10 for 2N2222)
- Larger footprint (DIP-18 package)
- Higher saturation voltage (1.6V vs 0.2V)

**Adjusted Current Limiting:**
```
R = (5V - 1.35V - 1.6V) / 0.1A
R = 2.05 / 0.1 = 20.5Ω
Use: 22Ω, 1W
```

---

## 🔌 IR Receiver Circuit

**Single receiver for all learning operations:**

```
IRM-3638T IR Receiver Module
        ┌─────┐
        │  ○  │  ← IR Sensor (faces outward)
        └──┬──┘
        1  2  3
        │  │  │
        │  │  └─── GND ────────────── ESP32 GND
        │  └────── VCC ────────────── 5V (or 3.3V)
        └───────── OUT ──[Optional]── ESP32 GPIO 18
                             │
                             └── 10kΩ pull-up to 3.3V (optional)

Optional Filtering (for noisy environments):
    VCC ──[100nF]── GND (near receiver)
    OUT ──────┬──── GPIO 18
              │
            [10kΩ] (pull-up to 3.3V)
              │
             3.3V
```

**Notes:**
- IRM-3638T output is **active LOW** (0V when receiving IR)
- ESP32 handles this with `flags.invert_in = true` in RMT config
- Pull-up resistor optional (ESP32 has internal pull-ups)
- Add 100nF capacitor for stable operation

**Positioning:**
- Mount receiver facing most common remote direction
- Keep away from IR transmitters (>5cm clearance)
- Can be on opposite side of PCB from transmitters

---

## 📐 PCB Layout Guidelines

### Option 1: Single-Sided PCB (DIY-Friendly)

**Board Size:** 60mm × 60mm
**Layers:** 1 (single-sided copper)
**Thickness:** 1.6mm standard

**Component Placement (Top View):**

```
        ┌──────────────────────────┐
        │                          │
        │       LED_N (↑)          │
        │          Q1              │
        │                          │
LED_W ──┤    ┌──────────┐          ├── LED_E
(←)  Q4│    │   ESP32  │          │Q2  (→)
        │    │  DevKit  │          │
        │    └──────────┘          │
        │          Q3              │
        │       LED_S (↓)          │
        │                          │
        │   [Status LED]           │
        │   [IR Receiver]          │
        │   [USB Power]            │
        └──────────────────────────┘

Legend:
LED_X = IR transmitter LED
QX = 2N2222 transistor + resistors
ESP32 = Development board (mounted with headers)
```

**Trace Widths:**
- Power traces (5V, GND): 1.5mm - 2mm (for 500mA)
- Signal traces (GPIO): 0.5mm - 0.8mm
- Keep IR LED traces short (<3cm)

**Ground Plane:**
- Use copper pour for GND on bottom layer
- Reduces noise and improves stability
- Connect with multiple vias

---

### Option 2: Double-Sided PCB (Professional)

**Board Size:** 50mm × 50mm
**Layers:** 2 (double-sided)
**Thickness:** 1.6mm

**Top Layer:**
- ESP32 module
- IR LEDs (positioned at edges)
- Status LED (WS2812B)
- Connector headers

**Bottom Layer:**
- IR receiver (opposite side from transmitters)
- Transistors and resistors (under ESP32)
- Power regulation (if using onboard 5V regulator)
- Ground plane

**Vias:**
- Use vias to connect top and bottom GND planes
- Place via near each transistor emitter
- Place vias around power pins

---

### PCB Design Tips

1. **LED Positioning:**
   - Place LEDs at board edges for clear IR path
   - Ensure 5mm clearance from board edge (mounting tolerance)
   - Angle LEDs slightly outward (10-15°) for better coverage

2. **Heat Dissipation:**
   - Transistors may get warm at 100mA
   - Add copper pour around transistor pads
   - Consider heatsink pads for high-power applications

3. **Interference Prevention:**
   - Keep IR receiver ≥5cm from IR transmitters
   - Use ground plane between receiver and transmitters
   - Add 100nF capacitor at receiver VCC

4. **Mounting Holes:**
   - Add 4× M3 mounting holes at corners
   - 3mm hole diameter, 6mm keepout area
   - Use non-conductive standoffs

5. **Connector Placement:**
   - USB power connector at edge
   - Programming header accessible
   - Status LED visible from top

---

## 🏭 Assembly Instructions

### Tools Required

- **Soldering iron** (25W-60W with fine tip)
- **Solder** (60/40 or 63/37 rosin core, 0.8mm)
- **Wire cutters** (flush cut)
- **Needle-nose pliers**
- **Multimeter** (for testing)
- **Solder wick** or desoldering pump
- **Helping hands** or PCB holder
- **Wire strippers** (for connections)

### Components Assembly Order

**Step 1: Resistors (Lowest Profile)**
1. Solder all 470Ω base resistors
2. Solder all 33Ω LED current limiting resistors
3. Trim excess leads flush

**Step 2: Transistors**
1. Identify pin orientation (flat side facing specific direction)
2. Insert transistors (do NOT bend leads excessively)
3. Solder from bottom
4. Trim excess leads

**Step 3: IR LEDs**
1. **IMPORTANT:** Identify polarity
   - **Cathode (-):** Shorter lead, flat side of dome
   - **Anode (+):** Longer lead
2. Insert LEDs facing OUTWARD at correct angles
3. Solder anode to 5V rail
4. Solder cathode to current limiting resistor
5. Do NOT trim leads yet (for angle adjustment)

**Step 4: IR Receiver Module**
1. Insert IRM-3638T with sensor facing OUTWARD
2. Connect to VCC, GND, OUT pads
3. Solder carefully (sensor window should be clear)

**Step 5: WS2812B RGB LED**
1. Identify GND, VCC, DIN, DOUT pins
2. Insert with correct orientation (check datasheet)
3. Solder all 4 pins
4. Add 470Ω resistor on DIN line

**Step 6: ESP32 Module**
1. Insert ESP32 development board into headers
2. Solder GPIO 17, 18, 22, 5V, GND connections
3. Verify pin connections with multimeter

**Step 7: Capacitors (Optional)**
1. Solder 100nF ceramic caps near VCC pins
2. Solder 1000µF electrolytic near power input (observe polarity!)

**Step 8: Power Connector**
1. Solder USB connector or barrel jack
2. Connect 5V and GND to board rails
3. Add fuse or polyfuse for protection (optional)

---

### LED Angle Adjustment

**For Maximum Coverage:**

1. **4-LED Configuration:**
   - Bend each LED 45° outward from center
   - North LED: points 0°
   - East LED: points 90°
   - South LED: points 180°
   - West LED: points 270°

```
Top View (before bending):
    │
    LED (vertical)

Side View (after bending 45°):
    /
   LED (angled outward)

Result: LEDs point at horizon, not up/down
```

2. **6-LED Configuration:**
   - Position at 60° intervals
   - Bend each 30° outward
   - Creates dome-like coverage

3. **8-LED Configuration:**
   - Position at 45° intervals
   - Bend each 20-30° outward
   - Near-perfect spherical coverage

**Adjustment Procedure:**
1. Insert all LEDs but don't solder yet
2. Mark desired angles on workbench or protractor
3. Bend each LED gently to angle
4. Verify angle with protractor
5. Solder in place
6. Trim leads AFTER angle is correct

---

## 🧪 Testing & Calibration

### Pre-Power Tests

**Before applying power:**

1. **Visual Inspection:**
   - Check for solder bridges (shorts)
   - Verify component orientation (transistors, LEDs, capacitors)
   - Ensure no bare wires touching

2. **Continuity Tests:**
   - GND to all emitters (should be continuous)
   - 5V to all LED anodes (should NOT be continuous to GND)
   - GPIO to each base resistor

3. **Resistance Tests:**
   - Measure base resistors: ~470Ω ± 5%
   - Measure LED resistors: ~33Ω ± 5%
   - Check for shorts: 5V to GND should be >1kΩ

---

### Power-Up Tests

**Step 1: Power Supply Test (without ESP32)**
1. Apply 5V to VCC and GND
2. Measure voltage at all LED anodes: should be 5V ± 0.1V
3. If incorrect: Check power connections, shorts

**Step 2: LED Forward Voltage Test**
1. Manually pull GPIO 17 HIGH (3.3V) using wire
2. Measure voltage across each LED:
   - Anode to cathode: ~1.35V (on)
   - If 0V: LED backwards or transistor not conducting
   - If 5V: Transistor not conducting or open circuit
3. Check current: Should be ~100mA per LED

**Step 3: ESP32 Test**
1. Flash firmware to ESP32
2. Open serial monitor
3. Enter command: `transmit 0`
4. Check LED status:
   - Should flash CYAN (transmitting)
   - Serial log: "Transmitting button 0"

**Step 4: IR Transmission Test**
1. **Use smartphone camera:**
   - Open camera app
   - Point at IR LED
   - Trigger transmission
   - You should see LED FLASHING on camera (IR appears as purple/white)
   - If not visible: LED not working or not transmitting

2. **Use IR receiver:**
   - Point another IR receiver at transmitter
   - Trigger transmission
   - Should detect 38kHz carrier

**Step 5: Range Test**
1. Position device 1m from TV/AC
2. Learn IR code from original remote
3. Transmit learned code
4. Verify device responds
5. Increase distance: 2m, 5m, 10m, 15m
6. Note maximum reliable range

**Step 6: Directional Coverage Test**
```
Setup:
          Device
         (Center)
            │
    ┌───────┼───────┐
    │       │       │
   5m      5m      5m
    │       │       │
   TV1     TV2     TV3

Test all 4/6/8 directions to verify coverage
```

---

### Calibration

**Current Adjustment:**

If range is insufficient or LEDs too dim:

1. **Measure actual current:**
   - Insert multimeter in series with LED
   - Target: 100mA

2. **Adjust current limiting resistor:**
   - Current too low (<80mA): Use lower resistor (e.g., 22Ω)
   - Current too high (>120mA): Use higher resistor (e.g., 47Ω)

**Angle Adjustment:**

If coverage gaps detected:

1. **Identify dead zones:**
   - Place device in room center
   - Test transmission to all corners
   - Mark areas where signal fails

2. **Adjust LED angles:**
   - Reheat solder on LED leads
   - Bend LED toward dead zone
   - Re-test coverage

---

## 📦 Enclosure Design

### Design Requirements

**Protection:**
- Dust and moisture resistance (IP40 minimum)
- Shock protection for PCB and LEDs
- Ventilation for heat dissipation

**Visibility:**
- Clear windows for IR LEDs
- Visible status LED
- Clear labeling

**Mounting:**
- Wall mount brackets
- Desk stand option
- Cable management

---

### Enclosure Option 1: 3D Printed Case

**Material:** PLA or PETG
**Wall Thickness:** 2mm
**Dimensions:** 80mm × 80mm × 40mm (for 4-LED)

**Features:**
```
Top View:
┌─────────────────────┐
│  ○   [LED]   ○     │  ○ = IR LED window (10mm Ø)
│                     │
│     [ESP32]        │
│                     │
│  ○           ○     │
│     [USB]          │
└─────────────────────┘

Side View:
┌─────────────────────┐
│                     │ ← Lid (snap-fit or screws)
│  ┌───────────┐     │
│  │   PCB     │     │ ← PCB standoffs (M3)
│  └───────────┘     │
│     ╱╱╱╱╱╱╱╱╱      │ ← Ventilation slots
└─────────────────────┘
```

**3D Print Files Needed:**
1. **Bottom case** - Main body with standoffs
2. **Top lid** - Removable for access
3. **IR LED bezels** - 4-8× clear acrylic or PC (optional)
4. **Wall mount** - Bracket with keyhole slots

**STL Design Tips:**
- 0.2mm layer height for smooth finish
- 20% infill sufficient
- Use clear filament for LED windows
- Post-process: Sand and polish windows

---

### Enclosure Option 2: Plastic Project Box

**Recommended:** Hammond 1591XXSBK (80mm × 80mm × 40mm)

**Modifications:**
1. **Drill holes for IR LEDs:**
   - 5mm diameter holes at 45° angles
   - 4-8 holes depending on configuration
   - Countersink for flush LED mount

2. **Drill hole for IR receiver:**
   - 5mm diameter on opposite side
   - Clear line of sight

3. **Drill hole for status LED:**
   - 8mm diameter on top
   - Use hot glue for WS2812B window

4. **Cut USB access:**
   - 10mm × 5mm rectangular slot
   - File smooth edges

5. **Add ventilation:**
   - 3mm holes in bottom (×6-10)
   - Covered with mesh (optional)

**Mounting:**
- Use 3M Command strips for wall mount
- Or add rubber feet for desk placement

---

### Enclosure Option 3: Custom Acrylic Case (Laser Cut)

**Material:** 3mm clear or frosted acrylic
**Cutting:** Laser cutter (or CNC router)

**Layers (from bottom to top):**
1. **Bottom plate** - Solid, mounting holes
2. **Spacer 1** - 10mm height, PCB clearance
3. **Middle plate** - Cut-outs for LEDs and components
4. **Spacer 2** - 15mm height, LED clearance
5. **Top plate** - Clear or frosted, status LED window

**Assembly:**
- Use M3 × 30mm screws + nuts
- 10mm nylon standoffs between layers
- Creates "floating" LED effect (looks professional!)

**Advantages:**
- Very professional appearance
- Easy to modify design
- Good ventilation
- Shows internal components (educational/decorative)

**Disadvantages:**
- Requires laser cutter access
- More expensive than 3D print
- Fragile (acrylic can crack)

---

## 💰 Bill of Materials (BOM)

### BOM: 4-LED Configuration (Recommended)

| Component | Description | Qty | Unit Price | Total | Source |
|-----------|-------------|-----|------------|-------|--------|
| **Main Components** |
| ESP32-DevKitC | ESP32-WROOM-32 development board | 1 | $4.00 | $4.00 | AliExpress |
| TSAL6200 | 940nm high-power IR LED | 4 | $0.50 | $2.00 | Digi-Key |
| IRM-3638T | 38kHz IR receiver module | 1 | $0.80 | $0.80 | Digi-Key |
| WS2812B | Addressable RGB LED (5050) | 1 | $0.30 | $0.30 | AliExpress |
| **Transistors** |
| 2N2222A | NPN transistor TO-92 | 4 | $0.10 | $0.40 | Digi-Key |
| **Resistors** |
| 33Ω, 1W | Metal film resistor | 4 | $0.05 | $0.20 | Digi-Key |
| 470Ω, 0.25W | Metal film resistor | 5 | $0.02 | $0.10 | Digi-Key |
| **Capacitors (Optional)** |
| 1000µF, 16V | Electrolytic capacitor | 1 | $0.30 | $0.30 | Digi-Key |
| 100nF, 50V | Ceramic capacitor | 4 | $0.05 | $0.20 | Digi-Key |
| **PCB & Hardware** |
| PCB | Custom PCB (60×60mm, 1.6mm) | 1 | $2.00 | $2.00 | JLCPCB |
| Headers | Pin headers 2.54mm pitch | 2 | $0.20 | $0.40 | AliExpress |
| Standoffs | M3 × 10mm nylon standoffs | 4 | $0.10 | $0.40 | AliExpress |
| Screws | M3 × 6mm screws | 8 | $0.02 | $0.16 | AliExpress |
| **Enclosure** |
| Project box | Hammond 1591XXSBK or 3D print | 1 | $3.00 | $3.00 | Amazon |
| **Cables & Connectors** |
| USB cable | Micro-USB or USB-C cable | 1 | $1.00 | $1.00 | Included |
| **TOTAL** | | | | **$15.26** | |

**Notes:**
- Prices are approximate (2024)
- Bulk discounts available for qty >10
- Shipping not included
- Alternative sources: Mouser, Arrow, LCSC

---

### BOM: 6-LED Configuration

**Additional Components (vs 4-LED):**
- +2× TSAL6200 IR LEDs: +$1.00
- +2× 2N2222 transistors: +$0.20
- +2× 33Ω resistors: +$0.10
- +2× 470Ω resistors: +$0.04
- Larger PCB (70×70mm): +$0.50

**Total for 6-LED:** ~$17.10

---

### BOM: 8-LED Configuration

**Additional Components (vs 4-LED):**
- +4× TSAL6200 IR LEDs: +$2.00
- +4× 2N2222 transistors: +$0.40
- +4× 33Ω resistors: +$0.20
- +4× 470Ω resistors: +$0.08
- Larger PCB (80×80mm): +$1.00
- External 5V/1.5A power supply: +$3.00 (USB not enough)

**Total for 8-LED:** ~$21.94

---

### BOM: Budget Version (2-LED)

**For small rooms, budget constraints:**

| Component | Qty | Total |
|-----------|-----|-------|
| ESP32-DevKitC | 1 | $4.00 |
| Generic 940nm LED | 2 | $0.20 |
| IRM-3638T receiver | 1 | $0.80 |
| 2N2222 transistor | 2 | $0.20 |
| Resistors | 6 | $0.30 |
| PCB (40×40mm) | 1 | $1.00 |
| Perfboard case | 1 | $1.00 |
| **TOTAL** | | **$7.50** |

**Trade-offs:**
- Only 2 directions (180° coverage)
- Shorter range (~5-8m)
- No status LED
- DIY perfboard construction

---

## 🔧 Troubleshooting

### Issue 1: IR LEDs Not Transmitting

**Symptoms:**
- LED doesn't flash on camera
- Device doesn't respond to commands
- Serial log shows "Transmitting" but no IR output

**Diagnostic Steps:**
1. **Check LED polarity:**
   - Measure voltage across LED during transmission
   - Should be ~1.35V (anode positive, cathode negative)
   - If reversed: Desolder and flip LED

2. **Check transistor:**
   - Measure base voltage: Should be ~0.7V when GPIO HIGH
   - Measure collector-emitter voltage: Should be ~0.2V when on
   - If >1V: Transistor not saturating (increase base current)

3. **Check current:**
   - Measure current through LED: Should be ~100mA
   - Too low: Reduce current limiting resistor
   - Too high: Increase resistor or check power supply

4. **Check wiring:**
   - Verify GPIO 17 connection to base resistor
   - Check ground continuity
   - Verify 5V power to LED anode

---

### Issue 2: Short Range (<5m)

**Symptoms:**
- IR transmission only works close to device (<5m)
- Needs multiple attempts to control device

**Solutions:**
1. **Increase LED current:**
   - Measure current (should be 80-100mA)
   - Reduce resistor from 33Ω to 22Ω
   - Re-test range

2. **Check LED angle:**
   - Ensure LED points directly at target
   - Adjust angle if needed
   - Add reflector behind LED (aluminum foil)

3. **Clean LED lens:**
   - Dust/dirt can reduce output
   - Wipe with alcohol and microfiber cloth

4. **Check power supply:**
   - Weak power supply can't deliver 100mA × N LEDs
   - Use quality 5V/1A+ adapter
   - Add 1000µF capacitor near LEDs

5. **Use higher power LED:**
   - Replace with TSAL6400 (higher output)
   - Or add second LED in parallel

---

### Issue 3: IR Learning Fails

**Symptoms:**
- Receiver doesn't detect IR signals
- Serial log: "Learning timeout" or "No signal detected"

**Solutions:**
1. **Check receiver orientation:**
   - Sensor dome must face outward
   - Verify with multimeter (OUT pin goes LOW when IR detected)

2. **Check receiver power:**
   - Measure VCC: Should be 5V (or 3.3V if wired to 3.3V)
   - Add 100nF decoupling capacitor

3. **Check receiver placement:**
   - Keep ≥5cm away from IR transmitters
   - Shield from transmitter light if needed

4. **Test receiver manually:**
   - Point TV remote at receiver
   - Press any button
   - OUT pin should pulse LOW (measure with multimeter or scope)

5. **Check firmware RMT config:**
   - Ensure `flags.invert_in = true` (IRM-3638T is active-LOW)
   - Check GPIO 18 assignment

6. **Replace remote batteries:**
   - Most common issue!
   - Weak batteries = weak IR signal

---

### Issue 4: Interference / Erratic Behavior

**Symptoms:**
- Random LED flashing
- Unintended IR transmissions
- ESP32 reboots during transmission

**Solutions:**
1. **Add bulk capacitor:**
   - 1000µF near power input
   - Smooths voltage spikes

2. **Add decoupling capacitors:**
   - 100nF near ESP32 VCC pin
   - 100nF near each IR LED

3. **Separate power supplies:**
   - Use dedicated 5V supply for IR LEDs
   - Share GND only

4. **Check ground loops:**
   - Ensure single ground connection point
   - Use star grounding topology

5. **Add ferrite beads:**
   - On USB cable near connector
   - Reduces high-frequency noise

---

### Issue 5: LED Doesn't Show Correct Colors

**Symptoms:**
- WS2812B shows wrong colors or doesn't light
- Colors are dim or flickering

**Solutions:**
1. **Check wiring:**
   - DIN to GPIO 22 (via 470Ω resistor)
   - VCC to 5V (WS2812B requires 5V, not 3.3V)
   - GND to GND

2. **Check signal level:**
   - WS2812B expects 5V logic
   - ESP32 outputs 3.3V
   - May need level shifter (74HCT245 or similar)
   - Or use WS2812B-3V variant

3. **Check timing:**
   - RMT configuration must match WS2812B timing
   - 800kHz data rate
   - GRB color order (not RGB!)

4. **Add capacitor:**
   - 100nF across VCC/GND of WS2812B
   - 1000µF bulk capacitor on power rail

---

### Issue 6: Device Won't Power On

**Symptoms:**
- No LED, no USB detection, completely dead

**Solutions:**
1. **Check power supply:**
   - Use known-good USB cable (data-capable)
   - Try different USB port or adapter
   - Verify 5V at ESP32 VIN pin

2. **Check for shorts:**
   - Disconnect all external components
   - Measure resistance: 5V to GND should be >1kΩ
   - Look for solder bridges under microscope

3. **Check ESP32:**
   - ESP32 may be damaged
   - Try flashing with esptool
   - If fails: Replace ESP32 module

---

## 📚 Additional Resources

### Datasheets
- **TSAL6200:** [Vishay TSAL6200 Datasheet](https://www.vishay.com/docs/81010/tsal6200.pdf)
- **IRM-3638T:** [Everlight IRM-3638T Datasheet](https://www.everlight.com/file/ProductFile/IRM-3638T.pdf)
- **2N2222:** [ON Semi 2N2222 Datasheet](https://www.onsemi.com/pdf/datasheet/p2n2222a-d.pdf)
- **WS2812B:** [WorldSemi WS2812B Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)

### PCB Design Tools
- **KiCad** - Free, open-source PCB design (https://kicad.org/)
- **EasyEDA** - Free, web-based (https://easyeda.com/)
- **Eagle** - Autodesk, free for hobbyists (https://www.autodesk.com/products/eagle)

### PCB Manufacturers
- **JLCPCB** - $2 for 5× 100×100mm PCBs (https://jlcpcb.com/)
- **PCBWay** - Similar pricing (https://www.pcbway.com/)
- **OSH Park** - USA-based, higher quality (https://oshpark.com/)

### 3D Printing
- **Thingiverse** - Free STL files (https://www.thingiverse.com/)
- **Printables** - Prusa's model library (https://www.printables.com/)
- **TinkerCAD** - Free online CAD (https://www.tinkercad.com/)

### Tutorials
- **IR Protocol Analysis:** [SB-Projects IR Protocols](https://www.sbprojects.net/knowledge/ir/index.php)
- **RMT Peripheral:** [ESP-IDF RMT Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/rmt.html)
- **PCB Design Basics:** [SparkFun PCB Tutorial](https://learn.sparkfun.com/tutorials/beginners-guide-to-kicad)

---

## ✅ Summary & Recommendations

### Recommended Configuration for Most Users

**4-LED Square Configuration:**
- ✅ Good 360° horizontal coverage
- ✅ Simple to build
- ✅ Low cost (~$15)
- ✅ 400mA power draw (USB-powered)
- ✅ Suitable for most room sizes

**Components:**
- 4× TSAL6200 IR LEDs (940nm, high-power)
- 4× 2N2222 NPN transistors
- 4× 33Ω, 1W resistors (LED current limiting)
- 4× 470Ω, 0.25W resistors (base)
- 1× IRM-3638T IR receiver
- 1× WS2812B RGB status LED
- 1× ESP32-DevKitC development board

**Assembly:**
- Single-sided PCB (60×60mm)
- 3D printed or plastic project box enclosure
- USB-powered (no external supply needed)
- DIY-friendly, beginner-level soldering

**Performance:**
- 10-15m range per direction
- Full 360° horizontal coverage
- Reliable NEC/Samsung/RAW protocol support
- Battery-free (always powered)

---

### For Advanced Users

**8-LED Octagonal Configuration:**
- Maximum 360° coverage (45° spacing)
- Professional-grade performance
- Higher cost (~$22)
- Requires external 5V/1.5A power supply
- Best for large rooms or commercial use

---

### For Budget Builds

**2-LED Configuration:**
- Front + back coverage (180°)
- ~$8 total cost
- Perfboard construction
- 5-8m range
- Ideal for dorm rooms or small apartments

---

**Ready to build? Start with the 4-LED configuration and expand later if needed!**

**Good luck with your hardware design! 🛠️✨**

---

**Document Version:** 1.0.0
**Last Updated:** December 24, 2024
**Author:** Sai Automations
**License:** MIT
