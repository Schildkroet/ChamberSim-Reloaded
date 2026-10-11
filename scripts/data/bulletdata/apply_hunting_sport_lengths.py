#!/usr/bin/env python3
"""Puts the bullet lengths measured on the hunting-sport.com photos (bullets_hunting_sport.csv) into the project's projectile records.

    apply_hunting_sport_lengths.py [--data DIR]

  * H&N and LOS records with a measured bullet of the same diameter (0.12 mm), weight (0.6 gr) and shape get its length;
  * the other estimated lead bullets (descr "... (lead), ...") are rescaled from the earlier density 10.3 to the shape-dependent one of
    estimate_length.lead_density().
The earlier lengths of the LOS records (length / width of the angled photos) were 14 % too short for the same reason. Idempotent."""
import argparse
import csv
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent))
import recordfiles
from estimate_length import lead_density

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
    photo = rescaled = 0
    for r in records:
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
        if "bullet type (lead), about 10 %" in d:
            old = r["glen"]
            r["glen"] = round(old * 10.3 / lead_density(r["mname"] + " " + r["pname"]), 1)
            r["descr"] = d.replace("(lead)", "(lead, by shape)")
            rescaled += int(r["glen"] != old)
    recordfiles.write(folder, records)
    print(f"{photo} records with a photo length, {rescaled} lead estimates rescaled")


if __name__ == "__main__":
    main()
