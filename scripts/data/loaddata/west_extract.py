#!/usr/bin/env python3
"""Turns the rifle tables of the Western Powders Handloading Guide (PDF; Accurate and Ramshot powders) into fit samples (CSV).

    west_extract.py guide.pdf [-o west_samples.csv]

The PDF's text layer has a space between all characters of the data rows ("2 0 0 N OSLE R PA R T 5 7 .3 2 , 6 5 4 ..."), so a row is read
with all spaces removed: bullet weight, maker, type, start load and velocity, maximum load and velocity, maximum pressure (PSI), cartridge
length. Cartridge headers ("308 WINCHESTER", "Barrel: 24" Twist: 1-10"") and powder headers ("ACCURATE 2015") are normal text.
"""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

from samples_common import Records  # noqa: E402

GRAIN_G = 0.06479891
IN = 25.4
PSI_BAR = 0.0689476
MAKERS = {"SIERRA": "Sierra", "HDY": "Hornady", "HORNADY": "Hornady", "NOSLER": "Nosler", "BARNES": "Barnes", "SPEER": "Speer", "BERGER": "Berger",
          "LAPUA": "Lapua", "SWIFT": "Swift", "WOODLEIGH": "Woodleigh", "NORMA": "Norma", "REMINGTON": "Remington", "WINCHESTER": "Winchester",
          "FEDERAL": "Federal", "SELLIER": "Sellier & Bellot", "SPR": "Speer", "SIE": "Sierra", "NOS": "Nosler", "BAR": "Barnes", "BER": "Berger"}
ROW = re.compile(r"^(\d{2,3})([A-Z][A-Z0-9\-/&.()+]*?)(?P<s>\d{1,3}\.\d)(?P<sv>\d,\d{3})(?P<m>\d{1,3}\.\d)(?P<mv>\d,\d{3})(?P<p>\d{2},\d{3})(?P<col>\d\.\d{3})")
ROW_MAX = re.compile(r"^(\d{2,3})([A-Z][A-Z0-9\-/&.()+]*?)(?P<m>\d{1,3}\.\d)(?P<mv>\d,\d{3})(?P<p>\d{2},\d{3})(?P<col>\d\.\d{3})")


def nospace(line: str) -> str:
    return re.sub(r"\s+", "", line)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("west_samples.csv"))
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    records = Records(args.data)
    count = int(re.search(r"Pages:\s+(\d+)", subprocess.run(["pdfinfo", str(args.pdf)], capture_output=True, text=True).stdout)[1])
    rows, miss, unparsed = [], {}, 0
    title = cal = barrel = twist = powder = None
    for page in range(1, count + 1):
        text = subprocess.run(["pdftotext", "-f", str(page), "-l", str(page), "-raw", str(args.pdf), "-"], capture_output=True, text=True).stdout
        if "RIFLE DATA" not in text:
            continue
        prev = ""
        for line in text.splitlines():
            raw = line.strip()
            ns = nospace(raw)
            if raw.startswith("Barrel:"):
                title = re.sub(r"\s*\(continued\)", "", prev).strip()
                cal = records.caliber(title)
                if not cal:
                    miss[title] = miss.get(title, 0) + 1
                b = re.match(r"Barrel:(\d+)", ns)
                t = re.search(r"Twist:1-(\d+)", ns)
                barrel = float(b[1]) * IN if b else None
                twist = float(t[1]) * IN if t else None
                powder = None
            elif re.match(r"^[A-Z][A-Z0-9 \-.&/()]+$", raw) and not re.match(r"^[\d ]+$", raw) and (
                    raw.startswith(("ACCURATE", "RAMSHOT", "HODGDON", "IMR", "WINCHESTER", "NORMA", "ALLIANT", "VIHTAVUORI"))):
                powder = re.sub(r"\s*\(CONTINUED\)", "", raw).strip()
            elif powder and cal and barrel and re.match(r"^\d \d", raw):
                m = ROW.match(ns)
                start = True
                if not m:
                    m, start = ROW_MAX.match(ns), False
                if not m:
                    unparsed += 1
                    prev = raw
                    continue
                grains, rest = float(m[1]), m[2]
                maker = next((v for k, v in sorted(MAKERS.items(), key=lambda kv: -len(kv[0])) if rest.startswith(k)), "")
                btype = rest[len(next((k for k in MAKERS if rest.startswith(k) and MAKERS[k] == maker), "")):] if maker else rest
                dia = cal.get("G1")
                if not dia:
                    prev = raw
                    continue
                length, source, tail = records.bullet(maker, btype, grains, dia)
                col = float(m["col"]) * IN
                name = powder.title().replace("Imr", "IMR").replace("Tac", "TAC")
                for kind, g, v, p in (("start", m["s"] if start else None, m["sv"] if start else None, ""),
                                      ("max", m["m"], m["mv"], f"{float(m['p'].replace(',', '')) * PSI_BAR:.0f}")):
                    if g is None:
                        continue
                    rows.append({"powder": name, "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""), "bullet": f"{maker} {btype}".strip(),
                                 "bullet_gr": grains, "bullet_dia_mm": dia, "bullet_len_mm": f"{length:.2f}", "len_source": source,
                                 "tail_type": tail, "col_mm": f"{col:.2f}", "barrel_mm": f"{barrel:.0f}", "twist_mm": f"{twist:.0f}" if twist else "",
                                 "kind": kind, "charge_g": round(float(g) * GRAIN_G, 4), "velocity_ms": round(float(v.replace(",", "")) * 0.3048, 1),
                                 "pressure_bar": p})
            prev = raw
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} samples -> {args.output} ({unparsed} rows not parsed)", file=sys.stderr)
    print("cartridges not found:", ", ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
