Implement orgasm detection and point-of-no-return handling in the Adaptive Edging mode of the OrgasmControlAlgorithm to properly handle cases where repeated edge cycles naturally progress to orgasm.

**Background:**
Currently, `runAdaptiveEdging()` only detects when arousal reaches the edge threshold (0.85) and treats all such events as edges requiring back-off. It does NOT detect actual orgasms (arousal ≥ 0.95 + contractions), which means:
- If the user orgasms during edging, the algorithm incorrectly treats it as an edge
- Stimulation stops mid-orgasm (unpleasant experience)
- No orgasm event is recorded in session statistics
- The algorithm doesn't recognize when the "point of no return" has been reached

**Requirements:**

1. **Add orgasm detection to the BUILDING state in `runAdaptiveEdging()`**:
   - Before checking for edge threshold, check for orgasm using the same criteria as forced orgasm mode: `m_arousalLevel >= m_orgasmThreshold (0.95)` AND `detectContractions()` returns true
   - If orgasm is detected during edging:
     - Set `m_inOrgasm = true`
     - Increment `m_orgasmCount`
     - Emit a new signal `unexpectedOrgasmDuringEdging(m_orgasmCount, m_edgeCount)` to distinguish from planned orgasms
     - Continue stimulation through the orgasm (do NOT stop or back off)
     - After orgasm completes (using same timing as forced orgasm mode: `ORGASM_DURATION_MS` + `POST_ORGASM_PAUSE_MS`), decide next action based on mode

2. **Add point-of-no-return detection to the BACKING_OFF state**:
   - Monitor if arousal continues rising during back-off despite stimulation being stopped
   - If `m_arousalLevel > m_previousArousal + 0.02` (arousal rising by 2% or more during back-off), this indicates orgasm is inevitable
   - When point of no return is detected:
     - Emit a new signal `pointOfNoReturnReached(m_edgeCount)`
     - Optionally resume gentle stimulation to support the orgasm rather than fighting it
     - Transition to orgasm handling (set `m_inOrgasm = true`, increment `m_orgasmCount`)

3. **Define post-orgasm behavior for edging mode**:
   - After an unexpected orgasm during edging completes, the algorithm should:
     - If in DENIAL mode: End the session with `edgingComplete()` and transition to COOLING_DOWN
     - If in ADAPTIVE_EDGING mode: Either end session or optionally continue edging after a longer recovery period (user configurable)
   - Track unexpected orgasms separately from planned forced orgasms in session statistics

4. **Add new signals to the header file**:
   - `void unexpectedOrgasmDuringEdging(int orgasmCount, int edgeCount);`
   - `void pointOfNoReturnReached(int edgeCount);`

5. **Preserve existing functionality**:
   - Keep all existing edge detection logic (arousal ≥ 0.85 → edge)
   - Keep the transition to FORCED_ORGASM mode after reaching target edges
   - Keep the BACKING_OFF → HOLDING → BUILDING state cycle for normal edges
   - Ensure orgasm detection in `runForcedOrgasm()` remains unchanged

After implementation:
- Verify no compilation errors using the diagnostics tool
- Explain how the new logic handles the progression from edging to orgasm
- Describe the state transitions when orgasm is detected during BUILDING vs during BACKING_OFF