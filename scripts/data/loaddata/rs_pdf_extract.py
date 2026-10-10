#!/usr/bin/env python3
"""Turns the Reload Swiss loading data guide (PDF, DE/EN) into fit samples (CSV), like rs_extract.py (which reads the web page).

    rs_pdf_extract.py guide.pdf [-o rs_pdf_samples.csv]

Reads `pdftotext -layout`. A data row is one line: cartridge, bullet mass (gr), type, maker, case, primer, powder (RSxx), the start
load and the maximum load (charge g and gr, V5 velocity, pressure, filling ratio), the energy E5, the test barrel (mm, twist) and the
cartridge length. Older rows of the guide miss the pressures or the start load; the number of numeric columns tells the layouts apart.
Start and maximum load become two samples (kind start / max). Cartridges and bullets are matched as in rs_extract.py.
"""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

from samples_common import IN, Records  # noqa: E402

NUMERIC = re.compile(r"^[\d,.\-\s()‘’'\"mm:]+$")


def f(s: str) -> float:
    try:
        return float(s.replace(",", "."))
    except ValueError:
        return 0.0


def loads(n: list[str]):
    """[(kind, charge g, velocity m/s, pressure bar)] from the numeric columns before the barrel, or None for an unknown layout."""
    k = len(n)
    if k >= 11:   # g gr V p fill g gr V p fill [E]
        return [("start", n[0], n[2], n[3]), ("max", n[5], n[7], n[8])]
    if k == 7:    # g gr V g gr V E
        return [("start", n[0], n[2], ""), ("max", n[3], n[5], "")]
    if k == 5:    # g gr V p E (maximum load only)
        return [("max", n[0], n[2], n[3])]
    if k == 4:    # g gr V E
        return [("max", n[0], n[2], "")]
    return None


def rows(text: str):
    for line in text.splitlines():
        t = [x for x in re.split(r"\s{2,}", line.strip()) if x]
        ip = [i for i, x in enumerate(t) if re.fullmatch(r"RS\d\d", x)]
        if not ip or ip[0] < 4 or not re.fullmatch(r"\d+(,\d)?", t[1]):
            continue
        i, tail = ip[0], t[ip[0] + 1:]
        while tail and not NUMERIC.match(tail[-1]):  # notes (C, HP, OL, ...)
            tail.pop()
        if len(tail) < 6 or not re.fullmatch(r"\d{2,3},\d", tail[-1]):
            yield None, line
            continue
        yield (t[0], f(t[1]), t[2], t[3], t[i], tail[-2], f(tail[-1]), tail[:-2]), line


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("rs_pdf_samples.csv"))
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    text = subprocess.run(["pdftotext", "-layout", str(args.pdf), "-"], capture_output=True, text=True, check=True).stdout
    records = Records(args.data)
    out, miss, skipped = [], {}, 0
    for r, line in rows(text):
        if r is None:
            skipped += 1
            continue
        name, grains, btype, maker, powder, barrel_s, col, num = r
        num = [x for x in num if re.fullmatch(r"-|\d+(,\d+)?", x)]
        parsed = loads(num)
        cal = records.caliber(name)
        if not cal:
            miss[name] = miss.get(name, 0) + 1
            continue
        barrel = re.match(r"(\d+)", barrel_s)
        twist = re.search(r"1:([\d.]+)", barrel_s)
        dia = cal.get("G1") or 0
        if not (parsed and barrel and dia and grains and col):
            skipped += 1
            continue
        length, source, tail = records.bullet(maker, btype, grains, dia)
        for kind, g, v, p in parsed:
            if f(g) > 0 and f(v) > 0:
                out.append({"powder": powder, "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""), "bullet": f"{maker} {btype}".strip(),
                            "bullet_gr": grains, "bullet_dia_mm": dia, "bullet_len_mm": f"{length:.2f}", "len_source": source,
                            "tail_type": tail, "col_mm": col, "barrel_mm": float(barrel[1]),
                            "twist_mm": f"{float(twist[1]) * IN:.0f}" if twist else "", "kind": kind, "charge_g": f(g),
                            "velocity_ms": f(v), "pressure_bar": f(p) or ""})
    with open(args.output, "w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, list(out[0].keys()))
        w.writeheader()
        w.writerows(out)
    print(f"{len(out)} samples -> {args.output} ({skipped} rows skipped)", file=sys.stderr)
    print("cartridges not found:", ", ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
