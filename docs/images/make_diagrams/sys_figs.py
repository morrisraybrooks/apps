"""System diagrams for the V-Contour / A-Contour controller (hand-placed SVG).

usage: python sys_figs.py OUT_DIR
Facts come from src/hardware/HardwareManager.h, ActuatorControl.h, FluidSensor.h,
MotionSensor.h, HeartRateSensor.h, src/safety/SafetyConstants.h, EmergencyStop.h.
"""
import sys, os

OUT = sys.argv[1]
FONT = "Helvetica, Arial, sans-serif"
C = dict(
    bg="#ffffff", ink="#111111", sub="#444444", line="#37474f",
    leg="#1f5fbf", legfill="#cfe0fb", clit="#c96a00", clitfill="#ffd6a0",
    chan="#1d7f43", chanfill="#d2f2dc", seal="#c2185b", sealfill="#f7b6d2",
    ctl="#283593", ctlfill="#e3e7fb", drv="#455a64", drvfill="#eceff1",
    air="#37474f", airfill="#e6e9ec", sens="#00796b", sensfill="#d4efea",
    tens="#7b1fa2", tensfill="#f0dcf7", warn="#b71c1c", warnfill="#fde7e7",
    skin="#e9cfb4",
)


class Svg:
    def __init__(self, W, H, title, subtitle=None):
        self.W, self.H, self.el = W, H, []
        self.text(40, 62, title, 46, bold=True)
        if subtitle:
            self.text(40, 106, subtitle, 28, C["sub"])

    def text(self, x, y, s, size=28, color=None, anchor="start", bold=False):
        color = color or C["ink"]
        w = "bold" if bold else "normal"
        s = s.replace("&", "&amp;").replace("<", "&lt;")
        self.el.append(f'<text x="{x}" y="{y}" font-family="{FONT}" font-size="{size}" fill="{color}" text-anchor="{anchor}" font-weight="{w}">{s}</text>')

    def box(self, x, y, w, h, lines, stroke, fill, size=30, dashed=False, align="middle", r=16):
        da = ' stroke-dasharray="14 9"' if dashed else ""
        self.el.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}" stroke="{stroke}" stroke-width="4"{da}/>')
        n = len(lines)
        sub = int(size * 0.78)
        total = size + (n - 1) * sub * 1.22
        ty = y + h / 2 - total / 2 + size * 0.8
        tx = x + w / 2 if align == "middle" else x + 22
        for i, ln in enumerate(lines):
            if i == 0:
                self.text(tx, ty, ln, size, stroke, align, True)
                ty += size * 0.35 + sub * 1.0
            else:
                self.text(tx, ty, ln, sub, C["ink"], align)
                ty += sub * 1.22

    def line(self, pts, color, w=6, dashed=False, arrow=False):
        d = "M" + " L".join(f"{x},{y}" for x, y in pts)
        da = ' stroke-dasharray="12 9"' if dashed else ""
        self.el.append(f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{w}" stroke-linejoin="round" stroke-linecap="round"{da}/>')
        if arrow:
            (x0, y0), (x1, y1) = pts[-2], pts[-1]
            import math
            a = math.atan2(y1 - y0, x1 - x0)
            L = 10 + w * 2.2
            p1 = (x1 - L * math.cos(a - 0.45), y1 - L * math.sin(a - 0.45))
            p2 = (x1 - L * math.cos(a + 0.45), y1 - L * math.sin(a + 0.45))
            self.el.append(f'<path d="M{p1[0]:.1f},{p1[1]:.1f} L{x1},{y1} L{p2[0]:.1f},{p2[1]:.1f} Z" fill="{color}" stroke="{color}" stroke-width="2"/>')

    def tag(self, x, y, s, color, size=22):
        """small label on a white pill (pin numbers on wires)"""
        w = len(s) * size * 0.56 + 20
        self.el.append(f'<rect x="{x - w/2:.0f}" y="{y - size*0.85:.0f}" width="{w:.0f}" height="{size*1.3:.0f}" rx="8" fill="#fff" stroke="{color}" stroke-width="2"/>')
        self.text(x, y + size * 0.22, s, size, color, "middle", True)

    def sensor(self, cx, cy, color=None, fill=None):
        color, fill = color or C["sens"], fill or C["sensfill"]
        self.el.append(f'<circle cx="{cx}" cy="{cy}" r="34" fill="{fill}" stroke="{color}" stroke-width="4"/>')
        self.text(cx, cy + 12, "P", 34, color, "middle", True)

    def air(self, x, y, direction="right", label="to air"):
        dx = {"right": 70, "left": -70, "up": 0, "down": 0}[direction]
        dy = {"right": 0, "left": 0, "up": -60, "down": 60}[direction]
        self.line([(x, y), (x + dx, y + dy)], "#78909c", 5, dashed=True, arrow=True)
        tx, ty = x + dx * 1.15, y + dy * 1.15
        anchor = {"right": "start", "left": "end", "up": "middle", "down": "middle"}[direction]
        self.text(tx + (8 if direction == "right" else -8 if direction == "left" else 0),
                  ty + (10 if direction in ("right", "left") else (-8 if direction == "up" else 30)),
                  label, 24, "#546e7a", anchor, True)

    def save(self, name):
        s = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {self.W} {self.H}" width="{self.W}" height="{self.H}">'
             f'<rect width="100%" height="100%" fill="{C["bg"]}"/>' + "".join(self.el) + "</svg>")
        open(os.path.join(OUT, name), "w").write(s)


# =====================================================================
# Vacuum plumbing
# =====================================================================
def plumbing():
    g = Svg(1760, 1150, "Vacuum plumbing",
            "Pump → tank → two separate lines, one per cup chamber. Each line has its own valve, vent and pressure sensor.")
    n0 = len(g.el)
    TOP, BOT = 330, 800          # clitoral line, leg line
    # pump + tank
    g.box(40, 500, 230, 140, ["Vacuum pump", "12 V DC", "speed by PWM"], C["air"], C["airfill"], 30)
    g.box(350, 480, 220, 180, ["Vacuum", "tank", "stores vacuum"], C["air"], C["airfill"], 34)
    g.line([(270, 570), (350, 570)], C["air"], 8)
    # tank sensor + tank vent
    g.line([(460, 480), (460, 420)], C["air"], 6)
    g.sensor(460, 386)
    g.text(412, 382, "Tank sensor", 26, C["sens"], "end", True)
    g.text(412, 410, "ADC ch 1", 22, C["sub"], "end")
    g.line([(460, 660), (460, 720)], C["air"], 6)
    g.box(385, 720, 150, 100, ["SOL3", "tank vent"], C["drv"], C["drvfill"], 32)
    g.air(460, 820, "down")
    # manifold
    g.line([(570, 570), (630, 570)], C["air"], 8)
    g.line([(630, TOP), (630, BOT)], C["air"], 8)

    def branch(y, col, fill, vac, vent, sens_name, ch, up):
        g.line([(630, y), (1300, y)], col, 8)
        g.box(680, y - 55, 180, 110, [vac, "vacuum on"], col, "#ffffff", 34)
        s = -1 if up else 1
        # sensor tee
        g.line([(960, y), (960, y + s * 70)], col, 6)
        g.sensor(960, y + s * 104)
        sy = y + s * 104
        g.text(912, sy - 4, sens_name, 26, C["sens"], "end", True)
        g.text(912, sy + 24, ch, 22, C["sub"], "end")
        # vent tee
        g.line([(1140, y), (1140, y + s * 60)], col, 6)
        vy = y + s * 60 if not up else y - 60 - 100
        g.box(1065, vy, 150, 100, [vent, "vent"], col, "#ffffff", 32)
        g.air(1140, vy if up else vy + 100, "up" if up else "down")

    branch(TOP, C["clit"], C["clitfill"], "SOL4", "SOL5", "Clitoral sensor", "ADC ch 2", True)
    branch(BOT, C["leg"], C["legfill"], "SOL1", "SOL2", "AVL sensor", "ADC ch 0, anti-detach", False)

    # cup block
    x0, w = 1310, 400
    g.el.append(f'<rect x="{x0}" y="200" width="{w}" height="760" rx="30" fill="#f5f7fa" stroke="{C["line"]}" stroke-width="4"/>')
    g.text(x0 + w / 2, 250, "A-Contour cup", 32, C["ink"], "middle", True)
    g.box(x0 + 30, 270, w - 60, 180, ["Clitoral chamber", "steady (engorge) or", "5–13 Hz air pulses"], C["clit"], C["clitfill"], 30)
    g.box(x0 + 30, 690, w - 60, 220, ["Leg chamber", "steady 30–50 mmHg", "engorge + hold on", "(both legs)"], C["leg"], C["legfill"], 30)
    g.line([(1300, TOP), (x0 + 30, TOP)], C["clit"], 8)
    g.line([(1300, BOT), (x0 + 30, BOT)], C["leg"], 8)
    for yy in (TOP, BOT):
        g.el.append(f'<circle cx="{x0 + 30}" cy="{yy}" r="13" fill="#fff" stroke="{C["ink"]}" stroke-width="5"/>')
    # manual flaps
    for yy in (310, 750):
        g.el.append(f'<rect x="{x0 + w - 58}" y="{yy}" width="34" height="34" rx="7" fill="{C["sealfill"]}" stroke="{C["seal"]}" stroke-width="5"/>')
    g.box(x0 + 30, 480, w - 60, 180, ["Open channel", "no vacuum; fluids", "drain out"], C["chan"], C["chanfill"], 28)
    g.text(x0 + w / 2, 1000, "Pink squares = manual vent flaps", 24, C["seal"], "middle", True)
    g.text(x0 + w / 2, 1032, "lift to release, no power needed", 22, C["sub"], "middle")

    # planned safety parts
    g.box(690, 470, 560, 200,
          ["Safety parts still to add", "• relief valve on each line, opens 85–100 mmHg",
           "• fluid filter at each cup port",
           "• vent valves open when power is lost"],
          C["warn"], "#fffafa", 28, dashed=True, align="start")
    g.text(40, 1120, "Controller limits: 30–50 mmHg normal · 75 mmHg cap · 80 mmHg emergency stop · 100 mmHg never.",
           26, C["ink"], "start", True)
    g.el[n0:] = ['<g transform="translate(0,50)">'] + g.el[n0:] + ['</g>']
    g.H += 50
    g.save("vacuum-plumbing.svg")


# =====================================================================
# Whole-system overview
# =====================================================================
def overview():
    g = Svg(1760, 1420, "V-Contour system overview",
            "Raspberry Pi 4 runs the Qt app. Pin numbers are BCM GPIO, from the source code.")
    # band 1
    g.box(40, 150, 330, 150, ["50\" touch display", "HDMI"], C["ctl"], C["ctlfill"], 32)
    g.box(40, 330, 330, 140, ["Power", "5 V → Pi", "12 V → pump + valves"], C["drv"], C["drvfill"], 30)
    g.box(520, 150, 720, 290, ["Raspberry Pi 4", "Qt control app: patterns, game modes,", "arousal tracking, logging",
                               "Safety: 75 mmHg cap · 80 mmHg e-stop",
                               "sensor checks · anti-detachment"], C["ctl"], C["ctlfill"], 40)
    g.box(1390, 150, 330, 150, ["Emergency stop", "button + on-screen", "GPIO 21"], C["warn"], C["warnfill"], 32)
    g.line([(370, 225), (520, 225)], C["ctl"], 6)
    g.line([(370, 400), (450, 400), (450, 330), (520, 330)], C["drv"], 6)
    g.line([(1240, 225), (1390, 225)], C["warn"], 6)

    # band 2: interfaces
    names = [
        (["Pump driver", "L293D"], "GPIO 18, 25", C["drv"], C["drvfill"]),
        (["Valve drivers", "2 × L293D, 12 V"], "GPIO 17 27 22 23 24", C["drv"], C["drvfill"]),
        (["ADC", "MCP3008"], "SPI 8–11", C["sens"], C["sensfill"]),
        (["Scale amp", "HX711"], "GPIO 26, 19", C["chan"], C["chanfill"]),
        (["I²C sensors", "bus 1"], "I²C", C["sens"], C["sensfill"]),
        (["TENS drivers", "×2"], "GPIO 5 6 12 16 / 19 20 13 21", C["tens"], C["tensfill"]),
    ]
    n, gap, x = 6, 22, 40
    bw = (1720 - x - gap * (n - 1)) / n
    xs = [x + i * (bw + gap) for i in range(n)]
    bus_y = 520
    g.line([(880, 440), (880, bus_y)], C["ctl"], 6)
    g.line([(xs[0] + bw / 2, bus_y), (xs[-1] + bw / 2, bus_y)], C["ctl"], 6)
    for (lines, pins, col, fill), xx in zip(names, xs):
        cx = xx + bw / 2
        g.line([(cx, bus_y), (cx, 640)], C["ctl"], 6)
        g.tag(cx, 585, pins.split(" / ")[0], C["ctl"], 20)
        g.box(xx, 640, bw, 130, lines, col, fill, 30)
    g.tag(xs[5] + bw / 2, 615, "19 20 13 21", C["ctl"], 20)

    # band 3: devices
    dev = [
        (["Pump → tank"], C["air"], C["airfill"]),
        (["SOL1–SOL5", "valves"], C["air"], C["airfill"]),
        (["3 pressure", "sensors", "MPX5010DP"], C["sens"], C["sensfill"]),
        (["Load cell", "under fluid", "catcher"], C["chan"], C["chanfill"]),
        (["Heart rate", "MAX30102", "Motion MPU6050"], C["sens"], C["sensfill"]),
        (["Electrodes", "clitoral +", "urethral"], C["tens"], C["tensfill"]),
    ]
    for (lines, col, fill), xx in zip(dev, xs):
        cx = xx + bw / 2
        if lines[0] in ("Pump → tank", "SOL1–SOL5", "Electrodes"):
            g.line([(cx, 770), (cx, 850)], col, 6, arrow=True)       # output
        else:
            g.line([(cx, 850), (cx, 772)], col, 6, arrow=True)       # input
        g.box(xx, 850, bw, 160, lines, col, fill, 28)

    # band 4: cup on the body
    g.el.append(f'<rect x="40" y="1110" width="1680" height="190" rx="24" fill="{C["skin"]}" fill-opacity="0.35" stroke="#a07a55" stroke-width="4"/>')
    g.text(70, 1150, "On the body", 30, "#7a5530", "start", True)
    g.box(560, 1130, 640, 150, ["A-Contour cup", "leg chamber (SOL1/2) + clitoral chamber (SOL4/5)",
                                "open channel drains into the fluid catcher"], C["leg"], "#ffffff", 32)
    # connections to cup
    def to_cup(i, tx, col, label=None, up=False):
        cx = xs[i] + bw / 2
        pts = [(cx, 1010), (cx, 1060), (tx, 1060), (tx, 1130)]
        if up:
            pts = list(reversed(pts))
        g.line(pts, col, 6, arrow=True)
    to_cup(1, 700, C["air"])                       # tubes
    g.line([(xs[0] + bw - 10, 930), (xs[1] + 10, 930)], C["air"], 6, arrow=True)
    to_cup(2, 880, C["sens"], up=True)             # sensors tee into the lines
    to_cup(3, 1100, C["chan"], up=True)            # fluid from channel to catcher
    to_cup(5, 1180, C["tens"])                     # electrodes in cup
    cx4 = xs[4] + bw / 2
    g.line([(cx4, 1010), (cx4, 1200)], C["sens"], 6, dashed=True)
    g.text(cx4 + 14, 1240, "worn on the body", 22, C["sub"])

    # pin clash warning
    g.box(40, 1320, 1680, 84, ["Pin clash in the code: GPIO 19 = HX711 clock and urethral TENS enable;  GPIO 21 = e-stop button and urethral TENS fault"],
          C["warn"], C["warnfill"], 26)
    g.save("system-overview.svg")


plumbing()
overview()
print("ok")
