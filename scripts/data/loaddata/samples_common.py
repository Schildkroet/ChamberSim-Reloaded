"""Shared by the extractors of published load data (lovex_extract.py, rs_extract.py): caliber and bullet matching."""
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bulletdata"))
from estimate_length import length_in  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import recordfiles  # noqa: E402
from vv_extract import norm  # noqa: E402

IN = 25.4
# names of the manufacturers' tables that differ from the C.I.P. names of the records
ALIASES = {".32 Smith & Wesson Long": "32 S.&W. Long", ".40 Smith & Wesson": "40 S.& W.", "7.5 x 55 Swiss": "7.5 x 55 Suisse", "45 ACP (Auto)": "45 Auto", "300 WSM": "300 Win Short Mag", "270 WSM": "270 Win Short Mag",
           "300 AAC BLK": "300 AAC Blackout", "300 Holland & Holland mag.": "300 H&H Mag.", "6,5x55 Swedish Mauser": "6.5 x 55 SE",
           "8x57 JS": "8 x 57 IS", "8x57 JRS": "8 x 57 IRS", "7,5x55 Swiss": "7.5 x 55 Suisse", "7 mm RUM": "7 mm Rem. Ultra Mag.",
           "7,62x54 R (Russian)": "7.62 x 54 R", "6,5x54 MS": "6.5 x 54 Mannl. Sch.",
           "6.5 X 55MM SWEDISH MAUSER": "6.5 x 55 SE", "7 X 57MM MAUSER": "7 x 57", "7.62 X 54R RUSSIAN": "7.62 x 54 R",
           "8 X 57MM JS MAUSER": "8 x 57 IS", "7.65 X 53MM MAUSER": "7.65 x 53 Arg.", "6.5 X 284 NORMA": "6.5 - 284 Norma",
           "6.5 X 284 WINCHESTER": "284 Win.", "6MM REMINGTON": "6 mm Rem.", "30 BR REMINGTON": "30 BR", "6MM BR REMINGTON": "6 mm BR Rem.",
           "280 REMINGTON (7MM EXPRESS)": "280 Rem.", "7MM STW (SHOOTING TIMES WESTERNER)": "7 mm STW", "300 RUGER COMPACT MAGNUM (300 RCM)": "300 RCM",
           "6.5 X 50MM ARISAKA": "6.5 x 50 R", "7.7 X 58MM ARISAKA": "7.7 x 58", "8MM-06": "8 mm-06", "6MM CREEDMOOR": "6 mm Creedmoor",
           ".30-06 Sprg.": "30-06 Spring.", ".300 WSM": "300 Win. Short Mag.", "7,5 x 55": "7.5 x 55 Suisse", "9,3 x 64": "9.3 x 64 Brenneke",
           # Reload Swiss guide 2025
           "38 Spec.": "38 Special", "45 ACP": "45 Auto", "7 mm WSM": "7 mm Win. Short Mag.", "7 x 64 Brenneke": "7 x 64", "9.3 x 64": "9.3 x 64 Brenneke",
           "7.65 Para": "7.65 Parabellum", "6 Norma BR": "6mm BR Norma", "25-20 Win CF": "25-20 Win.", "10.4 mm Ordonnanz": "10.40 Ord. It.",
           "416 Rigby": "416 Rigby N°2", "38 Super Comp": "38 Super Auto", "280 Rl. N.E. ROSS": "280 Riml. N. E. Ross", "243 WSSM": "243 Win. Super Short Mag.",
           "280 Ack Imp": "280 Ackley Improved", "577 N.E. 3\"": "577 N.E. 3\""}


class Records:
    """The project's calibers and projectiles (data/) with the lookups the extractors need."""

    def __init__(self, data: Path):
        self.calibers = {}
        for f in (data / "calibers").glob("*.json"):
            rec = json.loads(f.read_text(encoding="utf-8"))
            for n in [rec["cipname"], *rec.get("altname", "").split(",")]:
                self.calibers.setdefault(norm(n), rec)
        self.projectiles = [r for _, r in recordfiles.read(data / "projectiles")]

    def caliber(self, name: str):
        plain = re.sub(r"\s*\([^)]*\)", "", name).replace("®", "").strip()  # without "(WSM)", "(59,000 PSI)", ...
        for n in [ALIASES.get(name.strip(), name), ALIASES.get(plain, plain), *plain.split("/")]:
            if rec := self.calibers.get(norm(n)):
                return rec
        return None

    def bullet(self, maker: str, btype: str, grains: float, dia_mm: float):
        """(length mm, source, tail type): a record with the same maker, weight and diameter, else an estimated length."""
        match = [p for p in self.projectiles if p["mname"].lower() == maker.lower() and abs(p["gmass"] - grains) < 0.8
                 and abs(p["gdia"] - dia_mm) < 0.06]
        name = btype.lower()
        if match:
            best = max(match, key=lambda p: sum(w in p["pname"].lower() for w in re.findall(r"[a-z0-9]+", name)))
            return best["glen"], "database", best.get("gtailtype", 0)
        est, kind = length_in(grains, dia_mm / IN, f"{name} {maker}")
        return est * IN, "estimate:" + kind, 2 if re.search(r"bt|boat|a-?max|match|scenar|hpbt|otm|vld|smk", name) else 0
