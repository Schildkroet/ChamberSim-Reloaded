#!/usr/bin/env python3
"""Reads the bullet data published on hornady.com into a CSV (bullets_hornady.csv).

    hornady_fetch.py [-o bullets_hornady.csv] [--cache dir]

The site map lists the bullet pages; every page carries its data as a JSON object (weight, diameter, ballistic
coefficient, SKU). Hornady publishes no bullet length: bullets_to_json.py estimates it. Grain and inch.
"""
import argparse
import html
import json
import re
import sys
from pathlib import Path

from bulletfetch import get, number, write_csv

SITEMAP = "https://www.hornady.com/sitemap.xml"


def clean(s: str) -> str:
    s = html.unescape(re.sub(r"<[^>]+>", "", s or ""))
    return re.sub(r"[®™]", "", s.replace("‑", "-")).strip()


def parse(url: str, page: str) -> dict | None:
    m = re.search(r"var item = (\{.*?\});\s*(?:var|</script)", page, re.S)
    if not m:
        return None
    d = json.loads(m[1])
    if not d.get("weight") or not d.get("caliberdiameter"):
        return None
    bc = {"g1bc": "", "g7bc": ""}
    for i in ("1", "2"):
        value, kind = number(str(d.get(f"ball_coef_{i}") or "")), str(d.get(f"ball_coef_type_{i}") or "")
        if value and "G7" in kind.upper():
            bc["g7bc"] = value
        elif value and "G1" in kind.upper():
            bc["g1bc"] = value
    line = clean(d.get("linetitle") or d.get("linktitle") or "")
    title = clean(d.get("title") or "")
    return {"manufacturer": "Hornady", "line": line, "part": str(d.get("sku") or ""), "caliber_name": clean(d.get("caliber", "")),
            "weight_gr": number(str(d["weight"])), "diameter_in": number(d["caliberdiameter"]), **bc,
            "base_style": "Boat Tail" if re.search(r"\bBT\b|boat", title + " " + line, re.I) else "", "ogive_style": title, "url": url}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_hornady.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    urls = [u for u in re.findall(r"<loc>([^<]*)", get(SITEMAP, args.cache)) if re.match(r"https://www\.hornady\.com/bullets/(rifle|handgun)/.+", u)]
    rows, seen = [], set()
    for u in urls:
        try:
            row = parse(u, get(u, args.cache))
        except (ValueError, KeyError):
            row = None
        if row and (row["part"], row["weight_gr"]) not in seen:
            seen.add((row["part"], row["weight_gr"]))
            rows.append(row)
        elif not row:
            print("no data:", u, file=sys.stderr)
    write_csv(args.output, rows)
    print(f"{len(urls)} pages read", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
