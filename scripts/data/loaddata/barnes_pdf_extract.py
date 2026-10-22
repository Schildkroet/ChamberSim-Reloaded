#!/usr/bin/env python3
"""Turns the load data PDFs of barnesbullets.com/load-data into fit samples (CSV), like hodgdon_extract.py, and measures the bullets.

    barnes_pdf_extract.py <dir with the PDFs> [-o barnes_samples.csv] [--lengths barnes_lengths.csv] [--images dir]

One PDF per cartridge (test case, primer, trim length, barrel, twist), a block per bullet (weight and type, sectional density, ballistic
coefficient, cartridge length, a picture of the bullet) and a table with the starting and the maximum load of each powder (charge and
velocity, no pressure). Bullets that share a table (e.g. TSX and TTSX of one weight) get the same loads: the samples are written once, for the
first bullet. The picture is a side view with transparency; its silhouette gives the length of the bullet, the widest section being the
diameter of the cartridge's bullet (length = L / W x diameter), see measure_side_photos.py. Needs PyMuPDF (fitz), numpy and OpenCV.
The muzzleloader PDF (sabots, black powder substitutes) is skipped.
"""
import argparse
import csv
import re
import sys
from pathlib import Path

import cv2
import fitz
import numpy as np

from samples_common import Records  # noqa: E402

GRAIN_G = 0.06479891
IN = 25.4
MAX_DEVIATION = 0.30  # the length measured on the picture is used if it differs less than this from the project's record (else estimated) length
NUM = re.compile(r"^\d+(\.\d+)?$")
GRAIN = re.compile(r"^(\d+)-grain$", re.I)


# the cartridge names of the PDFs that the project's records spell differently (samples_common.ALIASES has more)
CARTRIDGES = {"40 Smith & Wesson": "40 S.& W.", ".44 Special": "44 S&W Special", "45 Automatic +P": "45 Auto", ".45 Colt Ruger and T/C": "45 Colt",
              ".460 Smith & Wesson": "460 S&W Mag", ".500 Smith & Wesson": "500 S&W Mag", "221 Fireball": "221 Rem. Fireball",
              "223 WSSM": "223 Win. Super Short Mag.", ".243 WSSM": "243 Win. Super Short Mag.", "257 Wby Mag": "257 Weath. Mag.",
              "6.5x284 Norma": "6.5 - 284 Norma", "6.5x55 Swedish Mauser": "6.5 x 55 SE", "7mm Remington Short Action Ultra Magnum":
              "7 mm Rem. SA Ultra Mag.", "7x57 Mauser": "7 x 57", ".300 RSAUM": "300 Rem. SA Ultra Mag.", "325 Winchester Short Magnum": "325 WSM",
              "8x57mm Mauser": "8 x 57 IS", "338 Lapua": "338 Lapua Mag.", "375 H&H": "375 H&H Mag.", ".416 Rigby": "416 Rigby N°2",
              ".404 Jeffery": "404 Riml. N.E.", ".470 Nitro Express": "470 N.E.", ".505 Gibbs": "505 Mag. Gibbs", ".500 Nitro Express 3\"": "500 N. E. 3''",
              "50 BMG": "50 Browning", ".577 Nitro Express": "577 N.E. 3\"", "7mm Shooting Times Westerner": "7 mm STW",
              "7mm Winchster Short Magnum": "7 mm Win. Short Mag.", "38 Special +P": "38 Special", "45-70": "45-70 Govt."}
BARREL_IN_TITLE = re.compile(r"^(5\.56 NATO|7\.62x51|7\.62x39)\b.*$")


def find_caliber(records: Records, title: str):
    if m := BARREL_IN_TITLE.match(title):
        title = {"5.56 NATO": "223 Rem.", "7.62x51": "308 Win.", "7.62x39": "7.62 x 39"}[m[1]]
    if title.startswith(".45-70"):
        title = "45-70"
    return records.caliber(CARTRIDGES.get(title, title))


def powder_name(raw: str) -> str:
    """The table's spelling (case, brand prefix, spaces differ between the PDFs) -> the name the other sample files use."""
    name = re.sub(r"\s+", " ", raw.replace("*", "").replace("\u2019", "'")).strip()
    low = name.lower()
    flat = re.sub(r"[^a-z0-9]", "", low)
    if m := re.fullmatch(r"(?:rl|reloder|reloader|alliant reloder)[- ]?(\d+|10x)", low):
        return "Alliant Reloder " + m[1].upper()
    if m := re.fullmatch(r"(?:a|accurate)?[- ]?(1680|2015|2200|2230|2460|2495|2520|2700|3100|4064|4100|4350|5744)", low):
        return "Accurate " + m[1]
    if m := re.fullmatch(r"(?:a[- ]?|accurate )?no[. ]*(\d+)", low):
        return f"Accurate No. {m[1]}"
    if m := re.fullmatch(r"(?:a[- ]?|accurate )?(?:no[. ]*|#)?(11) ?fs", low):
        return f"Accurate No. {m[1]} FS"
    if m := re.fullmatch(r"(?:power ?pro |pro |alliant )?(\d{3,4})-?(mr|r|mp)", low):
        return f"Alliant {m[1]}-{m[2].upper()}"
    if m := re.fullmatch(r"imr ?(\d{4}|700x|800x)(?: ?(s{1,2}c{1,2}|xbr))?", low) or re.fullmatch(
            r"(3031|4007|4064|4198|4227|4320|4350|4831|4895|4955|7828|7977|8133|8208)(?: ?(s{1,2}c{1,2}|xbr))?", low):
        return f"IMR {m[1].upper()}" + (f" {'SSC' if m[2][0] in 'sS' else m[2].upper()}" if m[2] else "")
    if m := re.fullmatch(r"(?:hodgdon )?(h\d+ ?[a-z]*)", low):
        return "Hodgdon " + m[1].upper().replace(" ", "").replace(" ", "")
    if m := re.fullmatch(r"(?:win(?:chester)? ?)w?(\d{3})", low):
        return "Winchester W" + m[1]
    if m := re.fullmatch(r"(?:vit|viht|vihta?vuori)?[ -]*(n[ -]?\d{3}\w*)", low):
        return "Vihtavuori " + m[1].upper().replace(" ", "").replace("-", "")
    if m := re.fullmatch(r"(?:winchester )?staball ?(6\.5|hd|match)", low):
        return "Winchester StaBALL " + (m[1].upper() if m[1] == "hd" else m[1].capitalize())
    if low in ("780", "supreme 780"):
        return "Hodgdon Supreme 780"
    fixed = {"benchmark": "Hodgdon Benchmark", "varget": "Hodgdon Varget", "blc2": "Hodgdon BL-C(2)", "cfe223": "Hodgdon CFE 223",
             "hybrid100v": "Hodgdon Hybrid 100V", "100v": "Hodgdon Hybrid 100V", "grand": "Ramshot Grand", "lrt": "Ramshot LRT", "longshot": "Hodgdon Longshot", "lilgun": "Hodgdon Lil'Gun", "retumbo": "Hodgdon Retumbo",
             "superformance": "Hodgdon Superformance", "titegroup": "Hodgdon Titegroup", "us869": "Hodgdon US 869", "hs6": "Hodgdon HS-6",
             "universal": "Hodgdon Universal", "clays": "Hodgdon Clays", "trailboss": "Hodgdon Trail Boss", "magpro": "Accurate Magpro",
             "xterminator": "Ramshot X-Terminator", "tac": "Ramshot TAC", "bigame": "Ramshot Big Game", "biggame": "Ramshot Big Game",
             "enforcer": "Ramshot Enforcer", "hunter": "Ramshot Hunter", "magnum": "Ramshot Magnum", "silhouette": "Ramshot Silhouette",
             "trueblue": "Ramshot True Blue", "zip": "Ramshot Zip", "competition": "Ramshot Competition", "bluedot": "Alliant Blue Dot",
             "bullseye": "Alliant Bullseye", "powerpistol": "Alliant Power Pistol", "arcomp": "Alliant AR-Comp", "2400": "Alliant 2400",
             "unique": "Alliant Unique", "powerpro300mp": "Alliant 300-MP", "powerpro4000mr": "Alliant 4000-MR", "powerpro2000mr": "Alliant 2000-MR",
             "powerpro1200r": "Alliant 1200-R", "herco": "Alliant Herco", "greendot": "Alliant Green Dot", "reddot": "Alliant Red Dot"}
    return fixed.get(flat, name)


def silhouette_ratio(doc: fitz.Document, xref: int):
    """(length / width of the silhouette in the bullet's axis, picture as BGRA) from the picture and its soft mask."""
    info = doc.extract_image(xref)
    if not info.get("smask"):
        return None
    mask = fitz.Pixmap(doc, info["smask"])
    pix = fitz.Pixmap(doc, xref)
    if pix.colorspace is None or pix.colorspace.n != 3:
        pix = fitz.Pixmap(fitz.csRGB, pix)
    alpha = np.frombuffer(mask.samples, np.uint8).reshape(mask.h, mask.w)
    rgb = cv2.resize(np.frombuffer(pix.samples, np.uint8).reshape(pix.h, pix.w, 3), (mask.w, mask.h))
    sil = (alpha > 128).astype(np.uint8)
    n, labels, stats, _ = cv2.connectedComponentsWithStats(sil)
    if n < 2:
        return None
    sil = (labels == 1 + int(np.argmax(stats[1:, cv2.CC_STAT_AREA]))).astype(np.uint8)
    pts = cv2.findNonZero(sil)
    (_, _), (w, h), _ = cv2.minAreaRect(pts)
    length, width = max(w, h), min(w, h)
    # width: the widest section across the axis (the minimum-area rectangle of a bullet with a boat tail is as wide as its bearing)
    return length / width, np.dstack([rgb[..., ::-1], alpha])


def lines_of(words, tol=3.0):
    """words (x0, y0, x1, y1, text) grouped into lines by their vertical centre."""
    out = []
    for w in sorted(words, key=lambda w: (w[1] + w[3]) / 2):
        yc = (w[1] + w[3]) / 2
        if out and abs(out[-1][0] - yc) <= tol:
            out[-1][1].append(w)
        else:
            out.append([yc, [w]])
    return [(yc, sorted(ws, key=lambda w: w[0])) for yc, ws in out]


def cartridge_title(page: fitz.Page) -> str:
    """The cartridge name: the text in the largest type of the first page (it is the headline and the sidebar)."""
    best = (0.0, "")
    for block in page.get_text("dict")["blocks"]:
        for line in block.get("lines", []):
            text = " ".join(sp["text"].strip() for sp in line["spans"] if sp["text"].strip())
            size = max((sp["size"] for sp in line["spans"]), default=0)
            if text and size > best[0] + 0.5 and not re.search(r"Revised|Case|Barrel|Twist|Primer|^Bullet", text):
                best = (size, text)
    return re.sub(r"\s*Load Data$", "", re.sub(r"\s+", " ", best[1]).strip())


def parse_header(text: str) -> dict:
    def find(pat):
        m = re.search(pat, text)
        return m[1].strip() if m else ""
    return {"case": find(r"Case:[ \t]*(\S[^\n]*?)[ \t]*(?:Primer|\n)"), "primer": find(r"Primer:[ \t]*(\S[^\n]*?)[ \t]*(?:Case Trim|\n)"),
            "trim": find(r"Case Trim(?: Length)?:\s*([\d.]+)"), "barrel": find(r"Barrel Length:\s*([\d.]+)"), "twist": find(r"Twist Rate:\s*1:\s*([\d.]+)")}


def blocks(page: fitz.Page):
    """[(table header y, table x of the charge column, [bullets], [rows])] of a page; bullet = dict(grain, type, coal, xref)."""
    words = [w[:5] for w in page.get_text("words")]
    heads = sorted(w for w in words if w[4] == "Powder")
    if not heads:
        return []
    images = [i for i in page.get_image_info(xrefs=True) if i["bbox"][2] - i["bbox"][0] < 100 and i["bbox"][3] - i["bbox"][1] < 100
              and max(i["width"], i["height"]) > 100 and i["xref"] > 0
              and max(i["width"], i["height"]) > 1.6 * min(i["width"], i["height"])]  # not the little antler icons
    res = []
    for k, head in enumerate(heads):
        top = head[1] - 14
        bottom = (heads[k + 1][1] - 14) if k + 1 < len(heads) else page.rect.height
        table_x = head[0] - 20
        charge_x = min((w[0] for w in words if w[4] == "Charge" and abs(w[1] - head[1]) < 30 and w[0] > head[0]), default=head[0] + 40) - 3
        rows = []
        left = [w for w in words if w[0] < table_x and top <= w[1] < bottom and w[0] > 30]
        for yc, ws in lines_of([w for w in words if w[0] >= table_x and head[3] < w[1] < bottom], 3.5):
            ws = [w for w in ws if w[4] not in ("c", "C")]
            name = " ".join(w[4] for w in ws if w[2] < charge_x)
            nums = [re.sub(r"[cC*]+$", "", w[4]) for w in ws if w[2] >= charge_x]
            if name and len(nums) == 4 and all(NUM.match(x) for x in nums):
                rows.append((name, [float(x) for x in nums]))
        bullets = []
        for yc, ws in lines_of(left, 3.5):
            text = " ".join(w[4] for w in ws)
            m = re.match(r"^(\d+)-grain\s+(.*)$", text)
            if m:
                bullets.append({"grain": float(m[1]), "type": m[2], "y": yc, "coal": None, "xref": None})
            elif bullets:
                c = re.search(r"C\.?O\.?A\.?L\.?\s*([\d.]+)", text)
                if c and bullets[-1]["coal"] is None:
                    bullets[-1]["coal"] = float(c[1])
        if not bullets:  # newer layout: a table above the powders (weight, type, SKU, sectional density, G1 BC, cartridge length), no pictures
            prev = heads[k - 1][1] if k else 0
            above = [w for w in words if prev < w[1] < head[1] - 2]
            for yc, ws in lines_of(above, 3.5):
                m = re.match(r"^(\d+)\s*gr\s+(.+?)\s+\d{4,6}\s+[\d.]+\s+[\d.]+\s+([\d.]+)", " ".join(w[4] for w in ws))
                if m:
                    bullets.append({"grain": float(m[1]), "type": m[2], "y": yc, "coal": float(m[3]), "xref": None})
        for b in bullets:  # the picture above the text line, the nearest one
            cand = [i for i in images if top - 5 <= i["bbox"][1] and i["bbox"][3] <= b["y"] + 4 and i["bbox"][0] < table_x]
            if cand:
                b["xref"] = max(cand, key=lambda i: i["bbox"][3])["xref"]
        res.append((head[1], bullets, rows))
    return res


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pdfs", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("barnes_samples.csv"))
    ap.add_argument("--lengths", type=Path, default=Path("barnes_lengths.csv"))
    ap.add_argument("--images", type=Path)
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    records = Records(args.data)
    if args.images:
        args.images.mkdir(parents=True, exist_ok=True)
    rows, lengths, miss, powders = [], [], {}, {}
    for pdf in sorted(args.pdfs.glob("*.pdf")):
        if "Muzzleloader" in pdf.name:
            continue
        doc = fitz.open(pdf)
        first = doc[0].get_text()
        title = cartridge_title(doc[0])
        head = parse_header(first)
        cal = find_caliber(records, title)
        if not cal:
            miss[pdf.name] = title
            continue
        dia = cal.get("G1")
        if not (dia and head["barrel"]):
            miss[pdf.name] = f"{title}: no diameter or barrel"
            continue
        seen = set()
        for page in doc:
            for _, bullets, table in blocks(page):
                for bi, b in enumerate(bullets):
                    ratio = silhouette_ratio(doc, b["xref"]) if b["xref"] else None
                    name = f"Barnes {b['type']}"
                    meas = ratio[0] * dia if ratio else None
                    if ratio:
                        key = (cal["cipname"], b["grain"], b["type"])
                        if key not in seen:
                            seen.add(key)
                            ref = records.bullet("Barnes", b["type"], b["grain"], dia)[0]
                            img = f"{pdf.stem}_{int(b['grain'])}_{re.sub(r'[^A-Za-z0-9]+', '-', b['type'])}.png"
                            lengths.append({"caliber": cal["cipname"], "bullet": name, "grain": b["grain"], "diameter_mm": dia,
                                            "length_mm": f"{meas:.2f}", "ratio": f"{ratio[0]:.3f}", "coal_in": b["coal"] or "", "image": img,
                                            "reference_mm": f"{ref:.2f}", "used": "yes" if abs(meas / ref - 1) < MAX_DEVIATION else "no"})
                            if args.images:
                                cv2.imwrite(str(args.images / img), ratio[1])
                    if bi:  # the loads are those of the table, written once
                        continue
                    length, source, tail = records.bullet("Barnes", b["type"], b["grain"], dia)
                    if meas and abs(meas / length - 1) < MAX_DEVIATION:  # the picture counts only when it is near the record or the estimate
                        length, source, tail = meas, "photo:barnes-pdf", 2 if re.search(r"BT|Boat", b["type"]) else 0
                    col = (b["coal"] or 0) * IN
                    if not col:
                        continue
                    for pname, (c0, v0, c1, v1) in table:
                        powders[pname] = powders.get(pname, 0) + 1
                        for kind, ch, v in (("start", c0, v0), ("max", c1, v1)):
                            rows.append({"powder": powder_name(pname), "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""),
                                         "bullet": name, "bullet_gr": b["grain"], "bullet_dia_mm": dia, "bullet_len_mm": f"{length:.2f}",
                                         "len_source": source, "tail_type": tail, "col_mm": f"{col:.2f}",
                                         "barrel_mm": f"{float(head['barrel']) * IN:.0f}",
                                         "twist_mm": f"{float(head['twist']) * IN:.0f}" if head["twist"] else "", "kind": kind,
                                         "charge_g": round(ch * GRAIN_G, 4), "velocity_ms": round(v * 0.3048, 1), "pressure_bar": "",
                                         "primer": head["primer"]})
    for path, data in ((args.output, rows), (args.lengths, lengths)):
        with open(path, "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, list(data[0].keys()))
            w.writeheader()
            w.writerows(data)
    print(f"{len(rows)} samples -> {args.output}, {len(lengths)} bullets measured -> {args.lengths}", file=sys.stderr)
    print("cartridges not found:", "; ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    print("powders:", ", ".join(sorted(powder_name(p) for p in powders)), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
