#!/usr/bin/env python3
"""Shotshell experiments, batch 11. Output: <out>/exp_*.grtload (default jobs/).

Batch 10 showed that the caliber's shotshell flag slows the burn of the original by a factor 0.83 ... 0.955 that
differs between loads (and lets the curve end exactly at the muzzle). This batch finds what the factor depends on: every
job comes as a pair, shotshell off ("_off") and on ("_on"), with ONE change on the 8x57 IS base load:
  z1_<v>, z2_<v>, a0_<v>   form function of the powder        Ba_<v>    burn rate
  ps_<bar>                 start pressure                     mc_<f>    charge
  k_<v>                    ratio of specific heats             9mm_*     the 9 mm Luger base load
The first job (exp_aa_warmup) only absorbs the original's empty-first-result glitch."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import jobgen
from jobgen import base8, build, getv, load_ratios, setv

jobgen.OUT = sys.argv[1] if len(sys.argv) > 1 else jobgen.OUT
write = jobgen.Writer()
write("aa_warmup", base8)
mc = getv(base8, "propellant", "mc")

def pair(name, s):
    write(f"{name}_off", setv(s, "caliber", "shotshell", "false"))
    write(f"{name}_on", setv(s, "caliber", "shotshell", "true"))

P = lambda s, k, v: setv(s, "propellant", k, v)
for z1 in (0.2, 0.35, 0.6):
    pair(f"sh_z1_{z1}", P(base8, "z1", z1))
pair("sh_z2_0.9", P(base8, "z2", 0.9))
for a0 in (0, 3):
    pair(f"sh_a0_{a0}", P(base8, "a0", a0))
for ba in (0.3, 1.5):
    pair(f"sh_Ba_{ba}", P(base8, "Ba", ba))
pair("sh_ps_100", setv(base8, "caliber", "ps", 100))
pair("sh_mc_0.8", P(base8, "mc", repr(mc * 0.8)))
pair("sh_k_1.3", P(base8, "k", 1.3))
ratios = load_ratios([("9mm", "titegroup")])
base9, _ = build("9mm", "titegroup", 0.45 / ratios["9mm_titegroup"])
pair("sh_9mm_z1_0.47", P(base9, "z1", 0.4697))
pair("sh_9mm_Ba_0.58", P(base9, "Ba", 0.5814))
print(write.n, "jobs written to", jobgen.OUT)
