#!/usr/bin/env python3
"""Reads the bullet data published on lapua.com into a CSV (bullets_lapua.csv).

    lapua_fetch.py [-o bullets_lapua.csv] [--cache dir]

The product list comes from the site's WordPress API; every bullet page lists caliber, bullet line, code, diameter, type,
weight and the G1 and G7 ballistic coefficients. No length: bullets_to_json.py estimates it. Grain and inch.
"""
import argparse
import json
import re
import sys
from pathlib import Path

from bulletfetch import get, number, text_lines, write_csv

API = "https://www.lapua.com/wp-json/wp/v2/product?per_page=100&_fields=id,slug,link&page="


def parse(url: str, page: str) -> dict | None:
    lines = text_lines(page)
    spec = {}
    for k, v in zip(lines, lines[1:]):
        if k in ("Bullet", "Bullet Code", "Bullet diameter", "Bullet type", "Bullet weight", "BC G1", "BC G7", "Caliber"):
            spec.setdefault(k, v)
    if "Bullet weight" not in spec or "Bullet diameter" not in spec:
        return None
    if "Bullet Code" in lines:  # "Bullet", <line>, "Bullet Code", <code>
        spec["Bullet"] = lines[lines.index("Bullet Code") - 1]
    dia_in = re.search(r"\((\.\d+)", spec["Bullet diameter"])  # 5.69 mm (.224'')
    mm = re.match(r"\s*([\d.]+)\s*mm", spec["Bullet diameter"])
    weight = re.search(r"\(([\d.]+)\s*gr\)", spec["Bullet weight"])
    return {"manufacturer": "Lapua", "line": spec.get("Bullet", ""), "part": spec.get("Bullet Code", ""),
            "caliber_name": spec.get("Caliber", ""), "weight_gr": weight[1] if weight else "",
            "diameter_in": f"{float(mm[1]) / 25.4:.4f}" if mm else number(dia_in[1] if dia_in else ""),
            "g1bc": number(spec.get("BC G1", "")), "g7bc": number(spec.get("BC G7", "")),
            "base_style": "Boat Tail" if re.search(r"scenar|match|otm|hpbt|boat", spec.get("Bullet", "") + spec.get("Bullet type", ""), re.I) else "",
            "ogive_style": spec.get("Bullet type", ""), "url": url}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_lapua.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    products = []
    for pg in range(1, 10):
        chunk = json.loads(get(API + str(pg), args.cache))
        products += chunk
        if len(chunk) < 100:
            break
    rows = []
    for p in products:
        if "bullet" not in p["slug"]:
            continue
        row = parse(p["link"], get(p["link"], args.cache))
        if row:
            rows.append(row)
        else:
            print("no specifications:", p["link"], file=sys.stderr)
    write_csv(args.output, rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
