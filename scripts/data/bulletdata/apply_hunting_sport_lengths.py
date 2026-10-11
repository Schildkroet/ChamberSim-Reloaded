#!/usr/bin/env python3
"""Puts the bullet lengths measured on product photos into the project's projectile records.

    apply_hunting_sport_lengths.py [--data DIR]

  * H&N records get the length of the photo of their own bullet on hn-sport.de (bullets_hn_photos.csv: the single standing bullet, the
    camera elevation 10 deg and the factor 1.0256 fixed with a caliper measurement of 20.0 mm on the TC .458 300 gr HS and the same lead
    alloy density of all bullets);
  * LOS records (and H&N ones without such a photo) with a measured bullet of hunting-sport.com (bullets_hunting_sport.csv) of the same
    diameter (0.12 mm), weight (0.6 gr) and shape get its length;
  * the other estimated lead bullets (descr "... (lead), ...") are rescaled from the earlier density 10.3 to the shape-dependent one of
    estimate_length.lead_density().
The earlier lengths of the LOS records (length / width of the angled photos) were 14 % too short for the same reason. Idempotent."""
import argparse
import csv
import math
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent))
import recordfiles
from estimate_length import GRAIN_PER_G, lead_density

HN_MEASURED = "Length measured on the product photo of hn-sport.de (standing bullet; camera elevation and scale fixed with a caliper measurement of one bullet), about 4 % uncertain"
MEASURED = "Length measured on a product photo of hunting-sport.com (standing bullet, camera elevation corrected), about 5 % uncertain"


def shape(name: str) -> str:
    t = name.upper()
    if re.search(r"\bWC\b|WC-H|WC HB|WC-HB", t):
        return "WC"
    if re.search(r"\bHP\b", t):
        return "HP"
    if "SWC" in t:
        return "SWC"
    if re.search(r"\bTC\b|\bFP\b|\bFN\b", t) and "RNFP" not in t:
        return "TC"
    return "RN" if "RN" in t else "?"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", default=str(HERE.parent.parent.parent / "data"))
    args = ap.parse_args()
    meas = list(csv.DictReader(open(HERE / "bullets_hunting_sport.csv", encoding="utf-8")))
    folder = Path(args.data) / "projectiles"
    records = [r for _, r in recordfiles.read(folder)]
    hn = {(m["line"], round(float(m["diameter_in"]), 3), float(m["weight_gr"])): float(m["length_mm"])
          for m in csv.DictReader(open(HERE / "bullets_hn_photos.csv", encoding="utf-8"))}
    photo = rescaled = 0
    for r in records:
        if r["mname"] == "H&N":
            mm = re.match(r"(?P<line>.+?) \.(?P<dia>\d{3}) (?P<gr>[\d.]+) gr$", r["pname"])
            key = (mm["line"].replace(" ", "-"), round(float("0." + mm["dia"]), 3), float(mm["gr"])) if mm else None
            if key in hn:
                r["glen"] = round(hn[key], 1)
                r["descr"] = HN_MEASURED
                photo += 1
                continue
        maker = "H&N" if r["mname"] == "H&N" else "LOS" if r["mname"] == "LOS" else None
        if maker:
            found = [float(m["length_mm"]) for m in meas if m["brand"] == maker and abs(float(m["diameter_in"]) * 25.4 - r["gdia"]) < 0.12
                     and abs(float(m["weight_gr"]) - r["gmass"]) < 0.6 and m["shape"] == shape(r["pname"])]
            if found:
                r["glen"] = round(sum(found) / len(found), 1)
                r["descr"] = MEASURED
                photo += 1
                continue
        d = r.get("descr") or ""
        if "bullet type (lead" in d:  # estimated lead bullet: the length from the shape-dependent density (idempotent)
            area_cm2 = math.pi * (r["gdia"] / 20.0) ** 2
            new = round(r["gmass"] / GRAIN_PER_G / lead_density(r["mname"] + " " + r["pname"]) / area_cm2 * 10.0, 1)
            rescaled += int(new != r["glen"])
            r["glen"] = new
            r["descr"] = re.sub(r"\(lead[^)]*\)", "(lead, by shape)", d)
    recordfiles.write(folder, records)
    print(f"{photo} records with a photo length, {rescaled} lead estimates rescaled")


if __name__ == "__main__":
    main()
