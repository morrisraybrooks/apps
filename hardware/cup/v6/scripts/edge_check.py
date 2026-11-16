"""Skin-safety check: are there sharp edges or thin slivers anywhere skin can touch?

usage: python edge_check.py <build_dir>      (dir with shell.stl, seal.stl, params.json)

Method
- Every 3rd mesh vertex (~0.5 mm coverage) over the WHOLE shell and seal is tested.
- Sharpness: the largest angle between a vertex normal and any vertex normal within 0.6 mm.
  A 1 mm round turns ~35-70 deg over that distance; a corner sharper than ~0.5 mm radius exceeds 70 deg.
- Thin sliver: a surface facing the opposite way less than 1.2 mm away.
- Skin-facing: step 0.8 mm off the surface along its normal; if that point is inside the cup envelope
  (the generator's `outer` field < 0) it is inside a chamber. Anything within 3 mm of the skin plane counts
  too. The whole silicone seal counts. Outside surfaces (port tubes, collars, top of the cup) are reported
  separately - skin never touches them.
"""
import sys, os, json, numpy as np, trimesh
from scipy.spatial import cKDTree

d = sys.argv[1]
HERE = os.path.dirname(os.path.abspath(__file__))
SHARP_DEG, RADIUS, THIN_MM = 70.0, 0.6, 1.2

# cup envelope from the generator itself (everything up to the ports), at 0.5 mm
src = open(os.path.join(HERE, "cup_sdf.py")).read().split("\n# ---------------- ports")[0]
argv0 = sys.argv
sys.argv = ["cup_sdf.py", "/tmp", "0.5", os.path.join(d, "params.json")]
ns = {"__name__": "edge_check"}; exec(src, ns)
sys.argv = argv0
outer, xs, ys, zs = ns["outer"], ns["xs"], ns["ys"], ns["zs"]

def inside_cup(p):
    i = np.clip(np.searchsorted(xs, p[:, 0]), 0, len(xs) - 1)
    j = np.clip(np.searchsorted(ys, p[:, 1]), 0, len(ys) - 1)
    k = np.clip(np.searchsorted(zs, p[:, 2]), 0, len(zs) - 1)
    return outer[i, j, k] < 0

report = {}
for name in ("shell", "seal"):
    m = trimesh.load(os.path.join(d, f"{name}.stl"))
    v, n = m.vertices, m.vertex_normals
    tree = cKDTree(v)
    idx = np.arange(0, len(v), 3)
    worst = np.array([np.degrees(np.arccos(np.clip((n[tree.query_ball_point(v[i], RADIUS)] @ n[i]).min(), -1, 1)))
                      for i in idx])
    thin = np.array([any(n[j] @ n[i] < -0.7 for j in tree.query_ball_point(v[i], THIN_MM)) for i in idx[::4]])
    if name == "shell":
        skin = inside_cup(v[idx] + n[idx] * 0.8) | (v[idx][:, 2] < 3.0)
    else:
        skin = np.ones(len(idx), bool)        # whole silicone part is held to the skin standard (lip AND vent flaps)
        lip = v[idx][:, 2] < 3.0
        sharp_tmp = worst > SHARP_DEG
        report["seal_breakdown"] = {"lip_sharp": int((sharp_tmp & lip).sum()), "vent_flaps_sharp": int((sharp_tmp & ~lip).sum())}
    sharp = worst > SHARP_DEG
    r = {"points_checked": int(len(idx)),
         "sharp_on_skin_side": int((sharp & skin).sum()),
         "thin_on_skin_side": int((thin & skin[::4]).sum()),
         "sharp_outside_only": int((sharp & ~skin).sum()),
         "worst_deg_skin_side": round(float(worst[skin].max()), 1) if skin.any() else 0.0}
    if (sharp & skin).any():
        r["where_mm"] = np.round(v[idx][sharp & skin][:8], 1).tolist()
    report[name] = r
report["PASS"] = all(report[k]["sharp_on_skin_side"] == 0 and report[k]["thin_on_skin_side"] == 0 for k in ("shell", "seal"))
print(json.dumps(report, indent=1))
