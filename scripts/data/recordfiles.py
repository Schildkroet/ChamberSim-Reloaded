"""The project's projectile and propellant records: one file per manufacturer, data/<projectiles|propellants>/<manufacturer>.json,
with an array of records (the program reads a file with a single record as well, e.g. an exported one put into the folder).

read(folder) gives every record with the file it is in; write(folder, records) writes them back grouped by manufacturer
(file name = slug of mname, records sorted by maker, name and lot), in the records' format (indent 2, keys in the given
order); single-record files are merged into the manufacturer's file.

    sys.path.insert(0, str(<path to tools>)); import recordfiles

As a command it merges a folder: an exported record put into data/projectiles or data/propellants goes into its manufacturer's
file (a record with the same maker, name and lot is replaced by it):

    python3 scripts/data/recordfiles.py data/propellants
"""
import json
import sys
import unicodedata
from pathlib import Path


def slug(*parts: str) -> str:
    """Lower case ASCII letters and digits, everything else one "-" (as the program's file names)."""
    s = unicodedata.normalize("NFKD", " ".join(p for p in parts if p)).lower()
    out = ""
    for c in s:
        if c.isascii() and c.isalnum():
            out += c
        elif out and not out.endswith("-"):
            out += "-"
    return out.strip("-")


def records_of(path: Path) -> list[dict]:
    data = json.loads(path.read_text(encoding="utf-8"))
    return data if isinstance(data, list) else [data]


def read(folder: Path) -> list[tuple[Path, dict]]:
    return [(f, r) for f in sorted(Path(folder).glob("*.json")) for r in records_of(f)]


def key(record: dict) -> str:
    """Sort key and identity of a record: maker, name and lot. Sorted like the former one-file-per-record names
    ("<slug>.json"), so the records kept their order (and with it their ids in the program)."""
    return slug(record.get("mname", ""), record.get("pname", ""), record.get("lotid", "")) + ".json"


def write(folder: Path, records: list[dict]) -> list[Path]:
    """Replaces the folder's files by one file per manufacturer; returns the files written."""
    folder = Path(folder)
    folder.mkdir(parents=True, exist_ok=True)
    groups: dict[str, list[dict]] = {}
    for r in records:
        groups.setdefault(slug(r.get("mname", "")) or "unknown", []).append(r)
    written = []
    for name, group in groups.items():
        path = folder / (name + ".json")
        path.write_text(json.dumps(sorted(group, key=key), indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        written.append(path)
    for old in folder.glob("*.json"):
        if old not in written:
            old.unlink()
    return written


def main(argv: list[str]) -> int:
    if len(argv) != 2 or not Path(argv[1]).is_dir():
        print(__doc__.strip(), file=sys.stderr)
        return 2
    folder = Path(argv[1])
    records: dict[str, dict] = {}
    files = sorted(folder.glob("*.json"), key=lambda f: isinstance(json.loads(f.read_text(encoding="utf-8")), dict))
    for _, r in ((f, r) for f in files for r in records_of(f)):  # the manufacturers' files first: single records replace theirs
        records[key(r)] = r
    written = write(folder, list(records.values()))
    print(f"{len(records)} records in {len(written)} files", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
