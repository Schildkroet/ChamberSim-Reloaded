#!/usr/bin/env python3
"""Controlled fast-powder experiments on the 8x57 IS base load. Output: jobs/exp_ba_*.grtload

The solver under-predicts the pressure of fast powders (Ba >= 1.1). The real powders differ in many parameters at
once, so this batch varies ONE thing at a time on the same load:
  ba_<Ba>            : Ba alone (0.3 ... 7)                           -> how the burn speed enters
  ba3_xe<mm>         : Ba = 3 with other barrel lengths                -> the step size dx = (xe - x0) / 2000 changes
  ba3_a0/z1/z2_...   : Ba = 3 with other form-function parameters      -> which parameter the effect depends on
  ba3_mc / ba3_mp    : Ba = 3 with other charge / bullet mass          -> pressure level and dynamics"""
import re, os
BASE = os.path.expanduser("~/Downloads/GordonsReloadingTool-2021.2040-NIGHTLY-linux/loads/8x57is.grtload")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "jobs")

def setv(s, section, name, value):
    m = re.search(rf"<{section}>.*?</{section}>", s, re.S); blk = m.group(0)
    new, n = re.subn(rf'(<input name="{name}" value=")[^"]*(")', lambda mm: mm.group(1) + str(value) + mm.group(2), blk)
    assert n == 1, (section, name, n)
    return s[:m.start()] + new + s[m.end():]
def getv(s, section, name):
    blk = re.search(rf"<{section}>.*?</{section}>", s, re.S).group(0)
    return float(re.search(rf'<input name="{name}" value="([^"]*)"', blk).group(1))

base = open(BASE, encoding="utf-8").read()
mc, mp = getv(base, "propellant", "mc"), getv(base, "projectile", "mp")
os.makedirs(OUT, exist_ok=True)
n = 0
def write(name, s):
    global n
    open(os.path.join(OUT, f"exp_{name}.grtload"), "w", encoding="utf-8").write(s); n += 1
def ba(s, v): return setv(s, "propellant", "Ba", v)

for v in (0.3, 0.8, 1.2, 1.8, 2.5, 3.5, 5, 7):
    write(f"ba_{v}", ba(base, v))
b3 = ba(base, 3.0)
for xe in (150, 300, 1200, 2400):
    write(f"ba3_xe{xe}", setv(b3, "gun", "xe", xe))
for a0 in (0.0, 3.5):
    write(f"ba3_a0_{a0}", setv(b3, "propellant", "a0", a0))
for z1 in (0.2, 0.6):
    write(f"ba3_z1_{z1}", setv(b3, "propellant", "z1", z1))
for z2 in (0.7, 0.9):
    write(f"ba3_z2_{z2}", setv(b3, "propellant", "z2", z2))
for f in (0.6, 1.15):
    write(f"ba3_mc_x{f}", setv(b3, "propellant", "mc", repr(mc * f)))
for f in (0.5, 2.0):
    write(f"ba3_mp_x{f}", setv(b3, "projectile", "mp", repr(mp * f)))
print(n, "jobs written to", OUT)
