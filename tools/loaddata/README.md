# Propellant models from published load data

Manufacturers publish load tables: charge, bullet, cartridge length, barrel, velocity (and sometimes pressure). That is
the data of the Measurements tab, so a propellant model can be fitted to it without own shooting.

1. `vv_extract.py N150 -o vv_N150.csv` reads Vihtavuori's reloading database (the JSON file behind the reloading data
   tool of vihtavuori.com) and writes one sample per start load and per maximum load of the powder. Cartridges and
   bullets are matched to the project's records (`data/calibers`, `data/projectiles`); a bullet that is not there gets a
   length from `tools/bulletdata/estimate_length.py`. The case capacity is the (estimated) value of the caliber record.
2. `grt_fitdata vv_N150.csv --params Ba` fits the model values (`--help` lists the options: start values, fitted
   values, pressure assumption, `--holdout` calibers that are only evaluated, `--out` for a record file).

Experience with N150 (920 samples, 34 cartridges):

| Fit | Velocity error (rms) |
|---|---|
| generic start values (Ba 0.6, a0 1.2, z1 0.45, z2 0.8) | 4.3 % |
| Ba fitted (0.578) | 3.97 % |
| Ba fitted on other cartridges, evaluated on 308 Win., 30-06, 6.5 Creedmoor, 260 Rem. | 3.4 % (Ba 0.581) |
| Ba, a0, z1, z2 fitted | 3.7 % (a0 runs to 0: velocities do not determine the form function) |

The load tables contain no pressures. A form function needs pressure data, so only Ba is determined well. The scatter of
3-4 % comes from the table data and the inputs that a table does not give (case capacity of a fired case, bullet length).
`Qex`, `k` and the densities stay at the start values: take them from the manufacturer's safety data sheet.

## Independent values for the powder record

Vihtavuori publishes a technical table per powder (industrial products brochure 2022, `vv_powder_data.csv`): relative burn rate
(N110 = 100, manometric bomb), grain length and diameter, bulk density, energy content in J/g and the nitroglycerine content of the
N500 series. The records use the **energy content as Qex** and the **bulk density as pcd** (the brochure states for N565 an
"energy content of 4000 J/g"). Fitted with these values, the velocity error is a little smaller than with generic ones
(3.9 % instead of 4.2 % mean) and no sample is rejected for a charge that does not fit the case.

Not published: k and the material density pc (typical values 1.23 and 1580 kg/m3 for nitrocellulose powders) and the form function
(a0, z1, z2). The grain geometry does not determine the form function of this model (its phases are calibrated shapes, and
powders with the same grains but different coatings burn differently), so `vv_fit_all.py --form` tries candidates while Ba is
fitted for each powder. Mean velocity error over the 16 rifle powders: a0 = 0, z1 = 0.3, z2 = 0.6: 3.92 %; 0 / 0.2 / 0.6: 3.94 %;
0 / 0.4 / 0.7: 3.95 %; 1.2 / 0.45 / 0.8: 4.17 %; 2.5 / 0.3 / 0.8: 4.15 %; 2.5 / 0.6 / 0.9: 4.43 %. The data hardly decide, the records
use the best, a0 = 0, z1 = 0.3, z2 = 0.6. Pressure data would be needed to determine the form function.

Plausibility check: Ba divided by the published relative burn rate is nearly constant for the N130-N170 series (0.0137-0.0155).

`vv_fit_all.py --table vv_powder_data.csv --form 0,0.3,0.6` fits all 29 powders (`vv_fits.csv`); `vv_to_json.py` writes the records of the
powders with at most 5.2 % error from at least 80 samples (the 16 rifle powders). The pistol and double-base powders fit worse
(8-22 % for N310 - N350 and 3N37) because of the model at low pressure and are not included.

## Lovex and Reload Swiss (with pressures)

Both publish measured pressures with every load, so `grt_fitdata --use-pressure` fits velocities and pressures (the CSV column
`pressure_bar`; weight `--pressure-weight`, default 0.5).

- `lovex_extract.py guide.pdf` reads the rifle tables of Explosia's LOVEX Reloading Guide 2019 (`pdftotext -layout`): per cartridge
  barrel and case, per bullet maker, type, mass and cartridge length, per powder the recommended and the maximum load with V5 velocity
  and the pressure of the maximum load. 1068 samples of 11 rifle powders.
- `rs_extract.py` reads the loading data page of reload-swiss.com. The table is embedded in the page's Nuxt state (a minified function
  whose arguments are the values); the script decodes it without running any script. 2002 samples of 14 RS powders, start and maximum
  load with pressures.
- `samples_common.py`: matches cartridges (with the table's spelling variants) to `data/calibers` and bullets to `data/projectiles`,
  else an estimated length (`estimate_length.py`).
- `published_fit.py samples.csv --maker Lovex --base-from-name --source "..." --write` fits Ba per powder (form function 0 / 0.3 / 0.6)
  and writes the records of the powders with at least 40 samples and at most 6 % (Reload Swiss: 6.5 %) velocity error.
  Results: `lovex_fits.csv`, `rs_fits.csv`.

| Powder | Samples | Ba | Velocity rms | Pressure rms |
|---|---|---|---|---|
| Lovex S060 / S062 / S065 / S070 / S071 | 162 / 198 / 194 / 122 / 44 | 0.855 / 0.695 / 0.637 / 0.553 / 0.540 | 3.2 / 4.5 / 3.8 / 5.4 / 4.6 % | 10-17 % |
| Lovex D073.4 / .5 / .6 | 68 / 68 / 146 | 0.657 / 0.646 / 0.599 | 3.9 / 4.0 / 3.7 % | 11.5 % |
| Reload Swiss RS50 / 52 / 60 / 62 / 70 / 76 | 184 / 206 / 294 / 246 / 228 / 88 | 0.536 / 0.553 / 0.501 / 0.454 / 0.437 / 0.368 | 4.9 / 5.6 / 5.2 / 6.2 / 4.7 / 5.4 % | 14-21 % |

The burn rates order as the manufacturers describe their powders (D073.4 faster than .5 faster than .6; S060 > S062 > S065 > S070).
Fitting a0, z1, z2 as well with the pressures does not help (pressure error 15 % -> 15 %, the parameters run to the limits), so the
form function stays the chosen one. Not included: pistol powders (Reload Swiss RS12-RS36: 7-22 % velocity error), Lovex S040, S053,
D060, D063 (fewer than 40 samples) and RS40 / RS80 (7 / 11 %).

## Hodgdon, IMR, Winchester, Accurate, Ramshot

- `hodgdon_extract.py manual.pdf` reads the rifle tables of the Hodgdon Reloading Manual 2024 (two columns per page; the right column starts
  at the page's second "Powder" heading): per cartridge barrel, twist, per bullet maker code, weight, type, cartridge length, per powder start
  and maximum load with velocity and pressure. Pressures in CUP are not comparable to PSI: those rows keep only their velocities.
  14,000 samples of about 100 powders.
- `west_extract.py guide.pdf` reads the Western Powders Handloading Guide 8.0 (Accurate and Ramshot powders). Its text layer has a space
  between all characters of a data row, so a row is read with all spaces removed. 5,500 samples.
- `published_fit.py a.csv b.csv --source "..." --fits fits.csv --write` merges the files (identical samples count once), fits Ba per
  powder as above and writes the records with at most 6 % velocity error from at least 40 samples (`hodgdon_western_fits.csv` has all).
  The rifle powders fit with 3-5.5 % velocity and 11-25 % pressure error, the pistol and shotgun powders (Titegroup, Universal, Clays, W231,
  Unique, ...) with 7-20 %: the model is made for rifle pressures, they are not included.
- The burn rates order as Hodgdon's own burn rate chart (H4198 > H322 > H335 > H4895 > Varget > H4350 > H4831 > H1000 > Retumbo, ...).

Not used: Norma (norma-ammunition.com, one page per cartridge with minimum and maximum charge and velocity): no test barrel and no
pressures are published, and with an assumed barrel the fitted Ba values did not follow the manufacturer's burn rate chart. Norma's
burning rate chart (relative velocity and pressure of one reference load, many manufacturers) is a ranking, not a load: Ba estimated from
it has about 10 % scatter (against the Vihtavuori records), too coarse for records. Hodgdon's, Alliant's and Accurate's websites refuse
automated requests or are not available. Vectan's website lists only black powder charges.

## Vectan (Nobel Sport catalogue)

`vectan_extract.py catalogue.pdf -o vectan_samples.csv --ocr-cache ocr.json` reads the pistol, revolver and rifle tables of the Vectan
reloading catalogue (pages 6-23). They are drawn as graphics without a text layer, so the pages are read with RapidOCR
(`pip install rapidocr-onnxruntime pillow numpy`); the columns are found from the positions of the words, the cartridge names are
matched to the records by a table of the OCR spellings, and rows with implausible values (start load above the maximum, ...) are
dropped (`--list-dropped`). 1,146 samples of 11 powders. The catalogue gives **no test barrel and no pressures**: the barrel is
assumed (600 mm rifle, 120 mm handgun) and only velocities are fitted.

- Rifle powders Tubal 3000 / 5000 / 7000 / 8000 (`published_fit.py --base single-base --max-rms 6.6`): Ba 0.776 / 0.701 / 0.601 / 0.514
  (as the catalogue orders them, 3000 the fastest), velocity error 6.5 / 6.0 / 4.9 / 4.4 %. A rifle barrel 100 mm shorter changes Ba by about
  10 %. Physical model: Ba 1.44 / 1.27 / 1.10 / 0.93 with the median form function, velocity 5.5 / 3.7 / 2.7 / 2.9 %.
- Handgun powders (Ba6 1/2 ... Ba10, A1, AS): velocity error 10-16 % and Ba that depends strongly on the assumed barrel (Ba9: 2.2 with
  200 mm, 3.3 with 80 mm): not included, as the other pistol powders.
- Not read: the shotshell tables (pages 27-44, with pressures; A24, AS, A1, A0, 206 SV, 206 V, D20, Prima SV, Prima V, GM3).

## ADI Powders (Australia)

`adi_extract.py -o adi_samples.csv --cache dir` reads the rifle data of adiworldclass.com.au/data-rifle: one page per cartridge (191), with
the test barrel (barrel length, twist, primer) and per bullet and powder the cartridge length and the starting and maximum load with
velocity and pressure (psi). The site shows the data only after the visitor acknowledged its warnings; running the script submits that form
once (the user of the script accepts the Warnings and Reloading Safety pages of ADI). Subsonic and reduced-recoil loads are skipped. 7,296
samples (3,251 with pressure) of 21 powders on 133 cartridges (the others are not in the C.I.P. records).

Fitted with `published_fit.py --min-samples 20 --max-rms 6.5 --min-cartridges 3`: 13 powders (AR2205, AR2206H, AR2207, AR2208, AR2209,
AR2213SC, AR2217, AR2218, AR2219, AR2225, Bench Mark 1, 2, 8208), velocity error 4.2-6.4 %, pressure error 13-27 %. Physical model
(`fit_physical_powders.py`, form function fitted from the pressures): velocity 4.6 %, pressure 11 % rms over all 13. Not included: the pistol and
shotgun powders (AP, AS, Trail Boss, AR2206: 7-13 % velocity error or too few cartridges).

`adi_extract.py --weapon pistol` reads the pistol and revolver data of the same site (`/data-pistol/`, 83 cartridges, 2,340 samples of 52 cartridges, 918 with pressure).
On these loads the powders fitted on rifle data (physical model) come out 0.8 % low in velocity (7.6 % rms) and 13 % low in pressure (18 % rms).

## Sources saved locally (`tools/loaddata/sources/`, not in the repository)

The manufacturers' manuals and pages are kept for later refits in `tools/loaddata/sources/` (git-ignored: the files are the
manufacturers' publications, not ours to redistribute). `pdf/` holds the documents, `samples/` the CSV files made from them with the
extractors above (re-made 2026-10-04, ready for `fit_physical_powders.py`, `calibrate_published.py` and `analysis/scan_physical.py`),
`adi_*_pages/` and `vv_cache/` the downloaded web pages and the Vihtavuori database.

| Source | File | Where it came from |
|---|---|---|
| Hodgdon Reloading Manual 2024 | `pdf/Hodgdon_Reloading_Manual_2024.pdf` | hodgdonpowderco.com/wp-content/uploads/2025/10/Hodgdon_Reloading_Manual_2024.pdf |
| Western Powders Handloading Guide 8.0 | `pdf/WesternPowdersHandloadingGuide8.0_WEB-1.pdf` | provided by the user |
| Lovex rifle data (Explosia, reloading_Lovex_EN_37) | `pdf/Lovex_reloading_EN_37_rifle.pdf` | www.reloading.co.uk/trade/lovex.pdf (explosia.cz refuses automated requests) |
| Reload Swiss loading data | `pdf/rs_loading_data_page.html` | www.reload-swiss.com/en/loading-data/rs-loading-data |
| Vihtavuori reloading database | `vv_cache/vv_data.json` | vihtavuori.com/reloading-data/reloading-data-tool/ |
| ADI Powders rifle and pistol data | `adi_rifle_pages/`, `adi_pistol_pages/` | adiworldclass.com.au/data-rifle/, /data-pistol/ |
| Vectan catalogue (Nobel Sport) | `pdf/Vectan-Reloading-catalogue_vUS_compressed.pdf` | shootersworldpowder.com/wp-content/uploads/2025/05/Vectan-Reloading-catalogue_vUS_compressed.pdf (user) |
| Hodgdon Reloading Data Center, 12 gauge 2 3/4" | `pdf/Hodgdon_RLDC_12ga_2.75_export.pdf` | export of the Reloading Data Center (user) |

Sample counts (2026-10-04): Hodgdon 14,664 (122 powders, 10,563 with pressure), Western 5,526 (29, 2,770), Lovex 1,056 (11, 528),
Reload Swiss 2,002 (14, 2,002), Vihtavuori 16 powders (no pressures), ADI 9,636 (rifle 7,296 + pistol 2,340).

## Shotshells (re-made 2026-10-05)

`hodgdon_shot_extract.py` (Hodgdon manual, 2,616 loads), `alliant_shot_extract.py` (Alliant Reloader's Guide 2022, 1,993 loads, downloaded from
alliantpowder.com/resources/catalog/AP398_2022AlliantPowderReloadersGuide_NoCrops.pdf) and `vectan_shot_extract.py` (Vectan catalogue, 450 loads, 12 /
16 / 20 gauge, nine powders) give the shot samples (`sources/samples/*_shot_samples.csv`). `analysis/calibrate_shotshell.py` fits the shot constants and
the burn rate of every shotgun powder to them. The refit with the shot constants free gave no better result (velocity 3.1 % instead of 2.9 %,
pressure 15.0 % instead of 15.6 %), so the constants stayed and only the burn rates were refitted (Hodgdon's pressure bias -7.9 % -> -3.0 %).
`analysis/fit_shot_factors.py` fits the factors of the shell (hull), wad and primer named in the tables (`shell`, `wad`, `primer` columns; the shell is the
table heading) in 4 seconds: the sensitivities of every load to the two shot factors are measured once, the factors then follow from a ridge-regularised linear
least squares and are checked with true simulations on held-out series (the load names its parts in the caliber entries `shothull`, `shotwad`, `shotprimer`).
The remaining error is that of the tables: the pressure depends on the wad (-27 % DRV-12 ... +11 % CB 6118 against the model's average) and the primer;
with a correction per wad and primer it would still be 12 %. A capacity factor per gauge would remove the 20 gauge bias (-12 %) but the 16 gauge (+13 %)
and 3.5" (-15 %) loads, which are few, point the other way: no term was added.

## Speed

The fit runs thousands of simulations. The solver uses a secant method (Illinois) instead of 70 bisections per step
(2.6 ms instead of 17.5 ms per simulation, identical results) and `powderfit::fit` runs the samples on all cores
(`Options::threads`).

## Recalibrating the solver on published pressures (experiment, not adopted)

`tools/grt_eval` simulates a samples CSV with the project's propellant records and any solver constants (`--set name=value`, `--ba "Maker Name=value"`);
`analysis/calibrate_published.py` uses it to fit shared solver constants and the burn rates of a few powders jointly (6 powders with the most samples:
Accurate 4350, Ramshot Hunter, Accurate 2700, Ramshot Magnum, Ramshot Big Game, Hodgdon H4350; 2169 loads, 3/4 of the cartridges) and compares
them with the current constants (burn rates refitted): on the held-out cartridges (788 loads) and on other powders (IMR 4350, Varget, H4831,
W760, IMR 4064, Accurate 2520, IMR 4831, H4895; Ba fitted on the training cartridges, 833 held-out loads). `grt_compare` shows the effect on the
accuracy against the original program (349 runs; current: mean |pmax| 2.81 %, |vmuz| 0.83 %).

| Constants | Pressure error, trained powders / held-out cartridges / other powders | Velocity error (same) | grt_compare mean pmax / vmuz |
|---|---|---|---|
| current | 17.5 / 18.2 / 15.3 % | 4.0 / 4.7 / 5.0 % | 2.8 / 0.8 % |
| 10 constants free | 7.2 / 8.9 / 8.7 % | 3.4 / 3.4 / 3.1 % | 40 / 5.5 % |
| 10 constants within +-30 % | 12.5 / 14.0 / 11.2 % | 3.5 / 4.1 / 3.9 % | 29 / 9.7 % |
| impetus, capacity, burn exponent only | 14.2 / 16.1 / 12.9 % | 4.1 / 4.9 / 4.9 % | 46 / 16 % |

The published pressures are fitted much better (and it generalises to other cartridges and powders), but with constants that are unphysical
(negative density factors, kinetic factor 1.9, capacity factor 0.26) and that no longer reproduce the original program. The original's model
differs from measured pressures by about 17 % rms on these loads; the solver follows the original, not the measurements.

### The "Published load data" profile (Settings > Solver)

A second fit keeps the records' burn rates (as a user's load would use them, no refit per powder) and fits `burnRateScale` and eleven constants
within limits (22 powders: Accurate 2230 / 2460 / 2520 / 2700 / 4064 / 4350 / Magpro, Ramshot Big Game / Hunter / Magnum / TAC, Hodgdon H1000 / H4350 /
H4831 / H4895 / Hybrid 100V / Varget, IMR 3031 / 4350 / 4895 / 7828, W760; 5449 loads). Tested on the held-out cartridges and on 9 other powders
(Accurate 2015, LT-30, Hodgdon H335, Retumbo, IMR 4064, 4831, 8208 XBR, StaBALL Match, Ramshot X-Terminator) with the records' burn rates:

| Constants | Pressure error (trained powders, held-out cartridges, other powders) | Velocity error (same) | grt_compare mean pmax / vmuz |
|---|---|---|---|
| original | 17.2 / 15.9 / 15.9 % | 4.0 / 4.5 / 4.6 % | 2.8 / 0.8 % |
| published (in the program) | 9.4 / 10.1 / 10.2 % | 3.4 / 3.5 / 3.4 % | 27 / 4.8 % |

Several constants sit at their limits (kinetic factor 1.5, intrusion 0.3, initial gas 1.2) and the covolume and solid-volume terms go to zero: the
profile is empirical. `calibrationFor(CalibrationProfile::Published)` in `src/core/solver.cpp` holds the values.

### Powders with less data

`published_fit.py --skip-existing --min-samples 9 --min-cartridges 3 --max-rms 7` looks at the powders that have no record yet. Written: Lovex S040 (9 loads),
S053 (14), D063 (20; 7.1 %), Accurate 4100, Ramshot Enforcer, Hodgdon H110 and Winchester W296 (125-190 loads, but 6.2-6.9 % velocity error and 25 %
pressure error; H110 / W296 are magnum handgun powders). Their descriptions say "use with more caution". Not written: the pistol and shotgun powders
(Accurate Nitro 100, No. 2 / 5 / 7 / 9, Hodgdon Titegroup, Universal, Clays, Longshot, HS-6, W231, W244, Trail Boss, ... 8-19 %), Reload Swiss RS12-RS40 and
RS80 (7-22 %), Hodgdon LVR (11 loads, 2 cartridges), CFE BLK (10.6 %), H4227 / IMR 4227 (8.7 / 8.9 %). `more_*_fits.csv` have the numbers.

### Shotgun loads

`hodgdon_shot_extract.py manual.pdf` reads the shotshell tables of the Hodgdon Reloading Manual 2024 (pages 173-194: 12 gauge 2 3/4", 1 and 1 1/8 oz lead
shot, Federal, Remington, Winchester, Cheddite and Fiocchi hulls, many wads and primers; charge, velocity, pressure in PSI): 3,448 loads of 10 powders
(Clays, Titewad, International, 700-X, Universal, Longshot, Titegroup, Super Handicap, WST, Competition). `grt_eval` simulates rows with a `shot_g`
column in shotshell mode (the CIP dimensions of the hull by gauge and length, shot column + 25 mm wad and 2.5 g wad as the projectile, 28" barrel).
`analysis/calibrate_shotshell.py` fits the burn rate of each powder (and, in a second step, the three shotshell constants), holding out the loads with the
Winchester 209 primer:

| Constants | Training: velocity / pressure | Held-out primer: velocity / pressure |
|---|---|---|
| current shotshell constants, Ba fitted | 3.2 / 14.6 % | 3.1 / 15.2 % |
| constants fitted too (capacity 1.94, friction 0.012) | 3.0 / 14.5 % | 3.0 / 15.1 % |

The constants hardly move, so they stay (the earlier fit to 64 loads was good); what was missing were records of the shotgun powders (rifle and handgun records
gave 30 % and more). The remaining pressure error is the scatter of the data: the same charge gives different pressure with another wad or primer. The records
(lot "shotshell") are only valid in shotshell mode.

`alliant_shot_extract.py guide.pdf` reads the shotshell tables of Alliant's Reloader's Guide 2022 (lead shot only; layouts: shot in ounces with one cell
"19.0 grs 7800 psi" per velocity level, and the international loads in grams with a (grains, psi) pair per powder; the cell belongs to the velocity column whose
heading starts nearest to it - in 866 pairs of neighbouring velocity levels the charge falls in 33, so the assignment is right): 1,993 loads of 14 powders (12, 16,
20 and 28 gauge, .410). Together with Hodgdon's, 5,198 loads give (current shotshell constants, Ba fitted):

| Loads | n | Velocity (bias) | Pressure (bias) |
|---|---|---|---|
| 12 gauge 2 3/4" | 5,039 | 3.1 % (-1.1) | 13.8 % (-0.6) |
| 12 gauge 3" | 36 | 3.0 % (-0.5) | 14.6 % (+2.2) |
| 12 gauge 3 1/2" | 31 | 5.3 % (-4.8) | 16.6 % (-13.4) |
| 20 gauge | 79 | 2.1 % (-0.3) | 18.7 % (-16.0) |
| 16 gauge | 13 | 5.1 % (+4.8) | 27.9 % (+25.7) |

Records for Alliant Red Dot, Green Dot, Blue Dot, Unique, Herco, Promo, American Select, Clay Dot, Pro Reach and 20/28. Not written: e3 (its burn rate runs to the
limit of the fit), 410, 2400 and Power Pro 300-MP (1-16 loads, and the .410 and the 2 3/4" 20 gauge hull data do not constrain them). The model's powder space and
friction constants are the same for every gauge; the gauge-dependent pressure bias of the 20 and 16 gauge loads suggests they would need their own factor, but 13-79
loads are too few to fit one.
