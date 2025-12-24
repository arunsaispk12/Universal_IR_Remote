# Universal IR Remote - PCB Layout Guide
## Professional PCB Design for Multi-Directional IR Transmission

**Version:** 1.0.0
**Date:** December 24, 2024

---

## 🎯 Quick Reference

### Recommended PCB Specifications

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Board Size** | 60×60mm | For 4-LED config |
| **Layers** | 2 (double-sided) | Top + Bottom |
| **Thickness** | 1.6mm | Standard |
| **Copper Weight** | 1oz (35µm) | Standard |
| **Min Trace Width** | 0.5mm | Signal traces |
| **Power Trace Width** | 1.5mm | 5V, GND |
| **Min Clearance** | 0.3mm | Trace to trace |
| **Via Size** | 0.8mm drill, 1.2mm pad | Standard |
| **Surface Finish** | HASL or ENIG | HASL cheaper |
| **Solder Mask** | Green (or any) | Personal preference |
| **Silkscreen** | White on green | Component labels |

---

## 📐 PCB Layout Options

### Option 1: DIY Single-Sided PCB (Beginner)

**Best for:** Prototyping, learning, hand-etching

```
Top View (Component Side):
┌────────────────────────────────────┐
│                                    │
│         LED_N                      │
│           ○                        │
│                                    │
│                                    │
│  LED_W         ESP32        LED_E  │
│    ○        ┌────────┐        ○    │
│             │        │             │
│             │  WROOM │             │
│             │   32   │             │
│             │        │             │
│             └────────┘             │
│                                    │
│           ○                        │
│         LED_S                      │
│                                    │
│  [WS2812B]  [Receiver] [USB]      │
│     ○           ○        ●         │
└────────────────────────────────────┘

Bottom View (Copper Side):
┌────────────────────────────────────┐
│  ╔════════════════════════════╗    │
│  ║    GROUND PLANE (POUR)     ║    │
│  ║                            ║    │
│  ║  Transistors & Resistors   ║    │
│  ║  (surface mount optional)  ║    │
│  ║                            ║    │
│  ║  +5V traces (1.5mm wide)   ║    │
│  ║                            ║    │
│  ╚════════════════════════════╝    │
└────────────────────────────────────┘
```

**Pros:**
- Simple to design
- Can be hand-etched
- Easy to modify
- Low cost

**Cons:**
- Limited routing space
- Manual wire jumpers needed
- Less professional appearance

---

### Option 2: Double-Sided PCB (Recommended)

**Best for:** Final product, professional appearance

```
Top Layer (Component + Signal):
┌────────────────────────────────────┐
│    ○ M3 Hole                       │
│                                    │
│         LED_N ┐                    │
│           ○   │ Signal traces      │
│               ↓ (0.5mm)            │
│  LED_W    ┌────────┐        LED_E  │
│    ○ ───→ │  ESP32 │ ←────   ○    │
│           │ DevKit │               │
│           │        │               │
│           └────────┘               │
│               ↑                    │
│           ○   │                    │
│         LED_S ┘                    │
│                                    │
│  [RGB]  [RX]  [USB]          ○ M3 │
└────────────────────────────────────┘

Bottom Layer (Power + Ground):
┌────────────────────────────────────┐
│  ╔════════════════════════════╗    │
│  ║    GROUND PLANE (FLOOD)    ║    │
│  ║                            ║    │
│  ║   Q1  Q2  Q3  Q4 (under)   ║    │
│  ║   R1  R2  R3  R4 (SMD)     ║    │
│  ║                            ║    │
│  ║   +5V Rails (1.5mm)        ║    │
│  ║                            ║    │
│  ║   Thermal relief vias      ║    │
│  ╚════════════════════════════╝    │
└────────────────────────────────────┘
```

**Pros:**
- Professional appearance
- Excellent ground plane
- Compact routing
- Low noise/interference
- Easy to manufacture

**Cons:**
- Requires PCB fab service
- 2-week lead time
- ~$10 for 5 boards (JLCPCB)

---

### Option 3: 4-Layer PCB (Advanced)

**Best for:** High-volume production, EMI-sensitive

```
Layer Stack-up:
┌────────────────────────────────────┐
│  Layer 1: Top Signal + Components  │  35µm copper
├────────────────────────────────────┤
│  Layer 2: Ground Plane (GND)       │  35µm copper
├────────────────────────────────────┤  ← FR4 Core
│  Layer 3: Power Plane (+5V, +3.3V) │  35µm copper
├────────────────────────────────────┤
│  Layer 4: Bottom Signal            │  35µm copper
└────────────────────────────────────┘

Total thickness: 1.6mm
```

**Pros:**
- Excellent EMI shielding
- Dedicated power/ground planes
- Very low impedance
- Professional grade

**Cons:**
- Higher cost ($30+ for 5 boards)
- Overkill for this application
- Longer lead time

**Recommendation:** Use 2-layer for this project.

---

## 🎨 Component Placement (Top Layer)

### Detailed Component Layout

```
        60mm × 60mm PCB
┌─────────────────────────────────────┐
│ M3  Mounting Hole (3.2mm)           │ ← 5mm from edge
│ ○                               ○   │
│                                     │
│        LED_N (North, 0°)            │
│          ○ ← 5mm from edge          │
│          │                          │
│          R1 (33Ω, 1W, TH)           │
│          │                          │
│          Q1 (2N2222, TO-92)         │
│          │                          │
│         R5 (470Ω, 1/4W)             │
│          │                          │
│          ●━━━━━━━━━━━━━━━━━━━━━━┐   │
│                                 │   │
│  LED_W          ESP32          LED_E│
│    ○        ┌──────────┐         ○  │
│    │        │          │         │  │
│   R2        │  ESP32   │        R3  │
│    │        │ DevKitC  │         │  │
│   Q2        │  WROOM   │        Q3  │
│    │        │   32     │         │  │
│   R6        │          │        R7  │
│    │        └──────────┘         │  │
│    ●━━━━━━━━━●━━━━━━━━━●━━━━━━━━●  │
│              │                      │
│             Q4                      │
│              │                      │
│             R4                      │
│              │                      │
│            LED_S (South, 180°)      │
│              ○                      │
│                                     │
│  Status LED  IR RX    Power         │
│  (WS2812B)  (IRM-    (USB or        │
│     ○        3638T)   Barrel)       │
│              ○          ●           │
│                                     │
│ ○                               ○   │
│    Mounting Holes                   │
└─────────────────────────────────────┘

Legend:
○ = Through-hole component (LED, transistor)
● = Connection point / trace
━ = Power/signal trace
R1-R4 = LED current limiting resistors (33Ω)
R5-R8 = Base resistors (470Ω)
Q1-Q4 = NPN transistors (2N2222)
```

---

### Component Clearances

**Minimum Clearances:**
- **Board edge to component:** 5mm (prevents damage during cutting)
- **LED to LED:** 20mm (allows angling without interference)
- **Component to mounting hole:** 6mm keepout (3mm hole + 3mm clearance)
- **High voltage to low voltage:** 2mm (not applicable here, all 5V)

**LED Positioning:**
- Place LEDs near board edges for unobstructed IR transmission
- Orient flat side of LED toward PCB center (for reference)
- Leave space for 45° angle bending
- Ensure clearance for heat shrink tubing (if used)

---

## 🛤️ Trace Routing Guidelines

### Trace Width Calculator

```
Required Current Capacity:

Trace Type          Current    Width (1oz Cu)   Temp Rise
────────────────────────────────────────────────────────
Signal (GPIO)       10mA       0.3mm (min)      <5°C
LED anode (+5V)     100mA      1.0mm            <10°C
Power rail (+5V)    500mA      1.5mm            <10°C
Ground return       500mA      2.0mm (pour)     <5°C

Formula (for 1oz copper, 10°C rise):
Width (mm) = 0.048 × I^0.44 × (ΔT)^0.725
where I = current in amps, ΔT = temperature rise
```

**Recommended Trace Widths:**
- **Signal traces (GPIO):** 0.5mm (wider than minimum for reliability)
- **LED power (+5V to anode):** 1.5mm
- **Base resistor to GPIO:** 0.5mm
- **Ground traces:** 2.0mm or pour
- **Main power rail:** 2.0mm or wider

---

### Routing Strategy

**Top Layer (Signal Layer):**
```
Priority 1: GPIO to base resistors
  ESP32 GPIO17 ──[0.5mm trace]──> Base resistor R5/R6/R7/R8

Priority 2: LED anodes to +5V
  +5V rail ──[1.5mm trace]──> LED anodes (via current limiting resistor)

Priority 3: Status LED & receiver
  ESP32 GPIO22 ──[0.5mm]──> WS2812B DIN
  ESP32 GPIO18 ──[0.5mm]──> IR Receiver OUT
```

**Bottom Layer (Power/Ground):**
```
Priority 1: Continuous ground plane
  Flood fill with copper pour
  Thermal relief on vias (4 spokes, 0.3mm)
  Hatched or solid fill (solid recommended)

Priority 2: Power distribution
  +5V rail from USB/power connector
  Branch to each LED resistor via vias
  Star topology from power source

Priority 3: Via placement
  Via near each transistor emitter (to ground)
  Via near each +5V branch point
  Via near ESP32 GND pins (multiple)
```

---

### Routing Rules

**Do's:**
✅ Use 45° angles (not 90°) for traces
✅ Keep traces as short as possible
✅ Use ground plane for all returns
✅ Add thermal relief to ground vias
✅ Route power traces wider than signals
✅ Use teardrop pads (optional, improves reliability)

**Don'ts:**
❌ Don't use 90° angles (increases impedance, EMI)
❌ Don't route long traces without ground reference
❌ Don't place vias under components
❌ Don't run signal traces parallel to power traces for long distances
❌ Don't forget thermal relief on ground connections (hard to solder!)

---

## ⚡ Power and Ground Planes

### Ground Plane Design

```
Bottom Layer Ground Plane:

┌────────────────────────────────────┐
│  ╔════════════════════════════╗    │
│  ║ ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ ║    │ ▓ = Copper pour
│  ║ ▓                        ▓ ║    │
│  ║ ▓   ○ Via (thermal       ▓ ║    │
│  ║ ▓     relief)            ▓ ║    │
│  ║ ▓                        ▓ ║    │
│  ║ ▓   Components (view     ▓ ║    │
│  ║ ▓   from bottom)         ▓ ║    │
│  ║ ▓                        ▓ ║    │
│  ║ ▓   ╱╲╱╲  Thermal relief ▓ ║    │
│  ║ ▓  ╱  ○  ╲ (cross-hatch) ▓ ║    │
│  ║ ▓ ╱   │   ╲              ▓ ║    │
│  ║ ▓▓▓▓▓▓│▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ ║    │
│  ╚═══════╪═════════════════════╝    │
│          │                          │
│       GND Pin                       │
└────────────────────────────────────┘
```

**Ground Plane Rules:**
- **Flood fill** entire bottom layer (or maximum area)
- **Thermal relief** on all vias and pads (makes soldering easier)
- **Via stitching** every 15-20mm around perimeter
- **Clearance** from board edge: 2mm minimum
- **Copper keepout** around mounting holes (3mm radius)

**Thermal Relief Pattern:**
```
Standard Via:              With Thermal Relief:
    Copper pour                 Copper pour
   ╔═══════╗                  ╔═══════╗
   ║▓▓▓▓▓▓▓║                  ║▓     ▓║
   ║▓▓▓○▓▓▓║  ← Hard to       ║▓ ╱╲  ▓║  ← Easy to
   ║▓▓▓│▓▓▓║    solder        ║ ╱  ○ ▓║    solder
   ║▓▓▓│▓▓▓║    (heat sink)   ║╱   │╲▓║    (limited sink)
   ╚═══╪═══╝                  ╚════╪═══╝
```

---

### Power Distribution

**Star Topology (Recommended):**
```
           USB Power Input
                 │
                 ○ Via to bottom layer
                 │
        ┌────────┴────────┬────────┬────────┐
        │                 │        │        │
       Via               Via      Via      Via
        │                 │        │        │
     LED_1             LED_2    LED_3    LED_4
     Anode             Anode    Anode    Anode
```

**Tree Topology (Alternative):**
```
           USB Power Input
                 │
        ┌────────┴────────┐
        │                 │
      LED_1             ┌─┴─┐
        │               │   │
      LED_2           LED_3 LED_4
```

**Star topology preferred:**
- Equal voltage drop to all LEDs
- More predictable behavior
- Easier to troubleshoot

---

## 🔌 Pad Design and Footprints

### Through-Hole Component Pads

**Standard Resistor (1/4W, 0.6mm leads):**
```
Pad diameter: 1.6mm
Hole diameter: 0.8mm
Annular ring: 0.4mm (pad radius - hole radius)

    ┌─────┐
    │  ○  │ ← Pad (1.6mm Ø)
    │  ●  │ ← Hole (0.8mm Ø)
    └─────┘
```

**Power Resistor (1W, 0.8mm leads):**
```
Pad diameter: 2.0mm
Hole diameter: 1.0mm
Annular ring: 0.5mm

Larger pads for higher current and better heat dissipation
```

**TO-92 Transistor (2N2222):**
```
Pad layout (front view):
    E   B   C
    ○───○───○
   1.6mm spacing

Pad diameter: 1.8mm
Hole diameter: 0.9mm

Add silkscreen outline showing flat side orientation
```

**5mm IR LED (TSAL6200):**
```
Pad layout:
    Cathode  Anode
       ─○─────○─
    (flat)  (long)

Pad diameter: 1.8mm
Hole diameter: 1.0mm (LEDs have thick leads)
Spacing: 2.54mm (0.1" standard)

Add polarity marker on silkscreen (+ for anode)
```

---

### Surface Mount Component Pads

**0805 Resistor (optional SMD version):**
```
Pad size: 1.2mm × 1.4mm
Gap: 0.8mm
Overall footprint: 2.0mm × 1.4mm

  ┌────┐     ┌────┐
  │    │     │    │  ← 1.2×1.4mm pads
  │    │     │    │
  └────┘     └────┘
    ╰──0.8mm──╯
```

**SOT-23 Transistor (optional SMD version):**
```
Standard SOT-23 footprint:

       1   2   3
       ○   ○   ○  ← Pins (top view)
      ┌─────────┐
      │         │
      │   SOT   │
      │   -23   │
      └─────────┘

Pad size: 0.6mm × 1.0mm
Pitch: 0.95mm (pins 1-2-3)
Gap: 1.9mm (pin 1 to pin 2/3)
```

---

## 🎨 Silkscreen Design

### Essential Silkscreen Elements

**Component Reference Designators:**
```
Top Silkscreen Layer:

    LED1          ← Component reference (white text)
     ○
     │
    [33Ω]         ← Value (optional)
     │
     Q1
   ┌─┴─┐
   │2N2│         ← Component type
   │222│
   └───┘

  Polarity marks:
    ┌───┐
    │ + │ ← Anode marker for LED
    └───┘

    ┌───┐
    │ ─ │ ← Cathode marker (flat side)
    └───┘
```

**Board Information:**
- **Project name:** "Universal IR Remote v1.0"
- **Date:** "2024-12-24"
- **Designer:** "Your Name"
- **Revision:** "Rev A" or "Rev 1.0"
- **Board ID:** Unique serial number or batch code

**Pin Functions:**
- Label GPIO pins: "TX", "RX", "LED", "GND", "5V"
- Mark USB connector: "USB 5V"
- Mark status LED: "STATUS"

**Polarity Indicators:**
- **"+"** on LED anodes
- **"─"** on LED cathodes (flat side)
- **Triangle** pointing to pin 1 of ICs
- **Dot** on transistor emitter

**Orientation Marks:**
```
LED Orientation:
    N
    ↑
  W ← → E
    ↓
    S

Shows which direction each LED faces
```

---

## 🔩 Mounting Holes and Board Outline

### Mounting Hole Specifications

**Standard M3 Mounting Holes:**
```
Hole diameter: 3.2mm (for M3 screw)
Pad diameter: 6.0mm (clearance for screw head)
Copper keepout: 6.5mm diameter
Solder mask opening: 6.0mm

    ╭─────────╮
   ╱           ╲
  │    ┌───┐    │ ← 6.0mm copper annular ring
  │    │ ● │    │ ← 3.2mm hole (M3 clearance)
  │    └───┘    │
   ╲           ╱
    ╰─────────╯
```

**Mounting Hole Placement:**
```
60mm × 60mm board:

 ○                               ○
 5mm from                    5mm from
 corner                      corner
 │                               │
 │                               │
 │                               │
 │           Board               │
 │          (60×60)              │
 │                               │
 │                               │
 │                               │
 ○                               ○

Hole positions (from corner):
  (5, 5), (55, 5), (5, 55), (55, 55)
```

---

### Board Outline

**Rounded Corners (Professional Look):**
```
┌───────────────────────────────┐
│ ╭───────────────────────────╮ │ ← 2mm radius
│ │                           │ │
│ │                           │ │
│ │        PCB Area           │ │
│ │                           │ │
│ │                           │ │
│ ╰───────────────────────────╯ │
└───────────────────────────────┘

Rounded corners:
  - Radius: 2mm
  - Prevents sharp edges
  - Professional appearance
  - Easier handling
```

**Castellated Holes (Advanced):**
```
Edge view:
┌──╮  ╭──┬──╮  ╭──┐
│  ╰──╯  │  ╰──╯  │
└─────────────────┘

Half-holes on board edge for direct soldering to motherboard
Advanced feature, not needed for this project
```

---

## 📏 Design Rule Check (DRC) Settings

### Minimum Design Rules

**For JLCPCB/PCBWay Standard:**
```
Trace Width:
  Min: 0.127mm (5 mil)
  Recommended: 0.25mm (10 mil) for signals
  Recommended: 0.5mm+ for signals in this project

Trace Spacing:
  Min: 0.127mm (5 mil)
  Recommended: 0.3mm (12 mil)

Via:
  Min drill: 0.3mm
  Min annular ring: 0.15mm
  Recommended: 0.8mm drill, 0.4mm ring

Pad to trace:
  Min: 0.2mm
  Recommended: 0.3mm

Pad to pad:
  Min: 0.2mm
  Recommended: 0.5mm

Copper to board edge:
  Min: 0.3mm
  Recommended: 2mm (prevents copper peeling)
```

**DRC Settings Template (KiCad):**
```
Clearance: 0.3mm
Track width: 0.5mm (signals), 1.5mm (power)
Via diameter: 1.2mm
Via drill: 0.8mm
Minimum annular ring: 0.2mm
```

---

## 🖨️ Gerber File Generation

### Required Gerber Files

**Standard Gerber set (RS-274X format):**
1. **Top Copper** - `ProjectName-F_Cu.gbr`
2. **Bottom Copper** - `ProjectName-B_Cu.gbr`
3. **Top Solder Mask** - `ProjectName-F_Mask.gbr`
4. **Bottom Solder Mask** - `ProjectName-B_Mask.gbr`
5. **Top Silkscreen** - `ProjectName-F_SilkS.gbr`
6. **Bottom Silkscreen** - `ProjectName-B_SilkS.gbr`
7. **Board Outline** - `ProjectName-Edge_Cuts.gbr`
8. **Drill File** - `ProjectName.drl` or `ProjectName-PTH.drl`

**Optional (recommended):**
9. **Paste Mask Top** - For SMD assembly (not needed for through-hole)
10. **Paste Mask Bottom**

---

### Pre-Submission Checklist

**Before ordering PCBs:**
- [ ] Run Design Rule Check (DRC) - Zero errors
- [ ] Run Electrical Rule Check (ERC) - Zero errors
- [ ] Verify all component footprints match datasheets
- [ ] Check silkscreen text is readable (min 1mm height)
- [ ] Verify mounting hole clearances (no copper under screws)
- [ ] Test fit ESP32 module footprint
- [ ] Verify USB connector orientation
- [ ] Check LED polarity markers
- [ ] Verify ground plane has no islands (all connected)
- [ ] Check power trace widths meet current requirements
- [ ] Preview Gerbers in viewer (online or CAM350)
- [ ] Verify board outline is correct (no gaps)

---

## 💰 PCB Manufacturing Options

### Budget Option: JLCPCB

**Specifications:**
- **Price:** $2 for 5× 100×100mm PCBs
- **Lead time:** 2-3 days production + 1-2 weeks shipping
- **Min order:** 5 boards
- **Options:** Color, HASL/ENIG, thickness

**Order Settings:**
```
Board size: 60×60mm (within 100×100mm)
Layers: 2
Thickness: 1.6mm
Copper weight: 1oz
Surface finish: HASL (lead-free)
Solder mask color: Green (or your choice)
Silkscreen color: White
Min track/spacing: 6/6mil
```

**Total cost:** ~$10-15 including shipping

---

### Premium Option: OSH Park

**Specifications:**
- **Price:** $5/sq inch × 3 boards minimum
- **Lead time:** 2 weeks
- **Quality:** High (Made in USA)
- **Purple boards** (signature color)

**Cost for 60×60mm:**
```
Area: 60mm × 60mm = 3600mm² = 5.58 sq in
Cost: 5.58 × $5 = $27.90 for 3 boards
```

---

### DIY Option: Home Etching

**Process:**
1. Print design on transparency film (laser printer)
2. UV exposure with photoresist PCB
3. Develop in sodium hydroxide
4. Etch in ferric chloride
5. Drill holes
6. Tin plate or solder

**Pros:**
- Immediate results (same day)
- No shipping wait
- Learning experience

**Cons:**
- Single-sided only (easily)
- Lower quality
- Messy chemicals
- No solder mask
- No silkscreen

---

## ✅ Final Design Checklist

### Before Manufacturing

**Electrical:**
- [ ] All nets connected (no airwires)
- [ ] Ground plane continuous
- [ ] Power distribution adequate (trace widths)
- [ ] No clearance violations
- [ ] ERC passed (KiCad/Eagle)
- [ ] DRC passed

**Mechanical:**
- [ ] Board size correct (60×60mm)
- [ ] Mounting holes positioned correctly
- [ ] Component clearances adequate
- [ ] No components too close to board edge
- [ ] USB connector accessible

**Manufacturing:**
- [ ] Gerber files generated
- [ ] Drill file included
- [ ] Board outline defined
- [ ] Fiducial marks (optional, for assembly)
- [ ] Tooling holes (optional)

**Documentation:**
- [ ] Schematic matches PCB
- [ ] BOM (Bill of Materials) complete
- [ ] Assembly drawing created
- [ ] Component values on silkscreen

---

## 📚 KiCad Tutorial (Quick Start)

### Creating the PCB in KiCad

**Step 1: Schematic Entry**
```
1. Open KiCad
2. Create new project: "UniversalIR_Remote"
3. Open Eeschema (schematic editor)
4. Place components:
   - ESP32 symbol
   - 4× TSAL6200 LEDs
   - 4× 2N2222 transistors
   - 8× Resistors (4× 33Ω, 4× 470Ω)
   - 1× IRM-3638T receiver
   - 1× WS2812B LED
5. Wire connections
6. Annotate components (auto-assign references)
7. Run ERC (Electrical Rule Check)
8. Assign footprints
```

**Step 2: PCB Layout**
```
1. Open Pcbnew (PCB editor)
2. Import netlist from schematic
3. Set board outline (60×60mm)
4. Place mounting holes (M3)
5. Position components (follow layout guide above)
6. Route traces (use top and bottom layers)
7. Add ground plane (flood fill on bottom)
8. Add silkscreen text
9. Run DRC (Design Rule Check)
10. Generate Gerber files
```

**Step 3: Gerber Export**
```
1. File → Plot
2. Select layers: F.Cu, B.Cu, F.Mask, B.Mask, F.SilkS, B.SilkS, Edge.Cuts
3. Output directory: "gerbers/"
4. Click "Plot"
5. Click "Generate Drill Files"
6. ZIP all files → ready for JLCPCB!
```

---

## 🎓 Additional Resources

### PCB Design Software

**Free:**
- **KiCad** - Open source, powerful (https://kicad.org/)
- **EasyEDA** - Web-based, integrated with JLCPCB (https://easyeda.com/)

**Commercial:**
- **Eagle** - Autodesk, hobbyist version free (https://www.autodesk.com/products/eagle/)
- **Altium Designer** - Professional, expensive ($$$)

### Learning Resources

- **KiCad Tutorial:** https://docs.kicad.org/5.1/en/getting_started_in_kicad/
- **Contextual Electronics** - YouTube channel for KiCad
- **PCB Design Guide** - IPC-2221 standards (industry standard)

### PCB Manufacturers

- **JLCPCB:** https://jlcpcb.com/
- **PCBWay:** https://www.pcbway.com/
- **OSH Park:** https://oshpark.com/ (USA, premium quality)
- **Seeed Fusion:** https://www.seeedstudio.com/fusion_pcb.html

---

**Document Version:** 1.0.0
**Last Updated:** December 24, 2024
**License:** MIT

**Ready to design your PCB! Follow this guide and create a professional board! 🎨✨**
