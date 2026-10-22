#!/usr/bin/env python3
"""Compares the project's Berger records with Berger's bullet reference chart and fills in what the chart has.

    berger_chart.py [--html chart.html] [--write] [-o data/projectiles]

bergerbullets.com/information/lines-and-designs/bullet-reference-charts/ is one table: part number, description, diameter, OAL, boat tail length,
nose length, base to ogive, bearing surface, sectional density, G1 and G7 BC, G7 form factor, minimum twist (inch). A record is found by the part
number in its name, else by diameter and weight (the records made from the product pages without a part number). A record gets the chart's OAL when
it differs by more than 0.15 mm, and the BC and the boat tail length where it has none; a bullet that has no record is added. All of them get gValid (validated).
The chart has typos (the weight is missing in "6 mm BR COLUMN Target"): the weight then follows from the sectional density.
"""
import argparse
import html
import re
import sys
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import recordfiles  # noqa: E402
from bullets_to_json import record  # noqa: E402

URL = "https://bergerbullets.com/information/lines-and-designs/bullet-reference-charts/"
MM = 25.4


def number(text: str):
    try:
        return float(text)
    except ValueError:
        return None


def chart(page: str) -> list[dict]:
    out = []
    for tr in re.findall(r"<tr.*?</tr>", page, re.S):
        c = [html.unescape(re.sub(r"<[^>]+>", "", x)).strip() for x in re.findall(r"<t[dh].*?</t[dh]>", tr, re.S)]
        if not c or not re.fullmatch(r"\d{5}", c[0]) or len(c) < 13:
            continue
        dia = float(re.search(r"\(([\d.]+)\)", c[2])[1])
        m = re.search(r"([\d.]+) ?gr", c[1])
        gr = float(m[1]) if m else round(number(c[8]) * 7000 * dia * dia, 1)
        out.append({"part": c[0], "text": c[1], "dia_in": dia, "gr": gr, "oal_in": float(c[3]), "boat": c[4] != "FB", "tail_in": number(c[4]) or 0.0,
                    "g1": number(c[9]) or 0.0, "g7": number(c[10]) or 0.0})
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--html", type=Path, help="a saved copy of the page")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("-o", "--output", type=Path, default=Path(__file__).resolve().parents[3] / "data" / "projectiles")
    args = ap.parse_args()
    page = args.html.read_text(encoding="utf-8") if args.html else urllib.request.urlopen(
        urllib.request.Request(URL, headers={"User-Agent": "Mozilla/5.0"})).read().decode("utf-8")
    records = {recordfiles.key(r): r for _, r in recordfiles.read(args.output)}
    berger = [r for r in records.values() if r["mname"] == "Berger"]
    changed = added = 0
    for row in chart(page):
        dia, oal, tail = row["dia_in"] * MM, row["oal_in"] * MM, row["tail_in"] * MM
        rec = next((r for r in berger if r["pname"].endswith(row["part"])), None) or next(
            (r for r in berger if not re.search(r"\d{5}$", r["pname"]) and abs(r["gmass"] - row["gr"]) < 0.6 and abs(r["gdia"] - dia) < 0.06), None)
        if not rec:
            line = re.sub(r"^.*?\bgr\s+", "", row["text"]).replace("LR ", "Long Range ")
            new = record({k: str(v) for k, v in {"manufacturer": "Berger", "line": line, "part": row["part"], "weight_gr": row["gr"], "diameter_in": row["dia_in"],
                                                "length_in": row["oal_in"], "g1bc": row["g1"], "g7bc": row["g7"],
                                                "base_style": "Boat Tail" if row["boat"] else "Flat Base", "boat_tail_length_in": row["tail_in"]}.items()})
            new["gValid"] = True
            print("new:", new["pname"], new["gmass"], "gr", new["glen"], "mm")
            records[recordfiles.key(new)] = new
            added += 1
            continue
        edits = {} if rec.get("gValid") else {"gValid": True}  # every bullet of the chart is validated
        if abs(rec["glen"] - oal) > 0.15:
            edits["glen"] = round(oal, 1)
        if row["g1"] and not rec.get("g1bc"):
            edits["g1bc"] = row["g1"]
        if row["g7"] and not rec.get("g7bc"):
            edits["g7bc"] = row["g7"]
        if row["boat"] and tail and not rec.get("gtailh"):
            edits["gtailh"] = round(tail, 1)
        if edits:
            if "glen" in edits:
                rec.pop("descr", None)  # was an estimate
            rec.update(edits)
            print("update:", rec["pname"], edits)
            changed += 1
    print(f"{changed} records updated, {added} added", file=sys.stderr)
    if args.write and (changed or added):
        recordfiles.write(args.output, list(records.values()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
