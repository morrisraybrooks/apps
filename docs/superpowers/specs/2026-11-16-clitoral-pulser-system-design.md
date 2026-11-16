# Clitoral Pulser – System Design

*2026-11-16. Status: **draft for approval**. Items marked **(my call)** are proposals Claude made; everything else
was decided by the owner in conversation.*

## 1. Why

The owner has spent about six months trying to get the machine to copy the **air-pulse feel of commercial toys**
(Womanizer, Satisfyer, LELO) **while the clitoris stays under steady vacuum**. Commercial toys can't hold vacuum, so
they rely on natural arousal; this machine can engorge first and then pulse. The solenoids can't do it: they take
10–30 ms to switch and each pulse has to empty and refill tubing and the tank line, so the code tops out at
3–15 Hz with tiny pulses.

The fix is to stop asking solenoids to make pulses. A **second 12 V diaphragm pump, modified into a pulser**, makes the
fast pulses; the solenoids go back to slow jobs: holding and releasing steady vacuum.

Background: [air-pulse-toy-research.md](../../air-pulse-toy-research.md).

## 2. Goals, non-goals, anti-goals

**Goals**
- Pulses at toy-like rates (roughly 20–80 per second, plus slow "deep" rhythms) on top of the clitoral chamber's
  steady engorgement vacuum.
- Leg chamber and clitoral chamber keep **independent** steady levels; the clitoral level goes up and down on its own
  (e.g. released during edging) while the legs stay sealed.
- **Quiet:** the pump fills the tank, then shuts off or slows right down.
- Steady vacuum + pulse peak **never above 75 mmHg**.

**Non-goals (this round)**
- Separate pulse-strength control. The pulser has a fixed stroke; only its **speed** is set. *(Owner: "we don't need
  all the extra.")*
- A helper microcontroller, extra air pockets, speed sensor on the pulser, restrictor on the SOL4 line.
- TENS, the HX711 fluid scale, the Jetson port, any cup redesign.

**Anti-goals (would be a failure even if it works)**
- Anything that can pull more than 75 mmHg on the body, including pulse peaks.
- Pulses that leak the steady vacuum away, or a pulser that pushes air *into* the body (pressure going positive).
- A loud machine (pump running the whole session).

## 3. Constraints (found in the project)

- Pressure sensors are **MPX5010DP, 0–10 kPa = 0–75 mmHg**. Anything deeper reads as 75.
- Main pump: Marhynchus 12 V diaphragm pump, can pull **450 mmHg**.
- `PatternEngine::applyPressureTarget()` today treats **pump speed as the pressure** and holds SOL1 open; nothing
  regulates the tank.
- `PatternEngine::performSafetyCheck()` emergency-stops if **tank** pressure exceeds the chamber limit.
- `ClitoralOscillator` makes pulses by cycling **SOL4/SOL5** through VACUUM → HOLD → VENT phases with a 1 ms
  `QTimer`.
- `AntiDetachmentMonitor` thresholds (attached ≥ 60 mmHg, detach < 50, releases only at 65) don't fit the 30–50 mmHg
  leg range; it pulls the legs to 65 mmHg during normal use.
- `ActuatorControl::setPWM()` is on/off only (≥ 50 % = on). Hardware PWM channels are shared: channel 0 = GPIO 12 (TENS)
  and 18 (pump); channel 1 = GPIO 13 (TENS) and 19 (TENS enable).
- Free GPIO: 15 (UART RX – serial port is already required off because GPIO 14 is the HX711 clock), 7 (SPI CE1, taken
  when SPI is on), 0/1 (reserved).
- Three L293D chips: 1 drives the pump, 2 drive the five valves → **several spare half-bridges**, each ~0.6 A.

## 4. Approaches considered

| | Approach | Trade-off | Decision |
|---|---|---|---|
| **A** | Keep all 5 valves; SOL4/SOL5 become slow hold/release valves; pulser on the clitoral line; keep tank and SOL3 | No re-plumbing; SOL3 still empties the tank from software | **Chosen by owner** |
| B | Remove SOL3; normally-closed feed valves + hand bleed on the tank | One less valve; tank can't be emptied from the screen | Not chosen |

Pulser hookup options:

| | Hookup | Decision |
|---|---|---|
| **1** | **Pump valves removed; intake and exhaust both connected to the clitoral line** | **Chosen by owner** |
| 2 | Pump valves kept; intake and exhaust both to the clitoral line (air loops round) | Not chosen |
| – | Valves removed, exhaust plugged | Superseded by 1 |

Pulse strength: speed-only (fixed stroke) chosen; switchable "softener" pockets, adjustable stroke and a voice-coil
pulser were set aside.

## 5. The design

### 5.1 Air side

```
pump ─► TANK ─┬─► SOL1 ─► leg line ──┬─► LEG CHAMBER (cup leg port)
  (Marhynchus)│                       ├─ AVL sensor (ADC 0)
              │                       ├─ SOL2 vent ─► air
              │                       └─ relief valve 85–100 mmHg   (my call)
              ├─► SOL4 ─► clitoral line ─┬─► CLITORAL CHAMBER (cup clitoral port)
              │                          ├─ clitoral sensor (ADC 2)
              │                          ├─ SOL5 vent ─► air
              │                          ├─ relief valve 85–100 mmHg   (my call)
              │                          └─ PULSER: both ports of the modified pump
              ├─ tank sensor (ADC 1) – upgraded to MPX5050DP (my call)
              ├─ SOL3 vent ─► air
              └─ relief valve ~150 mmHg   (my call)
```

- **Pulser:** a 12 V diaphragm pump with its check valves removed; **intake and exhaust both** piped to the clitoral
  line. With the valves out, both ports open into the same diaphragm chamber, so every turn of the motor pushes and
  pulls air in the clitoral chamber. Pulse rate = motor RPM ÷ 60. First candidate: the spare **YOGULGUL** pump (no
  built-in pressure switch or relief valve to leak). The pump body must be airtight.
- **Short, stiff tubing** from the pulser to the clitoral port (PU or PVC, as short as practical) – soft silicone
  soaks up pulses. *(my call)*
- **Tank:** kept. The pump fills it to a target, then stops (see 5.3).
- **Tank sensor upgrade (my call):** MPX5010DP → **MPX5050DP** (same family and pinout, 0–50 kPa ≈ 0–375 mmHg) so the
  tank can be held deeper than the chambers. The two chamber sensors stay MPX5010DP. The software conversion for ADC
  channel 1 changes to the new range.
- **Valve types (my call):** SOL1, SOL3, SOL4 **normally-closed** (power off = tank isolated and keeps its vacuum);
  SOL2, SOL5 **normally-open** (power off = both chambers vent).
- **Relief valves (my call):** ~150 mmHg on the tank; 85–100 mmHg on each chamber line. Mechanical backstop if a
  valve sticks or the software hangs.

### 5.2 Electronics

- **Pulser drive:** one spare **L293D half-bridge**, PWM input on **GPIO 15**, 12 V motor supply. *(my call)* If the
  pulser draws more than ~0.5 A, use a small logic-level MOSFET module instead.
- **PWM:** software PWM at about 1 kHz from a dedicated thread in `ActuatorControl` (libgpiod). Motor inertia smooths
  the jitter. Hardware PWM is not available without taking a channel from TENS. *(my call)*
- **No speed sensor.** Speed → pulse-rate is a **calibration table** measured on the bench (see 5.6).
- `scripts/check_gpio_pins.py` and the README pin table get GPIO 15 = pulser PWM.

### 5.3 Pressure control

**Tank (new)** *(my call on numbers)*
- Fill the tank to **120 mmHg**; switch the pump **off**; restart when the tank falls to **90 mmHg**.
- Optional "quiet top-up": instead of full speed, restart at low speed.
- The tank never feeds the body directly – only through SOL1 / SOL4.

**Each chamber (replaces "pump speed = pressure")**
- Each chamber has its own **target** (legs: 30–50 mmHg; clitoral: 0–75 mmHg minus pulse allowance, see 5.5).
- Hold the target with short **SOL1 / SOL4 openings** when below target − 2 mmHg, closing at target; vent with
  **SOL2 / SOL5** when above target + 3 mmHg. *(my call on the bands)*
- Openings are short (start with **30 ms**, then re-read and repeat) so the deep tank doesn't overshoot the chamber.
  *(my call)*
- **While the pulser runs, the clitoral regulator uses an averaged reading** (moving average over ~200 ms). The pulses
  (40–80 per second) are faster than the 100 Hz sensor loop, so raw readings would land anywhere in the swing and the
  valves would chatter chasing them. *(my call)*
- Lowering a target (e.g. releasing the clitoral chamber during edging) = vent valve opens until the new target.
- `PatternEngine::applyPressureTarget()` becomes "set leg target / set clitoral target" instead of driving the pump.

**Anti-detachment (fixed in this project, my call)** – it shares SOL1 with the new leg regulator, so it can't keep its
current numbers:
- Count it as a leak when the leg pressure is **more than 10 mmHg below the leg target**, or falls **faster than
  5 mmHg per 100 ms** (the rule already written in the unused `AdaptiveSealMonitor`).
- Respond with SOL1 openings back **to the target, never above it**; up to 3 attempts; if the seal won't hold:
  stop the pulser, vent both chambers, end the session and show why.
- Start watching only after the legs first reach their target.

### 5.4 Pulser control (replaces SOL4/SOL5 oscillation)

- `ClitoralOscillator` stops cycling SOL4/SOL5. It now:
  - turns the pulser **on/off** and sets its **speed** (pulse rate) through the calibration table;
  - makes **slow rhythms** (below what the motor can run smoothly, roughly < 20 per second) as **bursts**: pulser on
    for a set time, off for a set time;
  - leaves the steady clitoral vacuum to the chamber regulator (5.3).
- Existing presets (Warmup 5 Hz / Build-up 8 Hz / Climax 11 Hz / Afterglow 4 Hz) are re-mapped to bursts or motor
  speeds after the bench measurements; their pressure numbers become clitoral **targets**.
- Typical session (matches the Womanizer patent and the owner's plan): legs seal → clitoral chamber holds steady
  vacuum to engorge → pulser starts → rate rises; clitoral target can drop and return for edging.

### 5.5 Safety

- **Pulse allowance:** while the pulser runs, the clitoral target is capped at **75 mmHg − (measured pulse peak at
  that speed)**. Measured once on the bench, **worst case** (see 5.6), stored with the calibration table.
- **Tank has its own limit** (relief ~150, software stop at 140) – the safety check stops comparing the tank to the
  chamber limit.
- **Emergency stop / any fault:** pulser off → SOL1 and SOL4 closed → SOL2 and SOL5 open → pump off. SOL3 opens only on
  emergency stop or a manual "Vent tank" command; a normal session end keeps the tank vacuum (quiet next start).
  *(my call)*
- Chamber limits (75 mmHg cap, 80 mmHg emergency stop) unchanged and apply to the **average** reading; the pulse
  allowance above covers the peaks.
- Unchanged: 75 / 80 / 100 mmHg limits in `SafetyConstants.h`; manual vent flaps on the cup.

### 5.6 Bench calibration (before anything touches a body)

1. **Pulse rate:** run the pulser at a range of PWM settings; read the rate with a phone spectrum-analyzer app and, if
   possible, the clitoral sensor sampled fast. Record PWM → pulses per second.
2. **Pulse size, worst case:** cup sealed on a flat plate, clitoral chamber **partly filled** (e.g. a soft plug taking
   up ~70 % of the volume, standing in for an engorged clitoris – less air means bigger pulses), at several steady
   vacuum levels. Record the peak swing at each speed.
3. Measure the owner's own air-pulse toy the same way (step 1) to get the target rates.
4. Store the table in `config/` and refuse to run the pulser without one.

### 5.7 What changes in the code

| Area | Change |
|---|---|
| `ActuatorControl` | Claim GPIO 15; software-PWM thread for the pulser; `setPulserSpeed()` / `setPulserEnabled()` |
| `HardwareManager` | Pass-throughs for the pulser; tank sensor uses MPX5050DP conversion |
| `PatternEngine` | Chamber targets instead of pump speed; tank fill/stop loop; safety check with separate tank limit |
| `ClitoralOscillator` | Pulser speed + bursts instead of SOL4/SOL5 cycling |
| `AntiDetachmentMonitor` (or switch to `AdaptiveSealMonitor`) | Target-relative leak detection, stop at target, 3 attempts then safe stop |
| Emergency stop | Adds pulser off; SOL3 on e-stop only |
| `config/` | Pulser calibration table; tank thresholds |
| Docs | Plumbing picture, parts list (MPX5050DP, relief valves, pulser), pin table (GPIO 15) |

## 6. Left to the builder

- Exact software-PWM frequency and thread priority; burst timing resolution.
- Whether to fix `AntiDetachmentMonitor` in place or wire in `AdaptiveSealMonitor`.
- How the GUI shows pulser rate and the "Vent tank" button.
- Tests: simulation-mode coverage of tank fill/stop, chamber regulation, pulse allowance and e-stop ordering. Nothing is
  tested on a body; bench tests use the sealed plate.
