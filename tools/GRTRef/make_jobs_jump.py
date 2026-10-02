#!/usr/bin/env python3
"""Bullet jump and shotshell experiments, batch 10. Output: <out>/exp_*.grtload (default jobs/).

The solver ignores two inputs of the original so far: the bullet jump (gun section: gjump [mm], used only with
gjumpUnlock = true; the original reports a "gjumpTime") and the shotshell flag of the caliber. Their effect is not
documented; this batch measures it, one change at a time:
  ja_<base>_j<mm>      jump unlocked, distance 0 ... 4 mm (8x57 IS and 9 mm Luger base loads)
  jb_<base>_locked     distance 2 mm but locked: is the distance ignored then?
  jc_8x57_j1_ps<bar>   jump with another start pressure
  jd_8x57_Ba3_j<mm>    jump with a fast powder
  sa_<base>_shot       shotshell = true on the 8x57 / 9 mm base loads (same load otherwise)
  sb_12-70_<x>         a 12/70 slug load (Lyman Sabot Slug, N32C) with shotshell false / true, start pressure
The first job (exp_aa_warmup) only absorbs the original's empty-first-result glitch."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from jobgen import base8, build, build_load, db, load_ratios, setv, Writer, OUT

out = sys.argv[1] if len(sys.argv) > 1 else OUT
import jobgen; jobgen.OUT = out
write = Writer()
write("aa_warmup", base8)
ratios = load_ratios([("9mm", "titegroup")])
base9, _ = build("9mm", "titegroup", 0.45 / ratios["9mm_titegroup"])
bases = {"8x57": base8, "9mm": base9}

def jump(s, mm, unlocked=True):
    return setv(setv(s, "gun", "gjump", mm), "gun", "gjumpUnlock", "true" if unlocked else "false")

for name, s in bases.items():
    for mm in (0, 0.25, 0.5, 1, 2, 4):
        write(f"ja_{name}_j{mm:g}", jump(s, mm))
    write(f"jb_{name}_locked", jump(s, 2, unlocked=False))
    write(f"sa_{name}_shot", setv(s, "caliber", "shotshell", "true"))
for ps in (100, 500):
    write(f"jc_8x57_j1_ps{ps}", setv(jump(base8, 1), "caliber", "ps", ps))
b3 = setv(base8, "propellant", "Ba", 3.0)
for mm in (1, 4):
    write(f"jd_8x57_Ba3_j{mm}", jump(b3, mm))

cal = dict(db.execute("select * from caliber where id=531").fetchone())       # Cal. 12/70
slug = dict(db.execute("select * from projectile where id=3087").fetchone())  # Lyman Sabot Slug 525 gr
powder = dict(db.execute("select * from propellant where id=584").fetchone())  # Vihtavuori N32C
s12 = build_load(cal, slug, powder, 1.8, cal["L3"], 710)
write("sb_12-70_plain", s12)
write("sb_12-70_shot", setv(s12, "caliber", "shotshell", "true"))
write("sb_12-70_shot_ps100", setv(setv(s12, "caliber", "shotshell", "true"), "caliber", "ps", 100))
print(write.n, "jobs written to", out)
