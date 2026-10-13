#!/usr/bin/env python3
"""Fits the burn rate Ba of the Vihtavuori shotgun powders (N320, N340, 3N37, 3N38, N105) to Vihtavuori's shotgun load tables (12/76, lead and steel shot).

    fit_vv_shot.py <csr_eval> <data dir> vv_shot_samples.csv [-o ba.json]

Samples: scripts/data/loaddata/vv_shot_extract.py (velocities only, no pressures, measured 2.5 m in front of the muzzle with 3 mm pellets: converted to the
muzzle with the air drag of the pellets, as in fit_shotgun.py). Without pressures Ba is not determined by the velocities alone (N320 ran to 20 and
1,200 bar), so no load may exceed the C.I.P. maximum of 1,050 bar (a one-sided penalty as in fit_shotgun.py). Reports the baseline (Ba of the rifle / pistol record), the fit on all loads and a hold-out:
Ba fitted on the lead loads and tested on the steel loads, and the other way round (the shot column of steel is longer: shot_density column)."""
import argparse, csv, json, math, subprocess, tempfile, os, numpy as np
from scipy.optimize import least_squares

ap = argparse.ArgumentParser()
ap.add_argument("exe"); ap.add_argument("data"); ap.add_argument("csv"); ap.add_argument("-o", "--out")
a = ap.parse_args()
rows = list(csv.DictReader(open(a.csv)))
DENS = {"lead": 11340.0, "steel": 7870.0}

def muzzle(r):
    pr = float(r["pellet_mm"]) / 2000.0
    k = 1.2 * 0.47 * math.pi * pr * pr / (2.0 * DENS[r["material"]] * 4.0 / 3.0 * math.pi * pr ** 3)
    return float(r["velocity_ms"]) * math.exp(k * float(r["distance_m"]))

def simulate(rs, ba):
    with tempfile.TemporaryDirectory() as d:
        p = os.path.join(d, "x.csv")
        with open(p, "w", newline="") as f:
            w = csv.DictWriter(f, list(rows[0].keys())); w.writeheader(); w.writerows(rs)
        cmd = [a.exe, p, "--data", a.data] + [x for k, v in ba.items() for x in ("--ba", f"{k.lower()}={v:.8g}")]
        out = subprocess.run(cmd, capture_output=True, text=True).stdout
    v = np.full(len(rs), np.nan); p = np.full(len(rs), np.nan)
    for l in out.splitlines():
        i, vv, pp = l.split(","); v[int(i)] = float(vv); p[int(i)] = float(pp)
    return v, p

LIMIT = 1050.0  # bar, C.I.P. maximum of the 12/76

def resid(rs, ba, penalty=False):
    v, p = simulate(rs, ba)
    e = np.nan_to_num(v / np.array([muzzle(r) for r in rs]) - 1.0, nan=1.0)
    return np.concatenate([e, np.nan_to_num(np.maximum(p / LIMIT - 1.0, 0.0), nan=1.0)]) if penalty else e

def stats(rs, ba):
    e = resid(rs, ba); p = simulate(rs, ba)[1]
    mx = [q for q, r in zip(p, rs) if r["kind"] == "max"]
    return f"n={len(rs):2d} bias {100 * e.mean():+5.1f} %  rms {100 * np.sqrt((e ** 2).mean()):4.1f} %  p max-loads {min(mx):.0f}-{max(mx):.0f} bar"

powders = sorted({r["powder"] for r in rows})
def fit(rs):
    ps = sorted({r["powder"] for r in rs})
    def f(x):
        ba = dict(zip(ps, x)); return resid(rs, ba, True)
    return dict(zip(ps, least_squares(f, np.full(len(ps), 2.0), bounds=(0.1, 30), diff_step=2e-3, x_scale=2.0).x))

# baseline: the Ba of the (rifle / pistol) records, i.e. no --ba override
print("baseline (record Ba): ", stats(rows, {}))
lead = [r for r in rows if r["material"] == "lead"]; steel = [r for r in rows if r["material"] == "steel"]
ba_all = fit(rows)
print("Ba fitted on all:     ", stats(rows, ba_all), {k.split()[1]: round(v, 3) for k, v in ba_all.items()})
for name, train, test in (("lead -> steel", lead, steel), ("steel -> lead", steel, lead)):
    ps = {r["powder"] for r in test} & {r["powder"] for r in train}
    ba = fit(train); t = [r for r in test if r["powder"] in ps]
    print(f"hold-out {name}:", stats(t, ba), " (baseline of these:", stats(t, {}) + ")")
for p in powders:
    sub = [r for r in rows if r["powder"] == p]
    print("  %-18s Ba %.3f  %s  (record Ba: %s)" % (p, ba_all[p], stats(sub, ba_all), stats(sub, {})))
if a.out: json.dump({"ba": ba_all, "n": {p: sum(r["powder"] == p for r in rows) for p in powders}}, open(a.out, "w"), indent=1)
