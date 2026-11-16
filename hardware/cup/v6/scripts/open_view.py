"""Open the cup framed in Material Preview: blender a_contour_cup_v2.blend --python open_view.py"""
import bpy


def setup():
    for win in bpy.context.window_manager.windows:
        for area in win.screen.areas:
            if area.type != 'VIEW_3D':
                continue
            sp = area.spaces.active
            sp.shading.type = 'MATERIAL'
            sp.show_region_ui = True                      # open the N sidebar ...
            for r in area.regions:
                if r.type == 'UI':
                    try:
                        r.active_panel_category = 'Cup'   # ... on the Cup tab
                    except Exception:
                        pass
            sp.clip_start = 0.5
            sp.clip_end = 10000
            region = next(r for r in area.regions if r.type == 'WINDOW')
            with bpy.context.temp_override(window=win, area=area, region=region):
                bpy.ops.view3d.view_axis(type='TOP')
                bpy.ops.view3d.view_orbit(angle=0.9, type='ORBITUP')
                bpy.ops.view3d.view_orbit(angle=0.5, type='ORBITLEFT')
                for o in bpy.context.view_layer.objects:
                    o.select_set(o.name in ('shell', 'seal'))
                bpy.ops.view3d.view_selected()
    return None


def load_params():
    """CUP_PARAMS=/path/params.json -> put those values on the Cup sidebar sliders (no rebuild)."""
    import json, os
    path = os.environ.get("CUP_PARAMS")
    cup = getattr(bpy.context.scene, "cup", None)
    if not path or cup is None:
        return None
    p = json.load(open(path))
    live, cup.live = cup.live, False
    for k, v in p.items():
        if hasattr(cup, k):
            try:
                setattr(cup, k, bool(v) if isinstance(getattr(cup, k), bool) else float(v))
            except Exception:
                pass
    cup.live = live
    cup.status = "Loaded " + os.path.basename(os.path.dirname(path))
    return None


bpy.app.timers.register(setup, first_interval=1.5)
bpy.app.timers.register(load_params, first_interval=1.0)
