#!/usr/bin/env python3
"""Downloads the product photos of the handgun bullets of hunting-sport.com for measuring bullet lengths (image_length.py).

    hunting_sport_fetch.py [--out DIR] [--category Handgun-Bullets] [--delay 0.4]

Lists the category (100 products per page, `?af=100&seite=N`), reads every product page (title, brand, diameter and weight of the
specification table) and downloads its large photo (`/media/image/product/<id>/lg/...`) as

    <brand>__<title>__d<diameter in>__w<weight gr>__p<product id>.jpg

with `manifest.csv` (file, product_id, brand, title, diameter_in, weight_gr, weight_g, leadfree, url, image_url) in --out, so a photo can be
matched with a projectile record; deleting photos that are not usable for measuring leaves the manifest rows of the missing files unused.
Files that exist are not downloaded again."""
import argparse
import csv
import html
import json
import re
import time
import urllib.request
from pathlib import Path

BASE = "https://www.hunting-sport.com"
UA = "Mozilla/5.0 (ChamberSimReloaded data collection; contact: see repository)"


def get(url: str) -> bytes:
    with urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": UA}), timeout=60) as r:
        return r.read()


def slug(text: str) -> str:
    return re.sub(r"[^A-Za-z0-9.+-]+", "-", html.unescape(text)).strip("-")


def listing(category: str, delay: float) -> list[str]:
    """Product page URLs of the category, in listing order."""
    urls: list[str] = []
    for page in range(1, 20):
        h = get(f"{BASE}/{category}?af=100&seite={page}").decode("utf-8", "replace")
        found = []
        # a product card links its page next to its xs photo; the page links look like /Name-355-124gr-FMJ-RN_2
        for m in re.finditer(r'href="(https://www\.hunting-sport\.com/[^"#?]+)"[^>]*>(?:(?!</a>).)*?/media/image/product/\d+/xs/', h, re.S):
            if m.group(1) not in urls and m.group(1) not in found:
                found.append(m.group(1))
        if not found:
            break
        urls += found
        time.sleep(delay)
    return urls


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=str(Path.home() / "Dokumente/QT/bullet-images/hunting-sport"))
    ap.add_argument("--category", default="Handgun-Bullets")
    ap.add_argument("--delay", type=float, default=0.4)
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    rows = []
    urls = listing(args.category, args.delay)
    print(len(urls), "products")
    for n, url in enumerate(urls, 1):
        time.sleep(args.delay)
        p = get(url).decode("utf-8", "replace")
        title = html.unescape((re.findall(r"<h1[^>]*>(.*?)</h1>", p, re.S) or [""])[0]).strip()
        text = re.sub(r"\s+", " ", re.sub(r"<[^>]+>", " ", re.sub(r"<script.*?</script>|<style.*?</style>", "", p, flags=re.S)))
        brand = (re.findall(r'"brand":\s*\{[^}]*?"name":"([^"]+)"', p) or [""])[0]
        dia = re.search(r"Diameter:\s*(\.?\d*[.,]?\d+)", text)
        wt = re.search(r"Weight:\s*(\d+(?:[.,]\d+)?)\s*gr\.?\s*/\s*(\d+(?:[.,]\d+)?)\s*g", text)
        lead_free = re.search(r"Leadfree:\s*(\w+)", text)
        img = re.search(r"/media/image/product/(\d+)/lg/([^\"' ]+\.(?:jpg|png))", p)
        if not (img and dia and wt):
            print("skipped (no photo or data):", url)
            continue
        d_in = float(dia.group(1).replace(",", "."))
        w_gr = float(wt.group(1).replace(",", "."))
        name = f"{slug(brand)}__{slug(title)}__d{d_in:.3f}__w{w_gr:g}__p{img.group(1)}.{img.group(2).rsplit('.', 1)[1]}"
        path = out / name
        if not path.exists():
            time.sleep(args.delay)
            path.write_bytes(get(BASE + img.group(0)))
        rows.append({"file": name, "product_id": img.group(1), "brand": brand, "title": title, "diameter_in": d_in, "weight_gr": w_gr,
                     "weight_g": float(wt.group(2).replace(",", ".")), "leadfree": lead_free.group(1) if lead_free else "", "url": url,
                     "image_url": BASE + img.group(0)})
        print(f"{n}/{len(urls)} {name}")
    with open(out / "manifest.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(len(rows), "photos in", out)


if __name__ == "__main__":
    main()
