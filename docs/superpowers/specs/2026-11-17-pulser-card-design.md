# Clitoral Pulser Card: Pause, Hand Rate, Vent Tank — Design

Approved 2026-11-17. Builds on `2026-11-16-clitoral-pulser-system-design.md`.

## Why

The owner edges themselves: when close, they tap a big button to stop all clitoral stimulation, wait, then
bring it back. They also run sessions by hand and want to set the pulse rate themselves. The tank vent had
no button. The owner has low vision: big buttons, big numbers, high contrast.

## Goals

- **Pause / Resume** (every mode): pause stops the pulser and vents the clitoral chamber; the legs stay
  sealed. Stays paused until the owner taps Resume (no auto-resume). Resume restores the clitoral level and
  the pulses restart once the chamber is back there (the existing "pulses wait for the chamber" rule).
  **The pause wins over everything**: programs, patterns and presets cannot start the pulser or raise the
  clitoral target while paused. Their requests are remembered and applied on Resume.
- **Hand pulser session** (Manual mode, no pattern running): START PULSES seals the legs at 40 mmHg,
  sets the clitoral steady level to `hand_clit_mmhg` (default 20, from settings.json "pneumatics"),
  and starts the pulser at the hand rate. STOP PULSES turns the pulser off and vents both chambers
  (session over; the loop goes idle).
- **SLOWER / FASTER**: steps through 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 50, 60, 80, 100
  pulses/s, never above the calibrated maximum. Starts at 10, remembered while the app runs. Rates below
  the motor minimum play as bursts (existing behaviour). Gray while a program or pattern is in charge; the
  number then shows the rate actually in use.
- **VENT TANK**: calls `ventTank()`; enabled only when nothing is running.
- **Not calibrated**: card says "Pulser not calibrated"; START is gray. Pause and Vent Tank still work.

## Non-goals

Suction-level buttons, auto-resume timer, hand-held remote, rate control during programs/patterns.

## Anti-goals

- A paused pulser that something else restarts.
- Pause releasing the leg seal.
- Any path around the 75 mmHg allowance.

## Approaches

1. **Pause inside `PneumaticController` (picked).** A user-hold flag: while set, the pulser output and the
   clitoral target are forced to off/0 regardless of who asks; requests are stored and come back on
   Resume. One place, enforced at the 100 Hz loop, testable with FakePlant.
2. Pause in the GUI / ClitoralOscillator — rejected: OrgasmControlAlgorithm and PatternEngine re-enable the
   pulser through their own oscillators, so the pause would be undone.
3. Pause by stopping the running program — rejected: changes program state and loses the session.

## Decided

- Card name "CLITORAL PULSER", on the main dashboard next to SYSTEM CONTROL.
- Running = green PAUSE PULSES; paused = amber RESUME.
- Hand session legs 40 mmHg; clitoral default 20 mmHg (capped by the pulse allowance as usual).

## Left to the builder

- Exact grid placement and sizes (use ModernMedicalStyle large sizes).
- How "a program or pattern is in charge" is detected (algorithm running / PatternEngine running).
- How the card refreshes (signals and/or a short timer).
