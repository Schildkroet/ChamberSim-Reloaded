#!/usr/bin/env python3
"""Fits the shotshell constants of the solver (shotCapacityFactor, shotFrictionShare, shotEnergyFactor) to published
shotgun loads: the measured velocities, and the CIP maximum pressure as an upper limit of the maximum loads.
usage: fit_shotgun.py <grt_compare> <jobs dir with exp_sg_*_on.grtload and shotgun_meta.json> [pmax limit, default 1050]
(the jobs come from tools/GRTRef/make_jobs_shotgun.py; the published data stays local)."""
import json, os, re, subprocess, sys, numpy as np
from scipy.optimize import least_squares
EXE, JOBS = sys.argv[1], sys.argv[2]
LIMIT = float(sys.argv[3]) if len(sys.argv) > 3 else 1050.0
meta = {k: v for k, v in json.load(open(os.path.join(JOBS, "shotgun_meta.json"))).items() if v["shotshell"] == "on"}
names = ["shotCapacityFactor", "shotFrictionShare", "shotEnergyFactor"]

def run(x):
    args = [EXE, "--loads", JOBS] + sum((["--set", f"{n}={v:.8g}"] for n, v in zip(names, x)), [])
    out = {}
    for line in subprocess.run(args, capture_output=True, text=True).stdout.splitlines():
        m = re.match(r"exp_(\S+)\s+(\S+)\s+pmax\s+(\S+)\s+v0\s+(\S+)", line)
        if m: out[m.group(1)] = (m.group(2) == "ok", float(m.group(3)), float(m.group(4)))
    return out

def residuals(x):
    sim, r = run(x), []
    for k, m in meta.items():
        ok, p, v = sim.get(k, (False, 0, 0))
        r.append(v / m["v_ms"] - 1.0 if ok else 1.0)
        if m["which"] == "max": r.append(max(p / LIMIT - 1.0, 0.0) if ok else 1.0)  # above the CIP limit: penalty
    return np.array(r)

x0 = np.array([1.5, 0.2, 0.85])
sol = least_squares(residuals, x0, bounds=([1.0, 0.0, 0.5], [4.0, 0.8, 1.2]), diff_step=1e-3)
sim = run(sol.x)
ev = [100 * (sim[k][2] / m["v_ms"] - 1) for k, m in meta.items() if sim[k][0]]
pm = [sim[k][1] for k, m in meta.items() if sim[k][0] and m["which"] == "max"]
print("fitted:", " ".join(f"{n}={v:.4g}" for n, v in zip(names, sol.x)))
print("velocity error: mean %+.1f %%, mean |e| %.1f %%, max |e| %.1f %%   max-load pmax %.0f .. %.0f bar"
      % (np.mean(ev), np.mean(np.abs(ev)), np.max(np.abs(ev)), min(pm), max(pm)))
