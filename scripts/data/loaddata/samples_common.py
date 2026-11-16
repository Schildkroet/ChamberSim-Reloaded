"""Shared by the extractors of published load data (lovex_extract.py, rs_extract.py): caliber and bullet matching."""
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bulletdata"))
from estimate_length import length_in  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import recordfiles  # noqa: E402
from start_pressure import estimate as start_class  # noqa: E402
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
           "280 Ack Imp": "280 Ackley Improved", "577 N.E. 3\"": "577 N.E. 3\"",
           "8.5 x 63": "8.5 x 63 R", "6 x 47 Lapua": "6 x 47 SM"}  # 8.5 x 63: the rimless twin of the 8.5 x 63 R (same body)


# Abbreviations of the load tables (Hodgdon's RLDC and manual, Lyman, ...) -> words of the record names, per bullet maker.
BULLET_WORDS = {
    "nosler": {"bt": "ballistic tip", "ab": "accubond", "ablr": "accubond long range", "cc": "custom competition", "part": "partition", "pt": "partition",
               "e-tip": "e-tip", "rdf": "rdf"},
    "sierra": {"smk": "matchking", "sgk": "gameking", "tgk": "tipped gameking", "tmk": "tipped matchking", "spt": "pro-hunter", "bk": "blitzking",
               "sfire": "matchking"},
    "speer": {"gs": "grand slam", "hotcor": "hot cor", "hot-cor": "hot cor", "gd": "gold dot", "gdhp": "gold dot", "tnt": "tnt", "dgx": "dgx"},
    "hornady": {"i-lock": "interlock", "btsp": "interlock", "sp": "interlock", "bthp-m": "match", "eld-m": "eld match", "eld-vt": "eld-vt",
                "a-max": "a-max", "v-max": "v-max", "fmj-bt": "fmj"},
    "barnes": {"tsx-bt": "tsx bt", "tsx-fb": "tsx fb", "ttsx-bt": "ttsx bt", "ttsx-fb": "ttsx fb"},
    "berger": {"vld-ht": "vld hunting", "vld-tgt": "vld target", "hyb-tgt": "hybrid target", "bt-tgt": "bt target", "tgt-fb": "fb target", "fbt": "flat base"},
    "swift": {"aframe": "a-frame"},
}
# maker codes of the tables, never part of a record name
MAKER_CODES = {"hdy", "hrn", "sie", "nos", "spr", "bar", "ber", "sft", "sfi", "lap", "swi", "win", "rem", "nor", "ceb"}


def material(maker: str, name: str) -> str:
    """Construction class of a bullet for matching: copper, brass, lead or jacketed (start_pressure.py, without the handgun class)."""
    return start_class(maker, name, handgun=False)[1]


def bullet_words(maker: str, name: str) -> set:
    """The words of a table's bullet type that can be looked for in the record names (at least two letters, abbreviations expanded)."""
    words = set()
    expand = BULLET_WORDS.get(maker.lower(), {})
    for token in re.findall(r"[a-z0-9][a-z0-9/-]*", name.lower()):
        for t in {token, *token.split("-"), *token.split("/")}:
            if t in expand:
                words.update(re.findall(r"[a-z0-9-]+", expand[t]))
            if len(t) > 1 and t not in MAKER_CODES and re.fullmatch(r"[a-z0-9]+", t):
                words.add(t)
    return words


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

    def candidates(self, maker: str, grains: float, dia_mm: float):
        """The records of the maker with this weight and diameter."""
        return [p for p in self.projectiles if p["mname"].lower() == maker.lower() and abs(p["gmass"] - grains) < 0.8
                and abs(p["gdia"] - dia_mm) < 0.06]

    def match(self, maker: str, btype: str, grains: float, dia_mm: float):
        """The record of the same maker, weight and diameter whose name shares the most words with the type (the tables' abbreviations
        expanded, BULLET_WORDS); among records that share nothing (or equally much) the one closest to the estimated length. None without
        a candidate. Until 2026-11-15 the first record was taken: Hornady 190 gr BTSP .308 got the 42 mm CX instead of the 33 mm Sub-X."""
        match = self.candidates(maker, grains, dia_mm)
        if not match:
            return None
        name = btype.lower()
        words = bullet_words(maker, name)
        est, _ = length_in(grains, dia_mm / IN, f"{name} {maker}")

        def score(p):
            return sum(w in p["pname"].lower() for w in words)
        top = max(map(score, match))
        if top == 0:  # no record shares a word: only one of the same construction (copper, lead, jacketed) may stand in, else estimate
            own = material(maker, btype)
            match = [p for p in match if material(p["mname"], p["pname"]) == own]
            if not match:
                return None
        return min((p for p in match if score(p) == top), key=lambda p: abs(p["glen"] - est * IN))

    def bullet(self, maker: str, btype: str, grains: float, dia_mm: float):
        """(length mm, source, tail type): the length of the matching record (match()), else an estimated length."""
        if rec := self.match(maker, btype, grains, dia_mm):
            return rec["glen"], "database", rec.get("gtailtype", 0)
        name = btype.lower()
        est, kind = length_in(grains, dia_mm / IN, f"{name} {maker}")
        return est * IN, "estimate:" + kind, 2 if re.search(r"bt|boat|a-?max|match|scenar|hpbt|otm|vld|smk", name) else 0

    def split(self, bullet: str):
        """(maker, type) of a sample's bullet column "Maker Type" (the longest record maker that starts it), or (None, bullet)."""
        if not hasattr(self, "_makers"):
            self._makers = sorted({p["mname"] for p in self.projectiles}, key=len, reverse=True)
        low = bullet.lower()
        maker = next((m for m in self._makers if low.startswith(m.lower() + " ") or low == m.lower()), None)
        return (maker, bullet[len(maker):].strip()) if maker else (None, bullet)
