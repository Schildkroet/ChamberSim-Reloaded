#!/usr/bin/env python3
"""Scatter of the published peak pressures around a smooth trend inside their own table (the model plays no part).

    pressure_scatter.py [--samples dir] [--data data] [--min-group 12]

For every group of loads with the same source, cartridge, powder and kind (start or maximum load) with at least --min-group pressures,
ln p is fitted with a robust loss (Huber, IRLS) as
    A:  a + b ln(bullet mass) + c ln(charge)                       (what sample_filter.py does for the velocity)
    B:  A + d ln(barrel length) + e ln(cartridge length)           (the test conditions the table gives)
    C:  B + f ln(velocity)                                         (a load's velocity says how hard the powder burned)
The residual rms per class (rifle, pistol and revolver = C.I.P. table IV, see the cippdf of the caliber record) and per source is the scatter of the
table itself that no model can explain from these inputs. Residuals are corrected for the degrees of freedom of their group.
"""
import argparse
import csv
import glob
import json
import math
import os
from collections import defaultdict

import numpy as np

ap = argparse.ArgumentParser()
ap.add_argument("--samples", default="scripts/data/loaddata/sources/samples/by_maker")
ap.add_argument("--data", default="data")
ap.add_argument("--min-group", type=int, default=12)
args = ap.parse_args()

handgun = set()
for f in glob.glob(os.path.join(args.data, "calibers", "*.json")):
    r = json.load(open(f, encoding="utf-8"))
    if "/tab-iv/" in (r.get("cippdf") or "") or str(r.get("ciptype", "")).startswith("4"):
        handgun.add(r["cipname"])


def fnum(r, k):
    try:
        v = float(r.get(k) or 0)
    except ValueError:
        return 0.0
    return v


def huber_fit(X, y, k=1.345, iters=30):
    w = np.ones(len(y))
    for _ in range(iters):
        sw = np.sqrt(w)
        b = np.linalg.lstsq(X * sw[:, None], y * sw, rcond=None)[0]
        r = y - X @ b
        s = max(1.4826 * np.median(np.abs(r - np.median(r))), 1e-4)
        w = np.where(np.abs(r) <= k * s, 1.0, k * s / np.maximum(np.abs(r), 1e-12))
    return y - X @ b


MODELS = {"A": ["bullet_gr", "charge_g"], "B": ["bullet_gr", "charge_g", "barrel_mm", "col_mm"],
          "C": ["bullet_gr", "charge_g", "barrel_mm", "col_mm", "velocity_ms"]}
res = {m: defaultdict(list) for m in MODELS}  # model -> (source, class) -> residuals (relative, df corrected)
for path in sorted(glob.glob(os.path.join(args.samples, "*.csv"))):
    src = os.path.basename(path).split("_")[0]
    if src == "lovex2020":
        continue  # the same tables as lovex
    rows = list(csv.DictReader(open(path, encoding="utf-8")))
    if not rows or "bullet_gr" not in rows[0] or "pressure_bar" not in rows[0]:
        continue  # shotgun tables have other columns
    groups = defaultdict(list)
    for r in rows:
        if fnum(r, "pressure_bar") > 0 and fnum(r, "charge_g") > 0 and fnum(r, "bullet_gr") > 0:
            groups[(r["caliber"], r["powder"], r["kind"])].append(r)
    for (cal, _, _), g in groups.items():
        if len(g) < args.min_group:
            continue
        cls = "pistol" if cal in handgun else "rifle"
        y = np.log([fnum(r, "pressure_bar") for r in g])
        for m, cols in MODELS.items():
            feats = [[math.log(fnum(r, c)) if fnum(r, c) > 0 else math.nan for c in cols] for r in g]
            F = np.array(feats)
            ok = ~np.isnan(F).any(axis=1)
            F = F[:, [j for j in range(F.shape[1]) if np.ptp(F[ok][:, j]) > 1e-9]] if ok.any() else F  # a condition that never varies in the group
            X = np.column_stack([np.ones(len(g)), F])
            if ok.sum() < max(args.min_group, X.shape[1] + 4) or np.linalg.matrix_rank(X[ok]) < X.shape[1]:
                continue
            e = huber_fit(X[ok], y[ok])
            e *= math.sqrt(ok.sum() / (ok.sum() - X.shape[1]))
            res[m][(src, cls)].extend(e.tolist())


def line(label, values):
    v = np.array(values)
    if len(v) == 0:
        return f"{label:<26}{'-':>8}"
    robust = 1.4826 * np.median(np.abs(v))
    inside = v[np.abs(v) < 0.5]
    return (f"{label:<26}{len(v):>8}{100 * math.sqrt(np.mean(v ** 2)):>9.1f} %{100 * robust:>9.1f} %"
            f"{100 * math.sqrt(np.mean(inside ** 2)):>9.1f} %{100 * np.mean(np.abs(v) > 0.15):>9.0f} %")


for m, cols in MODELS.items():
    print(f"\nModel {m}: ln p ~ ln({'), ln('.join(cols)})\n{'':<26}{'loads':>8}{'rms':>11}{'MAD sd':>11}{'rms<50%':>11}{'>15% off':>11}")
    by_cls = defaultdict(list)
    for (src, cls), v in sorted(res[m].items()):
        print(line(f"{src} {cls}", v))
        by_cls[cls].extend(v)
    for cls, v in sorted(by_cls.items()):
        print(line(f"ALL {cls}", v))


# Level scatter: the maximum loads of a cartridge are built to reach its maximum pressure, so p / pmax of the maximum loads says how far the
# published pressures of different powders (and lots, barrels) are from one level.
print("\nMaximum loads: ln(p / pmax) of the published pressure over the cartridge's C.I.P./SAAMI maximum (one value per cartridge, powder, source)")
print(f"{'':<26}{'loads':>8}{'mean':>11}{'sd':>11}{'p10':>9}{'p90':>9}")
lv = defaultdict(list)
for path in sorted(glob.glob(os.path.join(args.samples, "*.csv"))):
    src = os.path.basename(path).split("_")[0]
    if src == "lovex2020":
        continue
    rows = list(csv.DictReader(open(path, encoding="utf-8")))
    if not rows or "pmax_bar" not in rows[0]:
        continue
    g = defaultdict(list)
    for r in rows:
        if r["kind"] == "max" and fnum(r, "pressure_bar") > 0 and fnum(r, "pmax_bar") > 0:
            g[(r["caliber"], r["powder"])].append(math.log(fnum(r, "pressure_bar") / fnum(r, "pmax_bar")))
    for (cal, _), v in g.items():
        lv[(src, "pistol" if cal in handgun else "rifle")].append(float(np.median(v)))
allc = defaultdict(list)
for (src, cls), v in sorted(lv.items()):
    a = np.array(v)
    print(f"{src + ' ' + cls:<26}{len(a):>8}{100 * a.mean():>9.1f} %{100 * a.std():>9.1f} %{100 * np.percentile(a, 10):>7.0f} %{100 * np.percentile(a, 90):>7.0f} %")
    allc[cls].extend(v)
for cls, v in sorted(allc.items()):
    a = np.array(v)
    print(f"{'ALL ' + cls:<26}{len(a):>8}{100 * a.mean():>9.1f} %{100 * a.std():>9.1f} %{100 * np.percentile(a, 10):>7.0f} %{100 * np.percentile(a, 90):>7.0f} %")
