# How It Works — A-Contour Vacuum Controller

*Written 2026-11-17 from the code, the config files and the docs in this repo. Updated the same day after the
fixes on branch `fix/sensor-range-and-estop` (see "What changed" just below).
It describes the system **as it is built today**, not as older docs say. Where the code and an older doc
disagree, this file follows the code and says so.*

> **What changed on 2026-11-17 (branch `fix/sensor-range-and-estop`)**
> 1. **Vacuum sensors can now read the full 0–75 mmHg range** (was P1). Each sensor output goes through a
>    10 kΩ / 20 kΩ resistor divider into the 3.3 V ADC, and the software converts with that divider in mind.
>    A reading pinned at the ADC's 3.3 V top now counts as a **sensor fault** and stops the machine, so a
>    missing divider can never hide a deep vacuum. *The divider still has to be soldered in.*
> 2. **The physical emergency stop button works** (was P2). It is created at start-up, read 20 times a
>    second, read the right way round (it used to fire on release), works again after a reset, stops the
>    machine if it is already held down at start-up, and the screen switches to EMERGENCY STOP.
> 3. **Crash on closing the app fixed.** The app deleted the hardware manager before the auto program,
>    which then used it while shutting down (found by AddressSanitizer).
> 4. Simulation mode is now passed to the hardware before start-up (needed to test the whole controller), and
>    an emergency stop can be reset in simulation mode.

**Purpose of this file:** a complete, plain description of the whole machine (hardware, air system,
electronics, software, safety) so a reviewer (a person or an AI) can check the design for bugs and
suggest improvements. Section 15 lists the problems already found and the open questions.

---

## Contents

1. What the machine is and what it is for
2. The whole system in one picture
3. The cup (A-Contour cup_020)
4. The air system (pump, tank, valves, pulser, sensors)
5. The electronics (Raspberry Pi, ADC, drivers, GPIO pins, power)
6. The software: how it is organised
7. The air control loop in detail (`PneumaticController`)
8. The clitoral pulser in detail
9. Three ways to run a session
10. The screen
11. Safety: every layer, and which ones are really active
12. Settings and config files
13. Extra sensors and add-ons (TENS, fluid, heart rate, motion, cameras, games, network)
14. Building, testing and installing
15. Known problems and open questions for review
16. Glossary

---

## 1. What the machine is and what it is for

The machine is a **hands-free sexual stimulation device for the vulva**, run by a Raspberry Pi
with a large touch screen.

- A clear plastic **cup** seals onto the vulva.
- The cup has **two separate vacuum chambers**:
  - The **leg chamber** (shaped like the two legs of an "A") holds the cup on and keeps the labia engorged
    with **steady** vacuum.
  - The **clitoral chamber** (a small tube over the clitoris) can hold its own steady vacuum **and** receive
    fast **air pulses**, like a commercial air-pulse toy.
- An **open channel** between the legs has no vacuum. It leaves the urethra and vaginal opening free and
  lets fluids drain out.

**What it is meant to do:**
- Engorge the tissue first with steady vacuum, then stimulate with air pulses.
- Run timed programs ("patterns"), or an automatic program that tries to read arousal and edge, deny or
  bring the user to orgasm.
- Let the owner run the pulser **by hand** (set the pulse rate, pause for self-edging).
- Keep the cup sealed during body movement (anti-detachment / seal watch).
- Stay safe: hard pressure cap of **75 mmHg** on the body, emergency stop, and venting on faults or power loss.

**Who uses it:** the owner, alone, at home. It is a personal project, not a certified medical device.

**Status:** the software is written and tested **in simulation only**. Nothing has run on the real
hardware yet (see section 14).

---

## 2. The whole system in one picture

```
                     ┌──────────────────────────────┐
                     │   50" HDMI touch screen       │
                     └──────────────┬───────────────┘
                                    │ HDMI + USB touch
┌────────────────┐   SPI   ┌────────┴─────────┐   GPIO   ┌──────────────────────────────┐
│ MCP3008 ADC    │◄───────►│  Raspberry Pi 4  │─────────►│ 3x L293D drivers + 1 for the  │
│ ch0 leg sensor │         │  (Qt5 C++ app)   │          │ pulser: pump, 5 valves, pulser│
│ ch1 tank sensor│         └────────┬─────────┘          └──────────────┬───────────────┘
│ ch2 clit sensor│                  │ GPIO 21 (E-stop button)           │ 12 V
│ ch3 TENS imp.  │                  │ I²C (heart rate, motion)          ▼
└───────▲────────┘                  │                     ┌──────────────────────────────┐
        │ 0.13–3.13 V via dividers  │                     │ Main pump → TANK              │
        │                           │                     │ SOL1/SOL2 → leg chamber        │
┌───────┴────────┐                  │                     │ SOL4/SOL5 → clitoral chamber   │
│ 3x MPX5010DP   │◄── tees into the air lines ────────────┤ SOL3 → tank vent               │
│ pressure sens. │                                        │ Pulser pump (GPIO 15) → clit   │
└────────────────┘                                        └──────────────┬───────────────┘
                                                                          │ 2 tubes (Luer)
                                                                   ┌──────▼──────┐
                                                                   │  A-Contour  │
                                                                   │  cup_020    │
                                                                   └─────────────┘
```

---

## 3. The cup (A-Contour cup_020)

Design files: `hardware/cup/v6/` (generated by `hardware/cup/v6/scripts/cup_sdf.py`).
More detail: `docs/cup explination.md`, `docs/cup-cleaning-and-entrapment-safety.md`.

| Part | What it is | Numbers |
|---|---|---|
| Shell | One piece, clear Tritan, 2.0 mm wall | 93.5 × 128.5 × 79.6 mm overall |
| Seal lip | Platinum-cure silicone molded onto the shell | — |
| **Leg chamber** (outer, "A" legs) | Holds the cup on, engorges the labia | 204.1 cm³; target 30–50 mmHg |
| **Clitoral chamber** (inner) | Glans-and-shaft shaped tube, leans 15° toward the pubic bone | 34.3 cm³, about 70 mm deep; shaft 27 mm wide, corona 33 mm, head 26 mm |
| Hood retractor ridge | Soft 1.5 mm silicone ridge that holds the clitoral hood back | — |
| **Open channel** | Between the legs, no plastic over it, no vacuum | — |
| Ports | 2 Luer ports: one leg port (right leg), one clitoral port (glans tip) | ISO 80369-7 |
| Manual vents | 2 silicone flaps (left leg, side of glans head), work with no power | — |
| Pressure rating | Cup max 100 mmHg; controller cap 75 mmHg; shell bench-tested empty to 200 mmHg | — |

**Important cup facts for review:**
- There is **no sensor tap on the cup**. Each pressure sensor tees into its tube **at the controller**, so the
  sensors read the line pressure, not the exact pressure at the skin. Tube length and diameter add lag and
  smoothing.
- The corona (33 mm) is wider than the shaft (27 mm), an **undercut**. Tissue drawn in can swell past it.
  Both chambers must be vented, and the swelling allowed to go down, before the cup is removed (entrapment risk).
- Must be made by SLA in skin-safe resin or molded Tritan, **not FDM** (FDM layers trap bacteria).

---

## 4. The air system

### 4.1 Plumbing

```
                       ┌───────────── SOL3 (tank vent, NC) ──► air
                       │
  MAIN PUMP ──pulls──► TANK ──┬── SOL1 (leg feed, NC) ────┬──► LEG CHAMBER (cup)
  (12 V diaphragm)     │      │                           ├── SOL2 (leg vent, NO) ──► air
                       │      │                           └── AVL sensor (ADC ch0)
                       │      │
                       │      └── SOL4 (clit feed, NC) ───┬──► CLITORAL CHAMBER (cup)
                       │                                  ├── SOL5 (clit vent, NO) ──► air
                       │                                  ├── clitoral sensor (ADC ch2)
                       │                                  └── T near the cup ◄── PULSER PUMP
                       └── tank sensor (ADC ch1)                                 (both ports, check valves removed)

  Still to fit: a passive relief valve (85–100 mmHg) on the tank and on each chamber line,
  and a hydrophobic filter / backflow protector at each cup port.
```

- **NC** = normally closed (closed with no power). **NO** = normally open (open with no power).
- **With power off:** SOL1, SOL3 and SOL4 close, so the tank keeps its vacuum. SOL2 and SOL5 open, so
  **both chambers vent to air**. This is the fail-safe: losing power releases the body.

### 4.2 Parts

| Part | Details |
|---|---|
| **Main pump** | Marhynchus 12 V micro diaphragm pump, 2.5–8.0 L/min, max vacuum **−60 kPa (about 450 mmHg)**, built-in pressure switch and relief valve. It can pull about 6× the 75 mmHg cap, which is why mechanical relief valves are needed. Driven through an L293D: enable GPIO 25, PWM GPIO 18. In the code it is **on/off only** (no real speed control). |
| **Vacuum tank** | A reservoir. The pump fills it, then stops (quiet), and the chambers are fed from it through short valve openings. |
| **5 solenoid valves** | 12 V, driven by 2 L293D chips. SOL1 GPIO 17, SOL2 GPIO 27, SOL3 GPIO 22, SOL4 GPIO 23, SOL5 GPIO 24. |
| **Pulser pump** | A second small 12 V diaphragm pump (spare YOGULGUL) with its check valves removed. Both its intake and exhaust connect to the clitoral line through a T close to the cup, so the diaphragm just pushes air back and forth and makes **pressure pulses** at the motor's speed (pulses per second = motor RPM ÷ 60). Driven from GPIO 15 with software PWM (about 1 kHz) through a spare L293D half-bridge, or a MOSFET module if it draws more than about 0.5 A. |
| **Tubing** | Silicone 3 mm ID / 5 mm OD for vacuum lines; short **stiff** PU or PVC from the pulser T to the cup (soft silicone absorbs the pulses). |
| **3 vacuum sensors** | MPX5010DP differential, 0–10 kPa (0–75 mmHg). Vacuum hose on the **P2** port, P1 open to room air, so the output **rises with vacuum**: 0.2 V at 0 mmHg to 4.7 V at 75 mmHg of vacuum (a hose on P1 would read zero all the time). 5 V supply. Each output goes through a **10 kΩ / 20 kΩ divider** to the ADC. AVL/leg (ch0), tank (ch1), clitoral (ch2). |
| **Relief valves** | 3 passive duckbill valves opening at 85–100 mmHg — **not fitted yet**. |

### 4.3 Pressure numbers used by the design

| Item | Value | Where it is set |
|---|---|---|
| Hard cap on the body | **75 mmHg** | `PulserCalibration::CAP_MMHG`, `SafetyConstants::MAX_PRESSURE_STIMULATION_MMHG` |
| Tank: pump stops at | 70 mmHg | `tank_fill_mmhg` |
| Tank: pump restarts at | 55 mmHg | `tank_restart_mmhg` |
| Tank must be deeper than a chamber's target by | 5 mmHg | `top_up_margin_mmhg` |
| Leg chamber target range | 30–50 mmHg (0 = released) | `leg_min_mmhg`, `leg_max_mmhg` |
| Clitoral chamber target ceiling | 60 mmHg | `clit_max_mmhg` |
| Hand session: legs / clitoral | 40 mmHg / 20 mmHg | `HandPulser::LEG_MMHG`, `hand_clit_mmhg` |

---

## 5. The electronics

### 5.1 Main parts

| Part | Job |
|---|---|
| **Raspberry Pi 4 (8 GB)** | Runs the whole program (Raspberry Pi OS, Qt5). Talks to everything over GPIO, SPI and I²C. |
| **MCP3008** | 8-channel, 10-bit ADC on SPI (SCK 11, MOSI 10, MISO 9, CS 8). Powered from **3.3 V, with VREF at 3.3 V**. The vacuum sensors reach it through 10 kΩ / 20 kΩ dividers (2/3 of the sensor voltage), so 4.7 V becomes 3.13 V. |
| **Sensor dividers** | 3 × (10 kΩ + 20 kΩ, 1 %): sensor Vout → 10 kΩ → ADC input → 20 kΩ → GND. **Must be fitted before the first run.** |
| **L293D motor drivers** | 3 chips (pump + 5 valves), plus a spare half-bridge for the pulser. About 0.6 A per channel, 1–2 V drop. |
| **E-stop button** | A normally-open button from GPIO 21 to GND, internal pull-up (pressed = low). Read 20 times a second (section 11). Recommended extra: a latching E-stop that cuts the 12 V supply. |
| **50" HDMI touch display** | The only user interface. |
| **Power** | 5 V for the Pi, 12 V for pumps, valves and drivers, with a common ground. |

### 5.2 GPIO pin map (BCM numbers)

| GPIO | Job |
|---|---|
| 2, 3 | I²C bus 1: MAX30102 heart rate, MPU6050 motion |
| 4 | Urethral TENS fault input |
| 5 / 6 / 12 / 16 | Clitoral TENS: enable / polarity / amplitude PWM / fault input |
| 8, 9, 10, 11 | SPI to MCP3008 |
| 13 / 19 / 20 | Urethral TENS amplitude PWM / enable / polarity |
| 14 | HX711 scale clock (serial port must be off) |
| **15** | **Clitoral pulser motor, software PWM** (serial port must be off; needs a pull-down resistor) |
| 17 | SOL1 leg feed |
| 18 | Main pump PWM |
| 21 | Emergency stop button |
| 22 | SOL3 tank vent |
| 23 | SOL4 clitoral feed |
| 24 | SOL5 clitoral vent |
| 25 | Main pump enable |
| 26 | HX711 scale data |
| 27 | SOL2 leg vent |

Note: hardware PWM channel 0 is shared by GPIO 12 and 18, so real hardware PWM could not drive TENS
amplitude and the pump separately.

### 5.3 How a vacuum reading is made

All numbers are **vacuum depth**: 0 mmHg = room air, 75 mmHg = strong suction.

1. The MPX5010DP (hose on P2) gives 0.2 V at 0 mmHg to 4.7 V at 75 mmHg of vacuum (0.45 V per kPa).
2. The 10 kΩ / 20 kΩ divider passes 2/3 of that to the ADC: 0.13 V to 3.13 V.
3. The MCP3008 turns it into a number 0–1023, with 3.3 V as full scale.
4. `MCP3008::sensorVolts` undoes the divider (÷ 2/3) to get the sensor's own voltage back
   (`PRESSURE_DIVIDER_RATIO = 20/30`, channels 0–2 only; channel 3 has no divider).
5. `MCP3008::pressureFromRaw`: mmHg = sensor volts × **16.67** − **3.33** (zero at 0.2 V), clamped at ≥ 0.
   The slope and offset are per **sensor** volt, as in the datasheet and `settings.json`.
6. **Fault check:** a raw reading of **1020 or more** (the ADC's 3.3 V top) returns an error instead of a number.
   With the divider fitted this only happens above about 77 mmHg, a wiring fault, or a missing divider.
   The error becomes "not a number" in the air control loop, which does an **emergency stop**.
7. `SensorInterface` applies a light low-pass filter (alpha 0.1) for display. The air control loop uses
   **unfiltered** "sample" reads so it sees fast changes. Its health check expects 0.1–5.0 sensor volts.

---

## 6. The software: how it is organised

- **Language:** C++17 with **Qt5** (Core, Widgets, Charts, …), **libgpiod 2.x** for GPIO, Linux `spidev` for SPI. Built with CMake.
- **Repo:** github.com/morrisraybrooks/apps (local working copy `~/apps`).
- **Size:** about 61,000 lines in `src/`.

### 6.1 Start-up

```
main()
 └─ QApplication + ModernMedicalStyle (big touch-friendly look)
 └─ VacuumController::initialize()
     ├─ HardwareManager      GPIO, SPI, sensors, valves, pump, PneumaticController, HandPulser,
     │                       TENS, fluid, motion, cameras
     ├─ SafetyManager        pressure limits and fault handling
     ├─ PatternEngine        timed patterns
     ├─ AntiDetachmentMonitor   (created, but its monitoring is never started; see 11)
     ├─ ThreadManager        data acquisition thread, GUI update thread, safety monitor thread
     ├─ CalibrationManager   sensor calibration
     └─ OrgasmControlAlgorithm  the automatic program
 └─ MainWindow (the touch screen)
 └─ start the monitoring threads
```

The app finds `config/` next to its program folder, or in the folder named by `VACUUM_CONFIG_DIR`.
It can run in **simulation mode** (no hardware; fake pressures) for testing.

### 6.2 Source folders

| Folder | What it does | Used at run time? |
|---|---|---|
| `pneumatics/` | **The air side**: PneumaticController and its parts (TankLogic, ChamberRegulator, SealWatch, PulserDrive, PulserCalibration, SoftwarePwm, HandPulser). The newest, best-tested code. | Yes |
| `hardware/` | HardwareManager, MCP3008, SensorInterface, ActuatorControl (GPIO lines), ClitoralOscillator (asks the pneumatics for pulses), TENSController, FluidSensor, MotionSensor, HeartRateSensor, CameraManager, ConsentManager | Mostly yes |
| `safety/` | SafetyManager, EmergencyStop (button), AntiDetachmentMonitor, AdaptiveSealMonitor, LightweightSafetyMonitor, EmergencyStopCoordinator, SafetyConstants | Partly (see 11) |
| `control/` | OrgasmControlAlgorithm: arousal estimate + automatic modes | Yes |
| `patterns/` | PatternEngine, PatternDefinitions, PatternValidator, PatternTemplateManager | Yes |
| `gui/` | MainWindow, cards and panels, PulserCard, custom pattern editor, settings, diagnostics | Yes |
| `threading/` | DataAcquisitionThread (~50 Hz sensor sampling), GuiUpdateThread (~30 fps), SafetyMonitorThread | Yes |
| `calibration/`, `testing/` | Sensor calibration, hardware self-tests | On demand |
| `logging/`, `reporting/`, `error/`, `performance/` | Data logs, export, crash handler, performance monitor | Yes |
| `game/`, `network/`, `admin/`, `licensing/` | Gamification ("rewards and punishments"), multi-user remote control, accounts, licences | **Not created by the app today** (dormant code) |

---

## 7. The air control loop in detail (`PneumaticController`)

Design spec: `docs/superpowers/specs/2026-11-16-clitoral-pulser-system-design.md`.
Rule: **only the PneumaticController should switch the pump, SOL1/2/4/5 and the pulser.** Everything else
(patterns, the auto program, the pulser card) asks it for *targets*.

### 7.1 What other code can ask for

| Request | Effect |
|---|---|
| `setLegTarget(mmHg)` | Hold the legs at this level (clamped to 30–50; 0 = release) |
| `holdLegs()` | Go back to the last leg target |
| `setClitoralTarget(mmHg)` | Steady clitoral vacuum (≤ 60, and lower while pulsing; see 8) |
| `setPulseRate(pps)`, `setPulserEnabled(on)` | Pulser rate and on/off (refused without a valid calibration) |
| `setUserPaused(on)` | Owner's pause (section 9.3) |
| `ventAll()` | Pulser off, both chambers vented, tank keeps its vacuum |
| `stop()` | Normal end: pulser and pump off, both chambers vented |
| `ventTank()` | Pump off, open SOL3 until the tank is below 5 mmHg (works when idle too) |
| `emergencyStop()` | Everything off and vented, and **latched** until reset |

Any request for vacuum **starts the loop automatically** unless it is latched by an emergency stop.
When nothing is requested any more and both chambers are below 3 mmHg, the loop goes **idle** by itself.

### 7.2 One tick (every 10 ms, 100 Hz)

1. **Read** the leg, tank and clitoral pressures (fresh, unfiltered).
2. **Sensor check:** any reading that is not a number (a failed read, or an ADC input pinned at its 3.3 V top)
   → **emergency stop** + "sensor fault".
3. **Clitoral cap:** if the clitoral average (about 200 ms) is ≥ 74.5 mmHg → **emergency stop**.
4. **Idle check:** nothing requested and chambers below 3 mmHg → go idle.
5. **Seal watch** on the legs (7.4). If the seal has failed → **emergency stop** + "seal lost".
6. **Chamber regulators** (7.3) decide feed / vent for each chamber. A feed valve is held shut while the
   tank is not at least 1 mmHg deeper than that chamber (it would just let air back in).
7. **Tank logic** (7.5) decides if the pump runs. While "vent tank" is on, the pump stays off.
8. **Pulser** (section 8) decides the motor duty. It is forced off while: the steady clitoral level is above
   the safe ceiling for the current rate, the owner has paused, or it is waiting after Resume.
9. **Write** the outputs: pulser duty, SOL1, SOL2, SOL4, SOL5, SOL3, pump. Only changed values are written.

### 7.3 Chamber regulator (one per chamber)

- **Feed** = open the feed valve (SOL1 or SOL4) to let tank vacuum in. **Vent** = open the vent valve (SOL2 or SOL5).
- Feed and vent are **never open together**.
- Target 0 → vent continuously.
- Below target − 2 mmHg → open the feed for one **30 ms** pulse, then close for one tick to re-read.
- Above target + 3 mmHg → vent until back at the target.
- **While the pulser runs** the clitoral reading jumps up and down, so the clitoral regulator switches to a
  **200 ms average** and acts at most once per 200 ms.

### 7.4 Seal watch (anti-detachment, leg chamber)

Replaces the old fixed-threshold AntiDetachmentMonitor. It only *decides*; refilling is the regulator's job.
- **Arms** once the legs first reach the target band (target − 2 mmHg). A higher target re-arms it.
- **Leak** = legs more than **10 mmHg** below target, **or** below the band and falling faster than
  **5 mmHg per 100 ms**. A lowered target gets 1.5 s to settle before fast falls count.
- On a leak: **Recovering**. The regulator keeps feeding from the tank. It gets **3 attempts × 2 s**. If the
  legs come back to the band → Sealed. If not → **Failed** → emergency stop (both chambers vent).

### 7.5 Tank logic ("fill then stop")

- The pump runs until the tank reads **70 mmHg**, then stops (quiet).
- It restarts when the tank falls to **55 mmHg**, or when a chamber needs topping up and the tank is less
  than 5 mmHg deeper than that chamber's target.

### 7.6 Emergency stop order

Fixed order: pulser off → SOL1 and SOL4 closed → SOL2 and SOL5 open (vent both chambers) → pump off →
SOL3 open (vent the tank). The controller is then **latched**: nothing can start it again until
`HardwareManager::resetEmergencyStop()` re-arms it.

---

## 8. The clitoral pulser in detail

### 8.1 Bench calibration (required)

The pulser **refuses to run** until it has been measured on the bench and the numbers saved in
`config/pulser_calibration.json` (procedure: `docs/pulser-bench-calibration.md`, tool: `build/PulserBench`).

- The cup is sealed on a flat plate with a soft plug filling about 70 % of the clitoral chamber (worst case:
  less air = bigger pulses). **Never on a body.**
- `PulserBench` holds the legs at 30 mmHg, then for each steady clitoral level (e.g. 10, 20, 35 mmHg) runs the
  pulser at each motor duty and measures the **swing** (max − min pressure).
- The owner measures the pulse **rate** with a phone spectrum-analyser app (motor hum frequency).
- Each calibration point = `{pwm_percent, pulses_per_second, peak_mmhg}`. Points must rise in both duty and
  rate. The file must say `"measured": true`.

### 8.2 How a rate becomes motor power (`PulserDrive`)

- Rate at or above the lowest calibrated rate → motor runs continuously at the duty for that rate
  (interpolated between points).
- Rate **below** the lowest calibrated rate → slow **bursts**: motor on (at the lowest point's duty) for half of
  each beat, off for the other half. Example: 2 pulses/s = 250 ms on, 250 ms off.
- No calibration, disabled, or rate 0 → off.

### 8.3 Keeping steady + pulse under 75 mmHg

- For each rate, the calibration gives the pulse swing (between points: the **larger** neighbour, on the safe side).
- While the pulser runs, the steady clitoral level is capped at **75 − swing**. If the steady level is above
  that, the pulses wait until it comes down.

### 8.4 Software PWM

GPIO 15 is toggled by its own thread at about 1 kHz (`SoftwarePwm`). Motor inertia smooths the jitter. When
it stops, the line is left low.

---

## 9. Three ways to run a session

### 9.1 Patterns (`PatternEngine`, `config/patterns.json`)

The "VACUUM CYCLE SELECTION" card lists them. The categories are:

| Category | Patterns |
|---|---|
| Pulse | Slow / Medium / Fast Pulse (on-off, 60/70/80 %) |
| Wave | Slow / Medium / Fast Wave (sine between min and max %) |
| Air Pulse | Slow / Medium / Fast Air Pulse |
| Milking | Slow / Medium / Fast Milking (5/7/10 strokes) |
| Automated Orgasm | Single (6.5 min, 7 phases), Triple (18 min), Continuous Marathon (4-min cycles until stopped) |
| Constant (legacy) | Slow / Medium / Fast Constant Orgasm |
| Special | Edging, Denial |
| Therapeutic | Gentle, Moderate, Intensive, Circulation Enhancer, Engorgement Protocol, Pre-Session Preparation |

**How a pattern drives the air:** pattern pressures are **percent of 75 mmHg**. The engine turns that into
mmHg and calls `setLegTarget()`. The controller then **clamps it to 30–50 mmHg**. Patterns that want the
clitoral air pulses set `clitoral_oscillation`, frequency and amplitude, and the engine passes them to
`ClitoralOscillator`, which asks the PneumaticController for that pulse rate. A custom pattern editor lets the
owner build new patterns.

The Single Automated Orgasm phases (from `patterns.json`):

| Phase | Time | Legs | Clitoral |
|---|---|---|---|
| 0 Engorgement | 20 s | 45 % | steady 40 mmHg, no pulses |
| 1 Sensitivity | 30 s | 35→55 % | pulses 5–6 /s, 35 mmHg |
| 2 Adaptation | 90 s | 60 % ±8 | 6–8 /s, 40 mmHg |
| 3 Arousal build-up | 120 s | 60→85 % | 8–10 /s, 45–50 mmHg |
| 4 Pre-climax | 45 s | 85 % ±8 | 10–12 /s, 55 mmHg |
| 5 Climax | 30 s | 90 % ±5 | 11–13 /s, 60 mmHg |
| 6 Recovery | 45 s | 30 % | 4–5 /s, 25 mmHg |

### 9.2 The automatic program (`OrgasmControlAlgorithm`)

Chosen on the "EXECUTION MODE" card. Modes: **Manual, Adaptive Edging, Forced Orgasm, Multi-Orgasm, Denial,
Milking**.

**Arousal estimate (every 100 ms), 0.0–1.0.** Made from the **clitoral chamber pressure**:

| Feature | Weight |
|---|---|
| Deviation from the baseline (measured in the first 10 s) | 0.25 |
| Variance (fluctuation) | 0.15 |
| Power in the 0.8–1.2 Hz band ("orgasmic contractions") | 0.20 |
| Rate of change | 0.10 |

These are normalised to 0–1. If a heart-rate signal exists it makes up 30 % (heart-rate zone, HRV,
acceleration). The result is multiplied by **seal integrity** (leg pressure ÷ baseline) and smoothed
(alpha 0.15).

- Arousal bands: Baseline < 0.2, Warming 0.2–0.5, Plateau 0.5–0.85, Pre-orgasm 0.85–0.95, Orgasm ≥ 0.95.
- Default thresholds: **edge 0.85**, **orgasm 0.95**, **recovery 0.70**.
- Control states: Stopped, Calibrating, Building, Backing off, Holding, Forcing, Milking, Danger reduction,
  Orgasm failure, Cooling down, Error.
- Behaviour: starts at intensity 0.30 and 6 pulses/s, ramps up; on reaching the edge it backs off
  (vents the clitoral chamber only, keeps the legs sealed) for at least 5 s, then holds and builds again.
  Forced Orgasm starts at 0.60 / 10 pulses/s and keeps going through the orgasm. Denial edges repeatedly with
  growing intensity and earlier back-offs. Milking keeps arousal in a 0.75–0.90 "zone" and counts an orgasm as a
  failure.
- Limits in the code: session max 30 min, max 2 min at high pressure, max intensity 0.95, max 13 pulses/s.
- Options: **TENS** (off by default) and **"anti-escape"** (off by default: if arousal drops, intensity goes
  *up*).
- Ends by calling `ventAll()`.

### 9.3 The hand session and the pulser card (`HandPulser`, `PulserCard`)

The "CLITORAL PULSER" card on the main screen, bottom right:

| Button | What it does |
|---|---|
| **START PULSES / STOP PULSES** | Starts a hand session: legs 40 mmHg, clitoral steady 20 mmHg, pulser at the chosen rate. STOP turns the pulser off and vents both chambers. Gray when the pulser is not calibrated, after an emergency stop, or while a program runs. |
| **SLOWER / FASTER** | Steps the rate through 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 50, 60, 80, 100 pulses/s (starts at 10), never above the highest calibrated rate. Gray while a program or pattern runs (the label then shows the program's rate). |
| **PAUSE PULSES / RESUME** | For **self-edging**. Pause: pulser off and the clitoral chamber vented; the **legs stay sealed**. It wins over every program. Resume: the clitoral level comes back first, and the pulses wait for it (up to 3 s). Only works while a session runs. Emergency stop, vent-all or stop clears the pause. |
| **VENT TANK** | Pump off and tank vented. Only when nothing is running. |
| Status line | Paused (orange) / Pulser not calibrated (red) / Program in charge (blue) / Pulsing (green) |

If a program or pattern starts during a hand session, the hand session **yields**: pulser off, clitoral
chamber released, legs kept for the program.

---

## 10. The screen

- Full-screen Qt app built for a 50" touch display, big buttons and big text.
- **Navigation bar:** MAIN CONTROL, SAFETY PANEL, SETTINGS, DIAGNOSTICS, **EMERGENCY STOP** (turns into RESET
  EMERGENCY after a stop).
- **Main dashboard cards:** Vacuum Cycle Selection, Pressure Monitoring (gauges and chart), Arousal Level
  Monitoring, System Control (START SYSTEM / PAUSE), Execution Mode, Clitoral Pulser, System Status.
- Other panels: Safety, Settings (including pin numbers and limits), System Diagnostics (can switch valves and
  the pump directly), Calibration, Custom Pattern Editor, Fluid / Motion / Camera monitors.
- Keyboard: **Esc** = emergency stop, F1–F4 = panels.

---

## 11. Safety: every layer, and which ones are really active

| # | Layer | How it works | Active today? |
|---|---|---|---|
| 1 | **Power-loss fail-safe** | Vent valves SOL2/SOL5 are normally open → both chambers vent with no power. Feeds are normally closed. | Yes (valve choice) — check the bought valves really are NO/NC as listed |
| 2 | **Manual vent flaps** on the cup | Lift to release each chamber by hand | Yes (part of the cup) |
| 3 | **Passive relief valves** 85–100 mmHg | Mechanical last line | **No — not fitted yet** |
| 4 | **Main pump's own pressure switch / relief** | Built into the pump | Yes, at the pump's own (unknown, high) setting |
| 5 | **Regulator limits** | Legs clamped 30–50, clitoral ≤ 60, tank fills to 70 | Yes (in software) |
| 6 | **75 mmHg clitoral cap** | 200 ms average ≥ 74.5 → emergency stop; pulses limited by 75 − measured swing | Yes (now readable, once the sensor dividers are fitted) |
| 7 | **Seal watch** | Leak → 3 reseal attempts → emergency stop | Yes |
| 8 | **Sensor fault check** | Failed read or ADC input pinned at its top (missing divider, wiring fault, far over range) → emergency stop | Yes. A disconnected sensor reading 0 is **not** caught yet. |
| 9 | **SafetyManager** | Checks leg and tank pressure against the max (75 by default); above max → vent; invalid sensor + pump "runaway" → full vent | Yes, but it **does not watch the clitoral chamber** |
| 10 | **On-screen EMERGENCY STOP** and Esc key | Calls the full-vent emergency stop | Yes |
| 11 | **Physical emergency stop button (GPIO 21)** | `HardwareManager` creates `EmergencyStop` at start-up. It reads the button 20 times a second (active-low, pressed = ACTIVE) and calls the full emergency stop. A button held down at start-up stops the machine at once; a press after a reset stops it again. The screen shows EMERGENCY STOP. If GPIO 21 can't be opened, the app logs a critical error. | **Yes** (fixed 2026-11-17). It is software: if the app freezes, the button can't act. A 12 V cut-off switch is still recommended. |
| 12 | **Pulser calibration gate** | No measured calibration → no pulses | Yes |
| 13 | **Owner's pause** | Pause wins over every program | Yes |
| 14 | Old `AntiDetachmentMonitor` | Opens SOL1 directly when the leg pressure drops | Created but **never started** (dormant; would fight the PneumaticController if started) |
| 15 | `AdaptiveSealMonitor`, `LightweightSafetyMonitor` | Older seal / safety monitors | **Not created** |
| 16 | Session time limits (auto program) | 30 min max, 2 min at high pressure | Yes, in the auto program only |

Fault handling routes to `HardwareManager::enterFullVentState()`: since 2026-11-16, **any fault vents both
chambers** (the old "keep the legs sealed on a fault" idea was dropped).

---

## 12. Settings and config files

| File | What it holds |
|---|---|
| `config/settings.json` | Everything else: GPIO pins (older subset), PWM, SPI, logging, maintenance, network (all off), **`pneumatics`** section (the numbers in 4.3 and 7), pattern settings, **safety settings** (`max_pressure_mmhg` 100, `warning_threshold_mmhg` 80, anti-detachment numbers), sensor calibration (leg and tank only: slope 16.67, offset −3.33), screen and theme, arousal settings (TENS off, anti-escape off) |
| `config/patterns.json` | The patterns in 9.1, plus `safety_limits` (stimulation max 75, emergency 80; therapeutic max 50) |
| `config/pulser_calibration.json` | Per machine, **not in git**; made by the bench calibration. `pulser_calibration.example.json` is a template that is always refused. |

Every pneumatics value from the file is clamped to safe limits in `PneumaticSettings::clampToSafeValues()`
(e.g. tank fill 10–72, legs ≤ 75, clitoral ≤ 75). `hand_clit_mmhg` is not in the file yet, so it uses its default of 20.

---

## 13. Extra sensors and add-ons

| Add-on | What it is | Status |
|---|---|---|
| **TENS** (electrical stimulation) | 2 channels: clitoral cup electrodes and urethral electrodes. Frequency 1–100 Hz, pulse width 50–500 µs, amplitude limits 80 mA clitoral / 50 mA urethral, biphasic. | **Driver hardware not chosen.** The code does not claim the TENS pins, so it does not work. Must be an isolated, current-limited, charge-balanced stimulator, never wired straight to the Pi or 12 V. |
| **Fluid catcher scale** | TAL220 load cell + HX711 (GPIO 26 data, 14 clock); volume, flow rate, session total; linked to arousal | HX711 code uses **stub** functions — not reading real hardware yet |
| **Heart rate** | MAX30102 on I²C; would add 30 % to the arousal estimate | `HeartRateSensor` is **never created**, so the arousal estimate is pressure-only |
| **Motion** | MPU6050 on I²C, stillness detection | Created |
| **Cameras** | Body camera and cup camera, OpenCV motion / orgasm detection, with a **ConsentManager** (opt-in per camera, revocable, privacy mode, data retention) | Created; needs the cameras and consent |
| **Games** | GameEngine, achievements, progress, ConsequenceEngine with "rewards" and "punishments" (including TENS shocks and max-vacuum pulses, with its own limits: TENS ≤ 70 %, vacuum ≤ 65 mmHg, ≤ 20 punishments a session) and a safeword panel | **Not created** (dormant) |
| **Multi-user remote control** | WebSocket server so other people can send actions | **Not created** (dormant). It would listen **unencrypted on every network interface**. |
| **Accounts, device registry, remote monitor, licensing** | Admin features | Mostly dormant |
| **Logging and reports** | Pressure and safety logs to `/var/log/vacuum-controller.log`, CSV/PDF export | Yes |

---

## 14. Building, testing and installing

- **Build (PC or Pi):** `mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j3`
- **Tests:** 13 Qt test programs under `tests/pneumatics/` (run with `ctest`). Most run the air logic against a
  **fake plant** (`FakePlant.h`) that simulates the tank, chambers, leaks and pulses: controller, tank,
  regulators, seal watch, pulser drive, calibration, hand pulser, pulser card, oscillator. Two were added on
  2026-11-17:
  - `tst_sensorscaling`: full vacuum (4.7 V) reads 75 mmHg, the tank's 70 mmHg is readable, zero reads 0,
    a pinned input is a fault, and the divider is undone for calibration.
  - `tst_emergencybutton`: the button is started, a press stops everything (pump, valves, pulser, latched),
    a held button stops at start-up, holding fires once, a press after a reset stops again, the timer reads it
    by itself, and the whole `VacuumController` shows EMERGENCY STOP.
  All 13 pass on the PC, also under AddressSanitizer. The GUI card tests run with `QT_QPA_PLATFORM=offscreen`.
  The real GPIO read of the button (active-low setting) can only be checked on the Pi.
- **Install on the Pi:** `scripts/pi4-setup.sh` enables SPI and I²C, installs packages, clones or pulls
  **main** from GitHub into `/opt/v-contour`, builds it **on the Pi** and installs it. A systemd service runs the
  app as **root** with `Restart=always` (restarts 10 s after a crash).
- **What has NOT been tested:** anything on the real Pi or hardware: the ARM build, real GPIO/SPI, real sensors,
  real valve and pump timing, the pulser on the real cup, the resume timing after Pause.

**Before the first hardware run:**
1. **Solder the three 10 kΩ / 20 kΩ dividers** between the vacuum sensors and the MCP3008 (section 5.1).
   Check with a meter: with a sensor at room air the ADC input should be about 0.13 V.
2. Check each sensor's hose is on its **P2** port.
3. Fit the pull-down resistor on GPIO 15 (pulser), so the motor cannot run while the Pi boots.
4. Wire the E-stop button (GPIO 21 to GND) and **test it**: press it, and the screen must show EMERGENCY STOP.
5. Fit the relief valves and filters.
6. Do the pulser bench calibration (cup on a plate, never on a body).

---

## 15. Known problems and open questions for review

P1, P2 and P2b were fixed on 2026-11-17 and are kept here so a reviewer can check the fixes.

Severity: **CRITICAL** = could hurt someone or break hardware; **HIGH** = a main feature will not work as
intended; **MEDIUM** = wrong or confusing behaviour; **LOW** = tidy-up.
Each item says how sure we are.

### P1 — FIXED in software, hardware still to do: vacuum sensors put out more voltage than the ADC can read
- **The problem:** the MPX5010DP outputs up to **4.7 V** at 75 mmHg of vacuum, but the MCP3008 runs at
  **3.3 V**. Readings stopped at about **51.7 mmHg**, so the tank never read "full" (70) and the pump would never
  stop on its own, the 75 mmHg cap could never fire, and 4.7 V can damage the MCP3008 (limit about 3.9 V).
- **The fix (2026-11-17):**
  - Hardware: a **10 kΩ / 20 kΩ divider** on each sensor output (4.7 V → 3.13 V). Added to
    `docs/PI4_SETUP.md` and the parts list. **Not soldered yet.**
  - Software: `MCP3008` undoes the divider (`PRESSURE_DIVIDER_RATIO`), so 4.7 V reads 75 mmHg again.
  - Software: an ADC input at its top (raw ≥ 1020) is a **fault**, which stops the machine. If the divider is
    missing, the machine stops as soon as the vacuum passes about 52 mmHg instead of running blind.
  - Test: `tst_sensorscaling` (fails on the old code: full vacuum read 48.9 mmHg).
- **Still open for review:** a sensor that is unplugged reads 0 V = "no vacuum", which is not caught (the
  MPX5010DP's own zero can be anywhere from 0 to 0.425 V, so a simple low-voltage check could give false alarms).
  An idea: flag a chamber that reads 0 while its feed valve has been open for a while.

### P2 — FIXED: the physical emergency stop button did nothing
- **The problems (all in `src/safety/EmergencyStop.cpp`):**
  1. No code ever created the `EmergencyStop` object, so GPIO 21 was never read.
  2. It read the button **backwards**: with the pull-up, "not pressed" read as pressed, so it would have
     fired on **release**, not on press.
  3. After one stop and a reset, it **ignored every later press**.
  4. A button already held down at start-up was taken as "normal".
  5. It went to the old "keep the legs sealed" safe state instead of the full stop.
- **The fix (2026-11-17):** `HardwareManager` creates and starts the button at start-up (and releases GPIO 21
  at shutdown). The line is requested **active-low** with the pull-up, so pressed = ACTIVE. A press always does
  the full emergency stop unless the machine is still stopped. The first poll treats a held button as a press.
  `VacuumController` listens to `HardwareManager::emergencyButtonPressed`, so the screen shows EMERGENCY STOP.
  If GPIO 21 can't be opened the app logs a critical error.
- Test: `tst_emergencybutton` (9 cases).
- **Still recommended:** a latching E-stop switch that also **cuts the 12 V supply**. The button is read by
  software every 50 ms on the main thread; if the app hangs, it can't act. Cutting 12 V lets the normally-open
  vent valves release both chambers with no software at all.

### P2b — FIXED: crash when the app closes
- `VacuumController::shutdown()` deleted the hardware manager first. The auto program and the calibration
  manager were deleted later, and the auto program's destructor then used the deleted hardware manager to vent
  the chambers (use-after-free, found by AddressSanitizer). Shutdown now deletes them before the hardware
  manager. Covered by `tst_emergencybutton` (it builds and closes the whole controller) under AddressSanitizer.

### P3 — HIGH: the arousal estimate reads the pressure the machine itself is making
*Design concern, from reading the code.*
- All four pressure features (deviation, variance, 0.8–1.2 Hz power, rate of change) come from the **clitoral
  chamber**, which the machine fills, vents and pulses on purpose. Most of what they see will be the machine,
  not the body.
- The heart-rate sensor is never created, so it is **100 % pressure-based** today.
- Edging, Denial, Milking and Forced modes all depend on this number.
- **Ideas:** measure only when the clitoral chamber is quiet (pulser off, valves shut); use the **leg** chamber
  with its valves shut as a contraction sensor; filter out the pulser frequency; turn on the heart-rate sensor;
  record real sessions and check the estimate against the owner's own "edge" button presses.

### P4 — MEDIUM: pulse rates in the programs may not suit the pulser pump
*Needs the bench numbers.*
- Patterns and the auto program ask for **4–13 pulses/s**. The pulser pump's natural rates (motor hum) will
  likely be much higher (the example calibration uses 31–52 /s).
- Rates below the lowest calibrated rate are played as **on/off bursts** (half on, half off). At 10 /s that is
  **50 ms on, 50 ms off**. The motor may not spin up in 50 ms, so the pulses may be weak or uneven.
- Check on the bench: what does a "10 /s" burst really feel and measure like?

### P5 — MEDIUM: the 75 mmHg cap uses a 200 ms average
- During pulses, single peaks are not checked against 75; they are only limited by the calibration
  (steady ≤ 75 − measured swing).
- So safety depends on the calibration being right. It must be re-measured after changing the pump, the
  tubing or the cup. Could a peak (max) check be added as well as the average?

### P6 — MEDIUM: other code can still switch valves and the pump directly
- `CalibrationManager`, `SystemDiagnosticsPanel`, `HardwareTester` and the dormant `AntiDetachmentMonitor` call
  `setSOL1…5` / `setPump…` directly, around the PneumaticController.
- If used during a session, they fight the 100 Hz loop.
- **Fix idea:** refuse direct valve and pump commands while the PneumaticController is active, or route them
  through it.

### P7 — MEDIUM: pressure limits disagree from place to place
- 75 (design cap, code), 100 (`settings.json` max), 80 (warning in `settings.json`, emergency in
  `patterns.json`, `MAX_SAFE_CLITORAL_PRESSURE` in the auto program), 65 (game limit). The Settings screen lets
  the max be set from 50 to 100.
- The real body limit comes from the regulators (legs ≤ 50, clitoral ≤ 60) and the 75 cap.
- **Fix idea:** one limit (75) in one place; the Settings screen may only lower it.

### P8 — MEDIUM: patterns lose most of their range
- Pattern pressures are percent of 75 mmHg, but the legs are clamped to **30–50 mmHg**. Slow Pulse 60 % (45),
  Medium 70 % (52 → 50) and Fast 80 % (60 → 50) feel almost the same. Waves from 30 % to 70 % become 30 → 50.
- Pattern "amplitude in mmHg" for the clitoral chamber is not used the way the old docs say. The pulser's
  pulse size is set by the motor and calibration, not by an amplitude number.
- Should the patterns be rewritten for the new air system (leg level + clitoral steady level + pulse rate)?

### P9 — MEDIUM: main pump has no speed control, and the L293D may be too small
- The pump is on/off only. The L293D gives about 0.6 A per channel with a 1–2 V drop. **The pump current has not
  been measured.** A MOSFET driver may be needed.
- Hardware PWM channel 0 is shared by GPIO 12 (TENS) and 18 (pump).

### P10 — MEDIUM: outputs during boot, crash and restart
- Fit pull-down resistors on **every** driver input (not only GPIO 15), so nothing turns on while the Pi
  boots or after the app dies.
- The service restarts the app 10 s after a crash, running as root. In that gap the feeds are closed and the
  vents are open (safe), as long as the driver inputs sit low. Confirm the app **never resumes a session by
  itself** after a restart.
- GPIO 14 and 15 are the serial-port pins: the serial console must be off.

### P11 — MEDIUM: older docs describe a different machine
- `docs/explanation.md` and `docs/AIR_PULSE_ARCHITECTURE.md` describe 2 sensors, 3 valves and a valve-switching
  oscillator (SOL4/SOL5 at 5–13 Hz). The current design uses a **pulser pump** on GPIO 15 and a 100 Hz
  PneumaticController. A reviewer should trust this file and the code over those two docs.

### P12 — Safety review needed for dormant features (if they are ever turned on)
- **Anti-escape** (raises intensity when arousal drops) and **Denial / Forced** modes keep stimulating while the
  user's body says otherwise. The local stop (screen, and a working physical E-stop, P2) must always win, at
  once.
- **Games / ConsequenceEngine** include TENS "punishments". **Multi-user control** would accept commands over an
  unencrypted WebSocket on all network interfaces. Before either is enabled: authentication, encryption,
  consent, rate limits, and a hard rule that the person in the cup can stop everything.
- **TENS:** needs a proper isolated, current-limited, biphasic stimulator. Urethral electrodes carry extra risk
  (infection, current density).

### P13 — LOW: smaller items already known
- `settings.json` has no `hand_clit_mmhg` (default 20 is used).
- The sensor calibration in `settings.json` lists only the leg and tank sensors, not the clitoral one.
- The fluid scale (HX711) uses stub code.
- VENT TANK looks pressable after an emergency stop but does nothing until reset.
- An emergency stop tells the screen "pause cleared" a moment before the valves are written.
- `HardwareManager::shutdown()` leaves the hand pulser pointing at a deleted controller (not reachable today).
- A disconnected sensor reads 0 mmHg, which looks like "no vacuum" (see P1, still open). Clipped sensors
  are now caught.
- A Debug build turns on the old `tests/` folder, whose data files are missing (configure fails); build with
  `-DBUILD_TESTS=OFF`.

### Open questions for the reviewer
1. Is "fill the tank, then feed the chambers in 30 ms valve pulses" a good way to get smooth, quiet
   regulation? Would proportional valves or pump speed control be better?
2. Is a 200 ms average the right way to regulate the clitoral steady level while pulsing?
3. Are the seal-watch numbers (10 mmHg drop, 5 mmHg / 100 ms, 3 × 2 s) sensible for a body that moves a lot
   near orgasm?
4. Is venting **both** chambers on every fault right, or should some faults keep the legs sealed?
5. How can the cup undercut (corona wider than the shaft) be made safer against entrapment?
6. What is the best real body signal for arousal and orgasm here (leg-chamber contractions, heart rate,
   motion, fluid)?
7. Does the tube length between the sensors (at the controller) and the cup make the readings too slow for
   the seal watch and the cap?

---

## 16. Glossary

| Term | Meaning |
|---|---|
| **mmHg** | Millimetres of mercury, a pressure unit. Here always **vacuum below room air**: 0 = no suction, 75 = strong suction. 7.5 mmHg = 1 kPa. |
| **AVL** | "Applied Vacuum Line": the leg chamber line and its sensor (ADC channel 0). |
| **Leg chamber / legs** | The outer A-shaped chamber that holds the cup on. |
| **Clitoral chamber / clit** | The inner chamber over the clitoris. |
| **Feed / vent** | Opening a valve to let tank vacuum in (feed) or room air in (vent). |
| **Pulser** | The second pump that makes the air pulses in the clitoral chamber. |
| **Pulses per second (pps, /s)** | Pulse rate; 1 pps = 1 Hz. |
| **Duty / PWM** | The percentage of time a motor is switched on; sets the pulser's speed. |
| **Swing** | The biggest minus the smallest pressure during pulsing (peak-to-peak). |
| **Seal watch** | The leak detector for the leg chamber. |
| **Latched** | After an emergency stop, the machine stays stopped until it is reset. |
| **Simulation mode / fake plant** | Running the software with pretend hardware, for testing. |
| **NC / NO valve** | Normally closed / normally open with no power. |
