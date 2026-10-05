#!/usr/bin/env python3
"""Adds the SAAMI maximum average pressure (MAP) to the project's caliber records (field PmaxSAAMI, bar).

    saami_pressures.py [--data data] [--cache dir] [--write] [--only "308 Win.,..."] [--shard i/n]

For every record of data/calibers the English Wikipedia article of the cartridge is looked up (search by the record's name, then by its
alternative name) and the pressure of its {{Infobox firearm cartridge}} whose method is SAAMI is taken; a pressure without method counts as SAAMI when the
value is in psi (American boxes give psi, the C.I.P. ones MPa and say so). A number without unit is MPa below 1000, else psi (the template's default unit). An article only
counts when its title has the same numbers as the record's name (".308 Winchester" for "308 Win."), the same letter suffix ("R",
"IRS": 7 x 57 R is not the 7x57 Mauser) and all of its words ("300 Rem. SA Ultra Mag." is not the .300 RUM, "32 Win. SL" not the .32
Winchester Special), a redirect leads to an article that shares a number with the record, and it has a cartridge infobox. An unlabelled
psi value equal to the record's C.I.P. Pmax (converted) is the C.I.P. value. Cartridges without a SAAMI standard (most European ones) get no value. The C.I.P. maximum pressure stays
in Pmax (from the C.I.P. sheets); the infobox's C.I.P. value is only printed for comparison. Without --write the result is printed.
"""
import argparse
import json
import re
import sys
import urllib.parse
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from wiki_calibers import PSI, fetch, infoboxes, search_title, value  # noqa: E402


def numbers(s: str) -> set[str]:
    return {n.lstrip("0") or "0" for n in re.findall(r"\d+(?:\.\d+)?", s)}


def query_of(name: str) -> str:
    q = name
    for a, b in [("Weath.", "Weatherby"), ("Rem.", "Remington"), ("Win.", "Winchester"), ("Spring.", "Springfield"), ("Mag.", "Magnum"),
                 ("Sav.", "Savage"), ("Spl.", "Special"), (" x ", "x")]:
        q = q.replace(a, b)
    q = re.sub(r"\bSL\b", "Self-Loading", q)
    return re.sub(r"\s+", " ", q).strip()


def words(name: str) -> set[str]:
    """The words of a cartridge name that tell it apart (abbreviations expanded; single letters, "mm" and "x" left out), lower case."""
    return {w.lower() for w in re.findall(r"[^\W\d_]{2,}", query_of(name)) if w.lower() not in ("mm", "cartridge")}


def same_words(name: str, title: str) -> bool:
    """Whether every word of the record's name is in the article title (or one is the beginning of the other: Mag / Magnum)."""
    have = words(title)
    return all(any(t == w or (len(w) >= 3 and (t.startswith(w) or w.startswith(t))) for t in have) for w in words(name))


def article(title: str, cache: Path) -> tuple[str, str]:
    """The final title (redirects followed) and the wikitext of an article."""
    for _ in range(5):
        text = fetch("https://en.wikipedia.org/w/index.php?action=raw&title=" + urllib.parse.quote(title.replace(" ", "_")), cache)
        m = re.match(r"#REDIRECT\s*\[\[([^\]#|]+)", text, re.I)
        if not m:
            return title, text
        title = m[1]
    return title, ""


def suffix(name: str) -> str:
    """The letters right after the last number of a cartridge name ("R" of "7 x 57 R", "IRS" of "8 x 57 IRS"), upper case."""
    m = re.search(r"\d\s*([A-Za-z]{1,3})\s*$", name.replace(".", " ").strip())
    return m[1].upper() if m and m[1].upper() in ("R", "IRS", "IS", "RS", "S", "J", "JR", "JRS", "SR") else ""


def in_psi(raw: str) -> bool:
    """Whether an infobox pressure is in psi: stated, or a plain number of 1000 and more (the template's default unit)."""
    raw = re.sub(r"<ref.*?(</ref>|/>)", "", raw, flags=re.S)
    if re.search(r"psi", raw, re.I):
        return True
    m = re.fullmatch(r"\s*(\d[\d,]*\.?\d*)\s*", raw)
    return bool(m) and float(m[1].replace(",", "")) >= 1000


def pressure_bar(raw: str) -> float | None:
    v = value(raw, "bar")
    if v is None:  # no unit: the template's default is psi; small numbers are MPa (the metric boxes)
        m = re.search(r"(\d[\d,]*\.?\d*)", re.sub(r"<ref.*?(</ref>|/>)", "", raw, flags=re.S))
        if not m:
            return None
        x = float(m[1].replace(",", ""))
        v = x * 10 if x < 1000 else x * PSI
    return v


def pressures(box: dict, cip: float) -> dict:
    """SAAMI and C.I.P. pressure of an infobox [bar]. `cip` is the record's C.I.P. Pmax: an unlabelled psi value equal to it (within
    0.2 %: a converted value; real SAAMI values are often close, .375 H&H 4275 / 4300 bar) is that C.I.P. value, not a SAAMI one."""
    out = {}
    for sfx in ("", "2", "3"):
        raw, method = box.get("max_pressure" + sfx, ""), box.get("pressure_method" + sfx, "").lower()
        p = pressure_bar(raw) if raw else None
        if not p or p < 300 or p > 7000:
            continue
        unlabelled = not re.sub(r"<!--.*?-->", "", method, flags=re.S).strip()
        if "saami" in method or (unlabelled and in_psi(raw) and not (cip > 0 and abs(p / cip - 1) <= 0.002)):
            out.setdefault("saami", p)
        elif "c.i.p" in method or "cip" in method:
            out.setdefault("cip", p)
    return out


def lookup(record: dict, cache: Path) -> tuple[str | None, dict]:
    want = numbers(record["cipname"])
    for name in (record["cipname"], record.get("altname", "")):
        if not name:
            continue
        for q in (query_of(name) + " cartridge", query_of(name)):
            try:
                title = search_title(q, cache)
            except Exception:  # noqa: BLE001 (network): try the next query
                continue
            if not title or numbers(title) != want or suffix(record["cipname"]) != suffix(title.replace("×", " x ")) or not same_words(name, title):
                continue
            # a redirect may lead to the same cartridge under another name (9 mm Luger -> 9x19mm Parabellum) or to another one
            # (5.6 x 35 R -> .22 Hornet, .22 Nosler -> Nosler cartridges): the target must share a number with the record
            final, text = article(title, cache)
            if not numbers(final) & want:
                continue
            boxes = infoboxes(text)
            if boxes:
                return final, pressures(boxes[0], float(record.get("Pmax") or 0))
    return None, {}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", default=str(Path(__file__).resolve().parents[3] / "data"))
    ap.add_argument("--cache", default="wikicache")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--only", default="")
    ap.add_argument("--shard", default="0/1", help="i/n: only every n-th record from the i-th on (parallel runs; Wikipedia is slow)")
    args = ap.parse_args()
    cache = Path(args.cache)
    cache.mkdir(parents=True, exist_ok=True)
    only = set(filter(None, args.only.split(",")))
    found = 0
    shard, shards = map(int, args.shard.split("/"))
    for f in sorted((Path(args.data) / "calibers").glob("*.json"))[shard::shards]:
        record = json.loads(f.read_text(encoding="utf-8"))
        if only and record["cipname"] not in only:
            continue
        title, p = lookup(record, cache)
        saami = round(p["saami"]) if "saami" in p else None
        print(f"{record['cipname']:28s} {str(title):36s} Pmax {record.get('Pmax', '')!s:>6}  wiki CIP {round(p['cip']) if 'cip' in p else '-':>5}"
              f"  SAAMI {saami if saami else '-':>5}", flush=True)
        if saami:
            found += 1
            if args.write:
                # right after the C.I.P. Pmax, in the records' format (indent 2, schema order)
                out = {}
                for k, v in record.items():
                    if k != "PmaxSAAMI": out[k] = v
                    if k == "Pmax": out["PmaxSAAMI"] = saami
                out.setdefault("PmaxSAAMI", saami)
                f.write_text(json.dumps(out, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"{found} records with a SAAMI pressure", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
