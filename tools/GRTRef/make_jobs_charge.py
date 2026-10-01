#!/usr/bin/env python3
"""Charge / loading-density experiments on the 8x57 IS base load. Output: jobs/exp_*.grtload

After the ignition ramp and the density-dependent covolume, the remaining peak-pressure error grows with the charge
mass (-5 % at 0.6 x charge, +5 % at 1.15 x). This batch separates charge, chamber volume and gas properties:
  ld_mc_x<f>       : base powder (Ba 0.58), charge x f            -> charge trend for a slow powder
  ld12_mc_x<f>     : Ba 1.2, charge x f                            -> ... for a medium powder
  ld3_mc_x<f>      : Ba 3, charge x f (adds to ba3_mc_x0.6/x1.15)  -> ... for a fast powder
  ld_cv_x<f>       : case volume x f, same charge (base / Ba 3)    -> loading density without changing the charge
  ld_pc<v>         : material density (covolume check)
  ld_qex<v>, ld_eta<v> : explosion heat, covolume factor eta
The first job (exp_aa_warmup) only absorbs the original's empty-first-result glitch."""
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
mc, cv = getv(base, "propellant", "mc"), getv(base, "caliber", "casevol")
os.makedirs(OUT, exist_ok=True)
n = 0
def write(name, s):
    global n
    open(os.path.join(OUT, f"exp_{name}.grtload"), "w", encoding="utf-8").write(s); n += 1
def ba(s, v): return setv(s, "propellant", "Ba", v)
def charge(s, f): return setv(s, "propellant", "mc", repr(mc * f))

write("aa_warmup", base)
for f in (0.4, 0.6, 0.8, 1.15, 1.3):
    write(f"ld_mc_x{f}", charge(base, f))
b12, b3 = ba(base, 1.2), ba(base, 3.0)
for f in (0.6, 1.3):
    write(f"ld12_mc_x{f}", charge(b12, f))
for f in (0.4, 0.8, 1.3):
    write(f"ld3_mc_x{f}", charge(b3, f))
for f in (0.85, 1.15, 1.4):
    write(f"ld_cv_x{f}", setv(base, "caliber", "casevol", repr(cv * f)))
for f in (1.15, 1.4):
    write(f"ld3_cv_x{f}", setv(b3, "caliber", "casevol", repr(cv * f)))
for pc in (1000, 1300, 1900):
    write(f"ld_pc{pc}", setv(base, "propellant", "pc", pc))
write("ld3_pc1000", setv(b3, "propellant", "pc", 1000))
for q in (3500, 4500):
    write(f"ld_qex{q}", setv(base, "propellant", "Qex", q))
write("ld_eta1.2", setv(base, "propellant", "eta", 1.2))
print(n, "jobs written to", OUT)
