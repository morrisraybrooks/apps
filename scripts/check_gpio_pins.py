#!/usr/bin/env python3
"""List every Pi GPIO pin defined in the code and report pins used twice.
Same-named constants that repeat one definition (e.g. TENS pins in both HardwareManager.h and TENSController.h)
count once, but a name with two different values is reported."""
import re, sys, os, collections
root = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
files = ["src/hardware/HardwareManager.h", "src/hardware/ActuatorControl.h", "src/hardware/TENSController.h",
         "src/hardware/FluidSensor.h", "src/safety/EmergencyStop.h", "src/safety/EmergencyStop.cpp"]
pat = re.compile(r'(?:static const int|#define)\s+((?:GPIO|DEFAULT_HX711|SPI)_[A-Z0-9_]+)\s*=?\s*(\d+)')
names = collections.defaultdict(set)
for f in files:
    for name, val in pat.findall(open(os.path.join(root, f)).read()):
        names[name].add((int(val), f.split('/')[-1]))
bad = 0
for n, vs in sorted(names.items()):
    if len({v for v, _ in vs}) > 1:
        print("NAME DISAGREES:", n, sorted(vs)); bad += 1
pins = collections.defaultdict(set)
for n, vs in names.items():
    for v, _ in vs:
        pins[v].add(n)
for p in sorted(pins):
    flag = "  <-- CLASH" if len(pins[p]) > 1 else ""
    if flag: bad += 1
    print(f"GPIO {p:2d}: {', '.join(sorted(pins[p]))}{flag}")
print("problems:", bad)
sys.exit(1 if bad else 0)
