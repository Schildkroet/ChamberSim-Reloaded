# Accuracy

The solver has two main models (File > Settings > Solver, see the Solver model chapter of the README). The default **Physical Model** is fitted to published
load data of powder manufacturers; **GRT Original** is calibrated against reference runs of the original program. Neither
is the algorithm of any other program.

## Physical Model against published loads

Simulated with the project's powder records and compared with the manufacturers' tables (velocity, and measured peak pressure where
published). Every powder's burn values were fitted on 3/4 of the cartridges, the figures are for the **cartridges that were left out**
(every 4th cartridge).

*2026-10-17, current equations:* the physical model now uses the classical Lagrange gas-mass share (1/4), burns at the breech pressure,
corrects the seated ogive volume and no longer counts the free run twice in the gas volume. The 193 powders' Ba were refitted with the
form functions of the previous fit. Held-out cartridges (173 powders, 9,434 simulated loads, 6,822 with a measured pressure): velocity
**4.85 % rms**, peak pressure **16.51 % rms**, velocity gain from start to maximum load **0.957** of the published. Split by class:
rifle **4.56 % / 16.30 %**, pistol and revolver **5.87 % / 17.12 %** (velocity / peak pressure rms); shotgun loads use their own
model and are reported separately. The table below is the 2026-10-04 fit with the previous equations (it is kept for comparison); a
per-maker split of the current equations is still pending.

*2026-10-17, refit without outliers:* 369 published loads whose simulated pressure was more than 45 % off the published one (342 too high,
27 too low; probably misread or invalid, `analysis/filter_published_outliers.py`) were left out and Ba of the 173 fitted powders was refitted with
the form functions kept (`analysis/fit_physical_powders.py --fit ba --keep-form --limit-pressure --limit-margin 50`). Shipped records against
the cleaned published loads (the fit used all of them, so the figures are not held out; velocity / peak pressure rms): rifle **4.00 % / 13.85 %**,
pistol and revolver **7.16 % / 16.77 %**, velocity gain from start to maximum load **0.974**. On the cartridges that were left out in the earlier
fit (every 4th): rifle 4.22 % / 15.59 %, pistol 5.17 % / 17.18 %; before the refit 4.29 % / 15.76 % and 5.31 % / 17.34 %
(`analysis/compare_physical_published.py`).

Before = the constants until 2026-10-04, after = the constants of that date (the same fit on the same data, 106 powders,
about 42,000 loads of Hodgdon, Western, Lovex, Reload Swiss, Vihtavuori, ADI and Vectan):

| Published data | Held-out loads | Velocity: before → after (rms) | Peak pressure: before → after (rms) |
|---|---:|---:|---:|
| All powders with published pressures (84) | 6,939 | 5.47 % → 4.83 % | 14.8 % → 14.6 % |
| All powders without pressures (20: Vihtavuori, Vectan) | 3,119 | 3.17 % → 2.90 % | - |
| Hodgdon (IMR, Winchester, ...) | 2,022 | 6.19 % → 5.91 % | 15.3 % → 14.8 % |
| Western (Accurate, Ramshot) | 630 | 4.10 % → 3.11 % | 10.5 % → 10.8 % |
| ADI Powders (rifle and pistol) | 1,660 | 3.94 % → 3.33 % | 9.6 % → 10.8 % |
| Lovex | 104 | 2.80 % → 2.41 % | 12.3 % → 12.3 % |
| Reload Swiss | 152 | 5.77 % → 4.63 % | 15.5 % → 16.9 % |
| Vihtavuori (velocities only) | 1,372 | 3.16 % → 2.96 % | - |
| Shotgun loads (Hodgdon, Alliant, Vectan; 4,883 loads, original's constants) | | 2.9 % | 15.6 % |

The error no longer depends on the charge and the bullet: the velocity of start loads was 2.3 % too high and of maximum loads 1.7 % too low,
now +1.3 % and -0.9 %; the pressure bias of start and maximum loads fell from +4.0 / -2.3 % to +1.0 / -0.6 %; the trend of the velocity
error with the bullet mass (heavy bullets too fast) is gone. The remaining scatter (3-4 % in velocity) is that of the published tables
(lots, barrels, components) and cannot be removed by the model.

Single loads can deviate much more. Details of the method: `scripts/data/loaddata/README.md`.

*2026-10-08:* published velocity rises with the charge (start to maximum load) with an elasticity of 0.91 (19,000 load pairs of all
makers), the model had 0.66. Changing the ignition time (0.3 -> 0.1 ms), the burn-law pressure exponent (-0.15 -> -0.05) and the engraving
share (1.0 -> 0.6) and refitting all burn values lifts it to about 0.8: on the 41,000 published loads the velocity rms falls from 5.8 % to
5.4 %, the bias of start and maximum loads from +1.1 / -1.5 % to +0.4 / -0.7 %; the pressure rms rises from 16.7 % to 17.2 % (figures on
the fitted data, not held out; the table above is from before this change). Vectan and Lovex loads got slightly worse.

*2026-10-11:* the Physical Model's sliding friction now scales with the bullet's oversize over the groove diameter
(`slideInterference` 1.5: +1.5 % per percent of oversize; median oversize 0.2 %, 95 % of the bullets below 1 %). On the held-out cartridges
of 14,000 loads (burn rate refitted per powder) the velocity rms fell from 5.37 % to 5.26 % and the pressure rms from 18.01 % to 17.74 %.
After refitting all 157 powders' burn values on everything the gain is mixed: powders with pressures 5.66 % -> 5.42 % in velocity and
17.11 % -> 17.37 % in pressure, powders with velocities only 3.40 % -> 3.53 % (figures on the fitted data). The table above is from before
this change. Scaling the engraving resistance the same way made the fit worse and is not used (`engravingInterference` 0).

*2026-10-11, per-caliber burn rate for pistol and revolver cartridges:* the published loads of auto-loading cartridges (9 mm Luger, .40 S&W,
.45 Auto, .38 Super, 10 mm, 357 SIG) came out 4-7 % too slow and those of revolver cartridges 1-4 % too fast with one burn rate per powder.
`data/physical/calibers.json` (`analysis/fit_caliber_scales.py`) gives 15 calibers with at least 100 published loads their own factor on the
physical burn rate (0.93 - 1.21, 25 % of the fitted deviation is dropped as the factors of half the powders do not transfer fully); other
calibers keep 1. Fitted on half of the powders and tested on the other half: pistol velocity rms 7.95 % -> 7.28 %. Over all published loads
(fitted data, powders as before), split by class (velocity rms / bias, pressure rms / bias):

| Class | Loads | Before | After |
|---|---:|---|---|
| Rifle | 23,574 | v 3.97 % / -0.5 %, p 14.7 % / -2.8 % | unchanged |
| Pistol and revolver | 5,183 | v 8.54 % / -1.1 %, p 20.0 % / +1.9 % | v 7.75 % / -0.4 %, p 18.9 % / +3.4 % |
| Shotgun (own model, not affected) | 4,875 | v 6.2 % / -1.0 %, p 35.8 % / -15.8 % | unchanged |

What did not explain the pistol error (all tested with the burn rates refitted): start pressure (also per auto / revolver class), engraving,
sliding stress (also lower for lead bullets), ignition time, heat transfer, primer energy, gas inertia, the burn-law exponent, a gas leak at the
revolver's cylinder gap, a shift of the revolver barrel length and the bullet length estimates. The published tables themselves scatter by
5.9 % rms around a smooth trend of velocity over bullet mass and charge; 62 pistol and 38 rifle loads that are more than 15 % off their own
table (mostly solid-copper Lehigh XD and CEB bullets) are left out of the calibrations (`analysis/sample_filter.py`, `--drop-inconsistent`).

*Bullet lengths of lead bullets (2026-10-11):* the length of a bullet fixes how deep it sits in the case and so the powder space (+1.5 mm = +1.9 % in
velocity, +16 % in peak pressure of a 9 mm Luger). The estimator assumed an effective density (mass / cross-section x length) of 10.3 g/cm3 for lead
bullets; measured on the product photos of hn-sport.de (the single standing bullet, camera elevation 10 degrees, scale fixed with a caliper
measurement of one bullet: TC .458 300 gr HS, exactly 20.0 mm) it is 8.1 - 9.0 g/cm3 by shape (round nose 8.5, semi-wadcutter 8.1, flat / truncated cone 9.0,
hollow point 8.4, wadcutter 8.7). Lead bullets were 15 % too short, the earlier measurements of the angled LOS photos 14 %. 63 H&N records are now
measured, the other estimated lead bullets use the new densities (`scripts/data/bulletdata/estimate_length.py`). Jacketed handgun bullets were already
right: 26 side views of hunting-sport.com give 8.8 +- 0.7 g/cm3, 1.5 % from the estimate. Effect on the 740 published loads with estimated lead bullets (burn rates
fixed): peak pressure bias -8.5 % -> +10 %, velocity unchanged (-4.5 %), and 77 loads no longer fit the case. With the right lengths the solver now
overshoots the pressure of deeply seated bullets: it counts everything seated in the case as a full cylinder, although the narrower nose of the bullet
is inside it (not corrected yet). The velocity shortfall of lead bullets (-4.5 %) is not a length effect.

## Against the original program's runs

<!-- accuracy:start -->
**GRT Original** (the model that reproduces the original program):

| Test set | Runs | Peak pressure: mean error | max | Muzzle velocity: mean error | max | Curve shape: mean deviation |
|---|---:|---:|---:|---:|---:|---:|
| Other cartridges (caliber sweep) | 28 | 1.3 % | 4.0 % | 1.2 % | 10.9 % | 4.5 % |
| Powders (40 powders, one cartridge) | 39 | 2.0 % | 6.4 % | 0.9 % | 6.3 % | 4.0 % |
| Charge, bullet weight, start pressure, case volume | 24 | 1.6 % | 4.4 % | 0.7 % | 3.4 % | 3.4 % |
| Barrel length, twist, Sebert factor | 10 | 1.0 % | 2.8 % | 0.4 % | 1.8 % | 2.6 % |
| Powder temperature | 26 | 0.5 % | 1.0 % | 0.3 % | 0.4 % | 2.3 % |
| Bullet friction | 11 | 1.2 % | 3.9 % | 0.8 % | 2.4 % | 2.8 % |
| Almost closed vessel (very heavy bullet) | 9 | 2.8 % | 7.2 % | 1.1 % | 2.4 % | 3.6 % |
| Fast powders (burn rate, step length, form function) | 21 | 1.4 % | 5.2 % | 0.6 % | 0.9 % | 3.6 % |
| Fast-powder probes on the 8x57 (Ba 1..8, form function, start pressure, gas, charge) | 32 | 2.9 % | 30.7 % | 0.8 % | 3.9 % | 3.4 % |
| Powder probes on a 9 mm Luger (Ba 1..32, form function, start pressure, gas, charge) | 34 | 5.4 % | 23.5 % | 1.2 % | 8.0 % | 7.7 % |
| Pistol and revolver loads with fast powders | 28 | 3.0 % | 8.1 % | 0.9 % | 2.8 % | 4.1 % |
| Slow powders at low pressure (9 mm Luger: bullet mass, barrel, charge, start pressure, ...) | 34 | 6.3 % | 12.6 % | 0.8 % | 10.3 % | 31.0 % |
| Bullet jump (0.25 ... 4 mm, 8x57 and 9 mm) | 18 | 2.8 % | 8.0 % | 0.9 % | 2.0 % | 3.0 % |
| Powder and start-pressure variations of the 8x57 (shotshell batch, flag off) | 12 | 2.4 % | 7.0 % | 0.7 % | 2.2 % | 5.5 % |
| Loading density (charge, case volume, powder density, Qex) | 22 | 3.4 % | 13.9 % | 0.7 % | 3.4 % | 4.8 % |
| Other | 1 | 9.2 % | 9.2 % | 0.7 % | 0.7 % | 6.6 % |
| **All runs** | 349 | 2.8 % | 30.7 % | 0.8 % | 10.9 % | 6.8 % |

**Physical Model** (the default), for comparison: how far its results are from the same runs of the original. The Physical Model is not fitted to the original program but to published load data, so these deviations are differences between two models, not errors of the Physical Model (its accuracy against published loads is given below):

| Test set | Runs | Peak pressure: mean error | max | Muzzle velocity: mean error | max | Curve shape: mean deviation |
|---|---:|---:|---:|---:|---:|---:|
| Other cartridges (caliber sweep) | 28 | 41.3 % | 88.3 % | 8.6 % | 18.0 % | 28.2 % |
| Powders (40 powders, one cartridge) | 39 | 28.8 % | 62.8 % | 6.2 % | 16.1 % | 20.5 % |
| Charge, bullet weight, start pressure, case volume | 24 | 43.0 % | 88.5 % | 6.4 % | 13.1 % | 23.8 % |
| Barrel length, twist, Sebert factor | 10 | 44.4 % | 58.8 % | 8.1 % | 18.3 % | 27.9 % |
| Powder temperature | 26 | 42.8 % | 46.6 % | 6.4 % | 7.7 % | 22.8 % |
| Bullet friction | 11 | 51.8 % | 74.3 % | 6.4 % | 16.1 % | 30.2 % |
| Almost closed vessel (very heavy bullet) | 9 | 4.8 % | 16.3 % | 6.5 % | 15.2 % | 11.3 % |
| Fast powders (burn rate, step length, form function) | 21 | 9.0 % | 49.3 % | 1.4 % | 7.1 % | 9.6 % |
| Fast-powder probes on the 8x57 (Ba 1..8, form function, start pressure, gas, charge) | 32 | 8.2 % | 42.9 % | 1.7 % | 7.0 % | 7.8 % |
| Powder probes on a 9 mm Luger (Ba 1..32, form function, start pressure, gas, charge) | 34 | 18.6 % | 75.4 % | 3.9 % | 13.3 % | 11.6 % |
| Pistol and revolver loads with fast powders | 28 | 16.6 % | 42.8 % | 4.1 % | 9.9 % | 12.9 % |
| Slow powders at low pressure (9 mm Luger: bullet mass, barrel, charge, start pressure, ...) | 33 | 44.5 % | 90.0 % | 1.3 % | 9.8 % | 29.1 % |
| Bullet jump (0.25 ... 4 mm, 8x57 and 9 mm) | 18 | 26.4 % | 50.6 % | 5.0 % | 7.9 % | 15.3 % |
| Powder and start-pressure variations of the 8x57 (shotshell batch, flag off) | 12 | 35.8 % | 66.3 % | 4.8 % | 9.4 % | 21.6 % |
| Loading density (charge, case volume, powder density, Qex) | 22 | 35.8 % | 96.1 % | 5.1 % | 11.1 % | 20.7 % |
| Other | 1 | 13.3 % | 13.3 % | 4.7 % | 4.7 % | 17.7 % |
| **All runs** | 348 | 29.3 % | 96.1 % | 4.8 % | 18.3 % | 19.1 % |

_Absolute deviations against 349 runs of the original program; updated 2026-10-17 with `tools/update_accuracy.py`. "Curve shape" is the rms relative pressure error along the barrel (where the pressure is above 5 % of its maximum). The charge-too-high rule matches the original for all tested loads._
<!-- accuracy:end -->

Unusual loads, other powders and other cartridges than the ones tested can deviate more. The known weak spot is
slow powders at low pressure (below about 150 bar, for example a slow powder with a small charge in a pistol case):
the original burns more slowly there than this model, and the curve shape is off. Loads with a very early second
transition (z2 below about 0.5) and start pressures above about 400 bar are also not covered well.

**Shotgun loads** (caliber with *Shotshell* = yes) use their own model terms: the wad and the shot column are the
projectile, the plastic wad seals the bore, and the powder space and the friction of wad and shot are fitted to 64
published shotgun loads, not to the original program: the 50 loads of the Vihtavuori Reloading Guide (12/76, lead and
steel shot; four maximum loads of this older edition are lower in the 2020 tables, see `scripts/data/loaddata/README.md`), two Reload Swiss loads (12/70, RS12 / RS20) and 12 loads of the Hodgdon Reloading Data Center (12/70,
Universal and W244); the last 14 with measured pressure. The published velocities are measured 1-3 m in front of the
muzzle and converted to the muzzle with the air drag of the pellets. Result: muzzle velocity typically 5-7 % low (up
to 15 %); pressure right on average, but **up to 30 % too high or too low depending on the powder** (the powders'
models were made for rifles and handguns; Universal and RS12 come out high, W244 and Nitro 100 low). With the project's records of shotgun powders
(lot "shotshell", their burn rates fitted to 5,059 loads of the Hodgdon Reloading Manual, the Alliant Reloader's Guide and the Vectan catalogue: 12, 16 and 20 gauge, 21 - 42 g of lead shot) the error against these loads is
2.9 % rms in velocity and 15.6 % in pressure over 4,883 loads (Hodgdon 2.9 % / 17.6 %, Alliant 2.8 % / 13.5 %, Vectan 2.9 % / 11.8 %). Even a model that knew the wad and the primer of every load
would still be 12 % off in pressure: it varies with them, which the model does not know. Almost all of these loads are 12 gauge 2 3/4"; the few 20 gauge loads come out 16 % low in pressure, the 16 gauge ones 26 % high. For comparison,
the original program with the same 50 Vihtavuori loads: velocity +3 % on average (typically 4.8 % off) - closer - but
pressures of the maximum loads up to 1800 bar, where the guide's loads are pressure-tested within the CIP limit of
1050 bar (its shotshell setting changes little). Always check a shotgun load against the powder maker's data. Enter a shotgun load like this, the fit depends on it:

- *Projectile*: mass = shot + wad (about 2.5 g for a plastic wad), length = height of the shot column
  (shot weight / (shot density x 0.62) / bore area) + height of the wad, diameter = bore diameter;
- seated completely inside the hull: *Cartridge Overall Length* = hull length, *Seating depth* = projectile length;
- *Initial Gas Pressure* (start pressure) = crimp release, about 20 bar.

A shotshell load also has the choices **Shotshell Hull**, **Shotshell Wad** and **Shotshell Primer** (Cartridge tab, below *Shotshell*; "not specified" = the
model's average). They matter: at the same velocity the pressure of published loads varies with the shell brand and type by about +-20 % (Remington STS
and Winchester AA hulls give the highest pressures, Federal Gold Medal the lowest), with the wad by +-15 % and with the primer by +-10 %, and the model does not
know them. Every kind offers three **groups** of similar parts: low-, medium- and high-pressure shells, wads or primers; the tooltip of a group lists its individual parts
(7 shells, 29 wads, 8 primers) and the correction it applies. Every group has two factors on the model (powder space and gas energy; `data/shotshell/parts.json`,
fitted by `analysis/fit_shot_factors.py` to 5,000 published loads of Hodgdon, Alliant and Vectan). On loads that were left out of the fit, naming the three groups
reduces the pressure error from 16.1 % to 10.6 % (velocity 2.9 % -> 2.8 %); with all individual parts it would be 10.3 %, with the shell group alone 12.0 %.
Crimp type is not in the tables (Vectan's are all fold crimps); paper hulls only in Alliant's (Federal Gold Medal paper). A file that names an individual part is
moved to the group of that part when it is opened.
