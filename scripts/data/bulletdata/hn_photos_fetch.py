#!/usr/bin/env python3
"""Downloads the product photos of the H&N reloading bullets (hn-sport.de) for measuring bullet lengths.

    hn_photos_fetch.py [--out DIR] [--delay 0.4]

Every bullet page has a photo of a single standing bullet (`..._einzeln.png`, transparent background, seen from slightly above) and one of two
bullets (`..._doppelt.png`). Both are saved as

    H-N__<shape and coating>__d<diameter in>__w<weight gr>__<einzeln|doppelt>.png

with `manifest.csv` (file, brand, title, diameter_in, weight_gr, kind, url, image_url), the same layout as hunting_sport_fetch.py so that
measure_standing_bullet.py works on both. Existing files are not downloaded again."""
import argparse
import csv
import re
import time
import urllib.request
from pathlib import Path

from bulletfetch import UA, get
from hn_fetch import BASE, parse


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=str(Path.home() / "Dokumente/QT/bullet-images/hn-sport"))
    ap.add_argument("--delay", type=float, default=0.4)
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    urls = sorted(set(re.findall(r'href="(/de/reloading/[^"?#]+)"', get(BASE + "/de/reloading", None))))
    rows = []
    for u in urls:
        time.sleep(args.delay)
        page = get(BASE + u, None)
        rec = parse(BASE + u, page)
        if not rec:
            continue
        for kind in ("einzeln", "doppelt"):
            m = re.search(r'/sites/default/files/[^"\']*_%s\.png' % kind, page)
            if not m:
                continue
            title = rec["line"].replace(" ", "-")
            name = f"H-N__{title}__d{float(rec['diameter_in']):.3f}__w{float(rec['weight_gr']):g}__{kind}.png"
            path = out / name
            if not path.exists():
                time.sleep(args.delay)
                path.write_bytes(urllib.request.urlopen(urllib.request.Request(BASE + m.group(0), headers=UA), timeout=60).read())
            rows.append({"file": name, "brand": "H&N", "title": f"H&N {rec['line']} {rec['diameter_in']} {rec['weight_gr']} gr", "diameter_in": rec["diameter_in"],
                         "weight_gr": rec["weight_gr"], "kind": kind, "url": BASE + u, "image_url": BASE + m.group(0)})
            print(name)
    with open(out / "manifest.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(len(rows), "photos in", out)


if __name__ == "__main__":
    main()
