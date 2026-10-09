#!/usr/bin/env python3
"""Turns the rifle or pistol data of ADI Powders (adiworldclass.com.au/data-rifle/, /data-pistol/) into fit samples (CSV), like lovex_extract.py.

    adi_extract.py [--weapon rifle|pistol] [-o adi_samples.csv] [--cache DIR] [--only 308-winchester,...] [--delay 0.5]

The site shows the data only after the visitor has acknowledged its warnings ("By clicking below you acknowledge the Warnings page and
agree to abide by the Reloading Safety page"). Running this script is that acknowledgement: it submits the form once and keeps the
cookie. Then it reads one page per cartridge (slugs from the cartridge filter of the index page; the pages are cached in --cache, one
request every --delay seconds). A page has the barrel length, twist and primer of the test barrel and per bullet and powder the
cartridge length, the starting and the maximum load with velocity (fps) and pressure (psi, or CUP, which is ignored).
Cartridges are matched to data/calibers, bullets to data/projectiles (else a length is estimated).
"""
import argparse
import csv
import html
import http.cookiejar
import re
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path

from samples_common import Records  # noqa: E402

SITE = "https://www.adiworldclass.com.au/data-"  # + rifle/ or pistol/
IN = 25.4
GR = 0.06479891
FPS = 0.3048
PSI = 0.0689476
UA = "Mozilla/5.0 (ChamberSim Reloaded data extraction)"
# names of the cartridge filter that differ from the C.I.P. names of the records
ALIASES = {"308 Winchester": "308 Win.", "30-06 Springfield": "30-06 Spring.", "223 Remington": "223 Rem.", "243 Winchester": "243 Win.",
           "6.5 x 55mm Swedish Mauser": "6.5 x 55 SE", "7 x 57mm Mauser": "7 x 57", "8 x 57mm Mauser": "8 x 57 IS",
           "9.3 x 62mm Mauser": "9.3 x 62", "7.5 x 55mm Swiss": "7.5 x 55 Suisse", "7.65 x 53mm Mauser": "7.65 x 53 Arg.",
           "6.5mm Carcano (6.5 x 52mm)": "6.5 x 52 Carcano", "7.62 x 39mm Russian": "7.62 x 39", "6mm Remington (244 Remington)": "6 mm Rem.",
           "6.5-284": "6.5 - 284 Norma", "250-3000 Savage": "250 Savage", "300 Ruger Compact Magnum": "300 RCM",
           "338 Ruger Compact Magnum": "338 RCM", "30-378 Weatherby": "30-378 Weath. Mag.", "257 Roberts +P": "257 Roberts",
           "6.5 Precision Rifle Cartridge (6.5 PRC)": "6.5 PRC", "300 Precision Rifle Cartridge (300 PRC)": "300 PRC",
           "7mm Precision Rifle Cartridge (7mm PRC)": "7 mm PRC", "22 Advanced Rifle Cartridge (22 ARC)": "22 ARC",
           "6mm Advanced Rifle Cartridge (6mm ARC)": "6 mm ARC", "6.5mm Grendel": "6.5 x 39", "325 Winchester Short Magnum": "325 WSM",
           "416 Rigby": "416 Rigby N°2", "22 PPC": "22 PPC-USA", "50 Browning Machine Gun": "50 Browning",
           "45-70 Government (Modern Rifles)": "45-70 Govt.", "45-70 Government (Lever Actions)": "45-70 Govt.",
           "45-70 Government (Trapdoor Rifles)": "45-70 Govt.", "45-70 Government": "45-70 Govt.",
           # pistol and revolver
           "25 ACP": "6.35 Browning", "32 ACP": "7.65 mm Browning", "32 Smith & Wesson": "32 S&W", "38 Smith & Wesson": "38 S&W",
           "38 Special +P": "38 Special", "380 Auto (9mm Kurz)": "9 mm Browning court", "40 Smith & Wesson": "40 S.& W.",
           "45 ACP": "45 Auto", "9 x 18mm Makarov": "9 mm Makarov", "44 Russian": "44 S&W Russian", "41 Action Express": "41 ACT EXP",
           "357 Remington Maximum": "357 Rem. Max.", "45 SUPER": "45 Auto", "500 Wyoming Express": "500 Wyoming Express"}


class Site:
    def __init__(self, cache: Path | None, delay: float, base: str):
        self.cache, self.delay, self.base = cache, delay, base
        self.opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
        self.opener.addheaders = [("User-Agent", UA)]
        self.last = 0.0
        self.acknowledged = False

    def get(self, url: str, name: str | None = None, data: dict | None = None) -> str:
        path = self.cache / f"{name}.html" if self.cache and name else None
        if path and path.exists():
            return path.read_text(encoding="utf-8")
        wait = self.last + self.delay - time.time()
        if wait > 0:
            time.sleep(wait)
        if not self.acknowledged:  # the cookie of the acknowledgement
            self.acknowledged = True
            self.opener.open(self.base).read()
            self.opener.open(self.base, urllib.parse.urlencode({
                "adi_powders_acknowledge__context": "_COMMON_v3", "adi_powders_acknowledge__submit": "I acknowledge this warning"}).encode()).read()
        text = self.opener.open(url).read().decode("utf-8", "replace")
        self.last = time.time()
        if path:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8")
        return text


def strip(s: str) -> str:
    s = re.sub(r'<span class="abbr-title">.*?</span>', "", s, flags=re.S)  # the expansion of an abbreviation
    return re.sub(r"\s+", " ", html.unescape(re.sub(r"<[^>]+>", " ", s))).strip()


def number(s: str) -> float | None:
    m = re.search(r"[\d.]+", s.replace(",", ""))
    return float(m[0]) if m and m[0] != "." else None


def parse(page: str):
    """(properties, rows): the description of the test barrel and one dict per bullet / powder row."""
    start = page.find('id="adi_data"')
    page = page[start:]
    props = {strip(k).rstrip(":"): strip(v) for k, v in re.findall(r'<td class="property">(.*?)</td>\s*<td class="value">(.*?)</td>', page, re.S)}
    rows = []
    for body in re.findall(r"<tbody>(.*?)</tbody>", page, re.S):
        cells = {}
        for cls, val in re.findall(r'<td class="([^"]*)"><span>(.*?)</span></td>', body, re.S):
            cells[cls.replace("value ", "").replace("load ", "")] = strip(val)
        if "powder-type" in cells:
            rows.append(cells)
    return props, rows


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--weapon", choices=["rifle", "pistol"], default="rifle")
    ap.add_argument("-o", "--output", type=Path, default=Path("adi_samples.csv"))
    ap.add_argument("--cache", type=Path, help="directory for the downloaded pages")
    ap.add_argument("--only", help="comma separated cartridge slugs (default: all)")
    ap.add_argument("--delay", type=float, default=0.5, help="seconds between two requests")
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    base = f"{SITE}{args.weapon}/"
    site = Site(args.cache, args.delay, base)
    index = site.get(base, "index")
    select = re.search(r'<select name="Filter_calibre".*?</select>', index, re.S)[0]
    slugs = [(v, html.unescape(n)) for v, n in re.findall(r'<option value="([^"]+)"[^>]*>(.*?)</option>', select)]
    if args.only:
        slugs = [s for s in slugs if s[0] in args.only.split(",")]
    records = Records(args.data)

    out, miss, nrows = [], {}, 0
    for slug, name in slugs:
        props, rows = parse(site.get(base + slug, slug))
        barrel, twist = number(props.get("Barrel Length", "")), re.search(r"1\s*:\s*([\d.]+)", props.get("Twist", ""))
        cal = records.caliber(ALIASES.get(name, name))
        if not cal:
            miss[name] = len(rows)
            continue
        if not barrel or not rows:
            continue
        for r in rows:
            m = re.match(r"([\d.]+)\s*GR\.?\s*(.*)", re.sub(r"\s*\(\d+\)\s*$", "", r.get("bullet-weight", "")), re.I)
            dia, col = number(r.get("bullet-diameter", "")), number(r.get("col", ""))
            if not (m and dia and col and r.get("powder-type")):
                continue
            flag = re.search(r"\(\s*([CSR])\s*\)\s*$", r["powder-type"])  # C compressed, S subsonic, R reduced recoil load
            if flag and flag[1] != "C":
                continue  # not loads the burn rate should follow
            powder = re.sub(r"\s*\(\s*[CSR]\s*\)\s*$", "", r["powder-type"])
            grains = float(m[1])
            words = m[2].split(" ", 1)
            maker, btype = words[0], (words[1] if len(words) > 1 else "")
            dia_mm = dia * IN
            if abs(dia_mm - (cal.get("G1") or dia_mm)) > 0.3:  # a bullet that does not belong to the cartridge: misprint
                continue
            length, source, tail = records.bullet(maker, btype, grains, cal.get("G1") or dia_mm)
            nrows += 1
            for kind in ("starting", "maximum"):
                g, v, p = (r.get(f"{kind}-powder-weight", ""), r.get(f"{kind}-velocity", ""), r.get(f"{kind}-pressure", ""))
                gn, vn = number(g), number(v)
                if not (gn and vn):
                    continue
                pressure = f"{number(p) * PSI:.0f}" if number(p) and "psi" in p.lower() else ""
                out.append({"powder": f"ADI {powder}", "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""),
                            "bullet": f"{maker} {btype}".strip(), "bullet_gr": grains, "bullet_dia_mm": cal.get("G1") or dia_mm,
                            "bullet_len_mm": f"{length:.2f}", "len_source": source, "tail_type": tail, "col_mm": f"{col * IN:.2f}",
                            "barrel_mm": f"{barrel * IN:.0f}", "twist_mm": f"{float(twist[1]) * IN:.0f}" if twist else "",
                            "kind": "start" if kind == "starting" else "max", "charge_g": f"{gn * GR:.3f}",
                            "velocity_ms": f"{vn * FPS:.1f}", "pressure_bar": pressure})
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(out[0].keys()))
        w.writeheader()
        w.writerows(out)
    print(f"{len(slugs)} cartridge pages, {nrows} loads, {len(out)} samples -> {args.output}", file=sys.stderr)
    print("cartridges not found:", ", ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
