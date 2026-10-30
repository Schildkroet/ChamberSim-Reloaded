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
  (`scripts/data/cipdata/saami_pressures.py`). Load data makers use the lower of the two. `primer` (also a column the project added) is the usual primer
  type a new load starts with: for 142 cartridges the most frequent primer of the Hodgdon, Alliant and Vihtavuori test cartridges, for the
  others from the case (base diameter, case type; `scripts/data/cipdata/caliber_primers.py`).
- **Projectiles (2470)**: the product pages of 16 manufacturers (Berger, Nosler, Speer, Hornady, Sierra, Barnes, Sellier & Bellot,
  Lapua, Norma, RWS, PPU, Federal, Winchester, Remington, H&N, Sako) and, from the listing of the reloading shop grauwolf.net (`grauwolf_fetch.py`, title only: maker, type,
  diameter, weight), 709 bullets of Fox, Degol, DK Bullets, LOS, Brenneke, GECO, Magtech, CopperBear, Frontier, GPA, Cutting Edge, Ballistix Academy,
  North Fork, Leader and Wi-La-Tech (lengths measured on the product photos, the rest estimated and scaled with the maker's photo ratio: Fox x1.18). Sax publishes weight, diameter, length and BC of every bullet on sax-munition.de: the 89 bullets are read from there (`sax_fetch.py`, validated), read with `scripts/data/bulletdata/*_fetch.py`: weight,
  diameter and, where published, ballistic coefficients. Only Berger, Nosler and Speer publish the bullet length; for the others
  it is estimated from mass, diameter and bullet type (`estimate_length.py`, about 5-10 % uncertain; lead bullets by their shape: 8.1 - 9.0 g/cm3 effective density, measured on photos).
  The lengths of the 64 H&N bullets (hn-sport.de) are measured on the product photo of the single standing bullet (`hn_photos_fetch.py`,
  `measure_standing_bullet.py`: camera elevation 10 degrees, scale fixed with the caliper-measured 20.0 mm of the TC .458 300 gr HS, about 4 % uncertain; table
  `bullets_hn_photos.csv`), those of LOS bullets on the two-bullet photos of hunting-sport.com (`hunting_sport_fetch.py`, table `bullets_hunting_sport.csv`);
  `apply_hunting_sport_lengths.py` writes them into the records. Federal publishes the length and BC of its component bullets (`federal_fetch.py`,
  `apply_csv_lengths.py`); Berger's bullet reference chart gives the length of 111 bullets (`berger_chart.py`, all marked validated); the 195 estimated Hornady lengths were
  replaced by 167 measured on the side-view illustrations of hornady.com (`hornady_images.py`, `measure_side_photos.py`, `apply_hornady_images.py`: 1 % too long, 5 % rms against the
  54 validated records; a picture more than 30 % from the estimate is ignored); the 127 estimated Sierra lengths were replaced by lengths measured on the product photos of sierrabullets.com (`sierra_images.py`; the brand banner is cut off; +1 % and 3 % rms against the 110 validated records) and 51 Norma lengths by measurements on the renderings of norma-ammunition.com (`norma_images.py`; `apply_measured_lengths.py` writes them, `apply_hornady_images.py` was generalised into it); the table of the Brenneke reloading bullets gives the BC (`brenneke_table.py`, no lengths). Fiocchi's site refuses
  automated requests and is not included.
  The initial gas pressure (`gpressure`, the pressure at which the bullet starts and engraves) is an estimate from the construction
  (`start_pressure.py`): lead (cast, plated) 150 bar, jacketed 250 bar, brass monolithic 290 bar, copper monolithic (Barnes TSX/TTSX/LRX,
  Hornady CX/GMX, lead-free ...) 350 bar. Checked with the physical model against published pressures: copper loads median peak
  pressure error 7.6 -> 6.3 % (no longer 5 % too low), lead loads bias +4 -> -2 %; the original's 150 bar for jacketed handgun bullets
  made those results worse and is not used.
  Where a bullet matches a record of the original's database (maker, name, mass, diameter; 335 of 2,481), `scripts/data/bulletdata/apply_original_start_values.py`
  takes that record's `gpressure` and, when it is not 0, its `gfriction` (Fox Classic Hunter +100 bar, whole line; the original also has negative values,
  e.g. Peregrine -100 bar). Lead bullets keep 150 bar.
  Rifle bullets with at least 8 published loads (362 records) then got their own `gpressure` and `gfriction` (-100..150 bar, 5 bar steps) from
  `analysis/fit_bullet_start_values.py`: loads more than 5 % off in velocity or pressure are swept over both, the record gets the median. Checked on
  held-out powders: velocity error 3.9 -> 3.5 %, pressure 25.1 -> 23.7 %. Handgun bullets are not fitted (the solver sets their start pressure).
  The physical model starts pistol and revolver cartridges (C.I.P. table IV) with at least 0.3 x the permitted pressure in every barrel
  (`handgunStartFraction`; until 0.14 only below 400 mm, which made the peak pressure jump at that length, issue 11): the published loads of the 9 mm Luger, .40 S&W and .357 Magnum need about 600 bar. The handgun powders'
  pressure bias went from -17 % to +6 % (rms 30 -> 24 %), rifle cartridges are unchanged.
- **Propellants (245)**: Vihtavuori N130-N170, N530-N570 and, with a low `Qlty`, the pistol and double-base powders N105-N120, N320-N350, 3N37, 3N38 and 24N41 (26), Lovex S040, S053, D063, S060-S071 and D073.4-D073.6 (11), Reload Swiss RS50-RS76 (6), ADI AR2205-AR2225 and Bench Mark (13: ADI Powders' rifle data with test barrel, velocities and pressures), Vectan Tubal 3000-8000 (4: from the Nobel Sport catalogue, which publishes no test barrel and no pressures, so a 600 mm barrel is assumed; its handgun powders fit too poorly and are not included), and
  55 rifle powders of Hodgdon, IMR, Winchester, Accurate and Ramshot (the Hodgdon and Western Powders manuals; Accurate 4100, Enforcer, H110 and W296 fit
  less well, 6-7 % in velocity, and the Lovex S040, S053 and D063 rest on 9-20 loads: their descriptions say so), 26 Alliant rifle, pistol and revolver powders (Reloder 7-33, 2000-MR, 4000-MR, AR-Comp, TS 15.5, Varmint, 1200-R, 2400, Bullseye, Unique, Power Pistol, BE-86, Sport Pistol, Blue Dot, Red Dot, American Select: 1,522 maximum loads of the Reloader's Guide on alliantpowder.com, velocities only), and 34 shotgun powders
  (Vihtavuori N320, N340, 3N37, 3N38 and N105 (steel and lead shot, 48 loads of the 2020 tables, velocities only); Vectan A24, AS, A1, A0, 206V, GM3, D20, Prima SV and Prima V; Hodgdon Clays, Titewad, International, 700-X, Universal, Longshot, Titegroup, Winchester Super Handicap and WST, Ramshot Competition; Alliant Red Dot,
  Green Dot, Blue Dot, Unique, Herco, Promo, American Select, Clay Dot, Pro Reach and 20/28; lot "shotshell", for shotshell loads only: 2,616 loads of the Hodgdon
  manual's 12 gauge tables, 1,993 of Alliant's Reloader's Guide and 450 of the Vectan catalogue, burn rates refitted 2026-10-05).
  Every record carries a fit quality `Qlty` (0-1, the original's column "Model quality", 0 = unknown): it falls with the velocity error (0 at 15 %),
  the pressure error (0 at 30 %, where the data have pressures) and with fewer than 100 fitted loads; shown in Tools > Burn rate table.
  Vihtavuori: explosion heat (the "energy content") and bulk density are taken from its published powder table; the burn rate
  Ba was fitted to its published load data (start and maximum loads, about 4,000 samples), which contain no pressures. Lovex
  (Explosia's Reloading Guide 2019), Reload Swiss (online loading data of Nitrochemie), the Hodgdon Reloading Manual 2024 and the
  Western Powders Handloading Guide 8.0 publish pressures as well (the Barnes Bullets load data PDFs, 3,904 start and maximum loads of 56 cartridges, give velocities only: H414, Supreme 780, IMR 4007 SSC and IMR 7977 were added from them together with the Hodgdon manual); the fit uses velocities and pressures (velocity error 3-6 % rms,
  pressure error 10-20 % rms; each record's `descr` states its numbers). None of them publishes the explosion heat or the bulk
  density: Qex is a typical value of a single-base (3800 kJ/kg) or double-base (4300 kJ/kg) powder (Lovex: S = single, D = double
  base by name; the others: the type that fits the velocities better), pcd is 900 kg/m3 unless a value is known: 130 records have a bulk density from the maker's guides (Vihtavuori, Shooters World, Lee, Nosler) or the original program's record, as collected by github.com/fbastin/tireur_reloaded (MIT; `apply_bulk_densities.py`; the source is in `descr`). The solver uses it only for the load ratio and the charge-too-high check, not for velocity or pressure.
  k and the material density are typical values everywhere; the form function (a0, z1, z2) is not published, it is the combination
  that reproduces the velocities best and the data hardly determine it. Pistol and shotgun powders are included as well (Lovex S011-D037.2, Reload Swiss RS12-RS80,
  ADI AP/APS, Hodgdon Titegroup, Universal, Clays, HS-6, W231, ..., Vectan A1, AS, Ba6-Ba10; velocity error 7-19 %, pressure error 20-35 %, a low `Qlty`:
  their descriptions say "use with more caution"), only powders with fewer than 9 samples are not. `tools/csr_fitdata` and `scripts/data/loaddata` do the work, see
  `scripts/data/loaddata/README.md` for the method and the results.
- **Physical model values** (`data/physical/`, fitted, generated: do not edit by hand): `propellants.json` has the burn rate Ba and form function of
  193 powders for the physical model (`analysis/fit_physical_powders.py`, 189 fitted to velocities and pressures, 4 to velocities only; for the Vihtavuori, Rottweil, Alliant and Vectan powders, whose tables give no pressures, the maximum loads count as the C.I.P. limit minus 50 bar, `--limit-pressure --limit-margin 50`), `calibers.json` a
  factor on the burn rate for 15 pistol and revolver calibers with at least 100 and, since 2026-10-19, for 71 rifle calibers with at least 60 published loads and a factor that differs from 1 by 1 % or more (`analysis/fit_caliber_scales.py --rifles`, 0.92 - 1.21). Both are fitted
  to the published load tables (`scripts/data/loaddata/sources/samples`) without the loads that disagree with their own table by more than 15 %
  (`analysis/sample_filter.py`) and, since 2026-10-17, without 369 loads whose pressure is more than 45 % off the simulation (probably misread or invalid;
  `analysis/filter_published_outliers.py`; they are kept in `samples/invalid/invalid_samples.csv`). 40 of the 193 powders carry an effective grain geometry (ball or
  single-perforated cylinder, `analysis/fit_grain_geometry.py`): fitted to the powder's form function, not measured; no manufacturer publishes the perforation.
  Bullet photos for the lengths: see Projectiles above.

The details, the commands to repeat each step and the limits of the data are in `scripts/data/cipdata/README.md` and
`scripts/data/loaddata/README.md`. Please check every load against the manufacturer's current load data.
