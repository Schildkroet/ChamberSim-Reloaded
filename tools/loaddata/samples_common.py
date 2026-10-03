"""Shared by the extractors of published load data (lovex_extract.py, rs_extract.py): caliber and bullet matching."""
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bulletdata"))
from estimate_length import length_in  # noqa: E402
from vv_extract import norm  # noqa: E402

IN = 25.4
# names of the manufacturers' tables that differ from the C.I.P. names of the records
ALIASES = {"7.5 x 55 Swiss": "7.5 x 55 Suisse", "45 ACP (Auto)": "45 Auto", "300 WSM": "300 Win Short Mag", "270 WSM": "270 Win Short Mag",
           "300 AAC BLK": "300 AAC Blackout"}


class Records:
    """The project's calibers and projectiles (data/) with the lookups the extractors need."""

    def __init__(self, data: Path):
        self.calibers = {}
        for f in (data / "calibers").glob("*.json"):
            rec = json.loads(f.read_text(encoding="utf-8"))
            for n in [rec["cipname"], *rec.get("altname", "").split(",")]:
                self.calibers.setdefault(norm(n), rec)
        self.projectiles = [json.loads(f.read_text(encoding="utf-8")) for f in (data / "projectiles").glob("*.json")]

    def caliber(self, name: str):
        for n in [ALIASES.get(name.strip(), name), *name.split("/")]:
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
