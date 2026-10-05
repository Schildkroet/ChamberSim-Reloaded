#!/usr/bin/env python3
"""Reads the bullet specifications published on speer.com into a CSV (bullets_speer.csv).

    speer_fetch.py [-o bullets_speer.csv] [--cache dir]

Takes the bullet category pages from the site map, follows their part links and reads their "Specs" list: grain weight, diameter, bullet length, ballistic
coefficient (G1), bullet style. Grain and inch as on the site. One request every half second; pages are cached.
"""
import argparse
import re
import sys
from pathlib import Path

from bulletfetch import get, number, text_lines, write_csv

SITEMAP = "https://www.speer.com/sitemap_0.xml"
PAGE = re.compile(r"https://www\.speer\.com/bullets/([a-z-]+)/([a-z0-9-]+)/([a-z0-9-]+)\.html")


def parse(url: str, page: str) -> dict | None:
    lines = text_lines(page)
    if "Specs" not in lines:
        return None
    body = lines[lines.index("Specs") + 1:]
    body = body[:body.index("Downloads")] if "Downloads" in body else body[:40]
    spec = {body[k]: body[k + 1] for k in range(0, len(body) - 1, 2)}
    m = PAGE.match(url)
    line = re.sub(r"\b(rifle|handgun|component|bullet|bullets)\b", "", m[2].replace("-", " ")).strip().title()
    style = spec.get("Bullet Style", "")
    return {"manufacturer": "Speer", "line": line, "part": m[3], "caliber_name": spec.get("Caliber", ""),
            "weight_gr": number(spec.get("Grain Weight", "")), "diameter_in": number(spec.get("Diameter In", "")),
            "length_in": number(spec.get("Bullet Length In", "")), "g1bc": number(spec.get("Ballistic Coefficient", "")),
            "base_style": "Boat Tail" if "boat" in style.lower() else "", "ogive_style": style, "url": url}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_speer.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    categories = [u for u in re.findall(r"<loc>([^<]*)", get(SITEMAP, args.cache)) if re.fullmatch(r"https://www\.speer\.com/bullets/[a-z-]+/[a-z0-9-]+/", u)]
    urls: dict[str, None] = {}  # the part pages are linked from their category page
    for c in categories:
        for path in re.findall(r'href="(/bullets/[a-z-]+/[a-z0-9-]+/[a-z0-9-]+\.html)"', get(c, args.cache)):
            urls["https://www.speer.com" + path] = None
    urls = list(urls)
    rows = []
    for u in urls:
        row = parse(u, get(u, args.cache))
        if row:
            rows.append(row)
        else:
            print("no specifications:", u, file=sys.stderr)
    write_csv(args.output, rows, needs_length=True)
    print(f"{len(urls)} pages read", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
