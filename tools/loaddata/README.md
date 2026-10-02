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

## All Vihtavuori powders

`vv_fit_all.py` fits Ba of all 29 powders (`vv_fits.csv`: samples, rejected samples, Ba, error before and after). Qex, k and the
densities are generic (single-base 3800 kJ/kg or double-base 4300 kJ/kg, the better fit is kept). Samples whose charge does
not fit the case at the assumed bulk density are left out (0-5 per powder).

| Group | Ba against the existing database (mean ratio, range) | Velocity error (rms) |
|---|---|---|
| N130 - N170 (rifle, fast to slow) | 1.02 (0.93 - 1.13) | 4.2 % |
| N530 - N570 (progressive rifle) | 1.13 (0.99 - 1.35) | 4.2 % |
| pistol, fast and double-base (20N29, 24N41, 3N37/38, N105 - N120, N310 - N350) | 0.88 (0.54 - 1.29) | 9.5 % (N310: 24 %) |

For the rifle powders the published data alone reproduces the burn rate within a few percent. For pistol powders the model
(low pressures, assumed Qex and form function) is the limit: do not use these Ba values without a measurement.

## Speed

The fit runs thousands of simulations. The solver uses a secant method (Illinois) instead of 70 bisections per step
(2.6 ms instead of 17.5 ms per simulation, identical results) and `powderfit::fit` runs the samples on all cores
(`Options::threads`).
