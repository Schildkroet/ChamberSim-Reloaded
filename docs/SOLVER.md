# Solver models

- **GRT Original**: constants fitted to runs of Gordon's Reloading Tool. Results like the original (the accuracy
  table in [ACCURACY.md](ACCURACY.md) is for this choice).
- **GRT Tuned**: the original's equations, constants fitted to the loads and measured pressures of the Hodgdon Reloading Manual
  and the Western Powders Handloading Guide (22 powders, the burn rates of the records unchanged). Against the published tables of
  held-out cartridges and of other powders it is off by about 3.5 % in velocity and 10 % in peak pressure (the original's constants:
  4.5 % and 16 %), but its results differ from the original program (against its reference runs 27 % in peak pressure). Some of its
  constants are empirical without a physical meaning (covolume about 0, case capacity x 0.74).
- **Physical Model**, the default: the project's own model, not the original's. Integrated in time to convergence (the result does not depend on the
  number of curve points); the primer lights the powder, the burning spreads over the grains (flame spread, about 0.1 ms) and the bullet
  starts at the start pressure; covolume of the gas (x eta) and volume of the unburnt powder (1 / pc); engraving, friction of the bearing
  surface (bullet length; the sliding stress rises with the bullet's oversize over the groove diameter, +1.5 % per percent), the extra friction gfriction [bar] and the air ahead of the bullet act on the bullet; heat loss to the chamber
  and bore walls; the spin (twist) takes energy. The peak pressure no longer depends on the barrel length. Results show the heat loss and
  the work of the sliding friction (the engraving and the air act on the bullet but are not part of that value); a bullet that cannot leave
  the barrel is reported. The gas moves with a Lagrange share of the charge of 1/4 (the share for the space-averaged pressure of a linear gas gradient) and the powder burns at the breech pressure
  (the mean pressure plus the gas-column term), so the momentum, energy and reported pressure use one consistent gradient model. The energy balance uses the same 1/4 for
  the gas kinetic energy, where a linear velocity profile has exactly 1/3; splitting the two changes no fit result (published loads, Ba refitted: 0.01 points in velocity and pressure). The part
  of the ogive nose seated in the case is corrected with its real volume, and the free run is not counted twice in the gas volume. The primer releases its energy over a finite time (`primerTime_ms`, 0.05 ms) and the chamber walls are at the load's powder temperature
  (neutral against the published loads: 0.00-0.01 points).

  **The start resistance is a fitted term, not an engraving force** (2026-10-17). It is `engravingShare x start pressure x bore area`,
  decaying over the **bearing length**; the start pressure is the model's release threshold and is `0.3 x pMax` for handguns, so the term
  gives pistols 2-4 times the start resistance of rifles. That scaling is what the published loads are fitted with. Replacing it with a
  physical engraving (absolute 150-300 bar x bore area, decaying over 0.2-2 mm) or adding case-neck friction (stress x pi x D x the
  remaining seating depth) does not improve the fit: with the Ba of every powder refitted for each variant (all published loads, fit
  residuals, velocity / peak pressure rms):

  | start resistance | rifle | pistol | velocity gain |
  |---|---|---|---|
  | share x ps, bearing decay (**current**) | 3.99 / 13.76 % | 7.93 / 19.66 % | 0.976 |
  | share x ps, decay 2 mm | 4.02 / 13.98 % | 7.85 / 19.59 % | 0.988 |
  | share x ps, decay 5 mm | 4.01 / 13.89 % | 7.91 / 19.59 % | 0.983 |
  | absolute 150 bar, decay 0.5 mm | 3.98 / 13.94 % | 8.02 / 19.76 % | 0.997 |
  | absolute 300 bar, decay 0.2 mm | 3.93 / 13.53 % | 7.93 / 20.27 % | 0.975 |
  | absolute 300 bar, decay 2 mm | 3.94 / 13.29 % | 7.94 / 20.85 % | 0.957 |
  | share 0.9, bearing decay | 4.10 / 13.56 % | 8.18 / 20.36 % | 0.954 |

  Each variant buys rifle pressure with pistol pressure or velocity, or with the start-to-maximum velocity gain; no combination improves
  both classes with one setting for all cartridges. **Separate settings per class do** (2026-10-18): rifles `engravingShare` 1.2 decaying over
  `engravingLength_mm` 2, pistols and revolvers `engravingShareHandgun` 0.6 over the bearing length (`engravingLengthHandgun_mm` 0). With Ba
  refitted: rifle 3.98 / 13.34 %, pistol 7.88 / 19.26 %, velocity gain 0.957 (fit residuals; the previous constants 3.98 / 13.73 % and
  7.96 / 19.57 %, 0.974); rifle 1.2 x 250 bar equals the absolute 300 bar of the table, but keeps the dependence on the start pressure of the bullet.
  Pistol share 0.4: 3.94 / 13.28 % and 7.84 / 19.08 % in the fit, but worse pressure on the held-out pistol cartridges (+0.5 points); 0.9 and an
  absolute 400 bar are worse. The price is the agreement with the original program's reference runs (peak pressure mean 29 -> 40 %).
  The same held for the bearing friction (0.35 / 0.7 / 1.4 MPa: pistol 8.11 / 7.97 / 9.57 % velocity). The parts of the term
  stay adjustable (`engravingPressure_bar`, `engravingLength_mm`, `neckStress_MPa`; 0 = the fitted behaviour above), but the defaults are
  the fitted optimum. Also dropped: counting only the **burnt** share of the charge in the momentum equation - the whole charge moves with
  the bullet (gas *and* unburnt grains), only its phase changes, so the burnt fraction is not the moving mass; and with a growing mass the
  momentum equation would need the influx term `-alpha * dm/dt * v` as well.
  Its constants and the **physical burn values** of 193 project powders (Ba, a0, z1, z2 fitted to each powder's published loads with
  pressures (Hodgdon, Western, Lovex, ReloadSwiss, ADI; Vihtavuori, Vectan, RWS and Alliant publish none: their maximum loads count as the C.I.P. limit minus 50 bar;
  4 Alliant powders with too few loads have the median form function of the others) come from `analysis/calibrate_published.py --profile physical` and
  `analysis/fit_physical_powders.py` (file `data/physical/propellants.json`). A powder also carries an optional **effective grain geometry**
  (ball or single-perforated cylinder, fitted to its form function by `analysis/fit_grain_geometry.py`); where the geometric surface
  matches the fitted curve within 0.08 in phi units it replaces the cubic, otherwise the cubic (a0, z1, z2) stays. Currently 40 of
  the 193 powders use a geometric surface. A `.csrload` stores the values in its `<physical>` section (`physBa`, `physA0`, `physZ1`,
  `physZ2`, `physGrain`, `physGrainLd`, `physGrainHole`, `physGrainAmp`, `physRef` = the record values they belong to); they apply only
  while Ba, a0, z1, z2 of the load are unchanged, otherwise (other powders, edited values) the load's burn rate is scaled and the
  deviations are larger. A load saved before these values existed gets them when it is opened. A load saved with the physical values of
  an earlier solver version keeps them (they are part of the file); reselect the powder or use the powder fit to get the refitted values.
  The curves and the barrel time begin when the bullet moves (it waits at the start pressure until the gas overcomes the engraving and
  friction).
  **Pistol and revolver cartridges** (C.I.P. table IV, calibers with at least 100 published loads: 9 mm Luger, .40 S&W, 10 mm, .45 Auto, .38 Super, 357 SIG,
  .38 Special, .357 Magnum, .44 Magnum, .45 Colt, .454 Casull, .460 and .500 S&W, .480 Ruger, ...) get a factor of their own on the powder's physical burn rate
  (0.93 - 1.21, `data/physical/calibers.json`, `analysis/fit_caliber_scales.py`): auto-loading cartridges burn faster than the powder's value, revolver
  cartridges a little slower. Since 2026-10-19 rifle calibers with at least 60 published loads have such a factor too (71 of them, 0.92 - 1.21: .30 Super Carry, .327 Federal, 300 Norma, .33 Nosler and 338 Lapua burn about 5 - 10 % faster than the powder's value, 45-70, 9.3 x 62 and 257 Weatherby slower); other calibers use the powder's value. They remain the least accurate class: velocity about 7.8 %, pressure 19 % rms
  (rifle 4.0 % / 14.7 %).
  Accuracy against the published loads: see the table in [ACCURACY.md](ACCURACY.md). The 2026-10-17 refit (new pressure model, ogive
  intrusion, no double-counted jump, Ba refitted with the old form functions, held-out cartridges of 173 powders): velocity 4.85 % rms,
  pressure 16.51 % rms, velocity gain from start to maximum load 0.957 of the published; split by class rifle 4.56 % / 16.30 % and
  pistol/revolver 5.87 % / 17.12 % (velocity / pressure rms, shotgun separate). Powders without physical values (own records, edited
  values) use the load's burn rate with a scale (`burnRateScale`) and are less accurate.
  Shotshells always keep the original's constants.

Method and numbers: `scripts/data/loaddata/README.md`, `analysis/calibrate_published.py`, `analysis/fit_physical_powders.py`,
`analysis/fit_caliber_scales.py` (and the experiments that did not help in [ACCURACY.md](ACCURACY.md)).
