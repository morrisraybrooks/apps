"""
A-Contour Vacuum Cup - Blender Python Script
=============================================
Generates the anatomical dual-chamber vacuum cup matching the chrome/glass reference.

Design Features:
- Two large bulbous pill-shaped leg chambers (A-legs) - completely separate
- Teardrop Zone 2 with loop handle at apex
- Dark edge seals around chamber bottoms
- TRUE OPEN CHANNEL design - NO crossbar/bridge between legs
- Chrome/glass highly reflective appearance
- Full open channel between the two legs for drainage and access

Dimensions:
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

# Overall dimensions
TOTAL_WIDTH = 125 * MM       # ~125mm total width
TOTAL_LENGTH = 110 * MM      # Length from apex to bottom of legs
DOME_HEIGHT = 70 * MM        # Cup depth/height


# A-Shape profile dimensions
CHANNEL_WIDTH = 20 * MM      # Open channel width between legs
LEG_WIDTH = 48 * MM          # Width of each leg chamber
LEG_LENGTH = 85 * MM         # Length of each leg

# Zone 2: Clitoral Cylinder (A-Apex) - 1" x 2"
CYLINDER_DIAMETER = 25.4 * MM  # Width: 1 inch = 25.4mm
CYLINDER_HEIGHT = 50.8 * MM    # Height: 2 inches = 50.8mm

# Colors - chrome/glass appearance matching reference
CHROME_COLOR = (0.85, 0.88, 0.92, 0.92)     # Bright chrome
ZONE2_COLOR = (0.80, 0.83, 0.88, 0.95)      # Slightly darker chrome
DARK_CHROME = (0.15, 0.18, 0.22, 1.0)       # Dark chrome for seals/lines
CROSSBAR_COLOR = (0.20, 0.22, 0.28, 1.0)    # Dark crossbar


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
    Create one A-leg chamber as a large bulbous pill-shaped dome.
    Matching the chrome reference image - smooth, rounded, converging at top.
    """
    sign = -1 if is_left else 1
    name = "LeftLeg" if is_left else "RightLeg"

    mball = bpy.data.metaballs.new(f"{name}_Mball")
    mball_obj = bpy.data.objects.new(name, mball)
    bpy.context.collection.objects.link(mball_obj)

    mball.resolution = 0.003  # Balanced resolution (lower = more detail but slower)
    mball.threshold = 0.55    # Consistent threshold across all metaballs

    # Base X offset - legs converge toward top
    x_base = sign * (CHANNEL_WIDTH / 2 + LEG_WIDTH / 2)

    # Main body - large rounded pill shape
    main = mball.elements.new()
    main.type = 'ELLIPSOID'
    main.co = (x_base * 0.95, -LEG_LENGTH * 0.05, DOME_HEIGHT * 0.5)
    main.radius = LEG_WIDTH * 0.55
    main.size_x = 0.95
    main.size_y = 1.6  # Elongated pill shape
    main.size_z = 1.3

    # Upper bulge - rounder at top, angles inward
    upper = mball.elements.new()
    upper.type = 'ELLIPSOID'
    upper.co = (x_base * 0.6, LEG_LENGTH * 0.28, DOME_HEIGHT * 0.6)
    upper.radius = LEG_WIDTH * 0.48
    upper.size_x = 0.85
    upper.size_y = 1.0
    upper.size_z = 1.15

    # Top connector - angles strongly inward to apex
    top_conn = mball.elements.new()
    top_conn.type = 'ELLIPSOID'
    top_conn.co = (x_base * 0.35, LEG_LENGTH * 0.42, DOME_HEIGHT * 0.65)
    top_conn.radius = LEG_WIDTH * 0.38
    top_conn.size_x = 0.7
    top_conn.size_y = 0.7
    top_conn.size_z = 0.95

    # Lower bulge - rounded bottom
    lower = mball.elements.new()
    lower.type = 'ELLIPSOID'
    lower.co = (x_base * 1.05, -LEG_LENGTH * 0.25, DOME_HEIGHT * 0.4)
    lower.radius = LEG_WIDTH * 0.5
    lower.size_x = 0.9
    lower.size_y = 1.1
    lower.size_z = 1.0

    # Bottom rounded tip
    tip = mball.elements.new()
    tip.type = 'ELLIPSOID'
    tip.co = (x_base * 1.1, -LEG_LENGTH * 0.42, DOME_HEIGHT * 0.32)
    tip.radius = LEG_WIDTH * 0.42
    tip.size_x = 0.85
    tip.size_y = 0.85
    tip.size_z = 0.75

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

    mball.resolution = 0.003  # Balanced resolution (matches leg chambers)
    mball.threshold = 0.55    # Consistent threshold for proper blending

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
    """Create the complete Zone 1 (A-legs + apex) as a HOLLOW shell with open bottoms.

    This is a vacuum cup - the bottom of each leg chamber must be open for suction.
    We use the Solidify modifier to create a hollow shell, then cut openings.
    """
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

    # Clean up mesh before remesh - merge overlapping vertices from joined meshes
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.remove_doubles(threshold=0.0005)  # Merge close vertices at seams
    bpy.ops.mesh.dissolve_degenerate(threshold=0.0001)
    bpy.ops.mesh.delete_loose()
    bpy.ops.object.mode_set(mode='OBJECT')

    # Apply remesh for cleaner topology - use VOXEL mode for better watertight results
    remesh = zone1.modifiers.new(name="Remesh", type='REMESH')
    remesh.mode = 'VOXEL'
    remesh.voxel_size = 0.002  # Balanced voxel size (smaller = more detail but slower)
    remesh.use_smooth_shade = True
    bpy.ops.object.modifier_apply(modifier="Remesh")

    # Additional smoothing pass to eliminate any remaining artifacts
    smooth = zone1.modifiers.new(name="Smooth", type='SMOOTH')
    smooth.factor = 0.5
    smooth.iterations = 2
    bpy.ops.object.modifier_apply(modifier="Smooth")

    # =========================================================================
    # MAKE HOLLOW SHELL - Apply Solidify modifier to create wall thickness
    # =========================================================================
    solidify = zone1.modifiers.new(name="Solidify", type='SOLIDIFY')
    solidify.thickness = 2.5 * MM  # Wall thickness ~2.5mm
    solidify.offset = -1.0  # Offset inward (shell on inside)
    solidify.use_even_offset = True
    solidify.use_quality_normals = True
    bpy.ops.object.modifier_apply(modifier="Solidify")

    # =========================================================================
    # CUT OPEN BOTTOMS - Create openings at bottom of each leg for vacuum suction
    # =========================================================================
    # Left leg opening
    left_x = -(CHANNEL_WIDTH/2 + LEG_WIDTH/2)
    bpy.ops.mesh.primitive_cylinder_add(
        radius=LEG_WIDTH * 0.40,  # Opening size
        depth=DOME_HEIGHT * 0.5,  # Tall enough to cut through
        location=(left_x, -LEG_LENGTH * 0.30, DOME_HEIGHT * 0.15),
        vertices=48
    )
    left_cutter = bpy.context.active_object
    left_cutter.name = "LeftLeg_Cutter"
    # Scale to oval shape matching leg profile
    left_cutter.scale = (0.85, 1.3, 1.0)
    left_cutter.rotation_euler = (0, 0, math.radians(15))
    bpy.ops.object.transform_apply(scale=True, rotation=True)

    # Right leg opening
    right_x = (CHANNEL_WIDTH/2 + LEG_WIDTH/2)
    bpy.ops.mesh.primitive_cylinder_add(
        radius=LEG_WIDTH * 0.40,
        depth=DOME_HEIGHT * 0.5,
        location=(right_x, -LEG_LENGTH * 0.30, DOME_HEIGHT * 0.15),
        vertices=48
    )
    right_cutter = bpy.context.active_object
    right_cutter.name = "RightLeg_Cutter"
    right_cutter.scale = (0.85, 1.3, 1.0)
    right_cutter.rotation_euler = (0, 0, math.radians(-15))
    bpy.ops.object.transform_apply(scale=True, rotation=True)

    # Apply boolean difference to cut openings
    bpy.context.view_layer.objects.active = zone1
    zone1.select_set(True)

    mod_left = zone1.modifiers.new(name="CutLeft", type='BOOLEAN')
    mod_left.operation = 'DIFFERENCE'
    mod_left.solver = 'EXACT'
    mod_left.object = left_cutter
    bpy.ops.object.modifier_apply(modifier="CutLeft")
    bpy.data.objects.remove(left_cutter)

    mod_right = zone1.modifiers.new(name="CutRight", type='BOOLEAN')
    mod_right.operation = 'DIFFERENCE'
    mod_right.solver = 'EXACT'
    mod_right.object = right_cutter
    bpy.ops.object.modifier_apply(modifier="CutRight")
    bpy.data.objects.remove(right_cutter)

    # Clean up after boolean cuts
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.remove_doubles(threshold=0.0001)
    bpy.ops.mesh.dissolve_degenerate(threshold=0.0001)
    bpy.ops.mesh.delete_loose()
    bpy.ops.object.mode_set(mode='OBJECT')

    bpy.ops.object.shade_smooth()

    # Apply chrome material
    mat = create_simple_material("Zone1_Material", CHROME_COLOR)
    zone1.data.materials.append(mat)

    return zone1


def create_zone2_clitoral_cylinder():
    """
    Create Zone 2: Teardrop-shaped cylinder with integrated loop handle at top.
    All components created as a single unified metaball object for seamless geometry.
    Matching the reference image's distinctive apex design.
    """
    apex_y = LEG_LENGTH * 0.42
    apex_z = DOME_HEIGHT * 0.72
    loop_z = apex_z + CYLINDER_HEIGHT * 0.85 + CYLINDER_DIAMETER * 0.15

    mball = bpy.data.metaballs.new("Zone2_Mball")
    mball_obj = bpy.data.objects.new("Zone2_Cylinder", mball)
    bpy.context.collection.objects.link(mball_obj)

    mball.resolution = 0.003  # Balanced resolution across all metaballs
    mball.threshold = 0.55    # Consistent threshold for proper blending

    # Teardrop body - wider at bottom, tapering up
    body = mball.elements.new()
    body.type = 'ELLIPSOID'
    body.co = (0, apex_y, apex_z + CYLINDER_HEIGHT * 0.25)
    body.radius = CYLINDER_DIAMETER * 0.5
    body.size_x = 1.0
    body.size_y = 1.2
    body.size_z = 1.8

    # Upper taper
    upper = mball.elements.new()
    upper.type = 'ELLIPSOID'
    upper.co = (0, apex_y, apex_z + CYLINDER_HEIGHT * 0.55)
    upper.radius = CYLINDER_DIAMETER * 0.38
    upper.size_x = 0.8
    upper.size_y = 0.9
    upper.size_z = 1.2

    # Neck (narrowing before loop)
    neck = mball.elements.new()
    neck.type = 'ELLIPSOID'
    neck.co = (0, apex_y, apex_z + CYLINDER_HEIGHT * 0.75)
    neck.radius = CYLINDER_DIAMETER * 0.25
    neck.size_x = 0.6
    neck.size_y = 0.7
    neck.size_z = 0.8

    # Base flange
    base = mball.elements.new()
    base.type = 'ELLIPSOID'
    base.co = (0, apex_y, apex_z)
    base.radius = CYLINDER_DIAMETER * 0.55
    base.size_x = 1.1
    base.size_y = 1.2
    base.size_z = 0.4

    # =========================================================================
    # Loop handle - integrated as metaball elements (replaces separate torus)
    # Creates a torus-like ring using multiple ellipsoids arranged in a circle
    # =========================================================================
    loop_major_radius = CYLINDER_DIAMETER * 0.22  # Outer radius of the loop
    loop_minor_radius = CYLINDER_DIAMETER * 0.06  # Thickness of the loop tube
    num_loop_elements = 12  # Number of metaballs to form the loop ring

    for i in range(num_loop_elements):
        angle = (2 * math.pi * i) / num_loop_elements
        # Loop is oriented vertically (standing up), so we use Y and Z for the circle
        elem_y = apex_y + loop_major_radius * math.cos(angle)
        elem_z = loop_z + loop_major_radius * math.sin(angle)

        loop_elem = mball.elements.new()
        loop_elem.type = 'BALL'
        loop_elem.co = (0, elem_y, elem_z)
        loop_elem.radius = loop_minor_radius * 1.8  # Slightly larger for smooth blending

    # Add connection elements to smoothly blend loop with neck
    # Bottom of loop connecting to top of cylinder
    loop_base = mball.elements.new()
    loop_base.type = 'ELLIPSOID'
    loop_base.co = (0, apex_y, apex_z + CYLINDER_HEIGHT * 0.82)
    loop_base.radius = CYLINDER_DIAMETER * 0.18
    loop_base.size_x = 0.5
    loop_base.size_y = 0.8
    loop_base.size_z = 0.6

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

    mball.resolution = 0.003  # Balanced resolution for crossbar
    mball.threshold = 0.55    # Consistent threshold to avoid surface discontinuities

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

    # Apply remesh to crossbar for cleaner topology before boolean
    remesh = crossbar.modifiers.new(name="Remesh", type='REMESH')
    remesh.mode = 'SMOOTH'
    remesh.octree_depth = 6
    remesh.use_smooth_shade = True
    bpy.ops.object.modifier_apply(modifier="Remesh")

    # Cut hole for clitoral cylinder - exact fit
    bpy.ops.mesh.primitive_cylinder_add(
        radius=CYLINDER_DIAMETER * 0.52,  # Slightly larger than cylinder for clean fit
        depth=20 * MM,  # Sufficient depth to cut through crossbar
        location=(0, apex_y, crossbar_z),
        vertices=64  # Higher vertex count for smoother boolean result
    )
    cutter = bpy.context.active_object
    cutter.name = "Crossbar_Cutter"

    # Use EXACT solver for cleaner boolean results
    mod = crossbar.modifiers.new(name="CutHole", type='BOOLEAN')
    mod.operation = 'DIFFERENCE'
    mod.object = cutter
    mod.solver = 'EXACT'  # EXACT solver produces cleaner results than FAST
    bpy.context.view_layer.objects.active = crossbar
    crossbar.select_set(True)
    bpy.ops.object.modifier_apply(modifier="CutHole")
    bpy.data.objects.remove(cutter)

    # Clean up mesh after boolean operation
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.remove_doubles(threshold=0.0001)  # Merge close vertices
    bpy.ops.mesh.dissolve_degenerate(threshold=0.0001)  # Remove zero-area faces
    bpy.ops.mesh.delete_loose()  # Remove loose geometry
    bpy.ops.object.mode_set(mode='OBJECT')

    bpy.ops.object.shade_smooth()

    mat = create_simple_material("Crossbar_Material", CROSSBAR_COLOR)
    crossbar.data.materials.append(mat)

    return crossbar


def create_edge_seals():
    """Create the edge seals/gaskets around the vacuum cup openings.

    These seals are positioned to OVERLAP with the leg geometry at the rim
    of each opening, so boolean union will smoothly merge them.
    """
    seals = []

    # Seals positioned to INTERSECT with the leg rim at the opening edge
    # The opening cut happens at z = DOME_HEIGHT * 0.15, extending down
    # Position seals higher to overlap with the remaining leg geometry
    seal_z = DOME_HEIGHT * 0.18  # At the rim edge where opening meets leg wall
    seal_minor_radius = 4 * MM   # Larger to ensure overlap with leg wall

    # Left leg rim seal - positioned to intersect with leg wall at opening
    left_x = -(CHANNEL_WIDTH/2 + LEG_WIDTH/2)
    bpy.ops.mesh.primitive_torus_add(
        major_radius=LEG_WIDTH * 0.40,  # Match opening radius
        minor_radius=seal_minor_radius,
        location=(left_x, -LEG_LENGTH * 0.30, seal_z),
        major_segments=48,
        minor_segments=16
    )
    left_seal = bpy.context.active_object
    left_seal.name = "Seal_LeftLeg"
    left_seal.scale = (0.85, 1.3, 0.6)  # Match oval opening shape, flatten slightly
    left_seal.rotation_euler = (0, 0, math.radians(15))
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    seals.append(left_seal)

    # Right leg rim seal
    right_x = (CHANNEL_WIDTH/2 + LEG_WIDTH/2)
    bpy.ops.mesh.primitive_torus_add(
        major_radius=LEG_WIDTH * 0.40,
        minor_radius=seal_minor_radius,
        location=(right_x, -LEG_LENGTH * 0.30, seal_z),
        major_segments=48,
        minor_segments=16
    )
    right_seal = bpy.context.active_object
    right_seal.name = "Seal_RightLeg"
    right_seal.scale = (0.85, 1.3, 0.6)
    right_seal.rotation_euler = (0, 0, math.radians(-15))
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    seals.append(right_seal)

    mat = create_simple_material("Seal_Material", DARK_CHROME)
    for seal in seals:
        seal.data.materials.append(mat)
        bpy.context.view_layer.objects.active = seal
        bpy.ops.object.shade_smooth()

    return seals


def create_vacuum_ports():
    """Create vacuum port nipples for tube connections.

    Ports are positioned to PENETRATE into the main body surface so boolean
    union operations can properly merge them. The depth is extended and
    position lowered to ensure solid overlap with the cup geometry.
    """
    apex_y = LEG_LENGTH * 0.35
    ports = []

    # Port depth extended to penetrate into the body
    port_depth = 20 * MM  # Longer to ensure intersection
    port_radius = 3.5 * MM

    # Zone 1 ports (one on each leg, at top) - positioned to intersect with leg surface
    for sign, name in [(-1, "Port_Zone1_Left"), (1, "Port_Zone1_Right")]:
        x = sign * (CHANNEL_WIDTH/2 + LEG_WIDTH/2) * 0.95  # Slightly inward
        # Position lower so the port base is inside the leg geometry
        z_pos = DOME_HEIGHT * 0.60  # Lower position to ensure intersection
        bpy.ops.mesh.primitive_cylinder_add(
            radius=port_radius,
            depth=port_depth,
            location=(x, LEG_LENGTH * 0.08, z_pos),
            vertices=24
        )
        port = bpy.context.active_object
        port.name = name
        ports.append(port)

    # Zone 2 port (on top of clitoral cylinder) - positioned to penetrate into cylinder top
    loop_top_z = DOME_HEIGHT * 0.72 + CYLINDER_HEIGHT * 0.85 + CYLINDER_DIAMETER * 0.37
    bpy.ops.mesh.primitive_cylinder_add(
        radius=2.5 * MM,
        depth=15 * MM,  # Extended depth
        location=(0, apex_y, loop_top_z),  # Positioned to intersect with loop top
        vertices=24
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

    # REMOVED: A-Crossbar (Diaphragm) - creating true open channel design
    # print("Creating A-Crossbar (Diaphragm)...")
    # crossbar = create_a_crossbar()
    crossbar = None  # No connecting bridge between legs

    print("Creating Edge Seals...")
    seals = create_edge_seals()

    print("Creating Vacuum Ports...")
    ports = create_vacuum_ports()

    # =========================================================================
    # MERGE MAIN CUP COMPONENTS INTO UNIFIED GEOMETRY
    # (Seals remain separate as they are gasket/rim elements)
    # =========================================================================
    print("Merging cup components into unified geometry...")

    # Use boolean union to merge Zone 2 (clitoral cylinder) into Zone 1
    bpy.ops.object.select_all(action='DESELECT')
    zone1.select_set(True)
    bpy.context.view_layer.objects.active = zone1

    mod_z2 = zone1.modifiers.new(name="Union_Zone2", type='BOOLEAN')
    mod_z2.operation = 'UNION'
    mod_z2.solver = 'EXACT'
    mod_z2.object = clitoral_cyl
    bpy.ops.object.modifier_apply(modifier="Union_Zone2")
    bpy.data.objects.remove(clitoral_cyl)

    # REMOVED: Crossbar union - true open channel design with no bridge
    # if crossbar:
    #     mod_cb = zone1.modifiers.new(name="Union_Crossbar", type='BOOLEAN')
    #     mod_cb.operation = 'UNION'
    #     mod_cb.solver = 'EXACT'
    #     mod_cb.object = crossbar
    #     bpy.ops.object.modifier_apply(modifier="Union_Crossbar")
    #     bpy.data.objects.remove(crossbar)

    # Merge vacuum ports into main body
    for port in ports:
        mod_port = zone1.modifiers.new(name=f"Union_{port.name}", type='BOOLEAN')
        mod_port.operation = 'UNION'
        mod_port.solver = 'EXACT'
        mod_port.object = port
        bpy.ops.object.modifier_apply(modifier=f"Union_{port.name}")
        bpy.data.objects.remove(port)

    # Merge edge seals into main body for smooth attachment at openings
    for seal in seals:
        mod_seal = zone1.modifiers.new(name=f"Union_{seal.name}", type='BOOLEAN')
        mod_seal.operation = 'UNION'
        mod_seal.solver = 'EXACT'
        mod_seal.object = seal
        bpy.ops.object.modifier_apply(modifier=f"Union_{seal.name}")
        bpy.data.objects.remove(seal)

    # Clean up the merged mesh
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.remove_doubles(threshold=0.0001)
    bpy.ops.mesh.dissolve_degenerate(threshold=0.0001)
    bpy.ops.mesh.delete_loose()
    bpy.ops.object.mode_set(mode='OBJECT')

    # Rename the unified object
    zone1.name = "A_Contour_Cup_Unified"
    bpy.ops.object.shade_smooth()

    print("Setting up Camera and Lighting...")
    setup_camera_and_lighting()

    # Parent to empty for easy manipulation
    bpy.ops.object.empty_add(type='PLAIN_AXES', location=(0, 0, 0))
    parent = bpy.context.active_object
    parent.name = "A_Contour_Cup_Assembly"
    zone1.parent = parent

    print("=" * 60)
    print("A-Contour Cup Created Successfully!")
    print(f"  Total Width:  {TOTAL_WIDTH/MM:.0f}mm")
    print(f"  Total Length: {TOTAL_LENGTH/MM:.0f}mm")
    print(f"  Dome Height:  {DOME_HEIGHT/MM:.0f}mm")
    print(f"  Zone 2 Diameter: {CYLINDER_DIAMETER/MM:.0f}mm")
    print(f"  Open Channel: {CHANNEL_WIDTH/MM:.0f}mm")
    print("=" * 60)

    return parent


def render_and_save(output_dir=None):
    """Render the scene and save blend file"""
    import os

    if output_dir is None:
        output_dir = os.path.dirname(os.path.abspath(__file__))

    # Save blend file
    blend_path = os.path.join(output_dir, "a_contour_cup.blend")
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)
    print(f"Saved: {blend_path}")

    # Render settings for nice output
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 128
    scene.render.resolution_x = 1080
    scene.render.resolution_y = 1440
    scene.render.film_transparent = True

    # Render
    render_path = os.path.join(output_dir, "a_contour_cup_render.png")
    scene.render.filepath = render_path
    bpy.ops.render.render(write_still=True)
    print(f"Rendered: {render_path}")


if __name__ == "__main__":
    cup = create_a_contour_cup()

    # Export blend file
    import os

    # Handle both command-line and Text Editor execution
    output_dir = None

    try:
        # When run from command line: blender --python script.py
        script_path = os.path.abspath(__file__)
        # Verify it's a real file path, not a Blender internal path like "/Text"
        if os.path.isfile(script_path):
            output_dir = os.path.dirname(script_path)
    except NameError:
        # When run from Blender's Text Editor, __file__ is not defined
        pass

    # Fallback to current blend file's directory or home directory
    if not output_dir:
        blend_filepath = bpy.data.filepath
        if blend_filepath and os.path.isfile(blend_filepath):
            output_dir = os.path.dirname(blend_filepath)
        else:
            output_dir = os.path.expanduser("~")

    # Ensure output_dir is a valid directory
    if not os.path.isdir(output_dir):
        output_dir = os.path.expanduser("~")

    blend_path = os.path.join(output_dir, "a_contour_cup.blend")
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)
    print(f"\nSaved blend file: {blend_path}")