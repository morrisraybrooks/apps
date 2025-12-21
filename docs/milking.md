milking.md
Research and implement a "Milking Cycle" mode for the OrgasmControlAlgorithm system.

**Phase 1: Research**
1. Search the web for information about:
   - Post-orgasm milking techniques and physiology
   - Clitoral/vulvar sensitivity during the refractory period
   - Forced post-orgasmic stimulation protocols
   - Duration and intensity patterns for milking cycles
   - Physiological responses (arousal levels, contractions, fluid production)
   - Safety considerations and overstimulation limits

2. Save all research findings to a new file: `docs/milking-cycle-research.md`
   - Include sources and citations
   - Document physiological parameters (timing, intensity, sensitivity curves)
   - Note any safety thresholds or contraindications
   - Summarize recommended implementation approach

**Phase 2: Design (after research is complete)**
3. Based on the research, design the Milking Cycle mode:
   - Add `MILKING` to the `Mode` enum in `OrgasmControlAlgorithm.h`
   - Define when milking begins (after orgasm in which modes?)
   - Specify state transitions (e.g., FORCING → MILKING → COOLING_DOWN)
   - Determine stimulation parameters (intensity, frequency, duration)
   - Define end conditions (time-based, arousal-based, or user-triggered)

**Phase 3: Implementation (after design approval)**
4. Implement the mode following the same patterns as existing modes (ADAPTIVE_EDGING, FORCED_ORGASM, etc.)

**Important Context:**
- This is a Qt/C++ application controlling vacuum and vibration hardware
- The system already tracks arousal levels, detects orgasms, and manages post-orgasm states
- Milking would involve continued stimulation during the hypersensitive post-orgasm period
- Consider integration with existing fluid tracking (`FluidSensor`) if relevant

Start with Phase 1 (research) only. Do not proceed to implementation until the research is reviewed and approved.

post orgasm torture" technique duration intensity BDSM

i thought the milking cycle would be a lot like the edging cycle but with a lower intensity and a chasty if you have a full on orgasm this is suppose to drain or milk the orgasm out of you with out you getting the full orgasm release so no hyper  sensitivity unless you loose it and have a full on orgasm
