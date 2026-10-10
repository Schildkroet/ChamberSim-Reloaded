"""Shared helpers of the *_fetch.py scripts: polite cached downloads, page text, CSV output."""
import csv
import html
import re
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

UA = {"User-Agent": "Mozilla/5.0 (ChamberSimReloaded data collection)"}
COLUMNS = ["manufacturer", "line", "part", "caliber_name", "weight_gr", "diameter_in", "length_in", "g1bc", "g7bc", "base_style",
           "ogive_style", "boat_tail_length_in", "discontinued", "valid", "note", "url", "length_note"]


def get(url: str, cache: Path | None, delay: float = 0.5) -> str:
    """The page as text; pages are kept in `cache` so a rerun does not fetch them again."""
    f = cache / re.sub(r"[^\w.-]", "_", url.split("//")[1])[:200] if cache else None
    if f and f.exists():
        return f.read_text(encoding="utf-8")
    for attempt in range(5):  # busy servers answer 503 now and then
        time.sleep(delay * (1 + 4 * attempt))
        try:
            text = urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=30).read().decode("utf-8", "replace")
            break
        except urllib.error.HTTPError as e:
            if e.code < 500 or attempt == 4:
                raise
    if f:
        f.write_text(text, encoding="utf-8")
    return text


def text_lines(page: str) -> list[str]:
    txt = re.sub(r"<script.*?</script>|<style.*?</style>", "", page, flags=re.S)
    txt = html.unescape(re.sub(r"<[^>]+>", "\n", txt))
    return [ln.strip() for ln in txt.splitlines() if ln.strip()]


def number(s: str) -> str:
    m = re.search(r"\d*\.?\d+", s or "")
    return m[0] if m else ""


def write_csv(path: Path, rows: list[dict], needs_length: bool = False) -> None:
    """Writes the rows. `valid` says whether bullets_to_json.py may use a row: "yes" if weight and diameter are there (and
    the length, if the maker publishes it: needs_length), else "no" with the reason in `note`. A value already set stays;
    edit the CSV to exclude a bullet by hand."""
    for row in rows:
        if row.get("valid"):
            continue
        missing = [name for name, key in (("weight", "weight_gr"), ("diameter", "diameter_in"))
                   if not row.get(key)] + (["length"] if needs_length and not row.get("length_in") else [])
        row["valid"] = "no" if missing else "yes"
        row["note"] = "missing: " + ", ".join(missing) if missing else ""
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, COLUMNS)
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} bullets -> {path}", file=sys.stderr)
