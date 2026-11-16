"""A-Contour cup: signed-distance-field model -> watertight meshes (mm).

usage: python cup_sdf.py OUT_DIR [VOXEL_MM] [params.json]

Outputs (in OUT_DIR):
  shell.stl / shell.ply   clear PC/Tritan shell (hollow wall, ports bored)
  seal.stl  / seal.ply    silicone seal lip under the rims
  stats.json              size, volumes, watertight flags
Coordinates: X = width, Y = apex (+) to leg ends (-), Z = up; skin plane is Z = 0.
Every value in P can be overridden from params.json (the Blender "Cup" sidebar writes that file).
"""
import sys, json, numpy as np, trimesh
from skimage.measure import marching_cubes

OUT = sys.argv[1] if len(sys.argv) > 1 else "."
RES = float(sys.argv[2]) if len(sys.argv) > 2 else 0.4   # voxel size, mm

# ---------------- parameters (mm) - defaults = v5 ----------------
P = dict(
    wall=2.0,              # shell wall
    divider=1.5,           # wall round the clitoral chamber (1.0 per spec; 1.5 prints reliably)
    leg_top_x=9.0, leg_top_y=38.0,      # apex-end centre of the right leg (left mirrored)
    leg_foot_x=31.0, leg_foot_y=-44.0,  # foot-end centre
    leg_r_top=19.5, leg_r_foot=25.0,    # leg half-width at apex end / foot end
    leg_bulge=3.5,         # extra half-width mid-leg (full legs, not cones)
    leg_h_top=34.0, leg_h_foot=36.0,    # dome height at apex end / foot end
    apex_blend=8.0,        # how smoothly the two leg tops merge into the apex
    tear_top_y=54.0,       # clitoral teardrop chamber position (top of the bulb)
    tear_width=25.4,       # 1"
    tear_length=50.8,      # 2"
    tear_tail_r=3.5,
    tear_height=24.0,      # cavity roof height (deepest point)
    tear_flip=0,           # 1 = bulb DOWN toward the gap (over the glans), tail UP along the shaft/hood
    tear_nose=0,           # 1 = rounded NOSE-shaped chamber: highest/roundest over the bulb (tip), sloping down to the tail (bridge)
    tear_mouth=0.0,        # nose chamber: opening width at the skin, mm (0 = same as tear_width = no undercut)
    tear_bridge=0.55,
    tear_full=2.6,         # nose chamber fullness: 2 = pointed ellipse, 2.6 = full rounded nose, 4 = squarish      # nose chamber: height at the tail end as a fraction of the full depth
    tear_turret=0,         # 1 = chamber rises ABOVE the cup as a straight-walled teardrop turret (deep chamber)
    tear_dome=0.0,         # 0 = flat-roofed prism (v5); 2 = round dome .. 6 = boxy dome; deepest over the bulb
    slot_on=1,             # open channel between the legs
    slot_top_y=1.0, slot_top_w=9.0, slot_flare=0.11, slot_round=6.0,
    ribs_on=1,             # raised teardrop outline + stem seam on the outside
    rib_height=1.1,
    side_port_x=15.0, side_port_y=30.0,  # Port 1 (right) and AVL tap (left)
    inner_fillet=2.0,      # CLEANING: radius (mm) of the inside corners where the divider meets the shell
    vents_on=0,            # 1 = manual silicone flap vent on each chamber (lift the tab = instant vent, no power)
    vent_hole=3.0,         # vent hole diameter (mm)
    flap_d=14.0,           # silicone flap diameter (mm)
    leg_link=0,            # 1 = pathway joining the two legs BEHIND the clitoral chamber (round its tail)
    link_r=7.0,            # pathway outer radius (mm); air channel is about 2*(link_r - wall) wide
    link_gap=6.0,          # clearance from the clitoral chamber wall to the pathway centre-line (mm)
    one_outer_port=0,      # 1 = single outer port on top of the pathway (SOL1/SOL2/AVL sensor teed at the
                           #     controller); removes the side Port 1 and the separate AVL tap
    avl_tap=1,             # 1 = small AVL sensor tube on the left leg (0 = two ports only; sensor teed at the controller)
    wrap_on=0,             # 1 = TOP WRAP: the outer (leg) chamber wraps over the top end (bridge) of the nose chamber
                           #     with an adjustable air gap, so both legs join round it; replaces the crease where the
                           #     legs meet the turret with one smooth sweep from the seal up to the nose bridge
    wrap_gap=6.0,          # air gap between the nose-chamber wall and the outer wall at the top (mm) = the opening joining the legs
    wrap_flare=6.0,        # extra spread of the outer wall down at the skin (mm): wall slopes smoothly from the seal to the bridge
    wrap_start=0.35,       # where along the nose the gap starts opening (0 = at the bulb, 1 = only at the bridge end)
    wrap_blend=8.0,        # how smoothly the wrap blends into the legs (mm)
    wrap_top=0.0,          # height (mm) where the top wrap fades out (0 = follows the whole chamber, as cup_018)
    glans_on=0,            # 1 = clitoral chamber shaped like a penis GLANS on a short SHAFT (replaces the nose shape)
    shaft_d=27.0,          #     shaft inside diameter at the skin (mm) - must fit the fully engorged clitoris
    shaft_len=18.0,        #     shaft length from the skin to the corona (mm)
    corona_d=33.0,         #     corona (widest part of the glans) inside diameter (mm) - wider than the shaft
    glans_len=26.0,        #     corona to tip (mm)
    corona_back=5.0,       #     depth of the corona's underside, down toward the shaft (mm)
    neck_round=3.0,        #     rounding where the shaft meets the corona (mm) - no sharp ring on swollen tissue
    glans_tilt=15.0,       #     lean toward the apex / pubic bone (degrees)
    glans_y=0.0,           #     move the glans chamber up (+) toward the apex or down (-) (mm, from the old bulb centre)
    hood_ridge=1.5,        #     soft silicone ridge on the apex side of the mouth that holds the hood back (mm, 0 = off)
    hood_arc=120.0,        #     how far round the mouth that ridge runs (degrees)
    luer_od=6.6, luer_id=2.6, luer_len=11.0,
    avl_od=4.4, avl_id=2.0, avl_len=7.0,
    seal_w=2.6, seal_h=2.5,
    min_gap=3.0,           # SKIN SAFETY: fill any chamber crevice narrower than this (mm) so tissue can't be pinched (0 = off)
    edge_round=1.0,        # SKIN SAFETY: radius (mm) on every rim, mouth and port-hole edge (0 = sharp, as v5..cup_006)
)
if len(sys.argv) > 3:
    with open(sys.argv[3]) as fh:
        P.update({k: v for k, v in json.load(fh).items() if k in P})
G = type("G", (), P)   # G.wall etc.

# ---------------- grid (sized from the parameters) ----------------
reach_x = max(G.leg_top_x + G.leg_r_top, G.leg_foot_x + G.leg_r_foot) + G.leg_bulge + 14
reach_y_lo = G.leg_foot_y - G.leg_r_foot - 10
reach_y_hi = max(G.leg_top_y + G.leg_r_top, G.tear_top_y) + 16 + (G.wrap_gap + G.wrap_flare if G.wrap_on else 0)
_gl_h = (G.shaft_len + G.glans_len) * np.cos(np.radians(G.glans_tilt)) + G.corona_d / 2 * np.sin(np.radians(G.glans_tilt))
reach_z = max(G.leg_h_top, G.leg_h_foot, (G.tear_height if not G.glans_on else _gl_h) + 6 if G.tear_turret else 0) + 20
xs = np.arange(-reach_x, reach_x, RES, dtype=np.float32) + RES * 0.37
ys = np.arange(reach_y_lo, reach_y_hi, RES, dtype=np.float32) + RES * 0.29
zs = np.arange(-(G.seal_h + 3.0), reach_z, RES, dtype=np.float32) + RES * 0.5   # below the seal lip with margin; half-voxel offset: no node on z=0
X, Y, Z = np.meshgrid(xs, ys, zs, indexing="ij")
print("grid", X.shape, round(X.size / 1e6, 2), "M voxels", flush=True)

def smin(a, b, k):
    k = max(k, 1e-3)
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0, 1)
    return b * (1 - h) + a * h - k * h * (1 - h)

def smax(a, b, k):
    """Smooth intersection/subtraction: rounds the convex edge it creates (k ~ edge radius)."""
    return -smin(-a, -b, k) if k > 0 else np.maximum(a, b)

def leg(sign):
    """Tapered dome swept along a straight line: straight sides, round ends, dome cross-section."""
    ax, ay = sign * G.leg_top_x, G.leg_top_y
    bx, by = sign * G.leg_foot_x, G.leg_foot_y
    dx, dy = bx - ax, by - ay
    t = np.clip(((X - ax) * dx + (Y - ay) * dy) / (dx * dx + dy * dy), 0, 1)
    r = G.leg_r_top + t * (G.leg_r_foot - G.leg_r_top) + G.leg_bulge * np.sin(np.pi * t)
    h = G.leg_h_top + t * (G.leg_h_foot - G.leg_h_top)
    u = np.sqrt((X - ax - t * dx)**2 + (Y - ay - t * dy)**2) / r
    return (np.sqrt(u**2 + (Z / h)**2) - 1.0) * r

def normalize(f):
    """Turn an implicit field into ~true distance near its surface (so the wall stays its set thickness)."""
    g = np.gradient(f, RES)
    mag = np.sqrt(g[0]**2 + g[1]**2 + g[2]**2); del g
    return f / np.clip(mag, 0.25, 4.0)

def uneven_capsule_2d(px, py, r1, r2, h):
    """2D teardrop: circle r1 at origin, r2 at (0,-h)."""
    px = np.abs(px); py = -py
    b = (r1 - r2) / h; a = np.sqrt(1 - b * b)
    k = px * (-b) + py * a
    return np.where(k < 0, np.sqrt(px**2 + py**2) - r1,
           np.where(k > a * h, np.sqrt(px**2 + (py - h)**2) - r2, px * a + py * b - r1))

outer = normalize(smin(leg(-1), leg(+1), G.apex_blend))

# OPEN CHANNEL: through-slot between the legs, narrow & rounded under the teardrop tail, flaring to the feet
# gap edge blend can't exceed the gap's own top radius, or it leaves a crease at the gap's top corners
slot_k = min(G.slot_round, G.slot_top_w / 2 * 0.9)
hw = G.slot_top_w / 2 + G.slot_flare * np.maximum(G.slot_top_y - Y, 0)
slot = np.minimum(np.maximum(np.abs(X) - hw, Y - G.slot_top_y),
                  np.sqrt(X**2 + (Y - G.slot_top_y)**2) - G.slot_top_w / 2)
if G.slot_on:
    outer = normalize(-smin(-outer, slot, slot_k))   # each leg gets its own closed wall along the gap

# clitoral teardrop chamber (prism with domed roof)
r1 = G.tear_width / 2
tail_h = max(G.tear_length - r1 - G.tear_tail_r, 1)
if G.tear_flip:   # bulb at the bottom (toward the gap), tail pointing up toward the apex
    bulb_y = G.tear_top_y - G.tear_length + r1
    tear2 = uneven_capsule_2d(X, -(Y - bulb_y), r1, G.tear_tail_r, tail_h)
else:             # v5: bulb at the top, tail pointing down
    bulb_y = G.tear_top_y - r1
    tear2 = uneven_capsule_2d(X, Y - bulb_y, r1, G.tear_tail_r, tail_h)
if G.glans_on:
    # GLANS + SHAFT chamber. Axis starts at the mouth centre on the skin (over the clitoris, where the bulb was)
    # and leans glans_tilt toward the apex. Shaft = round tube (open at the skin); glans = ellipsoid head whose
    # widest ring (corona) is wider than the shaft, with a short underside (corona_back) and a rounded neck.
    tl = np.radians(G.glans_tilt)
    gax = np.array((0.0, np.sin(tl), np.cos(tl)), np.float32)
    gc0 = np.array((0.0, bulb_y + G.glans_y, 0.0), np.float32)
    px_, py_, pz_ = X - gc0[0], Y - gc0[1], Z - gc0[2]
    s_ax = py_ * gax[1] + pz_ * gax[2]
    rad = np.sqrt(np.maximum(px_**2 + py_**2 + pz_**2 - s_ax**2, 0)); del px_, py_, pz_
    Rs, Rc = G.shaft_d / 2, G.corona_d / 2
    shaft = np.maximum(rad - Rs, s_ax - G.shaft_len - 1.0)
    axg = s_ax - G.shaft_len
    Lax = np.where(axg > 0, G.glans_len, G.corona_back)
    head = (np.sqrt((rad / Rc)**2 + (axg / Lax)**2) - 1.0) * min(Rc, G.glans_len)
    del rad, axg, Lax
    tear = normalize(smin(shaft, head, G.neck_round))
    tear = np.maximum(tear, -Z - 50)
    del shaft, head, s_ax
    glans_tip = gc0 + gax * (G.shaft_len + G.glans_len)
elif G.tear_nose:
    # Swept rounded profile along the teardrop axis (bulb centre -> tail tip): every cross-section is a
    # half-ellipse, widest at height zc, so the chamber is rounded all over (no flat roof, no box walls).
    # Mouth width < inside width lifts the widest point above the skin (zc > 0): an undercut.
    sgn = -1.0 if G.tear_flip else 1.0
    ax_y0 = bulb_y
    ax_y1 = bulb_y - sgn * (G.tear_length - r1 - G.tear_tail_r)        # tail-tip circle centre
    t = np.clip((Y - ax_y0) / (ax_y1 - ax_y0), 0, 1)
    a = r1 + t * (G.tear_tail_r - r1)                                      # local half-width
    mouth = G.tear_mouth if G.tear_mouth > 0 else G.tear_width
    m = min(mouth / G.tear_width, 1.0)
    c_bulb = G.tear_height / (1.0 + np.sqrt(max(1 - m * m, 0)))            # vertical radius at the bulb
    zc_bulb = G.tear_height - c_bulb                                        # widest point height (0 = at the skin)
    c = c_bulb * (1 - t * (1 - G.tear_bridge))                              # bridge lower than the tip
    zc = zc_bulb * (1 - t * (1 - G.tear_bridge))
    u = np.sqrt(X**2 + (Y - (ax_y0 + t * (ax_y1 - ax_y0)))**2) / a
    pf = G.tear_full
    tear = normalize(((u**pf + (np.abs(Z - zc) / c)**pf) ** (1.0 / pf) - 1.0) * a)
    tear = np.maximum(tear, -Z - 50)                                       # (cavity open at the skin plane)
elif G.tear_dome > 0:
    # domed "bulb" roof: deepest over the bulb centre, shallower toward the edges and the tail.
    # Walls never lean inward past the mouth (no undercut) so swollen tissue can always come back out.
    pexp = G.tear_dome
    u = np.maximum(1.0 + tear2 / r1, 0.0)
    tear = normalize(((u**pexp + (np.abs(Z) / G.tear_height)**pexp) ** (1.0 / pexp) - 1.0) * r1)
else:
    tear = np.maximum(tear2, Z - G.tear_height)
    tear = np.where((tear2 > 0) & (Z > G.tear_height), np.sqrt(np.maximum(tear2, 0)**2 + (Z - G.tear_height)**2), tear)

if G.leg_link:
    # Low rounded tunnel, open to the skin like the legs, running from the left leg round the TAIL of the
    # clitoral chamber to the right leg: makes the outer chamber one continuous U so one port serves both legs.
    tail_y = G.tear_top_y if G.tear_flip else G.tear_top_y - G.tear_length
    ty = tail_y + (1 if G.tear_flip else -1) * (G.wall + G.divider + G.link_gap)   # arc apex behind the tail tip
    sy = 1 if G.tear_flip else -1
    pts = []
    for th in np.linspace(-np.pi * 0.62, np.pi * 0.62, 13):            # arc round the tail, ends inside the legs
        R = 22.0
        pts.append((R * np.sin(th), ty - sy * R * (1 - np.cos(th))))
    link = None
    for (x0, y0), (x1, y1) in zip(pts[:-1], pts[1:]):
        dx, dy = x1 - x0, y1 - y0
        t = np.clip(((X - x0) * dx + (Y - y0) * dy) / (dx * dx + dy * dy), 0, 1)
        seg = np.sqrt((X - x0 - t * dx)**2 + (Y - y0 - t * dy)**2 + Z**2) - G.link_r
        link = seg if link is None else np.minimum(link, seg)
    link_top = (0.0, ty)
    outer = smin(outer, link, 4.0)
else:
    link_top = None

if G.tear_turret:
    # turret skin = the chamber cavity grown outward by the wall; merged smoothly into the cup, gap re-cut
    # (no re-normalize here: both fields are already ~true distance, and rescaling shrank the cavity)
    if G.wrap_on:
        # TOP WRAP: grow the turret skin outward by an extra gap that opens smoothly from wrap_start along the
        # nose (bulb -> bridge) and is widest at the bridge end. The extra space is outer-chamber air that runs
        # over the top of the nose chamber and joins the left and right legs. wrap_flare widens it toward the
        # skin, so the outer wall sweeps smoothly from the seal up over the bridge (no crease, no undercut).
        sg = -1.0 if G.tear_flip else 1.0
        y0 = bulb_y
        if G.glans_on:     # round chamber: wrap opens over its apex-side half
            y0 = bulb_y + G.glans_y
            y1 = y0 + G.corona_d / 2
        else:
            y1 = bulb_y - sg * (G.tear_length - G.tear_width / 2 - G.tear_tail_r)    # bridge-end circle centre
        tw = np.clip((Y - y0) / (y1 - y0), 0, None)                               # 0 at bulb, 1 at bridge end, >1 beyond
        s_ = np.clip((tw - G.wrap_start) / max(1 - G.wrap_start, 1e-3), 0, 1)
        w = s_ * s_ * (3 - 2 * s_)                                                # smoothstep: no kink where it starts
        if G.wrap_top > 0:  # fade the wrap out above wrap_top, smoothly over 10 mm
            fz = np.clip((G.wrap_top - Z) / 10.0, 0, 1)
            w = w * fz * fz * (3 - 2 * fz)
            h_b = G.wrap_top
        else:
            h_b = max(G.tear_height * G.tear_bridge, 1.0)                         # bridge height
        spread = G.wrap_gap + G.wrap_flare * np.clip(1 - Z / h_b, 0, 1) ** 2      # flare is greatest at the seal
        # offset from the EXACT distance to the nose chamber (the nose field itself is only approximate and
        # stretched vertically: offsetting it by > 3 mm made a hump and a sharp saddle over the bridge)
        import skfmm
        tear_d = skfmm.distance(tear.astype(np.float64), dx=RES).astype(np.float32)
        outer = smin(outer, tear_d - G.wall - w * (G.divider / 2 + spread), G.wrap_blend)
        del tear_d
        wrap_w = w
    else:
        outer = smin(outer, tear - G.wall, 3.0)
    if G.slot_on:
        outer = -smin(-outer, slot, slot_k)

# Exact distance field for the final outer shape (after all smooth blends / turret / gap cut), so every
# wall offset below is truly G.wall thick - blended fields under-state distance and left 0.5 mm slivers.
# Fast marching from the (sub-voxel) zero surface: exact distances with NO voxel terracing.
# (cup_007 used a voxel distance transform here - it left 0.1 mm ripples over every surface.)
import skfmm
outer = skfmm.distance(outer.astype(np.float64), dx=RES).astype(np.float32)

# ---------------- hollowing ----------------
# smooth (not sharp) junction between the chamber air and the divider walls = filleted INSIDE corners,
# so residue can't pack into a crease where the divider meets the shell
air = smax(outer + G.wall, -(np.abs(tear) - G.divider / 2), G.inner_fillet)
if G.min_gap > 0:
    # Morphological opening of the chamber air: keep only space a ball of diameter min_gap fits into.
    # Narrow slits/wedges (e.g. where the clit bulb wall nearly touches the gap wall) become solid plastic.
    r = G.min_gap / 2
    air0 = skfmm.distance(air.astype(np.float64), dx=RES)
    core = air0 + r                                            # erode the chamber air by r ...
    if (core < 0).any():
        opened = skfmm.distance(core, dx=RES) - r              # ... then grow it back by r (sub-voxel exact)
        air = np.maximum(opened, air0).astype(np.float32)      # never add air outside the original chambers
    del air0, core
solid = np.maximum(outer, -air)

if G.ribs_on:   # raised teardrop outline + stem seam, on the outer skin only
    loop = np.abs(tear2) - 1.1
    stem = np.maximum(np.abs(X) - 1.0, np.maximum(Y - (G.tear_top_y - G.tear_length + 8), -Y - 30))
    rib = np.maximum(outer - G.rib_height, np.minimum(loop, stem))
    rib = np.maximum(rib, 5 - Z)
    rib = np.maximum(rib, -(outer + G.wall * 0.6))
    solid = np.minimum(solid, rib)

# ---------------- ports ----------------
def cyl(p0, d, r, L):
    d = np.asarray(d, np.float32); d /= np.linalg.norm(d)
    px, py, pz = X - p0[0], Y - p0[1], Z - p0[2]
    t = px * d[0] + py * d[1] + pz * d[2]
    rad = np.sqrt(np.maximum(px**2 + py**2 + pz**2 - t**2, 0))
    a = rad - r; b = np.abs(t - L / 2) - L / 2
    return np.minimum(np.maximum(a, b), 0) + np.sqrt(np.maximum(a, 0)**2 + np.maximum(b, 0)**2)

def port(field, p0, d, od, idd, L, collar=True):
    dn = np.asarray(d, np.float32) / np.linalg.norm(d)
    body = cyl(p0, d, od / 2, L)
    if collar:  # Luer-style lug collar near the tip
        body = np.minimum(body, cyl(np.asarray(p0) + dn * (L - 3.0), d, od / 2 + 1.0, 1.6))
    body = np.maximum(body, -air)      # SKIN SAFETY: no part of a port tube may stick into a chamber
    field = np.minimum(field, body)
    # port hole edges rounded (inside the chamber AND at the tip) so tissue drawn against a hole meets no sharp lip
    return smax(field, -cyl(np.asarray(p0) - dn * 8, d, idd / 2, L + 9), min(G.edge_round, idd * 0.45))

def surf_z(x, y):
    i = np.argmin(np.abs(xs - x)); j = np.argmin(np.abs(ys - y))
    inside = np.where(outer[i, j, :] < 0)[0]
    return float(zs[inside.max()]) - 1.2 if len(inside) else 10.0

py2 = bulb_y if G.tear_flip else G.tear_top_y - 8      # Port 2 over the bulb
p2, d2 = (0.0, py2, surf_z(0, py2)), np.array((0, 0.3, 1), np.float32)   # near-perpendicular entry: evenly rounded hole
if G.glans_on:      # Port 2 at the glans tip, straight along the chamber axis
    d2 = gax.copy()
    q = glans_tip.astype(np.float64)
    for _ in range(int(80 / (RES / 2))):        # walk out along the axis to the cup's OUTER surface
        i, j, k = (int(np.clip(np.searchsorted(a_, v_), 0, len(a_) - 1)) for a_, v_ in zip((xs, ys, zs), q))
        if outer[i, j, k] > 0:
            break
        q = q + gax * (RES / 2)
    p2 = tuple(q - gax * 1.2)
    # the sleeve below must reach from the outer surface all the way down to the glans tip
    gl_gap = float(np.linalg.norm(q - glans_tip))
# solid sleeve round the Port 2 bore where it crosses outer-chamber air above the clit dome,
# so Port 2 opens ONLY into the clitoral chamber (stops short of the clit cavity itself)
_sl = 12.0 if not G.glans_on else max(12.0, gl_gap + 4.0)
sleeve = cyl(np.asarray(p2) - d2 / np.linalg.norm(d2) * _sl, d2, G.luer_od / 2, _sl + 0.5)
if G.glans_on:     # blended (rounded) into the walls it crosses, then the clitoral cavity re-opened
    solid = np.maximum(smin(solid, sleeve, 2.0), -(tear + G.divider / 2))
else:
    solid = np.minimum(solid, np.maximum(sleeve, -tear))
solid = port(solid, p2, d2, G.luer_od, G.luer_id, G.luer_len)          # Port 2 clitoral
if G.one_outer_port and link_top is not None:
    # single outer-chamber port on the pathway, centred behind the clitoral chamber, leaning back
    lx, ly = link_top
    solid = port(solid, (lx, ly, surf_z(lx, ly)), (0, 0.15 if G.tear_flip else -0.15, 1), G.luer_od, G.luer_id, G.luer_len)   # straight through the pathway roof
else:
    sx, sy = G.side_port_x, G.side_port_y
    solid = port(solid, (sx, sy, surf_z(sx, sy)), (0.35, 0.3, 1), G.luer_od, G.luer_id, G.luer_len)     # Port 1 outer
    if G.avl_tap:
        solid = port(solid, (-sx, sy, surf_z(-sx, sy)), (-0.35, 0.3, 1), G.avl_od, G.avl_id, G.avl_len, False)   # AVL tap

# ---------------- manual flap vents (one-piece: silicone flap overmolded with the seal) ----------------
flaps = None
vent_info = []
if G.vents_on:
    def rcyl(p0, d, r, L, re):      # cylinder with rounded edges (no sharp silicone edges)
        dn = np.asarray(d, np.float32) / np.linalg.norm(d)
        return cyl(np.asarray(p0) + dn * re, d, max(r - re, 0.1), max(L - 2 * re, 0.1)) - re
    def capsule(a, b, r):
        a, b = np.asarray(a, np.float32), np.asarray(b, np.float32); ab = b - a
        t = np.clip(((X - a[0]) * ab[0] + (Y - a[1]) * ab[1] + (Z - a[2]) * ab[2]) / (ab @ ab), 0, 1)
        return np.sqrt((X - a[0] - t * ab[0])**2 + (Y - a[1] - t * ab[1])**2 + (Z - a[2] - t * ab[2])**2) - r
    def ray_exit(p, d):
        """walk from p along d until we leave the cup; return the surface point"""
        d = np.asarray(d, np.float64) / np.linalg.norm(d)
        for s_ in np.arange(0, 60, RES / 2):
            q = np.asarray(p) + d * s_
            i, j, k = (np.clip(np.searchsorted(a_, v_), 0, len(a_) - 1) for a_, v_ in zip((xs, ys, zs), q))
            if outer[i, j, k] > 0:
                return q
        return None
    def add_vent(p_in, d, chamber):
        global solid, flaps
        dn = np.asarray(d, np.float32) / np.linalg.norm(d)
        p_out = ray_exit(p_in, dn)
        if p_out is None:
            return
        if G.glans_on and chamber == "clitoral":
            # the hole crosses leg-chamber air on its way out: a solid sleeve round it, blended into both walls
            # with rounded corners, keeps the two chambers sealed from each other
            p_in_ = np.asarray(p_in, np.float32)
            Lv = float(np.linalg.norm(p_out - p_in_))
            sv = cyl(p_in_, dn, G.vent_hole / 2 + 1.8, Lv)
            solid = np.maximum(smin(solid, sv, 2.0), -(tear + G.divider / 2))     # re-open the clitoral cavity
        back = 10.0 if not (G.glans_on and chamber == "clitoral") else float(np.linalg.norm(p_out - np.asarray(p_in))) + 1.0
        hole = cyl(p_out - dn * back, dn, G.vent_hole / 2, back + 2.5)
        solid = smax(solid, -hole, min(max(G.edge_round, 1.3), G.vent_hole * 0.45))        # fully rounded hole edges
        # conformal silicone flap: a 1.6 mm patch lying ON the curved outer surface over the hole (overmolded,
        # no rigid seat); fully rounded edge
        px, py, pz = X - p_out[0], Y - p_out[1], Z - p_out[2]
        tt = px * dn[0] + py * dn[1] + pz * dn[2]
        rad = np.sqrt(np.maximum(px**2 + py**2 + pz**2 - tt**2, 0))
        th = 3.0                                                                           # thick enough for a full 1.5 mm round edge (soft silicone still lifts easily)
        rim = np.sqrt(px**2 + py**2 + pz**2) - G.flap_d / 2                              # spherical rim: square to a curved surface
        flap = smax(np.abs(outer - th / 2) - th / 2, rim, th / 2)
        flap = np.maximum(flap, -(outer + 0.05))                                           # never inside the shell
        flap = np.maximum(flap, np.maximum(-tt - 4.0, tt - 6.0))                           # only right at THIS hole (the axis line is infinite)
        top = p_out + dn * th
        side = np.cross(dn, (0, 0, 1.0)); side = side / (np.linalg.norm(side) + 1e-9) if np.linalg.norm(side) > 0.1 else np.array((1.0, 0, 0))
        tang = np.cross(side, dn); tang = tang / np.linalg.norm(tang)
        if tang[2] > 0: tang = -tang                                                         # tab points down the cup
        a0 = top + tang * (G.flap_d / 2 - 1.0)
        # rounded PULL KNOB at the flap edge (grip it and peel the flap up) - blended in, no wedge/slit under it
        kc = top + tang * (G.flap_d / 2 - 1.5) + dn * 1.5
        tab = np.sqrt((X - kc[0])**2 + (Y - kc[1])**2 + (Z - kc[2])**2) - 2.8
        f = smin(flap, tab, 2.5)                                                             # knob blends smoothly into the flap
        f = smax(f, -(outer + 0.05), 0.6)                                                    # silicone never inside the shell; soft edge
        flaps = f if flaps is None else np.minimum(flaps, f)
        vent_info.append({"chamber": chamber, "at_mm": [round(float(v), 1) for v in p_out]})
    # clitoral chamber: through the side of the turret, high (tissue can't reach), opening straight outward
    if G.glans_on:   # through the side of the glans, just above the corona
        add_vent(tuple(gc0 + gax * (G.shaft_len + 0.35 * G.glans_len)), (1, 0, 0.15), "clitoral")
    else:
        zc_v = G.tear_height * 0.78
        add_vent((0.0, bulb_y, zc_v), (1, 0, 0.15), "clitoral")
    # outer chamber: top of the left leg (where the old AVL tap was)
    # outer chamber: on the flatter CREST of the left leg (the steep outer flank made the flap rim sharp)
    lvx, lvy = -22.0, -18.0     # on the gentle part of the leg dome, clear of the curve down into the gap
    add_vent((lvx, lvy, surf_z(lvx, lvy) - 6), (-0.2, 0.0, 1), "outer (legs)")

if G.slot_on:
    solid = np.maximum(solid, -slot)
# cut at the skin plane; with edge_round every wall's bottom edge (mouth rims of legs, gap and clitoral
# chamber, divider) becomes a smooth round instead of a 90-degree corner
solid_foot = np.maximum(solid, -Z)[:, :, int(round((0.6 - zs[0]) / RES))].copy()   # full-width wall footprint (before rim rounding) for the seal
solid = smax(solid, -Z, G.edge_round)

# ---------------- silicone seal lip ----------------
k0 = int(round((0.6 - zs[0]) / RES))
foot = skfmm.distance(solid_foot.astype(np.float64), dx=RES)      # 2D footprint of the shell on the skin
if G.min_gap > 0:
    # SKIN SAFETY for the silicone lip: close slits narrower than min_gap (no pinch gap between two lip bands),
    # then open by 1 mm (no sharp corners in the lip outline)
    rg = G.min_gap / 2
    foot = skfmm.distance(skfmm.distance(foot - rg, dx=RES) + rg, dx=RES)     # closing
    foot = skfmm.distance(skfmm.distance(foot + 0.7, dx=RES) - 0.7, dx=RES)   # opening (< half the 2 mm wall)
q1 = foot[:, :, None].astype(np.float32) - 0.6                  # across the lip (wall + 0.6 mm each side)
q2 = np.abs(Z + G.seal_h / 2) - G.seal_h / 2            # through its thickness
rr = min(1.4, G.seal_h / 2 - 0.05) if G.edge_round > 0 else 0.0     # full, soft round on the lip
# rounded-rectangle cross-section: the silicone lip has no square corners against skin
seal = (np.sqrt(np.maximum(q1 + rr, 0)**2 + np.maximum(q2 + rr, 0)**2) + np.minimum(np.maximum(q1 + rr, q2 + rr), 0) - rr)
if G.glans_on and G.hood_ridge > 0:
    # HOOD RETRACTOR: soft rounded silicone ridge under the lip on the apex side of the clitoral mouth. It stands
    # hood_ridge below the seal, presses the skin above the clitoral hood and holds the hood back, so the
    # vacuum draws the glans out into the chamber. Nothing goes under the hood.
    tr = 1.8                                                         # ridge tube radius (fully round)
    zr = -G.seal_h - G.hood_ridge + tr
    ry = G.shaft_d / 2 / np.cos(np.radians(G.glans_tilt)) + G.divider / 2   # mouth ellipse (tilted tube meets the skin)
    rx = G.shaft_d / 2 + G.divider / 2
    phs = np.radians(np.linspace(-G.hood_arc / 2, G.hood_arc / 2, 15))
    pts = [(rx * np.sin(f), bulb_y + G.glans_y + ry * np.cos(f), zr) for f in phs]
    ridge = None
    for a_, b_ in zip(pts[:-1], pts[1:]):
        a_, b_ = np.asarray(a_, np.float32), np.asarray(b_, np.float32); ab = b_ - a_
        t_ = np.clip(((X - a_[0]) * ab[0] + (Y - a_[1]) * ab[1] + (Z - a_[2]) * ab[2]) / (ab @ ab), 0, 1)
        sg_ = np.sqrt((X - a_[0] - t_ * ab[0])**2 + (Y - a_[1] - t_ * ab[1])**2 + (Z - a_[2] - t_ * ab[2])**2) - tr
        ridge = sg_ if ridge is None else np.minimum(ridge, sg_)
    seal = smin(seal, ridge, 1.5)
if flaps is not None:
    seal = np.minimum(seal, flaps)     # vent flaps are part of the same overmolded silicone

stats = {"params": P, "voxel_mm": RES, "vents": vent_info}

def export(field, name):
    v, f, n, _ = marching_cubes(np.pad(field, 1, constant_values=5.0), 0.0, spacing=(RES, RES, RES))
    v += np.array([xs[0], ys[0], zs[0]]) - RES
    m = trimesh.Trimesh(v, f[:, ::-1], process=True)
    # no mesh smoothing: Taubin shot needle spikes off a few vertices; the SDF is already smooth
    parts = m.split(only_watertight=False)
    big = max(len(p.faces) for p in parts)
    m = trimesh.util.concatenate([p for p in parts if len(p.faces) > 0.005 * big])   # drop specks, keep vent flaps
    m.fill_holes()
    m.fix_normals()
    if m.volume < 0:
        m.invert()
    spike = float(m.edges_unique_length.max())
    stats[name] = {"faces": len(m.faces), "watertight": bool(m.is_watertight),
                   "size_mm": [round(float(e), 1) for e in m.extents],
                   "plastic_cm3": round(abs(m.volume) / 1000, 2), "spike_free": spike < 3 * RES}
    print(name, stats[name], flush=True)
    m.export(f"{OUT}/{name}.stl"); m.export(f"{OUT}/{name}.ply")

export(solid, "shell")
export(seal, "seal")

inside = (air < 0) & (Z > 0)
cv = RES**3 / 1000
clit = inside & (tear < 0)
stats["outer_chamber_cm3"] = round(float((inside & ~clit).sum() * cv), 1)
stats["clit_chamber_cm3"] = round(float(clit.sum() * cv), 1)
# how much of the requested clitoral chamber actually ended up inside the cup (1.0 = all of it)
wanted = (tear < -G.divider / 2) & (Z > 0)          # the cavity itself (divider wall excluded)
stats["clit_fit"] = round(float(clit.sum() / max(wanted.sum(), 1)), 3)
# the clitoral and outer chambers must stay sealed from each other (a port bore through the divider would join them)
from skimage.measure import label
empty_inside = (solid > 0) & (outer < 0) & (Z > 0.5)
lab = label(empty_inside, connectivity=1)
clit_ids = set(np.unique(lab[clit & empty_inside])) - {0}
outer_ids = set(np.unique(lab[inside & ~clit & empty_inside & (tear > G.divider)])) - {0}
# seed the outer chamber from the LEG air far from the clitoral chamber (air inside a port/vent bore that crosses
# the old leg-air space inside its sealed sleeve must not count as leg air)
legs_far = inside & ~clit & empty_inside & (tear > G.divider) & (Y < -20)
outer_ids = set(np.unique(lab[legs_far])) - {0}
stats["chambers_separate"] = not (clit_ids & outer_ids)
# are the LEFT and RIGHT legs one connected air space (so a single port pulls both)?
legs_air = inside & ~clit & empty_inside & (tear > G.divider) & (Y < -20)
left_ids = set(np.unique(lab[legs_air & (X < -8)])) - {0}
right_ids = set(np.unique(lab[legs_air & (X > 8)])) - {0}
stats["legs_connected"] = bool(left_ids & right_ids)
print("chambers", stats["outer_chamber_cm3"], stats["clit_chamber_cm3"], flush=True)
with open(f"{OUT}/stats.json", "w") as fh:
    json.dump(stats, fh, indent=1)
