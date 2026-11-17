# Clitoral Pulser Card Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A big "CLITORAL PULSER" card on the main screen with PAUSE/RESUME (every mode), a hand-run pulser session with SLOWER/FASTER, and VENT TANK.

**Architecture:** The pause is a user-hold inside `PneumaticController`, so nothing else can undo it. A small `HandPulser` class (src/pneumatics) runs the hand session on top of the controller. `PulserCard` (src/gui) is the touch widget. `MainWindow` places it and tells it when a program or pattern is in charge.

**Tech Stack:** C++17, Qt5 (Core, Widgets, Test), CMake, existing PneumaticsLogic / VacuumControllerLib targets.

**Spec:** `docs/superpowers/specs/2026-11-17-pulser-card-design.md`

## Global Constraints

- Pause: pulser off + clitoral chamber vented; legs untouched. Stays paused until Resume; no auto-resume.
- While paused, programs, patterns and presets cannot start the pulser or raise the clitoral target. Their requests are kept and applied on Resume.
- Resume: clitoral level restored; pulses restart once the chamber is back at its level.
- Hand session: legs 40 mmHg; clitoral `hand_clit_mmhg` (default 20); rate starts at 10.
- Rate steps: 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 50, 60, 80, 100 pulses/s, never above the calibrated maximum.
- Rate buttons gray while a program or pattern is in charge; the number then shows the rate in use.
- VENT TANK enabled only when nothing is running.
- Not calibrated: "Pulser not calibrated", START gray; Pause and Vent Tank still work.
- Running pause button green "PAUSE PULSES"; paused amber "RESUME".
- No path around the 75 mmHg allowance; never release the leg seal on pause.

## Review Focus

1. **Resume when the clitoral level can't be reached** (tank low, leak): pulses must not wait forever. Expect them to start after at most 3 s. Test: Task 1 `resumeStartsPulsesAfterTimeout`.
2. **Pause pressed during an emergency-stopped or idle machine**: it must do nothing harmful and must not start the loop. Test: Task 1 `pauseWhileIdleDoesNotStart`.
3. **A pattern starts while a hand session runs**: the card must gray the rate buttons and show the pattern's rate; the pause must still work. Test: Task 3 `automationGraysRateButtons`.
4. **The session ends while paused** (Stop, emergency stop): the next session must not start paused and silent. Test: Task 1 `sessionEndClearsPause`.
5. **The calibrated maximum is below the current hand rate** (for example, re-calibrated at 40 while the rate is 60): FASTER must not go above the maximum, and START must use a clamped rate. Test: Task 2 `rateClampedToCalibratedMax`.

---

### Task 1: User pause in PneumaticController

**Files:**
- Modify: `src/pneumatics/PneumaticController.h`, `src/pneumatics/PneumaticController.cpp`
- Test: `tests/pneumatics/tst_pneumaticcontroller.cpp`

**Interfaces:**
- Produces: `void setUserPaused(bool paused)`, `bool isUserPaused() const`, signal `void userPauseChanged(bool paused)`, `double maxPulseRate() const` (calibration max rate, or 100 when not calibrated), `const PneumaticSettings& settings() const`.

Behaviour:
- While paused, `applyClitoralTarget()` sets the regulator target to 0, and `tick()` forces the pulser output off (bench duty too).
- `isPulserEnabled()` and `m_requestedClit` keep the requested state, so oscillators don't fight the pause.
- On Resume, the requested clitoral target comes back and a resume-wait is armed. Pulses stay off until the clitoral 200 ms mean is ≥ target − `feedBelowMmHg`, or 3000 ms have passed. If the target is 0 there is no wait.
- `ventAll()`, `stop()` and `emergencyStop()` clear the pause (emitting `userPauseChanged(false)`).
- `setUserPaused` never starts the loop.

- [ ] **Step 1: Write the failing tests** (use the file's `writeCalibration` and `run` helpers)
  - `pauseStopsPulsesVentsClitKeepsLegs`: calibrated; legs 40, clit 30, rate 20, pulser on; run 3000. `setUserPaused(true)`; run 1500. Expect `plant.duty == 0`, `plant.clit < 3`, `plant.leg` in [38, 43], `c.isPulserEnabled()` still true.
  - `requestsWhilePausedWaitForResume`: paused first, then `setClitoralTarget(30)` and `setPulserEnabled(true)`; run 2000. Expect `c.clitoralTarget() == 0`, `plant.clit < 3`, no `duty [1-9]` in `plant.log`.
  - `resumeRestoresLevelThenPulses`: from the paused state of the first test, `setUserPaused(false)`, then tick and step, recording any tick where `plant.duty > 0` while `plant.clit < 28`. Expect none, and `plant.duty > 0` by 4000 ms.
  - `resumeStartsPulsesAfterTimeout`: as above, but set `plant.pumpRate = 0` and `plant.tank = 10` before Resume. Expect `plant.duty > 0` within 3500 ms.
  - `pauseWhileIdleDoesNotStart`: fresh controller; `setUserPaused(true)`. Expect `!c.isActive()`, and `plant.log` empty.
  - `sessionEndClearsPause`: pause, then `ventAll()`. Expect `!c.isUserPaused()` and one `userPauseChanged(false)`. Repeat with `emergencyStop()`.
- [ ] **Step 2: Run** `make -j8 tst_pneumaticcontroller && ./tests/pneumatics/tst_pneumaticcontroller`. Expected: the build fails, because `setUserPaused` is not declared.
- [ ] **Step 3: Implement** the interfaces above. The resume-wait is `qint64 m_resumeWaitUntil` plus a flag, evaluated in `tick()` next to the existing ceiling check.
- [ ] **Step 4: Run** `ctest -R pneumatics`. Expected: all pass.
- [ ] **Step 5: Commit** `feat(pneumatics): user pause that wins over programs (pulser off, clitoral vented, legs kept)`

### Task 2: HandPulser

**Files:**
- Create: `src/pneumatics/HandPulser.h`, `src/pneumatics/HandPulser.cpp` (add to `src/pneumatics/CMakeLists.txt`)
- Modify: `src/pneumatics/PneumaticSettings.h` (add `double handClitMmHg = 20;`, JSON key `hand_clit_mmhg`, clamped to [0, clitMaxMmHg])
- Modify: `src/hardware/HardwareManager.h/.cpp` (own one `HandPulser`, created in `createPneumatics()`, getter `HandPulser* handPulser() const`)
- Test: `tests/pneumatics/tst_handpulser.cpp` (register with `pneumatics_test(handpulser)`)

**Interfaces:**
- Consumes: Task 1 `maxPulseRate()`, `settings()`, plus the existing `setLegTarget`, `setClitoralTarget`, `setPulseRate`, `setPulserEnabled`, `ventAll`, `hasCalibration`, `isLatched`, `isPulserEnabled`.
- Produces: `class HandPulser : public QObject` with:
  - `explicit HandPulser(PneumaticController* air, QObject* parent = nullptr)`
  - `static const QVector<double>& rateSteps()`
  - `double rate() const`
  - `void slower()`, `void faster()`
  - `bool canStart() const` (calibrated and not latched)
  - `bool start()`, `void stop()`
  - `bool isRunning() const`, true only while it started the pulser and `air->isPulserEnabled()` is still true
  - signal `void changed()`

Behaviour:
- `rate()` starts at 10.
- `faster()` and `slower()` move one step and never go above `maxPulseRate()`. A rate that is already above the maximum clamps down to the highest step allowed.
- Changing the rate while running calls `air->setPulseRate`.
- `start()` sets legs 40, clitoral `settings().handClitMmHg`, rate, then `setPulserEnabled(true)`. It returns false, and changes nothing, when `!canStart()`.
- `stop()` calls `setPulserEnabled(false)`, then `ventAll()`.

- [ ] **Step 1: Write the failing tests** (FakePlant, calibration 20→8 and 60→20 as in tst_pneumaticcontroller)
  - `stepsUpAndDown`: from 10, `faster()` gives 12; `slower()` twice gives 8; 18 `slower()` presses end at 1.
  - `rateClampedToCalibratedMax`: max 60; 30 `faster()` presses end at 60. Load a calibration with a 40 max: the next `faster()` gives 40, and `start()` sets `air->pulseRate() == 40`.
  - `startSetsLegsClitAndPulser`: `start()` is true; `legTarget() == 40`; clitoral target `== 20`; `isPulserEnabled()`; `isRunning()`.
  - `startRefusedWithoutCalibration`: `start()` is false; `legTarget() == 0`.
  - `stopVentsBoth`: start, run 2000, `stop()`; run 2000. Expect `plant.leg < 3`, `plant.clit < 3`, `!isRunning()`.
  - `notRunningAfterEmergencyStop`: start, then `air->emergencyStop()`. Expect `!isRunning()`.
- [ ] **Step 2: Run** `make -j8 tst_handpulser`. Expected: the build fails, because `HandPulser.h` is missing.
- [ ] **Step 3: Implement** `HandPulser`, the setting, and the HardwareManager ownership.
- [ ] **Step 4: Run** `ctest -R pneumatics`. Expected: all pass.
- [ ] **Step 5: Commit** `feat(pneumatics): hand pulser session with stepped rate`

### Task 3: PulserCard widget

**Files:**
- Create: `src/gui/PulserCard.h`, `src/gui/PulserCard.cpp` (add to the GUI source list in the top-level `CMakeLists.txt`, next to `src/gui/MainWindow.cpp`)
- Test: `tests/pneumatics/tst_pulsercard.cpp`. Same pattern as `tst_hardwarepneumatics` (links VacuumControllerLib, plus Qt5::Widgets). Use `QTEST_MAIN`, and set `QT_QPA_PLATFORM=offscreen` through the `add_test` ENVIRONMENT property.

**Interfaces:**
- Consumes: Task 1 pause API, Task 2 `HardwareManager::handPulser()`, `PneumaticController::isActive()/pulseRate()/hasCalibration()/ventTank()`.
- Produces: `class PulserCard : public QWidget` with:
  - `PulserCard(HardwareManager* hw, std::function<bool()> automationActive, QWidget* parent = nullptr)`
  - `void refresh()`, called from a 200 ms QTimer and after every button press

Child object names (the tests find buttons and labels by these): `startStopPulses`, `slowerButton`, `fasterButton`, `rateLabel`, `pauseButton`, `ventTankButton`, `statusLabel`.

Copy and state:
- **Start/stop button:** "START PULSES" / "STOP PULSES".
- **Pause button:** "PAUSE PULSES" with the `success` style; "RESUME" with the `warning` style.
- **Vent tank button:** "VENT TANK".
- **Rate label:** "%1 /s". Show whole numbers without decimals. While automation is in charge, show `air->pulseRate()`; otherwise show `handPulser()->rate()`.
- **Status label:**
  - "Pulser not calibrated" when it's not calibrated.
  - "Paused" while paused.
  - "Program in charge" when automation is active.
  - Empty otherwise.

Enabled rules:
- **START:** `canStart() && !automation`. When running, STOP is always enabled.
- **SLOWER / FASTER:** `!automation`.
- **PAUSE:** always enabled.
- **VENT TANK:** `!air->isActive()`.

Sizes: use `ModernMedicalStyle::scaleValue` the same way the System Control buttons do (minimum 450×180 for PAUSE, and at least 300×150 for the others). The rate label uses `ModernMedicalStyle::Typography::getTitle()` or larger.

- [ ] **Step 1: Write the failing tests** (HardwareManager in simulation mode; write a valid calibration into a `QTemporaryDir` with `VACUUM_CONFIG_DIR` set, as tst_clitoraloscillator does)
  - `notCalibratedGraysStart`: no calibration. `startStopPulses` is disabled; `statusLabel` text is "Pulser not calibrated"; `pauseButton` and `ventTankButton` are enabled.
  - `startAndStop`: calibrated. Click start: `handPulser()->isRunning()`, and the button text is "STOP PULSES". Click again: not running, and the text is "START PULSES".
  - `fasterUpdatesLabel`: click `fasterButton`. `rateLabel` text is "12 /s".
  - `pauseTogglesTextAndController`: click pause. `pneumatics()->isUserPaused()`, and the text is "RESUME". Click again: not paused, and the text is "PAUSE PULSES".
  - `automationGraysRateButtons`: the automation lambda returns true. Slower, faster and start are disabled; pause is enabled; status is "Program in charge"; `rateLabel` shows `pneumatics()->pulseRate()`.
  - `ventTankOnlyWhenIdle`: idle means enabled. After a hand start, after `refresh()`, it is disabled.
- [ ] **Step 2: Run** `make -j8 tst_pulsercard`. Expected: the build fails, because `PulserCard.h` is missing.
- [ ] **Step 3: Implement** `PulserCard`.
- [ ] **Step 4: Run** `ctest -R pneumatics`. Expected: all pass.
- [ ] **Step 5: Commit** `feat(gui): clitoral pulser card (pause, hand rate, vent tank)`

### Task 4: Put the card on the main screen

**Files:**
- Modify: `src/gui/MainWindow.h`, `src/gui/MainWindow.cpp` (new `setupPulserCard(QGridLayout*)`, called from `setupMainPanel()` after `setupControlPanelCard`)

**Interfaces:**
- Consumes: Task 3 `PulserCard`.

What to build:
- The card goes into `createDashboardCard("CLITORAL PULSER", card)` at grid cell (2, 2), and row 2 stretch becomes 2. It only appears when `m_controller->getHardwareManager()` is non-null.
- Automation lambda: true when `getOrgasmControlAlgorithm()->getState() != ControlState::STOPPED`, or when `getPatternEngine()->getState()` is not `PatternEngine::STOPPED`. Null pointers count as false.

- [ ] **Step 1: Build** the whole tree with `make -j8`. Expected: no new warnings in the touched files.
- [ ] **Step 2: Smoke run.** Run `QT_QPA_PLATFORM=offscreen timeout 5 ./VacuumController` (simulation if no hardware) from the build folder. Expected: it exits through the timeout (code 124), with no crash or assertion, and the log has no "PulserCard" errors.
- [ ] **Step 3: Run** `ctest -R pneumatics`. Expected: all pass.
- [ ] **Step 4: Commit** `feat(gui): clitoral pulser card on the main dashboard`
