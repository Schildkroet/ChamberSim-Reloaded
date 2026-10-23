#!/usr/bin/env python3
"""Refits Ba, a0, z1, z2 of powders without published pressures from several start shapes and keeps the best (lowest fit cost).

    fit_physical_multistart.py <csr_eval> samples.csv [samples2.csv ...] --powders "Maker Name,..." [--start FILE] [--out FILE] [--shapes 6]
                               [--limit-margin 50] [--min-gain 0.01] [--work DIR] [--keep "Maker Name,..."]

Powders without measured pressures (Vihtavuori, Alliant, Vectan, RWS, ...) get the maximum loads at the C.I.P. limit minus --limit-margin as their pressure
(fit_physical_powders.py --limit-pressure), which determines the form function only weakly, so a fit from one start value ends in a local optimum (N120: the
458 SOCOM pressure was 72 % too high). The fit is therefore run from --shapes + 2 starts: the current values, the form functions of the --shapes typical
(k-medoid) pressure-fitted powders of --start (the powders with measured pressures, form "fitted" or "kept") and the shape of Hodgdon H4198. Ba stays the
current one at the start. For every powder the start with the lowest cost wins when its cost is at least --min-gain (1 %) below that of the current values;
otherwise the powder keeps its values. --keep: powders that are never changed (grain geometry: R907, Tubal 3000 / 5000, N570, N110).
--out is a copy of --start with the changed powders replaced. A table of the costs per start is printed and written to --work/costs.csv."""
import argparse, csv, json, os, re, subprocess, sys
import numpy as np

ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
ap.add_argument("eval_exe"); ap.add_argument("samples", nargs="+")
ap.add_argument("--powders", required=True, help="comma separated \"maker name\" (lower case ok): the powders to refit")
ap.add_argument("--start", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "physical", "propellants.json"))
ap.add_argument("--out", required=True)
ap.add_argument("--shapes", type=int, default=6)
ap.add_argument("--limit-margin", type=float, default=50.0)
ap.add_argument("--min-gain", type=float, default=0.01)
ap.add_argument("--min-samples", type=int, default=20)
ap.add_argument("--work", default="multistart")
ap.add_argument("--keep", default="")
args = ap.parse_args()
HERE = os.path.dirname(os.path.abspath(__file__))
os.makedirs(args.work, exist_ok=True)
base = json.load(open(args.start))
key = lambda p: (p["mname"] + " " + p["pname"]).lower()
by = {key(p): p for p in base["propellants"]}
targets = [p.strip().lower() for p in args.powders.split(",") if p.strip()]
keep = {p.strip().lower() for p in args.keep.split(",") if p.strip()}
targets = [p for p in targets if p in by and p not in keep]
# typical shapes: k-medoids of the form functions (a0, z1, z2) of the powders fitted with pressures
pool = [p for k, p in by.items() if k not in targets and p.get("form") in ("fitted", "kept") and p.get("fit", {}).get("p_n", 0) >= 20]
pts = np.array([[p["a0"], p["z1"], p["z2"]] for p in pool]); scale = pts.std(axis=0) + 1e-9
z = pts / scale
dist = np.linalg.norm(z[:, None] - z[None], axis=2)
med = list(np.argsort(dist.sum(axis=1))[:args.shapes])  # start: the most central
for _ in range(50):
    labels = np.argmin(dist[:, med], axis=1)
    new = [int(np.nonzero(labels == k)[0][np.argmin(dist[np.ix_(labels == k, labels == k)].sum(axis=1))]) if (labels == k).any() else med[k] for k in range(len(med))]
    if new == med: break
    med = new
shapes = {f"medoid{k + 1}": pts[i] for k, i in enumerate(med)}
if "hodgdon h4198" in by: shapes["h4198"] = np.array([by["hodgdon h4198"][c] for c in ("a0", "z1", "z2")])
print("start shapes (a0, z1, z2):", {k: [round(float(x), 3) for x in v] for k, v in shapes.items()}, flush=True)
runs = {"current": args.start}
for name, shape in shapes.items():
    d = json.loads(json.dumps(base))
    for p in d["propellants"]:
        if key(p) in targets: p["a0"], p["z1"], p["z2"] = (float(x) for x in shape)
    runs[name] = os.path.join(args.work, f"start_{name}.json"); json.dump(d, open(runs[name], "w"))
costs, results = {}, {}
for name, start in runs.items():
    out = os.path.join(args.work, f"fit_{name}.json")
    cmd = [sys.executable, os.path.join(HERE, "fit_physical_powders.py"), args.eval_exe, *args.samples, "--fit", "form", "--start", start, "--out", out,
           "--limit-pressure", "--limit-margin", str(args.limit_margin), "--min-samples", str(args.min_samples), "--only", ",".join(targets)]
    log = subprocess.run(cmd, capture_output=True, text=True).stdout
    open(os.path.join(args.work, f"fit_{name}.log"), "w").write(log)
    for m in re.finditer(r"^cost (.+?): ([\d.e+-]+)$", log, re.M): costs.setdefault(m[1].lower(), {})[name] = float(m[2])
    results[name] = {key(p): p for p in json.load(open(out))["propellants"]} if os.path.exists(out) else {}
    print(f"start {name}: {len(results[name])} powders fitted", flush=True)
changed = 0
with open(os.path.join(args.work, "costs.csv"), "w", newline="") as f:
    w = csv.writer(f); w.writerow(["powder", *runs, "best", "adopted"])
    for p in targets:
        c = costs.get(p, {})
        if "current" not in c: w.writerow([p, *[c.get(r, "") for r in runs], "", "no fit"]); continue
        best = min(c, key=c.get)
        adopt = best != "current" and c[best] < (1 - args.min_gain) * c["current"] and p in results[best]
        w.writerow([p, *[round(c[r], 4) if r in c else "" for r in runs], best, "yes" if adopt else "no"])
        print(f"{p:28s} " + " ".join(f"{c.get(r, float('nan')):8.4f}" for r in runs) + f"  best {best}" + ("  -> adopted" if adopt else ""), flush=True)
        if adopt:
            new = results[best][p]
            old = by[p]
            old.update({k: new[k] for k in ("Ba", "a0", "z1", "z2", "fit")}); old["form"] = "fitted"
            changed += 1
base["propellants"] = list(by.values())
json.dump(base, open(args.out, "w"), indent=1)
print(f"{changed} of {len(targets)} powders changed -> {args.out}")
