#!/usr/bin/env python3
"""Shotgun loads of the Vihtavuori Reloading Guide (12/76, lead and steel shot), batch 12. Output: <out>/exp_*.grtload.

The guide's measured velocities (28" barrel, light gates 2.5 m from the muzzle) are the reference for both programs,
each load with the caliber's shotshell flag off ("_off") and on ("_on"): does the original's shotshell mode bring it
closer to reality, and how does ChamberSim Reloaded do?

usage: make_jobs_shotgun.py <shotgun.csv> [out dir]     (CSV: reference/vv2026_shotgun.csv, local only)
The shot column is the "projectile": shot + 2.5 g of wad, column length = shot volume (packing 0.62) + wad height
(from the wad name "H-24" = 24 mm, steel wads 25 mm), fully inside the hull (C.O.L. = hull length), crimp release 20 bar."""
import csv, json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import jobgen
from jobgen import base8, build_load, db, setv, GRAIN

rows = list(csv.DictReader(open(sys.argv[1], encoding="utf-8")))
jobgen.OUT = sys.argv[2] if len(sys.argv) > 2 else jobgen.OUT
write = jobgen.Writer()
write("aa_warmup", base8)
cal = dict(db.execute("select * from caliber where cipname='Cal. 12/76'").fetchone())
powders = {}
for r in db.execute("select * from propellant where geloescht=0 and mname='Vihtavuori' order by lotid"):
    powders[r["pname"]] = dict(r)
DENSITY = {"lead": 11.34, "steel": 7.87}  # g/cm3
meta = {}
for i, r in enumerate(rows):
    shot = float(r["shot_g"])
    m = re.search(r"H-(\d+)", r["wad"])
    wad_mm = float(m.group(1)) if m else 25.0
    column_mm = shot / (DENSITY[r["shot_type"]] * 0.62) * 1000.0 / cal["c_Q"] + wad_mm
    bullet = dict(mname="Shot", pname=f'{r["shot_g"]} g {r["shot_type"]} + wad', gdia=cal["c_Z"], glen=round(column_mm, 2),
                  gmass=(shot + 2.5) / GRAIN, gpressure=20, gtailh=0, gtaildiaA=0, gtaildiaB=0, gtailtype=0, g1bc=0, g7bc=0)
    for which in ("start", "max"):
        s = build_load(cal, bullet, powders[r["powder"]], float(r[f"{which}_g"]), cal["L3"], int(r["barrel_mm"]))
        for flag in ("off", "on"):
            name = f"sg_{i:02d}_{which}_{flag}"
            write(name, setv(s, "caliber", "shotshell", "true" if flag == "on" else "false"))
            meta[name] = dict(row=i, which=which, shotshell=flag, shot=f'{r["shot_g"]} g {r["shot_type"]}', powder=r["powder"],
                              charge_g=float(r[f"{which}_g"]), v_ms=float(r[f"{which}_ms"]), distance_m=2.5,
                              pellet_mm=3.0, material=r["shot_type"], limit_bar=1050.0 if which == "max" else 0.0)
json.dump(meta, open(os.path.join(jobgen.OUT, "shotgun_meta.json"), "w"), indent=1)
print(write.n, "jobs written to", jobgen.OUT)
