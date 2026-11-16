# Clitoral Pulser Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace SOL4/SOL5 pulse-making with a second diaphragm pump ("pulser") on GPIO 15, and replace "pump speed = pressure" with a quiet fill-then-stop tank plus per-chamber target regulation, a target-relative seal watch and a pulse allowance under the 75 mmHg cap.

**Architecture:** All decision-making goes into small, hardware-free classes in `src/pneumatics/` (built as a static library `PneumaticsLogic`, unit-tested with Qt Test against a fake plant). One QObject, `PneumaticController`, runs them on a 100 Hz timer against an `IPneumaticHardware` interface; `HardwareManager` owns the controller and provides the real adapter. Existing callers (`PatternEngine`, `OrgasmControlAlgorithm`, `ClitoralOscillator`) stop touching SOL1/2/4/5 and the pump directly and call the controller instead.

**Tech Stack:** C++17, Qt 5.15 (Core, Test), libgpiod 2.x (C++ bindings), CMake ≥ 3.16.

**Spec:** `docs/superpowers/specs/2026-11-16-clitoral-pulser-system-design.md`

## Global Constraints

- Steady vacuum + pulse peak never above **75 mmHg**; `SafetyConstants.h` limits (75 cap / 80 e-stop / 100 absolute) unchanged.
- Leg target range **30–50 mmHg** (0 = released); clitoral target **0–60 mmHg**, further capped by the pulse allowance while the pulser runs.
- Tank: fill to **70 mmHg**, pump off, restart at **55 mmHg**, or when a chamber needs topping up and the tank is not at least **5 mmHg** deeper than that chamber's target.
- Chamber regulation: open feed valve when (average) pressure < target − **2 mmHg**, in **30 ms** openings with a re-read between; vent when > target + **3 mmHg**, close vent at ≤ target.
- While the pulser runs the clitoral regulator uses a **~200 ms moving average**.
- Seal watch: leak = leg pressure more than **10 mmHg below the leg target**, or falling faster than **5 mmHg per 100 ms**; armed only after the legs first reach target; **3** recovery attempts (2 s each), then safe stop.
- Pulser PWM on **GPIO 15**, software PWM ≈ **1 kHz**; no speed sensor; rate comes from a measured calibration table, and **the pulser refuses to run without one**.
- Emergency stop / any fault: **pulser off → SOL1 & SOL4 closed → SOL2 & SOL5 open → pump off**, SOL3 open. Normal session end keeps tank vacuum (SOL3 closed).
- Nothing is ever tested on a body. Bench tests use the cup sealed on a plate.
- Build/test command used throughout:
  `cmake -S . -B build -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF && cmake --build build -j$(nproc) && ctest --test-dir build -R pneumatics --output-on-failure`

## Review Focus

1. **Missing, unmeasured or garbled calibration file** → pulser stays off with a clear message; vacuum functions keep working. *(Task 1, Task 6)*
2. **Tank sensor pinned at its 75 mmHg top** (pump stronger than expected) → treated as full, pump off. *(Task 2)*
3. **Pulse rate raised while the clitoral target sits near the allowance** → target re-clamped down immediately. *(Task 6)*
4. **Leg target lowered by a pattern (50 → 30)** → not reported as a leak. *(Task 4)*
5. **Emergency stop while a feed valve is open mid-pulse** → pulser off first, SOL1/SOL4 closed in the same call. *(Task 6)*

---

### Task 1: Pneumatics test target + `PulserCalibration`

**Files:**
- Create: `src/pneumatics/PulserCalibration.h`, `src/pneumatics/PulserCalibration.cpp`
- Create: `src/pneumatics/CMakeLists.txt` (static lib `PneumaticsLogic`, Qt5::Core, include dir `src/pneumatics`)
- Create: `tests/pneumatics/CMakeLists.txt`, `tests/pneumatics/tst_pulsercalibration.cpp`
- Modify: `CMakeLists.txt` – `option(BUILD_PNEUMATICS_TESTS "Pneumatics unit tests" ON)`; `add_subdirectory(src/pneumatics)` before `VacuumControllerLib`; link `PneumaticsLogic` into `VacuumControllerLib` (PUBLIC); `if(BUILD_PNEUMATICS_TESTS) enable_testing() add_subdirectory(tests/pneumatics) endif()` (independent of `BUILD_TESTS`, whose data files are missing).

**Interfaces:**
- Produces:
  - `struct CalibrationPoint { double pwmPercent; double pulsesPerSecond; double peakMmHg; };`
  - `class PulserCalibration` with `bool loadFromFile(const QString& path, QString* error = nullptr)`, `bool loadFromJson(const QJsonObject& obj, QString* error = nullptr)`, `bool isValid() const`, `double minRate() const` (pps of first point), `double maxRate() const`, `double pwmForRate(double pps) const` (linear interpolation, clamped to the table), `double peakAtRate(double pps) const` (interpolated; below `minRate()` returns the first point's peak), `double maxClitoralTarget(double pps) const` = `std::max(0.0, 75.0 - peakAtRate(pps))`.
  - JSON format: `{ "measured": true, "measured_on": "YYYY-MM-DD", "pump": "...", "points": [ {"pwm_percent": 40, "pulses_per_second": 30, "peak_mmhg": 12}, ... ] }`
  - Each test file is a Qt Test executable `tst_<name>` registered with `add_test(NAME pneumatics_<name> ...)`.

- [ ] **Step 1: Write the failing tests** in `tst_pulsercalibration.cpp`

```cpp
void loadsValidTable();        // 3 points 30/50/70 % → 20/40/60 pps, peaks 8/10/14: isValid, minRate()==20, maxRate()==60
void interpolates();           // pwmForRate(30)==40, peakAtRate(50)==12, maxClitoralTarget(60)==61
void clampsOutsideTable();     // pwmForRate(5)==30, pwmForRate(200)==70, peakAtRate(5)==8
void rejectsUnmeasured();      // "measured": false → loadFromJson false, isValid false, error mentions "measured"
void rejectsTooFewPoints();    // 1 point → false
void rejectsNonIncreasing();   // pps 20, 20 → false
void rejectsMissingFile();     // loadFromFile("/nonexistent.json") false, error non-empty
void rejectsGarbage();         // file containing "not json" → false
```

- [ ] **Step 2: Run to verify they fail**
Run: build/test command. Expected: build error (`PulserCalibration.h` not found).

- [ ] **Step 3: Implement `PulserCalibration`** (QJsonDocument parsing; points sorted as given and validated strictly increasing in both pwm and pps; peaks ≥ 0).

- [ ] **Step 4: Run tests** – Expected: `pneumatics_pulsercalibration` PASS; main app still builds.

- [ ] **Step 5: Commit** – `feat(pneumatics): pulser calibration table + pneumatics test target`

---

### Task 2: `PneumaticSettings` + `TankLogic`

**Files:**
- Create: `src/pneumatics/PneumaticSettings.h` (header-only struct), `src/pneumatics/TankLogic.h/.cpp`
- Test: `tests/pneumatics/tst_tanklogic.cpp`
- Modify: `config/settings.json` – add `"pneumatics"` section with the defaults below.

**Interfaces:**
- Produces:
  - `struct PneumaticSettings { double tankFillMmHg = 70; double tankRestartMmHg = 55; double topUpMarginMmHg = 5; double legMinMmHg = 30; double legMaxMmHg = 50; double clitMaxMmHg = 60; double feedBelowMmHg = 2; double ventAboveMmHg = 3; int feedPulseMs = 30; int averageWindowMs = 200; double leakDropMmHg = 10; double leakRateMmHgPer100ms = 5; int recoveryAttempts = 3; int recoveryWindowMs = 2000; bool benchMode = false; static PneumaticSettings fromJson(const QJsonObject& pneumaticsSection); };` (missing keys keep defaults; JSON keys snake_case of the fields, e.g. `tank_fill_mmhg`).
  - `class TankLogic { explicit TankLogic(const PneumaticSettings&); bool update(double tankMmHg, double highestChamberDemandMmHg); bool isFilling() const; };` – returns desired pump state. `highestChamberDemandMmHg` is the target of any chamber currently below its feed threshold, else 0.

- [ ] **Step 1: Failing tests**

```cpp
void fillsFromEmpty();          // update(0,0) → true
void stopsAtFill();             // filling, update(70,0) → false
void hysteresis();              // after stop: update(60,0) false; update(55,0) true
void topUpWhenTankNotDeeper();  // idle at 62, chamber demand 60 → true (62 < 60+5)
void noTopUpWhenTankDeeper();   // idle at 68, demand 50 → false
void saturatedSensorIsFull();   // update(75,0) → false   (Review Focus 2)
void settingsFromJson();        // {"tank_fill_mmhg":65} → tankFillMmHg 65, others default
```

- [ ] **Step 2: Run** – Expected: FAIL (missing types).
- [ ] **Step 3: Implement** – two-state hysteresis (filling / idle); stop when `tankMmHg >= tankFillMmHg`.
- [ ] **Step 4: Run** – Expected: PASS.
- [ ] **Step 5: Commit** – `feat(pneumatics): tank fill-then-stop logic and settings`

---

### Task 3: `ChamberRegulator`

**Files:**
- Create: `src/pneumatics/ChamberRegulator.h/.cpp`
- Test: `tests/pneumatics/tst_chamberregulator.cpp`

**Interfaces:**
- Consumes: `PneumaticSettings` (Task 2).
- Produces:
  - `struct ValveCommand { bool feedOpen = false; bool ventOpen = false; };`
  - `class ChamberRegulator { ChamberRegulator(const PneumaticSettings&); void setTarget(double mmHg); double target() const; void setAveraging(bool on); ValveCommand update(double pressureMmHg, qint64 nowMs); bool needsTopUp() const; double lastAverage() const; void reset(); };`
  - Behaviour: target 0 → vent open, feed closed. Below `target − feedBelow`: feed opens for `feedPulseMs`, then one closed tick (re-read) before another opening. Above `target + ventAbove`: vent open until pressure ≤ target. Feed and vent never open together. With averaging on, decisions use the mean of samples within the last `averageWindowMs`.

- [ ] **Step 1: Failing tests**

```cpp
void opensFeedBelowBand();      // target 40, p 37 at t=0 → feedOpen
void feedPulseIs30ms();         // feed true at t=0,10,20; false at t=30 (re-read tick) even if still low
void holdsInsideBand();         // target 40, p 39.5 → neither open
void ventsAboveBandUntilTarget(); // p 44 → vent; p 41 → vent; p 40 → closed
void zeroTargetVents();         // target 0, p 20 → vent open, feed closed
void neverBothOpen();           // sweep p 0..80 with target 40: never feedOpen && ventOpen
void averagingIgnoresPulses();  // averaging on, target 40, samples alternating 30/50 every 10 ms for 300 ms → no feed and no vent
void needsTopUpReflectsDemand(); // p 30, target 40 → true; p 40 → false
```

- [ ] **Step 2: Run** – FAIL. **Step 3: Implement.** **Step 4: Run** – PASS.
- [ ] **Step 5: Commit** – `feat(pneumatics): per-chamber target regulator`

---

### Task 4: `SealWatch` (replaces the anti-detachment thresholds)

**Files:**
- Create: `src/pneumatics/SealWatch.h/.cpp`
- Test: `tests/pneumatics/tst_sealwatch.cpp`

**Interfaces:**
- Consumes: `PneumaticSettings`.
- Produces: `enum class SealState { WaitingForSeal, Sealed, Recovering, Failed };` and `class SealWatch { SealWatch(const PneumaticSettings&); SealState update(double legMmHg, double legTarget, qint64 nowMs); SealState state() const; int attemptsUsed() const; void reset(); };`
  - Arms when `legMmHg >= legTarget - feedBelow` with `legTarget > 0`. Leak = pressure more than `leakDropMmHg` below target, or a fall faster than `leakRateMmHgPer100ms` measured over the last 100 ms. A leak starts an attempt (`Recovering`); back within `feedBelow` of target within `recoveryWindowMs` → `Sealed`; else next attempt; after `recoveryAttempts` failed attempts → `Failed` (sticky until `reset()`). When the target drops, the "fall" check uses the new target, and a fall that only follows a commanded target drop is not a leak. `legTarget == 0` → `WaitingForSeal`.
  - The watch never commands valves; refilling is the regulator's job.

- [ ] **Step 1: Failing tests**

```cpp
void waitsUntilFirstSeal();     // target 40, p rising 0→37 → WaitingForSeal; p 38.5 → Sealed
void slowDropBeyond10IsLeak();  // sealed at 40, p 29 → Recovering
void fastFallIsLeak();          // sealed, p 40 at t, 34 at t+100 ms → Recovering
void recovers();                // Recovering, p 39 within 2 s → Sealed, attemptsUsed()==1
void failsAfterThreeAttempts(); // three 2 s windows without recovery → Failed, stays Failed
void loweredTargetIsNotLeak();  // sealed at 50, target→30, p falls 50→31 over 300 ms → never Recovering  (Review Focus 4)
void zeroTargetDisarms();       // target 0 → WaitingForSeal
```

- [ ] **Step 2: Run** – FAIL. **Step 3: Implement.** **Step 4: Run** – PASS.
- [ ] **Step 5: Commit** – `feat(pneumatics): target-relative seal watch`

---

### Task 5: `PulserDrive` (rate → continuous or bursts) + `SoftwarePwm`

**Files:**
- Create: `src/pneumatics/PulserDrive.h/.cpp`, `src/pneumatics/SoftwarePwm.h/.cpp`
- Test: `tests/pneumatics/tst_pulserdrive.cpp`, `tests/pneumatics/tst_softwarepwm.cpp`

**Interfaces:**
- Consumes: `PulserCalibration` (Task 1).
- Produces:
  - `struct PulserOutput { bool on = false; double pwmPercent = 0; };`
  - `class PulserDrive { explicit PulserDrive(const PulserCalibration*); void setRate(double pulsesPerSecond); double rate() const; void setEnabled(bool); bool isEnabled() const; bool isBursting() const; PulserOutput update(qint64 nowMs); };` – rate ≥ `minRate()` → continuous at `pwmForRate(rate)`; 0 < rate < `minRate()` → bursts at the `minRate()` PWM, period `1000/rate` ms, 50 % on; invalid calibration or disabled → off.
  - `class SoftwarePwm { SoftwarePwm(std::function<void(bool)> write, int periodUs = 1000); ~SoftwarePwm(); void start(); void stop(); void setDuty(double percent); double duty() const; };` – own `std::thread`; `stop()` and the destructor always leave the line written `false`; duty 0 = never high, 100 = always high.

- [ ] **Step 1: Failing tests**

```cpp
// tst_pulserdrive
void continuousAboveMinRate();  // table 20..60 pps; setRate(40) → on, pwm == pwmForRate(40), isBursting false
void burstsBelowMinRate();      // setRate(5): on during [0,100) ms, off during [100,200), period 200 ms
void offWhenDisabled();         // setEnabled(false) → on false
void offWithoutCalibration();   // invalid calibration, enabled, rate 40 → on false
// tst_softwarepwm
void dutyThirtyPercent();       // fake write logs timestamps; 200 ms at 30 % → measured on-fraction 0.25–0.35
void zeroNeverHigh();           // duty 0 → write(true) never called
void stopLeavesLow();           // running at 80 %, stop() → last write was false
```

- [ ] **Step 2: Run** – FAIL. **Step 3: Implement.** **Step 4: Run** – PASS.
- [ ] **Step 5: Commit** – `feat(pneumatics): pulser drive and software PWM`

---

### Task 6: `PneumaticController` + `IPneumaticHardware`

**Files:**
- Create: `src/pneumatics/IPneumaticHardware.h`, `src/pneumatics/PneumaticController.h/.cpp` (QObject, `Q_OBJECT`, in `PneumaticsLogic`; enable `AUTOMOC` there)
- Test: `tests/pneumatics/tst_pneumaticcontroller.cpp`, `tests/pneumatics/FakePlant.h`

**Interfaces:**
- Consumes: Tasks 1–5.
- Produces:
  - `class IPneumaticHardware { public: virtual ~IPneumaticHardware() = default; virtual double legMmHg() = 0; virtual double tankMmHg() = 0; virtual double clitMmHg() = 0; virtual void setPump(bool on) = 0; virtual void setValve(int sol, bool open) = 0; /* sol 1..5 */ virtual void setPulserDuty(double percent) = 0; };`
  - `class PneumaticController : public QObject` with: `PneumaticController(IPneumaticHardware*, PneumaticSettings, QObject* parent = nullptr)`; `bool loadCalibration(const QString& path)`; `bool hasCalibration() const`; `void start()` (100 Hz `QTimer`); `void tick(qint64 nowMs)` (public for tests; the timer calls it); `void setLegTarget(double mmHg)` (clamped to 0 or [legMin, legMax]); `void holdLegs()` (leg target = last non-zero target, default 40); `void setClitoralTarget(double mmHg)`; `double clitoralTarget() const`; `void setPulseRate(double pps)`; `bool setPulserEnabled(bool on)` (false + `pulserUnavailable(QString)` signal when no calibration); `void ventAll()` (targets 0, pulser off; SOL3 untouched); `void ventTank()` (manual: SOL3 open until tank < 5 mmHg); `void emergencyStop()`; `bool isSealRecovering() const`; signals `sealFailed(QString)`, `sealStateChanged(SealState)`, `pulserUnavailable(QString)`, `pulseBurstChanged(bool on)`.
  - Clitoral clamp: `min(clitMax, pulser running ? calibration.maxClitoralTarget(rate) : 75)`, re-applied whenever the rate or pulser state changes.
  - `tick()`: read sensors → `SealWatch` on legs (on `Failed`: `emergencyStop()`, emit `sealFailed`) → both regulators (clitoral averaging on while pulser runs) → `TankLogic` with the highest demand → write pump, SOL1/2 (legs), SOL4/5 (clitoral), pulser duty from `PulserDrive`.
  - `emergencyStop()` order (write log in tests): `setPulserDuty(0)`, SOL1 false, SOL4 false, SOL2 true, SOL5 true, `setPump(false)`, SOL3 true; timer stopped.

- [ ] **Step 1: Failing tests** (FakePlant: feed open moves the chamber 30 % toward tank per 10 ms; vent halves it; pump adds 3 mmHg per tick to the tank; records every `setValve`/`setPump`/`setPulserDuty` call in order)

```cpp
void fillsTankThenStopsPump();      // 3 s of ticks, targets 0 → tank ends 70–75, last setPump(false)
void holdsLegAndClitIndependently(); // leg 40, clit 25 → after 5 s: leg 38–43, clit 23–28
void clitReleaseKeepsLegs();         // then clit target 0 → clit < 3 within 1 s, leg still 38–43
void pulserNeedsCalibration();       // no calibration: setPulserEnabled(true) == false, pulserUnavailable emitted, duty stays 0
void allowanceClampsTarget();        // peak 20 at 60 pps; clit target 60, pulser on at 60 pps → clitoralTarget()==55
void raisingRateReclamps();          // target 60 at 20 pps (peak 8 → 60 allowed); setPulseRate(60) → target 55  (Review Focus 3)
void sealFailureStops();             // legs sealed, then plant leaks faster than feed for 8 s → sealFailed emitted, emergencyStop order logged
void emergencyStopOrder();           // SOL1 open mid feed pulse, emergencyStop() → log == [duty 0, SOL1 false, SOL4 false, SOL2 true, SOL5 true, pump false, SOL3 true]  (Review Focus 5)
void sessionEndKeepsTank();          // ventAll() → SOL3 never written true
void legTargetClamped();             // setLegTarget(70) → 50; setLegTarget(10) → 30; setLegTarget(0) → 0
```

- [ ] **Step 2: Run** – FAIL. **Step 3: Implement.** **Step 4: Run** – all `pneumatics_*` PASS.
- [ ] **Step 5: Commit** – `feat(pneumatics): pneumatic controller tying tank, chambers, seal watch and pulser`

---

### Task 7: Hardware wiring – GPIO 15 pulser, `HardwareManager` owns the controller

**Files:**
- Modify: `src/hardware/ActuatorControl.h/.cpp` – `static const int GPIO_PULSER_PWM = 15;` added to the requested output offsets; member `std::unique_ptr<SoftwarePwm> m_pulserPwm` writing GPIO 15 through the existing line request; `void setPulserDuty(double percent)`; `emergencyStop()` and `safeShutdownAll()` set duty 0 first.
- Modify: `src/hardware/HardwareManager.h/.cpp` – private adapter class `HardwarePneumatics : public IPneumaticHardware` (maps to `readAVLPressure/readTankPressure/readClitoralPressure`, `setPumpEnabled`+`setPumpSpeed(100/0)`, `setSOLn`, `ActuatorControl::setPulserDuty`; simulation mode stores the duty); `PneumaticController* pneumatics() const`; created in `initialize()`, settings from `config/settings.json` "pneumatics", calibration from `config/pulser_calibration.json` (missing file = pulser unavailable, logged); `double sampleClitoralSwing(int durationMs)` (unfiltered `SensorInterface::readClitoralPressure()` in a tight loop, returns max − min).
- Modify: `enterFullVentState()` – call `m_pneumatics->emergencyStop()` first. `enterSealMaintainedSafeState()` – becomes a call to `enterFullVentState(reason)` (spec 5.5: any fault vents both chambers). `emergencyStop()` unchanged (already routes there).
- Modify: `docs/README.md` pin table (GPIO 15 = clitoral pulser PWM), `docs/images/make_diagrams/sys_figs.py` overview pin tags + regenerate `system-overview.svg`.

**Interfaces:**
- Consumes: `IPneumaticHardware`, `PneumaticController`, `SoftwarePwm`.
- Produces: `PneumaticController* HardwareManager::pneumatics() const`; `void ActuatorControl::setPulserDuty(double)`; `double HardwareManager::sampleClitoralSwing(int durationMs)`.

- [ ] **Step 1: Verify the pin map fails first** – add `GPIO_PULSER_PWM = 15` to `ActuatorControl.h` only, run `python3 scripts/check_gpio_pins.py`; expected: `GPIO 15: GPIO_PULSER_PWM`, `problems: 0` (confirms 15 is free).
- [ ] **Step 2: Implement** the members above.
- [ ] **Step 3: Build + tests** – build/test command; expected: app builds with 0 errors and no new warnings in the touched files; all `pneumatics_*` PASS.
- [ ] **Step 4: Commit** – `feat(hardware): pulser on GPIO 15; HardwareManager owns PneumaticController; faults vent both chambers`

---

### Task 8: Callers stop driving valves directly

**Files:**
- Modify: `src/patterns/PatternEngine.cpp` – `applyPressureTarget(double percent)` becomes `m_hardware->pneumatics()->setLegTarget(percent / 100.0 * 75.0)` (0 → 0); remove its pump/SOL1/SOL2 writes; stop starting `AntiDetachmentMonitor` (lines ~136–138); `isAntiDetachmentActive()` returns `pneumatics()->isSealRecovering()`; call `pneumatics()->start()` where the pattern starts and `ventAll()` where it stops.
- Modify: `src/control/OrgasmControlAlgorithm.cpp` – replace every `setSOL1(true); setSOL2(false);` pair with `m_hardware->pneumatics()->holdLegs();`, every lone `setSOL5(true)` with `setClitoralTarget(0.0)`, and the `setSOL2(true); setSOL5(true);` pair (~line 357) with `ventAll()`.

- [ ] **Step 1: Write the guard check** – `grep -rnE 'setSOL[1245]\(|setPumpSpeed\(|setPumpEnabled\(' src --include=*.cpp | grep -vE 'src/(hardware|pneumatics|testing|calibration)/|SystemDiagnosticsPanel|safety/(AntiDetachmentMonitor|AdaptiveSealMonitor)'`; run it before the change: expected matches in `PatternEngine.cpp` and `OrgasmControlAlgorithm.cpp`.
- [ ] **Step 2: Make the replacements.**
- [ ] **Step 3: Re-run the guard** – expected: no output. Build + tests – expected PASS.
- [ ] **Step 4: Commit** – `refactor: patterns and orgasm control set chamber targets instead of switching valves`

---

### Task 9: `ClitoralOscillator` runs the pulser

**Files:**
- Modify: `src/hardware/ClitoralOscillator.h/.cpp`

Public API and signals stay (callers in `PatternEngine`, `HardwareManager`, `OrgasmControlAlgorithm` unchanged). New behaviour:
- `start()` → `pneumatics()->setPulseRate(m_frequencyHz)` then `setPulserEnabled(true)`; on `false` emit `error("Pulser not calibrated")` and stay stopped.
- `stop()` → `setPulserEnabled(false)` (clitoral target untouched); `emergencyStop()` → `pneumatics()->emergencyStop()`.
- `setFrequency(hz)` → clamp to **1–100** (was 3–15) and forward while running.
- `setAmplitude(mmHg)` → `pneumatics()->setClitoralTarget(mmHg)` (the old "amplitude" becomes the steady target).
- `pulse(intensity, durationMs)` → pulser on for `durationMs` at the current rate.
- `vacuumPhaseChanged(bool)` is emitted from `PneumaticController::pulseBurstChanged` (keeps the TENS hook compiling).
- Remove all SOL4/SOL5 writes, the phase machine and the 1 ms timer; presets keep their rates (5/8/11/4 Hz – now burst rhythms) and set targets 20/40/55/15 mmHg.

- [ ] **Step 1: Guard** – `grep -n 'setSOL[45]' src/hardware/ClitoralOscillator.cpp`; expected before: several matches.
- [ ] **Step 2: Implement.**
- [ ] **Step 3: Guard again** – expected: no output. Build + tests PASS.
- [ ] **Step 4: Commit** – `refactor(oscillator): drive the pulser instead of cycling SOL4/SOL5`

---

### Task 10: Bench calibration tool, config and docs

**Files:**
- Create: `tools/pulser_bench/main.cpp` + target `PulserBench` in `CMakeLists.txt` (links `VacuumControllerLib`).
- Create: `config/pulser_calibration.example.json` (`"measured": false`, 3 placeholder points), `docs/pulser-bench-calibration.md`.
- Modify: `.gitignore` – ignore `config/pulser_calibration.json` (measured on each machine).
- Modify: `docs/vacuum Controller Project - Parts List.md` (relief valves 85–100 mmHg ×3; SOL1/SOL3/SOL4 normally-closed, SOL2/SOL5 normally-open; pulser wiring on GPIO 15 via a spare L293D half-bridge or MOSFET module), `docs/images/make_diagrams/sys_figs.py` plumbing picture (pulser on a T at the clitoral line, relief valves, NC/NO marks) + regenerate `vacuum-plumbing.svg`.

`PulserBench` behaviour: `PulserBench --steady <mmHg> --duty 30,40,50,60,70,80,90,100 --seconds 10`. Initializes `HardwareManager`, holds the clitoral chamber at `--steady` via `pneumatics()->setClitoralTarget()`, then for each duty: drives the pulser at that raw duty (bench-only `PneumaticController::setBenchDuty(double)`, refused unless the controller's `PneumaticSettings::benchMode` is true; only `PulserBench` sets it), prints `duty=NN%` so the rate can be read with a phone spectrum app, and prints `swing=XX.X mmHg` from `sampleClitoralSwing(2000)`. Refuses to start if `--steady` + any measured swing would pass 75 mmHg (stops that step, vents, prints why). Ctrl-C → `emergencyStop()`.

`docs/pulser-bench-calibration.md`: spec 5.6 steps 1–4 in plain words (cup sealed on a plate, soft plug filling ~70 % of the clitoral chamber, phone app for rate, how to fill `config/pulser_calibration.json`, set `"measured": true`).

- [ ] **Step 1: Test** – add `tst_pulsercalibration::exampleFileIsRejected()` loading `config/pulser_calibration.example.json` → false (example must never run the pulser). Run – FAIL (file missing).
- [ ] **Step 2: Implement** the tool, `setBenchDuty`, example file, docs, pictures.
- [ ] **Step 3: Run** – build/test command; expected PASS; `./build/PulserBench --help` prints usage; `python3 scripts/check_gpio_pins.py` → `problems: 0`.
- [ ] **Step 4: Commit** – `feat: pulser bench calibration tool, example config, docs and pictures`

---

## Not in this plan (from the spec's non-goals)

Separate pulse-strength control, helper microcontroller, speed sensor, restrictor, TENS, HX711, Jetson port, cup changes. Hardware purchases (relief valves, wiring the pulser) are the owner's; bench calibration (Task 10 procedure) must be done before the pulser is used on a body.
