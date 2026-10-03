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

## Speed

The fit runs thousands of simulations. The solver uses a secant method (Illinois) instead of 70 bisections per step
(2.6 ms instead of 17.5 ms per simulation, identical results) and `powderfit::fit` runs the samples on all cores
(`Options::threads`).
