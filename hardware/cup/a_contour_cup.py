"""
A-Contour Vacuum Cup - Blender Python Script
=============================================
Generates the anatomical dual-chamber vacuum cup design matching the reference image.

Design Features:
- Two large elongated dome chambers (left and right A-legs)
- Connected at apex where clitoral cylinder (Zone 2) sits
- A-Crossbar internal diaphragm separating zones
- Open channel between the two leg chambers
- Smooth, organic silicone-like appearance

Dimensions (from documentation):
- Total width: ~125mm
- Depth: ~62mm
- Channel width: ~29mm
- Clitoral cylinder: 1" wide x 2" tall (25.4mm x 50.8mm)

Usage:
  blender --python a_contour_cup.py
"""

import bpy
import bmesh
import math
from mathutils import Vector

# =============================================================================
# PARAMETERS (all in mm, converted to Blender units = meters)
# =============================================================================
MM = 0.001  # 1mm in Blender meters

# Overall dimensions from documentation
TOTAL_WIDTH = 125 * MM       # ~125mm total width
TOTAL_LENGTH = 100 * MM      # Length from apex to bottom of legs
DOME_HEIGHT = 62 * MM        # Cup depth/height (~62mm from side view)
WALL_THICKNESS = 2.5 * MM    # Shell wall thickness

# A-Shape profile dimensions (from documentation)
CHANNEL_WIDTH = 29 * MM      # Open channel width between legs (~29mm from docs)
LEG_WIDTH = 45 * MM          # Width of each leg chamber
LEG_LENGTH = 80 * MM         # Length of each leg (80mm total vulva coverage per doc)

# Zone 2: Clitoral Cylinder (A-Apex) - from documentation
# Dimensions: 1 inch wide x 2 inches tall
CYLINDER_DIAMETER = 25.4 * MM  # Width: 1 inch = 25.4mm
CYLINDER_HEIGHT = 50.8 * MM    # Height: 2 inches = 50.8mm

# A-Crossbar (internal diaphragm)
CROSSBAR_THICKNESS = 2 * MM

# Colors - metallic/glass appearance like reference image
SHELL_COLOR = (0.7, 0.75, 0.8, 0.85)        # Silver/chrome translucent
ZONE2_COLOR = (0.6, 0.65, 0.7, 0.9)         # Slightly darker for Zone 2
CROSSBAR_COLOR = (0.4, 0.45, 0.5, 0.95)     # Dark chrome for crossbar
SEAL_COLOR = (0.3, 0.35, 0.4, 1.0)          # Dark edge lines


def clear_scene():
    """Remove all mesh objects from scene"""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)


def create_glass_material(name, color, roughness=0.1):
    """Create a glass/chrome-like material matching reference image"""
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    mat.blend_method = 'BLEND'

    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    output = nodes.new('ShaderNodeOutputMaterial')
    output.location = (400, 0)

    mix = nodes.new('ShaderNodeMixShader')
    mix.location = (200, 0)
    mix.inputs[0].default_value = 0.3

    glass = nodes.new('ShaderNodeBsdfGlass')
    glass.location = (0, 100)
    glass.inputs['Color'].default_value = color[:3] + (1.0,)
    glass.inputs['Roughness'].default_value = roughness
    glass.inputs['IOR'].default_value = 1.45

    glossy = nodes.new('ShaderNodeBsdfGlossy')
    glossy.location = (0, -100)
    glossy.inputs['Color'].default_value = color[:3] + (1.0,)
    glossy.inputs['Roughness'].default_value = roughness * 0.5

    links.new(glass.outputs['BSDF'], mix.inputs[1])
    links.new(glossy.outputs['BSDF'], mix.inputs[2])
    links.new(mix.outputs['Shader'], output.inputs['Surface'])

    return mat


def create_simple_material(name, color):
    """Create a simple transparent material"""
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    mat.blend_method = 'BLEND'

    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = color[:3] + (1.0,)
    bsdf.inputs["Alpha"].default_value = color[3]
    bsdf.inputs["Roughness"].default_value = 0.15
    bsdf.inputs["Metallic"].default_value = 0.8
    bsdf.inputs["Specular IOR Level"].default_value = 0.8

    return mat


def create_single_leg_chamber(is_left=True):
    """
    Create one A-leg chamber as an elongated dome that angles inward toward apex.
    Per documentation: A-legs provide 80mm total vulva coverage (labia majora/minora).
    The legs should converge toward the apex to form the characteristic A-shape.
    """
    sign = -1 if is_left else 1
    name = "LeftLeg" if is_left else "RightLeg"

    mball = bpy.data.metaballs.new(f"{name}_Mball")
    mball_obj = bpy.data.objects.new(name, mball)
    bpy.context.collection.objects.link(mball_obj)

    mball.resolution = 0.002
    mball.threshold = 0.6

    # Base X offset - legs spread outward at bottom, converge at top (A-shape)
    x_base = sign * (CHANNEL_WIDTH / 2 + LEG_WIDTH / 2)

    # Main body - large central ellipsoid (labia coverage)
    main = mball.elements.new()
    main.type = 'ELLIPSOID'
    main.co = (x_base, 0, DOME_HEIGHT * 0.45)
    main.radius = LEG_WIDTH * 0.5
    main.size_x = 0.9
    main.size_y = 1.8  # Elongated along Y (anterior-posterior)
    main.size_z = 1.2

    # Upper section - angled INWARD toward apex (A-shape convergence)
    upper = mball.elements.new()
    upper.type = 'ELLIPSOID'
    upper.co = (x_base * 0.7, LEG_LENGTH * 0.25, DOME_HEIGHT * 0.6)  # More inward
    upper.radius = LEG_WIDTH * 0.42
    upper.size_x = 0.8
    upper.size_y = 1.0
    upper.size_z = 1.0

    # Apex connector - angles strongly inward to meet at apex triangle
    apex_conn = mball.elements.new()
    apex_conn.type = 'ELLIPSOID'
    apex_conn.co = (x_base * 0.5, LEG_LENGTH * 0.38, DOME_HEIGHT * 0.55)  # Strong inward angle
    apex_conn.radius = LEG_WIDTH * 0.38
    apex_conn.size_x = 0.75
    apex_conn.size_y = 0.8
    apex_conn.size_z = 0.9

    # Lower section - stays wider (perineum/vestibular bulb coverage)
    lower = mball.elements.new()
    lower.type = 'ELLIPSOID'
    lower.co = (x_base * 1.1, -LEG_LENGTH * 0.2, DOME_HEIGHT * 0.35)
    lower.radius = LEG_WIDTH * 0.42
    lower.size_x = 0.85
    lower.size_y = 1.0
    lower.size_z = 0.9

    # Bottom tip - slight outward splay (anatomical perineum contour)
    tip = mball.elements.new()
    tip.type = 'ELLIPSOID'
    tip.co = (x_base * 1.15, -LEG_LENGTH * 0.4, DOME_HEIGHT * 0.25)
    tip.radius = LEG_WIDTH * 0.35
    tip.size_x = 0.75
    tip.size_y = 0.75
    tip.size_z = 0.7

    # Convert to mesh
    bpy.context.view_layer.objects.active = mball_obj
    mball_obj.select_set(True)
    bpy.ops.object.convert(target='MESH')

    obj = bpy.context.active_object
    obj.name = f"Zone1_{name}"
    bpy.ops.object.shade_smooth()

    return obj


def create_apex_bridge():
    """Create the apex region that connects the two legs at the top."""
    mball = bpy.data.metaballs.new("Apex_Mball")
    mball_obj = bpy.data.objects.new("ApexBridge", mball)
    bpy.context.collection.objects.link(mball_obj)

    mball.resolution = 0.002
    mball.threshold = 0.55

    # Main apex dome
    apex = mball.elements.new()
    apex.type = 'ELLIPSOID'
    apex.co = (0, LEG_LENGTH * 0.35, DOME_HEIGHT * 0.65)
    apex.radius = 30 * MM
    apex.size_x = 1.5
    apex.size_y = 0.8
    apex.size_z = 0.9

    # Left connection to left leg
    left_conn = mball.elements.new()
    left_conn.type = 'ELLIPSOID'
    left_conn.co = (-CHANNEL_WIDTH * 0.4, LEG_LENGTH * 0.28, DOME_HEIGHT * 0.55)
    left_conn.radius = 22 * MM
    left_conn.size_x = 1.0
    left_conn.size_y = 0.8
    left_conn.size_z = 0.85

    # Right connection to right leg
    right_conn = mball.elements.new()
    right_conn.type = 'ELLIPSOID'
    right_conn.co = (CHANNEL_WIDTH * 0.4, LEG_LENGTH * 0.28, DOME_HEIGHT * 0.55)
    right_conn.radius = 22 * MM
    right_conn.size_x = 1.0
    right_conn.size_y = 0.8
    right_conn.size_z = 0.85

    # Convert to mesh
    bpy.context.view_layer.objects.active = mball_obj
    mball_obj.select_set(True)
    bpy.ops.object.convert(target='MESH')

    obj = bpy.context.active_object
    obj.name = "Zone1_Apex"
    bpy.ops.object.shade_smooth()

    return obj


def create_zone1_complete():
    """Create the complete Zone 1 (A-legs + apex) as a unified shell."""
    left_leg = create_single_leg_chamber(is_left=True)
    right_leg = create_single_leg_chamber(is_left=False)
    apex = create_apex_bridge()

    # Join all parts into one mesh
    bpy.ops.object.select_all(action='DESELECT')
    left_leg.select_set(True)
    right_leg.select_set(True)
    apex.select_set(True)
    bpy.context.view_layer.objects.active = left_leg
    bpy.ops.object.join()

    zone1 = bpy.context.active_object
    zone1.name = "Zone1_AShape"

    # Apply remesh for cleaner topology
    remesh = zone1.modifiers.new(name="Remesh", type='REMESH')
    remesh.mode = 'SMOOTH'
    remesh.octree_depth = 7
    remesh.use_smooth_shade = True
    bpy.ops.object.modifier_apply(modifier="Remesh")

    bpy.ops.object.shade_smooth()

    # Apply material
    mat = create_simple_material("Zone1_Material", SHELL_COLOR)
    zone1.data.materials.append(mat)

    return zone1


def create_zone2_clitoral_cylinder():
    """Create Zone 2: Clitoral Cylinder at apex - teardrop/oval shape"""
    apex_y = LEG_LENGTH * 0.35
    apex_z = DOME_HEIGHT * 0.68

    mball = bpy.data.metaballs.new("Zone2_Mball")
    mball_obj = bpy.data.objects.new("Zone2_Cylinder", mball)
    bpy.context.collection.objects.link(mball_obj)

    mball.resolution = 0.0015
    mball.threshold = 0.6

    # Main cylinder body (oval/teardrop shape)
    body = mball.elements.new()
    body.type = 'ELLIPSOID'
    body.co = (0, apex_y, apex_z + CYLINDER_HEIGHT * 0.4)
    body.radius = CYLINDER_DIAMETER * 0.5
    body.size_x = 0.9
    body.size_y = 1.3
    body.size_z = 1.5

    # Top dome
    top = mball.elements.new()
    top.type = 'ELLIPSOID'
    top.co = (0, apex_y, apex_z + CYLINDER_HEIGHT * 0.8)
    top.radius = CYLINDER_DIAMETER * 0.4
    top.size_x = 0.85
    top.size_y = 1.0
    top.size_z = 0.7

    # Base flange (connects to crossbar)
    base = mball.elements.new()
    base.type = 'ELLIPSOID'
    base.co = (0, apex_y, apex_z)
    base.radius = CYLINDER_DIAMETER * 0.55
    base.size_x = 1.0
    base.size_y = 1.1
    base.size_z = 0.5

    # Convert to mesh
    bpy.context.view_layer.objects.active = mball_obj
    mball_obj.select_set(True)
    bpy.ops.object.convert(target='MESH')

    cylinder = bpy.context.active_object
    cylinder.name = "Zone2_ClitoralCylinder"
    bpy.ops.object.shade_smooth()

    mat = create_simple_material("Zone2_Material", ZONE2_COLOR)
    cylinder.data.materials.append(mat)

    return cylinder


def create_a_crossbar():
    """Create A-Crossbar: Internal diaphragm separating Zone 1 from Zone 2"""
    apex_y = LEG_LENGTH * 0.35
    crossbar_z = DOME_HEIGHT * 0.62

    mball = bpy.data.metaballs.new("Crossbar_Mball")
    mball_obj = bpy.data.objects.new("A_Crossbar", mball)
    bpy.context.collection.objects.link(mball_obj)

    mball.resolution = 0.002
    mball.threshold = 0.7

    # Left wing of crossbar
    left = mball.elements.new()
    left.type = 'ELLIPSOID'
    left.co = (-TOTAL_WIDTH * 0.2, apex_y - 5*MM, crossbar_z)
    left.radius = 18 * MM
    left.size_x = 1.5
    left.size_y = 0.8
    left.size_z = 0.15

    # Right wing of crossbar
    right = mball.elements.new()
    right.type = 'ELLIPSOID'
    right.co = (TOTAL_WIDTH * 0.2, apex_y - 5*MM, crossbar_z)
    right.radius = 18 * MM
    right.size_x = 1.5
    right.size_y = 0.8
    right.size_z = 0.15

    # Center section around clitoral cylinder
    center = mball.elements.new()
    center.type = 'ELLIPSOID'
    center.co = (0, apex_y, crossbar_z)
    center.radius = CYLINDER_DIAMETER * 0.7
    center.size_x = 1.0
    center.size_y = 0.9
    center.size_z = 0.2

    # Convert to mesh
    bpy.context.view_layer.objects.active = mball_obj
    mball_obj.select_set(True)
    bpy.ops.object.convert(target='MESH')

    crossbar = bpy.context.active_object
    crossbar.name = "A_Crossbar"

    # Cut hole for clitoral cylinder
    bpy.ops.mesh.primitive_cylinder_add(
        radius=CYLINDER_DIAMETER * 0.45,
        depth=15 * MM,
        location=(0, apex_y, crossbar_z),
        vertices=32
    )
    cutter = bpy.context.active_object

    mod = crossbar.modifiers.new(name="CutHole", type='BOOLEAN')
    mod.operation = 'DIFFERENCE'
    mod.object = cutter
    bpy.context.view_layer.objects.active = crossbar
    bpy.ops.object.modifier_apply(modifier="CutHole")
    bpy.data.objects.remove(cutter)

    bpy.ops.object.shade_smooth()

    mat = create_simple_material("Crossbar_Material", CROSSBAR_COLOR)
    crossbar.data.materials.append(mat)

    return crossbar


def create_edge_seals():
    """Create the dark edge lines/seals visible in the reference image"""
    seals = []

    # Left leg bottom edge seal
    bpy.ops.mesh.primitive_torus_add(
        major_radius=LEG_WIDTH * 0.45,
        minor_radius=1.5 * MM,
        location=(-CHANNEL_WIDTH/2 - LEG_WIDTH/2, -LEG_LENGTH * 0.35, DOME_HEIGHT * 0.08),
        major_segments=32,
        minor_segments=8
    )
    left_seal = bpy.context.active_object
    left_seal.name = "Seal_LeftLeg"
    left_seal.scale = (0.9, 1.4, 0.3)
    left_seal.rotation_euler = (0, 0, math.radians(15))
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    seals.append(left_seal)

    # Right leg bottom edge seal
    bpy.ops.mesh.primitive_torus_add(
        major_radius=LEG_WIDTH * 0.45,
        minor_radius=1.5 * MM,
        location=(CHANNEL_WIDTH/2 + LEG_WIDTH/2, -LEG_LENGTH * 0.35, DOME_HEIGHT * 0.08),
        major_segments=32,
        minor_segments=8
    )
    right_seal = bpy.context.active_object
    right_seal.name = "Seal_RightLeg"
    right_seal.scale = (0.9, 1.4, 0.3)
    right_seal.rotation_euler = (0, 0, math.radians(-15))
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    seals.append(right_seal)

    mat = create_simple_material("Seal_Material", SEAL_COLOR)
    for seal in seals:
        seal.data.materials.append(mat)
        bpy.context.view_layer.objects.active = seal
        bpy.ops.object.shade_smooth()

    return seals


def create_vacuum_ports():
    """Create vacuum port nipples for tube connections"""
    apex_y = LEG_LENGTH * 0.35
    ports = []

    # Zone 1 ports (one on each leg, at top)
    for sign, name in [(-1, "Port_Zone1_Left"), (1, "Port_Zone1_Right")]:
        x = sign * (CHANNEL_WIDTH/2 + LEG_WIDTH/2)
        bpy.ops.mesh.primitive_cylinder_add(
            radius=3 * MM,
            depth=12 * MM,
            location=(x, LEG_LENGTH * 0.1, DOME_HEIGHT * 0.75),
            vertices=16
        )
        port = bpy.context.active_object
        port.name = name
        ports.append(port)

    # Zone 2 port (on top of clitoral cylinder)
    bpy.ops.mesh.primitive_cylinder_add(
        radius=2.5 * MM,
        depth=10 * MM,
        location=(0, apex_y, DOME_HEIGHT * 0.68 + CYLINDER_HEIGHT + 5 * MM),
        vertices=16
    )
    port2 = bpy.context.active_object
    port2.name = "Port_Zone2"
    ports.append(port2)

    mat = create_simple_material("Port_Material", (0.4, 0.4, 0.45, 1.0))
    for p in ports:
        p.data.materials.append(mat)

    return ports


def setup_camera_and_lighting():
    """Set up scene to match reference image perspective"""
    # Camera - front view slightly elevated
    bpy.ops.object.camera_add(location=(0, -0.35, 0.12))
    camera = bpy.context.active_object
    camera.rotation_euler = (math.radians(72), 0, 0)
    bpy.context.scene.camera = camera
    camera.data.lens = 85

    # Key light from above-front
    bpy.ops.object.light_add(type='AREA', location=(0, -0.2, 0.4))
    key = bpy.context.active_object
    key.data.energy = 200
    key.data.size = 0.5
    key.rotation_euler = (math.radians(45), 0, 0)

    # Fill light from side
    bpy.ops.object.light_add(type='AREA', location=(-0.3, 0, 0.2))
    fill = bpy.context.active_object
    fill.data.energy = 80
    fill.data.size = 0.4

    # Rim light from behind
    bpy.ops.object.light_add(type='AREA', location=(0, 0.3, 0.15))
    rim = bpy.context.active_object
    rim.data.energy = 100
    rim.data.size = 0.3
    rim.rotation_euler = (math.radians(-30), 0, 0)

    bpy.context.scene.render.engine = 'BLENDER_EEVEE_NEXT'

    # Light gray background
    bpy.context.scene.world.use_nodes = True
    bg = bpy.context.scene.world.node_tree.nodes['Background']
    bg.inputs[0].default_value = (0.85, 0.85, 0.87, 1)
    bg.inputs[1].default_value = 1.0


# =============================================================================
# MAIN ASSEMBLY
# =============================================================================
def create_a_contour_cup():
    """Main function to create the complete A-Contour cup assembly"""
    print("=" * 60)
    print("Creating A-Contour Dual-Chamber Vacuum Cup")
    print("=" * 60)

    clear_scene()

    print("Creating Zone 1 (A-Legs + Apex)...")
    zone1 = create_zone1_complete()

    print("Creating Zone 2 (Clitoral Cylinder)...")
    clitoral_cyl = create_zone2_clitoral_cylinder()

    print("Creating A-Crossbar (Diaphragm)...")
    crossbar = create_a_crossbar()

    print("Creating Edge Seals...")
    seals = create_edge_seals()

    print("Creating Vacuum Ports...")
    ports = create_vacuum_ports()

    print("Setting up Camera and Lighting...")
    setup_camera_and_lighting()

    # Parent all to empty for easy manipulation
    bpy.ops.object.empty_add(type='PLAIN_AXES', location=(0, 0, 0))
    parent = bpy.context.active_object
    parent.name = "A_Contour_Cup_Assembly"

    all_objects = [zone1, clitoral_cyl, crossbar] + seals + ports
    for obj in all_objects:
        if obj:
            obj.parent = parent

    print("=" * 60)
    print("A-Contour Cup Created Successfully!")
    print(f"  Total Width:  {TOTAL_WIDTH/MM:.0f}mm")
    print(f"  Total Length: {TOTAL_LENGTH/MM:.0f}mm")
    print(f"  Dome Height:  {DOME_HEIGHT/MM:.0f}mm")
    print(f"  Zone 2 Diameter: {CYLINDER_DIAMETER/MM:.0f}mm")
    print(f"  Open Channel: {CHANNEL_WIDTH/MM:.0f}mm")
    print("=" * 60)

    return parent


if __name__ == "__main__":
    create_a_contour_cup()
