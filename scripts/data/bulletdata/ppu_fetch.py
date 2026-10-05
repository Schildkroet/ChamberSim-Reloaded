#!/usr/bin/env python3
"""Reads the component bullet search of prvipartizan.com (PPU) into a CSV (bullets_ppu.csv).

    ppu_fetch.py [-o bullets_ppu.csv] [--cache dir]

The three bullet pages (handgun, rifle, rifle match) have a search form per caliber; its answer is a table of bullet code, caliber,
diameter, type and weight. No length and no ballistic coefficient: bullets_to_json.py estimates the length. Grain and inch.
"""
import argparse
import html
import re
import subprocess
import sys
from pathlib import Path

from bulletfetch import get, number, text_lines, write_csv

BASE = "https://www.prvipartizan.com/"
FORMS = [("handgun_bullets.php", "search_hbl.php", ""), ("rifle_bullets.php", "search_rbl.php", ""), ("riflem_bullets.php", "search_rblm.php", "match")]


def search(action: str, caliber: str) -> str:
    # a POST: not cached, the answer is small and the number of calibers low
    return subprocess.run(["curl", "-sS", "-m", "30", "-L", "-A", "Mozilla/5.0", "-d", "searchtype=calibre", "--data-urlencode",
                           f"searchterm={caliber}", BASE + action], capture_output=True, text=True, check=True).stdout


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_ppu.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    rows = []
    for page, action, kind in FORMS:
        calibers = [html.unescape(c).strip() for c in re.findall(r"<option>\s*([^<]+?)\s*</option>", get(BASE + page, args.cache))]
        for cal in calibers:
            lines = text_lines(search(action, cal))
            if "Weight (gr)" not in lines:
                continue
            body = lines[lines.index("Weight (gr)") + 1:]
            for k in range(0, len(body) - 5, 6):
                code, calibre, dia, typ, grams, gr = body[k:k + 6]
                if not re.fullmatch(r"B-\w+", code):
                    break
                rows.append({"manufacturer": "PPU", "line": f"{typ} match" if kind else typ, "part": code, "caliber_name": calibre,
                             "weight_gr": number(gr), "diameter_in": number(dia),
                             "base_style": "Boat Tail" if re.search(r"BT|boat|HPBT|match", typ + kind, re.I) else "",
                             "ogive_style": f"{typ} {kind}".strip(), "url": BASE + page})
    write_csv(args.output, rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
