# Solver models

- **GRT Original**: constants fitted to runs of Gordon's Reloading Tool. Results like the original (the accuracy
  table in [ACCURACY.md](ACCURACY.md) is for this choice).
- **GRT Tuned**: the original's equations, constants fitted to the loads and measured pressures of the Hodgdon Reloading Manual
  and the Western Powders Handloading Guide (22 powders, the burn rates of the records unchanged). Against the published tables of
  held-out cartridges and of other powders it is off by about 3.5 % in velocity and 10 % in peak pressure (the original's constants:
  4.5 % and 16 %), but its results differ from the original program (against its reference runs 27 % in peak pressure). Some of its
  constants are empirical without a physical meaning (covolume about 0, case capacity x 0.74).
- **Physical Model**, the default: the project's own model, not the original's. Integrated in time to convergence (the result does not depend on the
  number of curve points); the primer lights the powder, the burning spreads over the grains (flame spread, about 0.3 ms) and the bullet
  starts at the start pressure; covolume of the gas (x eta) and volume of the unburnt powder (1 / pc); engraving, friction of the bearing
  surface (bullet length; the sliding stress rises with the bullet's oversize over the groove diameter, +1.5 % per percent), the extra friction gfriction [bar] and the air ahead of the bullet act on the bullet; heat loss to the chamber
  and bore walls; the spin (twist) takes energy. The peak pressure no longer depends on the barrel length. Results show the heat loss and
  the work of the sliding friction (the engraving and the air act on the bullet but are not part of that value); a bullet that cannot leave
  the barrel is reported.
  Its constants and the **physical burn values** of 193 project powders (Ba, a0, z1, z2 fitted to each powder's published loads with
  pressures (Hodgdon, Western, Lovex, ReloadSwiss, ADI; Vihtavuori, Vectan, RWS and Alliant publish none: their maximum loads count as the C.I.P. limit minus 50 bar;
  4 Alliant powders with too few loads have the median form function of the others) come from `analysis/calibrate_published.py --profile physical` and
  `analysis/fit_physical_powders.py` (file `data/physical/propellants.json`). A `.csrload` stores them in its `<physical>` section (`physBa`,
  `physA0`, `physZ1`, `physZ2`, `physRef` = the record values they belong to); they apply only while Ba, a0, z1, z2 of the load are
  unchanged, otherwise (other powders, edited values) the load's burn rate is scaled and the deviations are larger. A load saved
  before these values existed gets them when it is opened. The curves and the barrel time begin when the bullet moves (it waits at
  the start pressure until the gas overcomes the engraving and friction).
  **Pistol and revolver cartridges** (C.I.P. table IV, calibers with at least 100 published loads: 9 mm Luger, .40 S&W, 10 mm, .45 Auto, .38 Super, 357 SIG,
  .38 Special, .357 Magnum, .44 Magnum, .45 Colt, .454 Casull, .460 and .500 S&W, .480 Ruger, ...) get a factor of their own on the powder's physical burn rate
  (0.93 - 1.21, `data/physical/calibers.json`, `analysis/fit_caliber_scales.py`): auto-loading cartridges burn faster than the powder's value, revolver
  cartridges a little slower. Other calibers use the powder's value. They remain the least accurate class: velocity about 7.8 %, pressure 19 % rms
  (rifle 4.0 % / 14.7 %).
  Accuracy against the published loads: see the table in [ACCURACY.md](ACCURACY.md) (held-out cartridges: velocity 4.8 %, pressure 14.6 % rms for the
  powders with published pressures; 2.9 % in velocity for those without). Powders without physical values (own records, edited values)
  use the load's burn rate with a scale (`burnRateScale`) and are less accurate.
  Shotshells always keep the original's constants.

Method and numbers: `scripts/data/loaddata/README.md`, `analysis/calibrate_published.py`, `analysis/fit_physical_powders.py`,
`analysis/fit_caliber_scales.py` (and the experiments that did not help in [ACCURACY.md](ACCURACY.md)).
