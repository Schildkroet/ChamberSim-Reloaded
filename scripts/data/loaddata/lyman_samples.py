#!/usr/bin/env python3
"""Turns the CSV of lyman_extract.py into fit samples (CSV for csr_fitdata), like hornady_handbook_samples.py.

    lyman_samples.py lyman_loads.csv [-o lyman_samples.csv]

A row gives a starting and a maximum load: two samples (kinds "start" and "max"). Only P.S.I. pressures are used (C.U.P. is not comparable, as in
the other extractors); velocity-only rows (T/C Contender and Encore) get no pressure. Bullets: the maker and type come from the cartridge page's
list of the bullets used (matched by weight and by the words of the table header), the length from the project's record or the estimate.
Cast-bullet tables (#number) are skipped. Cartridges, powders and bullets that the project does not know are listed on stderr.
"""
import argparse
import csv
import difflib
import json
import re
import sys
from collections import Counter
from pathlib import Path

from hornady_handbook_samples import find_caliber  # noqa: E402
from samples_common import IN, Records  # noqa: E402

GRAIN_G = 0.06479891
FPS = 0.3048
PSI_BAR = 0.0689476
MAKERS = {"homady": "Hornady", "hornady": "Hornady", "bames": "Barnes", "barnes": "Barnes", "sierra": "Sierra", "speer": "Speer", "nosler": "Nosler",
          "swift": "Swift", "berger": "Berger", "lapua": "Lapua", "remington": "Remington", "winchester": "Winchester", "federal": "Federal"}
CARTRIDGES = {"22 Hornet (5.6x35mm)": "22 Hornet", "6mm Remington (244 Remington)": "6 mm Rem.", "250 Savage (250/3000 Savage)": "250 Savage",
              "25-20 Winchester (25 WCF)": "25-20 Win.", "30 M -1 Carbine": "30 Carbine", "7mm Mauser (7x57mm)": "7 x 57",
              "8mm Mauser (8 x 57mm JS)": "8 x 57 IS", "8mm Mauser (8x57mm)": "8 x 57 IS", "26 Nosier": "26 Nosler", "28 Nosier": "28 Nosler", "223 WSSM": "223 Win. Super Short Mag.", "25WSSM": "25 Win. Super Short Mag.",
              "30Q Weatherby Magnum": "300 Weath. Mag.", "32 Automatic (32 ACP)": "7.65 mm Browning", "338 Lapua (8.58 x 71mm)": "338 Lapua Mag.",
              "380 Automatic (380 ACP, 9mm Kurz)": "9 mm Browning court", "500 Smith & Wesson": "500 S&W Mag", "6.5x55mm Swedish Mauser": "6.5 x 55 SE",
              "7.62 Tokarev/30 Mauser": "7.62 x 25 Tokarev", "7.62x39mm Russian": "7.62 x 39", "7.62x54Rmm Russian": "7.62 x 54 R",
              "7.65mm Argentine Mauser (7.65x53mm)": "7.65 x 53 Arg.", "7mm WSM (Winchester Short Magnum)": "7 mm Win. Short Mag.",
              "30 Luger": "7.65 Parabellum", "44 Russian": "44 Russian",
              "7mm STW (Shooting Times Westerner)": "7 mm STW", "30-06 Springfield": "30-06 Spring."}
# Lyman's spelling of a powder -> maker and name of the project's record
POWDERS = {"Ace. #5": "Accurate No. 5", "Ace. #7": "Accurate No. 7", "Ace. #9": "Accurate No. 9", "Hunter": "Ramshot Hunter",
           "Enforcer": "Ramshot Enforcer", "Big Game": "Ramshot Big Game", "Magnum": "Ramshot Magnum", "True Blue": "Ramshot True Blue",
           "2400": "Alliant 2400", "Unique": "Alliant Unique", "Blue Dot": "Alliant Blue Dot", "Bullseye": "Alliant Bullseye",
           "Red Dot": "Alliant Red Dot", "Green Dot": "Alliant Green Dot", "Power Pistol": "Alliant Power Pistol", "5744": "IMR 5744",
           "231": "Winchester 231", "748": "Winchester 748", "760": "Winchester 760", "296": "Winchester 296", "WSF": "Hodgdon WSF",
           "Titegroup": "Hodgdon Titegroup", "Varget": "Hodgdon Varget", "Lil'Gun": "Hodgdon Lil'Gun", "BL-C(2)": "Hodgdon BL-C(2)",
           "HS-6": "Hodgdon HS-6", "HS-7": "Hodgdon HS-7", "H110": "Hodgdon H110", "H-110": "Hodgdon H110", "Clays": "Hodgdon Clays",
           "Universal": "Hodgdon Universal", "SR-4756": "IMR SR 4756", "SR-4759": "IMR SR 4759", "SR-7625": "IMR SR 7625", "PB": "IMR PB",
           "700X": "Alliant 700-X", "800": "Alliant 800-X", "700": "Alliant 700-X"}


def powder_name(raw: str, known: set[str]) -> str:
    name = raw.strip()
    if re.fullmatch(r"[HN]-?[0-9lIO!]{3,4}[A-Za-z]{0,3}", name):  # H110 read as HllO, N110 as NllO
        name = name[0] + name[1:].replace("l", "1").replace("I", "1").replace("!", "1").replace("O", "0")
    if name in POWDERS:
        return POWDERS[name]
    m = re.fullmatch(r"RX\s*(\d+)", name)
    if m:
        return f"Alliant Reloder {m[1]}"
    m = re.fullmatch(r"(IMR)[- ]?(\d{4}\w*)", name)
    if m:
        return f"IMR {m[2]}"
    m = re.fullmatch(r"H-?(\d{3,4})", name)
    if m:
        return f"Hodgdon H{m[1]}"
    m = re.fullmatch(r"N-?(\d{2,3})", name)
    if m:
        return f"Vihtavuori N{m[1]}"
    m = re.fullmatch(r"(1680|2015|2200|2230|2460|2495|2520|2700|3100|4064|4350)", name)
    if m:
        return f"Accurate {m[1]}"
    return name


def canonical(names: Counter) -> dict[str, str]:
    """OCR variants (lMR-4350, Acc.#7, Rx15, BIg Game) -> the spelling that occurs often: a close match to a name seen at least 8 times."""
    vocab = [n for n, c in names.items() if c >= 8]
    out = {}
    for n in names:
        key = re.sub(r"^(?:[lI1i]+[MmNnvV]+[RrIi]+)", "IMR", n.replace("Rx", "RX"))
        key = re.sub(r"^(Ac[ce]|Aee)\.?\s*#?\s*(\d)", r"Ace. #\2", key)
        key = re.sub(r"^[^A-Za-z0-9]+|^[a-z]{1,2} (?=\S)", "", key).replace("Acc.#", "Ace. #")
        if key in names and names[key] >= 8 or key in vocab:
            out[n] = key
            continue
        close = difflib.get_close_matches(key, vocab, n=1, cutoff=0.8)
        out[n] = close[0] if close else key
    return out


def pick_bullet(used: str, grains: float, header: str):
    """(maker, type) of the bullet used with that weight whose type shares words with the table header."""
    cands = []
    for item in used.split(";"):
        if ":" not in item:
            continue
        gr, rest = item.split(":", 1)
        if abs(float(gr) - grains) > 2.5:
            continue
        rest = re.sub(r"^(?:Jacketed ?)?(?:Bullets ?)?Used ?", "", rest).strip()
        maker, _, btype = rest.partition(" ")
        cands.append((MAKERS.get(maker.lower(), maker), btype))
    if not cands:
        return None
    words = set(re.findall(r"[a-z0-9]+", header.lower()))
    return max(cands, key=lambda c: len(words & set(re.findall(r"[a-z0-9]+", f"{c[0]} {c[1]}".lower()))))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("loads", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("lyman_samples.csv"))
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    records = Records(args.data)
    known = {f"{p['mname']} {p['pname']}" for f in (args.data / "propellants").glob("*.json") for p in json.loads(f.read_text(encoding="utf-8"))}
    rows, miss, unknown, skipped = [], Counter(), Counter(), Counter()
    loads = list(csv.DictReader(args.loads.open(encoding="utf-8")))
    canon = canonical(Counter(r["powder"] for r in loads))
    for r in loads:
        r["powder"] = canon[r["powder"]]
        title = r["cartridge"]
        cal = find_caliber(records, CARTRIDGES.get(title, title)) if CARTRIDGES.get(title, title) else None
        if not cal:
            miss[title] += 1
            continue
        head = r["bullet"].lstrip("*& ")
        if head.startswith("#"):
            skipped["cast bullet"] += 1
            continue
        m = re.search(r"(\d{2,3})\s*[Gg]r\b", head)
        if not m or not r["barrel_in"] or not r["oal_in"]:
            skipped["no weight, barrel or length"] += 1
            continue
        grains = float(m[1])
        dia = (float(r["groove_in"]) if r["groove_in"] else cal["G1"] / IN) * IN
        maker, btype = pick_bullet(r["bullets_used"], grains, head) or ("Lyman", re.sub(r"^\d+\s*gr\.?\s*", "", head))
        length, source, tail = records.bullet(maker, btype, grains, dia)
        pname = powder_name(r["powder"], known)
        if pname not in known:
            unknown[r["powder"]] += 1
        psi = r["pressure_unit"] == "PSI"
        base = {"powder": pname, "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""), "bullet": f"{maker} {btype}", "bullet_gr": grains,
                "bullet_dia_mm": f"{dia:.3f}", "bullet_len_mm": f"{length:.2f}", "len_source": source, "tail_type": tail,
                "col_mm": f"{float(r['oal_in']) * IN:.2f}", "barrel_mm": f"{float(r['barrel_in']) * IN:.0f}",
                "twist_mm": f"{float(r['twist_in']) * IN:.0f}" if r["twist_in"] else "", "primer": r["primer"]}
        for kind, gr, fps, p in (("start", r["start_gr"], r["start_fps"], r["start_pressure"]), ("max", r["max_gr"], r["max_fps"], r["max_pressure"])):
            rows.append({**base, "kind": kind, "charge_g": round(float(gr) * GRAIN_G, 4), "velocity_ms": round(float(fps) * FPS, 1),
                         "pressure_bar": f"{float(p) * PSI_BAR:.0f}" if psi and p else ""})
    with args.output.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} samples -> {args.output}", file=sys.stderr)
    print("skipped:", dict(skipped), file=sys.stderr)
    print("cartridges not found:", "; ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    print("powders without a project record:", "; ".join(f"{k} ({v})" for k, v in unknown.most_common()), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
