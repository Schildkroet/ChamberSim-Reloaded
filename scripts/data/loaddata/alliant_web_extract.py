#!/usr/bin/env python3
"""Turns the recipe pages of Alliant Powder's online Reloader's Guide (alliantpowder.com/reloaders, pistol / revolver and rifle) into fit samples (CSV),
like rs_extract.py.

    alliant_web_extract.py <folder of saved pages: d_<gtypeid>_<shell>_<bullet>_<weight>.html> [-o alliant_web_samples.csv]

The site wants the safety notice accepted first (its "Enter the Guide" link, a session cookie); RecipeList.aspx?page=/reloaders/RecipeList.aspx&gtypeid=1
(pistol) or 2 (rifle) lists the recipes, recipedetail.aspx?gtypeid=..&weight=..&shellid=..&bulletid=.. has one table per cartridge and bullet: minimum
overall length (in), test barrel (in), primer, powder, charge (grains) and velocity (fps). Alliant publishes no pressures (the pressure column is
commented out of the page and not read here) and says to reduce every charge by 10 % for a starting load: every row is a maximum load.
"""
import argparse
import csv
import html
import re
import sys
from pathlib import Path

from samples_common import IN, Records  # noqa: E402

GRAIN_G = 0.06479891
FPS = 0.3048
MAKERS = ["Speer", "Hornady", "Sierra", "Nosler", "Berger", "Barnes", "Lapua", "Swift", "Federal", "Winchester", "Remington", "Norma", "Acme", "Extreme",
          "Xtreme", "Lehigh", "Cutting Edge", "Woodleigh", "Fiocchi", "Lyman", "Rainier", "Berry"]
# the site's names (without the "+P" and the text in brackets) that are not the C.I.P. names of the project's records
ALIASES = {"9mm Luger": "9 mm Luger", "25 Auto": "6.35 Browning", "32 Auto": "7.65 mm Browning", "380 Auto": "9 mm Browning court",
           "9x18mm Makarov": "9 mm Makarov", "30 M1 Carbine": "30 Carbine", "30-378 Weatherby": "30-378 Weath. Mag.",
           "300 Rem. Short Action Ultra Magnum": "300 Rem. SA Ultra Mag.", "7mm Rem. Short Action Ultra Magnum": "7 mm Rem. SA Ultra Mag.",
           "325 Win. Short Magnum": "325 WSM", "45/70 Government": "45-70 Govt.", "470 Nitro Express": "470 N.E.", "50 BMG": "50 Browning",
           "6.5 X 55 Swedish Mauser": "6.5 x 55 SE", "7mm Mauser": "7 x 57", "8mm Mauser": "8 x 57 IS", "7mm Shooting Times Westerner": "7 mm STW"}


def number(text: str) -> float:
    """A table value; "-", "n/a" and the like are 0 (not published)."""
    try:
        return float(text.replace(",", "").strip() or 0)
    except ValueError:
        return 0.0


def cells(row: str) -> list[str]:
    return [html.unescape(re.sub(r"<[^>]+>", "", c)).strip() for c in re.findall(r"<td[^>]*>(.*?)</td>", row, re.S)]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("folder", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("alliant_web_samples.csv"))
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    records = Records(args.data)
    rows, miss, skipped = [], {}, 0
    for page in sorted(args.folder.glob("d_*.html")):
        text = page.read_text(encoding="utf-8", errors="replace")
        title = re.search(r'lblRecipeTitle">([^<]*)<', text)
        detail = [html.unescape(x).strip() for x in re.findall(r"<li>([^<]*)</li>", text)]
        # "115 gr FMJ", "Speer 115 gr CPRN", "110 GR Speer GDHP", "147 FP" (no unit; the page's weight is in its file name)
        bullet = next((d for d in detail if re.search(r"\d\s*gr\b", d, re.I) or re.match(r"\d+\s+\w", d)), "")
        weight = re.search(r"(\d+(?:\.\d+)?)\s*(?:gr\b)?", bullet, re.I)
        m = weight and (weight[1], (bullet[:weight.start()] + " " + re.sub(r"^\s*gr\b", "", bullet[weight.end():], flags=re.I)).strip())
        if not (title and m):
            skipped += 1
            continue
        name = html.unescape(title.group(1)).strip()
        plain = re.sub(r"\s*\+P$", "", re.sub(r"\s*\(.*?\)\s*", " ", name).strip()).strip()
        cal = records.caliber(ALIASES.get(name, ALIASES.get(plain, plain)))
        if not cal:
            miss[name] = miss.get(name, 0) + 1
            continue
        grains, btype = float(m[0]), m[1].strip()
        maker = next((k for k in MAKERS if re.search(rf"\b{k}\b", btype, re.I)), "")
        btype = re.sub(rf"\b{maker}\b", "", btype, flags=re.I).strip() if maker else btype
        dia = cal.get("G1") or 0
        if not dia:
            skipped += 1
            continue
        length, source, tail = records.bullet(maker, btype, grains, dia)
        # columns by their headings: rifle pages have a case column first and a notes column (pistol pages neither)
        heads = [re.sub(r"\s+", " ", h).split("(")[0].strip().lower() for h in cells(re.sub(r"<br\s*/?>", " ", re.search(r'<tr class="columns">(.*?)</tr>', text, re.S)[1]).replace("h6", "td"))] \
            if re.search(r'<tr class="columns">', text) else []
        col = {name: heads.index(key) for name, key in (("oal", "minimum oal"), ("barrel", "bbl length"), ("primer", "primer"), ("powder", "powder"),
                                                       ("charge", "charge weight"), ("velocity", "velocity")) if key in heads}
        if len(col) < 6:
            skipped += 1
            continue
        for row in re.findall(r'<tr class="data">(.*?)</tr>', text, re.S):
            c = cells(row)
            if len(c) <= max(col.values()) or not all(number(c[col[k]]) > 0 for k in ("oal", "barrel", "charge", "velocity")):
                continue
            rows.append({"powder": "Alliant " + c[col["powder"]], "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""), "bullet": f"{maker} {btype}".strip(),
                         "bullet_gr": grains, "bullet_dia_mm": dia, "bullet_len_mm": f"{length:.2f}", "len_source": source, "tail_type": tail,
                         "col_mm": f"{number(c[col['oal']]) * IN:.2f}", "barrel_mm": f"{number(c[col['barrel']]) * IN:.0f}", "twist_mm": "", "kind": "max",
                         "charge_g": f"{number(c[col['charge']]) * GRAIN_G:.4f}", "velocity_ms": f"{number(c[col['velocity']]) * FPS:.1f}", "pressure_bar": "",
                         "primer": c[col["primer"]]})
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} samples -> {args.output}; {skipped} pages skipped", file=sys.stderr)
    print("cartridges not found:", ", ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
