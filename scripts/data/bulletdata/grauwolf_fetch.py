#!/usr/bin/env python3
"""Reads the bullet listing of grauwolf.net (a German reloading shop) into a CSV (bullets_grauwolf.csv).

    grauwolf_fetch.py [-o bullets_grauwolf.csv] [--cache dir]

One request (?limit=all) returns the whole listing. Per bullet only the product title is there: "<.diameter in> / <mm> mm <weight> gr /
<g> g <maker> <type words> Geschosse <pack> Stück". No length, no ballistic coefficient. The same bullet is listed once per pack size and
as a test pack: the rows are made unique per maker, type, diameter and weight. Pellets (Diabolo) and round balls are left out. The German
type words are translated for estimate_length.py (Vollmantel = full metal jacket, Teilmantel = soft point, Hohlspitz = hollow point,
Blei = lead, bleifrei = lead free, taken as solid copper). Grain and inch. The listing covers makers that have no site of their own here
(Fox, Sax, Degol, DK Bullets, Brenneke, ...): bullets_to_json.py is run for those, the other makers have their own fetch scripts.
"""
import argparse
import html
import re
import sys
from pathlib import Path

from bulletfetch import get, write_csv

PAGE = "https://www.grauwolf.net/wiederladen/geschosse.html?limit=all"
MAKERS = ["Ballistix Academy", "Cutting Edge", "North Fork", "Sellier & Bellot", "DK Bullets", "Hornady", "Sierra", "Nosler", "Barnes", "Speer",
          "Berger", "Lapua", "Norma", "RWS", "Sako", "Federal", "Winchester", "Remington", "H&N", "Prvi Partizan", "Prvi", "PPU", "Degol",
          "Sax", "Fox", "LOS", "Brenneke", "CopperBear", "Magtech", "GECO", "Geco", "GPA", "Frontier", "Leader", "Alsa", "Hasler", "Swift",
          "Browning", "Buffalo Bore", "Cam Pro", "TAC", "ACME", "Armscor"]
# makers with their own fetch script (or no records wanted): the rows stay in the CSV with valid = no
OWN = {"Hornady", "Sierra", "Nosler", "Barnes", "Speer", "Berger", "Lapua", "Norma", "RWS", "Sako", "Federal", "Winchester", "Remington", "H&N",
       "PPU", "Sellier & Bellot"}
KEEP = {"Degol", "Sax", "Fox", "LOS", "Brenneke", "GECO", "Magtech", "CopperBear", "Frontier", "DK Bullets", "GPA", "Cutting Edge",
        "Ballistix Academy", "North Fork", "Leader", "Wi-La-Tech"}
RENAME = {"Prvi": "PPU", "Prvi Partizan": "PPU", "Geco": "GECO"}
TITLE = re.compile(r"^(?:\*[^*]*\*\s*)?(?P<dia>\.\d+)\s*/\s*[\d.,]+\s*mm(?:\s*\([^)]*\))?\s+(?P<gr>[\d.,]+)\s*gr\s*/\s*[\d.,]+\s*gr?\s+"
                   r"(?P<rest>.+?)\s+\d+\s*St(?:ü|ue)ck\b(?P<tail>.*)$")
WORDS = [(r"\bVollmantel\b|\bFMJ\b", "full metal jacket"), (r"\bTeilmantel\b|\bSP\b", "soft point"), (r"Hohlspitz|\bHP\b", "hollow point"),
         (r"\bBlei\b|Bleirund", "lead"), (r"[Bb]leifrei|Kupfer", "all-copper")]


def parse(title: str) -> dict | None:
    m = TITLE.match(html.unescape(title))
    if not m or re.search(r"Diabolo|Rundkugel|Pellet", title):
        return None
    rest = m["rest"]
    maker = next((k for k in MAKERS if re.match(re.escape(k) + r"\b", rest)), rest.split()[0])
    line = rest[len(maker):].strip()
    free = bool(re.search(r"[Bb]leifrei", line + m["tail"]))
    line = re.sub(r"\s*(?:Spitz|Rundkopf)?[Gg]eschosse?\b", "", line).strip()
    style = " ".join(en for rx, en in WORDS if re.search(rx, line + (" bleifrei" if free else "")))
    return {"manufacturer": RENAME.get(maker, maker), "line": line, "part": "", "caliber_name": "", "weight_gr": m["gr"].replace(",", "."),
            "diameter_in": "0" + m["dia"], "base_style": "Boat Tail" if re.search(r"\bBT\b|HPBT|TMBT|BTSP", line) else
            "Flat Base" if re.search(r"\bFB\b|HPFB", line) else "", "ogive_style": f"{line} {style}".strip()}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_grauwolf.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    page = get(PAGE, args.cache)
    titles = set(re.findall(r'<a href="([^"]+)" title="([^"]+)" class="product-image"', page))
    rows, seen, skipped = [], set(), []
    for url, title in sorted(titles, key=lambda t: t[1]):
        row = parse(title)
        if not row:
            skipped.append(html.unescape(title))
            continue
        key = (row["manufacturer"], row["line"], row["diameter_in"], row["weight_gr"])
        if key not in seen:
            seen.add(key)
            ok = row["manufacturer"] in KEEP
            rows.append({**row, "url": url, "valid": "yes" if ok else "no",
                         "note": "" if ok else "maker has its own fetch script" if row["manufacturer"] in OWN else "maker not wanted (few bullets, unclear)"})
    write_csv(args.output, rows)
    print(f"{len(skipped)} titles skipped (pellets, round balls, no weight): e.g. {skipped[:3]}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
