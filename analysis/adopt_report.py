#!/usr/bin/env python3
"""Which refitted powders are worth adopting: per powder, the shipped burn values against a candidate file, on the loads of the powder itself.

    adopt_report.py <dump.csv> <shipped propellants.json> <candidate.json> [--out adopted.json] [--min-loads 20] [--min-gain 0.1]

<dump.csv> is made by compare_physical_published.py --dump (shipped build as the baseline, the candidate file as --eval-arg=--physical=...). A powder is adopted when
its velocity rms over its own loads falls by at least --min-gain points and its pressure rms (loads with a published pressure) does not rise by more than
--max-pressure-loss points. --out writes the shipped file with the adopted entries replaced (data/ itself is not changed). The measured loads (data/measurements: 458 SOCOM N120,
DEVA N340) are the last check and are judged by hand with refit.sh's output: the adoption rule of 2026-10-23.
"""
import argparse, csv, json, math, collections

ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
ap.add_argument("dump"); ap.add_argument("shipped"); ap.add_argument("candidate")
ap.add_argument("--out", default="")
ap.add_argument("--min-loads", type=int, default=20)
ap.add_argument("--min-gain", type=float, default=0.1)
ap.add_argument("--max-pressure-loss", type=float, default=0.0)
args = ap.parse_args()


def f(x):
    try:
        v = float(x)
        return v if math.isfinite(v) else None
    except (TypeError, ValueError):
        return None


ship, cand = json.load(open(args.shipped)), json.load(open(args.candidate))
key = lambda p: (p["mname"] + " " + p["pname"]).strip()
by_key = {key(p): p for p in ship["propellants"]}
by_name = collections.defaultdict(list)
for k in by_key:
    by_name[k.split(" ", 1)[-1].upper()].append(k)
err = collections.defaultdict(lambda: collections.defaultdict(list))
for r in csv.DictReader(open(args.dump)):
    k = r["powder"] if r["powder"] in by_key else (by_name.get(r["powder"].upper(), [None]) if len(by_name.get(r["powder"].upper(), [])) != 1 else by_name[r["powder"].upper()])[0]
    if k is None:
        continue
    pv, pp = f(r["pub_v"]), f(r["pub_p"])
    for run in ("baseline", "current"):
        v, p = f(r[run + "_v"]), f(r[run + "_p"])
        if v is not None and pv: err[k][run + "_v"].append((v / pv - 1) * 100)
        if p is not None and pp: err[k][run + "_p"].append((p / pp - 1) * 100)
rms = lambda a: math.sqrt(sum(x * x for x in a) / len(a)) if a else float("nan")
rows, adopt = [], []
for k, d in err.items():
    n = len(d["baseline_v"])
    if n < args.min_loads:
        continue
    bv, cv, bp, cp = rms(d["baseline_v"]), rms(d["current_v"]), rms(d["baseline_p"]), rms(d["current_p"])
    ok = bv - cv >= args.min_gain and (math.isnan(bp) or cp <= bp + args.max_pressure_loss)
    rows.append((k, n, len(d["baseline_p"]), bv, cv, bp, cp, ok))
    if ok: adopt.append(k)
rows.sort(key=lambda r: r[4] - r[3])
print(f"{len(rows)} powders with >= {args.min_loads} loads; adopt {len(adopt)} (velocity rms falls by >= {args.min_gain} point, pressure rms not worse); "
      f"{sum(1 for r in rows if r[4] - r[3] >= args.min_gain)} get worse by that much")
print(f"{'powder':36} {'loads':>6} {'p-loads':>7} | {'v shipped':>9} {'v cand.':>8} | {'p shipped':>9} {'p cand.':>8} | adopt")
for r in rows:
    if r[7] or r[4] - r[3] >= args.min_gain or r[3] - r[4] >= 1.0:
        print(f"{r[0]:36} {r[1]:6d} {r[2]:7d} | {r[3]:9.2f} {r[4]:8.2f} | {r[5]:9.1f} {r[6]:8.1f} | {'yes' if r[7] else ''}")
if args.out:
    cmap = {key(p): p for p in cand["propellants"]}
    out = dict(ship)
    out["propellants"] = [cmap[key(p)] if key(p) in adopt else p for p in ship["propellants"]]
    json.dump(out, open(args.out, "w"), indent=1)
    print(f"{len(adopt)} powders adopted -> {args.out}")
