bl_info = {
    "name": "A-Contour Cup Designer",
    "author": "morris + Claude",
    "version": (6, 0, 0),
    "blender": (4, 2, 0),
    "location": "3D View > Sidebar (N) > Cup",
    "description": "Sliders that rebuild the A-Contour vacuum cup (shell + seal) from cup_sdf.py",
    "category": "Object",
}

import bpy, json, os, shutil, subprocess, time
from bpy.props import FloatProperty, BoolProperty, StringProperty, PointerProperty
from bpy.types import Operator, Panel, PropertyGroup

HERE = os.path.dirname(os.path.realpath(__file__))
DEFAULT_SCRIPT = os.path.expanduser("~/jetson/jetson-vacuum-controller/hardware/cup/v6/cup_sdf.py")
DEFAULT_PYTHON = os.path.expanduser("~/.venvs/cup-sdf/bin/python")
PREVIEW_MM, FINAL_MM = 0.8, 0.3

# (prop name, label, default, min, max, section)  - names match cup_sdf.py's P dict
SLIDERS = [
    ("leg_top_x", "Apex: leg tops apart", 9.0, 0.0, 25.0, "Apex (top of the A)"),
    ("leg_top_y", "Apex: height up the A", 38.0, 20.0, 60.0, "Apex (top of the A)"),
    ("leg_r_top", "Apex: width", 19.5, 10.0, 35.0, "Apex (top of the A)"),
    ("leg_h_top", "Apex: dome height", 34.0, 20.0, 50.0, "Apex (top of the A)"),
    ("apex_blend", "Apex: roundness", 8.0, 0.5, 25.0, "Apex (top of the A)"),
    ("leg_foot_x", "Legs: spread at feet", 31.0, 15.0, 50.0, "Legs"),
    ("leg_foot_y", "Legs: length (feet position)", -44.0, -70.0, -20.0, "Legs"),
    ("leg_r_foot", "Legs: foot width", 25.0, 12.0, 40.0, "Legs"),
    ("leg_bulge", "Legs: fullness", 3.5, 0.0, 12.0, "Legs"),
    ("leg_h_foot", "Legs: dome height", 36.0, 15.0, 50.0, "Legs"),
    ("tear_width", "Clit chamber: width", 25.4, 12.0, 45.0, "Clitoral chamber"),
    ("tear_length", "Clit chamber: length", 50.8, 20.0, 75.0, "Clitoral chamber"),
    ("tear_top_y", "Clit chamber: position (top end)", 54.0, 10.0, 80.0, "Clitoral chamber"),
    ("tear_height", "Clit chamber: depth", 24.0, 10.0, 80.0, "Clitoral chamber"),
    ("tear_dome", "Clit chamber: dome (0 flat, 2 round, 6 boxy)", 0.0, 0.0, 6.0, "Clitoral chamber"),
    ("tear_mouth", "Clit chamber: mouth width (0 = full width)", 0.0, 0.0, 45.0, "Clitoral chamber"),
    ("tear_bridge", "Clit chamber: bridge height (tail, x depth)", 0.55, 0.2, 1.0, "Clitoral chamber"),
    ("tear_full", "Clit chamber: fullness (2 pointed, 4 square)", 2.6, 2.0, 4.0, "Clitoral chamber"),
    ("slot_top_w", "Gap: width at top", 9.0, 3.0, 30.0, "Open channel (gap)"),
    ("slot_flare", "Gap: widening toward feet", 0.11, 0.0, 0.4, "Open channel (gap)"),
    ("slot_top_y", "Gap: how far up it goes", 1.0, -30.0, 25.0, "Open channel (gap)"),
    ("slot_round", "Gap: edge roundness", 6.0, 0.5, 12.0, "Open channel (gap)"),
    ("shaft_d", "Glans: shaft width inside (mm)", 27.0, 15.0, 40.0, "Glans chamber (clitoral)"),
    ("shaft_len", "Glans: shaft length (mm)", 18.0, 5.0, 40.0, "Glans chamber (clitoral)"),
    ("corona_d", "Glans: corona width (mm)", 33.0, 15.0, 50.0, "Glans chamber (clitoral)"),
    ("glans_len", "Glans: head length (mm)", 26.0, 10.0, 45.0, "Glans chamber (clitoral)"),
    ("corona_back", "Glans: corona underside (mm)", 5.0, 1.0, 12.0, "Glans chamber (clitoral)"),
    ("neck_round", "Glans: neck rounding (mm)", 3.0, 1.0, 8.0, "Glans chamber (clitoral)"),
    ("glans_tilt", "Glans: lean toward apex (deg)", 15.0, 0.0, 40.0, "Glans chamber (clitoral)"),
    ("glans_y", "Glans: position up (+) / down (-) (mm)", 0.0, -10.0, 15.0, "Glans chamber (clitoral)"),
    ("hood_ridge", "Glans: hood retractor ridge (mm, 0 off)", 1.5, 0.0, 4.0, "Glans chamber (clitoral)"),
    ("hood_arc", "Glans: hood ridge length (deg)", 120.0, 40.0, 200.0, "Glans chamber (clitoral)"),
    ("wrap_top", "Top wrap: fades out above (mm, 0 = never)", 0.0, 0.0, 60.0, "Top wrap (legs joined over the nose)"),
    ("wrap_gap", "Top wrap: gap over the bridge (mm)", 6.0, 0.0, 20.0, "Top wrap (legs joined over the nose)"),
    ("wrap_flare", "Top wrap: slope out to the seal (mm)", 6.0, 0.0, 20.0, "Top wrap (legs joined over the nose)"),
    ("wrap_start", "Top wrap: starts along nose (0 bulb, 1 bridge)", 0.35, 0.0, 0.95, "Top wrap (legs joined over the nose)"),
    ("wrap_blend", "Top wrap: smoothness into the legs", 8.0, 1.0, 20.0, "Top wrap (legs joined over the nose)"),
    ("wall", "Wall thickness", 2.0, 1.0, 4.0, "Wall, seal, ports"),
    ("edge_round", "Edge rounding (skin safety, mm)", 1.0, 0.0, 2.0, "Wall, seal, ports"),
    ("min_gap", "Fill pinch slits narrower than (mm)", 3.0, 0.0, 6.0, "Wall, seal, ports"),
    ("inner_fillet", "Inside corner fillet (cleaning, mm)", 2.0, 0.0, 4.0, "Wall, seal, ports"),
    ("vent_hole", "Manual vent: hole size (mm)", 3.0, 1.5, 5.0, "Wall, seal, ports"),
    ("flap_d", "Manual vent: flap size (mm)", 14.0, 8.0, 20.0, "Wall, seal, ports"),
    ("divider", "Clit chamber divider", 1.5, 0.8, 3.0, "Wall, seal, ports"),
    ("seal_h", "Seal lip thickness", 2.5, 1.0, 5.0, "Wall, seal, ports"),
    ("seal_w", "Seal lip width", 2.6, 1.0, 6.0, "Wall, seal, ports"),
    ("link_r", "Leg pathway: size", 7.0, 4.0, 12.0, "Wall, seal, ports"),
    ("link_gap", "Leg pathway: distance behind clit chamber", 6.0, 3.0, 15.0, "Wall, seal, ports"),
    ("side_port_x", "Side ports: spread", 15.0, 5.0, 35.0, "Wall, seal, ports"),
    ("side_port_y", "Side ports: position", 30.0, 0.0, 50.0, "Wall, seal, ports"),
]
TOGGLES = [("slot_on", "Open channel between legs", True), ("ribs_on", "Raised teardrop outline", True),
           ("tear_flip", "Clit teardrop bulb DOWN (over the glans)", False),
           ("tear_turret", "Clit chamber turret (rises above the cup)", False),
           ("tear_nose", "Clit chamber nose shape (rounded)", False),
           ("leg_link", "Pathway joining the legs behind the clit chamber", False),
           ("one_outer_port", "One outer port (on the pathway; no AVL tap)", False),
           ("glans_on", "Clit chamber glans + shaft shape", False),
           ("wrap_on", "Top wrap: legs join over the nose bridge", False),
           ("avl_tap", "Small AVL sensor tube on left leg", True),
           ("vents_on", "Manual flap vents (lift tab to release)", False)]
SECTIONS = []
for s in SLIDERS:
    if s[5] not in SECTIONS:
        SECTIONS.append(s[5])

_job = {"proc": None, "final": False, "started": 0.0, "pending": False, "last_change": 0.0}


def work_dir():
    d = os.path.join(os.path.dirname(bpy.context.scene.cup.script_path), "work")
    os.makedirs(d, exist_ok=True)
    return d


def params_dict(cup):
    p = {name: getattr(cup, name) for name, *_ in SLIDERS}
    p.update({name: int(getattr(cup, name)) for name, *_ in TOGGLES})
    return p


def on_change(self, context):
    _job["last_change"] = time.time()
    if self.live:
        _job["pending"] = True
        if not bpy.app.timers.is_registered(_live_tick):
            bpy.app.timers.register(_live_tick, first_interval=0.4)


def _live_tick():
    # wait until sliders have been still for 0.5 s and no build is running, then preview
    if _job["proc"] is not None or time.time() - _job["last_change"] < 0.5:
        return 0.3
    if _job["pending"]:
        _job["pending"] = False
        start_build(bpy.context, final=False)
    return None


class CupSettings(PropertyGroup):
    __annotations__ = {}
    for _n, _l, _d, _lo, _hi, _s in SLIDERS:
        __annotations__[_n] = FloatProperty(name=_l, default=_d, min=_lo, max=_hi, step=10,
                                            precision=2, unit='NONE', update=on_change)
    for _n, _l, _d in TOGGLES:
        __annotations__[_n] = BoolProperty(name=_l, default=_d, update=on_change)
    __annotations__["live"] = BoolProperty(name="Live preview while dragging", default=True)
    __annotations__["status"] = StringProperty(default="")
    __annotations__["stats"] = StringProperty(default="")
    __annotations__["script_path"] = StringProperty(name="cup_sdf.py", default=DEFAULT_SCRIPT, subtype='FILE_PATH')
    __annotations__["python_path"] = StringProperty(name="Python", default=DEFAULT_PYTHON, subtype='FILE_PATH')
    __annotations__["show_paths"] = BoolProperty(name="Show paths", default=False)


def _material(name, color, transmission, rough):
    m = bpy.data.materials.get(name)
    if m:
        return m
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = color
    b.inputs["Roughness"].default_value = rough
    b.inputs["Transmission Weight"].default_value = transmission
    b.inputs["IOR"].default_value = 1.57
    return m


def swap_mesh(obj_name, ply_path, mat):
    target = bpy.data.objects.get(obj_name)      # look up BEFORE import: the importer names its object "shell"/"seal" too
    before = set(bpy.data.objects)
    bpy.ops.wm.ply_import(filepath=ply_path)
    new = [o for o in bpy.data.objects if o not in before][0]
    if target is None:   # first build in an empty scene
        new.name = obj_name; new.location.z = 2.5
        new.data.materials.append(mat)
        target = new
    else:
        old = target.data
        mats = list(old.materials)
        target.data = new.data
        target.data.name = obj_name
        for m in (mats or [mat]):
            target.data.materials.append(m)
        bpy.data.objects.remove(new)
        if old.users == 0:
            bpy.data.meshes.remove(old)
    for p in target.data.polygons:
        p.use_smooth = True


def start_build(context, final):
    cup = context.scene.cup
    if _job["proc"] is not None:
        _job["pending"] = True
        return
    if not (os.path.exists(cup.python_path) and os.path.exists(cup.script_path)):
        cup.status = "Missing python or cup_sdf.py - check paths"
        return
    wd = work_dir()
    pj = os.path.join(wd, "params.json")
    with open(pj, "w") as fh:
        json.dump(params_dict(cup), fh, indent=1)
    res = FINAL_MM if final else PREVIEW_MM
    log = open(os.path.join(wd, "build.log"), "w")
    _job["proc"] = subprocess.Popen([cup.python_path, cup.script_path, wd, str(res), pj],
                                    stdout=log, stderr=subprocess.STDOUT)
    _job["final"] = final
    _job["started"] = time.time()
    cup.status = "Building FINAL (0.3 mm, about a minute)..." if final else "Building preview..."
    bpy.app.timers.register(_poll, first_interval=0.3)


def _poll():
    proc = _job["proc"]
    cup = bpy.context.scene.cup
    if proc.poll() is None:
        cup.status = ("Building FINAL" if _job["final"] else "Building preview") + \
            f"... {time.time() - _job['started']:.0f}s"
        _redraw()
        return 0.3
    _job["proc"] = None
    wd = work_dir()
    if proc.returncode != 0:
        with open(os.path.join(wd, "build.log")) as fh:
            tail = fh.read().strip().splitlines()[-1:] or ["?"]
        cup.status = "Build failed: " + tail[0][:120]
        _redraw()
        return None
    swap_mesh("shell", os.path.join(wd, "shell.ply"), _material("Clear Tritan", (0.86, 0.9, 0.97, 1), 1.0, 0.03))
    swap_mesh("seal", os.path.join(wd, "seal.ply"), _material("Blue Silicone", (0.05, 0.1, 0.45, 1), 0.0, 0.35))
    with open(os.path.join(wd, "stats.json")) as fh:
        st = json.load(fh)
    sh = st["shell"]
    ok = sh["watertight"] and st["seal"]["watertight"] and sh["spike_free"]
    cup.stats = (f"{sh['size_mm'][0]:.0f} x {sh['size_mm'][1]:.0f} x {sh['size_mm'][2]:.0f} mm | "
                 f"outer {st['outer_chamber_cm3']:.0f} cm3 | clit {st['clit_chamber_cm3']:.1f} cm3 | "
                 + ("printable" if ok else "NOT watertight"))
    if not st.get("legs_connected", True):
        cup.stats += " | NOTE: left and right legs are NOT connected - each leg needs its own port (turn on Top wrap and widen its gap)"
    if not st.get("chambers_separate", True):
        cup.stats += " | WARNING: a port breaks through the divider - clit and outer chambers are joined; move the side ports"
    p = st.get("params", {})
    if p.get("wrap_on") and p.get("wrap_gap", 9) <= p.get("min_gap", 0) + 0.5:
        cup.stats += " | NOTE: top-wrap gap is not wider than the pinch-slit fill - it will be filled solid; widen it"
    if p.get("glans_on") and p.get("corona_d", 0) > p.get("shaft_d", 0):
        cup.stats += " | NOTE: corona wider than shaft - vent fully and wait for swelling to go down before removing"
    if p.get("tear_nose") and 0 < p.get("tear_mouth", 0) < 0.98 * p.get("tear_width", 1):
        cup.stats += " | NOTE: mouth narrower than inside - vent fully before removing so swollen tissue releases"
    fit = st.get("clit_fit", 1.0)
    if fit < 0.97:
        cup.stats += (f" | WARNING: only {fit * 100:.0f}% of the clit chamber is inside the cup"
                      " - lower its position/length or raise/widen the apex")
    cup.status = ("FINAL ready - use Save as new version" if _job["final"] else "Preview ready") + \
        f" ({time.time() - _job['started']:.0f}s)"
    _job["last_final"] = _job["final"]
    if _job["pending"]:
        _job["pending"] = False
        start_build(bpy.context, final=False)
    _redraw()
    return None


def _redraw():
    for w in bpy.context.window_manager.windows:
        for a in w.screen.areas:
            if a.type == 'VIEW_3D':
                a.tag_redraw()


class CUP_OT_preview(Operator):
    bl_idname = "cup.preview"; bl_label = "Rebuild preview"
    bl_description = "Rebuild the cup quickly (0.8 mm detail) from the sliders"
    def execute(self, context):
        start_build(context, final=False); return {'FINISHED'}


class CUP_OT_final(Operator):
    bl_idname = "cup.final"; bl_label = "Build final (print quality)"
    bl_description = "Rebuild at 0.3 mm detail for printing (about a minute)"
    def execute(self, context):
        start_build(context, final=True); return {'FINISHED'}


class CUP_OT_save(Operator):
    bl_idname = "cup.save_version"; bl_label = "Save as new version"
    bl_description = "Copy the current STLs + settings into a new numbered folder. Never overwrites."
    def execute(self, context):
        cup = context.scene.cup
        wd = work_dir()
        if not _job.get("last_final"):
            self.report({'WARNING'}, "Build final first so the saved STL is print quality")
            return {'CANCELLED'}
        base = os.path.join(os.path.dirname(cup.script_path), "saved")
        os.makedirs(base, exist_ok=True)
        n = 1
        while os.path.exists(os.path.join(base, f"cup_{n:03d}")):
            n += 1
        dst = os.path.join(base, f"cup_{n:03d}")
        os.makedirs(dst)
        for f in ("shell.stl", "seal.stl", "params.json", "stats.json", "skin_safety_check.json"):
            if not os.path.exists(os.path.join(wd, f)):
                continue
            shutil.copy2(os.path.join(wd, f), os.path.join(dst, f))
        self.report({'INFO'}, f"Saved {dst}")
        cup.status = f"Saved saved/cup_{n:03d}"
        return {'FINISHED'}


class CUP_OT_load(Operator):
    bl_idname = "cup.load_params"; bl_label = "Load settings"
    bl_description = "Load slider settings from a params.json (e.g. a saved version)"
    filepath: StringProperty(subtype='FILE_PATH')
    def invoke(self, context, event):
        context.window_manager.fileselect_add(self); return {'RUNNING_MODAL'}
    def execute(self, context):
        cup = context.scene.cup
        with open(self.filepath) as fh:
            p = json.load(fh)
        live = cup.live; cup.live = False
        for name, *_ in SLIDERS + TOGGLES:
            if name in p:
                setattr(cup, name, p[name])
        cup.live = live
        start_build(context, final=False)
        return {'FINISHED'}


class CUP_OT_reset(Operator):
    bl_idname = "cup.reset"; bl_label = "Reset to v5 shape"
    def execute(self, context):
        cup = context.scene.cup
        live = cup.live; cup.live = False
        for name, _l, d, *_ in SLIDERS:
            setattr(cup, name, d)
        for name, _l, d in TOGGLES:
            setattr(cup, name, d)
        cup.live = live
        start_build(context, final=False)
        return {'FINISHED'}


class CUP_OT_skin_check(Operator):
    bl_idname = "cup.skin_check"; bl_label = "Skin safety check"
    bl_description = "Scan the last FINAL build for sharp edges or thin slivers on any surface skin can touch (~1 min)"
    def execute(self, context):
        cup = context.scene.cup
        if not _job.get("last_final"):
            self.report({'WARNING'}, "Build final first - the check needs the print-quality mesh")
            return {'CANCELLED'}
        wd = work_dir()
        script = os.path.join(os.path.dirname(cup.script_path), "edge_check.py")
        out = subprocess.run([cup.python_path, script, wd], capture_output=True, text=True)
        try:
            r = json.loads(out.stdout[out.stdout.index("{"):])
            cup.status = ("SKIN SAFETY: PASS - no sharp edges or slivers where skin touches" if r["PASS"] else
                          f"SKIN SAFETY: FAIL - {r['shell']['sharp_on_skin_side'] + r['seal']['sharp_on_skin_side']} sharp, "
                          f"{r['shell']['thin_on_skin_side'] + r['seal']['thin_on_skin_side']} thin spots on the skin side")
            with open(os.path.join(wd, "skin_safety_check.json"), "w") as fh:
                json.dump(r, fh, indent=1)
        except Exception:
            cup.status = "Skin safety check failed to run: " + (out.stderr.strip().splitlines() or ["?"])[-1][:100]
        return {'FINISHED'}


class CUP_PT_main(Panel):
    bl_space_type = 'VIEW_3D'; bl_region_type = 'UI'; bl_category = "Cup"
    bl_label = "A-Contour Cup"
    def draw(self, context):
        cup = context.scene.cup
        col = self.layout.column(align=True)
        col.scale_y = 1.3
        col.prop(cup, "live")
        col.operator("cup.preview", icon='FILE_REFRESH')
        col.operator("cup.final", icon='MOD_REMESH')
        col.operator("cup.skin_check", icon='CHECKMARK')
        col.operator("cup.save_version", icon='FILE_TICK')
        row = col.row(align=True)
        row.operator("cup.load_params", icon='FILE_FOLDER')
        row.operator("cup.reset", icon='LOOP_BACK')
        if cup.status:
            self.layout.label(text=cup.status, icon='INFO')
        if cup.stats:
            for part in cup.stats.split(" | "):
                self.layout.label(text=part)
        for n, _l, _d in TOGGLES:
            self.layout.prop(cup, n)


def _section_panel(title, idx):
    class P(Panel):
        bl_space_type = 'VIEW_3D'; bl_region_type = 'UI'; bl_category = "Cup"
        bl_parent_id = "CUP_PT_main"; bl_label = title
        bl_idname = f"CUP_PT_sec{idx}"
        def draw(self, context):
            col = self.layout.column(align=True)
            col.scale_y = 1.2
            for name, label, *_rest in SLIDERS:
                if _rest[-1] == title:
                    col.prop(context.scene.cup, name, text=label.split(": ")[-1].capitalize())
    P.__name__ = f"CUP_PT_sec{idx}"
    return P


class CUP_PT_paths(Panel):
    bl_space_type = 'VIEW_3D'; bl_region_type = 'UI'; bl_category = "Cup"
    bl_parent_id = "CUP_PT_main"; bl_label = "Paths"; bl_options = {'DEFAULT_CLOSED'}
    def draw(self, context):
        self.layout.prop(context.scene.cup, "script_path")
        self.layout.prop(context.scene.cup, "python_path")


CLASSES = [CupSettings, CUP_OT_preview, CUP_OT_final, CUP_OT_skin_check, CUP_OT_save, CUP_OT_load, CUP_OT_reset, CUP_PT_main] + \
          [_section_panel(t, i) for i, t in enumerate(SECTIONS)] + [CUP_PT_paths]


def register():
    for c in CLASSES:
        bpy.utils.register_class(c)
    bpy.types.Scene.cup = PointerProperty(type=CupSettings)


def unregister():
    del bpy.types.Scene.cup
    for c in reversed(CLASSES):
        bpy.utils.unregister_class(c)


if __name__ == "__main__":
    register()
