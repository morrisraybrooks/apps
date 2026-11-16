"""Import SDF meshes into Blender, apply clear-Tritan + blue-silicone materials, light, render, save .blend.
blender -b --python build_blend.py -- <mesh_dir> <out.blend> <render_prefix> [samples]
"""
import bpy, sys, math, os
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
MESH, BLEND, PREFIX = argv[0], argv[1], argv[2]
SAMPLES = int(argv[3]) if len(argv) > 3 else 64

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.unit_settings.system = 'METRIC'; sc.unit_settings.scale_length = 0.001; sc.unit_settings.length_unit = 'MILLIMETERS'

def imp(name):
    bpy.ops.wm.ply_import(filepath=os.path.join(MESH, name + ".ply"))
    o = bpy.context.selected_objects[0]; o.name = name
    bpy.ops.object.shade_smooth()
    return o

def mat_tritan():
    m = bpy.data.materials.new("Clear Tritan"); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (0.86, 0.90, 0.97, 1)
    b.inputs["Roughness"].default_value = 0.03
    b.inputs["IOR"].default_value = 1.57
    b.inputs["Transmission Weight"].default_value = 1.0
    return m

def mat_silicone():
    m = bpy.data.materials.new("Blue Silicone"); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (0.05, 0.10, 0.45, 1)
    b.inputs["Roughness"].default_value = 0.35
    b.inputs["Subsurface Weight"].default_value = 0.3
    return m

shell = imp("shell"); shell.data.materials.append(mat_tritan())
seal = imp("seal"); seal.data.materials.append(mat_silicone())
for o in (shell, seal):
    o.location.z = 2.5   # sit seal on the floor

# floor + world
bpy.ops.mesh.primitive_plane_add(size=2000); fl = bpy.context.object; fl.name = "Floor"
fm = bpy.data.materials.new("Floor"); fm.use_nodes = True
fm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.92, 0.92, 0.93, 1)
fm.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 0.25
fl.data.materials.append(fm)
w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
w.node_tree.nodes["Background"].inputs[0].default_value = (0.8, 0.82, 0.86, 1)
w.node_tree.nodes["Background"].inputs[1].default_value = 0.6

def light(name, loc, energy, size):
    L = bpy.data.lights.new(name, 'AREA'); L.energy = energy; L.size = size
    o = bpy.data.objects.new(name, L); sc.collection.objects.link(o); o.location = loc
    d = Vector((0, 0, 20)) - o.location; o.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
for n, loc, e, s in [("Key", (-180, -220, 300), 9e5, 250), ("Fill", (260, -60, 160), 3.5e5, 300),
                     ("Rim", (0, 300, 220), 6e5, 200), ("Top", (0, 0, 400), 3e5, 400)]:
    light(n, loc, e, s)

cam_d = bpy.data.cameras.new("Cam"); cam_d.lens = 70; cam_d.clip_start = 1; cam_d.clip_end = 5000
cam = bpy.data.objects.new("Camera", cam_d); sc.collection.objects.link(cam); sc.camera = cam

sc.render.engine = 'CYCLES'; sc.cycles.samples = SAMPLES; sc.cycles.use_denoising = True
sc.cycles.max_bounces = 16; sc.cycles.transmission_bounces = 16
RES_PX = int(os.environ.get("PX", 1100)); sc.render.resolution_x = sc.render.resolution_y = RES_PX
sc.view_settings.view_transform = 'AgX'

def shot(tag, loc, target=(0, 5, 18)):
    cam.location = loc
    cam.rotation_euler = (Vector(target) - Vector(loc)).to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = f"{PREFIX}_{tag}.png"
    bpy.ops.render.render(write_still=True)

views = {"hero": (110, -230, 210), "top": (0, 5, 400), "side": (-330, -40, 70), "under": (60, -170, -230)}
for k in (argv[4].split(",") if len(argv) > 4 else views):
    if k == "under":
        fl.hide_render = True
    shot(k, views[k])
    fl.hide_render = False

cam.location = views["hero"]
cam.rotation_euler = (Vector((0, 5, 18)) - Vector(views["hero"])).to_track_quat('-Z', 'Y').to_euler()
bpy.ops.wm.save_as_mainfile(filepath=BLEND)
print("saved", BLEND)
