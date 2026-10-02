#!/usr/bin/env python3
"""Reads the Sako bullet lines (Gamehead, Hammerhead, Powerhead, ...) from sako.global into a CSV (bullets_sako.csv).

    sako_fetch.py [-o bullets_sako.csv] [--cache dir] [--calibers data/calibers]

Sako publishes the bullets as loaded cartridges: every ammunition page of a bullet line lists, per caliber, the bullet weight and
ballistic coefficient. The bullet diameter follows from the caliber: it is looked up in the project's caliber records (G1);
a caliber that is not found makes the row invalid. No length: bullets_to_json.py estimates it. Grain and inch.
"""
import argparse
import json
import re
import sys
from pathlib import Path

from bulletfetch import get, number, text_lines, write_csv

BASE = "https://www.sako.global"
LINES = ["gamehead", "gamehead-pro", "gamehead-varmint", "gamehead-varmint-rx", "hammerhead", "powerhead-blade", "powerhead-blade-pro",
         "powerhead-blade-solid", "speedhead", "super-hammerhead", "trg-defense", "trg-precision", "trg-sport"]
ABBREVIATIONS = [("winchester", "win"), ("remington", "rem"), ("magnum", "mag"), ("weatherby", "weath"), ("creedmoor", "creed")]


def norm(name: str) -> str:
    s = name.lower().replace(",", ".").replace("×", "x")
    for a, b in ABBREVIATIONS:
        s = s.replace(a, b)
    return re.sub(r"[^a-z0-9]", "", s).lstrip("0")


def diameters(calibers: Path) -> dict[str, float]:
    out = {}
    for f in calibers.glob("*.json"):
        rec = json.loads(f.read_text(encoding="utf-8"))
        if rec.get("G1"):
            for n in [rec["cipname"], *rec.get("altname", "").split(",")]:
                out.setdefault(norm(n), rec["G1"] / 25.4)
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_sako.csv"))
    ap.add_argument("--cache", type=Path)
    ap.add_argument("--calibers", type=Path, default=Path(__file__).resolve().parents[2] / "data" / "calibers")
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    dia = diameters(args.calibers)
    rows, seen = [], set()
    for line in LINES:
        url = f"{BASE}/ammunition/{line}"
        lines = text_lines(get(url, args.cache))
        if "Product options" not in lines:
            continue
        i = lines.index("Product options")
        while i < len(lines):
            if lines[i] == "Caliber" and i + 4 < len(lines) and lines[i + 2] == "Bullet weight":
                caliber, weight = lines[i + 1], re.search(r"([\d.]+)\s*gr", lines[i + 3])
                bc = lines[lines.index("Ballistic coefficient", i) + 1] if "Ballistic coefficient" in lines[i:i + 12] else ""
                d = dia.get(norm(caliber)) or dia.get(norm("." + caliber))
                key = (line, caliber, weight[1] if weight else "")
                if weight and key not in seen:
                    seen.add(key)
                    rows.append({"manufacturer": "Sako", "line": line.replace("-", " ").title(), "part": "", "caliber_name": caliber,
                                 "weight_gr": weight[1], "diameter_in": f"{d:.3f}" if d else "", "g1bc": number(bc),
                                 "base_style": "Boat Tail" if re.search(r"hammerhead|trg|gamehead pro|speedhead", line) else "",
                                 "ogive_style": line.replace("-", " "), "url": url})
                i += 4
            else:
                i += 1
    write_csv(args.output, rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
