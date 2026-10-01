#!/usr/bin/env python3
"""Explicit temperature coefficients at realistic values. Output: jobs/exp_tc_*.grtload

The only powder of the database with explicit coefficients has tcc = -0.0476 and tch = +0.0668 (about 100 times
larger than the values of the first sweep). Questions answered by this batch:
 - does the linear law (effect ~ coefficient * |T - 21|, with the sign of the coefficient) hold at those values?
 - what happens when only one of tcc / tch is set (default for the other side, or no effect)?"""
import re, os
BASE = os.path.expanduser("~/Downloads/GordonsReloadingTool-2021.2040-NIGHTLY-linux/loads/8x57is.grtload")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "jobs")

def setv(s, section, name, value):
    m = re.search(rf"<{section}>.*?</{section}>", s, re.S); blk = m.group(0)
    new, n = re.subn(rf'(<input name="{name}" value=")[^"]*(")', lambda mm: mm.group(1) + str(value) + mm.group(2), blk)
    assert n == 1, (section, name, n)
    return s[:m.start()] + new + s[m.end():]

base = open(BASE, encoding="utf-8").read()
os.makedirs(OUT, exist_ok=True)
n = 0
def job(name, T, tcc, tch):
    global n
    s = setv(setv(setv(base, "propellant", "pt", T), "propellant", "tcc", tcc), "propellant", "tch", tch)
    open(os.path.join(OUT, f"exp_tc_{name}.grtload"), "w", encoding="utf-8").write(s); n += 1

TCC, TCH = -0.0476, 0.0668
for T in (-20, 0, 40, 60):                                  # both coefficients, like the database powder
    job(f"both_{'m' if T < 0 else 'p'}{abs(T)}", T, TCC, TCH)
job("cold_only_p60", 60, TCC, 0)                            # hot side with tch = 0: default or no effect?
job("hot_only_m20", -20, 0, TCH)                            # cold side with tcc = 0: default or no effect?
job("hot_only_p60", 60, 0, TCH)
job("cold_only_m20", -20, TCC, 0)
for c in (-0.01, -0.02):                                    # linearity of the cold side
    job(f"tcc{abs(c)}_m20", -20, c, 0)
for c in (0.02, 0.04):                                      # linearity of the hot side
    job(f"tch{c}_p60", 60, 0, c)
job("tcc_pos0.0016_m20", -20, 0.0016, 0)                    # sign check at a small positive value
print(n, "jobs written to", OUT)
