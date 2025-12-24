# Universal IR Remote - Circuit Schematics
## Complete Wiring Diagrams and Schematics

**Version:** 1.0.0
**Date:** December 24, 2024

---

## 📐 Complete System Schematic (4-LED Configuration)

### Full Circuit Diagram

```
                                    +5V Power Rail
                                         │
        ┌────────────────────────────────┼────────────────────────────────┐
        │                                │                                │
        │                                │                                │
    [33Ω,1W]                         [33Ω,1W]                         [33Ω,1W]
        │                                │                                │
      ┌─┴─┐                            ┌─┴─┐                            ┌─┴─┐
      │LED│ TSAL6200                   │LED│ TSAL6200                   │LED│ TSAL6200
      │ N │ (North)                    │ E │ (East)                     │ S │ (South)
      └─┬─┘                            └─┬─┘                            └─┬─┘
        │                                │                                │
        C                                C                                C
        │                                │                                │
    ┌───┴───┐                        ┌───┴───┐                        ┌───┴───┐
    │2N2222 │                        │2N2222 │                        │2N2222 │
    │  Q1   │                        │  Q2   │                        │  Q3   │
    B───┬───E                        B───┬───E                        B───┬───E
        │   │                            │   │                            │   │
     [470Ω] GND                       [470Ω] GND                       [470Ω] GND
        │                                │                                │
        │                                │                                │
     ESP32                            ESP32                            ESP32
    GPIO17────────────────────────────GPIO17───────────────────────────GPIO17
                                         │
                                         │
                                    [33Ω,1W]
                                         │
                                       ┌─┴─┐
                                       │LED│ TSAL6200
                                       │ W │ (West)
                                       └─┬─┘
                                         │
                                         C
                                         │
                                     ┌───┴───┐
                                     │2N2222 │
                                     │  Q4   │
                                     B───┬───E
                                         │   │
                                      [470Ω] GND
                                         │
                                      ESP32
                                     GPIO17
```

### IR Receiver Circuit

```
        IRM-3638T IR Receiver
              ┌─────┐
              │  ○  │  ← IR Sensor Window (faces outward)
              └──┬──┘
              1  2  3
              │  │  │
     (Signal) │  │  └─── 3: GND ────────────── ESP32 GND
              │  │
     (Power)  │  └────── 2: VCC ────────────── +5V (or 3.3V)
              │
              └───────── 1: OUT ──┬──────────── ESP32 GPIO 18
                                  │
                              [10kΩ] (optional pull-up)
                                  │
                                 +3.3V

Optional Noise Filtering:
     VCC (Pin 2) ──┬──[100nF]──┬── GND
                   │           │
                 IRM-3638T  Ceramic Cap
```

### WS2812B RGB LED Circuit

```
        WS2812B RGB Status LED
              ┌─────┐
              │ RGB │
              │ LED │
              └──┬──┘
              1  2  3  4
              │  │  │  │
              │  │  │  └─── 4: DIN ──[470Ω]── ESP32 GPIO 22
              │  │  │
              │  │  └────── 3: GND ────────── ESP32 GND
              │  │
              │  └───────── 2: VCC ────────── +5V
              │
              └──────────── 1: DOUT (not connected)

Optional Power Filtering:
     VCC (Pin 2) ──┬──[100nF]──┬── GND (high-freq decoupling)
                   │           │
                +5V ──[1000µF]─┴── GND (bulk capacitor)
```

### Power Distribution

```
                    USB 5V Input
                         │
                    ┌────┴────┐
                    │ Fuse/   │ (Optional: 1A polyfuse)
                    │ Switch  │
                    └────┬────┘
                         │
                    +5V Rail ──────┬────────┬────────┬────────┬────────┐
                         │         │        │        │        │        │
                         │         │        │        │        │        │
                    ESP32 VIN   LED_N    LED_E    LED_S    LED_W   WS2812B
                                 Anode    Anode    Anode    Anode    VCC
                         │
                    GND Rail ──────┬────────┬────────┬────────┬────────┐
                         │         │        │        │        │        │
                    ESP32 GND   Q1_E     Q2_E     Q3_E     Q4_E    WS2812B
                                                                      GND
```

---

## 🔌 Individual LED Driver Circuit (Detailed)

### Single LED Driver Module

```
   ESP32                                                      +5V
   GPIO17 ─────┬──────────────────────────────────────────────┘
               │
            [470Ω]    R_base (Base Resistor)
            0.25W     Limits base current to ~5mA
               │      Ensures transistor saturation
               │
               B ←─── Base (control input)
               │
           ┌───┴───┐
           │2N2222 │  NPN Transistor (switching)
           │  NPN  │  • Collector current: up to 800mA
           │       │  • Gain (hFE): 100-300 (typical 200)
           C───┬───E  • Saturation voltage: ~0.2V
               │   │
               │   GND ← Emitter (connects to ground)
               │
            [33Ω]     R_led (Current Limiting Resistor)
             1W       Sets LED current to ~100mA
               │      Power dissipation: ~0.33W
               │
         Cathode (─)
               │
           ┌───┴───┐
           │ IR LED│  TSAL6200 High-Power IR LED
           │TSAL   │  • Wavelength: 940nm
           │6200   │  • Forward voltage: 1.35V @ 100mA
           └───┬───┘  • Viewing angle: 20°
               │      • Radiant intensity: 120 mW/sr
         Anode (+)
               │
              +5V ← Power supply
```

### Voltage & Current Analysis

```
Analysis at 100mA LED current:

V_supply = 5V (USB power)
V_LED = 1.35V (TSAL6200 forward voltage at 100mA)
V_CE(sat) = 0.2V (2N2222 saturation voltage)
I_LED = 100mA (target LED current)

Voltage drop across R_led:
V_R = V_supply - V_LED - V_CE(sat)
V_R = 5V - 1.35V - 0.2V = 3.45V

Required resistance:
R_led = V_R / I_LED
R_led = 3.45V / 0.1A = 34.5Ω
Use: 33Ω (nearest standard E12 value)

Actual current with 33Ω:
I_LED = 3.45V / 33Ω = 104.5mA (acceptable, within spec)

Power dissipation in R_led:
P_R = V_R × I_LED
P_R = 3.45V × 0.104A = 0.36W
Use: 1W resistor (safety margin 2.7×)

Base current calculation:
V_GPIO = 3.3V (ESP32 HIGH output)
V_BE = 0.7V (base-emitter junction voltage)
I_B_min = I_C / hFE = 100mA / 100 = 1mA (minimum for saturation)
I_B_actual = 5mA (use 5× overdrive for hard saturation)

R_base = (V_GPIO - V_BE) / I_B
R_base = (3.3V - 0.7V) / 5mA = 520Ω
Use: 470Ω (nearest standard E12 value)

Actual base current with 470Ω:
I_B = (3.3V - 0.7V) / 470Ω = 5.5mA (excellent for saturation)
```

---

## 🔧 4-LED Parallel Configuration (Simplified)

### All LEDs on Single GPIO

```
                              +5V Power Rail
                                   │
        ┌──────────────────┬────────┼────────┬──────────────────┐
        │                  │        │        │                  │
     [33Ω,1W]          [33Ω,1W] [33Ω,1W] [33Ω,1W]
        │                  │        │        │
      LED_N              LED_E    LED_S    LED_W
        │                  │        │        │
        C                  C        C        C
        │                  │        │        │
    2N2222_Q1          2N2222_Q2  Q3       Q4
        │                  │        │        │
        B                  B        B        B
        │                  │        │        │
     [470Ω]             [470Ω]  [470Ω]  [470Ω]
        │                  │        │        │
        └──────────────────┴────────┴────────┴─────────┐
                                                        │
                                                   ESP32 GPIO 17
                                                        │
        Emitters: E ──────────────────────────── GND ──┘
```

**Advantages:**
- Single GPIO control
- All LEDs fire simultaneously
- Simpler firmware (no GPIO multiplexing)
- More reliable (fewer connections)

**Disadvantages:**
- Higher peak current (4 × 100mA = 400mA)
- No directional control (can't select specific LED)
- Slightly higher power consumption

**Current Draw:**
- Peak: 400mA (during 38kHz pulses)
- Average: ~40mA (10% duty cycle)
- USB can handle this (500mA max)

---

## 📊 Alternative Configurations

### Configuration 1: ULN2803 Darlington Array

```
                    ULN2803A IC (Darlington Array)
                         DIP-18 Package
                    ┌─────────────────────┐
                    │  1B  2B  3B  4B  5B │ ← Inputs (connect to ESP32 GPIOs)
     ESP32 GPIO17 ──┤1                    │
     ESP32 GPIO18 ──┤2                    │
     ESP32 GPIO19 ──┤3        ULN2803A    │
     ESP32 GPIO21 ──┤4                    │
                    │                     │
                    │  1C  2C  3C  4C  5C │ ← Outputs (connect to LED cathodes)
             +5V ───┤  │   │   │   │     │
              │     │  │   │   │   │     │
              │     └──┼───┼───┼───┼─────┘
              │        │   │   │   │
            [22Ω]   [22Ω][22Ω][22Ω] (Lower resistor due to higher V_CE(sat))
              │        │   │   │   │
            LED_1    LED_2 LED_3 LED_4 (Cathodes)
              │        │   │   │   │
            (Anodes connected to +5V)

                    │
               GND ─┤9
                    └─ Pin 9: Common GND
                       Pin 10: Common (connect to +5V for flyback protection)
```

**Key Differences from 2N2222:**
- **V_CE(sat):** 1.6V (vs 0.2V for 2N2222)
- **No base resistors needed** (internal current limiting)
- **Built-in flyback diodes** (for inductive loads)
- **8 channels** (support up to 8 LEDs easily)

**Resistor Recalculation:**
```
R_led = (V_supply - V_LED - V_CE(sat)) / I_LED
R_led = (5V - 1.35V - 1.6V) / 0.1A
R_led = 2.05V / 0.1A = 20.5Ω
Use: 22Ω (nearest standard)
```

---

### Configuration 2: MOSFET Driver (Low Loss)

```
                                                +5V
                                                 │
                                              [33Ω,1W]
                                                 │
                                               LED Anode (+)
                                                 │
                                               Cathode (─)
                                                 │
                                                 D (Drain)
                                                 │
                                             ┌───┴───┐
     ESP32 GPIO17 ──[10kΩ]──────────────────┤ MOSFET│  IRLZ44N (N-channel)
                                         G   │ Logic │  • V_GS(th): 1-2V (logic-level)
                                  (Gate)     │ Level │  • R_DS(on): 0.022Ω @ 5V V_GS
                                             └───┬───┘  • I_D(max): 47A
                                                 │      • Very low voltage drop
                                                 S (Source)
                                                 │
                                                GND

Optional Gate Resistor:
     ESP32 GPIO17 ──[100Ω]──[10kΩ to GND]── Gate
                       │
                    (Limits inrush current)
```

**Advantages of MOSFET:**
- **Ultra-low voltage drop** (0.022Ω × 0.1A = 2.2mV vs 200mV for BJT)
- **No base current** (voltage-driven, not current-driven)
- **Higher efficiency** (less heat)
- **Faster switching** (important for 38kHz modulation)

**Disadvantages:**
- **More expensive** (~$0.60 vs $0.10 for 2N2222)
- **Requires logic-level MOSFET** (standard MOSFETs need 10V+ gate voltage)
- **Larger package** (TO-220 vs TO-92)

**Recommended MOSFET:**
- **IRLZ44N** - Logic-level N-channel, 47A, 0.022Ω, $0.60
- **2N7000** - Small signal, 200mA, 5Ω, $0.15 (for single LED)
- **AO3400** - SOT-23 SMD, 5.8A, 0.027Ω, $0.10 (for PCB)

---

## 🎯 LED Positioning Diagrams

### 4-LED Square Configuration (Top View)

```
        PCB Board (60mm × 60mm)
    ┌───────────────────────────────┐
    │                               │
    │         LED_N (↑ 0°)          │
    │            ○                  │
    │                               │
    │                               │
    │  LED_W        ESP32        LED_E
    │   (← 270°)   ┌─────┐      (→ 90°)
    │      ○       │     │        ○
    │              │WROOM│          │
    │              │ 32  │          │
    │              └─────┘          │
    │                               │
    │            ○                  │
    │         LED_S (↓ 180°)        │
    │                               │
    │   [WS2812B]   [Receiver]      │
    │      ○            ○           │
    └───────────────────────────────┘

LED Angles (Side View):
      ╱  ← 45° outward bend
     ○ LED (bent toward horizon)
     │
    PCB

Coverage per LED: ~90° horizontal
Total coverage: 360° (with overlap)
```

---

### 6-LED Hexagonal Configuration (Top View)

```
        PCB Board (70mm × 70mm)
    ┌───────────────────────────────┐
    │                               │
    │       LED_0° (N)              │
    │           ○                   │
    │     ○           ○             │
    │  LED_300°      LED_60°        │
    │                               │
    │         ┌─────┐               │
    │         │ESP32│               │
    │         │WROOM│               │
    │         │ 32  │               │
    │         └─────┘               │
    │                               │
    │  LED_240°      LED_120°       │
    │     ○           ○             │
    │           ○                   │
    │       LED_180° (S)            │
    │                               │
    └───────────────────────────────┘

Angular spacing: 360° / 6 = 60° per sector
Coverage per LED: ~70° horizontal (with overlap)
Better coverage uniformity than 4-LED
```

---

### 8-LED Octagonal Configuration (Top View)

```
        PCB Board (80mm × 80mm)
    ┌───────────────────────────────┐
    │           ○ 0° (N)            │
    │                               │
    │     ○               ○         │
    │   315°             45°        │
    │                               │
    │ ○       ┌─────┐       ○       │
    │270°     │ESP32│      90° (E)  │
    │  (W)    │WROOM│               │
    │         │ 32  │               │
    │         └─────┘               │
    │ ○                       ○     │
    │225°                    135°   │
    │                               │
    │     ○               ○         │
    │           ○ 180° (S)          │
    └───────────────────────────────┘

Angular spacing: 360° / 8 = 45° per sector
Coverage per LED: ~60° horizontal (with overlap)
Near-perfect omnidirectional coverage
Maximum performance configuration
```

---

## 🔋 Power Supply Considerations

### USB Power Budget

```
Standard USB 2.0:
    Maximum current: 500mA
    Voltage: 5V ± 0.25V (4.75V - 5.25V)

Current Budget Breakdown (4-LED config):

Component                Current (Peak)    Current (Avg)
─────────────────────────────────────────────────────
ESP32 WiFi active        160mA             80mA
IR LED × 4 @ 100mA       400mA (pulsed)    40mA (10% duty)
WS2812B RGB LED          60mA              20mA (typical)
IR Receiver              1mA               1mA
─────────────────────────────────────────────────────
TOTAL                    621mA (peak)      141mA (average)

Peak exceeds USB limit!
```

**Solution Options:**

**Option 1: Reduce LED Current** (Recommended)
```
Use 47Ω instead of 33Ω resistors:
I_LED = (5V - 1.35V - 0.2V) / 47Ω = 73mA
Total LED current: 4 × 73mA = 292mA (peak)
Total system: 292mA + 160mA + 60mA + 1mA = 513mA (peak)

Still slightly over, but within tolerance
Range reduced to ~12m (from 15m)
```

**Option 2: Pulse LEDs Sequentially**
```
Fire LEDs one at a time, 90° at a time:
Peak current: 1 × 100mA + 160mA + 60mA = 320mA
Well within USB limit
Requires firmware modification
Slight delay between directions (~5ms)
```

**Option 3: External Power Supply** (Best for 6-8 LEDs)
```
Use 5V/1.5A wall adapter:
    Sufficient for 8 LEDs @ 100mA = 800mA
    Margin for WiFi and peripherals
    Share GND with ESP32 USB (important!)
    No USB current limit issues

Wiring:
    Wall Adapter (+) ───→ LED Power Rail (+5V)
    Wall Adapter (─) ───→ Common GND ←─── ESP32 GND (USB)
    ESP32 USB ───────→ ESP32 only (not LEDs)
```

---

### Power Supply Circuit with Filtering

```
External 5V Power Supply (for 6-8 LED configs):

    Wall Adapter 5V/1.5A
         (+)  (─)
          │    │
          │    GND ──────────────┬────────── Common GND
          │                      │
        [Fuse]                   │
        500mA-1A                 │
          │                      │
          ├──[1000µF]─────────┬──┘  Bulk filtering
          │   Electrolytic    │
          │   16V, low ESR     │
          │                    │
          ├──[100nF]──────────┬┘   High-freq decoupling
          │   Ceramic, 50V    │
          │                   │
         +5V Rail ────────────┴──── To all LED anodes
          │
          │
    ┌─────┴─────┬─────────┬─────────┬──────────┐
    │           │         │         │          │
   LED1        LED2      LED3      LED4     WS2812B
   Anode       Anode     Anode     Anode      VCC
    │           │         │         │          │

ESP32 USB Power (separate):
    USB 5V ───→ ESP32 VIN (for ESP32 only)
    USB GND ──→ Common GND (shared with wall adapter GND)
```

**Important Notes:**
- **NEVER connect USB +5V and wall adapter +5V together!**
- **ALWAYS share GND between USB and wall adapter**
- This allows ESP32 to control transistors via GPIO
- Power rails remain isolated (prevents backfeed)

---

## 🧪 Testing Points

### Measurement Points for Troubleshooting

```
         +5V ←─── Test Point 1 (TP1): Should be 5V ± 0.25V
          │
       [33Ω]
          │
        LED ←─── TP2: Anode voltage = 5V (LED off) or 3.65V (LED on)
          │
          │ ←──── TP3: Cathode voltage = 3.65V (off) or 0.2V (on)
          │
          C
          │
      2N2222 ←─── TP4: Collector voltage = 3.65V (off) or 0.2V (on)
          │
          B ←──── TP5: Base voltage = 0V (off) or 0.7V (on)
          │
          E
          │
         GND ←─── TP6: Should be 0V (ground reference)

Measuring with Multimeter:

Test Point    Expected (OFF)    Expected (ON)    Meaning
─────────────────────────────────────────────────────────────
TP1 (+5V)     5.0V ± 0.25V     5.0V ± 0.25V     Power rail
TP2 (Anode)   5.0V             3.65V            LED voltage
TP3 (Cathode) 3.65V            0.2V             Transistor drop
TP4 (Collector) 3.65V          0.2V             Saturation check
TP5 (Base)    0V               0.7V             Transistor active
TP6 (GND)     0V               0V               Ground reference

Current Measurement Points:

Insert ammeter in series:
    - Between +5V and LED anode: Measure LED current (should be ~100mA ON)
    - Between GPIO and 470Ω resistor: Measure base current (should be ~5mA ON)
    - Between +5V and entire circuit: Measure total system current
```

---

## 📝 Component Substitution Guide

### IR LED Alternatives

| Original | Alternative | Notes |
|----------|-------------|-------|
| TSAL6200 | TSAL6100 | Lower power (50mW vs 200mW), shorter range |
| TSAL6200 | TSUS5400 | Similar specs, good substitute |
| TSAL6200 | SFH4550 | Osram, 950nm, slightly different wavelength |
| TSAL6200 | VSLY5940 | Vishay, near-identical specs |

### Transistor Alternatives

| Original | Alternative | Notes |
|----------|-------------|-------|
| 2N2222A | PN2222 | Plastic package, same specs |
| 2N2222A | 2N3904 | Lower gain, works for 100mA |
| 2N2222A | BC547 | Lower current (100mA max), use for single LED |
| 2N2222A | TIP120 | Overkill (5A), use for high-power (500mA+) |

### IR Receiver Alternatives

| Original | Alternative | Notes |
|----------|-------------|-------|
| IRM-3638T | VS1838B | Same 38kHz, different package |
| IRM-3638T | TSOP38238 | Vishay, very popular, same specs |
| IRM-3638T | IRM-3638N | Same specs, different model |

---

## ✅ Final Checklist

**Before powering on:**
- [ ] All component polarities correct (LEDs, transistors, capacitors)
- [ ] No solder bridges between adjacent pins
- [ ] Current limiting resistors installed (33Ω for LEDs)
- [ ] Base resistors installed (470Ω for transistors)
- [ ] Ground connections continuous
- [ ] 5V rail not shorted to GND (measure >1kΩ)

**After power-on:**
- [ ] +5V rail measures 5V ± 0.25V
- [ ] ESP32 boots (blue LED flashes)
- [ ] WS2812B shows dim blue (idle status)
- [ ] No smoke or hot components
- [ ] IR receiver OUT pin toggles when IR remote pressed

**During transmission test:**
- [ ] LED flashes visible on smartphone camera
- [ ] Current draw ~100mA per LED
- [ ] WS2812B flashes cyan
- [ ] Target device responds to IR signal

---

**Document Version:** 1.0.0
**Last Updated:** December 24, 2024
**Author:** Sai Automations
**License:** MIT

**Ready to build! Follow the schematics carefully and double-check all connections! 🔌✨**
