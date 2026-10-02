#!/usr/bin/env python3
"""Compares simulated muzzle velocities with the PUBLISHED ones (jobs of make_jobs_vv.py).
usage: compare_vv.py <vv_meta.json> <grt_compare --loads output> [<results dir of the original>]
Prints the error of GRT Reloaded and, if the original's results are given, of the original, per cartridge,
for the start and maximum charges, and over the simulated peak pressure."""
import json, os, re, sys, statistics as st
meta = json.load(open(sys.argv[1]))
ours = {}
for line in open(sys.argv[2]):
    m = re.match(r"exp_(\S+)\s+(\S+)\s+pmax\s+(\S+)\s+v0\s+(\S+)", line)
    if m and m.group(2) == "ok": ours[m.group(1)] = (float(m.group(3)), float(m.group(4)))
orig = {}
if len(sys.argv) > 3:
    for name in meta:
        f = os.path.join(sys.argv[3], f"exp_{name}.json")
        if not os.path.exists(f): continue
        d = json.load(open(f))
        v = re.match(r"[\d.]+", d.get("summary", {}).get("MuzzleVelocity", "") or "")
        if d.get("points") and v and float(v.group(0)) > 0: orig[name] = float(v.group(0))

def line(label, names):
    def stat(vals):
        if not vals: return "        -           "
        return "%+5.1f %% (|e| %4.1f %%)" % (st.mean(vals), st.mean(abs(x) for x in vals))
    both = [n for n in names if n in ours and (not orig or n in orig)]
    e1 = [100 * (ours[n][1] / meta[n]["v_ms"] - 1) for n in both]
    e2 = [100 * (orig[n] / meta[n]["v_ms"] - 1) for n in both if n in orig]
    print("  %-30s n=%4d   GRT Reloaded %s   original %s" % (label, len(both), stat(e1), stat(e2) if orig else "(no results)"))

names = list(meta)
line("all", names)
for w in ("start", "max"): line(w + " charges", [n for n in names if meta[n]["which"] == w])
for c in sorted({m["cartridge"] for m in meta.values()}): line(c[:30], [n for n in names if meta[n]["cartridge"] == c])
print(" by simulated peak pressure (GRT Reloaded):")
for lo, hi in ((0, 1000), (1000, 1800), (1800, 2800), (2800, 4000), (4000, 9000)):
    line("%d-%d bar" % (lo, hi), [n for n in names if n in ours and lo <= ours[n][0] < hi])
