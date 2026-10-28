# Release notes

Changes in ChamberSim Reloaded. Pre-built binaries are on the
[releases page](https://github.com/Schildkroet/ChamberSim-Reloaded/releases).

## 0.14

### Added and changed

- **Charge tab.** A fourth input tab after *Propellant* with large fields for the propellant charge, the number of
  ladder steps and the charge step, a grain/gram switch for the charge and the step, and a table of the ladder's
  charges, maximum pressures and velocities (click a row to show its curve). It edits the same fields as the
  propellant table.
- **Gun weight in the New load dialog**, so a new load has recoil figures without editing the file afterwards.

### Fixed

- **macOS:** the menu bar (File, Edit, View, Tools, Help) was missing; it is now the native menu at the top of the
  screen (the own title bar, if switched on, still holds it). The unit drop-downs in the property tables no longer
  show the cell's text through the box (issue 9).
- **Tests on Qt 6.12:** `tst_core` crashed because it ran without an application object; `tools/build.sh` works on
  macOS (no `nproc`).

## 0.13

### Added and changed

- **Powder names without spaces.** A file saved by the original program names a powder `RS 40`, the project data
  `RS40`: the physical burn values and the notice "Powder values differ from the database" now find the project's
  record of such a load (affects the ReloadSwiss RS series, ADI AR 22xx, Vectan Ba 9 / 10, Reloder 10X, StaBALL 6.5).
- **Refitted physical powder values**, and the gas share wording: 1/4 is the volume-average share (1/3 for the kinetic
  energy changes no fit result).
- **Bulk densities.** 152 propellant records have a sourced bulk density instead of the 900 kg/m3 default: the
  maker's guides (Vihtavuori, Shooters World, Lee, Nosler) or the original program's record, collected by
  github.com/fbastin/tireur_reloaded. The source is in the record's description. The solver uses the bulk density
  only for the load ratio and the charge-too-high check, not for velocity or pressure.
- **Projectile data.** Sax: all 89 bullets of the maker's own listing (sax-munition.de) with part number, length, BC
  and diameter, validated; they replace the 64 estimated ones. Lapua: lengths, base data and BC of 126 bullets from
  the maker's bullet file. Seven bullets of LOS, GECO and H&N measured with a caliper. Hornady ELD Match 26177: length
  33.274 mm as Hornady lists it, was 32.8 (issue 8).
- **Caliber.** 375 XLR is a C.I.P. cartridge.

## 0.12

### Added and changed

- **Bullet lead time (BLT) and optimal barrel time (OBT).** The results, the report (`result.BulletLeadTime`,
  `result.OptimalBarrelTime`, `result.OptimalBarrelNode`) and `csr_sim` show the time from 10 % of the peak pressure
  to the muzzle and the nearest node of Chris Long's OBT master model (nodes 0 to 24.5 with the original's half
  nodes); it reproduces the original program's values.
- **Result panel.** Maximum pressure, muzzle velocity, projectile energy and barrel time as large cards, the other
  results in three groups (Barrel, Projectile, Propellant); the overpressure warning stands above the results.
- **Themes.** New color schemes Gruvbox Light and Dark, Nord, Dracula, Catppuccin Latte and Frappé
  (*View ▸ Theme*, `--theme`). *Light* is now a fixed light scheme on a dark desktop too, *System default* follows
  the color scheme of the operating system (Qt 6.5 and later), and the category rows of the property tables follow a
  theme change.
- **Start pressure and primer.** The start pressure of a load is again the original's value: 250 bar plus the
  *Additional Start Pressure*, without the -400 to 400 bar limit of 0.11. Only the Physical Model adds the
  difference between the primer type's start pressure and 250 bar; the Original and Published models get the
  load's start pressure unchanged. A `.grtload` keeps its start pressure exactly (0.11 changed it for bullets far
  from 250 bar, the original's records go from 20 to 950 bar). A load of 0.11 that was converted from an older file
  can show a different start pressure in the Original model (Small Pistol: 196 bar less).
- **Loads without a caliber record.** A `.grtload` from another tool that has no `<caliberfile>` block gets the
  caliber record it names (*CaliberName*) from the database: cartridge drawing, primer and the pistol calibration
  work for it (issue 6).
- **Table search.** The records that match the query best are listed first: the whole cell, then its beginning,
  then somewhere in it.
- **Projectile data.** 593 validated bullets (was 420): the Berger reference chart (111), Federal's published
  lengths (29) and Fox's bullet file (33 Classic Hunter, with BC). Lengths measured on the makers' product pictures
  for Sierra (127), Norma (51), Lapua (30) and Hornady; Federal BCs and a Brenneke BC table. New: Hornady XTP 35571
  and 5 GECO bullets of the maker's reloading list (Hexagon .357/.431/.451, JHP and FMJ .451).
- **Propellants.** New records with physical burn values: Hodgdon H414, IMR 4007 SSC, IMR 7977 and Supreme 780. The
  powder fit now also uses Barnes' load data (3,904 loads) and the H&N load tables; a search over several start
  shapes changed the burn values of Alliant 4000-MR, Reloder 26, TS 15.5 and IMR 7977, the other powders were
  already close to their best values.
- **Caliber.** 375 XLR (9.52 x 70).
- **Import bullets.** A bullet that is listed twice in a file is imported once, and a file is imported completely or
  not at all.
- The record export adds `.json` also to names with a dot (`9.3x62`); the search is faster in large tables.

## 0.11

### Added and changed

- **Primer per load.** The Primer (small or large, pistol or rifle, standard or magnum, .50 BMG) of the
  *Cartridge & gun* tab gives the start pressure; the old start pressure value is now the *Additional Start Pressure*
  (-400 to 400 bar) on top. A new load takes the usual primer of the caliber (484 caliber records have one) and its
  additional start pressure from the bullet record; loads of earlier versions keep their start pressure. Fitted to
  14,547 Hodgdon loads: Large Rifle 250 bar, the others 230 to 450 bar. Rifle pressure rms over all published loads
  12.62 to 12.54 %, velocity unchanged; pistol and revolver loads start at 0.3 x Pmax at least, so there is no change.
  Files exported as `.grtload` contain only the sum.
- **Measured pressure loads.** Eight DEVA protocols and one Beschussamt Muenchen test (.458 SOCOM) are used in the
  powder fit. N340 was refitted with its 9 mm Luger loads, N120 with a better starting shape (.458 SOCOM peak pressure
  +72 % to +17 %), and the form functions of 27 powders without published pressures were refitted from several start
  shapes. Velocity rms over all published loads: rifle 3.48 to 3.46 %, pistol 7.14 to 7.10 %.
- **Report: twist length and bullet jump.** New placeholders `work.twistlen` and `work.gjump`, shown next to the
  barrel length in the default template.
- **Sebert factor.** Every value from 0 to 1 (0 included) is kept as loaded and used as it is by all models; the
  Physical Model no longer takes 0.5 for 0 in propellant impulse and recoil. Values outside 0 to 1 become 0.5, the
  property panel accepts 0 to 1 only.
- **File names.** *Export report as PDF* adds `.pdf` and the record export `.json` to a name without that suffix.
- Barnes TTSX 30642 got an estimated boat tail.

## 0.10

### Added and changed

- **Bullet file for manufacturers (`.csrbullets`).** Makers can publish the data of their projectiles as one
  JSON file; *Database ▸ Projectiles ▸ Import bullets* adds them to the user's own records (nothing is sent
  anywhere). Format and an example: `docs/BULLET_FILE.md`, `docs/examples/example.csrbullets`.
  `bul_to_json.py` converts QuickLOAD bullet files.
- **Projectiles: "Validated" column.** A new boolean field marks bullets whose length was measured on the
  bullet or checked against the original's database (420 records); it is shown as a green check mark (yes) or
  a red cross (no).
- **Projectile database: 3,010 bullets from 51 makers.** New: 52 Hasler monolithic copper bullets, 230 Styria
  Arms AERO and 33 Balistix bullets (from the makers' bullet files), 214 bullets of the grauwolf.net listing that
  were missing (15 new makers). The lengths of the estimated records (Barnes, Brenneke, Hornady, Lapua, LOS, PPU,
  RWS, Sierra) were recalculated, and 142 more bullets got a base type.
- **Physical Model: burn-rate factor per rifle caliber.** 71 rifle calibers with at least 60 published loads now
  carry a factor on the powder's burn rate (0.92 to 1.21) like the pistol and revolver calibers did. On powders
  left out of the fit: rifle velocity 3.79 to 3.29 % rms, peak pressure 13.1 to 12.6 %. Pistol and revolver
  figures are unchanged. A higher start pressure and a factor on the case volume were tested and not adopted
  (`docs/ACCURACY.md`).
- **The report footer shows the program version** (key `app.version` in the default template).
- **No ellipsis on button labels** (for example "Import bullets", "Export").
- **User manual as a web site.** The manual chapters (`docs/manual/ug_*.md`) are built with MkDocs into a web
  site and a PDF; the site is published to GitHub Pages when a version tag is pushed
  (`docs/build/README.md`).

## 0.9

### Added and changed

- **Physical Model: new gas equations.** The gas column is treated with the classical Lagrange share
  (1/4 of the charge moves with the bullet) and the powder burns at the breech pressure; the part of
  the bullet's ogive seated in the case is counted with its real volume and the free run is no longer
  counted twice in the gas volume. The primer releases its energy over about 0.05 ms and the chamber
  walls are at the load's powder temperature.
- **Physical Model: own start resistance for rifles and handguns.** With one setting for all
  cartridges no variant improved both classes. Rifles now use 1.2 x the start pressure decaying over
  2 mm, pistols and revolvers (C.I.P. table IV) 0.6 x the start pressure over the bearing length as
  before. Against the published loads: rifle velocity / peak pressure 4.00 / 13.85 to 4.00 / 13.13 %
  rms, pistol 7.15 / 16.86 to 7.13 / 16.66 %. The agreement with the original program's reference runs
  got worse (peak pressure mean 29 to 40 %); the GRT Original model is unchanged. The constants
  `engravingPressure_bar` (absolute engraving pressure), `engravingLength_mm`, `neckStress_MPa` (case-neck
  friction) and the handgun values `engravingShareHandgun` / `engravingLengthHandgun_mm` can be set with
  `csr_eval --set`; the tests that led here are in the Solver model chapter of `docs/SOLVER.md`.
- **Powder values refitted without invalid loads.** 369 published loads whose pressure was more than
  45 % off the simulation (misread or invalid, mostly Reload Swiss, Hodgdon and ADI pistol) were taken
  out of the sample data and kept in `samples/invalid/invalid_samples.csv`; Ba of 173 powders was
  refitted. 40 of the 193 powders carry an effective grain geometry (ball or single-perforated cylinder)
  instead of the cubic form function; the values are fitted to the form function, not measured.
- **New developer tools.** `csr_dbeval` simulates loads made of records of the original's database
  by id. `analysis/compare_grt_published.py` compares GRT Original with published pressures using the
  original's own records; `analysis/compare_physical_published.py` compares the shipped powder values with
  published loads for rifle and pistol separately, for two builds side by side;
  `analysis/filter_published_outliers.py` finds and moves invalid loads.
- **Shorter label for the extra friction.** The property panel shows "Bullet friction"
  instead of "Special Bullet Resistance Inside The Barrel" (German, French and Italian
  as well); the German, French and Italian translations have no unfinished entries left.
- **User manual included.** The AppImage and the Windows zip contain the manual (PDF);
  *Help ▸ User manual* (F1) opens it.
- **Windows: the Visual C++ runtime is bundled**, so the program no longer depends on the
  version installed on the PC.
- **Windows: icon and version information in the program file.**

### Fixed

- **The cartridge drawing uses the load's case length** (issue #3). It took the case length from the
  caliber record, so a trimmed case was drawn too long and the overall length shifted by the same
  amount.
- **Property panel: the values are visible without scrolling** (issue #2). The default
  column widths followed the panel before the table had its new size, so the property
  column took the whole width; they now follow the table itself.
- **`csr_sim` gives the same results as the program.** The command-line simulation did not
  have the project's physical burn values and pistol / revolver caliber factors built in,
  so loads without stored burn values (every `.grtload`, older files) were calculated with
  the scaled record burn rate: for example 4,711 bar instead of 3,881 bar for a .308 Win.
  load.
- **A large negative extra friction no longer multiplies the pressure** in the GRT models:
  its effect is limited to half the pressure force (the original's database goes down to
  -100 bar, about 3.5 %).

## 0.8

### Added and changed

- **The friction unlock switch is gone; the extra friction always applies.** The "unlocked
  friction" row is no longer shown, and a bullet's extra friction (`gfriction`, bar) acts
  whether or not an old file has it switched on. Older loads with a non-zero friction and
  the switch off now include that friction in the result. A `.grtload` export still writes
  the original's switch as on, so the original program applies the friction too. Negative
  values stay possible: the original's database uses them for low-friction bullets
  (Peregrine -100 bar, TPM -80 to -95 bar).
- **Own start pressure and friction for 362 rifle bullets.** For every published load with a
  pressure, the loads more than 5 % off in velocity or pressure are swept over start pressure
  (50 to 500 bar) and friction (-100 to 150 bar) in steps of 5 bar, and each bullet takes the
  median of its loads (`analysis/fit_bullet_start_values.py`, bullets with at least 8 loads).
  Checked on powders that were not used for the fit: velocity error 3.9 to 3.5 %, pressure
  error 25.1 to 23.7 %, loads more than 5 % off in pressure 68 to 59 %. Bullets that also
  appear in pistol or revolver loads are left out (their start pressure is set by the solver).
  Existing loads keep their stored values until the bullet is selected again.
- **`csr_eval` takes per-row `gpressure` and `gfriction` columns** (for the fits).

## 0.7

### Added and changed

- **Vihtavuori powders refitted with the C.I.P. limit.** The physical burn values of
  26 Vihtavuori powders (rifle N130-N170 and N530-N570, pistol and fast powders
  24N41, N105, N110, N120, N320, N330, N340, N350, 3N37, 3N38) are fitted to the
  published velocities and to the cartridge's C.I.P. maximum pressure minus
  50 bar for every maximum load, which the Vihtavuori tables give instead of
  measured pressures. The 16 rifle powders: over their 3,996 maximum loads the
  model now stays at 0.98 of the limit on average (before 1.07), loads more than
  20 % over the limit drop from 729 to 154, and the velocity error stays at
  3.2 % rms. The pressure of one load still scatters by about 12 % (for example
  a 178 gr bullet with a compressed charge of N140 in .308 Win.: 4,660 bar
  instead of 5,050 bar, limit 4,150 bar). The 10 pistol and fast powders were
  at 1.24 of the limit on average and are now at 0.93 (loads more than 20 % over:
  269 of 511 before, 34 now); their velocity error falls from 12.9 % to 8.0 %
  rms, still much worse than for rifle powders. N310, N32C and 20N29 have too
  few loads and keep their record values. Existing loads keep their stored
  values until the propellant is selected again.
- **Rottweil, Alliant and Vectan powders refitted with the C.I.P. limit.** Their
  tables give no pressures either, so the 40 powders (5 Rottweil, 26 Alliant,
  9 Vectan) are fitted like the Vihtavuori ones: the maximum loads count as the
  cartridge's C.I.P. limit minus 50 bar. Mean calculated pressure of the maximum
  loads / limit: Rottweil 1.09 to 0.98, Alliant 1.12 to 0.97, Vectan 1.42 to 1.01;
  loads more than 20 % over the limit: 73 to 9, 471 to 37 and 164 to 36. The
  velocity error rises a little (Rottweil 3.5 to 3.6 %, Alliant 3.9 to 4.5 %,
  Vectan 6.1 to 7.2 % rms). The American Alliant loads are limited by SAAMI, which
  differs from C.I.P. for some pistol cartridges. Alliant American Select, Red Dot, 1200-R and
  Reloder 33 have too few loads to fit the burn shape and use the median.
- **Maker names of the Rottweil and Reload Swiss records match the original database.**
  The project records were called "Rottweil" and "Reload Swiss", the original
  database "RWS-Rottweil" and "ReloadSwiss", so loads made with the original's
  records (maker "RWS-Rottweil", e.g. R904) found no record of the project and the
  powders were listed twice. The project records now use the original's spelling.
  Loads saved with the old names ("Rottweil", "Reload Swiss") no longer match the
  project records; select the propellant again to update them.

## 0.6

### Fixed

- **Windows: crash when creating or opening a load.** The program stopped as soon
  as it needed the physical burn values of a propellant, on PCs whose Visual C++
  runtime (msvcp140.dll) is older than the compiler of the release build. The
  Windows build no longer uses the mutex constructor that fails with the older
  runtime.
- **Exporting a record no longer shows a false warning.** The check for the
  project's database ran on the database row, which also holds `id`,
  `geloescht` and `source`, so every export of a caliber, projectile or
  propellant warned that these "must not be in the file" or were an "unknown
  column". The exported file was always correct; only the check was wrong, it
  now looks at the record as it is written.

### Added and changed

- **Simulate from the command line.** The new program `csr_sim` runs the solver
  without a window and prints the results of one or several loads as a table,
  CSV or JSON (maximum and muzzle pressure, velocity, energy, burnt fraction,
  ladder steps, ...), optionally with the pressure/velocity curve as CSV. The
  charge and the ladder can be overridden and the solver model chosen. The
  exit code tells scripts whether every simulation succeeded. See the manual
  (Command-line options) and `docs/CHEATSHEET.md`.
- **Shotshell powders of Vihtavuori** — burn-rate records (lot "shotshell") for
  N320, N340, 3N37, 3N38 and N105, fitted to Vihtavuori's 2020 shotgun tables
  for 12/76 (3") lead and steel shot, 48 start and maximum loads in all. They
  are used for shotshell loads only; rifle and handgun cartridges keep their
  existing records. In shotshell mode the velocity error of these powders on
  those loads falls from 13.5 % to 3.0 % rms (bias -10.6 % to -1.0 %). Fitted on
  lead only and tested on steel it is 4.4 %, the other way round 2.2 %.
- **Pressures of these powders are not validated.** Vihtavuori publishes none
  for these loads, so the records carry a low quality (0.05). Maximum loads
  come out at about 440-1,130 bar in the model, a few steel loads slightly
  above the 1,050 bar C.I.P. limit. Check every load against the
  manufacturer's current data.
- **Steinbock Precision bullets** — 11 boat tail bullets in .264, .277, .284,
  .306 (7.5x55), .308, .323 and .338 with weight, length, base diameter,
  boat-tail length and G1 ballistic coefficient. The two .308 165 gr bullets
  (standard and Magnum) are separate records because their BCs differ.
- **Alliant rifle, pistol and revolver powders** — 26 new propellant records
  from the recipes of Alliant's online Reloader's Guide (1,522 maximum loads,
  211 pistol / revolver and 605 rifle recipe pages): Reloder 7, 10X, 15, 16,
  17, 19, 22, 23, 25, 26 and 33, 2000-MR, 4000-MR, AR-Comp, TS 15.5, Varmint,
  1200-R, 2400, Bullseye, Unique, Power Pistol, BE-86, Sport Pistol, Blue Dot,
  Red Dot and American Select. Burn rates and the physical model's burn values
  are fitted to the published velocities: about 2-4 % rms for the rifle
  powders, 3-10 % for the pistol and revolver powders (Bullseye, 2400 and Blue
  Dot are the least accurate). Alliant publishes no pressures, so the records
  have a low quality, and whether a powder is single or double base is assumed
  from the fit. Check every load against Alliant's current data.
- The database now has 245 propellant records, and the physical model has burn
  values for 183 powders.

### Project

- All C++ source files carry a GPL-3.0-or-later license header.
- `csr_eval` (developer tool): new `shot_density` column for steel shot, and
  the "shotshell" powder records are used for shotshell rows only, other rows
  use the regular records. Pistol and rifle fit figures may change slightly.
