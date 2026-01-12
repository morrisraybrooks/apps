"""
Render the A-Contour Cup from the saved blend file
Fast render with lower samples for quick preview
"""
import bpy
import os

# Get the directory of this script
script_dir = os.path.dirname(os.path.abspath(__file__))
blend_path = os.path.join(script_dir, "a_contour_cup.blend")

# Open the blend file
bpy.ops.wm.open_mainfile(filepath=blend_path)

# Render settings - FAST preview quality
scene = bpy.context.scene
scene.render.engine = 'CYCLES'
scene.cycles.samples = 64  # Lower for speed (was 256)
scene.render.resolution_x = 1080
scene.render.resolution_y = 1440
scene.render.film_transparent = True

# Enable denoising for cleaner result even with low samples
scene.cycles.use_denoising = True
scene.cycles.denoiser = 'OPENIMAGEDENOISE'

# Use GPU if available for faster rendering
scene.cycles.device = 'GPU'
prefs = bpy.context.preferences.addons['cycles'].preferences
prefs.compute_device_type = 'CUDA'  # or 'OPTIX' or 'HIP' depending on your GPU
prefs.get_devices()
for device in prefs.devices:
    device.use = True

# Render
render_path = os.path.join(script_dir, "a_contour_cup_render_no_bridge.png")
scene.render.filepath = render_path
print(f"\n🎬 Starting render (64 samples with denoising)...")
bpy.ops.render.render(write_still=True)
print(f"✅ Rendered: {render_path}")

