code mods.md

I want to implement the oscillating pressure wave functionality (air-pulse stimulation) for the clitoral cylinder (SOL4/SOL5) in the V-Contour system. 

**Context:**
- Commercial toys (Womanizer, Satisfyer) use a vibrating diaphragm/membrane to create pressure oscillations
- My system uses solenoid valves (SOL4 and SOL5) instead of a diaphragm
- I need to create rapid pressure oscillations in the 5-13 Hz frequency range (based on the research showing 8-13 Hz as optimal)
- The system has 3 pressure sensors: Sensor 1 (Outer V-seal), Sensor 2 (Tank), Sensor 3 (Clitoral cylinder)

**What I need help with:**
1. Design the control algorithm for SOL4/SOL5 to create smooth oscillating pressure waves (not just on/off switching)
2. Determine the best approach: Should I modify existing pattern code or create new pattern classes?
3. Implement sensor feedback from Sensor 3 (clitoral cylinder) to maintain target pressure amplitude during oscillation
4. Ensure the algorithm can handle different frequencies (5 Hz, 8 Hz, 10 Hz, 12 Hz) and pressure amplitudes (0-75 mmHg range)
5. Coordinate with the outer chamber (SOL1/SOL2) which maintains sustained vacuum simultaneously

**Specific questions:**
- What's the best valve timing strategy to create smooth sine-wave-like pressure oscillations using solenoid valves?
- How should I structure the code to avoid conflicts between the dual-chamber control loops?
- Which existing code files should I examine or modify for this implementation?
- Do I need PID control or is simple timed valve switching sufficient for air-pulse patterns?