#!/usr/bin/env python3
"""Fits an effective grain geometry to the physical model's form function.

    fit_grain_geometry.py <csr_eval> [--file data/physical/propellants.json] [--write] [--threshold 0.08]

For every powder in the physical burn file, the form function phi(z) of the current physical calibration (a0, z1, z2 and the
calibration's phase polynomials) is the target. Two geometric surfaces are fitted:

  * ball:    phi(z) = amplitude * (1 - z)^(2/3)
  * cylinder: a single-perforated cylinder burning on both ends; length/diameter `ld` and perforation/outer radius `hole`
              are searched, the burnt fraction is the volume fraction, and the surface area is normalized to its initial value.

The solver uses the geometric surface when it is written to the file; otherwise it keeps the cubic form function. A powder gets
a geometry only when its rms difference from the cubic curve is below `--threshold` (default 0.08 in phi units), so the geometric
model never replaces a curve it cannot represent. The amplitude is the effective initial surface ratio and is fitted here.

The script is a developer tool: it needs the release `csr_eval` and numpy (no scipy).
"""
import argparse
import json
import subprocess

import numpy as np

ap = argparse.ArgumentParser()
ap.add_argument("eval_exe")
ap.add_argument("--file", default="data/physical/propellants.json")
ap.add_argument("--write", action="store_true")
ap.add_argument("--threshold", type=float, default=0.08)
ap.add_argument("--ld-min", type=float, default=0.05)
ap.add_argument("--ld-max", type=float, default=20.0)
ap.add_argument("--ld-steps", type=int, default=60)
ap.add_argument("--hole-steps", type=int, default=31)
args = ap.parse_args()

# The physical calibration of the running solver (phase polynomials are exposed component-wise by --list-params).
cal = {}
out = subprocess.run([args.eval_exe, "--list-params", "--profile", "physical", "x"], capture_output=True, text=True).stdout
for line in out.splitlines():
    parts = line.split()
    if len(parts) == 2:
        try:
            cal[parts[0]] = float(parts[1])
        except ValueError:
            pass
cal["phase2"] = [cal["phase2_3"], cal["phase2_2"], cal["phase2_1"]]
cal["phase3"] = [cal["phase3_3"], cal["phase3_2"], cal["phase3_1"]]

ZS = np.linspace(0.0, 0.98, 99)


def poly(c, c0, x):
    return ((c[0] * x + c[1]) * x + c[2]) * x + c0


def phi_cubic(z, a0, z1, z2):
    floor = 0.02
    def phase1(x):
        return cal["burnIntercept"] + (cal["burnSlopeA0"] * a0 + cal["burnSlopeOffset"]) * x
    if z < z1:
        return max(phase1(z), floor)
    phi1 = max(phase1(z1), floor)
    if z < z2:
        return max(phi1 * poly(cal["phase2"], 1.0, (z - z1) / max(z2 - z1, 1e-9)), floor)
    at_z2 = poly(cal["phase2"], 1.0, 1.0)
    return max(phi1 * poly(cal["phase3"], at_z2, min((z - z2) / max(1.0 - z2, 1e-9), cal["phase3Limit"])), floor)


def cylinder_surface(ld, hole, n=257):
    """Normalized surface S(z)/S(0) of a single-perforated cylinder on a uniform z grid (amplitude 1)."""
    r0 = float(np.clip(hole, 0.0, 0.95))
    length = max(ld, 0.05) * 2.0
    e = (1.0 - r0) / 2.0
    w_max = min(e, 0.5 * length)
    v0 = np.pi * (1.0 - r0 * r0) * length
    s0 = 2.0 * np.pi * (1.0 + r0) * length + 2.0 * np.pi * (1.0 - r0 * r0)
    if v0 <= 0.0 or s0 <= 0.0:
        return None
    w = w_max * np.arange(n) / (n - 1)
    outer = 1.0 - w
    inner = r0 + w
    v = np.pi * (outer * outer - inner * inner) * (length - 2.0 * w)
    s = 2.0 * np.pi * (outer + inner) * (length - 2.0 * w) + 2.0 * np.pi * (outer * outer - inner * inner)
    z = np.clip(1.0 - np.maximum(v, 0.0) / v0, 0.0, 1.0)
    z[0], z[-1] = 0.0, 1.0
    phi = np.maximum(s, 0.0) / s0
    return np.interp(ZS, z, phi)


def fit_amplitude(surface, target):
    denom = float(np.dot(surface, surface))
    if denom <= 0.0:
        return 1.0, 1e9
    amp = float(np.dot(surface, target) / denom)
    rms = float(np.sqrt(np.mean((amp * surface - target) ** 2)))
    return amp, rms


with open(args.file) as f:
    doc = json.load(f)

ld_values = np.geomspace(args.ld_min, args.ld_max, args.ld_steps)
hole_values = np.linspace(0.0, 0.9, args.hole_steps)
surface_cache = {}
rows = []
for entry in doc["propellants"]:
    target = np.array([phi_cubic(z, entry["a0"], entry["z1"], entry["z2"]) for z in ZS])
    ball = np.maximum(1.0 - ZS, 0.0) ** (2.0 / 3.0)
    best = ("ball", 0.0, 0.0, *fit_amplitude(ball, target))
    for ld in ld_values:
        for hole in hole_values:
            key = (round(float(ld), 6), round(float(hole), 6))
            surf = surface_cache.get(key)
            if surf is None:
                surf = cylinder_surface(float(ld), float(hole))
                surface_cache[key] = surf
            if surf is None:
                continue
            amp, rms = fit_amplitude(surf, target)
            if rms < best[4]:
                best = ("cylinder", float(ld), float(hole), amp, rms)
    shape, ld, hole, amp, rms = best
    use = shape != "fitted" and rms <= args.threshold
    rows.append((entry["mname"], entry["pname"], shape, ld, hole, amp, rms, use))
    if use:
        entry["grain"] = {"shape": shape, "ld": ld if shape == "cylinder" else 0.0,
                          "hole": hole if shape == "cylinder" else 0.0, "amplitude": amp, "rms": rms}
    else:
        entry.pop("grain", None)

used = [r for r in rows if r[7]]
print(f"{len(used)} of {len(rows)} powders use a geometric surface (threshold {args.threshold})")
for mname, pname, shape, ld, hole, amp, rms, use in sorted(rows, key=lambda r: r[6])[:20]:
    print(f"  {mname} {pname:<14} {shape:<8} rms {rms:.4f}  ld {ld:.3f}  hole {hole:.3f}  amp {amp:.3f}  {'USE' if use else ''}")
print("worst:")
for mname, pname, shape, ld, hole, amp, rms, use in sorted(rows, key=lambda r: r[6], reverse=True)[:10]:
    print(f"  {mname} {pname:<14} {shape:<8} rms {rms:.4f}  {'USE' if use else ''}")

if args.write:
    with open(args.file, "w") as f:
        json.dump(doc, f, indent=1)
        f.write("\n")
    print("wrote", args.file)
