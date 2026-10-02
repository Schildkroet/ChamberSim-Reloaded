#!/usr/bin/env python3
"""Fits the shotshell constants of the solver (shotCapacityFactor, shotFrictionShare, shotEnergyFactor) to published
shotgun loads.
usage: fit_shotgun.py <grt_compare> <jobs dir with exp_*.grtload and shotgun_meta.json>
(jobs: tools/GRTRef/make_jobs_shotgun.py and own additions; the published data stays local)

Per load the meta data gives the published velocity v_ms measured distance_m in front of the muzzle with pellets of
pellet_mm (material lead / steel), optionally a measured maximum pressure p_bar and an upper limit limit_bar (CIP
maximum for the guide's maximum loads). The pellets slow down in the air before the light gates: the measured
velocity is converted to the muzzle with v0 = v * exp(k * d), k = rho_air * cd * area / (2 * mass) of one pellet."""
import json, math, os, re, subprocess, sys, numpy as np
from scipy.optimize import least_squares
EXE, JOBS = sys.argv[1], sys.argv[2]
meta = json.load(open(os.path.join(JOBS, "shotgun_meta.json")))
names = ["shotCapacityFactor", "shotFrictionShare", "shotEnergyFactor"]
DENSITY = {"lead": 11340.0, "steel": 7870.0}  # kg/m3

def muzzle_velocity(m):
    r = m["pellet_mm"] / 2000.0
    k = 1.2 * 0.47 * math.pi * r * r / (2.0 * DENSITY[m["material"]] * 4.0 / 3.0 * math.pi * r ** 3)
    return m["v_ms"] * math.exp(k * m["distance_m"])

def run(x):
    args = [EXE, "--loads", JOBS] + sum((["--set", f"{n}={v:.8g}"] for n, v in zip(names, x)), [])
    out = {}
    for line in subprocess.run(args, capture_output=True, text=True).stdout.splitlines():
        m = re.match(r"exp_(\S+)\s+(\S+)\s+pmax\s+(\S+)\s+v0\s+(\S+)", line)
        if m: out[m.group(1)] = (m.group(2) == "ok", float(m.group(3)), float(m.group(4)))
    return out

def residuals(x, weight_p=1.0):
    sim, r = run(x), []
    for k, m in meta.items():
        ok, p, v = sim.get(k, (False, 0, 0))
        r.append(v / muzzle_velocity(m) - 1.0 if ok else 1.0)
        if m.get("p_bar"): r.append(weight_p * (p / m["p_bar"] - 1.0) if ok else 1.0)
        if m.get("limit_bar"): r.append(max(p / m["limit_bar"] - 1.0, 0.0) if ok else 1.0)
    return np.array(r)

# the gas energy cannot exceed what the powder delivers: factor <= 1 (above it the fit trades it against the volume)
sol = least_squares(residuals, np.array([1.5, 0.1, 0.9]), bounds=([0.5, 0.0, 0.5], [4.0, 0.8, 1.0]), diff_step=1e-3)
sim = run(sol.x)
print("fitted:", " ".join(f"{n}={v:.4g}" for n, v in zip(names, sol.x)))
ev = [100 * (sim[k][2] / muzzle_velocity(m) - 1) for k, m in meta.items() if sim[k][0]]
print("muzzle velocity error: mean %+.1f %%, mean |e| %.1f %%, max |e| %.1f %%" % (np.mean(ev), np.mean(np.abs(ev)), np.max(np.abs(ev))))
for k, m in meta.items():
    if m.get("p_bar"):
        print("  %-6s pressure %4.0f bar (measured %4.0f), muzzle velocity %4.0f m/s (measured %4.0f at %.1f m = %4.0f at the muzzle)"
              % (k, sim[k][1], m["p_bar"], sim[k][2], m["v_ms"], m["distance_m"], muzzle_velocity(m)))
pm = [sim[k][1] for k, m in meta.items() if sim[k][0] and m.get("limit_bar")]
print("guide maximum loads: pmax %.0f .. %.0f bar" % (min(pm), max(pm)))
