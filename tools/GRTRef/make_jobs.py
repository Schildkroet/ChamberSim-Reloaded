#!/usr/bin/env python3
"""Generate controlled-experiment .grtload files (one input changed per file)
from a base load. Output: jobs/exp_<name>.grtload"""
import re, os, sys, urllib.parse

BASE = os.path.expanduser("~/Downloads/GordonsReloadingTool-2021.2040-NIGHTLY-linux/loads/8x57is.grtload")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "jobs")

def setv(s, section, name, value):
    """Set <input name=NAME value=...> inside <section>...</section>."""
    m = re.search(rf"<{section}>.*?</{section}>", s, re.S)
    blk = m.group(0)
    new, n = re.subn(rf'(<input name="{name}" value=")[^"]*(")', rf"\g<1>{value}\g<2>", blk)
    assert n == 1, (section, name, n)
    return s[:m.start()] + new + s[m.end():]

def getv(s, section, name):
    blk = re.search(rf"<{section}>.*?</{section}>", s, re.S).group(0)
    return re.search(rf'<input name="{name}" value="([^"]*)"', blk).group(1)

base = open(BASE, encoding="utf-8").read()
mc, mp = float(getv(base, "propellant", "mc")), float(getv(base, "projectile", "mp"))
casevol, xe = float(getv(base, "caliber", "casevol")), float(getv(base, "gun", "xe"))

# name -> list of (section, key, value)
E = {"base": []}
for f in (0.6, 0.8, 1.2):                      E[f"mc_x{f}"] = [("propellant", "mc", mc * f)]
for f in (0.5, 2.0, 4.0):                      E[f"mp_x{f}"] = [("projectile", "mp", mp * f)]
for s_ in (0.0, 0.25, 1.0):                    E[f"sebert_{s_}"] = [("caliber", "sebert", s_)]
for ps in (50, 100, 500):                      E[f"ps_{ps}"] = [("caliber", "ps", ps), ("projectile", "gpressure", ps)]
for f in (0.8, 1.25):                          E[f"casevol_x{f}"] = [("caliber", "casevol", casevol * f)]
for L in (100, 200, 400, 1000):                E[f"xe_{L}"] = [("gun", "xe", L)]
for tw in (0, 120, 480):                       E[f"twist_{tw}"] = [("gun", "twistlen", tw)]
for fr in (50, 200):                           E[f"friction_{fr}"] = [("projectile", "gfriction", fr), ("projectile", "gfrictionUnlock", "true")]
E["friction_0unlock"] = [("projectile", "gfriction", 0), ("projectile", "gfrictionUnlock", "true")]
for pt in (-20, 0, 40, 60):                    E[f"pt_{pt}"] = [("propellant", "pt", pt)]

os.makedirs(OUT, exist_ok=True)
for name, edits in E.items():
    s = base
    for sec, key, val in edits:
        s = setv(s, sec, key, repr(val) if isinstance(val, float) else val)
    open(os.path.join(OUT, f"exp_{name}.grtload"), "w", encoding="utf-8").write(s)
print(len(E), "files written to", OUT)
