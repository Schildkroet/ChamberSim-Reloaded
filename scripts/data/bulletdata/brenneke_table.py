#!/usr/bin/env python3
"""Reads the table of the reloading bullets on brenneke-ammunition.de and fills the project's Brenneke records with it.

    brenneke_table.py [--html page.html] [--write]

brenneke-ammunition.de/marken/buechsenmunition/wiederladergeschosse is one table: brand (TOG, TAG, TUG, TIG, Basic, ...), cartridges, diameter, BC,
pack size, weight (g/gr) and article number. It has no lengths and its pictures are logos (the product pages show cartridges and a cutaway without a
scale), so the lengths stay as they are. A record of the same diameter (0.06 mm) and weight (0.6 gr, or the gram value of the table) gets the BC (G1 assumed: the page does not say);
a bullet without a record is added with an estimated length (bullets_to_json.py)."""
import argparse
import html
import json
import re
import sys
import urllib.request
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import recordfiles  # noqa: E402
from bullets_to_json import record  # noqa: E402

URL = "https://www.brenneke-ammunition.de/marken/buechsenmunition/wiederladergeschosse"


def rows(page: str):
    seg = page[page.find("<table"):page.find("</table>")]
    for tr in re.findall(r"<tr.*?</tr>", seg, re.S)[1:]:
        c = [html.unescape(re.sub(r"<[^>]+>", "", x)).strip() for x in re.findall(r"<t[dh].*?</t[dh]>", tr, re.S)]
        if len(c) >= 8 and re.match(r"[\d,]+ ?/ ?\d+", c[6]):
            name = re.match(r"([A-Za-z]+(?: nature)?)", c[1])[1]
            yield {"line": name.capitalize() if name.isupper() and len(name) > 3 else name, "dia_in": float(c[3]), "gr": float(c[6].split("/")[1]),
                   "g": float(c[6].split("/")[0].replace(",", ".")),
                   "bc": float(c[4].replace(",", ".")), "part": c[7]}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--html", type=Path)
    ap.add_argument("--write", action="store_true")
    ap.add_argument("-o", "--output", type=Path, default=HERE.parents[2] / "data" / "projectiles" / "brenneke.json")
    args = ap.parse_args()
    page = args.html.read_text(encoding="utf-8") if args.html else urllib.request.urlopen(
        urllib.request.Request(URL, headers={"User-Agent": "Mozilla/5.0"})).read().decode("utf-8")
    records = recordfiles.records_of(args.output)
    updated = added = 0
    for r in rows(page):
        dia = r["dia_in"] * 25.4
        rec = next((x for x in records if abs(x["gdia"] - dia) < 0.06 and (abs(x["gmass"] - r["gr"]) < 0.6 or abs(x["gmass"] / 15.4324 - r["g"]) < 0.06)), None)
        if rec:
            if not rec.get("g1bc"):
                rec["g1bc"] = r["bc"]
                updated += 1
            continue
        new = record({"manufacturer": "Brenneke", "line": r["line"], "weight_gr": str(r["gr"]), "diameter_in": str(r["dia_in"]), "g1bc": str(r["bc"]),
                      "base_style": "Boat Tail" if r["line"] in ("TOG", "TAG", "TUG", "TIG") else ""})
        print("new:", new["pname"], new["glen"], "mm (estimated)")
        records.append(new)
        added += 1
    print(f"{updated} BC added, {added} records added", file=sys.stderr)
    if args.write:
        args.output.write_text(json.dumps(sorted(records, key=recordfiles.key), indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
