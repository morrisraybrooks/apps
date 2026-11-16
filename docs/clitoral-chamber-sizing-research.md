# Clitoral Chamber Sizing – Anatomy Research & Design Basis

*Compiled 2026-11-14 for the A-Contour cup redesign (hardware/cup/v6, `cup_sdf.py` / Blender "Cup" sidebar).
Updated 2026-11-16 for the current design, **cup_020**.*

## Current design – cup_020 (glans + shaft)

The chamber went through three shapes:

1. **Teardrop** (cup_004–005): 1" × 2", then flipped bulb-down, 32 × 55 mm.
2. **Nose** (cup_006–018): rounded, bulb over the glans, 32 × 55 mm, 63.5 mm deep, mouth at full width (no undercut).
3. **Glans + shaft** (cup_019, then cup_020): shaped like a penis head and shaft. Good size and shape references exist for
   that shape, but none were found for a clitoris drawn under vacuum.

In **cup_020** the shaft is lengthened so the head and corona stand up **out through the shell**. The base of the shaft
sits inside the cup, surrounded by the leg chamber's air.

| Feature | cup_020 | Reason |
|---|---|---|
| Shaft width at the skin | **27 mm** (> 1") | Room for the fully engorged clitoris |
| Shaft length (skin → corona) | **44 mm** | Lets the head and corona stand above the shell (cup_019 used 18 mm) |
| Corona | **33 mm** – wider than the shaft | A full corona was chosen deliberately – this is an **undercut** (see Safety) |
| Head (corona → tip) | 26 mm | |
| Total depth | **≈ 70 mm (2.75")** | Same length as a large commercial cylinder (70 mm) |
| Volume | 34.3 cm³ | |
| Neck where shaft meets corona | rounded 3 mm | No sharp ring pressing on swollen tissue |
| Lean | 15° toward the apex / pubic bone | Follows the shaft and hood |
| Position | moved 4 mm toward the apex | Keeps a clean gap above the open channel (urethra stays out of the vacuum) |
| Hood retractor ridge | soft silicone, 1.5 mm, 120° arc on the apex side of the mouth | Presses the skin above the hood and holds the hood back; **nothing goes under the hood** |
| Clitoral port | at the glans tip, sleeved through the leg air | Pulls from the deepest point |
| Manual vent | silicone flap on the side of the glans head | Releases the chamber with no power |
| Outer (leg) chamber | 204.1 cm³; legs joined over the top; open channel between the legs | Fluids drain out of the channel |
| Overall | 93.5 × 128.5 × 79.6 mm, one piece, two ports | Passes the skin-safety edge check (0 sharp / 0 thin on the skin side) |

The rest of this document is the original research and the first design basis. The measured anatomy still applies.

## Why this exists

The clitoral chamber was a 1" × 2" teardrop with the **bulb at the top** and a shallow, flat roof.
Under vacuum the clitoris engorges and swells a lot, so the chamber needs to be **bigger around,
more bulbous and deeper**, and the teardrop should be **flipped 180°** so it matches the shape of
the resting clitoris (narrow shaft/hood going up, round glans at the bottom).

## Measured anatomy (resting)

| Structure | Typical size | Source |
|---|---|---|
| Glans | mean **6.4 mm long × 5.1 mm wide** | Longhurst et al. 2024 meta-analysis (21 studies, MRI/ultrasound/cadaver/living) |
| Glans (prepuce retracted, calipers) | **8.7 ± 2.1 mm long × 6.0 ± 1.5 mm wide** | Turk J Obstet Gynecol, 208 premenopausal women |
| Body / shaft | mean **25.5 mm long × 9.0 mm wide** | Longhurst 2024 |
| Prepuce (hood) | mean **23.2 mm long** | Longhurst 2024 |
| Crura (internal) | mean 52.4 mm long × 8.7 mm wide | Longhurst 2024 |
| Bulbs (internal) | mean 52.0 mm long × 10.3 mm wide | Longhurst 2024 |
| Clitoris → urethral meatus | mean **22.3 mm** | Longhurst 2024 |
| Clitoris → vaginal opening | mean **43.1 mm** | Longhurst 2024 |
| Clitoris → anus | mean 76.3 mm | Longhurst 2024 |
| Labia minora | median 19 mm wide, 35.5 mm long (200 women); pooled mean width 18.3 mm | J Sex Med 2020; 2025 meta-analysis |

All structures vary widely between individuals – design for the large end, not the average.

## Engorgement

- MRI can measure clitoral volume change during arousal reproducibly (Maravilla et al. 2003; Suh et al. 2004).
- A patent based on that MRI work reports **clitoral size +85 % ± 5 %** and blood volume +43 % ± 5 %
  with erotic stimulus. *Patent figure, small samples (≈8 women) – treat as approximate.*
- No published measurement was found for size **under vacuum** specifically. Vacuum draws engorged
  tissue (glans, hood, and the tops of the labia minora) into the chamber, so expect growth beyond
  the arousal figures above.

## Commercial reference points

- **EROS-CTD** (FDA De Novo DEN000006, 2000): small soft vacuum cup over the clitoris, prescription device.
  Cup size and pressure were not in the public summaries found.
- Clitoral pump cylinders on the market: **small ≈ 15 mm (0.6") opening × 38 mm long**;
  **large ≈ 32 mm (1.25") wide × 70 mm long**. Round or oval, straight-walled.

## First design basis (teardrop / nose, cup_004–018)

*Kept for history. The current glans + shaft values are in the cup_020 table at the top.*

| Decision | Value | Reason |
|---|---|---|
| Orientation | **Teardrop flipped: bulb DOWN, tail UP** | Glans (round) sits at the bottom of the shaft; shaft + hood (narrow) run up toward the pubic bone |
| Bulb width | **≈ 32 mm** | Same as a large commercial cylinder; room for a glans/hood complex roughly doubling in size |
| Length | **≈ 55 mm** | Glans + shaft (≈ 25 mm) + hood (≈ 23 mm) + margin |
| Depth | **≈ 38 mm**, domed roof (superellipse p ≈ 3) | Tissue can draw in without hitting the roof; deepest over the bulb, shallower toward the tail |
| Walls | **No undercut** – the mouth is the widest point | A chamber wider inside than its opening can trap swollen tissue and make removal painful; commercial cylinders are straight-walled for this reason. **cup_020 departs from this on purpose (corona wider than the shaft)** – see Safety |
| Urethra | Open gap must start **≥ ~18 mm below the bulb centre** | Meatus is ~22 mm below the glans; keep it in the open channel, never under the clitoral vacuum |
| Port 2 | Over the bulb | Pulls from the deepest part of the chamber |

## Safety

- A bigger, deeper chamber gives tissue **room**; it does **not** pull harder. Suction force is set by
  the vacuum level only, which stays capped by the controller: **75 mmHg hard cap, 80 mmHg emergency
  stop, 100 mmHg absolute max** (`src/safety/SafetyConstants.h`).
- Known risks of excessive vacuum or duration: petechiae (broken capillaries), numbness, tissue damage.
  Start low and short; stop on pain, numbness or very dark discoloration.
- Clitoral pumps must not be applied to the vaginal opening (Wikipedia, *Clitoral pump*).

### cup_020-specific

- **Undercut:** the corona (33 mm) is wider than the shaft (27 mm), so engorged tissue can fill the head and be
  wider than the way out. Before removing: **vent both chambers** (lift the flap knobs), **wait for the swelling to go
  down**, and never pull the cup off under vacuum. See
  [cup-cleaning-and-entrapment-safety.md](cup-cleaning-and-entrapment-safety.md).
- **Deep tube:** a 70 mm chamber gives tissue room to be drawn in a long way, and how far it goes depends on
  vacuum level **and time**. cup_019 kept the depth to about 44 mm because, with the hood held back, a deeper pull
  could stretch the clitoris. cup_020 goes deeper so the head stands out through the shell, which makes the
  "start low and short" rule more important. This matters most where the tissue is thinner, for example after menopause.

## Sources

- Longhurst G. et al. (2024) *Beyond the tip of the iceberg: a meta-analysis of the anatomy of the clitoris*, Clinical Anatomy –
  https://fgmcri.org/academic_repository/beyond-the-tip-of-the-iceberg-a-meta-analysis-of-the-anatomy-of-the-clitoris
- *Do external female genital measurements affect genital perception and sexual function and orgasm?* Turk J Obstet Gynecol –
  https://tjoddergisi.org/articles/do-external-female-genital-measurements-affect-genital-perception-and-sexual-function-and-orgasm/doi/tjod.galenos.2020.89896
- Maravilla K.R. et al. (2003) *Dynamic MR imaging of the sexual arousal response in women*, J Sex Marital Ther, PMID 12735090 –
  https://api-pubmed-ncbi.laibokeji.com/12735090
- Suh D.D. et al. (2004) *MRI of female genital and pelvic organs during sexual arousal* –
  https://www.bohrium.com/paper-details/mri-of-female-genital-and-pelvic-organs-during-sexual-arousal/812330991564619776-7462
- US Patent 6,548,044 *Imaging sexual response* (85 % size / 43 % blood-volume figures) –
  https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/6548044
- FDA (2000) EROS-CTD announcement – https://webarchive.library.unt.edu/eot2008/20081019172149/http://www.fda.gov/bbs/topics/ANSWERS/ANS01012.html
- FDA De Novo DEN000006 – https://fda.innolitics.com/device/DEN000006
- Clitoral pump – https://en.wikipedia.org/wiki/Clitoral_pump
- Temptasia beginner's clitoral pumping system (cylinder 2.75" × 1.25") –
  https://m.reach.dog/shop/mypleasure/products/temptasia-beginners-clitoral-pumping-system
- Labia minora measurements, Frontiers in Surgery (2022) – https://www.frontiersin.org/journals/surgery/articles/10.3389/fsurg.2022.961247/full
