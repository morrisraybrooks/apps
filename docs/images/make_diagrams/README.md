# Diagram scripts

These scripts draw the SVG pictures in `docs/images/`.

| Script | Draws | Needs |
|---|---|---|
| `sys_figs.py` | `system-overview.svg`, `vacuum-plumbing.svg` | plain Python 3 |
| `cup_figs.py` | `cup020-footprint.svg`, `cup020-side-cut.svg`, `cup020-cross-cuts.svg` | Python with `numpy`, `trimesh`, `shapely` |

```bash
python sys_figs.py ../
python cup_figs.py ../../../hardware/cup/v6 ../
```

`cup_figs.py` slices the real `shell.stl` and `seal.stl`, so the cup pictures match the model exactly.
The system pictures are drawn by hand from the pin and sensor numbers in `src/hardware/` and `src/safety/`.
