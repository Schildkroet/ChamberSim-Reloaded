#!/usr/bin/env python3
"""Reads the load tables of the Hornady Handbook of Cartridge Reloading (10th edition, PDF) into a CSV, and the bullet pictures into PNG files.

    hornady_handbook_extract.py <handbook.pdf> [-o hornady_handbook_loads.csv] [--bullets hornady_handbook_bullets.csv] [--images dir]

A cartridge starts with a page of specifications (test firearm, barrel, case, primer, maximum cartridge length, bullet diameter). Then come
tables per bullet weight: the bullets of that weight (item number, cartridge length, G1/G7 ballistic coefficient) and a table of velocity steps
(fps) against the charge (gr) of each powder. The cell of a maximum load is filled red (a vector rectangle behind the number).
The loads CSV has one row per cell: cartridge, bullet (every bullet of the group shares the table), powder, velocity, charge, maximum flag.
The page layout is read with PyMuPDF (fitz). Units are those of the book (inch, grain, fps).
"""
import argparse
import csv
import re
import sys
from pathlib import Path

import fitz

BULLET = re.compile(
    r"(?P<mass>\d+(?:\.\d+)?) gr\. (?P<name>[^\n]+?)\n(?P<rest>(?:[^\n]*\n)*?)Item No\. (?P<item>\w+)\n(?:C\.O\.L\.: (?P<col>[\d.]+)\"\n)?"
    r"(?:G1 B\.C\.: (?P<g1>[\d.]+)\n?)?(?:G7 B\.C\.: (?P<g7>[\d.]+))?")
SPEC = {"barrel": r"Barrel:[ .]*([^\n]+?)(?:\s{2,}|\n)", "firearm": r"(?:Rifle|Handgun):[ .]*([^\n]+?)(?:\s{2,}|\n)"}
CHARGE = re.compile(r"^(\d+(?:\.\d+)?) gr\.$")
VELOCITY = re.compile(r"^\d{3,4}$")


def is_red(rgb) -> bool:
    return rgb[0] > 190 and rgb[1] < 90 and rgb[2] < 90


def running_title(page) -> str:
    """The cartridge name printed rotated in the margin of every page of the cartridge's section."""
    for b in page.get_text("dict")["blocks"]:
        for l in b.get("lines", []):
            if abs(l["dir"][0]) < 0.5 and l["spans"] and l["spans"][0]["size"] > 14:
                return re.sub(r"\s+", " ", "".join(s["text"] for s in l["spans"]).replace("\xa0", " ")).strip()
    return ""


def spec_of(page, text: str) -> dict | None:
    """The test data of a specification page (it carries 'Bullet Diameter' and 'Barrel')."""
    if "Bullet Diameter" not in text or "Barrel:" not in text:
        return None
    flat = re.sub(r"[ .]{3,}", " ", text.replace("\xa0", " "))
    spec = {"cartridge": running_title(page)}
    for key, rx in (("barrel", r"Barrel:\s*([^\n]+)"), ("firearm", r"(?:Rifle|Handgun):\s*([^\n]+)"),
                    ("case", r"Case:\s*([^\n]+)"), ("primer", r"Primer:\s*([^\n]+)"),
                    ("max_col_in", r"Maximum COL:\s*([\d.]+)"), ("case_len_in", r"Max\. Case Length:\s*([\d.]+)"),
                    ("trim_len_in", r"Case Trim Length:\s*([\d.]+)"), ("diameter_in", r"Bullet Diameter:\s*([\d.]+)")):
        m = re.search(rx, flat)
        spec[key] = m[1].strip() if m else ""
    # the labels and values of both columns alternate; keep what stands before the second column's label
    for key in ("barrel", "firearm", "case", "primer"):
        spec[key] = re.split(r"\s+(?:Bullet Diameter|Maximum COL|Max\. Case Length|Case Trim Length)", spec[key])[0].strip()
    return spec


def table_of(page):
    """[(powder, velocity, charge, is_max)] of the velocity table of a page, or None."""
    words = page.get_text("words")
    head = next((w for w in words if w[4] == "POWDER"), None)
    if head is None:
        return None
    pix = page.get_pixmap(dpi=72)
    cols = sorted((w for w in words if abs(w[1] - head[1]) < 3 and VELOCITY.match(w[4]) and w[0] > head[2]), key=lambda w: w[0])
    if not cols:
        return None
    centers = [(w[0] + w[2]) / 2 for w in cols]
    stop = next((w[1] for w in words if w[4] == "Create" and w[1] > head[1]), page.rect.height)
    lines: dict[int, list] = {}
    for w in words:
        if head[3] - 1 < w[1] < stop:
            lines.setdefault(round(w[1] / 3), []).append(w)
    out = []
    for key in sorted(lines):
        ws = sorted(lines[key], key=lambda w: w[0])
        name = " ".join(w[4] for w in ws if w[2] < cols[0][0] - 8)
        if not name:
            continue
        # "4.2 gr." arrives as two words ("4.2", "gr.") that share a column
        nums = [w for w in ws if w[0] >= cols[0][0] - 25 and re.fullmatch(r"\d+(?:\.\d+)?", w[4])]
        for w in nums:
            c = (w[0] + w[2]) / 2
            i = min(range(len(centers)), key=lambda k: abs(centers[k] - c))
            if abs(centers[i] - c) > 22:
                continue
            # a maximum load has a red cell; sample the left edge of the cell (the text is white on red, black on white)
            x, y = int(w[0]) - 3, int((w[1] + w[3]) / 2)
            out.append((name, int(cols[i][4]), float(w[4]), is_red(pix.pixel(x, y)) if 0 <= x < pix.width and 0 <= y < pix.height else False))
    return out


def group_bullets(page, text: str) -> list[dict]:
    """The bullets pictured above the table (mass, name, item number, cartridge length, ballistic coefficients)."""
    bullets = []
    for b in page.get_text("blocks"):
        t = b[4].replace("®", "").replace("™", "")
        if m := BULLET.search(t):
            name = (m["name"] + " " + " ".join(l for l in m["rest"].splitlines() if l.strip())).strip()
            bullets.append({"mass_gr": m["mass"], "name": name, "item": m["item"], "col_in": m["col"] or "", "g1": m["g1"] or "",
                            "g7": m["g7"] or "", "rect": fitz.Rect(b[:4])})
    return bullets


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--out", type=Path, default=Path("hornady_handbook_loads.csv"))
    ap.add_argument("--bullets", type=Path, default=Path("hornady_handbook_bullets.csv"))
    ap.add_argument("--images", type=Path, help="write the side view of every bullet picture here (PNG)")
    args = ap.parse_args()

    doc = fitz.open(args.pdf)
    specs: dict[str, dict] = {}
    loads, bullets_out, seen, manifest = [], [], set(), []
    for pno, page in enumerate(doc):
        text = page.get_text()
        if s := spec_of(page, text):
            specs[s["cartridge"]] = s
            continue
        table = table_of(page)
        if not table:
            continue
        cart = running_title(page)
        spec = specs.get(cart, {})
        spec = {**spec, "cartridge": cart}
        mass_hdr = re.search(r"(\d+(?:\.\d+)?(?:-\d+(?:\.\d+)?)?) GRAIN BULLETS", text)
        sd = re.search(r"SECTIONAL DENSITY:\s*([\d.\-]+)", text)
        bullets = group_bullets(page, text)
        group = f"p{pno + 1}"
        for b in bullets:
            bullets_out.append({"page": pno + 1, "cartridge": spec.get("cartridge", ""), "group": mass_hdr[1] if mass_hdr else "",
                                "diameter_in": spec.get("diameter_in", ""), **{k: v for k, v in b.items() if k != "rect"}})
        names = " | ".join(f"{b['mass_gr']} gr. {b['name']}" for b in bullets)
        for powder, fps, gr, is_max in table:
            loads.append({"page": pno + 1, "cartridge": spec.get("cartridge", ""), "barrel": spec.get("barrel", ""),
                          "firearm": spec.get("firearm", ""), "case": spec.get("case", ""), "primer": spec.get("primer", ""),
                          "diameter_in": spec.get("diameter_in", ""), "max_col_in": spec.get("max_col_in", ""),
                          "bullet_group_gr": mass_hdr[1] if mass_hdr else "", "bullets": names,
                          "col_in": next((b["col_in"] for b in bullets if b["col_in"]), ""), "powder": powder, "velocity_fps": fps,
                          "charge_gr": gr, "maximum": int(is_max), "sectional_density": sd[1] if sd else "", "group": group})
        if args.images:
            args.images.mkdir(parents=True, exist_ok=True)
            drawings = page.get_drawings()
            for b in bullets:
                if (b["item"], pno) in seen:
                    continue
                seen.add((b["item"], pno))
                xs = sorted({round(o["rect"].x0) for o in bullets if abs(o["rect"].y0 - b["rect"].y0) < 5})
                right = next((x for x in xs if x > b["rect"].x0 + 20), b["rect"].x0 + 128)
                parts = [d["rect"] for d in drawings if d["rect"].width < 125 and d["rect"].height < 40 and d["rect"].x0 > b["rect"].x0 - 4
                         and d["rect"].x1 < right - 2 and b["rect"].y0 - 45 < d["rect"].y0 and d["rect"].y1 < b["rect"].y0 + 1]
                if not parts:
                    continue
                clip = fitz.Rect(parts[0])
                for r in parts[1:]:
                    clip |= r
                clip = fitz.Rect(clip.x0 - 1, clip.y0 - 1, clip.x1 + 1, clip.y1 + 1) & page.rect
                if clip.height > 5:
                    name = re.sub(r"[^A-Za-z0-9]+", "_", f"{b['item']}_{b['mass_gr']}gr_{b['name']}").strip("_")
                    page.get_pixmap(clip=clip, dpi=300).save(args.images / f"{name}.png")
                    manifest.append({"file": f"{name}.png", "brand": "Hornady", "title": f"{b['mass_gr']} gr. {b['name']}",
                                     "diameter_in": spec.get("diameter_in", ""), "weight_gr": b["mass_gr"]})

    for path, rows in ((args.out, loads), (args.bullets, bullets_out)):
        if not rows:
            continue
        with path.open("w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0]))
            w.writeheader()
            w.writerows(rows)
    if manifest:  # the input of measure_side_photos.py
        with (args.images / "manifest.csv").open("w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=list(manifest[0]))
            w.writeheader()
            w.writerows(manifest)
    print(f"{len(loads)} load cells, {len({(r['cartridge']) for r in loads})} cartridges, {len(bullets_out)} bullet entries", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
