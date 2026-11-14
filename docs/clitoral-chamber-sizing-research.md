# Clitoral Chamber Sizing – Anatomy Research & Design Basis

*Compiled 2026-11-14 for the A-Contour cup redesign (hardware/cup/v6, `cup_sdf.py` / Blender "Cup" sidebar).*

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

## Design decisions for the chamber

| Decision | Value | Reason |
|---|---|---|
| Orientation | **Teardrop flipped: bulb DOWN, tail UP** | Glans (round) sits at the bottom of the shaft; shaft + hood (narrow) run up toward the pubic bone |
| Bulb width | **≈ 32 mm** | Same as a large commercial cylinder; room for a glans/hood complex roughly doubling in size |
| Length | **≈ 55 mm** | Glans + shaft (≈ 25 mm) + hood (≈ 23 mm) + margin |
| Depth | **≈ 38 mm**, domed roof (superellipse p ≈ 3) | Tissue can draw in without hitting the roof; deepest over the bulb, shallower toward the tail |
| Walls | **No undercut** – the mouth is the widest point | A chamber wider inside than its opening can trap swollen tissue and make removal painful; commercial cylinders are straight-walled for this reason |
| Urethra | Open gap must start **≥ ~18 mm below the bulb centre** | Meatus is ~22 mm below the glans; keep it in the open channel, never under the clitoral vacuum |
| Port 2 | Over the bulb | Pulls from the deepest part of the chamber |

## Safety

- A bigger, deeper chamber gives tissue **room**; it does **not** pull harder. Suction force is set by
  the vacuum level only, which stays capped by the controller: **75 mmHg hard cap, 80 mmHg emergency
  stop, 100 mmHg absolute max** (`src/safety/SafetyConstants.h`).
- Known risks of excessive vacuum or duration: petechiae (broken capillaries), numbness, tissue damage.
  Start low and short; stop on pain, numbness or very dark discoloration.
- Clitoral pumps must not be applied to the vaginal opening (Wikipedia, *Clitoral pump*).

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
