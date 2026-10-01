#!/usr/bin/env python3
"""Least-squares calibration of the solver constants against the reference runs.
Usage: calibrate.py <path to grt_compare> [param ...]   (default: a standard parameter set)"""
import os, subprocess, sys, json, numpy as np
from scipy.optimize import least_squares

EXE = sys.argv[1]
names = sys.argv[2:] or ["impetusFactor", "kineticFactor", "covolume_cm3g", "intrusionFactor", "chamberOffset_cm3",
    "hotCoeffDefault", "coldCoeffDefault", "burnIntercept", "burnSlopeA0", "burnSlopeOffset", "burnExponent",
    "phase2_3", "phase2_2", "phase2_1", "phase2_0", "phase3_3", "phase3_2", "phase3_1", "phase3_0"]
base = {}
for line in subprocess.run([EXE, "--list-params"], capture_output=True, text=True).stdout.splitlines():
    k, v = line.split(" = "); base[k] = float(v)
x0 = np.array([base[n] for n in names])
extra = [a for a in []]

def run(x):
    args = [EXE, "--residuals"] + (["--zweight", os.environ["GRT_ZWEIGHT"]] if "GRT_ZWEIGHT" in os.environ else [])
    for n, v in zip(names, x): args += ["--set", f"{n}={v:.10g}"]
    out = subprocess.run(args, capture_output=True, text=True).stdout
    for line in out.splitlines():
        if line.startswith("RES "): return np.array([float(t) for t in line[4:].split()])
    raise RuntimeError(out)

calls = 0
def fun(x):
    global calls; calls += 1
    r = run(x)
    if calls % 40 == 1: print("eval %4d  rms %.4f" % (calls, np.sqrt(np.mean(r**2))), flush=True)
    return r

r0 = fun(x0); print("start rms %.4f (%d residuals)" % (np.sqrt(np.mean(r0**2)), len(r0)), flush=True)
sol = least_squares(fun, x0, method="trf", diff_step=2e-3, x_scale=np.maximum(np.abs(x0), 0.05), max_nfev=40, verbose=0)
print("final rms %.4f after %d evals" % (np.sqrt(np.mean(sol.fun**2)), calls))
res = dict(zip(names, map(float, sol.x)))
json.dump(res, open("calibrated.json", "w"), indent=1)
for n in names: print("  %-20s %12.6g   (was %.6g)" % (n, res[n], base[n]))
print("SET:", " ".join(f"--set {n}={v:.8g}" for n, v in res.items()))
