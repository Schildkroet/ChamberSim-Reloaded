#!/usr/bin/env python3
"""Flags published loads that cannot be right, independent of the solver (extraction errors, typos, unit mix-ups).

    check_published_loads.py [--samples DIR] [--out DIR] [--trend 0.15] [--apply-hard] [samples.csv ...]

Default input: the per-maker files of scripts/data/loaddata/sources/samples/by_maker (without the shotshell files). Checks:
  range      velocity, charge, bullet mass, cartridge length or barrel length outside anything a real load has (or missing)
  energy     muzzle energy per powder energy (Qex 4000 J/g, "efficiency") above 0.55, or far below that of a working load (rifle < 0.08, handgun < 0.03)
  pressure   published pressure above 1.25 x (handguns 2 x) the cartridge's permitted maximum (pmax_bar; rifle tables stay within 1.11 x), or below 50 bar
  order      start load above the maximum load of the same bullet and powder (charge or velocity)
  duplicate  the same load (powder, cartridge, bullet, cartridge length, barrel, kind, charge) with velocities more than 10 % apart
  trend      velocity more than --trend off the trend of its own table (sample_filter.py)
  density    (bullet length from the project's records only) the bullet's mean density above that of lead
Writes DIR/flagged.csv (every flagged row with the checks that fired) and a summary; with --apply-hard additionally DIR/<file> copies without the
loads of the checks range, energy, pressure and duplicate (the order, trend and density flags are softer and stay in the copies); a charge that is plausible
when read as grains instead of grams (the table gives grains, the extraction took grams) is converted instead of dropped, those rows are flagged "fixed".
"""
import argparse, collections, csv, glob, math, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from sample_filter import inconsistent  # noqa: E402

ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
ap.add_argument("samples", nargs="*")
ap.add_argument("--samples-dir", default=os.path.join(ROOT, "scripts/data/loaddata/sources/samples/by_maker"))
ap.add_argument("--out", default="")
ap.add_argument("--trend", type=float, default=0.15)
ap.add_argument("--apply-hard", action="store_true")
args = ap.parse_args()
files = args.samples or sorted(f for f in glob.glob(os.path.join(args.samples_dir, "*.csv")) if "shot" not in os.path.basename(f))
GRAIN = 0.06479891
QEX = 4000.0  # J/g, typical nitrocellulose powder

rows, header = [], []
for path in files:
    with open(path, newline="", encoding="utf-8") as f:
        rd = csv.DictReader(f)
        header = list(dict.fromkeys(header + list(rd.fieldnames)))
        for i, r in enumerate(rd):
            r["_file"], r["_line"] = os.path.basename(path), i + 2
            rows.append(r)


def num(r, k):
    try:
        v = float(r.get(k, ""))
        return v if math.isfinite(v) else None
    except ValueError:
        return None


flags = collections.defaultdict(list)  # row index -> [(check, detail)]
HARD = {"range", "energy", "pressure", "duplicate"}  # "order": the loads of such a pair simulate as well as any other (median error 2.8 % rifle / 5.5 % handgun), only the table is odd


def flag(i, check, detail):
    flags[i].append((check, detail))


for i, r in enumerate(rows):
    v, c, m, col, bar = (num(r, k) for k in ("velocity_ms", "charge_g", "bullet_gr", "col_mm", "barrel_mm"))
    if None in (v, c, m):
        flag(i, "range", "velocity, charge or bullet mass missing")
        continue
    if not 60 <= v <= 1750: flag(i, "range", f"velocity {v:g} m/s")
    if c <= 0: flag(i, "range", f"charge {c:g} g")
    if c > 12.0 and m < 500: flag(i, "range", f"charge {c:g} g with a {m:g} gr bullet (grains read as grams?)")
    if not 15 <= m <= 950: flag(i, "range", f"bullet {m:g} gr")
    if col is not None and not 15 <= col <= 150: flag(i, "range", f"cartridge length {col:g} mm")
    if bar is not None and not 20 <= bar <= 1200: flag(i, "range", f"barrel {bar:g} mm")
    e = 0.5 * m * GRAIN * 1e-3 * v * v  # J
    eff = e / (c * QEX)
    handgun = (num(r, "pmax_bar") or 9999) < 2800
    # measured over the tables: rifle 1 % / 99 % of the efficiencies at 0.17 / 0.40, handgun 0.03 / 0.45 (the tail up to 0.5 is the tables' own scatter)
    if eff > 0.55: flag(i, "energy", f"energy {e:.0f} J from {c:g} g = {100 * eff:.0f} % of the powder's energy")
    elif eff < (0.03 if handgun else 0.08) and v > 150: flag(i, "energy", f"energy {e:.0f} J from {c:g} g = {100 * eff:.1f} %")
    p, pm = num(r, "pressure_bar"), num(r, "pmax_bar")
    if p is not None:
        if p < 50: flag(i, "pressure", f"{p:g} bar")
        elif pm and p > (2.0 if pm < 2800 else 1.25) * pm: flag(i, "pressure", f"{p:g} bar over the permitted {pm:g} bar")  # hot handgun loads reach 1.9 x
    ln, dia = num(r, "bullet_len_mm"), num(r, "bullet_dia_mm")
    if ln and dia and str(r.get("len_source", "")).startswith("database"):
        dens = m * GRAIN / (math.pi / 4 * (dia / 10) ** 2 * (ln / 10))  # g/cm3 of the equivalent cylinder
        if dens > 11.6: flag(i, "density", f"mean bullet density {dens:.1f} g/cm3")

# order: start above max within one bullet / powder / cartridge / cartridge length
groups = collections.defaultdict(lambda: {"start": [], "max": []})
for i, r in enumerate(rows):
    if r.get("kind") in ("start", "max") and None not in (num(r, "charge_g"), num(r, "velocity_ms")):
        groups[(r["powder"], r["caliber"], r["bullet"], r["bullet_gr"], r.get("col_mm"), r.get("barrel_mm"), r.get("primer", ""))][r["kind"]].append(i)
for g in groups.values():
    for s in g["start"]:
        for mx in g["max"]:
            cs, cm, vs, vm = (num(rows[s], "charge_g"), num(rows[mx], "charge_g"), num(rows[s], "velocity_ms"), num(rows[mx], "velocity_ms"))
            if cs > cm * 1.001: flag(s, "order", f"start charge {cs:g} g > max {cm:g} g"); flag(mx, "order", f"max charge {cm:g} g < start {cs:g} g")
            elif vs > vm * 1.03: flag(s, "order", f"start velocity {vs:g} > max {vm:g} m/s"); flag(mx, "order", f"max velocity {vm:g} < start {vs:g} m/s")

# duplicates with conflicting velocities
seen = collections.defaultdict(list)
for i, r in enumerate(rows):
    if num(r, "velocity_ms") is not None:
        seen[(r["powder"], r["caliber"], r["bullet"], r["bullet_gr"], r.get("col_mm"), r.get("barrel_mm"), r["kind"], r["charge_g"])].append(i)
for idx in seen.values():
    vs = [num(rows[i], "velocity_ms") for i in idx]
    if len(idx) > 1 and max(vs) > 1.10 * min(vs):
        for i in idx: flag(i, "duplicate", f"velocities {min(vs):g} .. {max(vs):g} m/s for the same load")

# trend of the own table (needs caliber, powder, kind, bullet_gr, charge_g, velocity_ms)
usable = [i for i, r in enumerate(rows) if None not in (num(r, "bullet_gr"), num(r, "charge_g"), num(r, "velocity_ms")) and num(r, "charge_g") > 0 and num(r, "bullet_gr") > 0 and num(r, "velocity_ms") > 0]
sub = [{k: rows[i][k] for k in ("caliber", "powder", "kind", "bullet_gr", "charge_g", "velocity_ms")} for i in usable]
for j in inconsistent(sub, threshold=args.trend):
    flag(usable[j], "trend", f"velocity off the trend of its table by > {100 * args.trend:g} %")

# repair: a charge given in grains and read as grams (energy or charge flag) becomes plausible when divided by 15.4324
fixed = {}
for i, fl in flags.items():
    kinds = {c for c, d in fl if c in ("energy", "range") and ("charge" in d or "energy" in d)}
    r = rows[i]
    v, c, m = num(r, "velocity_ms"), num(r, "charge_g"), num(r, "bullet_gr")
    if not kinds or None in (v, c, m) or any(c2 == "range" and "charge" not in d and "energy" not in d for c2, d in fl): continue
    handgun = (num(r, "pmax_bar") or 9999) < 2800
    eff = 0.5 * m * GRAIN * 1e-3 * v * v / (c / 15.4324 * QEX)
    if (0.10 if handgun else 0.12) <= eff <= 0.45: fixed[i] = c / 15.4324
for i in fixed: flags[i].append(("fixed", f"charge {rows[i]['charge_g']} read as grains: {fixed[i]:.4f} g"))

count = collections.Counter(chk for fl in flags.values() for chk in {c for c, _ in fl})
print(f"{len(rows)} loads in {len(files)} files; {len(flags)} flagged ({100 * len(flags) / len(rows):.1f} %)")
for chk, n in count.most_common():
    print(f"  {chk:10s} {n:6d}  {'(hard)' if chk in HARD else '(soft)'}")
hard = {i for i, fl in flags.items() if any(c in HARD for c, _ in fl) and i not in fixed}
print(f"  rows to drop (hard flag, not repairable): {len(hard)}; charge repaired (grains read as grams): {len(fixed)}")
by_file = collections.Counter(rows[i]["_file"] for i in hard)
print("  hard flags per file:", dict(by_file.most_common()))
if args.out:
    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "flagged.csv"), "w", newline="", encoding="utf-8") as f:
        cols = ["checks", "detail", "file", "line"] + [h for h in header if not h.startswith("_")]
        w = csv.DictWriter(f, cols, extrasaction="ignore")
        w.writeheader()
        for i in sorted(flags, key=lambda i: (-len({c for c, _ in flags[i]} & HARD), rows[i]["_file"], rows[i]["_line"])):
            w.writerow(dict(rows[i], checks="+".join(sorted({c for c, _ in flags[i]})), detail="; ".join(d for _, d in flags[i]), file=rows[i]["_file"], line=rows[i]["_line"]))
    if args.apply_hard:
        for path in files:
            name = os.path.basename(path)
            keep = [dict(r, charge_g=round(fixed[i], 4)) if i in fixed else r for i, r in enumerate(rows) if r["_file"] == name and i not in hard]
            with open(os.path.join(args.out, name), "w", newline="", encoding="utf-8") as f:
                cols = [h for h in header if not h.startswith("_")]
                w = csv.DictWriter(f, cols, extrasaction="ignore", restval="")
                w.writeheader()
                w.writerows(keep)
