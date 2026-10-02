#!/usr/bin/env python3
"""Collects calibers that have no C.I.P. sheet in cip_calibers.csv from Wikipedia's cartridge infoboxes.

    wiki_calibers.py titles.txt [-o wiki_calibers.csv] [--pdf-dir dir] [--cache dir]

titles.txt: one Wikipedia article per line: "title", or "title | infobox name | our name" when the article covers several
cartridges (the infobox is picked by its name) or the cartridge should be named differently. For every article:
- a cited C.I.P. TDCC sheet (bobp.cip-bobp.org/uploads/tdcc/...) is downloaded into --pdf-dir; run cip_extract.py over it
  afterwards for the full record;
- otherwise the infobox values are written to the CSV in the columns of cip_calibers.csv: bullet, neck, shoulder, base and
  rim diameter, case and overall length, case capacity (grain H2O), maximum pressure and shoulder angle. Lengths of body and
  shoulder (L1, L2) are not in infoboxes and stay empty.
Units are converted to mm / bar. Be polite: one request per second.
"""
import argparse
import csv
import math
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

UA = {"User-Agent": "GRTReloaded-data/1.0 (personal project; contact via GitHub)"}
IN, PSI = 25.4, 0.0689476
CIPPDF = re.compile(r"https?://bobp\.cip-bobp\.org/uploads/tdcc/[\w./%+-]+?\.pdf", re.I)


def fetch(url: str, cache: Path | None) -> str:
    f = cache / re.sub(r"[^\w.-]", "_", url.split("//")[1])[:180] if cache else None
    if f and f.exists():
        return f.read_text(encoding="utf-8")
    for attempt in range(6):  # Wikipedia answers 429 when requests come too fast: wait as told and try again
        time.sleep(2)
        try:
            text = urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=30).read().decode("utf-8", "replace")
            break
        except urllib.error.HTTPError as e:
            if e.code not in (429, 500, 502, 503) or attempt == 5:
                raise
            time.sleep(min(int(e.headers.get("Retry-After", 0) or 0) or 20 * (attempt + 1), 120))
    if f:
        f.write_text(text, encoding="utf-8")
    return text


def wikitext(title: str, cache: Path | None) -> str:
    text = fetch("https://en.wikipedia.org/w/index.php?action=raw&title=" + urllib.parse.quote(title.replace(" ", "_")), cache)
    m = re.match(r"#REDIRECT\s*\[\[([^\]#|]+)", text, re.I)
    return wikitext(m[1], cache) if m else text


def search_title(query: str, cache: Path | None) -> str | None:
    """First article found for a query (articles move and redirects point into other articles)."""
    import json
    data = json.loads(fetch("https://en.wikipedia.org/w/api.php?action=opensearch&limit=1&format=json&search=" + urllib.parse.quote(query), cache))
    return data[1][0] if data and data[1] else None


def split_top(body: str) -> list[str]:
    """Splits a template body at the pipes that are not inside nested {{ }} or [[ ]]."""
    parts, depth, cur, i = [], 0, "", 0
    while i < len(body):
        two = body[i:i + 2]
        if two in ("{{", "[["):
            depth += 1
            cur += two
            i += 2
        elif two in ("}}", "]]"):
            depth -= 1
            cur += two
            i += 2
        elif body[i] == "|" and depth == 0:
            parts.append(cur)
            cur = ""
            i += 1
        else:
            cur += body[i]
            i += 1
    return parts + [cur]


def infoboxes(text: str) -> list[dict]:
    """The {{Infobox firearm cartridge}} templates of an article as dicts."""
    boxes = []
    for m in re.finditer(r"\{\{\s*Infobox firearm cartridge", text, re.I):
        depth, j = 0, m.start()
        while j < len(text):
            if text.startswith("{{", j):
                depth += 1
                j += 2
            elif text.startswith("}}", j):
                depth -= 1
                j += 2
                if depth == 0:
                    break
            else:
                j += 1
        fields = {}
        for part in split_top(text[m.start() + 2:j - 2])[1:]:
            k, _, v = part.partition("=")
            fields[k.strip().lower()] = v.strip()
        boxes.append(fields)
    return boxes


def value(raw: str, kind: str, inch_below: float = 0.0) -> float | None:
    """A number from an infobox value, converted to mm (kind "len"), bar ("bar"), grain ("gr") or degrees ("deg").
    A length without unit is taken as inch when it is below `inch_below` (dimensions that are mm in a metric box are above)."""
    raw = re.sub(r"<ref.*?(</ref>|/>)", "", raw, flags=re.S)
    m = re.search(r"\{\{(?:convert|cvt)\|\s*(\d*\.?\d[\d,]*)\s*\|\s*([a-zA-Z0-9]+)", raw)
    num, unit = (m[1], m[2].lower()) if m else (None, "")
    if num is None:
        m = re.search(r"(\d*\.?\d+)\s*(mm|in|inch|psi|bar|mpa|gr|grains?|cm3|cc|ml)?", raw)
        if not m:
            return None
        num, unit = m[1], (m[2] or "").lower()
    x = float(num.replace(",", ""))
    if kind == "len":
        if unit in ("in", "inch", "inches"):
            return x * IN
        if unit == "mm":
            return x
        if unit == "cm":
            return x * 10
        return x * IN if x < inch_below else x
    if kind == "bar":
        return {"psi": x * PSI, "mpa": x * 10, "bar": x}.get(unit)
    if kind == "gr":
        # capacity: grain H2O; cm3 / ml when stated, or when a unit-less number is too small for grains
        return x * 15.4324 if unit in ("cm3", "cc", "ml", "cm") or (not unit and x < 6 and not re.search(r"gr", raw)) else x
    return x


def row(box: dict, name: str, source: str) -> dict:
    r = {"cipname": name, "origin": "en.wikipedia.org", "source": source}
    for key, col, kind, below in (("bullet", "G1", "len", 1.1), ("neck", "H1", "len", 1.1), ("shoulder", "P2", "len", 1.1),
                                  ("base", "P1", "len", 1.1), ("rim_dia", "R1", "len", 1.1), ("rim_thick", "E", "len", 0.3),
                                  ("shoulder_angle", "alpha", "deg", 0), ("case_length", "L3", "len", 6.0), ("length", "L6", "len", 9.0)):
        v = value(box.get(key, ""), kind, below) if box.get(key) else None
        if v is not None:
            r[col] = f"{v:.2f}"
    if "H1" in r:
        r["H2"] = r["H1"]
    if box.get("case_capacity"):
        v = value(box["case_capacity"], "gr")
        if v:
            r["V"] = f"{v:.1f}"
    for sfx in ("", "2"):
        method = box.get("pressure_method" + sfx, "")
        p = value(box.get("max_pressure" + sfx, ""), "bar") if box.get("max_pressure" + sfx) else None
        if p and ("Pmax" not in r or "c.i.p" in method.lower()):
            r["Pmax"] = f"{p:.0f}"
            r["standard"] = "CIP" if "c.i.p" in method.lower() else "SAAMI"
    case = box.get("case_type", "").lower()
    r["ciptype"] = ("3 belted" if "belted" in case else "2 rimmed" if re.search(r"rimmed|semi", case) else "1 rimless"
                    if "rimless" in case or "rebated" in case else "")
    origin = re.sub(r"\[\[|\]\]", "", box.get("origin", "")).strip()
    r["ciporigin"] = {"United States": "US", "Germany": "DE", "Switzerland": "CH", "Sweden": "SE", "Finland": "FI"}.get(origin, "")
    year = re.search(r"\d{4}", box.get("design_date", ""))
    r["cipdate"] = year[0] + "-01-01" if year else ""
    note = "Dimensions from the Wikipedia infobox (published cartridge drawing values)"
    if "G1" in r:  # no bore data in infoboxes: the bore cross-section of the bullet diameter (the simulation needs it)
        r["c_Q"] = f"{math.pi / 4 * float(r['G1']) ** 2:.2f}"
        note += "; bore area from the bullet diameter"
    if "V" not in r and all(k in r for k in ("L3", "P1", "G1")):  # no capacity in the article: estimate it from the outline
        from cip_estimate_v import WALL, HEAD, capacity
        l3, g1 = float(r["L3"]), float(r["G1"])
        t = {"P1": r["P1"], "L3": r["L3"], "H2": r.get("H2", r["P1"]), "G1": r["G1"]}
        if "P2" in r and "H1" in r:  # bottleneck: neck about one bullet diameter long, shoulder from the angle (35 deg if unknown)
            half = float(r.get("alpha") or 35) / 2
            l2 = l3 - g1
            t |= {"L2": f"{l2:.2f}", "L1": f"{l2 - (float(r['P2']) - float(r['H1'])) / 2 / math.tan(math.radians(half)):.2f}",
                  "P2": r["P2"], "H1": r["H1"]}
        v = capacity(t, WALL, HEAD)
        if v:
            r["V"] = f"{v:.1f}"
            note += "; case capacity estimated from the outline"
    r["descr"] = note
    return r


COLUMNS = ["cipname", "altname", "standard", "ciporigin", "ciptype", "cipdate", "origin", "descr", "L3", "L6", "R1", "E", "P1", "P2", "H1", "H2", "alpha",
           "G1", "c_Q", "Pmax", "V", "source"]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("titles", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("wiki_calibers.csv"))
    ap.add_argument("--pdf-dir", type=Path)
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    for d in (args.cache, args.pdf_dir):
        if d:
            d.mkdir(parents=True, exist_ok=True)
    rows = []
    for line in args.titles.read_text(encoding="utf-8").splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        title, _, rest = (p.strip() for p in line.partition("|"))
        cart, _, own_name = (p.strip() for p in rest.partition("|"))
        try:
            text = wikitext(title, args.cache)
        except OSError:
            text = ""
        boxes = infoboxes(text)
        if not boxes:  # unknown or renamed article: take the first search hit
            found = search_title(title + " cartridge", args.cache)
            if found:
                title = found
                text = wikitext(title, args.cache)
                boxes = infoboxes(text)
        pdfs = sorted(set(CIPPDF.findall(text)))
        if args.pdf_dir and pdfs:
            for u in pdfs:
                dest = args.pdf_dir / ("wiki__" + u.rsplit("/", 1)[1])
                if not dest.exists():
                    time.sleep(0.5)
                    try:
                        dest.write_bytes(urllib.request.urlopen(urllib.request.Request(u, headers=UA), timeout=40).read())
                    except OSError as e:  # links in articles go stale
                        print(f"{title}: {u}: {e}", file=sys.stderr)
        plain = lambda t: re.sub(r"[^a-z0-9]", "", t.lower())  # noqa: E731
        box = next((b for b in boxes if not cart or plain(cart) in plain(b.get("name", ""))), boxes[0] if boxes else None)
        if not box:
            print(f"{title}: no infobox", file=sys.stderr)
            continue
        name = own_name or re.sub(r"\[\[|\]\]|<.*?>", "", cart or box.get("name", title)).strip()
        r = row(box, name, "https://en.wikipedia.org/wiki/" + title.replace(" ", "_"))
        r["cip_sheet"] = ", ".join(pdfs)
        rows.append(r)
        print(f"{title}: {len(boxes)} infobox, {len(pdfs)} CIP sheet(s)", file=sys.stderr)
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, [*COLUMNS, "cip_sheet"], extrasaction="ignore")
        w.writeheader()
        w.writerows(rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
