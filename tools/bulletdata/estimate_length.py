"""Estimates the length of a bullet from its mass, diameter and type (for makers that publish no length).

The bullets of Berger, Nosler and Speer with published lengths (469) show that the effective density
(mass / (cross section x length)) depends mainly on the bullet class and a little on the caliber:

    class           effective density g/cm3         scatter (1 sigma)
    match (boat tail, VLD, hybrid, A-MAX, ...)   7.8 - 2.35 d      6 %
    hollow point                                 5.66 + 7.94 d     7 %
    fmj / total metal jacket                     8.7 + 0.47 d      6 %
    soft point and other                         5.71 + 6.88 d    11 %
    lead (round nose, wadcutter), not validated  10.3              -
    solid copper (TSX, LRX, ...), not validated  8.0               -

with d in inch. The estimate is uncertain by the scatter above; records made with it say so in `descr`.
"""
import math
import re

GRAIN_PER_G = 15.4324
# (name, regex on the text of the line and style, density a, density b); the first match wins
CLASSES = [
    ("lead", r"\b(lrn|lfn|wc|swc|lswc|rnfp)\b|wadcutter|round nose lead|lead", 10.3, 0.0),
    ("copper", r"\b(tsx|ttsx|lrx|cx|gmx|e-?tip|tac-?tx|tac-?xp|vor-?tx)\b|monolithic|all-copper", 8.0, 0.0),
    ("fmj", r"fmj|tmj|total metal|full metal|metal jacket", 8.7, 0.47),
    ("match", r"hpbt|otm|vld|hybrid|match|boat|\bbt\b|a-?max|smk|\bsmk\b|lapua", 7.8, -2.35),
    ("hollow", r"jhp|hollow|\bhp\b|xtp|sjhp|\bjhc\b|\bshp\b", 5.66, 7.94),
    ("other", r".*", 5.71, 6.88),
]


def classify(text: str) -> str:
    t = text.lower()
    return next(name for name, pattern, _, _ in CLASSES if re.search(pattern, t))


def length_in(weight_gr: float, diameter_in: float, text: str) -> tuple[float, str]:
    """(length in inch, class)."""
    name = classify(text)
    a, b = next((a, b) for n, _, a, b in CLASSES if n == name)
    density = a + b * diameter_in  # g/cm3
    mass_g = weight_gr / GRAIN_PER_G
    area_cm2 = math.pi * (diameter_in * 2.54) ** 2 / 4
    return mass_g / density / area_cm2 / 2.54, name
