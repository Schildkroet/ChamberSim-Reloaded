#!/usr/bin/env python3
"""Temperature and friction sweeps on the 8x57 IS base load. Output: jobs/exp_t_*.grtload, jobs/exp_fr_*.grtload

Temperature: pt with the powder's default coefficients (tcc = tch = 0) and with explicit tcc/tch, given as
multiples of the solver's current default slopes (tcc ~ 0.000677*Ba, tch ~ 0.000962*Ba per K).
Friction: gfrictionUnlock = true with different gfriction values, barrel lengths and bullet masses."""
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
ba, mp = getv(base, "propellant", "Ba"), getv(base, "projectile", "mp")
os.makedirs(OUT, exist_ok=True)
n = 0
def write(name, s):
    global n
    open(os.path.join(OUT, f"exp_{name}.grtload"), "w", encoding="utf-8").write(s); n += 1

COLD, HOT = 0.000677371 * ba, 0.000962335 * ba
for t in (-10, 10, 30, 50):                                   # more points with the default coefficients
    write(f"t_default_{'m' if t < 0 else 'p'}{abs(t)}", setv(base, "propellant", "pt", t))
for mult in (2, 4):                                           # explicit coefficients
    s = setv(setv(base, "propellant", "pt", -20), "propellant", "tcc", repr(COLD * mult)); write(f"t_cold_m20_x{mult}", s)
    s = setv(setv(base, "propellant", "pt", 60), "propellant", "tch", repr(HOT * mult)); write(f"t_hot_p60_x{mult}", s)
s = setv(setv(base, "propellant", "pt", 0), "propellant", "tcc", repr(COLD * 4)); write("t_cold_p0_x4", s)
s = setv(setv(base, "propellant", "pt", 40), "propellant", "tch", repr(HOT * 4)); write("t_hot_p40_x4", s)

def friction(g, xe=600, mass=mp):
    s = setv(setv(base, "projectile", "gfrictionUnlock", "true"), "projectile", "gfriction", g)
    s = setv(s, "gun", "xe", xe)
    return setv(s, "projectile", "mp", repr(mass)) if mass != mp else s
for g in (-50, -20, 20, 100, 400):
    write(f"fr_g{'m' if g < 0 else 'p'}{abs(g)}", friction(g))
for xe in (200, 400, 1000):
    write(f"fr_g100_xe{xe}", friction(100, xe=xe))
write("fr_g100_mp2", friction(100, mass=mp * 2))
print(n, "jobs written to", OUT)
