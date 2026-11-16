# Cup Safety: Cleanability & Tissue Entrapment

*Compiled 2026-11-14 for the A-Contour cup (hardware/cup/v6). Updated 2026-11-16 for the current design,
**cup_020** (glans + shaft chamber). Companion to
[clitoral-chamber-sizing-research.md](clitoral-chamber-sizing-research.md).*

The cup touches mucosa and collects body fluids (lubrication, urine, ejaculate), and vacuum pulls engorged
tissue into it. Two hazards follow: **residue that can't be cleaned out** and **tissue that can't come back out**.

---

## 1. Regulatory reference points

| Source | What it requires | Numbers given |
|---|---|---|
| **FDA – Clitoral Engorgement Devices, Class II special controls** (21 CFR 884.5970, 2000) | Design must include *"safety mechanisms for quick release action of the device (e.g., vacuum ceasing mechanisms)"* and *"maximum safe operating limits (e.g., total time of use, maximum vacuum limiting mechanisms)"*. Labeling: *"use of minimal vacuum"*, *"limited time of use, minimum time between uses"*, *"cessation of device use if pain occurs"*, no use over a piercing, not under alcohol/drugs. Named risks: *unknown effects of extended use*, misplacement / use over compromised tissue. | None |
| **FDA – External Penile Rigidity Devices, Class II special controls** (21 CFR 876.5020, 2004) | A vacuum device should have *a manually operated way to quickly release the vacuum*; design should not invite extended continuous use. Typical pumps draw **< 17 inHg (≈ 430 mmHg)**. | 17 inHg |
| EROS-CTD pilot (Billups 2001, 25 women) | No clitoral trauma, bruising or irritation at its low vacuum. | – |

**This device:** controller hard cap **75 mmHg** (≈ 1/6 of a typical penile pump), emergency stop 80 mmHg,
absolute max 100 mmHg (`src/safety/SafetyConstants.h`). Vacuum level is already conservative; the gaps are
quick release that works **without** the electronics, and time limits.

---

## 2. Entrapment – how tissue gets stuck, and the design answer

| Mechanism | Why it matters | Design answer | Status in cup_020 |
|---|---|---|---|
| **Undercut chamber** – mouth narrower than the inside | Engorged tissue swells past the mouth and can't pass back out after release; constriction-device series report oedema in >90 % of entrapments and call removal *challenging* | Mouth is always the widest point of every chamber. Straight or outward-flaring walls only. | **cup_020 has a deliberate undercut:** the corona (33 mm) is wider than the shaft (27 mm). This was chosen knowingly to get a full corona. The cup's own design can't release swollen tissue, so safe removal depends on the procedure below: **vent both chambers and wait for the swelling to go down** before removing. The nose chambers up to cup_018 had no undercut. |
| **No way to release vacuum if electronics fail** (power loss, solenoid stuck closed, software hang) | FDA requires a quick-release / vacuum-ceasing mechanism on both classes of vacuum device | 1) Vent solenoids **normally-open** (power loss = vent). 2) A **manual vent on the cup** that needs no power. 3) A passive relief valve (duckbill) cracking just above the cap (85–100 mmHg). | **Manual vents (since cup_017; in cup_020):** a silicone flap valve on each chamber – clitoral (on the side of the glans head, sleeved through the leg air) and legs (top of the left leg). Vacuum holds the flap shut; **lift the tab with a fingertip = instant vent**, no power, no moving rigid parts, molded in the same silicone as the seal. Relief valve + normally-open solenoids still to do. |
| **Ring constriction at the rim** | A hard, narrow rim acts like a constriction ring on swollen tissue | Wide, soft, rounded seal lip; no sharp rim | Silicone lip with rounded profile; every rim rounded 1 mm (`edge_round`). The soft 1.5 mm hood retractor ridge is part of the same silicone lip. The neck where the shaft meets the corona is rounded 3 mm, so there's no hard ring inside the chamber. |
| **Tissue drawn into small holes** (port bores, sensor taps) | A 2–3 mm hole concentrates suction on a small spot → blisters/petechiae | Keep port openings where tissue can't reach (high in the chamber), round their edges, or cover with a diffuser/grille | Clitoral port at the glans tip, about 70 mm from the skin; leg port on top of the right leg; hole edges rounded. AVL tap removed (two ports only, sensor teed at controller). |
| **Pinch slits** – narrow crevices tissue can be pulled into | Pinching + concentrated stress | Fill any chamber space narrower than ~3 mm | `min_gap = 3 mm` (default on). |
| **Extended continuous use** | FDA: *unknown effects of extended use*; oedema increases with time | Session time limit, enforced rest between sessions, start low | Controller `pattern_timeout_minutes = 60`. **Recommend a shorter hard limit for sustained (non-pulsed) vacuum and a mandatory rest period.** This matters more with cup_020: its 70 mm chamber lets tissue be drawn further the longer the vacuum is held. |

### Removal procedure (for the user manual)
1. Stop the pattern → vent **both** chambers (SOL2 / manual vent) → wait until the pressure reads 0.
2. Break the seal at the edge with a finger; never pull the cup off under vacuum.
3. If tissue stays swollen inside the clitoral chamber: keep vented, wait, cool compress; do not pry.
   With cup_020's corona undercut, expect to wait. The head is wider than the way out until the swelling goes down.
   Pain, numbness or very dark discolouration → stop use; persistent → medical help.

---

## 3. Cleanability – design rules

Reprocessing guidance (FDA *Reprocessing Medical Devices* 2015/2017; ASTM F3357-19 design for cleaning)
lists the features that retain soil: **dead-end spaces, narrow or blind lumens, cracks and crevices, mated
surfaces, textured surfaces, hinges, sharp internal corners, and parts that can't be taken apart.**

| Rule | Why | Status in cup v6 |
|---|---|---|
| Every chamber open to the skin side – no blind pockets | Rinse/brush reaches every surface; fluid drains | Both legs and the clitoral chamber are open at the bottom. Pinch slits filled. The corona undercut and the air space around the shaft base are harder to reach: **check that a soft brush reaches both on the printed cup.** |
| No sealed internal tunnels | Can't be brushed or inspected | The legs are joined over the top of the clitoral chamber (the top wrap), which is part of the leg chamber and **open to the skin**. The older closed pathway (cup_008–017) is gone. |
| Short, straight, through-lumens only | Narrow blind lumens are worst-case for cleaning | Port bores are short straight Luer bores – flush with a syringe. |
| **Generous concave fillets** where walls meet | Sharp inside corners trap residue | Convex edges rounded 1 mm; **inside corners (divider ↔ shell) filleted 2 mm** (`inner_fillet`, since cup_017); pinch-slit fill also rounds every inside corner to ≥ 1.5 mm. |
| Smooth surface finish | Layer lines and texture hold residue/biofilm (food-safe AM guidance: ~32 µin Ra or better) | Model is smooth; **finish depends on manufacturing** – see below. |
| **One piece – nothing comes apart** | Every joint between separate parts is a seam: a sharp edge against skin and a crevice that traps residue | Silicone seal lip and vent flaps **overmolded / permanently bonded** onto the shell. No press-fits, grooves or removable parts. |
| Keep fluids out of the tubing | Fluid in lines/controller can't be cleaned and contaminates the pump | **Add an inline hydrophobic filter / backflow protector at each port** (as on breast pumps). The open-channel design already drains most fluid away from the ports. |
| Transparent shell | Residue is visible | Clear Tritan/PC. |

### Manufacturing & materials

| Option | Surface / cleaning | Notes |
|---|---|---|
| **FDM print** | Visible layer lines and voids; hard to clean, can't be sealed reliably | **Not recommended for body contact.** |
| **SLA / resin print** | Much smoother, layer-uniform; can be polished/coated | Use a resin rated for skin/mucosal contact (ISO 10993 / USP Class VI). Standard resins are not body-safe. |
| **Molded / cast** | Best finish, no porosity | Print a master, cast or mold in clear Tritan or medical silicone for production. |

Material care (general material knowledge – confirm with the specific grade's datasheet):
- **Tritan copolyester:** good chemical resistance, dishwasher-safe grades exist – best choice for the shell.
- **Polycarbonate:** prone to stress-cracking/crazing with alcohols and many cleaners – avoid IPA on PC.
- **Platinum-cure silicone (seal):** can be boiled 3–5 min or washed with mild soap; oil-based lubricants degrade silicone (also an FDA labeling item).

### Cleaning procedure (for the user manual)
1. Vent (lift both flap tabs), remove, disconnect tubing at the Luer ports. The cup is one piece – do not try to separate the seal.
2. Rinse warm water → mild unscented soap, soft brush into every chamber and along the pathway.
3. Flush each port bore with a syringe of soapy water, then clean water.
4. Wash the whole piece; lift each vent flap and rinse under it. Use a method safe for **both** materials (Tritan shell + silicone): warm soapy water, or top-rack dishwasher if the Tritan grade allows. Do not boil (the shell is not rated for it).
5. Air-dry fully, open side down. Inspect for residue, cracks or crazing before each use.

---

## 4. Action list

| # | Item | Where |
|---|---|---|
| 1 | ~~Manual vent on each chamber (works without power)~~ **done – silicone flap vents, since cup_017** | cup |
| 2 | Passive relief duckbill, cracking 85–100 mmHg | tubing |
| 3 | Normally-open vent solenoids (power loss = vent) | controller hardware |
| 4 | Inline hydrophobic filter / backflow protector on both ports | tubing |
| 5 | ~~Concave fillets (divider ↔ shell) for cleanability~~ **done – 2 mm, since cup_017** | `cup_sdf.py` |
| 6 | Hard session-time limit + rest period for sustained vacuum | controller software |
| 7 | cup_020 has a deliberate corona undercut: the "vent both chambers and wait" removal step must be printed in the manual **and** on a quick-start card kept with the cup | docs |
| 8 | Body-safe manufacturing (SLA biocompatible resin or molded Tritan; no FDM) | production |
| 9 | Removal & cleaning procedures above into `docs/USER_MANUAL.md` | docs |
| 10 | Check on the printed cup that a soft brush reaches inside the corona and around the shaft base | test |

## Sources
- FDA, *Clitoral Engorgement Devices – Class II Special Controls Guidance* (2000): https://www.fda.gov/medical-devices/guidance-documents-medical-devices-and-radiation-emitting-products/clitoral-engorgement-devices-class-ii-special-controls-guidance-document-industry-and-fda-reviewers
- 21 CFR 884.5970: https://fda.innolitics.com/submissions/OB/subpart-f%E2%80%94obstetrical-and-gynecological-therapeutic-devices/884.5970
- FDA, *External Penile Rigidity Devices – Class II Special Controls Guidance* (2004): https://www.fda.gov/medical-devices/external-penile-rigidity-devices-class-ii-special-controls-guidance-document-industry-and-fda-staff
- Billups et al. 2001, EROS-CTD pilot: https://acnpsearch.tweb-dev.unibo.it/singlejournalindex/84147205
- *Treatment Options and Outcomes of Penile Constriction Devices* (oedema in 92.5 %, removal challenging): https://www.ncbi.nlm.nih.gov/pmc/articles/PMC6541129/
- FDA, *Factors Affecting Quality Reprocessing*: https://www.fda.gov/medical-devices/reprocessing-reusable-medical-devices/factors-affecting-quality-reprocessing
- ASTM F3357-19, design features to minimise debris retention: https://cdn.standards.iteh.ai/samples/104201/fdd433b654ae4834ab03fcaa9dfd0c75/ASTM-F3357-19.pdf
- Nelson Labs, design considerations for cleaning: https://www.nelsonlabs.com/wp-content/uploads/2014/09/DesignConsideration_Horizons_Spring2012.pdf
- Xometry, AM guidelines for food products (crevices, ~32 Ra): https://www.xometry.com/resources/3d-printing/additive-manufacturing-guidelines-for-food-products/
- Formlabs, food-safe 3D printing (SLA smoother, coatings): https://formlabs.com/global/blog/guide-to-food-safe-3d-printing/
- O'Connor 2025, FDM vs SLA surface comparison: https://uhra.herts.ac.uk/id/eprint/25899/2/o-connor-2025-comparative-analysis-of-the-mechanical-properties-of-fdm-and-sla-3d-printed-components.pdf
- Sex-toy cleaning guidance (boiling silicone 3–5 min, alcohol caveats): https://www.mumsnet.com/articles/how-to-clean-sex-toys
