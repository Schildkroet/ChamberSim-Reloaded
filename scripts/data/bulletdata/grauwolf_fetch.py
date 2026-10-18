#!/usr/bin/env python3
"""Reads the bullet listing of grauwolf.net (a German reloading shop) into a CSV (bullets_grauwolf.csv).

    grauwolf_fetch.py [-o bullets_grauwolf.csv] [--cache dir]

One request (?limit=all) returns the whole listing. Per bullet only the product title is there: "<.diameter in> / <mm> mm <weight> gr /
<g> g <maker> <type words> Geschosse <pack> Stück". No length, no ballistic coefficient. The same bullet is listed once per pack size and
as a test pack: the rows are made unique per maker, type, diameter and weight. Pellets (Diabolo) and round balls are left out. The German
type words are translated for estimate_length.py (Vollmantel = full metal jacket, Teilmantel = soft point, Hohlspitz = hollow point,
Blei = lead, bleifrei = lead free, taken as solid copper). Grain and inch. The listing covers makers that have no site of their own here
(Fox, Sax, Degol, DK Bullets, Brenneke, ...): bullets_to_json.py is run for those, the other makers have their own fetch scripts.
"""
import argparse
import html
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import recordfiles  # noqa: E402
from bulletfetch import get, write_csv

PAGE = "https://www.grauwolf.net/wiederladen/geschosse.html?limit=all"
MAKERS = ["Ballistix Academy", "Cutting Edge", "North Fork", "Sellier & Bellot", "DK Bullets", "Hornady", "Sierra", "Nosler", "Barnes", "Speer",
          "Berger", "Lapua", "Norma", "RWS", "Sako", "Federal", "Winchester", "Remington", "H&N", "Prvi Partizan", "Prvi", "PPU", "Degol",
          "Sax", "Fox", "LOS", "Brenneke", "CopperBear", "Magtech", "GECO", "Geco", "GPA", "Frontier", "Leader", "Alsa", "Hasler", "Swift",
          "Browning", "Buffalo Bore", "Cam Pro", "TAC", "ACME", "Armscor", "GM", "CloverBullets", "Shooting Technology", "ACP", "Ve-Loads",
          "Fiocchi", "Woodleigh", "Missouri Bullet Co.", "Bloodsport", "Buffalo Arms", "Balleurope", "Sierra Pro-Hunter", "BA"]
# makers with their own fetch script (or no records wanted): the rows stay in the CSV with valid = no
OWN = {"Hornady", "Sierra", "Nosler", "Barnes", "Speer", "Berger", "Lapua", "Norma", "RWS", "Sako", "Federal", "Winchester", "Remington", "H&N",
       "PPU", "Sellier & Bellot"}
KEEP = {"Degol", "Sax", "Fox", "LOS", "Brenneke", "GECO", "Magtech", "CopperBear", "Frontier", "DK Bullets", "GPA", "Cutting Edge",
        "Ballistix Academy", "North Fork", "Leader", "Wi-La-Tech"}
# rows whose bullet the database already has are marked valid = no (see --data); muzzle-loader bullets are not wanted
CARTRIDGE_PREFIX = r"^(?:g|Hornet|\.45/70|\.50 Action Express|\.50|Action Express|9,3x72R|10,75x68|350 Legend|\.25 ACP|Browning|TAC)\s+(?=[A-Z])"
TYPOS = {"Lapuar": "Lapua", "BBallistix": "Ballistix Academy", "SierraPro-Hunter": "Sierra Pro-Hunter", "Privi": "Prvi"}
SKIP_MAKERS = {"Davide"}
SKIP_LINE = r"Vorderlader|Sabot|Great Plains|Bore Driver|Minie"
RENAME = {"Sierra Pro-Hunter": "Sierra", "Missouri Bullet Co.": "Missouri Bullet", "Prvi": "PPU", "Prvi Partizan": "PPU", "Geco": "GECO"}
TITLE = re.compile(r"^(?:\*[^*]*\*\s*)?(?P<dia>\.\d+)\s*/\s*[\d.,]+\s*mm(?:\s*\([^)]*\))?\s+(?P<gr>[\d.,]+)\s*gr\s*/\s*[\d.,]+\s*gr?\s+"
                   r"(?P<rest>.+?)\s+\d+\s*St(?:ü|ue)ck\b(?P<tail>.*)$")
WORDS = [(r"\bVollmantel\b|\bFMJ\b", "full metal jacket"), (r"\bTeilmantel\b|\bSP\b", "soft point"), (r"Hohlspitz|\bHP\b", "hollow point"),
         (r"\bBlei\b|Bleirund", "lead"), (r"[Bb]leifrei|Kupfer", "all-copper")]


def parse(title: str) -> dict | None:
    m = TITLE.match(html.unescape(title))
    if not m or re.search(r"Diabolo|Rundkugel|Pellet", title):
        return None
    rest = re.sub(r"^\([^)]*\)\s*", "", m["rest"])  # the cartridge in front of the maker: "(6,35 Browning) Fiocchi ..."
    rest = re.sub(CARTRIDGE_PREFIX, "", rest)
    rest = re.sub(r"^(Lapuar|BBallistix|SierraPro-Hunter|Privi\b)", lambda g: TYPOS[g[1]], rest)
    maker = next((k for k in MAKERS if re.match(re.escape(k) + r"\b", rest)), rest.split()[0])
    line = rest[len(maker):].strip()
    free = bool(re.search(r"[Bb]leifrei", line + m["tail"]))
    line = re.sub(r"\s*(?:Spitz|Rundkopf)?[Gg]eschosse?\b", "", line).strip()
    style = " ".join(en for rx, en in WORDS if re.search(rx, line + (" bleifrei" if free else "")))
    return {"manufacturer": RENAME.get(maker, maker), "line": line, "part": "", "caliber_name": "", "weight_gr": m["gr"].replace(",", "."),
            "diameter_in": "0" + m["dia"], "base_style": "Boat Tail" if re.search(r"\bBT\b|HPBT|TMBT|BTSP", line) else
            "Flat Base" if re.search(r"\bFB\b|HPFB", line) else "", "ogive_style": f"{line} {style}".strip()}


def in_database(row: dict, records: list[dict]) -> bool:
    """A record of the same maker, diameter (0.06 mm) and mass (3 gr) is there: the listing's bullet is not missing."""
    maker, gr, dia = row["manufacturer"].lower(), float(row["weight_gr"]), float(row["diameter_in"]) * 25.4
    return any(r["mname"].lower() == maker and abs(r["gmass"] - gr) <= 3 and abs(r["gdia"] - dia) < 0.06 for r in records)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_grauwolf.csv"))
    ap.add_argument("--cache", type=Path)
    ap.add_argument("--data", type=Path, default=Path("data/projectiles"), help="projectile records: bullets found there are marked valid = no")
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    page = get(PAGE, args.cache)
    titles = set(re.findall(r'<a href="([^"]+)" title="([^"]+)" class="product-image"', page))
    records = [r for _, r in recordfiles.read(args.data)]
    rows, seen, skipped = [], set(), []
    for url, title in sorted(titles, key=lambda t: t[1]):
        row = parse(title)
        if not row:
            skipped.append(html.unescape(title))
            continue
        key = (row["manufacturer"], row["line"], row["diameter_in"], row["weight_gr"])
        if key not in seen:
            seen.add(key)
            if row["manufacturer"] in SKIP_MAKERS or re.search(SKIP_LINE, row["line"]):
                note = "muzzle-loader bullet, not wanted"
            elif in_database(row, records):
                note = "already in the database"
            else:
                note = ""
            rows.append({**row, "url": url, "valid": "no" if note else "yes", "note": note})
    write_csv(args.output, rows)
    print(f"{len(skipped)} titles skipped (pellets, round balls, no weight): e.g. {skipped[:3]}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
