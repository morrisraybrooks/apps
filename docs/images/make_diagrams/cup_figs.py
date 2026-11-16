"""Labelled cup_020 figures, sliced from the real STL meshes.

usage: python cup_figs.py CUP_DIR OUT_DIR
"""
import sys, os, numpy as np, trimesh
from shapely.geometry import MultiLineString, LineString, Point, Polygon
from shapely.ops import polygonize, unary_union

CUP, OUT = sys.argv[1], sys.argv[2]
shell = trimesh.load(os.path.join(CUP, "shell.stl"))
seal = trimesh.load(os.path.join(CUP, "seal.stl"))

# ---- palette (shared with the system diagrams) ----
C = dict(
    bg="#ffffff", ink="#111111", sub="#444444",
    wall="#5b6472", wallfill="#9aa3b1",
    leg="#1f5fbf", legfill="#cfe0fb",
    clit="#c96a00", clitfill="#ffd6a0",
    chan="#1d7f43", chanfill="#d2f2dc",
    seal="#c2185b", sealfill="#f7b6d2",
    skin="#e9cfb4", skinline="#a07a55",
)
FONT = "Helvetica, Arial, sans-serif"


def section(mesh, origin, normal, axes):
    s = trimesh.intersections.mesh_plane(mesh, normal, origin)
    return s[:, :, axes]


def faces(segs, extra=()):
    segs = np.round(np.asarray(segs, float), 4)
    segs = [s for s in segs if np.linalg.norm(s[0] - s[1]) > 0]
    lines = [list(map(tuple, s)) for s in segs] + [list(e) for e in extra]
    return list(polygonize(unary_union(MultiLineString(lines))))


def thickness(p):
    return 2 * p.area / p.length


class Fig:
    def __init__(self, xmin, xmax, ymin, ymax, k, pad_l, pad_r, pad_t, pad_b, title, subtitle=None):
        self.k, self.xmin, self.ymax = k, xmin, ymax
        self.pl, self.pt = pad_l, pad_t
        self.W = pad_l + (xmax - xmin) * k + pad_r
        self.H = pad_t + (ymax - ymin) * k + pad_b
        self.body, self.top = [], []
        self.title, self.subtitle = title, subtitle

    def P(self, x, y):
        return self.pl + (x - self.xmin) * self.k, self.pt + (self.ymax - y) * self.k

    def poly(self, g, fill, stroke, sw=2.5, layer=None):
        layer = self.body if layer is None else layer
        geoms = getattr(g, "geoms", [g])
        for p in geoms:
            if p.is_empty:
                continue
            d = ""
            for ring in [p.exterior] + list(p.interiors):
                pts = [self.P(x, y) for x, y in ring.coords]
                d += "M" + " L".join(f"{a:.1f},{b:.1f}" for a, b in pts) + " Z "
            layer.append(f'<path d="{d}" fill="{fill}" fill-rule="evenodd" stroke="{stroke}" stroke-width="{sw}" stroke-linejoin="round"/>')

    def rect_mm(self, x0, y0, x1, y1, fill, stroke="none"):
        (a, b), (c, d) = self.P(x0, y1), self.P(x1, y0)
        self.body.append(f'<rect x="{a:.1f}" y="{b:.1f}" width="{c-a:.1f}" height="{d-b:.1f}" fill="{fill}" stroke="{stroke}"/>')

    def text(self, x, y, lines, size=30, color=None, anchor="start", weight="normal"):
        color = color or C["ink"]
        out = f'<text x="{x:.1f}" y="{y:.1f}" font-family="{FONT}" font-size="{size}" fill="{color}" text-anchor="{anchor}" font-weight="{weight}">'
        for i, ln in enumerate(lines):
            w = "bold" if i == 0 else "normal"
            sz = size if i == 0 else int(size * 0.82)
            dy = 0 if i == 0 else size * 1.12
            fill = color if i == 0 else C["sub"]
            out += f'<tspan x="{x:.1f}" dy="{dy:.1f}" font-weight="{w}" font-size="{sz}" fill="{fill}">{ln}</tspan>'
        self.top.append(out + "</text>")

    def label(self, at_mm, tx, ty, lines, color, anchor="start", size=30):
        """at_mm: one (x, y) point or a list of them; leader lines run from the text to each."""
        pts = at_mm if isinstance(at_mm[0], (tuple, list)) else [at_mm]
        w = max([len(lines[0]) * size * 0.6] + [len(l) * size * 0.82 * 0.52 for l in lines[1:]]) if lines else 0
        left = {"start": tx, "end": tx - w, "middle": tx - w / 2}[anchor]
        right = left + w
        for pt in pts:
            ax, ay = self.P(*pt)
            if ax > right + 20:
                ex, ey = right + 12, ty - size * 0.35
            elif ax < left - 20:
                ex, ey = left - 12, ty - size * 0.35
            else:
                ex = min(max(ax, left), right)
                ey = ty - size - 6 if ay < ty else ty + size * 0.3 + (len(lines) - 1) * size * 1.12 + 6
            self.top.append(f'<line x1="{ax:.1f}" y1="{ay:.1f}" x2="{ex:.1f}" y2="{ey:.1f}" stroke="{color}" stroke-width="3"/>')
            self.top.append(f'<circle cx="{ax:.1f}" cy="{ay:.1f}" r="7" fill="{color}" stroke="#fff" stroke-width="2"/>')
        self.text(tx, ty, lines, size, color, anchor)

    def scalebar(self, x, y, mm=20):
        L = mm * self.k
        self.top.append(f'<g stroke="{C["ink"]}" stroke-width="4"><line x1="{x}" y1="{y}" x2="{x+L}" y2="{y}"/>'
                        f'<line x1="{x}" y1="{y-12}" x2="{x}" y2="{y+12}"/><line x1="{x+L}" y1="{y-12}" x2="{x+L}" y2="{y+12}"/></g>')
        self.text(x + L / 2, y + 42, [f"{mm} mm"], 26, anchor="middle")

    def save(self, name):
        head = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {self.W:.0f} {self.H:.0f}" width="{self.W:.0f}" height="{self.H:.0f}">'
                f'<rect width="100%" height="100%" fill="{C["bg"]}"/>')
        t = [f'<text x="40" y="58" font-family="{FONT}" font-size="44" font-weight="bold" fill="{C["ink"]}">{self.title}</text>']
        if self.subtitle:
            t.append(f'<text x="40" y="102" font-family="{FONT}" font-size="28" fill="{C["sub"]}">{self.subtitle}</text>')
        open(os.path.join(OUT, name), "w").write(head + "".join(t + self.body + self.top) + "</svg>")


def skin_band(f, x0, x1):
    f.rect_mm(x0, -9, x1, 0, C["skin"])
    a, b = f.P(x0, 0)
    c, _ = f.P(x1, 0)
    f.body.append(f'<line x1="{a:.1f}" y1="{b:.1f}" x2="{c:.1f}" y2="{b:.1f}" stroke="{C["skinline"]}" stroke-width="3" stroke-dasharray="10 7"/>')


def seal_polys(origin, normal, axes):
    fs = faces(section(seal, origin, normal, axes))
    return [p for p in fs if p.area > 1]


# =====================================================================
# 1) FOOTPRINT (top view, at the skin)
# =====================================================================
def top_view():
    segs = section(shell, [0, 0, 0.3], [0, 0, 1], [0, 1])
    fs = faces(segs)
    walls = [p for p in fs if thickness(p) < 5]
    rooms = sorted([p for p in fs if thickness(p) >= 5], key=lambda p: -p.area)
    leg, clit = rooms[0], rooms[1]
    foot = unary_union(fs)
    closed = foot.buffer(25).buffer(-25)
    diff = closed.difference(foot)
    chan = max(getattr(diff, "geoms", [diff]), key=lambda g: g.area)
    seal_fs = seal_polys([0, 0, -1.0], [0, 0, 1], [0, 1])
    head = faces(section(shell, [0, 0, 62], [0, 0, 1], [0, 1]))
    head_out = max(head, key=lambda p: p.area) if head else None
    tip = faces(section(shell, [0, 0, 77], [0, 0, 1], [0, 1]))
    tipc = max(tip, key=lambda p: p.area).centroid if tip else Point(0, 40)

    f = Fig(-50, 50, -73, 63, 6.6, 520, 520, 150, 130,
            "A-Contour cup_020 — footprint on the body",
            "Looking straight down through the clear shell. Apex (top) points to the pubic bone.")
    for p in seal_fs:
        f.poly(p.buffer(0.6), C["sealfill"], C["seal"], 2)
    f.poly(chan, C["chanfill"], C["chan"], 0)
    f.poly(leg, C["legfill"], C["leg"], 0)
    f.poly(clit, C["clitfill"], C["clit"], 0)
    for w in walls:
        f.poly(w, C["wallfill"], C["wall"], 1.5)
    if head_out is not None:
        pts = " ".join("%.1f,%.1f" % f.P(x, y) for x, y in head_out.exterior.coords)
        f.top.append(f'<polygon points="{pts}" fill="none" stroke="{C["clit"]}" stroke-width="4" stroke-dasharray="14 9"/>')
    for (x, y), col in [((tipc.x, tipc.y), C["clit"]), ((24.0, 5.0), C["leg"])]:
        a, b = f.P(x, y)
        f.top.append(f'<circle cx="{a:.1f}" cy="{b:.1f}" r="14" fill="#fff" stroke="{col}" stroke-width="6"/><circle cx="{a:.1f}" cy="{b:.1f}" r="5" fill="{col}"/>')
    for (x, y) in [(16.6, 32.2), (-23.4, -18.0)]:
        a, b = f.P(x, y)
        f.top.append(f'<rect x="{a-13:.1f}" y="{b-13:.1f}" width="26" height="26" rx="6" fill="{C["sealfill"]}" stroke="{C["seal"]}" stroke-width="5"/>')

    L, R = 470, f.W - 470
    f.label((0, 17), L, 330, ["Clitoral chamber", "glans + shaft, 27 mm", "mouth at the skin"], C["clit"], "end")
    f.label((-6, 30), L, 480, ["Glans head (dashed)", "stands up out of the", "shell, 33 mm corona"], C["clit"], "end")
    f.label((-23.4, -18.0), L, 660, ["Leg vent flap", "lift = instant release"], C["seal"], "end")
    f.label((-32, -40), L, 820, ["Leg chamber (outer)", "steady 30–50 mmHg,", "swells the labia,", "holds the cup on"], C["leg"], "end")
    f.label((0, -52), L, 1040, ["Open channel", "no vacuum: urethra and", "vagina stay free,", "fluids drain out"], C["chan"], "end")
    f.label((tipc.x, tipc.y), R, 230, ["Clitoral port", "at the glans tip"], C["clit"])
    f.label((16.6, 32.2), R, 340, ["Clitoral vent flap"], C["seal"])
    f.label((24.0, 5.0), R, 450, ["Leg port", "one port pulls", "both legs"], C["leg"])
    f.label((44, -40), R, 680, ["Silicone seal lip", "soft, rounded, all round"], C["seal"])
    f.text(R, 860, ["93.5 × 128.5 mm", "79.6 mm tall", "one piece, two ports"], 28)
    f.scalebar(R, 1030)
    f.save("cup020-footprint.svg")


# =====================================================================
# 2) SIDE CUT (x = 0, through the middle)
# =====================================================================
def side_view():
    segs = section(shell, [0, 0, 0], [1, 0, 0], [1, 2])
    fs = faces(segs, [[(-75, 0.2), (65, 0.2)], [(-75, 75.0), (65, 75.0)]])
    walls = [p for p in fs if thickness(p) < 5 and p.area > 5]
    rooms = [p for p in fs if thickness(p) >= 5 and p.area > 30 and p.bounds[1] < 1]
    clit = max(rooms, key=lambda p: p.bounds[3])
    legs = [p for p in rooms if p is not clit]
    seal_fs = seal_polys([0, 0, 0], [1, 0, 0], [1, 2])

    f = Fig(-72, 64, -12, 84, 6.6, 120, 120, 150, 230,
            "Side cut through the middle",
            "Leg ends on the left, apex (pubic bone) on the right.")
    skin_band(f, -72, 64)
    xw = min(p.bounds[0] for p in walls)
    f.rect_mm(-72, 0.05, xw - 0.3, 5, C["chanfill"])
    for p in legs:
        f.poly(p, C["legfill"], C["leg"], 0)
    f.poly(clit, C["clitfill"], C["clit"], 0)
    for w in walls:
        f.poly(w, C["wallfill"], C["wall"], 1.5)
    for p in seal_fs:
        f.poly(p, C["sealfill"], C["seal"], 2)

    tip = max(clit.exterior.coords, key=lambda c: c[1])
    f.label((tip[0], 78), 140, 200, ["Clitoral port", "at the tip"], C["clit"])
    cc = clit.representative_point()
    f.label((24, 52), 140, 330, ["Clitoral chamber", "shaft 27 mm wide × 44 mm long,", "corona 33 mm, about 70 mm deep,", "leans 15° toward the apex"], C["clit"])
    lp = min(legs, key=lambda p: p.centroid.x).representative_point()
    f.label((lp.x, lp.y), 140, 540,
            ["Leg-chamber air", "both blue areas: it wraps", "round the shaft base, and", "the legs join over the top"], C["leg"])
    f.label((-55, 2.5), 140, f.H - 70, ["Open channel", "no shell over it"], C["chan"])
    sx, sy = max(seal_fs, key=lambda p: p.centroid.x).centroid.coords[0]
    f.label((sx, sy), f.W - 130, f.H - 70, ["Seal lip on the skin"], C["seal"], "end")
    a, b = f.P(-70, -6)
    f.text(a, b + 10, ["Body"], 28, C["skinline"])
    f.save("cup020-side-cut.svg")


# =====================================================================
# 3) CROSS CUTS (across the legs, across the apex)
# =====================================================================
def cross(y, xoff):
    segs = section(shell, [0, y, 0], [0, 1, 0], [0, 2]) + np.array([xoff, 0])
    fs = faces(segs, [[(xoff - 52, 0.2), (xoff + 52, 0.2)]])
    walls = [p for p in fs if thickness(p) < 5 and p.area > 5]
    rooms = [p for p in fs if thickness(p) >= 5 and p.area > 20]
    sf = [p for p in faces(section(seal, [0, y, 0], [0, 1, 0], [0, 2]) + np.array([xoff, 0])) if p.area > 1]
    return walls, rooms, sf


def cross_view():
    f = Fig(-52, 172, -12, 70, 6.2, 90, 300, 150, 300,
            "Cuts across the cup",
            "Left: across the legs.   Right: across the apex, through the clitoral chamber.")
    skin_band(f, -52, 172)
    w1, r1, s1 = cross(-25, 0)
    xs = sorted(p.bounds[2] for p in r1)
    f.rect_mm(-7.5, 0.05, 7.5, 24, C["chanfill"])
    for p in r1:
        f.poly(p, C["legfill"], C["leg"], 0)
    for w in w1:
        f.poly(w, C["wallfill"], C["wall"], 1.5)
    for p in s1:
        f.poly(p, C["sealfill"], C["seal"], 2)
    w2, r2, s2 = cross(30, 120)
    inner = [p for p in r2 if abs(p.centroid.x - 120) < 6]
    outer = [p for p in r2 if p not in inner]
    for p in outer:
        f.poly(p, C["legfill"], C["leg"], 0)
    for p in inner:
        f.poly(p, C["clitfill"], C["clit"], 0)
    for w in w2:
        f.poly(w, C["wallfill"], C["wall"], 1.5)
    for p in s2:
        f.poly(p, C["sealfill"], C["seal"], 2)

    yb = f.H - 190
    f.label((-28, 12), f.P(-40, 0)[0], yb, ["Leg chambers", "both blue domes;", "pull on the labia"], C["leg"], "middle")
    f.label((0, 12), f.P(0, 0)[0] + 170, yb + 110, ["Open channel", "nothing over it; fluids run out"], C["chan"], "middle")
    ic = inner[0].representative_point() if inner else Point(120, 30)
    f.label((ic.x, 45), f.P(85, 0)[0], 260, ["Clitoral chamber", "its own vacuum"], C["clit"], "end")
    f.label([tuple(p.representative_point().coords[0]) for p in outer], f.P(120, 0)[0], yb, ["Leg air around it"], C["leg"], "middle")
    vf = [p for p in s2 if p.centroid.y > 10]
    if vf:
        v = vf[0].centroid
        f.label((v.x, v.y), f.P(160, 0)[0], 300, ["Clitoral", "vent flap"], C["seal"], "start")
    f.save("cup020-cross-cuts.svg")


top_view()
side_view()
cross_view()
print("ok")
