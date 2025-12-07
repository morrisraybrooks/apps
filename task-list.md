Vacuum Cup Design — Agent Task List

Context
- Goal: Finalize design artifacts for a vacuum cup compatible with the Vacuum Controller (SOL1/SOL2/SOL3, AVL/Tank, anti-detachment).
- Defaults: Inner oval 22 × 14 mm; dome depth 14 mm; wall 2.0 mm.
- Ports: Off-the-shelf female Luer-lock inserts (ISO 80369-7), Part Number: 0-0-1.
- Safety/specs: −40 kPa continuous, −60 kPa transient; ISO 10993 skin-contact materials.
- Keep check-list.md as transcript; this file is the actionable checklist.

Deliverables (should already exist in repo)
- docs/cup_design.svg (overview diagram)
- hardware/cup/cup.scad (parametric OpenSCAD model)
- docs/manufacturing/cup_exploded.svg (exploded view)
- docs/manufacturing/cup_tech_drawing.svg (dimensioned drawing)
- docs/manufacturing/cup_tech_drawing.dxf (exchange drawing)
- docs/manufacturing/README.md (instructions)
- docs/manufacturing/Makefile (PDF export via Inkscape)

Checklist
- [ ] Validate environment
  - [ ] Check memory and temp (free -h; vcgencmd measure_temp)
  - [ ] Verify Inkscape installed (inkscape --version)
- [ ] Export PDFs
  - [ ] Overview PDF: docs/cup_design.pdf
  - [ ] Manufacturing PDFs via Makefile: docs/manufacturing/cup_exploded.pdf, docs/manufacturing/cup_tech_drawing.pdf
- [ ] Verify outputs exist and open cleanly (no missing text/lines)
- [ ] Optional: Commit PDFs (ask before committing) and update README with locations

Commands (run from repo root)
- Memory/thermal checks:
  - free -h
  - vcgencmd measure_temp  # if available
- Export overview diagram to PDF:
  - inkscape docs/cup_design.svg --export-type=pdf --export-filename=docs/cup_design.pdf
- Export manufacturing PDFs (limit jobs per Pi rules):
  - make -C docs/manufacturing -j3 pdf

Acceptance criteria
- docs/cup_design.pdf exists and displays labels for: primary/aux female Luer-lock ports (P/N 0-0-1), AVL tee, optional diaphragm cavity, optional relief and hydrophobic filter.
- docs/manufacturing/cup_exploded.pdf and cup_tech_drawing.pdf are generated without errors; dimensions and notes visible (22 × 14 mm inner, wall 2.0 mm, depth 14 mm, ISO 10993, −40/−60 kPa).
- No overheating (>70°C) or memory overuse (>6 GB) during export.
- No changes to check-list.md (remains transcript).

Notes
- If Inkscape is missing, request permission to install it (sudo apt install inkscape) or switch to cairosvg/export alternatives.
- Luer-lock sockets assume off-the-shelf female inserts; verify press-fit/bond dimensions against vendor datasheet when selected.
- For larger (vulva) sizes, adjust hardware/cup/cup.scad parameters and regenerate drawings if needed.
