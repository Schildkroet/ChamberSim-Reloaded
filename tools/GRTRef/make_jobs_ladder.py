#!/usr/bin/env python3
"""Charge-ladder batch: ladder loads with every focus curve, plus single-charge runs at candidate charges."""
import re, os
BASE = os.path.expanduser("~/Downloads/GordonsReloadingTool-2021.2040-NIGHTLY-linux/loads/8x57is.grtload")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "jobs")
GRAIN = 0.06479890999975

def setv(s, section, name, value):
    m = re.search(rf"<{section}>.*?</{section}>", s, re.S); blk = m.group(0)
    new, n = re.subn(rf'(<input name="{name}" value=")[^"]*(")', lambda mm: mm.group(1) + str(value) + mm.group(2), blk)
    assert n == 1, (section, name, n)
    return s[:m.start()] + new + s[m.end():]
def setconfig(s, name, value):
    new, n = re.subn(rf'(<config name="{name}" value=")[^"]*(")', lambda mm: mm.group(1) + str(value) + mm.group(2), s)
    assert n == 1, name
    return new

base = open(BASE, encoding="utf-8").read()
mc = float(re.search(r'name="mc" value="([^"]*)"', base).group(1))
step = 2 * GRAIN
os.makedirs(OUT, exist_ok=True)
def write(name, s): open(os.path.join(OUT, f"exp_{name}.grtload"), "w", encoding="utf-8").write(s)

for f in range(4):                                  # ladder of 4 steps of 2 grain, every focus index
    s = setv(base, "propellant", "laddercnt", 4); s = setv(s, "propellant", "laddermc", repr(step)); s = setconfig(s, "CurveFocusIdx", f)
    write(f"lad_cnt4_f{f}", s)
for k in range(-3, 4):                              # single charges around the selected one
    s = setv(base, "propellant", "mc", repr(mc + k * step))
    write(f"lad_single_{'m' if k < 0 else 'p'}{abs(k)}", s)
s = setv(base, "propellant", "laddercnt", 10); s = setv(s, "propellant", "laddermc", repr(GRAIN)); s = setconfig(s, "CurveFocusIdx", 9)
write("lad_cnt10_f9", s)
s = setv(base, "propellant", "laddercnt", 3); s = setv(s, "propellant", "laddermc", repr(step)); s = setconfig(s, "CurveFocusIdx", 7)   # focus beyond the count
write("lad_cnt3_focus_out_of_range", s)
print("ladder jobs written, mc =", mc, "step =", step)
