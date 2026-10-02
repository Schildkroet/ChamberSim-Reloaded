#!/usr/bin/env python3
"""Validation against PUBLISHED load data: the Vihtavuori Reloading Guide lists start and maximum charges with the
measured velocity (test barrel, C.O.L.). This builds one load per charge from the database (caliber, the closest
bullet of the same maker and weight, the newest model of the powder) so that the solver - and, on the VM, the original
program - can be compared with the measured velocities.

usage: make_jobs_vv.py <vv.csv> [--out DIR] [--cartridges a,b,...] [--sample N]
The CSV comes from tools/GRTRef/parse_vv.py (a local copy of the guide; the data is not part of the repository).
Writes DIR/exp_vv_*.grtload and DIR/vv_meta.json (published charge and velocity of each job)."""
import argparse, csv, json, os, random, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from jobgen import db, build_load, GRAIN

CAL = {"9 mm Luger / 9x19 mm": 499, ".45 Auto / .45 ACP": 244, ".38 Special": 203, ".357 Magnum": 176, ".40 S&W": 214,
       ".44 Remington Magnum": 238, ".308 Winchester": 129, ".30-06 Springfield": 91, "8 x 57 IS (8 mm Mauser)": 470}
MAKER = {"berry´s": "berry", "berry’s": "berry", "berry's": "berry", "x-treme": "x-treme", "x-treme bullets": "x-treme", "h&n": "h&n",
         "alsa pro": "alsa", "up bullets": "up", "lehigh defense": "lehigh", "sig sauer": "sig"}

def norm(s): return re.sub(r"[^a-z0-9]+", " ", s.lower()).strip()

ap = argparse.ArgumentParser()
ap.add_argument("csv"); ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "jobs"))
ap.add_argument("--cartridges", default=",".join(CAL)); ap.add_argument("--sample", type=int, default=0)
a = ap.parse_args()
rows = [r for r in csv.DictReader(open(a.csv, encoding="utf-8")) if r["cartridge"] in a.cartridges.split(",")]

powders = {}
for r in db.execute("select * from propellant where geloescht=0 and mname='Vihtavuori' order by lotid"):
    powders[r["pname"]] = dict(r)  # the newest model (lot date) wins
bullets = [dict(r) for r in db.execute("select * from projectile where geloescht=0 and gdia>0 and glen>0 and gmass>0")]

def find_bullet(cal, row):
    gr = float(row["bullet_gr"]); mk = MAKER.get(row["mfg"].lower(), norm(row["mfg"]).split()[0])
    cand = [b for b in bullets if abs(b["gdia"] - cal["c_Z"]) < 0.15 and abs(b["gmass"] - gr) < 0.6]
    same = [b for b in cand if norm(b["mname"]).startswith(mk)]
    pool = same or cand
    if not pool: return None, "none"
    words = set(norm(row["type"]).split())
    pool.sort(key=lambda b: -len(words & set(norm(b["pname"]).split())))
    return pool[0], ("maker" if same else "weight")

os.makedirs(a.out, exist_ok=True)
for f in os.listdir(a.out):
    if f.startswith("exp_vv_") and f.endswith(".grtload"): os.remove(os.path.join(a.out, f))
if a.sample: random.seed(7); rows = random.sample(rows, min(a.sample, len(rows)))
meta, skipped = {}, {"powder": 0, "bullet": 0, "depth": 0}
for i, row in enumerate(rows):
    cal = dict(db.execute("select * from caliber where id=?", (CAL[row["cartridge"]],)).fetchone())
    pw = powders.get(row["powder"])
    if not pw: skipped["powder"] += 1; continue
    b, how = find_bullet(cal, row)
    if not b: skipped["bullet"] += 1; continue
    col = float(row["col_mm"])
    if not (0.3 < cal["L3"] + b["glen"] - col < b["glen"]): skipped["depth"] += 1; continue
    tag = re.sub(r"[^A-Za-z0-9]+", "", row["cartridge"].split("/")[0])[:10]
    for which in ("start", "max"):
        name = f"vv_{tag}_{i:04d}_{which}"
        charge = float(row[f"{which}_g"])
        open(os.path.join(a.out, f"exp_{name}.grtload"), "w", encoding="utf-8").write(build_load(cal, b, pw, charge, col, int(row["barrel_mm"])))
        meta[name] = dict(cartridge=row["cartridge"], bullet=f'{row["bullet_gr"]} gr {row["mfg"]} {row["type"]}', db_bullet=f'{b["mname"]} {b["pname"]} {b["gmass"]} gr',
                          match=how, powder=row["powder"], powder_lot=pw["lotid"], charge_g=charge, col_mm=col, barrel_mm=int(row["barrel_mm"]),
                          v_ms=float(row[f"{which}_ms"]), which=which)
json.dump(meta, open(os.path.join(a.out, "vv_meta.json"), "w"), indent=1)
print(len(meta), "jobs written to", a.out, "; skipped rows:", skipped)
