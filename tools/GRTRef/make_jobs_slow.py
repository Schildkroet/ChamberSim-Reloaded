#!/usr/bin/env python3
"""Slow, low-pressure burn experiments, batch 8. Output: jobs/exp_*.grtload.

Batch 7 showed that the original's burn increment per step is NOT proportional to Ba * phi(z) * p * dt at low
pressure (80..140 bar, slow powders): the ratio starts at 0.2, rises to 0.7 within ~80 steps and falls again later; at
200 bar and above it is 1.0. This batch separates TIME from POSITION and PRESSURE effects on the 9 mm Luger base load
(Titegroup with another Ba, load ratio 0.45, 150 mm barrel):
  ga_mp<f>_Ba<b>   bullet mass x f        -> the time per step changes, the distance per step does not
  gb_xe<mm>_Ba<b>  barrel length          -> the distance per step changes (dx = (xe - x0) / 2000)
  gc_mc<f>_Ba<b>   charge x f             -> the pressure level (Ba 1 with 2 x the charge reaches 480 bar)
  gd_ps<bar>_Ba<b> start pressure         -> the starting point of the ramp
  ge_cv<f>_Ba<b>   case volume x f        -> gas volume / density at the same charge
  gf_Ba<b>         Ba 0.3 ... 3           -> how the ratio depends on the burn speed itself
  gg_a0_/z1_       form function          -> separates phi(z) from the time effect
  gh_sebert<f>     Sebert factor          -> effective mass of the moving gas
The first job (exp_aa_warmup) only absorbs the original's empty-first-result glitch."""
from jobgen import *

ratios = load_ratios([("9mm", "titegroup")])
LOW = 0.45
write = Writer()
write("aa_warmup", base8)
base, _ = build("9mm", "titegroup", LOW / ratios["9mm_titegroup"])
mc, mp, cv = getv(base, "propellant", "mc"), getv(base, "projectile", "mp"), getv(base, "caliber", "casevol")
def ba(s, v): return setv(s, "propellant", "Ba", v)
def tag(v): return f"{v:g}"

for b in (1, 2):
    s0 = ba(base, b)
    for f in (0.25, 0.5, 2, 4):
        write(f"ga_mp{tag(f)}_Ba{b}", setv(s0, "projectile", "mp", repr(mp * f)))
    for xe in ((25, 50, 100, 300, 1200) if b == 1 else (50, 300)):
        write(f"gb_xe{xe}_Ba{b}", setv(s0, "gun", "xe", xe))
    for f in ((0.25, 0.5, 1.5, 2) if b == 1 else (1.5, 2)):  # more is "charge too high"
        write(f"gc_mc{tag(f)}_Ba{b}", setv(s0, "propellant", "mc", repr(mc * f)))
    for ps in ((20, 50, 150, 250) if b == 1 else (20, 150)):
        write(f"gd_ps{ps}_Ba{b}", setv(s0, "caliber", "ps", ps))
    for f in (0.75, 2):
        write(f"ge_cv{tag(f)}_Ba{b}", setv(s0, "caliber", "casevol", repr(cv * f)))
for v in (0.5, 0.75, 1.5, 3):
    write(f"gf_Ba{tag(v)}", ba(base, v))
b1 = ba(base, 1)
for a0 in (-2, 0, 3):
    write(f"gg_a0_{a0}_Ba1", setv(b1, "propellant", "a0", a0))
for z1 in (0.2, 0.7):
    write(f"gg_z1_{z1}_Ba1", setv(b1, "propellant", "z1", z1))
for f in (0.0, 0.5, 1.0):
    write(f"gh_sebert{tag(f)}_Ba1", setv(b1, "caliber", "sebert", f))
print(write.n, "jobs written to", OUT)
