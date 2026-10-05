#!/usr/bin/env python3
"""Reads the reloading bullets of hn-sport.de (H&N) into a CSV (bullets_hn.csv).

    hn_fetch.py [-o bullets_hn.csv] [--cache dir]

The reloading listing leads to the bullet pages (shape, diameter, weight, coating in the title; diameter and weight in
the data list). The bullets are lead (plain, greased "L", coated "HS"): the text "lead" in the type makes
estimate_length.py use the density of lead. No length, no ballistic coefficient. Grain and inch.
"""
import argparse
import re
import sys
from pathlib import Path

from bulletfetch import get, number, text_lines, write_csv

BASE = "https://www.hn-sport.de"
TITLE = re.compile(r"^(?P<shape>[A-Z][A-Z -]*?)\s+(?P<dia>\.\d{3,4}),?\s+(?P<gr>\d+(?:\.\d+)?)\s*gr(?:\s+(?P<coat>[A-Za-z-]+))?", re.I)


def parse(url: str, page: str) -> dict | None:
    lines = text_lines(page)
    title = next((l for l in lines if TITLE.match(l.split(" »")[0])), None)
    if not title:
        return None
    m = TITLE.match(title.split(" »")[0])
    spec = dict(zip(lines, lines[1:]))
    dia = number(spec.get("Durchmesser", "")) or m["dia"]
    shape, coat = m["shape"].strip(), (m["coat"] or "").strip()
    return {"manufacturer": "H&N", "line": f"{shape} {coat}".strip(), "part": "", "caliber_name": spec.get("Kaliber", ""),
            "weight_gr": m["gr"], "diameter_in": dia, "base_style": "Hollow Base" if re.search(r"\bHB\b|H-HB", shape) else "",
            "ogive_style": f"lead bullet {shape} {coat}", "url": url}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_hn.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    listing = get(BASE + "/de/reloading", args.cache)
    urls = sorted(set(re.findall(r'href="(/de/reloading/[^"?#]+)"', listing)))
    rows = []
    for u in urls:
        row = parse(BASE + u, get(BASE + u, args.cache))
        if row:
            rows.append(row)
        else:
            print("no title:", u, file=sys.stderr)
    write_csv(args.output, rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
