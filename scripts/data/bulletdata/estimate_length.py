"""Estimates the length of a bullet from its mass, diameter and type (for makers that publish no length).

The bullets of Berger, Nosler and Speer with published lengths (469) show that the effective density
(mass / (cross section x length)) depends mainly on the bullet class and a little on the caliber:

    class           effective density g/cm3         scatter (1 sigma)
    match (boat tail, VLD, hybrid, A-MAX, ...)   7.8 - 2.35 d      6 %
    hollow point                                 5.66 + 7.94 d     7 %
    fmj / total metal jacket                     8.15 + 0.44 d     6 %   (photos)
    soft point and other                         5.71 + 6.88 d    11 %
    lead (coated handgun bullets), by shape       RN 8.8, SWC 8.6, flat / truncated cone 9.4, hollow point 8.9, wadcutter 10.2   (4-6 %)
    hollow-point copper (TSX, E-Tip, ...)        6.65             -   (photos)

with d in inch. The lead densities are measured: 54 H&N and LOS handgun bullets (bullets_hunting_sport.csv), standing bullet of the two-bullet product photos
of hunting-sport.com with the camera elevation corrected (measure_standing_bullet.py). They replace the earlier 10.3 (not validated), which made lead
bullets 15 % too short; the earlier length / width measurement of those angled photos (image_length.py) was too short as well. 2026-10-11: the product photos of grauwolf.net (about 1,400 bullets measured as length / width x diameter, image_length.py)
confirm the match, hollow-point and soft-point classes (photo / estimate 0.98, 1.01, 1.00) and show copper 20 % and fmj 7 % longer than the earlier
densities (8.0 and 8.7 + 0.47 d) gave; the two classes were rescaled. The estimate is uncertain by the scatter above; records made with it say so in `descr`.
"""
import math
import re

GRAIN_PER_G = 15.4324
# (name, regex on the text of the line and style, density a, density b); the first match wins
CLASSES = [
    ("lead", r"\b(lrn|lfn|wc|swc|lswc|rnfp)\b|wadcutter|round nose lead|lead", 8.9, 0.0),  # by shape: lead_density()
    ("copper", r"\b(tsx|ttsx|lrx|cx|gmx|e-?tip|tac-?tx|tac-?xp|vor-?tx)\b|monolithic|all-copper", 6.65, 0.0),
    ("fmj", r"fmj|tmj|total metal|full metal|metal jacket", 8.15, 0.44),
    ("match", r"hpbt|otm|vld|hybrid|match|boat|\bbt\b|a-?max|smk|\bsmk\b|lapua", 7.8, -2.35),
    ("hollow", r"jhp|hollow|\bhp\b|xtp|sjhp|\bjhc\b|\bshp\b", 5.66, 7.94),
    ("other", r".*", 5.71, 6.88),
]


def classify(text: str) -> str:
    t = text.lower()
    return next(name for name, pattern, _, _ in CLASSES if re.search(pattern, t))


def lead_density(text: str) -> float:
    """Effective density of a coated lead handgun bullet by its shape (measured on product photos, see above)."""
    t = text.lower()
    if re.search(r"\bl?wc\b|wadcutter|wc-?h?-?hb", t):
        return 10.2
    if re.search(r"swc", t):
        return 8.6
    if re.search(r"\bl?f[np]\b|\btc\b|\bltc\b|flat|\bw[lf]n\b|\blw[lf]n", t) and not re.search(r"rnfp|rn fp", t):
        return 9.4
    if re.search(r"\bhp\b|hollow", t):
        return 8.9
    return 8.8 if re.search(r"\bl?rn\b|round|rnfp|rn fp", t) else 8.9


def length_in(weight_gr: float, diameter_in: float, text: str) -> tuple[float, str]:
    """(length in inch, class)."""
    name = classify(text)
    a, b = next((a, b) for n, _, a, b in CLASSES if n == name)
    density = lead_density(text) if name == "lead" else a + b * diameter_in  # g/cm3
    mass_g = weight_gr / GRAIN_PER_G
    area_cm2 = math.pi * (diameter_in * 2.54) ** 2 / 4
    return mass_g / density / area_cm2 / 2.54, name
