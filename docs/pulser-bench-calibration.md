# Pulser Bench Calibration

The clitoral pulser **will not run** until it has been measured on the bench and the numbers saved in
`config/pulser_calibration.json`. This is how the machine knows which motor setting gives which pulse rate,
and how big the pulses are, so it can keep **steady vacuum + pulse peak under 75 mmHg**.

> **Never do this with the cup on a body.** Everything here is done with the cup sealed on a flat plate.

## What you need

- The cup sealed on a smooth flat plate (glass or acrylic), seal lip lightly lubricated.
- A **soft plug** in the clitoral chamber filling about **70 %** of it (for example a rolled-up piece of
  silicone or foam). It stands in for an engorged clitoris. Less air in the chamber means bigger pulses,
  so this gives the **worst case** — the safe number to use.
- A phone with a spectrum-analyzer app (Spectroid on Android, phyphox on Android/iPhone).
- The pulser pump wired to GPIO 15 and connected to the clitoral line (both ports on a T near the cup).

## Steps

1. **Build** the project; the tool is `build/PulserBench`.
2. **Run it** with the cup on the plate:
   ```bash
   ./build/PulserBench --steady 10,20,35 --duty 30,40,50,60,70,80,90,100 --seconds 10
   ```
   It holds the legs at 30 mmHg, then for **each steady clitoral level** (10, 20, then 35 mmHg) runs the
   pulser at each duty for 10 seconds and prints the measured **swing** at each step. Pulse size changes
   with how much vacuum is already in the chamber, so testing several levels finds the worst case. If
   steady + swing would pass 75 mmHg it stops the pulser, skips the higher duties at that level and moves
   on. At the end it prints the **worst swing per duty** across all levels. **Ctrl-C** is an emergency
   stop.
3. **Read the pulse rate** at each step: hold the phone near the pulser; the strongest peak in the app is the
   motor hum = pulses per second. Write it next to the duty.
4. **Measure your own air-pulse toy** the same way (phone app) at each of its levels — those are the rates
   you'll want to copy.
5. **Write the file.** Copy `config/pulser_calibration.example.json` to `config/pulser_calibration.json` and
   replace the points with your numbers, lowest duty first:
   ```json
   {
     "measured": true,
     "measured_on": "2026-11-20",
     "pump": "YOGULGUL 12 V mini air pump, valves removed",
     "points": [
       {"pwm_percent": 40, "pulses_per_second": 31, "peak_mmhg": 9.5},
       {"pwm_percent": 70, "pulses_per_second": 52, "peak_mmhg": 13.0}
     ]
   }
   ```
   - `peak_mmhg` = the **worst swing per duty** from the summary at the end. (It is the full up-and-down swing, which is bigger than
     the peak above the steady level — so the limit it gives is on the safe side.)
   - Every point must have a higher duty **and** a higher rate than the one before.
   - Leave out duties where the motor stalls or the rate doesn't rise.
6. **Restart** the app. The log says "Pulser calibration loaded". The app finds `config/` next to its
   program folder (or set `VACUUM_CONFIG_DIR` to the folder holding the file), so it works the same when
   started as a service. `"measured": true` is required — the example
   file is always refused.

## What the numbers do

- **Rates between your points** get an in-between motor setting, but the **bigger** of the two neighbouring
  pulse sizes (the safe side). Rates **below** your lowest point are played as slow
  **bursts** (pulser on for half of each beat, off for the other half).
- While the pulser runs, the clitoral steady vacuum is capped at **75 mmHg − the measured swing** at that rate.
- Re-measure after changing the pump, the tubing between pump and cup, or the cup.

`config/pulser_calibration.json` is per machine and is not committed to git.
