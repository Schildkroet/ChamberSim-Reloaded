# Where the project's caliber and projectile records come from

Both sets were collected from public sources, not from the original program's database.

## Calibers (`data/calibers`)

1. `cip_extract.py <dir with the C.I.P. sheets>` reads the TDCC data sheets (tables I-V, PDF from bobp.cip-bobp.org)
   with `pdftotext` into `cip_calibers.csv`. Shotshells (table VII) are not read.
2. `cip_estimate_v.py cip_calibers.csv` adds the case capacity V (grain H2O) and a note in `descr`. The sheets do not
   contain it; it is estimated from the outer shape (wall and head thickness assumed, about 6 % rms for centerfire).
3. `cip_to_json.py cip_calibers.csv wiki_calibers.csv` writes the JSON files. `cipname` is the C.I.P. name of the sheet (decimal comma
   replaced by a dot); `altname` takes the names from the CSV column of that name plus the inch spelling with a leading
   dot (".308 Win." for "308 Win."). Fill the `altname` column of the CSV for other names (comma separated).

Cartridges that have no C.I.P. sheet (SAAMI, wildcats, newer than the sheets) come from Wikipedia: `wiki_calibers.py titles.txt`
reads the cartridge infoboxes (and downloads cited C.I.P. sheets, which `cip_extract.py` then reads like the others) into
`wiki_calibers.csv`. These records carry less detail (no L1/L2, no tolerances), say so in `descr`, and the case capacity is
taken from the article or estimated like above. `cip_to_json.py` adds a row of a later CSV only if no record with the same
bullet diameter, case length and base diameter exists yet.

SAAMI pressures: `saami_pressures.py --write` adds `PmaxSAAMI` (bar) to the records whose Wikipedia article gives a SAAMI value (97 of
467). The article must match the record's numbers, suffix and words, an unlabelled psi value counts as SAAMI unless it is the C.I.P. value
converted, and a value of 1000 and more without unit is psi, below MPa. Wikipedia is slow: `--shard i/n` runs parallel parts.

## Projectiles (`data/projectiles`)

`berger_fetch.py`, `nosler_fetch.py`, `speer_fetch.py`, `hornady_fetch.py`, `sierra_fetch.py`, `barnes_fetch.py`,
`sellier_fetch.py`, `lapua_fetch.py`, `norma_fetch.py`, `rws_fetch.py`, `ppu_fetch.py`, `federal_fetch.py`, `winchester_fetch.py`,
`remington_fetch.py`, `hn_fetch.py`, `sako_fetch.py` (in `scripts/data/bulletdata`) read the specification lists of the manufacturers' web shops into
`bullets_<maker>.csv` (grain, inch); `bullets_to_json.py` writes the records (mm, grain) into the manufacturers' files `data/projectiles/<maker>.json`
(`scripts/data/recordfiles.py`).

- The column `valid` says whether a row may be used: `no` (with the reason in `note`) when weight, diameter or, for the
  makers that publish it (Berger, Nosler, Speer), the length is missing. Set it to `no` by hand to exclude a bullet.
- Fiocchi is not included: its site refuses automated requests.
- Sako publishes its bullets only as loaded cartridges (caliber, weight, BC); the diameter comes from the project's caliber records.
- Hornady, Sierra, Barnes, Sellier & Bellot, Lapua, Norma, RWS, PPU, Federal, Winchester, Remington, H&N and Sako publish no length: `bullets_to_json.py` estimates it from mass, diameter
  and bullet type (`estimate_length.py`, about 5-10 % uncertain) and says so in `descr`. Their BCs are missing
  (Hornady: G1 only) as the sites do not publish them.
- The base diameters of boat tails are not published; only the boat tail length is stored (Berger).
