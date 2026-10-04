#!/usr/bin/env python3
"""Turns the pistol, revolver and rifle tables of the Vectan reloading catalogue (Nobel Sport, PDF) into fit samples (CSV).

    vectan_extract.py catalogue.pdf [-o vectan_samples.csv] [--ocr-cache ocr.json] [--list-unmatched]

The tables of the catalogue (pages 6-23) are drawn as graphics, the PDF has no text layer for them: the pages are rendered with
pdftoppm and read with RapidOCR (`pip install rapidocr-onnxruntime pillow numpy`; the result is cached with --ocr-cache). The table
columns are found from the position of the words. Per cartridge the catalogue gives the cartridge length (mm) and per bullet
(weight in grains, type, primer) and powder the starting and the maximum load (grains) with the velocity (fps). It gives NO
barrel (C.I.P. test barrels, "fired in proof barrels") and NO pressures: the barrel is an assumption (--rifle-barrel,
--handgun-barrel) and the fit can only use velocities.

Values that the OCR got wrong (or that are missing) are found by plausibility checks (start < maximum charge and velocity,
ratio of the charges) and the row is dropped; --list-dropped prints them with the page, so they can be looked up in the PDF.
Cartridges are matched to data/calibers (OCR spelling variants in TITLES), bullets to data/projectiles (the maker is not given:
a record of any maker with the same weight and diameter, else a length estimated from mass, diameter and type).
The shotshell tables (pages 27-44, with pressures) are not read here.
"""
import argparse
import csv
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

from samples_common import Records  # noqa: E402

GR = 0.06479891  # g per grain
FPS = 0.3048  # m/s per fps
PAGES = (6, 23)
TOP = 270  # below the column headers (pixels at 200 dpi)

# OCR spelling (spaces removed, upper case) -> name of the project's record; titles that norm() cannot match by themselves
TITLES = {key: name for keys, name in [
    (["6,35BROWNING(25AUTO)(25ACP)", "6,35BR0WNING(25AUT0)(25ACP)"], "6.35 Browning"),
    (["7,62X25TOKAREV", "7,62X25T0KAREV"], "7.62 x 25 Tokarev"),
    (["7,65BROWNING(32AUTO)(32ACP)", "7,65BR0WNING(32AUT0)(32ACP)"], "7.65 mm Browning"),
    (["9MMPARABELLUM(9LUGER)(9X19)", "9MMPARABELLUM(9LUGER)"], "9 mm Luger"),
    (["22H0RNET(5,6X35R)"], "22 Hornet"), (["222REMINGT0N(5,7X43)"], "222 Rem."),
    (["223REMINGT0N(5,56US0U5,56X45)"], "223 Rem."),
    ([".270WSM"], "270 Win. Short Mag."), ([".300WSM"], "300 Win. Short Mag."), ([".416RIGBY"], "416 Rigby N°2"),
    (["22-250REMINGT0N(5,6X49R)"], "22-250 Rem"), (["223WSSM"], "223 Win. Super Short Mag."),
    (["243WSSM"], "243 Win. Super Short Mag."), (["30-06CARTRYC0URT"], "30-06 Court Cartry"), (["303SP0RTING"], "303 Sporting"),
    (["308N0RMAMAGNUM"], "308 Norma Mag."), (["32SMITH&WESSONLONGNP"], "32 S&W Long"),
    (["32SMITH&WESSONLONGWADCUTTER"], "32 S&W Long Wad Cut."), (["35WHELENREMINGTON"], ".35 Whelen"),
    (["38SPECIALWADCUTTER"], "38 Spl. Wad Cut."), (["40SMITH&WESSON"], "40 S.& W."), (["44SMITH&WESSONSPECIAL"], "44 S&W Special"),
    (["45ACP(45MOD-1911)"], "45 Auto"), (["45-70G0VERNMENT(11,5X53R)"], "45-70 Govt."), (["500SMITH&WESSONMAGNUM"], "500 S&W Mag"),
    (["5,6X57RWSET5,6X57RWS"], "5.6 x 57"), (["6X62FRERESET6X62RFRERES"], "6 x 62 Freres"), (["6MMPPCET6MMPPC-USA"], "6 mm PPC"),
    (["6MMREMINGTONBRET6MMBRNORMA"], "6 mm BR Rem."), (["6,5X39GRENDEL"], "6.5 x 39"),
    (["6,5X63ET6,5X63RMESSNERMAGNUM"], "6.5 x 63 Messner Mag."), (["600NITR0EXPRESS"], "600 N.E."), (["7X65RBRENNEKE"], "7 x 65 R"),
    (["7MMBR"], "7 mm BR Rem."), (["7XWSM"], "7 mm Win. Short Mag."), (["8X57IRS0UJRS"], "8 x 57 IRS"), (["8X57IS0UJS"], "8 x 57 IS"),
    (["8X60SET8X60RS"], "8 x 60 S"), (["8X64SBRENNEKEET8X65RSBRENNEKE"], "8 x 64 S"), (["8X68"], "8 x 68 S"),
] for key in keys}

# type abbreviations of the catalogue (page 24) -> words that estimate_length.py classifies
TYPES = {"SSP": "soft point", "TNTHP": "hollow point", "HPBTMK": "hpbt match", "BK": "match boat tail", "BT": "boat tail",
         "SPBT": "boat tail", "V-MAX": "hollow point", "PART": "soft point", "FMJBT": "fmj boat tail", "SP": "soft point",
         "SPT": "soft point", "SST": "hollow point", "JSP": "soft point", "JHP": "jhp", "JRN": "fmj", "RN": "fmj",
         "LRN": "lead round nose", "LFN": "lead flat nose", "LSWC": "lead semi wadcutter", "SWC": "lead semi wadcutter",
         "WC": "lead wadcutter", "LBBWC": "lead wadcutter", "LHBWC": "lead wadcutter", "DEWC": "lead wadcutter",
         "KSPB": "lead", "LCN": "lead", "LFP": "lead", "LFNGC": "lead", "LTB": "lead", "(L)": "lead", "L": "lead", "TMJ": "fmj",
         "FPJ": "fmj", "MC": "fmj", "FMC": "fmj", "XTP": "jhp", "HP": "hollow point", "FP": "soft point", "FB": "soft point"}


def key(s: str) -> str:
    return s.upper().replace(" ", "")


def ocr_pages(pdf: Path, cache: Path | None) -> dict:
    if cache and cache.exists():
        return {int(k): v for k, v in json.loads(cache.read_text()).items()}
    from rapidocr_onnxruntime import RapidOCR  # noqa: E402

    engine = RapidOCR()
    out = {}
    with tempfile.TemporaryDirectory() as tmp:
        subprocess.run(["pdftoppm", "-r", "200", "-png", "-f", str(PAGES[0]), "-l", str(PAGES[1]), str(pdf), f"{tmp}/p"], check=True)
        for f in sorted(Path(tmp).glob("p-*.png")):
            page = int(f.stem.split("-")[-1])
            result, _ = engine(str(f))
            items = [{"x0": b[0][0], "y0": b[0][1], "x1": b[2][0], "y1": b[2][1], "text": t} for b, t, _ in (result or [])]
            out[page] = {"items": items, "lines": separator_lines(f)}
    if cache:
        cache.write_text(json.dumps(out))
    return out


def separator_lines(png: Path) -> list[int]:
    """y of the horizontal rules between the bullet groups (grey lines across the bullet and primer columns)."""
    import numpy as np
    from PIL import Image

    a = np.asarray(Image.open(png).convert("L"))[:, 70:470] <= 200
    ys = np.nonzero(a.mean(axis=1) > 0.97)[0]
    lines = []
    for y in ys:
        if not lines or y - lines[-1][-1] > 2:
            lines.append([y])
        else:
            lines[-1].append(y)
    return [int(sum(g) / len(g)) for g in lines]


def num(text: str) -> float | None:
    s = text.replace(" ", "").replace(",", ".")
    return float(s) if re.fullmatch(r"\d+(\.\d+)?", s) else None


def charge(text: str) -> float | None:
    """A charge in grains; the OCR sometimes drops the decimal point ("660" is 66.0)."""
    s = text.replace(" ", "").replace(",", ".")
    if re.fullmatch(r"\d{3,4}", s):
        return int(s) / 10
    return num(s)


POWDER = re.compile(r"^(Ba\s*10|Ba\s*9\s*1/2|Ba\s*9|Ba\s*7\s*1/2|Ba\s*6\s*1/2|Tu\s*\d000|A24|AS|A1|A0)$", re.I)


def powder_name(text: str) -> str:
    s = re.sub(r"\s+", "", text)
    if s.lower().startswith("tu"):
        return "Tubal " + s[2:]
    s = s.replace("1/2", " 1/2")
    return "A" + s[1:].upper() if s[0] in "aA" and not s.lower().startswith("ba") else "Ba" + s[2:]


def parse_page(page: dict, pageno: int, dropped: list):
    items = [{**i, "cx": (i["x0"] + i["x1"]) / 2, "cy": (i["y0"] + i["y1"]) / 2} for i in page["items"] if (i["y0"] + i["y1"]) / 2 > TOP]
    lines = sorted(page["lines"])
    bands = []  # cartridge title rows
    for i in items:
        if re.search(r"LENGTH", i["text"], re.I) and i["cx"] > 750:
            same = [j for j in items if abs(j["cy"] - i["cy"]) < 15]
            title = "".join(j["text"] for j in sorted(same, key=lambda j: j["x0"]) if j["cx"] < 750)
            length = next((num(m) for j in sorted(same, key=lambda j: j["x0"]) if j["cx"] > 750
                           for m in re.findall(r"\d+[.,]\d+", j["text"])), None)
            bands.append({"cy": i["cy"], "title": title, "length": length})
    bands.sort(key=lambda b: b["cy"])
    anchors = sorted((i for i in items if 480 < i["cx"] < 590 and POWDER.match(i["text"].strip())), key=lambda i: i["cy"])
    rows = []
    for a in anchors:
        row = {"cy": a["cy"], "powder": powder_name(a["text"]), "page": pageno}
        for i in items:
            if abs(i["cy"] - a["cy"]) > 13 or i is a:
                continue
            col = "c1" if 580 <= i["cx"] < 690 else "v1" if 690 <= i["cx"] < 800 else "c2" if 800 <= i["cx"] < 915 else "v2" if i["cx"] >= 915 else None
            if col and col not in row:
                row[col] = i["text"]
        rows.append(row)
    bounds = sorted(set(lines) | {b["cy"] - 20 for b in bands})
    for r in rows:
        band = [b for b in bands if b["cy"] < r["cy"]]
        r["band"] = band[-1] if band else None
        lo = max([y for y in bounds if y < r["cy"]], default=TOP)
        hi = min([y for y in bounds if y > r["cy"]], default=10_000)
        group = [i for i in items if lo < i["cy"] < hi and i["cx"] < 480]
        weight = next((num(i["text"]) for i in sorted(group, key=lambda i: i["cy"]) if i["cx"] < 190 and num(i["text"])), None)
        btype = " ".join(i["text"] for i in sorted(group, key=lambda i: (round(i["cy"] / 8), i["x0"])) if 190 <= i["cx"] < 345)
        primer = " ".join(i["text"] for i in sorted(group, key=lambda i: (round(i["cy"] / 8), i["x0"])) if 345 <= i["cx"] < 480)
        r.update(weight=weight, btype=btype.strip(), primer=primer.strip())
    return rows


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("vectan_samples.csv"))
    ap.add_argument("--ocr-cache", type=Path, help="JSON file with the OCR result (made when missing)")
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[2] / "data")
    ap.add_argument("--rifle-barrel", type=float, default=600.0, help="assumed test barrel of rifle cartridges, mm")
    ap.add_argument("--handgun-barrel", type=float, default=120.0, help="assumed test barrel of pistol and revolver cartridges, mm")
    ap.add_argument("--list-dropped", action="store_true")
    args = ap.parse_args()
    pages = ocr_pages(args.pdf, args.ocr_cache)
    records = Records(args.data)

    rows, dropped, missing = [], [], {}
    for pageno, page in sorted(pages.items()):
        for r in parse_page(page, pageno, dropped):
            band = r["band"]
            if not band or not band["length"]:
                dropped.append((pageno, r["powder"], "no cartridge"))
                continue
            title = band["title"]
            cal = records.caliber(TITLES.get(key(title).upper(), title))
            if not cal:
                missing[title] = missing.get(title, 0) + 1
                continue
            c1, v1, c2, v2 = charge(r.get("c1", "")), num(r.get("v1", "")), charge(r.get("c2", "")), num(r.get("v2", ""))
            ok = None not in (c1, v1, c2, v2) and c1 < c2 <= 1.5 * c1 and v1 < v2 <= 1.5 * v1 and r["weight"]
            if not ok:
                dropped.append((pageno, f"{r['powder']} {title}", f"{r['weight']} gr {r['btype']}: {c1} {v1} {c2} {v2}"))
                continue
            dia = cal.get("G1") or 0
            if not dia:
                continue
            abbr = TYPES.get(r["btype"].upper().replace(" ", ""), r["btype"])
            length, source, tail = records.bullet("", abbr, r["weight"], dia)
            handgun = "/tab-iv/" in cal.get("cippdf", "")
            barrel = args.handgun_barrel if handgun else args.rifle_barrel
            for kind, g, v in (("start", c1, v1), ("max", c2, v2)):
                rows.append({"powder": f"Vectan {r['powder']}", "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""),
                             "bullet": f"{r['weight']:g} gr {r['btype']}".strip(), "bullet_gr": r["weight"], "bullet_dia_mm": dia,
                             "bullet_len_mm": f"{length:.2f}", "len_source": source, "tail_type": tail, "col_mm": band["length"],
                             "barrel_mm": barrel, "twist_mm": "", "kind": kind, "charge_g": f"{g * GR:.3f}",
                             "velocity_ms": f"{v * FPS:.1f}", "pressure_bar": ""})
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} samples -> {args.output}; {len(dropped)} rows dropped", file=sys.stderr)
    print("cartridges not found:", ", ".join(f"{k} ({v})" for k, v in sorted(missing.items())), file=sys.stderr)
    if args.list_dropped:
        for d in dropped:
            print("dropped:", *d, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
