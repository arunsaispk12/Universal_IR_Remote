# Hardware Wiring Guide - Universal IR Remote

## Overview

This guide covers complete hardware wiring for the Universal IR Remote system, including component selection, circuit design, and critical considerations for **long-distance wiring** between the ESP32 controller and remote IR transmission/reception modules.

---

## 📋 Table of Contents

1. [Component Requirements](#component-requirements)
2. [Basic Wiring Schematic](#basic-wiring-schematic)
3. [Short-Distance Wiring (<1m)](#short-distance-wiring-1m)
4. [Long-Distance Wiring (1-20m)](#long-distance-wiring-1-20m)
5. [Power Distribution](#power-distribution)
6. [Signal Integrity](#signal-integrity)
7. [EMI/Interference Mitigation](#emiinterference-mitigation)
8. [Cable Specifications](#cable-specifications)
9. [Installation Best Practices](#installation-best-practices)
10. [Troubleshooting](#troubleshooting)

---

## Component Requirements

### Essential Components

#### ESP32 Controller Module
```
MCU: ESP32-WROOM-32 or ESP32-WROOM-32D
Flash: 4MB minimum (for firmware + OTA)
RAM: 520KB (built-in)
Power: 3.3V regulated, 500mA peak
Pins Required:
  - GPIO 18: IR TX (RMT channel 0)
  - GPIO 19: IR RX (RMT channel 1)
  - GND: Common ground
  - 3.3V: Power supply
```

#### IR Transmitter (TX) Module
```
IR LED: 940nm wavelength (standard IR remote frequency)
Type: 5mm high-power IR LED (TSAL6200, VSLY5940, or similar)
Forward Current: 100mA continuous, 1A peak (pulsed)
Forward Voltage: 1.2-1.5V @ 100mA
Viewing Angle: 20-30° (narrow for longer range)
Power: 150-200mW typical

Transistor Driver: 2N2222, 2N3904, BC547 (NPN)
OR: MOSFET: 2N7000, BS170 (N-channel)
Purpose: Current amplification (ESP32 GPIO can only source 12mA)
```

#### IR Receiver (RX) Module
```
IR Receiver: TSOP38238, TSOP4838, VS1838B
Carrier Frequency: 38kHz (most common)
Also available: 36kHz (TSOP36238), 40kHz (TSOP4840)
Supply Voltage: 2.5-5.5V (3.3V compatible)
Supply Current: 0.3-0.5mA (idle), 1.5mA (active)
Output: Active LOW (idle HIGH, signal pulls LOW)
Demodulator: Built-in (removes 38kHz carrier, outputs pulses)
```

#### Power Supply
```
Voltage: 3.3V regulated (for ESP32 logic)
Current: 500mA minimum, 1A recommended
Source Options:
  - LM1117-3.3 (800mA linear regulator)
  - AMS1117-3.3 (1A linear regulator)
  - Buck converter (5V → 3.3V, 2A capable for multiple modules)
  - USB power (5V → 3.3V regulation on board)

Note: IR LED requires separate 3.3V or 5V supply (high current)
```

### Optional Components (Recommended)

#### Signal Conditioning (Long-Distance)
```
Line Driver IC: 74HC125 (quad buffer) or SN74LVC125A
Purpose: Boost signal strength for long cable runs
Output: 3.3V CMOS, ±25mA drive capability

Schmitt Trigger: 74HC14 (hex inverter with hysteresis)
Purpose: Clean up noisy signals, square up edges
Hysteresis: ~0.9V (prevents noise triggering)
```

#### Protection Components
```
Resistors:
  - IR LED current limiting: 22Ω to 47Ω (for 100mA @ 3.3V)
  - GPIO pull-up/down: 10kΩ
  - Base resistor (transistor): 1kΩ
  - Gate resistor (MOSFET): 100Ω

Capacitors:
  - Decoupling (ESP32 VCC): 100nF ceramic + 10µF electrolytic
  - Decoupling (IR RX VCC): 100nF ceramic
  - Decoupling (IR TX VCC): 100µF electrolytic (bulk)

Diodes:
  - Flyback protection (if using relay/motor): 1N4148 or 1N4007
  - ESD protection (long cables): TVS diode (3.3V)

Ferrite Beads:
  - Power line filtering: 600Ω @ 100MHz
  - Signal line filtering (if needed): 220Ω @ 100MHz
```

---

## Basic Wiring Schematic

### Standard Short-Distance Setup (<1m)

```
┌──────────────────────────────────────────────────────────────────────┐
│                        ESP32 MODULE                                  │
│                                                                      │
│  3.3V  GND  GPIO18(TX)  GPIO19(RX)                                  │
└───┬────┬──────┬───────────┬──────────────────────────────────────────┘
    │    │      │           │
    │    │      │           │  ┌────────────────────────────────┐
    │    │      │           └──┤ IR RX Module (TSOP38238)       │
    │    │      │              │                                │
    │    │      │              │  VCC  GND  OUT                 │
    │    │      │              │   │    │    │                  │
    │    │      │              └───┼────┼────┼──────────────────┘
    │    │      │                  │    │    │
    │    │      │              ┌───┘    │    │
    │    │      │              │   ┌────┘    │
    │    │      │              │   │    ┌────┘
    │    │      │              │   │    │
    │    │      │         ┌────┴───┴────┴──────┐
    │    │      │         │  +100nF capacitor  │
    │    │      │         └────────────────────┘
    │    │      │
    │    │      └─────────┐
    │    │                │
    │    │           ┌────┴────┐   IR TX Circuit
    │    │           │  1kΩ    │   (Transistor Driver)
    │    │           └────┬────┘
    │    │                │
    │    │           ┌────┴────┐
    │    │           │ 2N2222  │ (NPN transistor)
    │    │           │  (NPN)  │
    │    │           │ C  B  E │
    │    │           └─┬──┬──┬─┘
    │    │             │  │  │
    │    │             │  │  └────┐ (to GND)
    │    │             │  │       │
    │    │       ┌─────┘  └───────┼─── (Base current control)
    │    │       │                │
    │    │   ┌───┴───┐         ┌──┴──┐
    │    │   │ IR LED│         │ 22Ω │ (current limiting)
    │    │   │ 940nm │         └──┬──┘
    │    │   └───┬───┘            │
    │    │       │                │
    │    │   ────┴────────────────┘
    │    │       │
    │    └───────┴──────────────────── GND (common)
    │
    └────────────────────────────────── 3.3V (regulated)


Components:
• 1x 2N2222 NPN transistor (or equivalent)
• 1x 22Ω resistor (IR LED current limiting for 100mA)
• 1x 1kΩ resistor (transistor base)
• 1x 940nm IR LED (high-power)
• 1x TSOP38238 IR receiver
• 1x 100nF ceramic capacitor (decoupling)
```

### Detailed IR TX Circuit (High-Power)

```
ESP32 GPIO18 ───┬─── 1kΩ ───┬─── Base (2N2222)
                │            │
                │        Collector ───┬─── (+) IR LED (Anode)
                │                     │
                │                     │
                │                  22Ω resistor
                │                     │
                │                     └─── 3.3V or 5V (TX power)
                │
                │        Emitter ────────── GND
                │
                └─── Optional: 10kΩ pull-down to GND

Notes:
• 2N2222 can handle 800mA collector current (sufficient for IR LED)
• 22Ω resistor limits current: (3.3V - 1.4V LED drop) / 22Ω ≈ 86mA
• For 5V supply: use 33Ω resistor → (5V - 1.4V) / 33Ω ≈ 109mA
• Add 100µF capacitor across power supply (bulk decoupling)
```

### MOSFET Alternative (Better for PWM)

```
ESP32 GPIO18 ───┬─── 100Ω ───┬─── Gate (2N7000)
                │             │
                │         Drain ───┬─── (+) IR LED (Anode)
                │                  │
                │               22Ω resistor
                │                  │
                │                  └─── 3.3V or 5V (TX power)
                │
                │        Source ────────── GND
                │
                └─── 10kΩ pull-down to GND (mandatory)

Advantages over BJT:
• Lower gate current (microamps vs milliamps)
• Faster switching (better for 38kHz PWM)
• Lower voltage drop (RDS(on) ≈ 5Ω vs VCE(sat) ≈ 0.2V)
```

---

## Short-Distance Wiring (<1m)

### Recommended: Direct PCB Connection

**Scenario**: IR modules mounted on same PCB or within 1 meter

**Wiring**:
```
Connection Type: Direct PCB traces or short jumper wires
Wire Gauge: 22-26 AWG (0.3-0.6mm²)
Wire Type: Solid core or stranded (any standard hookup wire)
Shielding: Not required
Max Length: 1 meter
```

**Considerations**:
- ✅ Use standard breadboard jumper wires
- ✅ No special signal conditioning needed
- ✅ ESP32 GPIO can drive directly (via transistor for TX)
- ✅ Minimal voltage drop
- ✅ No crosstalk issues

**Example Layout**:
```
[ESP32 Board] ──── 20cm jumper ──── [IR TX LED on wall mount]
              ──── 30cm jumper ──── [IR RX on ceiling mount]
```

---

## Long-Distance Wiring (1-20m)

### Critical Challenge: Signal Integrity Over Distance

**Problems at Long Distances**:
1. **Capacitance loading** (cables act as capacitors, slow rise/fall times)
2. **Resistance voltage drop** (3.3V signal may drop to 2.8V or lower)
3. **Electromagnetic interference** (EMI from power lines, motors)
4. **Crosstalk** (signals couple between adjacent wires)
5. **Ground loops** (different ground potentials cause noise)

### Solution 1: Buffered Signal Lines (Recommended for 1-5m)

**Schematic**:
```
ESP32 Module                      Remote IR Module
┌────────────┐                    ┌──────────────────┐
│            │                    │                  │
│  GPIO18 ───┼──┐                 │    ┌─── IR TX    │
│            │  │  74HC125        │    │             │
│  GPIO19 ───┼──┼─── Buffer ──────┼────┼─── IR RX    │
│            │  │   (line         │    │             │
│  3.3V   ───┼──┼─── driver)      │    └─── 3.3V     │
│            │  │                 │                  │
│  GND    ───┼──┼─────────────────┼────────── GND    │
│            │  │                 │                  │
└────────────┘  │                 └──────────────────┘
                │
            ┌───┴────┐
            │74HC125 │ (quad buffer)
            │ or     │
            │74LVC125│ (3.3V CMOS)
            └────────┘

Wire Specifications:
• Type: 4-conductor shielded cable (signal + power + ground + shield)
• Gauge: 22 AWG (0.6mm²) for signals, 20 AWG (0.8mm²) for power
• Length: Up to 5 meters
• Shield: Connected to GND at ESP32 side ONLY (avoid ground loops)
```

**Component Details**:
```c
// 74HC125 Quad Buffer (3-state, non-inverting)
// Pinout (DIP-14):
// Pin 1: 1OE (Output Enable, active LOW) → GND (always enabled)
// Pin 2: 1A (Input) → ESP32 GPIO18 (TX signal)
// Pin 3: 1Y (Output) → Cable to remote IR TX
// Pin 7: GND
// Pin 14: VCC (3.3V)

// Benefits:
// • Drives up to 25mA (vs 12mA from GPIO)
// • Fast edges (11ns propagation delay)
// • 3.3V CMOS compatible
// • Low power (1µA standby)
```

**Wiring Example**:
```
ESP32 Side:                        Cable:                  Remote Side:
┌─────────┐                        ┌────────┐              ┌─────────┐
│ GPIO18  ├──► 74HC125 Pin 2 ──────┤ Blue   ├──────────────┤ IR TX   │
│ GPIO19  ├◄── 74HC125 Pin 5 ◄─────┤ Green  ├──────────────┤ IR RX   │
│ 3.3V    ├────────────────────────┤ Red    ├──────────────┤ 3.3V    │
│ GND     ├────────────────────────┤ Black  ├──────────────┤ GND     │
│         │                        │ Shield ├──────────────┤ NC      │
└─────────┘                        └────────┘              └─────────┘
                                      ↓
                                   Foil/Braid Shield
                                   (connected GND at ESP32 only)
```

### Solution 2: Differential Signaling (Recommended for 5-20m)

**Why Differential?**
- Common-mode noise rejection (EMI affects both lines equally, cancels out)
- Higher voltage swing (can use ±1.5V differential vs 0-3.3V single-ended)
- Professional-grade reliability

**Implementation with RS-485**:
```
ESP32 Module                    RS-485 Transceiver         Remote Module
┌────────────┐                 ┌──────────────┐          ┌──────────────┐
│            │                 │   MAX485     │          │   MAX485     │
│  GPIO18 ───┼─► UART TX ──────┼─► DI    A ───┼──┬───────┼─► RO ───► RX │
│            │                 │       (A+)   │  │       │              │
│  GPIO19 ◄──┼─◄ UART RX ◄─────┼─◄ RO    B ───┼──┴───────┼─◄ DI ◄─── TX│
│            │                 │       (B-)   │          │              │
│  3.3V   ───┼─────────────────┼─► VCC        │          │              │
│            │                 │              │          │              │
│  GND    ───┼─────────────────┼─► GND    GND ┼──────────┼───► GND     │
└────────────┘                 └──────────────┘          └──────────────┘
                                      │  │
                                      A  B (twisted pair, 120Ω termination)

Wire Specifications:
• Type: CAT5e/CAT6 Ethernet cable (twisted pair) or RS-485 cable
• Pairs: Use 1 twisted pair for A/B, separate wires for power/ground
• Length: Up to 20 meters (100m+ possible with proper termination)
• Termination: 120Ω resistor across A-B at far end
• Topology: Daisy-chain (not star) if multiple remote modules
```

**MAX485 Configuration**:
```c
// ESP32 Side (Transmitter)
#define DE_PIN  GPIO_NUM_4   // Driver Enable (HIGH = transmit)
#define RE_PIN  GPIO_NUM_4   // Receiver Enable (LOW = receive, tied to DE)
#define TX_PIN  GPIO_NUM_17  // UART TX
#define RX_PIN  GPIO_NUM_16  // UART RX

// For IR transmission:
gpio_set_level(DE_PIN, 1);  // Enable transmit
uart_write_bytes(UART_NUM_1, ir_data, length);
vTaskDelay(pdMS_TO_TICKS(10));  // Wait for transmission
gpio_set_level(DE_PIN, 0);  // Enable receive

// Remote Side: Mirror configuration (DE/RE tied together)
```

**Advantages**:
- ✅ Up to 20m+ distance (tested to 100m in industrial settings)
- ✅ Immune to EMI (motors, power lines, fluorescent lights)
- ✅ No ground loop issues (differential signal referenced to itself)
- ✅ Low cost (MAX485 ~$0.50, CAT5e cable cheap)
- ✅ Multiple remote modules possible (multi-drop bus)

**Disadvantages**:
- ⚠️ Requires UART protocol encoding (add framing overhead)
- ⚠️ Latency increase (~1-2ms for encoding/decoding)
- ⚠️ More complex firmware (packet framing, checksums)

### Solution 3: Optical Isolation (Best for Noisy Environments)

**When to Use**:
- Industrial environments (motors, welders, high-voltage equipment)
- Different power domains (ESP32 on 3.3V, remote module on 5V)
- Ground potential difference >0.5V
- Lightning/surge protection needed

**Schematic**:
```
ESP32 Module                  Optocoupler                Remote Module
┌────────────┐               ┌───────────┐             ┌──────────────┐
│            │               │  PC817    │             │              │
│  GPIO18 ───┼─► 330Ω ───────┼─►[LED]  │             │              │
│            │               │     │   [Photo] ──────► IR TX         │
│  GND    ───┼───────────────┼──►  │    Transistor]   │              │
│            │               │     └─────┘   │         │              │
└────────────┘               └───────────────┘         │              │
                                    ↓ │                │              │
                             Isolated │                │              │
                             Ground   │                │              │
                             (separate)│               │              │
                                      └────────────────┤ GND (remote) │
                                                       └──────────────┘

Wire Specifications:
• No electrical connection between ESP32 and remote module grounds
• Separate power supplies (isolated)
• Signal transfer via light (LED → phototransistor)
• Max distance: 5m (limited by optocoupler speed, ~200kHz max)
```

**PC817 Optocoupler Details**:
```c
// Input Side (ESP32):
// GPIO → 330Ω resistor → LED anode
// LED cathode → GND
// Current: (3.3V - 1.2V LED drop) / 330Ω ≈ 6mA

// Output Side (Remote):
// Collector → 10kΩ pull-up to remote VCC (3.3V or 5V)
// Emitter → remote GND
// Output: Active LOW (inverted signal)

// Transfer Ratio: 50-200% (CTR)
// Isolation Voltage: 5000V RMS
// Response Time: ~4µs rise, ~3µs fall
```

**Limitations**:
- ⚠️ Slower than direct connection (4µs vs 11ns for 74HC125)
- ⚠️ Inverted signal (software compensation needed)
- ⚠️ Requires careful CTR selection (PC817 variants: PC817A/B/C/D)
- ⚠️ Not suitable for very high-speed signals (>200kHz bandwidth)

**For IR RMT** (1MHz clock, 38kHz carrier):
- ✅ Adequate for RMT symbols (shortest pulse ~300µs for NEC)
- ⚠️ May distort 38kHz carrier edges (use carrier after isolation)

---

## Power Distribution

### Power Budget Calculation

```c
// ESP32 Power Consumption
#define ESP32_IDLE_MA        80   // WiFi off, CPU idle
#define ESP32_WIFI_TX_MA     240  // WiFi transmitting
#define ESP32_PEAK_MA        500  // WiFi + BLE + peripherals

// IR Receiver Power
#define IR_RX_IDLE_MA        0.3  // Waiting for signal
#define IR_RX_ACTIVE_MA      1.5  // Demodulating signal

// IR Transmitter Power (per LED)
#define IR_TX_IDLE_MA        0    // No current when off
#define IR_TX_PEAK_MA        100  // During transmission pulse

// Total Worst-Case
// ESP32: 500mA
// IR RX: 1.5mA
// IR TX: 100mA (33% duty cycle) = 33mA average
// Total: 534.5mA peak, ~350mA average

// Recommended Power Supply: 1A @ 3.3V (2x safety margin)
```

### Power Distribution for Long Cables

**Problem**: Voltage drop over long cables causes brownouts

**Voltage Drop Calculation**:
```c
// Copper wire resistance (22 AWG):
// R = 0.052 Ω/meter (round trip, both wires)

// For 10m cable @ 500mA load:
// V_drop = I × R = 0.5A × (0.052 Ω/m × 10m) = 0.26V

// At ESP32 end: 3.3V regulated
// At remote end: 3.3V - 0.26V = 3.04V (still acceptable)

// Minimum acceptable: 3.0V for ESP32, 2.5V for TSOP38238
```

**Solutions**:

#### Option A: Heavier Gauge Wire
```
Wire Gauge vs Resistance (copper, round trip):
• 26 AWG: 0.134 Ω/m → 10m = 1.34Ω → 0.67V drop @ 500mA ❌ TOO HIGH
• 24 AWG: 0.084 Ω/m → 10m = 0.84Ω → 0.42V drop @ 500mA ⚠️  Marginal
• 22 AWG: 0.052 Ω/m → 10m = 0.52Ω → 0.26V drop @ 500mA ✅  Good
• 20 AWG: 0.033 Ω/m → 10m = 0.33Ω → 0.17V drop @ 500mA ✅  Excellent
• 18 AWG: 0.021 Ω/m → 10m = 0.21Ω → 0.11V drop @ 500mA ✅  Overkill

Recommendation: 20 AWG for power, 22 AWG for signals
```

#### Option B: Higher Voltage + Local Regulation
```
ESP32 Module                        Cable (5V)              Remote Module
┌────────────┐                      ┌────────┐             ┌──────────────┐
│            │                      │        │             │  AMS1117-3.3 │
│  3.3V Reg  ├──────────────────────┤ +5V    ├─────────────┤───► 3.3V out │
│  (local)   │                      │        │             │              │
│            │                      │        │             │ IR TX + RX   │
│  GND    ───┼──────────────────────┤ GND    ├─────────────┤───► GND      │
└────────────┘                      └────────┘             └──────────────┘

Advantages:
• Lower current on cable (same power, higher voltage)
• 5V → 3.3V drop acceptable (AMS1117 needs 1V headroom minimum)
• Can use thinner wire (22 AWG acceptable for 10m @ 300mA)

Example:
• Power needed: 500mA @ 3.3V = 1.65W
• At 5V: 1.65W / 5V = 330mA
• Voltage drop: 0.33A × 0.52Ω = 0.17V (5V - 0.17V = 4.83V at remote)
• AMS1117-3.3 input: 4.83V (plenty of headroom for 3.3V output)
```

#### Option C: Separate Power at Remote End
```
ESP32 Module                    Signals Only           Remote Module
┌────────────┐                  ┌──────────┐          ┌──────────────┐
│            │                  │ GPIO18   │          │  Local 3.3V  │
│  Signals ──┼──────────────────┤ GPIO19   ├──────────┤  Power Supply│
│  Only      │                  │ GND      │          │              │
│            │                  └──────────┘          │ IR TX + RX   │
│  Local 3.3V│                                        │              │
│  Power     │                  No power on cable     └──────────────┘
└────────────┘                  (signal + GND only)          ↑
                                                     Separate PSU
                                                     (USB, wall adapter)

Advantages:
• Zero voltage drop issues
• Thin signal wires (26 AWG acceptable)
• No ground loop if proper shielding used

Disadvantages:
• Requires power outlet at remote location
• More complex installation
• Ground potential difference (use optical isolation if >0.5V)
```

---

## Signal Integrity

### Rise/Fall Time Degradation

**Problem**: Long cables act as capacitors, slow down signal edges

**Cable Capacitance**:
```
Typical coaxial cable: 30 pF/foot (100 pF/meter)
Typical twisted pair: 15 pF/foot (50 pF/meter)
Typical parallel wires: 10 pF/foot (33 pF/meter)

For 10m cable:
• Coaxial: 1000 pF (1 nF)
• Twisted pair: 500 pF
• Parallel: 330 pF

Rise time calculation:
t_rise = 2.2 × R × C

With ESP32 GPIO (R ≈ 50Ω):
• Coaxial 10m: t_rise = 2.2 × 50Ω × 1nF = 110 ns
• Twisted pair 10m: t_rise = 2.2 × 50Ω × 500pF = 55 ns
• Parallel 10m: t_rise = 2.2 × 50Ω × 330pF = 36 ns

RMT symbol timing (NEC protocol):
• Shortest pulse: 560µs (mark/space)
• Rise time budget: <10% of 560µs = 56µs

Verdict: All cable types acceptable for IR timing (56µs >> 110ns)
```

**When Rise Time Matters**:
- 38kHz carrier generation (half-period = 13µs)
- If rise time > 1µs, carrier waveform distorts
- Solution: Generate carrier at remote end (after signal transport)

### Recommended: Transport RMT Symbols, Generate Carrier Locally

```
ESP32 Module                        Remote IR TX Module
┌────────────┐                      ┌──────────────────────┐
│            │                      │                      │
│  RMT TX ───┼──► Demodulated ──────┼──► 38kHz Carrier ───┼──► IR LED
│  (symbols) │     Symbols          │     Generator        │
│            │     (on/off)         │     (timer PWM)      │
└────────────┘                      └──────────────────────┘
                Long cable (10m)          Fast edges
                Slow edges OK             (local generation)

Implementation:
• ESP32 sends: GPIO HIGH = "transmit mark", GPIO LOW = "space"
• Remote module: When GPIO HIGH, generate 38kHz PWM to IR LED
• Carrier timing critical path is short (local PCB traces)
```

---

## EMI/Interference Mitigation

### Sources of Interference

```
1. Power Line Noise (50/60 Hz + harmonics)
   • AC mains cables running parallel to signal wires
   • Mitigation: Cross signal/power at 90°, ferrite beads, shielding

2. Switching Power Supplies (20-100 kHz)
   • Buck/boost converters generate high-frequency noise
   • Mitigation: LC filters, separate grounds (star topology)

3. Fluorescent Lights (40 kHz ballast frequency)
   • Very close to IR carrier (38 kHz), can interfere with RX
   • Mitigation: Software noise filtering (implemented in v2.3.0)

4. Motors/Relays (transient spikes)
   • Back-EMF can couple into signal lines
   • Mitigation: Flyback diodes, TVS diodes, physical separation

5. WiFi/Bluetooth (2.4 GHz)
   • ESP32 WiFi can cause supply voltage ripple
   • Mitigation: Bulk capacitors (100µF), linear regulator for IR modules

6. Long Cable as Antenna
   • Unshielded cables pick up broadcast RF (AM/FM radio, cellular)
   • Mitigation: Shielding, twisted pairs, common-mode chokes
```

### Mitigation Techniques

#### 1. Cable Shielding
```
┌─────────────────────────────────────────────────────┐
│  Foil or Braid Shield (connected to GND at ONE end) │
│  ┌──────────────────────────────────────────────┐   │
│  │  Core Conductors:                            │   │
│  │  • Red:    +3.3V or +5V (power)              │   │
│  │  • Black:  GND (common ground)               │   │
│  │  • Blue:   GPIO18 signal (IR TX)             │   │
│  │  • Green:  GPIO19 signal (IR RX)             │   │
│  └──────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
         ↓ Shield Ground Connection
    Connect at ESP32 side ONLY (avoid ground loops)
    If ground loop exists (>0.5V difference), use
    ferrite bead on shield connection

Shield Effectiveness:
• Foil shield: 85-95% EMI rejection
• Braid shield: 90-98% EMI rejection (better flexibility)
• Double-shielded (foil + braid): 98-99.9% rejection
```

#### 2. Twisted Pair for Differential Signals
```
Why Twisted?
• Magnetic fields induce equal noise in both wires (common-mode)
• Differential receiver subtracts common-mode (cancels noise)
• Twist pitch: tighter = better high-frequency rejection

Recommended Twist Pitch:
• <5m: 10-20 twists per meter (casual hand-twist OK)
• 5-10m: CAT5e cable (standardized twist, <50mm pitch)
• >10m: CAT6 or RS-485 cable (tighter twist, <30mm pitch)
```

#### 3. Ferrite Beads (Common-Mode Chokes)
```
Placement: Clip-on ferrite near ESP32 module (both ends if available)

Ferrite Specifications:
• Impedance: 300-600Ω @ 100MHz
• Material: Nickel-Zinc (NiZn) for HF (>10MHz)
• Material: Manganese-Zinc (MnZn) for LF (<10MHz)
• Size: 10-15mm inner diameter (fits bundled wires)

Application:
[ESP32]──────[Ferrite Bead]───────── Long Cable ─────────[Remote]
             (absorbs HF noise)

Effect:
• Suppresses RF pickup (AM radio, cellular, WiFi interference)
• Minimal effect on low-frequency signals (DC - 1MHz OK for IR)
```

#### 4. Power Supply Filtering
```
ESP32 VCC Pin:
┌─────┬──── 3.3V Supply
│     │
│  100µF Electrolytic (bulk, low ESR)
│     │
├─────┤
│     │
│  100nF Ceramic (high-frequency decoupling, X7R or C0G)
│     │
└─────┴──── GND

IR RX Module VCC Pin:
┌─────┬──── 3.3V Supply
│     │
│  10µF Ceramic (moderate bulk)
│     │
├─────┤
│     │
│  100nF Ceramic (HF decoupling)
│     │
└─────┴──── GND

Placement: As close as possible to IC pins (<10mm)
```

#### 5. Ground Plane & Star Topology
```
           ┌─────────── Analog GND (IR modules)
           │
       ┌───┴───┐
       │  ESP32│
       │  GND  │──────── Digital GND (GPIO, logic)
       └───┬───┘
           │
           └─────────── Power Supply GND (single point)

Star Ground Rules:
• All grounds converge at power supply (single point)
• Avoid ground loops (multiple paths between grounds)
• Keep analog (IR RX) and digital (ESP32 GPIO) grounds separate
• Connect analog/digital grounds at single point (near ESP32)
```

---

## Cable Specifications

### Cable Selection Guide

#### Short Distance (<1m): Standard Hookup Wire
```
Type: Solid core or stranded hookup wire
Gauge: 22-26 AWG (0.3-0.6mm²)
Insulation: PVC or silicone
Shielding: Not required
Colors: 4-conductor (red, black, blue, green)
Cost: $0.05-0.10/meter
Example: Standard breadboard jumper wires
```

#### Medium Distance (1-5m): Multi-Conductor Cable
```
Type: 4-conductor stranded cable
Gauge: 22 AWG for signals, 20 AWG for power
Insulation: PVC (indoor) or UV-resistant PVC (outdoor)
Shielding: Foil or braid recommended
Configuration: (Signal1, Signal2, Power, Ground) + Shield
Cost: $0.30-0.50/meter
Example: Alarm cable, telephone cable (4-wire + shield)
```

#### Long Distance (5-10m): CAT5e/CAT6 Ethernet Cable
```
Type: CAT5e or CAT6 twisted pair
Pairs: 4 pairs (use 2 for signals, 1 for power, 1 spare)
Gauge: 24 AWG solid copper
Shielding: UTP (unshielded) or STP (shielded, better)
Twist Rate: <50mm pitch (standardized)
Bandwidth: 100 MHz (CAT5e), 250 MHz (CAT6)
Cost: $0.15-0.30/meter (bulk)
Advantages:
  • Readily available (networking stores)
  • Pre-twisted (excellent noise rejection)
  • RJ45 connectors optional (easy install/disconnect)
  • Multiple pairs (can run multiple IR modules)

Pinout Suggestion:
  Pair 1 (Blue):   IR TX signal + GND
  Pair 2 (Orange): IR RX signal + GND
  Pair 3 (Green):  +5V power + GND
  Pair 4 (Brown):  Spare (or second IR module)
```

#### Very Long Distance (10-20m): RS-485 Cable or CAT6
```
Type: RS-485 rated cable or shielded CAT6
Pairs: 1-2 twisted pairs + power conductors
Gauge: 22-24 AWG (RS-485 spec)
Shielding: Braid or foil + drain wire (mandatory)
Impedance: 120Ω characteristic (for RS-485 termination)
Cost: $0.40-0.80/meter
Configuration: Use RS-485 transceivers (MAX485, SN75176)

Advantages:
  • Differential signaling (immune to common-mode noise)
  • Up to 1200m possible (with proper termination)
  • Industrial-grade reliability
  • Multi-drop support (multiple remote modules on one bus)
```

### Cable Routing Best Practices

```
✅ DO:
• Run signal cables perpendicular to power lines (90° crossing)
• Use separate conduit/cable tray for IR signals vs AC power
• Secure cables every 30cm (avoid vibration-induced intermittent contact)
• Leave 10% slack (strain relief, thermal expansion/contraction)
• Label both ends of cable (IR_TX, IR_RX, PWR, GND)
• Use cable ties or velcro (avoid metal clamps that can crush wires)

❌ DON'T:
• Run signal cables parallel to AC mains (>50cm separation minimum)
• Bundle IR signal cables with motor/relay power cables
• Create sharp bends (radius >5× cable diameter minimum)
• Pull cables through tight spaces (can damage insulation)
• Leave cables loose (vibration causes intermittent faults)
• Mix outdoor/indoor rated cables (moisture ingress risk)
```

---

## Installation Best Practices

### 1. IR Transmitter Placement

**Optimal Positioning**:
```
           Ceiling or High Wall Mount
                    ↓
            ┌───────────────┐
            │   IR TX LED   │ ← Wide-angle LED (30-60°)
            │   (940nm)     │    OR multiple narrow LEDs
            └───────┬───────┘    for full room coverage
                    │
                    ↓ IR beam (cone)

         Target Devices (TV, AC, STB)
         ┌─────────────────────────┐
         │  [ TV ]   [ AC ]  [STB] │
         └─────────────────────────┘

Placement Rules:
• Height: 2-3m above floor (ceiling mount ideal)
• Angle: Point down 15-30° toward devices
• Distance: 3-10m from target devices
• Coverage: Use wide-angle LED (60°) or multiple LEDs
• Avoid: Direct sunlight on IR LED (can reduce range)
```

**Multiple LED Configuration for Full Room**:
```
            LED 1 ──┬─── 22Ω ──┬──── 3.3V
            LED 2 ──┤          │
            LED 3 ──┘          │
                               │
                       2N2222 Collector
                               │
                       ESP32 GPIO18 (via 1kΩ)

Total Current: 100mA × 3 LEDs = 300mA (within 2N2222 limit)
Coverage: 3× LEDs at 120° spacing = 360° room coverage
```

### 2. IR Receiver Placement

**Optimal Positioning**:
```
         User with Remote
               ↓
         ┌─────────────┐
         │   Person    │
         │  [Remote]   │ ← Pointing remote at RX
         └─────────────┘
               │
               ↓ IR beam from remote

         ┌───────────────┐
         │  IR RX Module │ ← TSOP38238 mounted visibly
         │  (TSOP38238)  │    (wall or ceiling, line-of-sight)
         └───────────────┘

Placement Rules:
• Line-of-sight: Must see user's typical remote-pointing direction
• Height: Eye level (1.5-2m) OR ceiling (wide coverage)
• Avoid: Direct sunlight (IR RX has daylight filter, but not perfect)
• Avoid: Reflections from glass/mirrors (can cause false triggers)
• Indicator: Add visible LED to show RX signal received (debugging)
```

**Indicator LED Circuit**:
```
IR RX OUT ───┬──── GPIO19 (ESP32)
             │
             └──► LED (via 470Ω resistor) ──► GND
                  (lights when signal received)
```

### 3. Cable Management

**Wall Installation**:
```
ESP32 Controller          Cable Path                Remote IR Module
┌──────────────┐          ┌─────────┐              ┌────────────────┐
│              │          │ Conduit │              │  IR TX + RX    │
│  Wall Mount  ├──────────┤ or      ├──────────────┤  Wall/Ceiling  │
│  Near Power  │          │ Raceway │              │  Mount         │
│  Outlet      │          └─────────┘              └────────────────┘
└──────────────┘               ↑
                         Cable hidden inside wall
                         OR surface mount raceway

Surface Mount Raceway:
• Use adhesive-backed cable channel (white PVC, 20×10mm)
• Paint to match wall color
• Total cost: ~$2-5/meter
```

**Ceiling Installation** (Commercial/Office):
```
Drop Ceiling Tiles
┌────────────────────────────────────┐
│                                    │
│  ┌──────────────┐                  │
│  │   IR TX      │ (hidden above)   │
│  │   Module     │                  │
│  └──────────────┘                  │
│         │                          │
│         │ Cable runs above ceiling │
│         │                          │
│  ┌──────┴───────┐                  │
│  │   ESP32      │ (in wiring       │
│  │   Controller │  closet)         │
│  └──────────────┘                  │
└────────────────────────────────────┘

Advantages:
• Clean install (no visible wires)
• Easy cable routing (above ceiling tiles)
• Professional appearance
```

### 4. Strain Relief

**At Connectors**:
```
        ┌───────────┐
        │  ESP32    │
        │  Module   │
        └─────┬─────┘
              │ Cable
              │
        ┌─────┴─────┐
        │  Cable Tie│ ← Secure to board mount hole
        │  or Clamp │    (prevents strain on solder joints)
        └───────────┘
              │
              ↓ Cable continues

Distance: 10-20cm from connector (not too close, acts as spring)
```

**At Long Cable Mid-Span**:
```
        Cable ──────┬──────── Cable
                    │
              ┌─────┴─────┐
              │ Cable Tie │
              │    or     │
              │ P-Clip    │
              └─────┬─────┘
                    │
                Wall Anchor
                (every 30-50cm)
```

---

## Troubleshooting

### Problem 1: IR Transmission Range Poor (<2m)

**Possible Causes**:
1. ❌ IR LED current too low
   - Measure: Voltage across current-limiting resistor should be ~2V
   - Fix: Reduce resistor value (47Ω → 22Ω for 100mA)

2. ❌ Wrong LED wavelength
   - Check: IR LED should be 940nm (some are 850nm for cameras)
   - Fix: Replace with 940nm LED (TSAL6200, VSLY5940)

3. ❌ Weak transistor drive
   - Measure: Collector-emitter voltage during pulse (<0.3V saturated)
   - Fix: Reduce base resistor (2.2kΩ → 1kΩ), check transistor type

4. ❌ Incorrect carrier frequency
   - Check: Protocol database returns correct carrier (38kHz vs 40kHz)
   - Fix: Verify `code->carrier_freq_hz` matches device (Sony = 40kHz)

5. ❌ Power supply insufficient
   - Measure: VCC voltage during transmission (should stay >3.0V)
   - Fix: Add 100µF capacitor near IR LED, upgrade power supply

**Test Procedure**:
```c
// Use smartphone camera to visualize IR LED (appears purple/white)
// Point IR LED at camera while transmitting
// Should see bright flashing (38kHz appears as solid due to camera shutter)

// Oscilloscope check:
// Probe GPIO18 pin:
// - Should see RMT symbol pulses (560µs mark/space for NEC)
// Probe IR LED anode:
// - Should see 38kHz carrier bursts during mark periods
// - Amplitude: ~2V peak (if using 3.3V supply)
```

### Problem 2: IR Reception Unreliable (False Triggers, No Detection)

**Possible Causes**:
1. ❌ TSOP38238 orientation wrong
   - Check: Pin 1 (OUT) should go to GPIO19, not VCC
   - Pinout: (Facing front) OUT - GND - VCC (left to right)

2. ❌ Wrong carrier frequency
   - TSOP38238 = 38kHz (most common)
   - If using 36kHz remote (RC5), need TSOP36238
   - If using 40kHz remote (Sony), need TSOP4840
   - Fix: Match TSOP variant to remote carrier frequency

3. ❌ Direct sunlight on receiver
   - IR RX has daylight filter, but intense sun can saturate
   - Fix: Shade receiver, add IR-pass/visible-block filter

4. ❌ Electrical noise (fluorescent lights, motors)
   - Symptom: Random triggers, ghost button presses
   - Fix: Software noise filtering (v2.3.0 implemented), shielding

5. ❌ No pull-up resistor on output (if needed)
   - TSOP38238 has weak internal pull-up (~35kΩ)
   - For long cables, add external 10kΩ pull-up to 3.3V

**Test Procedure**:
```c
// Point known working remote at IR RX
// Press button, observe:
// - ESP32 logs: "Received X RMT symbols"
// - If no logs: Check TSOP power, ground, output pin connection
// - If logs but decode fails: Check carrier frequency match

// Oscilloscope check:
// Probe GPIO19 (RX output):
// - Idle: HIGH (3.3V)
// - Signal: Pulses LOW (0V) at received mark periods
// - Should see demodulated pulses (38kHz carrier removed by TSOP)
```

### Problem 3: Intermittent Connection on Long Cable

**Possible Causes**:
1. ❌ Poor connection (cold solder joint, loose crimp)
   - Test: Wiggle cable while transmitting, observe if signal drops
   - Fix: Re-solder or re-crimp connections, use heat shrink

2. ❌ Cable too thin (voltage drop)
   - Measure: Voltage at remote end during peak load (<3.0V = problem)
   - Fix: Upgrade to 20 AWG wire or use higher voltage (5V) with regulation

3. ❌ EMI pickup
   - Test: Move cable away from power lines, observe if improves
   - Fix: Shield cable, use twisted pair, add ferrite beads

4. ❌ Capacitance loading (slow edges)
   - Measure: Rise time with oscilloscope (>1µs = problem)
   - Fix: Add line driver (74HC125), lower cable capacitance

**Test Procedure**:
```c
// Continuity check:
// Multimeter Ω mode: ESP32 end GND ↔ Remote end GND (<1Ω)
// If >1Ω: Poor connection, oxidation, or broken wire

// Voltage check:
// Multimeter DC mode: Measure 3.3V at remote end
// No load: Should be 3.25-3.35V
// With load (IR TX on): Should be >3.0V
// If <3.0V: Voltage drop too high, upgrade wire or use local regulation
```

### Problem 4: Multiple IR Modules Interfere

**Scenario**: Multiple remote IR TX/RX in same room

**Causes**:
1. ❌ IR RX picking up own IR TX transmissions
   - Solution: Physical barrier between TX and RX
   - Solution: Software filtering (ignore frames within 100ms of TX)

2. ❌ Cross-talk between modules
   - Solution: Address/ID protocol (software distinguishes modules)
   - Solution: Time-division (modules transmit in turn, not simultaneously)

**Implementation**:
```c
// Software solution: Ignore echo
#define TX_ECHO_IGNORE_MS  100

static uint64_t last_tx_time = 0;

void ir_transmit(ir_code_t *code) {
    // Transmit IR code
    last_tx_time = esp_timer_get_time() / 1000;  // ms
}

void ir_receive_task() {
    // Received IR code
    uint64_t now = esp_timer_get_time() / 1000;

    if (now - last_tx_time < TX_ECHO_IGNORE_MS) {
        ESP_LOGD(TAG, "Ignoring echo (within %dms of TX)", TX_ECHO_IGNORE_MS);
        return;  // Ignore (likely our own transmission)
    }

    // Process received code
}
```

---

## Summary Recommendations

### For Different Distance Ranges

| Distance | Wire Type | Shielding | Signal Conditioning | Power |
|----------|-----------|-----------|---------------------|-------|
| **<1m** | 22-26 AWG hookup | Not needed | Direct GPIO | 3.3V direct |
| **1-3m** | 22 AWG multi-conductor | Foil/braid | Direct GPIO | 3.3V or 5V |
| **3-5m** | 22 AWG shielded cable | Mandatory | 74HC125 buffer | 5V + local reg |
| **5-10m** | CAT5e/CAT6 | Shielded (STP) | 74HC125 buffer | 5V + local reg |
| **10-20m** | RS-485 cable | Mandatory | MAX485 (RS-485) | 5V + local reg |
| **>20m** | Fiber optic or wireless | N/A | Media converter | Separate PSU |

### Quick Reference Checklist

**Before Installation**:
- [ ] Calculate power budget (ESP32 + IR modules)
- [ ] Measure cable distance (choose wire gauge accordingly)
- [ ] Identify interference sources (AC power, motors, lights)
- [ ] Plan cable routing (avoid parallel runs with power)
- [ ] Select appropriate shielding (foil/braid for >3m)

**During Installation**:
- [ ] Test components on breadboard first (verify function)
- [ ] Solder all connections (crimp acceptable for power only)
- [ ] Add decoupling capacitors (100nF + 10µF at every IC)
- [ ] Use strain relief (cable ties, P-clips)
- [ ] Label all wires (both ends, use heat shrink labels)
- [ ] Shield ground connected at ESP32 end ONLY

**After Installation**:
- [ ] Measure voltages (3.3V at remote module within 5%)
- [ ] Test continuity (GND to GND <1Ω)
- [ ] Test transmission range (should achieve 5-10m)
- [ ] Test reception sensitivity (remote works from 3-5m)
- [ ] Wiggle test (move cables, check for intermittents)
- [ ] Long-term test (24 hours continuous operation)

---

## Advanced Topics

### Multi-Drop Bus (Multiple Remote Modules)

**RS-485 Bus Configuration**:
```
ESP32 (Master)              Remote 1           Remote 2           Remote 3
┌──────────┐               ┌─────────┐        ┌─────────┐        ┌─────────┐
│ MAX485   │   A/B Pair    │ MAX485  │        │ MAX485  │        │ MAX485  │
│ TX ┬ A ──┼───────────────┼─► A RX  │────────┼─► A RX  │────────┼─► A RX  │
│    │     │               │         │        │         │        │         │
│ RX └ B ──┼───────────────┼─► B TX  │────────┼─► B TX  │────────┼─► B TX  │
└──────────┘               └─────────┘        └─────────┘        └─────┬───┘
                                                                        │
                                                                   120Ω Termination
                                                                   (at far end only)

Protocol:
• Master polls slaves: "Module 1, transmit NEC code 0x00FF629D"
• Slave responds: "ACK" or "NACK"
• Time-division multiplexing (no collisions)
• Up to 32 modules on one bus (using 5-bit address)
```

### Wireless Alternative (For Impossible Wiring)

**ESP-NOW (ESP32-to-ESP32 Wireless)**:
```
ESP32 Controller            WiFi/ESP-NOW          ESP32 Remote Module
┌──────────────┐            ┌──────────┐          ┌──────────────────┐
│              │            │          │          │                  │
│  Main App ───┼─► ESP-NOW ┼──► WiFi ──┼──► ESP-NOW ─► IR TX/RX    │
│              │  Transmit  │  2.4 GHz │  Receive │                  │
└──────────────┘            └──────────┘          └──────────────────┘

Advantages:
• No wiring required (wireless)
• Range: 50-200m (line-of-sight, up to 1km with directional antenna)
• Low latency: <10ms
• Multiple modules: Broadcast or addressed

Disadvantages:
• Each remote needs ESP32 + power supply
• WiFi interference (2.4 GHz band crowded)
• Higher cost (~$3-5 per ESP32 module)
• More complex firmware (WiFi + IR dual functionality)
```

**Recommendation**: Use wired for critical installations (home automation), wireless for retrofit (existing homes, no wire access).

---

**Document Version**: 1.0
**Last Updated**: December 27, 2025
**Firmware Version**: v2.3.0+
**Author**: Technical Documentation Team

**Related Documents**:
- `COMMERCIAL_GRADE_FEATURES.md` - Commercial reliability features
- `README.md` - User guide and quick start
- `RELEASE_NOTES_v2.3.0.md` - Latest firmware release notes
