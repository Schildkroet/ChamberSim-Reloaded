# Accuracy

The solver has two main models (File > Settings > Solver, see the Solver model chapter of the README). The default **Physical Model** is fitted to published
load data of powder manufacturers; **GRT Original** is calibrated against reference runs of the original program. Neither
is the algorithm of any other program.

## Physical Model against published loads

Simulated with the project's powder records and compared with the manufacturers' tables (velocity, and measured peak pressure where
published). Every powder's burn values were fitted on 3/4 of the cartridges, the figures are for the **cartridges that were left out**
(every 4th cartridge). Before = the constants until 2026-10-04, after = the current ones (the same fit on the same data, 106 powders,
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
| Other cartridges (caliber sweep) | 28 | 30.2 % | 50.8 % | 7.7 % | 12.9 % | 24.6 % |
| Powders (40 powders, one cartridge) | 39 | 22.9 % | 49.8 % | 4.9 % | 11.6 % | 18.5 % |
| Charge, bullet weight, start pressure, case volume | 24 | 34.7 % | 83.1 % | 5.8 % | 9.1 % | 21.9 % |
| Barrel length, twist, Sebert factor | 10 | 35.9 % | 49.5 % | 7.2 % | 14.5 % | 25.2 % |
| Powder temperature | 26 | 34.5 % | 37.5 % | 6.1 % | 7.0 % | 21.2 % |
| Bullet friction | 11 | 44.4 % | 69.6 % | 6.0 % | 14.5 % | 28.4 % |
| Almost closed vessel (very heavy bullet) | 9 | 4.8 % | 16.4 % | 6.5 % | 15.2 % | 11.2 % |
| Fast powders (burn rate, step length, form function) | 21 | 9.5 % | 43.8 % | 1.2 % | 5.7 % | 10.9 % |
| Fast-powder probes on the 8x57 (Ba 1..8, form function, start pressure, gas, charge) | 32 | 9.9 % | 45.1 % | 1.4 % | 5.6 % | 9.9 % |
| Powder probes on a 9 mm Luger (Ba 1..32, form function, start pressure, gas, charge) | 34 | 17.1 % | 65.8 % | 3.4 % | 10.7 % | 10.7 % |
| Pistol and revolver loads with fast powders | 28 | 15.5 % | 38.3 % | 3.8 % | 9.7 % | 11.9 % |
| Slow powders at low pressure (9 mm Luger: bullet mass, barrel, charge, start pressure, ...) | 34 | 38.0 % | 73.1 % | 1.2 % | 8.7 % | 28.5 % |
| Bullet jump (0.25 ... 4 mm, 8x57 and 9 mm) | 18 | 19.4 % | 37.3 % | 4.8 % | 6.3 % | 13.4 % |
| Powder and start-pressure variations of the 8x57 (shotshell batch, flag off) | 12 | 29.5 % | 59.0 % | 4.4 % | 8.5 % | 19.7 % |
| Loading density (charge, case volume, powder density, Qex) | 22 | 31.6 % | 93.5 % | 4.8 % | 10.8 % | 20.3 % |
| Other | 1 | 23.7 % | 23.7 % | 3.1 % | 3.1 % | 21.1 % |
| **All runs** | 349 | 24.7 % | 93.5 % | 4.3 % | 15.2 % | 18.1 % |

_Absolute deviations against 349 runs of the original program; updated 2026-10-08 with `tools/update_accuracy.py`. "Curve shape" is the rms relative pressure error along the barrel (where the pressure is above 5 % of its maximum). The charge-too-high rule matches the original for all tested loads._
<!-- accuracy:end -->

Unusual loads, other powders and other cartridges than the ones tested can deviate more. The known weak spot is
slow powders at low pressure (below about 150 bar, for example a slow powder with a small charge in a pistol case):
the original burns more slowly there than this model, and the curve shape is off. Loads with a very early second
transition (z2 below about 0.5) and start pressures above about 400 bar are also not covered well.

**Shotgun loads** (caliber with *Shotshell* = yes) use their own model terms: the wad and the shot column are the
projectile, the plastic wad seals the bore, and the powder space and the friction of wad and shot are fitted to 64
published shotgun loads, not to the original program: the 50 loads of the Vihtavuori Reloading Guide (12/76, lead and
steel shot), two Reload Swiss loads (12/70, RS12 / RS20) and 12 loads of the Hodgdon Reloading Data Center (12/70,
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
