# Where the project database comes from

The records in `data/` were collected from public sources with scripts; nothing was copied from the database of Gordon's
Reloading Tool. The scripts, the collected tables (CSV) and the notes on quality are in the repository, so the data can be
rebuilt and checked. Records made from estimates say so in their `descr` field.

- **Calibers (467)**: the C.I.P. TDCC data sheets (public PDFs of bobp.cip-bobp.org, tables I-V) are read with
  `scripts/data/cipdata/cip_extract.py`: dimensions of cartridge and chamber, pressures, angles. The sheets give no case capacity;
  `cip_estimate_v.py` estimates it from the outline (about 6 % rms against known capacities, 10 % for rimfire). 25 more
  cartridges that have no sheet (SAAMI, wildcats, newer than the sheets: .224 Valkyrie, 6.8 Western, .22-.33 Nosler, ...) come
  from Wikipedia's cartridge infoboxes (`wiki_calibers.py`); they lack body and shoulder lengths and tolerances. `cipname` is the
  C.I.P. name, `altname` other names. Shotshells are not included. `Pmax` is the C.I.P. maximum pressure; `PmaxSAAMI` (a column the
  project added, shown next to it) the SAAMI maximum average pressure of 97 cartridges, from Wikipedia's infoboxes
  (`scripts/data/cipdata/saami_pressures.py`). Load data makers use the lower of the two.
- **Projectiles (2470)**: the product pages of 16 manufacturers (Berger, Nosler, Speer, Hornady, Sierra, Barnes, Sellier & Bellot,
  Lapua, Norma, RWS, PPU, Federal, Winchester, Remington, H&N, Sako) and, from the listing of the reloading shop grauwolf.net (`grauwolf_fetch.py`, title only: maker, type,
  diameter, weight), 709 bullets of Fox, Sax, Degol, DK Bullets, LOS, Brenneke, GECO, Magtech, CopperBear, Frontier, GPA, Cutting Edge, Ballistix Academy,
  North Fork, Leader and Wi-La-Tech (366 lengths measured on the product photos, the rest estimated and scaled with the maker's photo ratio: Sax x1.29, Fox x1.18), read with `scripts/data/bulletdata/*_fetch.py`: weight,
  diameter and, where published, ballistic coefficients. Only Berger, Nosler and Speer publish the bullet length; for the others
  it is estimated from mass, diameter and bullet type (`estimate_length.py`, about 5-10 % uncertain; lead bullets by their shape: 8.1 - 9.0 g/cm3 effective density, measured on photos).
  The lengths of the 64 H&N bullets (hn-sport.de) are measured on the product photo of the single standing bullet (`hn_photos_fetch.py`,
  `measure_standing_bullet.py`: camera elevation 10 degrees, scale fixed with the caliper-measured 20.0 mm of the TC .458 300 gr HS, about 4 % uncertain; table
  `bullets_hn_photos.csv`), those of LOS bullets on the two-bullet photos of hunting-sport.com (`hunting_sport_fetch.py`, table `bullets_hunting_sport.csv`);
  `apply_hunting_sport_lengths.py` writes them into the records. Fiocchi's site refuses
  automated requests and is not included.
  The initial gas pressure (`gpressure`, the pressure at which the bullet starts and engraves) is an estimate from the construction
  (`start_pressure.py`): lead (cast, plated) 150 bar, jacketed 250 bar, brass monolithic 290 bar, copper monolithic (Barnes TSX/TTSX/LRX,
  Hornady CX/GMX, lead-free ...) 350 bar. Checked with the physical model against published pressures: copper loads median peak
  pressure error 7.6 -> 6.3 % (no longer 5 % too low), lead loads bias +4 -> -2 %; the original's 150 bar for jacketed handgun bullets
  made those results worse and is not used.
  The physical model starts pistol and revolver cartridges (C.I.P. table IV) with at least 0.3 x the permitted pressure when the barrel is shorter than 400 mm
  (`handgunStartFraction`, `handgunMaxBarrel_mm`): the published loads of the 9 mm Luger, .40 S&W and .357 Magnum need about 600 bar. The handgun powders'
  pressure bias went from -17 % to +6 % (rms 30 -> 24 %), rifle cartridges are unchanged.
- **Propellants (216)**: Vihtavuori N130-N170, N530-N570 and, with a low `Qlty`, the pistol and double-base powders N105-N120, N320-N350, 3N37, 3N38 and 24N41 (26), Lovex S040, S053, D063, S060-S071 and D073.4-D073.6 (11), Reload Swiss RS50-RS76 (6), ADI AR2205-AR2225 and Bench Mark (13: ADI Powders' rifle data with test barrel, velocities and pressures), Vectan Tubal 3000-8000 (4: from the Nobel Sport catalogue, which publishes no test barrel and no pressures, so a 600 mm barrel is assumed; its handgun powders fit too poorly and are not included), and
  55 rifle powders of Hodgdon, IMR, Winchester, Accurate and Ramshot (the Hodgdon and Western Powders manuals; Accurate 4100, Enforcer, H110 and W296 fit
  less well, 6-7 % in velocity, and the Lovex S040, S053 and D063 rest on 9-20 loads: their descriptions say so), and 34 shotgun powders
  (Vihtavuori N320, N340, 3N37, 3N38 and N105 (steel and lead shot, 48 loads of the 2020 tables, velocities only); Vectan A24, AS, A1, A0, 206V, GM3, D20, Prima SV and Prima V; Hodgdon Clays, Titewad, International, 700-X, Universal, Longshot, Titegroup, Winchester Super Handicap and WST, Ramshot Competition; Alliant Red Dot,
  Green Dot, Blue Dot, Unique, Herco, Promo, American Select, Clay Dot, Pro Reach and 20/28; lot "shotshell", for shotshell loads only: 2,616 loads of the Hodgdon
  manual's 12 gauge tables, 1,993 of Alliant's Reloader's Guide and 450 of the Vectan catalogue, burn rates refitted 2026-10-05).
  Every record carries a fit quality `Qlty` (0-1, the original's column "Model quality", 0 = unknown): it falls with the velocity error (0 at 15 %),
  the pressure error (0 at 30 %, where the data have pressures) and with fewer than 100 fitted loads; shown in Tools > Burn rate table.
  Vihtavuori: explosion heat (the "energy content") and bulk density are taken from its published powder table; the burn rate
  Ba was fitted to its published load data (start and maximum loads, about 4,000 samples), which contain no pressures. Lovex
  (Explosia's Reloading Guide 2019), Reload Swiss (online loading data of Nitrochemie), the Hodgdon Reloading Manual 2024 and the
  Western Powders Handloading Guide 8.0 publish pressures as well; the fit uses velocities and pressures (velocity error 3-6 % rms,
  pressure error 10-20 % rms; each record's `descr` states its numbers). None of them publishes the explosion heat or the bulk
  density: Qex is a typical value of a single-base (3800 kJ/kg) or double-base (4300 kJ/kg) powder (Lovex: S = single, D = double
  base by name; the others: the type that fits the velocities better), pcd is 900 kg/m3.
  k and the material density are typical values everywhere; the form function (a0, z1, z2) is not published, it is the combination
  that reproduces the velocities best and the data hardly determine it. Pistol and shotgun powders are included as well (Lovex S011-D037.2, Reload Swiss RS12-RS80,
  ADI AP/APS, Hodgdon Titegroup, Universal, Clays, HS-6, W231, ..., Vectan A1, AS, Ba6-Ba10; velocity error 7-19 %, pressure error 20-35 %, a low `Qlty`:
  their descriptions say "use with more caution"), only powders with fewer than 9 samples are not. `tools/csr_fitdata` and `scripts/data/loaddata` do the work, see
  `scripts/data/loaddata/README.md` for the method and the results.
- **Physical model values** (`data/physical/`, fitted, generated: do not edit by hand): `propellants.json` has the burn rate Ba and form function of
  157 powders for the physical model (`analysis/fit_physical_powders.py`, 127 fitted to velocities and pressures, 30 to velocities only), `calibers.json` a
  factor on the burn rate for 15 pistol and revolver calibers with at least 100 published loads (`analysis/fit_caliber_scales.py`, 0.93 - 1.21). Both are fitted
  to the published load tables (`scripts/data/loaddata/sources/samples`) without the loads that disagree with their own table by more than 15 %
  (`analysis/sample_filter.py`). Bullet photos for the lengths: see Projectiles above.

The details, the commands to repeat each step and the limits of the data are in `scripts/data/cipdata/README.md` and
`scripts/data/loaddata/README.md`. Please check every load against the manufacturer's current load data.
