#!/usr/bin/env python3
"""(batch 2: near-closed vessel) Generate controlled-experiment .grtload files (one input changed per file)
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

# near-closed-vessel batch: very heavy bullet -> (almost) no motion
HEAVY = 200.0
E = {}
for f in (0.3, 0.5, 0.7, 1.0, 1.3, 1.6, 2.0):
    E[f"cv_mp200_mc{f}"] = [("projectile", "mp", mp * HEAVY), ("propellant", "mc", mc * f)]
for f in (0.7, 1.5):
    E[f"cv_mp200_casevol{f}"] = [("projectile", "mp", mp * HEAVY), ("caliber", "casevol", casevol * f)]
for ps in (50, 500):
    E[f"cv_mp200_ps{ps}"] = [("projectile", "mp", mp * HEAVY), ("caliber", "ps", ps), ("projectile", "gpressure", ps)]
E["cv_mp1000_mc1.0"] = [("projectile", "mp", mp * 1000.0)]
E["cv_mp50_mc1.0"] = [("projectile", "mp", mp * 50.0)]

os.makedirs(OUT, exist_ok=True)
for name, edits in E.items():
    s = base
    for sec, key, val in edits:
        s = setv(s, sec, key, repr(val) if isinstance(val, float) else val)
    open(os.path.join(OUT, f"exp_{name}.grtload"), "w", encoding="utf-8").write(s)
print(len(E), "files written to", OUT)
