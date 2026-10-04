#!/usr/bin/env python3
"""Least-squares calibration of the solver constants against the reference runs.
Usage: calibrate.py <path to grt_compare> [param ...]   (default: a standard parameter set)
Environment: GRT_EXCLUDE (regex of runs to skip; replaces the default exclusion of the stale runs), GRT_STEPS (solver steps, default 2000), GRT_START, GRT_FIXED (see below), GRT_ZWEIGHT (weight of the muzzle burnt-fraction error), GRT_OUT (result file, default calibrated.json).
The Jacobian is evaluated in parallel (one grt_compare process per parameter)."""
import os, subprocess, sys, json, numpy as np
from concurrent.futures import ThreadPoolExecutor
from scipy.optimize import least_squares

EXE = sys.argv[1]
names = sys.argv[2:] or ["impetusFactor", "kineticFactor", "covolume_cm3g", "covolumeDensityFactor", "intrusionFactor",
    "capacityFactor", "chamberOffsetLength_mm", "initialGasFactor", "burnIntercept", "burnSlopeA0", "burnSlopeOffset",
    "burnExponent", "phase2_3", "phase2_2", "phase2_1", "phase3_3", "phase3_2", "phase3_1",
    "ignitionRate", "ignitionLead"]
base = {}
for line in subprocess.run([EXE, "--list-params"], capture_output=True, text=True).stdout.splitlines():
    k, v = line.split(" = "); base[k] = float(v)
# GRT_START: json file with start values (e.g. an earlier result); GRT_FIXED: extra "name=value ..." that stay fixed
if "GRT_START" in os.environ: base.update(json.load(open(os.environ["GRT_START"])))
fixed = [t for t in os.environ.get("GRT_FIXED", "").split() if t]
for t in fixed: base[t.split("=")[0]] = float(t.split("=")[1])
x0 = np.array([base[n] for n in names])
scale = np.maximum(np.abs(x0), 0.05)

def run(x):
    args = [EXE, "--residuals"] + (["--steps", os.environ["GRT_STEPS"]] if "GRT_STEPS" in os.environ else []) + (["--zweight", os.environ["GRT_ZWEIGHT"]] if "GRT_ZWEIGHT" in os.environ else [])
    if "GRT_EXCLUDE" in os.environ: args += ["--exclude", os.environ["GRT_EXCLUDE"]]
    for t in fixed: args += ["--set", t]
    for n, v in zip(names, x): args += ["--set", f"{n}={v:.10g}"]
    out = subprocess.run(args, capture_output=True, text=True).stdout
    for line in out.splitlines():
        if line.startswith("RES "): return np.array([float(t) for t in line[4:].split()])
    raise RuntimeError(out)

calls = 0
def fun(x):
    global calls; calls += 1
    r = run(x)
    print("eval %3d  rms %.5f" % (calls, np.sqrt(np.mean(r**2))), flush=True)
    return r

pool = ThreadPoolExecutor(int(os.environ.get("GRT_JOBS", min(14, os.cpu_count() or 1))))
def jac(x):
    f0 = run(x); h = 2e-3 * scale
    cols = pool.map(lambda i: (run(x + h[i] * np.eye(len(x))[i]) - f0) / h[i], range(len(x)))
    return np.array(list(cols)).T

sol = least_squares(fun, x0, jac=jac, method="trf", x_scale=scale, max_nfev=25)
print("final rms %.5f after %d evals" % (np.sqrt(np.mean(sol.fun**2)), calls))
res = dict(zip(names, map(float, sol.x)))
json.dump(res, open(os.environ.get("GRT_OUT", "calibrated.json"), "w"), indent=1)
for n in names: print("  %-24s %12.6g   (was %.6g)" % (n, res[n], base[n]))
print("SET:", " ".join(f"--set {n}={v:.8g}" for n, v in res.items()))
