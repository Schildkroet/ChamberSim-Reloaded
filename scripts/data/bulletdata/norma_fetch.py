#!/usr/bin/env python3
"""Reads the component bullets of norma-ammunition.com into a CSV (bullets_norma.csv).

    norma_fetch.py [-o bullets_norma.csv] [--cache dir]

The site is built on a Sanity content database whose public query interface lists the bullet products with product
number, name with the diameter ("Norma GTX 5.7 mm (.224) 69gr"), product line, bullet description, ballistic coefficient and
weight. No length: bullets_to_json.py estimates it. Grain and inch.
"""
import argparse
import json
import re
import sys
import urllib.parse
from pathlib import Path

from bulletfetch import get, number, write_csv

QUERY = '*[_type=="productData" && cartridgeType=="Centerfire Rifle - Bullet"]{sku,name,nameImp,caliber,productLine,status,majorFacts,claimsData,urlPaths}'
API = "https://6psbvfzb.api.sanity.io/v2021-06-07/data/query/production?query="


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_norma.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    items = json.loads(get(API + urllib.parse.quote(QUERY), args.cache))["result"]
    rows = []
    for it in items:
        facts = it.get("majorFacts") or {}
        name = it.get("nameImp") or it.get("name") or ""
        dia = re.search(r"\((\.\d{3})\)", name) or re.search(r"\((\.\d{3})\)", it.get("caliber", ""))
        desc = facts.get("bulletDescription", "")
        claims = " ".join(it.get("claimsData") or [])
        url = next((u["url"] for u in it.get("urlPaths", []) if u.get("language") == "en"), "")
        rows.append({"manufacturer": "Norma", "line": it.get("productLine", "").removeprefix("Norma ").strip(), "part": it.get("sku", ""),
                     "caliber_name": it.get("caliber", ""), "weight_gr": number(str(facts.get("weight_grain", ""))),
                     "diameter_in": number(dia[1]) if dia else "", "g1bc": number(str(facts.get("ballisticCoefficientG1", ""))),
                     "g7bc": number(str(facts.get("ballisticCoefficientG7", ""))),
                     "base_style": "Boat Tail" if re.search(r"boat|bt|hpbt|otm", desc + " " + claims, re.I) else "",
                     "ogive_style": desc, "discontinued": "" if it.get("status") == "active" else "yes",
                     "url": "https://www.norma-ammunition.com" + url if url else ""})
    write_csv(args.output, rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
