// =============================================================================
// A-CONTOUR VACUUM CUP - Parametric Model
// =============================================================================
// Revolutionary dual-chamber design with anatomical "A" shape geometry
// Features: A-Apex (clitoral cylinder), A-Crossbar (diaphragm), A-Legs (bilateral chambers)
// Open-channel design for fluid drainage and vaginal/urethral access
//
// Materials: Shell PC/Tritan (transparent); Seal lip silicone (Shore A 10–15)
// Pressure rating: −40 kPa continuous, −60 kPa transient
// Compliance: ISO 80369-7 Luer connections, ISO 10993 biocompatible
// =============================================================================

// =============================================================================
// CORE DIMENSIONS - A-CONTOUR GEOMETRY
// =============================================================================
// Overall footprint matches vulva anatomy
total_width_mm = 80;           // Total width across both A-legs
total_length_mm = 100;         // Total length from apex to base of legs
wall_thickness_mm = 2.0;       // Shell wall thickness
dome_height_mm = 35;           // Height of the domed shell above base

// A-APEX (Zone 2: Clitoral Cylinder)
apex_cylinder_diameter_mm = 15;    // Clitoral cylinder inner diameter
apex_cylinder_height_mm = 25;      // Height of clitoral chamber
apex_position_y_mm = 35;           // Y position from center (toward top)

// A-LEGS (Zone 1: Bilateral Outer Chambers)
leg_width_mm = 25;                 // Width of each A-leg
leg_length_mm = 60;                // Length of each leg extending down
leg_spacing_mm = 30;               // Gap between legs (open channel width)
leg_curve_radius_mm = 15;          // Radius of leg curvature for comfort

// A-CROSSBAR (Internal Diaphragm)
crossbar_thickness_mm = 1.0;       // Thickness of chamber separator
crossbar_height_mm = 12;           // Height where crossbar separates chambers

// OPEN CHANNEL (Drainage/Access)
channel_width_mm = 30;             // Width of open channel between legs
channel_depth_mm = 15;             // How deep the channel cuts into base

// Base sealing
base_rim_width_mm = 4;             // Width of base rim for sealing
seal_lip_width_mm = 3;             // Outer seal lip overhang
seal_lip_thickness_mm = 1.5;       // Thickness of silicone seal lip

// =============================================================================
// PORT CONFIGURATION - Dual Independent Vacuum
// =============================================================================
connector_type = "luer_lock_female";
port_height_mm = 8;                // Height of port extensions
port_diameter_mm = 6;              // External diameter of ports

// Port positions on A-shape
outer_port_position = [25, 20, dome_height_mm];   // Left A-leg port (Zone 1)
inner_port_position = [0, apex_position_y_mm, apex_cylinder_height_mm + 5]; // A-apex port (Zone 2)

// Luer-lock dimensions (ISO 80369-7)
luer_taper_deg = 3.4375;           // half-angle for 6% taper
luer_socket_len_mm = 7.0;
luer_socket_min_id_mm = 3.9;
luer_socket_max_id_mm = luer_socket_min_id_mm + 2*luer_socket_len_mm*tan(luer_taper_deg);

// AVL sensor pickoff (anti-detachment monitoring on outer chamber)
avl_pickoff_id_mm = 2.0;
avl_pickoff_position = [30, 10, dome_height_mm * 0.6];

// =============================================================================
// CAMERA PLACEMENT OPTIONS
// =============================================================================
// Option A: "THROUGH_DOME" - Camera views through transparent PC/Tritan dome
//           Pro: No vacuum seal compromise, cleaner design
//           Con: Some optical distortion, may need anti-fog coating
// Option B: "WINDOW_VOID" - Dedicated optical window with flat glass insert
//           Pro: Better optical clarity, can use IR-transparent material
//           Con: Additional seal point, more complex manufacturing
// Option C: "TOP_MOUNT" - Camera mounted on top looking down through dome apex
//           Pro: Centered view, protected position
//           Con: May block port access, visible externally
camera_placement = "THROUGH_DOME";  // Change to test different options

// Camera window parameters (for WINDOW_VOID option)
camera_window_enabled = (camera_placement == "WINDOW_VOID");
camera_window_diameter_mm = 12;      // Optical window diameter
camera_window_position_angle = 180;  // Position on dome (degrees from front)
camera_window_height_ratio = 0.7;    // Height on dome (0=base, 1=top)
camera_window_glass_thickness_mm = 1.5; // Borosilicate or sapphire insert

// Camera mount boss (for external camera mounting)
camera_mount_enabled = (camera_placement == "TOP_MOUNT");
camera_mount_diameter_mm = 8;        // M8 thread or bayonet mount
camera_mount_height_mm = 6;          // Boss height above dome

// =============================================================================
// STRAP ATTACHMENT SYSTEM
// =============================================================================
// Temporary retention until anti-detachment vacuum system is active
// Straps connect to harness/belt to prevent cup falling during setup
strap_attachments_enabled = true;
strap_lug_count = 4;                 // Number of attachment points
strap_lug_width_mm = 8;              // Width of each lug
strap_lug_height_mm = 6;             // Height extending from base
strap_lug_thickness_mm = 3;          // Thickness of lug
strap_slot_width_mm = 4;             // Slot for strap to pass through
strap_slot_height_mm = 3;            // Height of strap slot

// Strap lug positions for A-shape (on outer edges of legs and apex)
strap_positions = [
  [-total_width_mm/2 - 5, -leg_length_mm/2, 0],   // Left leg bottom
  [total_width_mm/2 + 5, -leg_length_mm/2, 0],    // Right leg bottom
  [-total_width_mm/2 - 5, 10, 0],                  // Left leg top
  [total_width_mm/2 + 5, 10, 0]                    // Right leg top
];

// =============================================================================
// VISUALIZATION OPTIONS
// =============================================================================
show_cross_section = false;    // Cut view to see internal chambers
show_dimensions = false;       // Show dimension helper lines
show_chamber_colors = true;    // Color-code Zone 1 vs Zone 2
show_camera_options = true;    // Show camera placement features
show_strap_lugs = true;        // Show strap attachment points
show_anatomy_labels = false;   // Show A-Apex, A-Crossbar, A-Legs labels

// =============================================================================
// A-CONTOUR CUP ASSEMBLY - Main Entry Point
// =============================================================================
module a_contour_cup_assembly() {

  // === ZONE 1: A-LEGS (Bilateral Outer Chambers) ===
  color([0.6, 0.8, 1.0, 0.25]) // Light blue, transparent
  difference() {
    a_legs_shell();
    a_legs_cavity();
    open_channel_cutout();
    // Port socket for outer chamber (SOL1/SOL2)
    translate(outer_port_position) outer_chamber_port();
    // AVL sensor tap
    translate(avl_pickoff_position)
      rotate([0, 90, 0])
        cylinder(h = wall_thickness_mm + 2, d = avl_pickoff_id_mm);
    // Camera window (if enabled)
    if (camera_window_enabled) camera_window_void();
  }

  // === ZONE 2: A-APEX (Clitoral Cylinder) ===
  color([1.0, 0.7, 0.7, 0.3]) // Light red/pink, transparent
  difference() {
    a_apex_shell();
    a_apex_cavity();
    // Port socket for clitoral chamber (SOL4/SOL5)
    translate(inner_port_position) inner_chamber_port();
  }

  // === A-CROSSBAR (Internal Diaphragm) ===
  color([0.9, 0.9, 0.95, 0.5]) // Light gray, semi-transparent
  a_crossbar_diaphragm();

  // === PORTS ===
  // Outer chamber port (Zone 1)
  color([0.7, 0.85, 0.7, 0.7])
  translate(outer_port_position) port_extension("OUTER");

  // Clitoral chamber port (Zone 2)
  color([0.85, 0.7, 0.7, 0.7])
  translate(inner_port_position) port_extension("INNER");

  // === SEAL LIP ===
  color([0.85, 0.85, 0.9, 0.8]) // Silicone appearance
  a_contour_seal_lip();

  // === OPTIONAL FEATURES ===
  if (show_camera_options && camera_window_enabled) camera_window_flange();
  if (show_camera_options && camera_mount_enabled) camera_mount_boss();
  if (show_strap_lugs && strap_attachments_enabled) a_contour_strap_lugs();
}

// =============================================================================
// A-CONTOUR GEOMETRY MODULES
// =============================================================================

// Helper: 2D A-shape profile for extrusion
module a_shape_2d() {
  // Create the characteristic "A" shape:
  // - Two bilateral legs at the bottom
  // - Connected at the apex (top)
  // - Open channel between legs
  hull() {
    // Left leg base
    translate([-total_width_mm/2 + leg_width_mm/2, -leg_length_mm/2])
      circle(d = leg_width_mm);
    // Right leg base
    translate([total_width_mm/2 - leg_width_mm/2, -leg_length_mm/2])
      circle(d = leg_width_mm);
    // Apex region (where legs meet)
    translate([0, apex_position_y_mm])
      circle(d = leg_width_mm * 1.5);
  }
  // Subtract the open channel between legs
}

// A-LEGS SHELL: Bilateral outer chambers (Zone 1)
module a_legs_shell() {
  difference() {
    // Outer shell - A-shape extruded with dome top
    hull() {
      // Left leg - elongated dome
      translate([-total_width_mm/4, 0, 0])
        scale([leg_width_mm/40, leg_length_mm/60, 1])
          cylinder(h = dome_height_mm, d1 = 40, d2 = 30);
      // Right leg - elongated dome
      translate([total_width_mm/4, 0, 0])
        scale([leg_width_mm/40, leg_length_mm/60, 1])
          cylinder(h = dome_height_mm, d1 = 40, d2 = 30);
      // Connect at apex
      translate([0, apex_position_y_mm - 10, 0])
        cylinder(h = dome_height_mm, d1 = leg_width_mm * 1.2, d2 = leg_width_mm);
    }
    // Don't subtract cavity here - done in assembly
  }
}

// A-LEGS CAVITY: Internal space of bilateral chambers
module a_legs_cavity() {
  inner_offset = wall_thickness_mm;
  // Left leg cavity
  translate([-total_width_mm/4, 0, wall_thickness_mm])
    scale([(leg_width_mm - 2*inner_offset)/40, (leg_length_mm - 2*inner_offset)/60, 1])
      cylinder(h = dome_height_mm, d1 = 38, d2 = 28);
  // Right leg cavity
  translate([total_width_mm/4, 0, wall_thickness_mm])
    scale([(leg_width_mm - 2*inner_offset)/40, (leg_length_mm - 2*inner_offset)/60, 1])
      cylinder(h = dome_height_mm, d1 = 38, d2 = 28);
}

// OPEN CHANNEL CUTOUT: Drainage/access between legs
module open_channel_cutout() {
  // Central channel that runs between the A-legs
  // Allows fluid drainage and vaginal/urethral access
  translate([0, -leg_length_mm/4, -1])
    scale([1, 1.5, 1])
      cylinder(h = dome_height_mm + 10, d = channel_width_mm);
}

// A-APEX SHELL: Clitoral cylinder (Zone 2)
module a_apex_shell() {
  // Small cylinder at the apex for focused clitoral stimulation
  translate([0, apex_position_y_mm, 0]) {
    // Outer shell
    cylinder(h = apex_cylinder_height_mm, d = apex_cylinder_diameter_mm + 2*wall_thickness_mm);
    // Domed top
    translate([0, 0, apex_cylinder_height_mm])
      sphere(d = apex_cylinder_diameter_mm + 2*wall_thickness_mm);
  }
}

// A-APEX CAVITY: Internal space of clitoral cylinder
module a_apex_cavity() {
  translate([0, apex_position_y_mm, wall_thickness_mm]) {
    // Main cavity
    cylinder(h = apex_cylinder_height_mm + 5, d = apex_cylinder_diameter_mm);
  }
}

// A-CROSSBAR DIAPHRAGM: Separates Zone 1 from Zone 2
module a_crossbar_diaphragm() {
  // Horizontal barrier that separates the outer chambers from the clitoral cylinder
  translate([0, apex_position_y_mm - apex_cylinder_diameter_mm, crossbar_height_mm])
    difference() {
      // Crossbar plate connecting the two legs around the apex
      hull() {
        translate([-total_width_mm/4, -10, 0])
          cylinder(h = crossbar_thickness_mm, d = leg_width_mm);
        translate([total_width_mm/4, -10, 0])
          cylinder(h = crossbar_thickness_mm, d = leg_width_mm);
        translate([0, 5, 0])
          cylinder(h = crossbar_thickness_mm, d = leg_width_mm);
      }
      // Hole for clitoral cylinder passage
      translate([0, apex_cylinder_diameter_mm, -1])
        cylinder(h = crossbar_thickness_mm + 2, d = apex_cylinder_diameter_mm + 1);
    }
}

// PORT EXTENSIONS with chamber identification
module port_extension(chamber_type) {
  difference() {
    cylinder(h = port_height_mm, d = port_diameter_mm);
    translate([0, 0, -1])
      cylinder(h = port_height_mm + 2,
               d = (chamber_type == "OUTER") ? luer_socket_min_id_mm : luer_socket_min_id_mm * 0.8);
  }
}

// OUTER CHAMBER PORT - connects to SOL1/SOL2 for Zone 1
module outer_chamber_port() {
  translate([0, 0, -wall_thickness_mm])
    cylinder(h = wall_thickness_mm + 2,
             d1 = luer_socket_max_id_mm,
             d2 = luer_socket_min_id_mm);
}

// INNER CHAMBER PORT - connects to SOL4/SOL5 for Zone 2
module inner_chamber_port() {
  translate([0, 0, -wall_thickness_mm])
    cylinder(h = wall_thickness_mm + 2,
             d1 = luer_socket_max_id_mm * 0.9,
             d2 = luer_socket_min_id_mm * 0.8);
}

// A-CONTOUR SEAL LIP - follows the A-shape perimeter
module a_contour_seal_lip() {
  translate([0, 0, -seal_lip_thickness_mm])
    difference() {
      // Outer seal following A-shape
      hull() {
        translate([-total_width_mm/4, 0, 0])
          scale([leg_width_mm/40 + 0.2, leg_length_mm/60 + 0.2, 1])
            cylinder(h = seal_lip_thickness_mm, d = 44);
        translate([total_width_mm/4, 0, 0])
          scale([leg_width_mm/40 + 0.2, leg_length_mm/60 + 0.2, 1])
            cylinder(h = seal_lip_thickness_mm, d = 44);
        translate([0, apex_position_y_mm - 10, 0])
          cylinder(h = seal_lip_thickness_mm, d = leg_width_mm * 1.4);
      }
      // Inner cutout (slightly smaller than shell base)
      hull() {
        translate([-total_width_mm/4, 0, -1])
          scale([leg_width_mm/40, leg_length_mm/60, 1])
            cylinder(h = seal_lip_thickness_mm + 2, d = 40);
        translate([total_width_mm/4, 0, -1])
          scale([leg_width_mm/40, leg_length_mm/60, 1])
            cylinder(h = seal_lip_thickness_mm + 2, d = 40);
        translate([0, apex_position_y_mm - 10, -1])
          cylinder(h = seal_lip_thickness_mm + 2, d = leg_width_mm * 1.2);
      }
      // Open channel in seal lip too
      translate([0, -leg_length_mm/4, -2])
        scale([1, 1.5, 1])
          cylinder(h = seal_lip_thickness_mm + 4, d = channel_width_mm + seal_lip_width_mm);
    }
}

// A-CONTOUR STRAP LUGS - positioned around A-shape perimeter
module a_contour_strap_lugs() {
  for (pos = strap_positions) {
    translate(pos)
      strap_lug();
  }
}

module strap_lug() {
  difference() {
    // Lug body
    cube([strap_lug_thickness_mm, strap_lug_width_mm, strap_lug_height_mm], center = true);
    // Strap slot
    translate([0, 0, strap_lug_height_mm/4])
      cube([strap_lug_thickness_mm + 2, strap_slot_width_mm, strap_slot_height_mm], center = true);
  }
}

// =============================================================================
// CAMERA WINDOW MODULE (for WINDOW_VOID option)
// =============================================================================
module camera_window_void() {
  if (camera_window_enabled) {
    window_z = dome_height_mm * camera_window_height_ratio;
    // Position on left A-leg for best viewing angle
    translate([-total_width_mm/4, 0, window_z])
      rotate([0, 90, 0])
        cylinder(h = wall_thickness_mm + 5, d = camera_window_diameter_mm, center = true);
  }
}

module camera_window_flange() {
  if (camera_window_enabled) {
    window_z = dome_height_mm * camera_window_height_ratio;
    color([0.9, 0.95, 1.0, 0.3])
    translate([-total_width_mm/4 - wall_thickness_mm, 0, window_z])
      rotate([0, 90, 0])
        cylinder(h = camera_window_glass_thickness_mm, d = camera_window_diameter_mm - 1);
  }
}

module camera_mount_boss() {
  if (camera_mount_enabled) {
    color([0.3, 0.3, 0.35, 0.9])
    // Mount on apex cylinder top
    translate([0, apex_position_y_mm, apex_cylinder_height_mm + 5])
      difference() {
        cylinder(h = camera_mount_height_mm, d = camera_mount_diameter_mm + 4);
        translate([0, 0, -1])
          cylinder(h = camera_mount_height_mm + 2, d = camera_mount_diameter_mm);
      }
  }
}

// =============================================================================
// RENDER ENTRYPOINT
// =============================================================================
$fn = 64; // Smooth curves

if (show_cross_section) {
  // Cross-section view - cut through center to see chambers
  difference() {
    a_contour_cup_assembly();
    // Cut plane through center (Y-Z plane)
    translate([-100, 0, -10])
      cube([200, 100, 100]);
  }
} else {
  // Normal full assembly view
  a_contour_cup_assembly();
}

// Optional dimension helpers
if (show_dimensions) {
  color([0, 1, 0, 0.3]) {
    // Zone 1 footprint (A-legs)
    translate([0, 0, 0.5]) linear_extrude(0.5) a_shape_2d();
  }
  color([1, 0, 0, 0.3]) {
    // Zone 2 footprint (A-apex)
    translate([0, apex_position_y_mm, crossbar_height_mm + 1])
      cylinder(h = 0.5, d = apex_cylinder_diameter_mm);
  }
}

// =============================================================================
// CONSOLE OUTPUT - Design Specifications
// =============================================================================
echo("╔═══════════════════════════════════════════════════════════════╗");
echo("║         A-CONTOUR DUAL-CHAMBER VACUUM CUP                     ║");
echo("║         Revolutionary Anatomical Design                       ║");
echo("╚═══════════════════════════════════════════════════════════════╝");
echo("");
echo("OVERALL DIMENSIONS:");
echo(str("  Total width:  ", total_width_mm, "mm (across both A-legs)"));
echo(str("  Total length: ", total_length_mm, "mm (apex to leg base)"));
echo(str("  Dome height:  ", dome_height_mm, "mm"));
echo(str("  Wall thickness: ", wall_thickness_mm, "mm"));
echo("");
echo("ZONE 1 - A-LEGS (Bilateral Outer Chambers):");
echo(str("  Leg width:    ", leg_width_mm, "mm each"));
echo(str("  Leg length:   ", leg_length_mm, "mm"));
echo(str("  Leg spacing:  ", leg_spacing_mm, "mm (open channel)"));
echo("  Function:     Peripheral seal, blood engorgement, labia stimulation");
echo("  Vacuum:       Constant/slow variation (30-50 mmHg)");
echo("  Connected to: SOL1/SOL2 + AVL sensor");
echo("");
echo("ZONE 2 - A-APEX (Clitoral Cylinder):");
echo(str("  Cylinder ID:  ", apex_cylinder_diameter_mm, "mm"));
echo(str("  Cylinder height: ", apex_cylinder_height_mm, "mm"));
echo("  Function:     Targeted clitoral air-pulse stimulation");
echo("  Modes:        Sustained vacuum OR rapid pulsing (5-13 Hz)");
echo("  Connected to: SOL4/SOL5");
echo("");
echo("A-CROSSBAR (Internal Diaphragm):");
echo(str("  Height:       ", crossbar_height_mm, "mm from base"));
echo(str("  Thickness:    ", crossbar_thickness_mm, "mm"));
echo("  Function:     Separates Zone 1 from Zone 2 for independent control");
echo("");
echo("OPEN CHANNEL (Key Differentiator):");
echo(str("  Width:        ", channel_width_mm, "mm"));
echo("  Benefits:");
echo("    - Fluid drainage (urine, lubrication, ejaculate)");
echo("    - Squirting-compatible design");
echo("    - Urethral/vaginal access during operation");
echo("    - Easy cleanup - vacuum lines stay dry");
echo("");
echo("CAMERA OPTIONS:");
echo(str("  Current mode: ", camera_placement));
if (camera_window_enabled) {
  echo(str("  Window diameter: ", camera_window_diameter_mm, "mm"));
}
echo("");
echo("STRAP ATTACHMENTS:");
echo(str("  Enabled: ", strap_attachments_enabled ? "YES" : "NO"));
echo(str("  Positions: ", len(strap_positions), " lugs on A-leg perimeter"));
echo("");
echo("MATERIALS:");
echo("  Shell:      PC/Tritan (transparent, autoclavable)");
echo("  Seal lip:   Silicone Shore A 10-15");
echo("  Compliance: ISO 10993 biocompatible, ISO 80369-7 Luer");
