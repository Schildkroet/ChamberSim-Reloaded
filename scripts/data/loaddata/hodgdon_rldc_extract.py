#!/usr/bin/env python3
"""Turns a PDF export of Hodgdon's Reloading Data Center (hodgdonreloading.com, "Print" of one cartridge's search) into fit samples (CSV),
with the columns of hodgdon_extract.py.

    hodgdon_rldc_extract.py export.pdf... [-o hodgdon_rldc_samples.csv]

The export has one comma-separated row per load (`pdftotext -layout`): bullet (weight, maker code, type), diameter, case, primer, powder maker,
powder, cartridge length, then start and maximum load with charge in grains (C = compressed), velocity in ft/s, pressure in PSI or CUP and
load density. The header gives twist and barrel length. Only PSI values are used as pressure; CUP rows keep their velocities. A start load
can be missing (empty charge, velocity 0). Several exports (one per cartridge) can be read in one run.
"""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

from samples_common import Records

GRAIN_G = 0.06479891
IN = 25.4
PSI_BAR = 0.0689476
# maker codes of the table; SFI (SinterFire, frangible) and BLUE (Blue Bullets, coated cast lead) have no records: estimated lengths
MAKERS = {"BAR": "Barnes", "BER": "Berger", "CEB": "Cutting Edge", "HDY": "Hornady", "NOS": "Nosler", "SIE": "Sierra", "SPR": "Speer",
          "SFT": "Swift", "LAP": "Lapua", "NOR": "Norma", "WIN": "Winchester", "REM": "Remington"}
LEAD = {"BLUE"}  # cast lead bullets (csr_eval column gmaterial)
NUM = r"\d[\d,]*\.?\d*"
# one load: charge (C = compressed), velocity, pressure with unit, load density
LOAD = rf"\s*(?P<{{0}}g>{NUM})?(?P<{{0}}c>C?)\s*,\s*(?P<{{0}}v>{NUM})\s*,\s*(?:(?P<{{0}}p>{NUM})\s+(?P<{{0}}u>PSI|CUP))?\s*,\s*(?:\d+%|N/A)\s*"
ROW = re.compile(r"(?P<gr>\d+) GR\. (?P<code>[A-Z]+) (?P<type>[^,]*?)\s*,(?P<dia>[\d.]+)\",(?P<case>[^,]*?)\s*,(?P<primer>.*?)\s*,(?P<maker>[A-Za-z]+)\s*,"
                 r"(?P<powder>[^,]*?)\s*,(?P<col>[\d.]+)\"," + LOAD.format("s") + "," + LOAD.format("m") + r"$")
HEAD = re.compile(r"Twist: 1:([\d.]+)\" \| Barrel Length: ([\d.]+)\"")


def n(s: str) -> float:
    return float(s.replace(",", ""))


def powder_name(maker: str, p: str) -> str:
    p = re.sub(r"\s+", " ", p).strip()
    if maker == "Accurate":
        p = re.sub(r"^A(?=\d)", "", p)  # A2460 -> Accurate 2460 (the records' name)
    return p if p.startswith(maker) else f"{maker} {p}"


def read(pdf: Path, records: Records) -> list[dict]:
    text = subprocess.run(["pdftotext", "-layout", str(pdf), "-"], capture_output=True, text=True, check=True).stdout
    lines = text.splitlines()
    title = next(line.strip() for line in lines if line.strip() and "Reloading Data Center" not in line)
    head = HEAD.search(text)
    cal = records.caliber(title)
    if not head or not cal or not cal.get("G1"):
        print(f"{pdf.name}: {title}: {'cartridge not in the records' if not cal else 'no barrel / twist header'}, skipped", file=sys.stderr)
        return []
    twist, barrel = float(head[1]) * IN, float(head[2]) * IN
    rows, bad, loads = [], [], 0
    for raw in lines:
        if " GR. " not in raw or ("PSI" not in raw and "CUP" not in raw):
            continue
        m = ROW.search(raw)
        if not m:
            bad.append(raw.strip())
            continue
        loads += 1
        maker = MAKERS.get(m["code"], "")
        length, source, tail = records.bullet(maker, m["type"], float(m["gr"]), cal["G1"])
        for k, kind in (("s", "start"), ("m", "max")):
            if not m[k + "g"] or n(m[k + "v"]) <= 0:
                continue
            psi = m[k + "p"] and m[k + "u"] == "PSI"
            rows.append({"powder": powder_name(m["maker"], m["powder"]), "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""),
                         "bullet": f"{maker or m['code']} {m['type']}".strip(), "bullet_gr": float(m["gr"]), "bullet_dia_mm": cal["G1"],
                         "bullet_len_mm": f"{length:.2f}", "len_source": source, "tail_type": tail, "col_mm": f"{float(m['col']) * IN:.2f}",
                         "barrel_mm": f"{barrel:.0f}", "twist_mm": f"{twist:.0f}", "kind": kind, "charge_g": round(n(m[k + "g"]) * GRAIN_G, 4),
                         "velocity_ms": round(n(m[k + "v"]) * 0.3048, 1), "pressure_bar": f"{n(m[k + 'p']) * PSI_BAR:.0f}" if psi else "",
                         "primer": m["primer"].strip(), "gmaterial": "lead" if m["code"] in LEAD else ""})
    print(f"{pdf.name}: {title}: {loads} loads, {len(rows)} samples" + (f", {len(bad)} rows not read" if bad else ""), file=sys.stderr)
    for b in bad[:5]:
        print("  ?", b, file=sys.stderr)
    return rows


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pdf", type=Path, nargs="+")
    ap.add_argument("-o", "--output", type=Path, default=Path("hodgdon_rldc_samples.csv"))
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    records = Records(args.data)
    rows = [r for pdf in args.pdf for r in read(pdf, records)]
    if not rows:
        return 1
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} samples -> {args.output}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
