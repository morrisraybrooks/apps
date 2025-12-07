// Vacuum Cup - Parametric Model
// Default: 22x14 mm inner oval, Luer-lock female ports (ISO 80369-7), AVL tap
// Materials (comments for manufacturing): Shell PC/Tritan; Seal lip silicone (Shore A 10–15)
// Pressure rating: −40 kPa continuous, −60 kPa transient

// DUAL-CHAMBER VACUUM CUP DESIGN
// Core params (mm) - Advanced dual-chamber system
dome_diameter_mm = 80;         // Base diameter - vulva coverage
dome_height_mm = 50;           // Height of dome from base
wall_thickness_mm = 2.0;       // Wall thickness

// Dual Chamber Configuration
outer_chamber_diameter_mm = 80;    // Vulva zone - full coverage
inner_chamber_diameter_mm = 15;    // Clitoral zone - focused stimulation
chamber_separation_height_mm = 8;  // Height where chambers separate
diaphragm_thickness_mm = 1.0;      // Internal barrier thickness

// Base sealing
base_rim_width_mm = 4;         // Width of base rim for sealing
seal_lip_width_mm = 3;         // Outer seal lip
seal_lip_thickness_mm = 1.5;   // Thickness of seal lip

// Dual Port Configuration
connector_type = "luer_lock_female";
port_height_mm = 8;            // Height of port extensions
port_diameter_mm = 6;          // External diameter of ports
port_separation_angle = 45;    // Angle between ports (degrees)

// Luer-lock dimensions (simplified): ISO 80369-7 slip taper 6% (1:16)
luer_taper_deg = 3.4375; // half-angle
luer_socket_len_mm = 7.0;
luer_socket_min_id_mm = 3.9; // near seat
luer_socket_max_id_mm = luer_socket_min_id_mm + 2*luer_socket_len_mm*tan(luer_taper_deg);

// Aux port nominal smaller flow
aux_luer_min_id_mm = 3.0;

// AVL pickoff micro-lumen
avl_pickoff_id_mm = 2.0;
avl_pickoff_offset_mm = 6; // distance from primary socket toward cavity

// Dual-chamber diaphragm
dual_chamber_enabled = true;
diaphragm_thickness_mm = 0.5;
diaphragm_clear_diam_mm = 10; // ~79 mm^2 area
diaphragm_floor_offset_mm = 4;

// Visualization options
show_cross_section = false; // Set to true to see dual-chamber cross-section
show_dimensions = false;    // Set to true to show dimension helpers
show_chamber_colors = true; // Color-code chambers for visualization

// DUAL-CHAMBER ASSEMBLY - Advanced vacuum cup system
module cup_assembly() {
  // Clear plastic dome shell (PC/Tritan) - highly transparent
  color([0.95, 0.97, 1.0, 0.2]) // Very light blue tint, highly transparent
  difference() {
    // Main dome shell
    dome_shell();
    // Outer chamber cavity (vulva zone)
    outer_chamber_cavity();
    // Inner chamber cavity (clitoral zone)
    inner_chamber_cavity();
    // Port 1 socket (Outer chamber - SOL1/SOL2 + AVL)
    translate([port_diameter_mm * cos(port_separation_angle),
               port_diameter_mm * sin(port_separation_angle),
               dome_height_mm])
      outer_chamber_port();
    // Port 2 socket (Inner chamber - SOL3)
    translate([port_diameter_mm * cos(-port_separation_angle),
               port_diameter_mm * sin(-port_separation_angle),
               dome_height_mm])
      inner_chamber_port();
    // AVL sensor tap on outer chamber
    translate([dome_diameter_mm/3, 0, dome_height_mm * 0.6])
      rotate([0, 90, 0])
        cylinder(h = wall_thickness_mm + 2, d = avl_pickoff_id_mm);
  }

  // Internal chamber separation diaphragm - key component
  color([0.9, 0.9, 1.0, 0.4]) // Light blue for internal barrier
  chamber_diaphragm();

  // Port 1 extension (Outer chamber - Vulva zone)
  color([0.8, 0.9, 0.8, 0.5]) // Light green for outer chamber port
  translate([port_diameter_mm * cos(port_separation_angle),
             port_diameter_mm * sin(port_separation_angle),
             dome_height_mm])
    port_extension("OUTER");

  // Port 2 extension (Inner chamber - Clitoral zone)
  color([0.9, 0.8, 0.8, 0.5]) // Light red for inner chamber port
  translate([port_diameter_mm * cos(-port_separation_angle),
             port_diameter_mm * sin(-port_separation_angle),
             dome_height_mm])
    port_extension("INNER");

  // Base seal lip (silicone) - soft translucent
  color([0.85, 0.85, 0.9, 0.7]) // Light gray for silicone seal
  seal_lip();
}

// DUAL-CHAMBER GEOMETRY IMPLEMENTATIONS

// Main dome shell - smooth hemisphere with base
module dome_shell() {
  union() {
    // Main dome - hemisphere
    translate([0, 0, dome_height_mm])
      scale([1, 1, 0.8]) // Slightly flattened dome
        sphere(d = dome_diameter_mm);

    // Base rim for sealing
    translate([0, 0, 0])
      cylinder(h = base_rim_width_mm, d = dome_diameter_mm + 2*base_rim_width_mm);
  }
}

// Outer chamber cavity - vulva zone (full dome interior)
module outer_chamber_cavity() {
  union() {
    // Outer dome cavity - full interior space
    translate([0, 0, dome_height_mm])
      scale([1, 1, 0.8])
        sphere(d = dome_diameter_mm - 2*wall_thickness_mm);

    // Base cavity for outer chamber
    translate([0, 0, -1])
      cylinder(h = chamber_separation_height_mm + 1,
               d = dome_diameter_mm - 2*wall_thickness_mm);
  }
}

// Inner chamber cavity - clitoral zone (centered smaller chamber)
module inner_chamber_cavity() {
  union() {
    // Inner chamber - smaller centered cavity
    translate([0, 0, dome_height_mm])
      scale([1, 1, 0.6]) // More focused dome shape
        sphere(d = inner_chamber_diameter_mm);

    // Base cavity for inner chamber
    translate([0, 0, chamber_separation_height_mm])
      cylinder(h = dome_height_mm - chamber_separation_height_mm + 1,
               d = inner_chamber_diameter_mm);
  }
}

// Chamber separation diaphragm - critical component
module chamber_diaphragm() {
  difference() {
    // Diaphragm disk at separation height
    translate([0, 0, chamber_separation_height_mm])
      cylinder(h = diaphragm_thickness_mm,
               d = dome_diameter_mm - 2*wall_thickness_mm);

    // Central hole for inner chamber
    translate([0, 0, chamber_separation_height_mm - 1])
      cylinder(h = diaphragm_thickness_mm + 2,
               d = inner_chamber_diameter_mm);
  }
}

// Port extensions with chamber identification
module port_extension(chamber_type) {
  difference() {
    // Port cylinder
    cylinder(h = port_height_mm, d = port_diameter_mm);
    // Port bore - different sizes for different chambers
    translate([0, 0, -1])
      cylinder(h = port_height_mm + 2,
               d = (chamber_type == "OUTER") ? luer_socket_min_id_mm : luer_socket_min_id_mm * 0.8);
  }
}

// Outer chamber port - connects to SOL1/SOL2 and AVL sensor
module outer_chamber_port() {
  // Tapered socket bore per ISO 80369-7 - larger for main vacuum
  translate([0, 0, -wall_thickness_mm])
    cylinder(h = wall_thickness_mm + 2,
             d1 = luer_socket_max_id_mm,
             d2 = luer_socket_min_id_mm);
}

// Inner chamber port - connects to SOL3 for pattern control
module inner_chamber_port() {
  // Tapered socket bore - slightly smaller for precise control
  translate([0, 0, -wall_thickness_mm])
    cylinder(h = wall_thickness_mm + 2,
             d1 = luer_socket_max_id_mm * 0.9,
             d2 = luer_socket_min_id_mm * 0.8);
}

// Seal lip - clean base rim (like reference image)
module seal_lip() {
  translate([0, 0, -seal_lip_thickness_mm])
    difference() {
      // Outer seal ring
      cylinder(h = seal_lip_thickness_mm,
               d = dome_diameter_mm + 2*base_rim_width_mm + 2*seal_lip_width_mm);
      // Inner cutout
      translate([0, 0, -1])
        cylinder(h = seal_lip_thickness_mm + 2,
                 d = dome_diameter_mm + 2*base_rim_width_mm);
    }
}

// Render entrypoint for immediate viewing
if (show_cross_section) {
  // Cross-section view - cut in half to see dual-chamber system
  difference() {
    cup_assembly();
    translate([-50, 0, -10])
      cube([100, 50, 50]);
  }

  // Show chamber boundaries in cross-section
  if (show_dimensions) {
    color([0, 1, 0, 0.6]) {
      // Outer chamber boundary
      translate([0, 0, 1])
        cylinder(h = 0.5, d = outer_chamber_diameter_mm);
      // Inner chamber boundary
      translate([0, 0, chamber_separation_height_mm + 1])
        cylinder(h = 0.5, d = inner_chamber_diameter_mm);
      // Separation plane
      translate([0, 0, chamber_separation_height_mm])
        cylinder(h = 0.2, d = outer_chamber_diameter_mm - 4);
    }
  }
} else {
  // Normal full assembly view
  cup_assembly();

  // Optional chamber visualization helpers
  if (show_dimensions) {
    color([0, 1, 0, 0.3]) {
      // Outer chamber footprint
      translate([0, 0, 1])
        cylinder(h = 0.5, d = outer_chamber_diameter_mm);
    }
    color([1, 0, 0, 0.3]) {
      // Inner chamber footprint
      translate([0, 0, chamber_separation_height_mm + 1])
        cylinder(h = 0.5, d = inner_chamber_diameter_mm);
    }
  }
}

// Assembly information (visible in console)
echo("=== DUAL-CHAMBER VACUUM CUP ASSEMBLY ===");
echo(str("Overall dome diameter: ", dome_diameter_mm, "mm (vulva coverage)"));
echo(str("Dome height: ", dome_height_mm, "mm"));
echo(str("Wall thickness: ", wall_thickness_mm, "mm"));
echo("");
echo("CHAMBER CONFIGURATION:");
echo(str("Outer Chamber (Vulva Zone): ", outer_chamber_diameter_mm, "mm diameter"));
echo("  - Function: Anti-detachment + baseline vacuum");
echo("  - Pressure: -20 to -40 kPa constant");
echo("  - Connected to: SOL1/SOL2 + AVL sensor");
echo(str("Inner Chamber (Clitoral Zone): ", inner_chamber_diameter_mm, "mm diameter"));
echo("  - Function: Variable patterns + air pulse stimulation");
echo("  - Pressure: 0 to -60 kPa variable");
echo("  - Connected to: SOL3 valve");
echo("");
echo(str("Chamber separation height: ", chamber_separation_height_mm, "mm"));
echo(str("Diaphragm thickness: ", diaphragm_thickness_mm, "mm"));
echo(str("Port separation angle: ", port_separation_angle, "°"));
echo(str("AVL sensor: ", avl_pickoff_id_mm, "mm ID on outer chamber"));
echo("");
echo("DESIGN FEATURES:");
echo("- Clear dome aesthetic with dual-chamber functionality");
echo("- Independent vacuum control for each zone");
echo("- Anti-detachment monitoring on outer chamber");
echo("- Focused clitoral stimulation in inner chamber");
echo("- Materials: Clear PC/Tritan shell + Silicone base seal");
echo("- Compliance: ISO 10993 biocompatible, ISO 80369-7 Luer connections");
