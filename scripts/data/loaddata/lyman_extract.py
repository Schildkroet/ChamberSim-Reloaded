#!/usr/bin/env python3
"""Reads the load data of the Lyman Reloading Handbook, 50th edition (scanned PDF with an OCR text layer) into a CSV.

    lyman_extract.py "lyman 50th reloading book.pdf" [-o lyman_loads.csv]

Per cartridge: an introduction page (test components: case, primer, trim length; test specifications: firearm, barrel length, twist, groove diameter,
"Velocity Only" or "Velocity & Pressure") and data pages with one table per bullet: header (weight and type or cast bullet number and alloy, cartridge
length "OAL", BC, SD) and a row per powder: suggested starting load (grains, velocity fps, pressure) and maximum load (the shaded columns). Pressure is
C.U.P. or P.S.I. (per page). One output row per powder and load (start and max). Units are those of the book (inch, grain, fps, C.U.P. / P.S.I.).
The OCR is poor in places (lost decimal points and commas): a row is kept only when its numbers are consistent (start load below maximum load,
velocity rising, plausible pressures); the others are counted in `rejected` on stderr.
"""
import argparse
import csv
import re
import sys
from pathlib import Path

import fitz

NUM = re.compile(r"^[*]*(\d{1,3}(?:[.,]\d{1,3})?|\d{4,6})\+?$")
HEAD_WEIGHT = re.compile(r"^(\d{2,3})\s*gr\.?\s*(.*)$")
BC = re.compile(r"(?:BC|Be|8C|BG)\s*:\s*(\.?\d+)")
SD = re.compile(r"SD\s*:\s*(\.?\d+)")
OAL = re.compile(r"(\d\.\d{3})\s*[\"']*\s*[O0]\s*A\s*[LI]")


def lines_of(words, tol=3.5):
    out = []
    for w in sorted(words, key=lambda w: ((w[1] + w[3]) / 2, w[0])):
        yc = (w[1] + w[3]) / 2
        if out and abs(out[-1][0] - yc) <= tol:
            out[-1][1].append(w)
        else:
            out.append([yc, [w]])
    return [(yc, sorted(ws, key=lambda w: w[0])) for yc, ws in out]


def number(token: str) -> float | None:
    t = token.strip("*+").replace(",", "")
    try:
        return float(t)
    except ValueError:
        return None


def toc_titles(doc) -> dict[int, str]:
    """{printed page: cartridge name} of the table of contents (pdf pages 6-9; two columns of 'name ..... page')."""
    out = {}
    for i in range(5, 10):
        page = doc[i]
        words = page.get_text("words")
        mid = page.rect.width / 2
        for _, ws in lines_of(words, 4.0):
            for half in (0, 1):
                part = [w[4] for w in ws if (w[0] < mid - 5) == (half == 0)]
                if len(part) < 2:
                    continue
                m = re.match(r"^(.*?)[\s.'\":,;_|-]*[.,]?(\d{3})$", " ".join(part))
                if m and not re.search(r"Data Section|Introduction|Basics|Reloading (Rifle|Handgun)|Index|Cartridges$", m[1]):
                    name = re.sub(r"\s+", " ", re.sub(r"[.\s'\"]*$", "", m[1])).strip(" .,'\"")
                    if 3 < len(name) < 70 and re.search(r"[A-Za-z0-9]", name):
                        out.setdefault(int(m[2]), name)
    return out


def parse_spec(text: str) -> dict:
    def find(pat):
        m = re.search(pat, text)
        return m[1].strip() if m else ""
    barrel = re.search(r"Barrel Length[^\n\"]*?(\d+(?:\.\d+)?|IS)\s*[^\d\s]{0,3}\"", text)
    inches = ""
    if barrel:
        raw = "15" if barrel[1] == "IS" else barrel[1]
        # the OCR glues fractions to the number (25\u00bc" read as 2514, 4\u2075/\u2081\u2086 as 41*6): the barrel has at most 40 inches, so keep the leading digits that do
        while "." not in raw and len(raw) > 1 and float(raw) > 40:
            raw = raw[:-1]
        inches = raw
    twist = re.search(r"Twist[ .'\"_]*\(?1\s*[-\u00b7.:_~]\s*(\d+(?:\.\d+)?)", text)
    return {"firearm": find(r"Firearm Used[ .]+([^\n]+)"), "barrel_in": inches, "twist_in": twist[1] if twist else "",
            "groove_in": find(r"Groove Dia[ .]+(\.\d+)"), "case": find(r"Cases[ .]+([^\n]+)"), "trim_in": find(r"Trim-to Length[ .]+(\d\.\d+)"),
            "primer": find(r"Primers[ .]+([^\n]+)"), "primer_size": find(r"Primer Size[ .]+([^\n]+)"),
            "bullets_used": ";".join(f"{m[3]}:{m[1]} {m[2].strip()}" for m in re.finditer(
                r"([A-Z][A-Za-z]+)\s+([A-Za-z0-9/ .-]{2,22}?)\s*#\s*(\w+)\s*,?\s*(\d{2,3})\s*gr", text) if False) or ";".join(
                f"{m[4]}:{m[1]} {m[2].strip()}" for m in re.finditer(r"([A-Z][A-Za-z]+)\s+([A-Za-z0-9/ .-]{2,22}?)\s*#\s*(\w+)\s*,?\s*(\d{2,3})\s*gr", text)),
            "velocity_only": "1" if re.search(r"Velocity\s+Only", text) else "0"}


def parse_blocks(page):
    """[(header text, bc, sd, oal, unit, rows)]; rows = (powder, [numbers])."""
    words = [w for w in page.get_text("words") if w[1] > 90]
    half = page.rect.width / 2
    res = []
    for side in (0, 1):
        ws = [w for w in words if (w[0] < half - 8) == (side == 0) and not (side == 1 and w[0] > page.rect.width - 60)]
        lines = lines_of(ws)
        i = 0
        while i < len(lines):
            text = " ".join(w[4] for w in lines[i][1])
            m = re.search(r"(?:BC|Be|8C|BG)\s*:", text)
            # the header is the line (or the two lines) that carries BC and the title: "90 gr. Jacketed HP  BC: .115" / "SD: .102 1.010 OAL"
            if not m:
                i += 1
                continue
            head, y0 = text, lines[i][0]
            bc = BC.search(text)
            j = i + 1
            nxt = " ".join(" ".join(w[4] for w in lines[q][1]) for q in (j, j + 1) if q < len(lines))
            oal = OAL.search(nxt.replace(" ", " ")) or OAL.search(text)
            sd = SD.search(nxt) or SD.search(text)
            # the title line may stand above (cast bullets: "#356242" then "120 gr. (#2 Alloy) 1.065" OAL")
            title_line = text[: m.start()].strip()
            if not title_line and i > 0:
                title_line = " ".join(w[4] for w in lines[i - 1][1])
            rows, unit = [], ""
            k = j
            while k < len(lines):
                t = " ".join(w[4] for w in lines[k][1])
                if re.search(r"(?:BC|Be|8C|BG)\s*:", t) or t.startswith("Note"):
                    break
                if re.search(r"C\.?U\.?P|cur|P\.?S\.?I", t) and not unit:
                    unit = "PSI" if re.search(r"P\.?S\.?I", t) else "CUP"
                toks = [w[4] for w in lines[k][1]]
                if k > j + 1 and len(toks) >= 4:
                    nums = [t for t in toks if NUM.match(t)]
                    name = " ".join(t for t in toks if not NUM.match(t) or t.startswith("#")).strip("*").strip()
                    rows.append((name, toks))
                k += 1
            res.append((title_line, text, nxt, bc[1] if bc else "", sd[1] if sd else "", oal[1] if oal else "", unit, rows, side))
            i = k
    return res


def clean(token: str) -> str:
    """One OCR token: letters read for digits (O, l) and trailing junk (17,400C, 2885-, 1879a) removed from number-like tokens."""
    t = token.strip()
    if re.match(r"^[0-9][0-9OolI.,]*[^\d]{0,2}$", t) or re.match(r"^[-+]?\d[\dOo.,]*\+?[^\d+]?$", t):
        t = re.sub(r"[Oo]", "0", t)
        t = re.sub(r"(?<=\d)[lI](?=\d)", "1", t)
        t = re.sub(r"[^0-9.,+]+$", "", t)
    return re.sub(r"[^0-9.,+*#A-Za-z/()'-]+$", "", t)


DASH = re.compile(r"^[-_.:|~\u2014\u2013]+$")


def parse_row(toks: list[str], velocity_only: bool):
    """(powder, start grains, start fps, start pressure, max grains, max fps, max pressure, compressed) or None.
    A row with four numbers (or dashes for the pressures) has no pressures."""
    raw = [t for t in toks if not DASH.match(t.strip()) and t.strip() not in ("I", "j", "l", ",", "'")]
    toks = [clean(t) for t in raw]
    toks = [t for t in toks if t]
    nums = [i for i, t in enumerate(toks) if NUM.match(t) and not t.startswith("#")]
    first = nums[0] if nums else 0
    if len(nums) in (5, 7) and nums[0] == 0:  # a powder named by a number (231, 760, 2400)
        nums = nums[1:]
        first = 1
    if len(nums) not in (4, 6) or nums[0] != first:
        return None
    name = re.sub(r"^[^A-Za-z0-9]+", "", " ".join(toks[:first])).strip("*+ ").strip()
    if not name:
        return None
    vals = [number(toks[i]) for i in nums]
    if any(v is None for v in vals):
        return None
    compressed = toks[nums[3 if len(nums) == 6 else 2]].endswith("+")
    if len(nums) == 4:
        sg, v1, mg, v2 = vals
        p1 = p2 = None
    else:
        sg, v1, p1, mg, v2, p2 = vals
    gi = (0, 3 if len(nums) == 6 else 2)
    # a lost decimal point: 45 for 4.5 (the other grain value has one, and the two differ by a factor of ten)
    if mg > 3 * sg and mg >= 10 and "." not in toks[nums[gi[1]]]:
        mg /= 10
    elif sg > 3 * mg and sg >= 10 and "." not in toks[nums[0]]:
        sg /= 10
    if not (sg < mg and 400 < v1 < v2 < 5200 and (p1 is None or (3000 < p1 < 90000 and 3000 < p2 < 90000))):
        return None
    return name, sg, v1, p1, mg, v2, p2, compressed


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--out", type=Path, default=Path("lyman_loads.csv"))
    args = ap.parse_args()
    doc = fitz.open(args.pdf)
    titles = toc_titles(doc)
    spec, cart, out, rejected, rows_seen = {}, "", [], 0, 0
    for pno, page in enumerate(doc):
        if pno < 117:
            continue
        text = page.get_text()
        if ("Test Components" in text or "Test Specifications" in text) and "Important: Tes" not in text:
            printed = pno  # printed page = pdf page - 1 (1-based: pno + 1 - 1)
            cart = titles.get(printed) or {140: "223 Remington", 247: "30-06 Springfield", 284: "8mm Mauser (8x57mm)"}.get(printed, f"page {printed}")
            spec = parse_spec("\n".join(" ".join(w[4] for w in ws) for _, ws in lines_of(page.get_text("words"), 3.0)))
            continue
        if not cart:
            continue
        for title, head, nxt, bc, sd, oal, unit, rows, side in parse_blocks(page):
            for name, toks in rows:
                rows_seen += sum(1 for t in toks if NUM.match(clean(t))) >= 4
                r = parse_row(toks, False)
                if not r:
                    rejected += sum(1 for t in toks if NUM.match(clean(t))) >= 4
                    continue
                out.append({"page": pno, "cartridge": cart, **{k: v for k, v in spec.items()}, "bullet": title, "bc": bc, "sd": sd,
                            "oal_in": oal, "pressure_unit": unit, "powder": r[0], "start_gr": r[1], "start_fps": r[2], "start_pressure": r[3] or "",
                            "max_gr": r[4], "max_fps": r[5], "max_pressure": r[6] or "", "compressed": int(r[7])})
    if out:
        with args.out.open("w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=list(out[0]))
            w.writeheader()
            w.writerows(out)
    print(f"{len(out)} rows ({rejected} of {rows_seen} candidate rows rejected), {len({r['cartridge'] for r in out})} cartridges -> {args.out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
