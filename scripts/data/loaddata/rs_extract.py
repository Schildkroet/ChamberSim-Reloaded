#!/usr/bin/env python3
"""Turns the load data of Reload Swiss (Nitrochemie, reload-swiss.com) into fit samples (CSV), like vv_extract.py.

    rs_extract.py [-o rs_samples.csv] [--html page.html]

The page of the online loading data embeds its whole table in the Nuxt state (a minified JavaScript function with the values as
arguments); this script reads that state without running any script. Per row: cartridge, powder, start and maximum load (charge,
V5 velocity, pressure), bullet (mass, type, maker), test barrel, cartridge length. Start and maximum load become two samples, both with
the measured pressure (column pressure_bar). Cartridges and bullets are matched as in lovex_extract.py.
"""
import argparse
import csv
import json
import re
import sys
import urllib.request
from pathlib import Path

from samples_common import IN, Records  # noqa: E402

PAGE = "https://www.reload-swiss.com/en/loading-data/rs-loading-data"


def split_literals(text: str) -> list[str]:
    """Top-level comma separated JavaScript literals (strings, numbers, identifiers, void 0)."""
    out, i, n = [], 0, len(text)
    while i < n:
        j, depth = i, 0
        while j < n and (depth or text[j] != ","):
            if text[j] in "([{":
                depth += 1
            elif text[j] in ")]}":
                depth -= 1
            elif text[j] == '"':
                j += 1
                while text[j] != '"':
                    j += 2 if text[j] == "\\" else 1
            j += 1
        out.append(text[i:j].strip())
        i = j + 1
    return out


def literal(t: str):
    if t.startswith('"'):
        return json.loads(t)
    return {"void 0": "", "null": "", "!0": True, "!1": False}.get(t, t)


def table(html: str) -> list[list]:
    js = re.search(r"<script>(window\.__NUXT__=.*?)</script>", html, re.S)[1]
    params = re.match(r"window\.__NUXT__=\(function\(([^)]*)\)", js)[1].split(",")
    args = js[js.rindex("}(") + 2:]
    env = dict(zip(params, (literal(t) for t in split_literals(args[:args.rindex("))")]))))
    start = js.index("data:[[", js.index('type:"load-data"')) + 5
    depth, k = 0, start
    while True:  # matching bracket, skipping strings
        c = js[k]
        if c == '"':
            k += 1
            while js[k] != '"':
                k += 2 if js[k] == "\\" else 1
        elif c == "[":
            depth += 1
        elif c == "]":
            depth -= 1
            if depth == 0:
                break
        k += 1
    return [[literal(t) if t.startswith('"') or t in ("void 0", "null") else env.get(t, t) for t in split_literals(r)]
            for r in re.findall(r"\[([^\[\]]*)\]", js[start + 1:k])]


def num(v) -> float:
    try:
        return float(v)
    except (TypeError, ValueError):
        return 0.0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("rs_samples.csv"))
    ap.add_argument("--html", type=Path, help="saved page instead of downloading it")
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    if args.html:
        html = args.html.read_text(encoding="utf-8")
    else:
        req = urllib.request.Request(PAGE, headers={"User-Agent": "Mozilla/5.0 (GRTReloaded data collection)"})
        html = urllib.request.urlopen(req, timeout=60).read().decode("utf-8")
    records = Records(args.data)
    rows, miss = [], {}
    for r in table(html)[4:]:
        if len(r) < 23 or not r[0] or not r[3]:
            continue
        cal = records.caliber(r[0])
        if not cal:
            miss[r[0]] = miss.get(r[0], 0) + 1
            continue
        dia, grains = cal.get("G1") or 0, num(r[18])
        barrel = re.search(r"([\d.]+)\s*mm", r[21])
        twist = re.search(r"1:([\d.]+)", r[21])
        if not (dia and grains and barrel and num(r[22])):
            continue
        length, source, tail = records.bullet(r[20], r[19], grains, dia)
        for kind, g, v, p in (("start", r[4], r[6], r[8]), ("max", r[10], r[12], r[14])):
            if num(g) > 0 and num(v) > 0:
                rows.append({"powder": r[3], "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""), "bullet": f"{r[20]} {r[19]}".strip(),
                             "bullet_gr": grains, "bullet_dia_mm": dia, "bullet_len_mm": f"{length:.2f}", "len_source": source,
                             "tail_type": tail, "col_mm": num(r[22]), "barrel_mm": float(barrel[1]),
                             "twist_mm": f"{float(twist[1]) * IN:.0f}" if twist else "", "kind": kind, "charge_g": num(g),
                             "velocity_ms": num(v), "pressure_bar": num(p) or ""})
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} samples -> {args.output}", file=sys.stderr)
    print("cartridges not found:", ", ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
