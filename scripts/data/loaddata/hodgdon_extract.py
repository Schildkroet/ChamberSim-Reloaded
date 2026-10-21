#!/usr/bin/env python3
"""Turns the rifle tables of the Hodgdon Reloading Manual (PDF) into fit samples (CSV), like vv_extract.py.

    hodgdon_extract.py manual.pdf [-o hodgdon_samples.csv] [--pressure psi|all]

The manual has two columns per page (pdftotext crops them one after the other) and lists per cartridge (test barrel, twist, trim
length, case, primer, cartridge length) and bullet (maker code, weight, type) the starting and maximum load of every powder with
velocity and pressure. It covers Hodgdon, IMR, Winchester, and the Accurate and Ramshot powders that Hodgdon sells. Pressures are
given in PSI or CUP; only PSI values (piezo) are used as pressure, CUP rows keep their velocities (--pressure all: both, wrong for CUP).
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
MAKERS = {"BAR": "Barnes", "HDY": "Hornady", "BER": "Berger", "NOS": "Nosler", "SPR": "Speer", "SIE": "Sierra", "LAP": "Lapua", "SWI": "Swift",
          "HRN": "Hornady", "SFIRE": "Sierra", "WIN": "Winchester", "REM": "Remington", "NOR": "Norma", "TS": "", "CAST": ""}
POWDER_MAKER = [(r"^A-|^LT-|^Accurate|^Magpro|^MagPro|^No\. ?\d|^Solo|^Nitro", "Accurate"), (r"^IMR", "IMR"),
                (r"^(TAC|X-Terminator|Ramshot|Magnum|Hunter|Big Game|Enforcer|LRT|Competition|Silhouette|Zip|True Blue|Sport)", "Ramshot"),
                (r"^(W\d|StaBALL|Winchester)", "Winchester")]
NUM = r"(\d[\d,]*\.?\d*)"
ROW = re.compile(rf"^(?P<p>[A-Za-z][\w()&.\- ]*?)\s+{NUM}(C?)\*?\s+{NUM}\s+{NUM}\s+(PSI|CUP)\s+{NUM}(C?)\*?\s+{NUM}\s+{NUM}\s+(PSI|CUP)\s*$")
ROW_MAX = re.compile(rf"^(?P<p>[A-Za-z][\w()&.\- ]*?)\s{{3,}}{NUM}(C?)\*?\s+{NUM}\s+{NUM}\s+(PSI|CUP)\s*$")
BARREL = re.compile(r"Barrel:\s*([\d.]+)\S*(?:\s+Twist:\s*1:\s*([\d.]+))?")
BULLET = re.compile(r"Bullet:\s*(\d+)\s*GR\.?\s*(\S+)\s*(.*?)\s+Dia:\s*([\d.]+)")
CASE = re.compile(r"Col:\s*([\d.]+)")
PRIMER = re.compile(r"Primer:\s*(.*?)\s{2,}Col:")


def n(s: str) -> float:
    return float(s.replace(",", ""))


def powder_name(p: str) -> str:
    p = re.sub(r"\s+", " ", p).strip()
    maker = next((m for pat, m in POWDER_MAKER if re.search(pat, p)), "Hodgdon")
    p = re.sub(r"^A-", "", p)
    return p if p.startswith(maker) else f"{maker} {p}"


def gutter(pdf: Path, page: int) -> int:
    """x where the right column starts: the second "Powder" heading of the page (the names are left-aligned with it), else the middle."""
    box = subprocess.run(["pdftotext", "-f", str(page), "-l", str(page), "-bbox", str(pdf), "-"], capture_output=True, text=True).stdout
    heads = sorted(float(x) for x in re.findall(r'xMin="([\d.]+)" yMin="[\d.]+" xMax="[\d.]+" yMax="[\d.]+">Powder<', box))
    return int(heads[-1]) - 3 if len(heads) >= 2 and heads[-1] > 250 else 335


def pages(pdf: Path):
    count = int(re.search(r"Pages:\s+(\d+)", subprocess.run(["pdfinfo", str(pdf)], capture_output=True, text=True).stdout)[1])
    for page in range(1, count + 1):
        split = gutter(pdf, page)
        for x, w in ((0, split), (split, 670 - split)):
            yield subprocess.run(["pdftotext", "-f", str(page), "-l", str(page), "-layout", "-x", str(x), "-y", "0", "-W", str(w), "-H", "783",
                                  str(pdf), "-"], capture_output=True, text=True).stdout.splitlines()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("hodgdon_samples.csv"))
    ap.add_argument("--pressure", choices=["psi", "all"], default="psi")
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    records = Records(args.data)
    rows, miss, codes = [], {}, {}
    title = barrel = twist = bullet = None
    primer = ""
    cal = None
    col = None
    prev = ""
    for lines in pages(args.pdf):
        for raw in lines:
            line = raw.strip()
            if not line:
                continue
            m = BARREL.match(line)
            if m:
                title = re.split(r"\s{2,}", prev)[0].rstrip("*")
                cal = records.caliber(title)
                if not cal:
                    miss[title] = miss.get(title, 0) + 1
                barrel = float(m[1]) * IN
                twist = float(m[2]) * IN if m[2] else None
                bullet = None
                prev = line
                continue
            m = BULLET.match(line)
            if m:
                code = m[2].rstrip(".")
                codes[code] = codes.get(code, 0) + 1
                bullet = {"gr": float(m[1]), "maker": MAKERS.get(code, ""), "type": f"{code} {m[3]}".strip()}
                col = None
                prev = line
                continue
            m = CASE.search(line)
            if m and line.startswith("Case:"):
                col = float(m[1]) * IN
                pm = PRIMER.search(line)
                primer = pm[1].strip() if pm else ""
                prev = line
                continue
            r = ROW.match(line)
            start = None
            if r:
                g = r.groups()
                start, mx = (n(g[1]), n(g[3]), n(g[4]), g[5]), (n(g[6]), n(g[8]), n(g[9]), g[10])
            else:
                r = ROW_MAX.match(line)
                if r:
                    g = r.groups()
                    mx = (n(g[1]), n(g[3]), n(g[4]), g[5])
            if r and cal and bullet and col and cal.get("G1") and barrel:
                dia = cal["G1"]
                length, source, tail = records.bullet(bullet["maker"], bullet["type"], bullet["gr"], dia)
                for kind, v in (("start", start), ("max", mx)):
                    if not v:
                        continue
                    pressure = n(str(v[2])) * PSI_BAR if (v[3] == "PSI" or args.pressure == "all") else ""
                    rows.append({"powder": powder_name(r.group("p")), "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""),
                                 "bullet": f"{bullet['maker']} {bullet['type']}".strip(), "bullet_gr": bullet["gr"], "bullet_dia_mm": dia,
                                 "bullet_len_mm": f"{length:.2f}", "len_source": source, "tail_type": tail, "col_mm": f"{col:.2f}",
                                 "barrel_mm": f"{barrel:.0f}", "twist_mm": f"{twist:.0f}" if twist else "", "kind": kind,
                                 "charge_g": round(v[0] * GRAIN_G, 4), "velocity_ms": round(v[1] * 0.3048, 1),
                                 "pressure_bar": f"{pressure:.0f}" if pressure != "" else "", "primer": primer})
                continue
            if not r:
                prev = line
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} samples -> {args.output}", file=sys.stderr)
    print("bullet codes:", dict(sorted(codes.items(), key=lambda kv: -kv[1])), file=sys.stderr)
    print("cartridges not found:", ", ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
