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
  surface (bullet length; the sliding stress rises with the bullet's oversize over the groove diameter, +1.5 % per percent; a lead bullet without a jacket,
  bullet material "lead", slides with 0.45 x that stress in rifle cartridges, fitted on the Lyman Cast Bullet Handbook, pistol and revolver
  cartridges keep it), the extra friction gfriction [bar] and the air ahead of the bullet act on the bullet; heat loss to the chamber
  and bore walls; the spin (twist) takes energy. The peak pressure no longer depends on the barrel length. The velocity gain with the barrel length (exponent about 0.24 between 16 and 24 inches) is slightly steeper than measured cut-down series (0.21), see docs/ACCURACY.md. Results show the heat loss and
  the work of the sliding friction (the engraving and the air act on the bullet but are not part of that value); a bullet that cannot leave
  the barrel is reported. The gas moves with a share of the charge of 1/4 and the powder burns at the breech pressure
  (the mean pressure plus the gas-column term). Both are **fitted**: the Lagrange gradient (linear gas velocity, parabolic pressure) gives a share of 1/3 in
  the momentum and the energy equation, and a charge spread over the gas column burns at the mean pressure (see "Gas column, bearing surface and volume constants" below). The part
  of the ogive nose seated in the case is corrected with its real volume, and the free run is not counted twice in the gas volume. The primer releases its energy over a finite time (`primerTime_ms`, 0.05 ms) and the chamber walls are at the load's powder temperature
  (neutral against the published loads: 0.00-0.01 points).

  **The start resistance is a fitted term, not an engraving force** (2026-10-17). It is `engravingShare x start pressure x bore area`,
  decaying over the **bearing length**; the start pressure is the model's release threshold and is `0.3 x pMax` for handguns (changes of psadd and primer against the load's reference `psref` are added to it, `handgunStartGain` 1), so the term
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
  pressures (Hodgdon, Western, Lovex, ReloadSwiss, ADI; Vihtavuori, Vectan, RWS and Alliant publish none: their maximum loads count as the C.I.P. limit minus 50 bar, except the 18 Vihtavuori powders of `analysis/form_references.json`, which take the burn form of a powder with pressures (the limit rule gave 16 to 50 % too much pressure on measured loads);
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
  (0.85 - 1.15, `data/physical/calibers.json`, `analysis/fit_caliber_scales.py`; refitted 2026-11-18 with the joint refit per class): auto-loading cartridges burn faster than the powder's value, revolver
  cartridges a little slower. Since 2026-10-19 rifle calibers with at least 60 published loads have such a factor too (69 of them, 0.87 - 1.37: .30 Super Carry, .327 Federal, 300 Norma, .33 Nosler and 338 Lapua burn about 5 - 10 % faster than the powder's value, 45-70, 9.3 x 62 and 257 Weatherby slower); other calibers use the powder's value. They remain the least accurate class: velocity about 7.8 %, pressure 19 % rms
  (rifle 4.0 % / 14.7 %).
  Accuracy against the published loads: see the table in [ACCURACY.md](ACCURACY.md). The 2026-10-17 refit (new pressure model, ogive
  intrusion, no double-counted jump, Ba refitted with the old form functions, held-out cartridges of 173 powders): velocity 4.85 % rms,
  pressure 16.51 % rms, velocity gain from start to maximum load 0.957 of the published; split by class rifle 4.56 % / 16.30 % and
  pistol/revolver 5.87 % / 17.12 % (velocity / pressure rms, shotgun separate). Powders without physical values (own records, edited
  values) use the load's burn rate with a scale (`burnRateScale`) and are less accurate.
  Shotshells always keep the original's constants.

  **Gas column, bearing surface and volume constants** (2026-11-17). Four corrections towards the textbook model were tested on the published
  loads; only the nose length is used (see the next paragraph). They are constants of the calibration (`chambrage`, `burnAtBreech`, `noseFromMass`,
  besides `gasMassShare`, `capacityFactor`, `gasCovolume_cm3g`), the others off in the shipped set:
  - *Lagrange share 1/3, burning at the mean pressure* instead of 1/4 and the breech pressure.
  - *Chambrage*: in a case wider than the bore the gas moves slower; the moving share is `1/3 x (1 - c (V0/V)^3)`, `c = 1 - (A/Ac)^2`.
  - *Nose length from the bullet's mass* instead of one calibre (the bearing surface of a spitzer is about half of what one calibre gives).
  - *Physical volume constants*: case capacity x 1 (shipped 1.15) and covolume 1.0 cm3/g (shipped 0.8).

  Held-out cartridges, per-caliber burn factors off, velocity / peak pressure rms (each row adds to the one above):

  | variant | rifle, Ba refitted | pistol, Ba refitted | rifle, Ba + form refitted (1/3 of the loads) |
  |---|---|---|---|
  | shipped | 5.16 / 14.76 % | 7.07 / 18.40 % | 4.39 / 14.09 % |
  | share 1/3, mean-pressure burning | 5.22 / 14.90 % | 7.12 / 18.47 % | |
  | + chambrage | 5.50 / 15.88 % | 7.12 / 18.48 % | 4.84 / 16.74 % |
  | + nose from mass (sliding stress not refitted) | 5.35 / 16.10 % | 8.00 / 18.98 % | 4.92 / 17.02 % |
  | + capacity 1, covolume 1 | 5.33 / 20.66 % | 8.16 / 33.29 % | 5.26 / 20.33 % |

  With eight global constants refitted as well for each variant (`calibrate_published.py`, impetus, heat transfer, sliding stress, start
  resistance, burn exponent, primer energy; 10 training powders, their held-out cartridges): shipped equations rifle 4.63 / 12.44 %, pistol
  7.95 / 16.68 %; all corrections with the shipped volume constants 4.97 / 14.55 % and 7.69 / 16.83 %; with the physical volume constants
  5.02 / 13.26 % and 9.95 / 18.29 %. Every one of these refits runs constants into their limits (impetus 1.1, rifle start resistance 3 x ps,
  burn exponent -0.2 to -0.3), so none is adopted. The share 1/3 with mean-pressure burning is neutral; with it the pressure error gets a
  trend over the bullet mass (slope 0.05 to 0.09 instead of 0.02), which the fitted pair removes. The volume constants are not a matter of
  the powders: the published pressures need about 15 % more room than the case capacity, whatever the powders' burn rates and forms are.

  **Why chambrage does not fit** (analysis 2026-11-17). The term is the textbook one: with the gas velocity proportional to the volume behind
  it, the kinetic energy of the gas is `mc v^2 / 6 x (1 - c r^3)` and the contraction into the bore adds `1/2 rho v^2 c r^2` to the breech
  pressure (both re-derived for a chamber of area Ac and a bore of area A). The fitted share 1/4 is already about the chambrage average
  (1/3 x (1 - 0.7 x 0.5) = 0.22 at the travel of the peak pressure), so the term on top of it counts the effect twice. Its signature on the
  shipped equations (Ba refitted, caliber factors off, 18,000 rifle loads with pressure): straight cases +2.5 to +3 %, bottleneck cases with
  a charge of more than a third of the bullet's mass -4 to -7 % peak pressure; rifle held-out pressure 14.38 -> 15.86 % (with the share 1/3:
  16.23 %), a pressure trend over the bullet mass of +0.06 instead of 0.00. The published pressures want the opposite spread (the shipped
  model is +5 to +10 % high on straight cases with a heavy charge and low on most bottleneck groups). Without chambrage the case shape has no
  signal of its own: in a regression of the rifle pressure error on the bottleneck c = 1 - (A / Ac)^2 together with the charge / bullet mass,
  loading density, bore, expansion ratio and bullet mass / bore area, c adds nothing (residual 11.68 % with and without it); the raw trend
  over c (-1.2 % per standard deviation) comes from the overbore cartridges' large charges. What is left is a charge / bullet-mass error
  (-5 % per standard deviation) and a loading-density error (+6.6 %), which no gas-column term of this kind removes. Chambrage stays off.

  **Charge and loading density** (analysis 2026-11-17, rifle loads with pressure, Ba refitted per variant, caliber factors off). Two trends of the
  pressure error were examined.
  - *Charge within a table* (same cartridge, bullet and powder): the model's pressure rises as charge^2.27, the published as charge^2.05. That is a
    Hodgdon effect: per source the published exponent is Hodgdon manual 1.84 and Reloading Data Center 1.61, but ADI 2.29, Lyman 2.32 and Reload
    Swiss 2.54, which the model matches (2.31 / 2.29 / 2.21), and its velocity exponent matches all of them (model 0.894, published 0.898).
  - *Loading density between tables*: with the means of each powder in each source and of each cartridge removed, the pressure error rises by
    +0.5 per unit of ln(loading density) (9,768 tables; rms 8.9 -> 8.1 % when it is removed), in every source (Lee +0.46, Western +0.29, ADI +0.21,
    Lyman +0.15, Hodgdon +0.09 without the cartridge means), with bullet mass, seating depth, bullet length, start pressure and solid copper
    controlled, for bullets with fitted start values as well. It is stronger where the maximum load is at the pressure limit (+0.65) than well
    below it (+0.47), so part of it is the selection of the published charges.
  Every mechanism that moves the second trend also flattens the velocity gain from the start to the maximum load (published 0.898; form refitted
  where noted), because the loading density changes inside every table too:

  | variant | rifle v / p rms | pistol v / p rms | velocity ~ charge^ | between-table slope |
  |---|---|---|---|---|
  | shipped | 4.66 / 14.38 % | 8.28 / 16.39 % | 0.894 | +0.54 |
  | covolume 0.5 / 1.0 cm3/g | 4.54 / 14.08 %, 4.89 / 14.87 % | 8.23 / 16.50 %, 8.35 / 16.72 % | 0.809 / 0.954 | +0.52 / +0.56 |
  | burnExponent -0.15 | 4.48 / 13.52 % | 8.58 / 16.12 % | 0.794 | +0.46 |
  | ... with the form refitted (1/3 of the loads; shipped 4.41 / 12.72 %, 7.62 / 15.63 %) | 4.31 / 11.89 % | 7.64 / 14.47 % | 0.792 | |
  | ... + start resistance 2 x ps | 4.51 / 12.93 % | 8.65 / 16.21 % | 0.759 | +0.42 |
  | ... + sliding stress 1.6 / 2.2 MPa | 5.03 / 13.63 %, 5.84 / 14.14 % | | 0.805 / 0.821 | |
  | ... + heat transfer x 2 | 4.50 / 13.54 % | 8.55 / 16.10 % | 0.797 | +0.47 |
  | flame spread ~ fill^4 (`ignitionFillExponent`) | 4.53 / 13.50 % | 9.72 / 17.20 % | 0.837 | +0.45 |
  | flame spread ~ fill^2, 0.2 ms | 4.47 / 13.14 % | 10.20 / 16.44 % | 0.793 | +0.35 |
  | burnExponent -0.15 + heat loss ~ fill^-0.5 / -1 / -2 (`heatFillExponent`) | 4.49 / 13.53 %, 4.51 / 13.54 %, 4.61 / 13.55 % | 8.57 / 16.12 %, 8.56 / 16.12 %, 8.51 / 16.10 % | 0.796 / 0.799 / 0.804 | |
  | burnExponent -0.15 + sliding friction ~ fill^-1 / -2 (`slideFillExponent`) | 5.00 / 13.05 %, 5.68 / 12.62 % | 8.66 / 15.59 %, 11.74 / 14.79 % | 0.814 / 0.828 (pistol 0.69 / 0.84, pub 0.88) | |

  None is adopted: the shipped constants keep the velocity gain with the charge that all sources agree on. burnExponent -0.15 with refitted forms
  is the best pressure fit (rifle -0.8, pistol -1.2 points) at the cost of about 1.7 % velocity between the start and the maximum load (the reason
  for -0.05 since 2026-10-08). `ignitionFillExponent` (flame spread time x (fill / 0.85)^k) stays as a switch, off.

  A loss term to give back that velocity (2026-11-17): losses that act mostly after the peak and fall with the fill, so the velocity rises with
  the charge without a matching rise in peak pressure. The heat loss is too small a share of the energy (the velocity exponent moves by 0.005 per
  unit of k). The sliding friction restores the slope (pistol nearly fully at k = 2) but ruins the velocity between tables: the fill ranges from
  about 0.3 (pistol) to 1.1 between cartridges, far more than the +-10 % of the charge inside a table, so the term shifts whole tables and some
  light pistol loads no longer leave the barrel. A charge term inside the table would need the table's own reference charge, which a single load
  does not have. Both stay as switches, off.

  A Sebert factor that falls with the charge (`gasShareRatioExponent`: gasMassShare x ((charge / bullet) / 0.3)^-k, Pidduck-Kent direction),
  with burnExponent -0.15, k = 1 / 2 / 4: rifle velocity exponent 0.847 / 0.873 / 0.887 (pub 0.898) and the loading-density trend gone, but
  rifle held-out 5.09 / 14.28 %, 5.60 / 14.96 %, 5.87 / 15.14 % against 4.48 / 13.52 %: the charge-to-bullet ratio spans 0.1-1 between
  cartridges, so the share moves whole tables, and the pressure exponent returns to 2.1-2.2 (the gain of burnExponent -0.15 is undone). Pistol
  unchanged (mc / m about 0.05). Switch, off.

  **Joint refit per class** (2026-11-18, adopted). Were those terms only worse because every other constant had been fitted without them? All
  global constants of the physical model (23: burn exponent, gas share and its charge-ratio exponent, chambrage, covolume, capacity, impetus,
  sliding and engraving terms, case-neck friction, heat loss, gas constant, primer energy and time, flame spread, spin inertia, the three
  fill exponents) were fitted together with the Ba of every powder and the burn form of every powder with at least 20 pressures, on the
  training cartridges, rifle and pistol/revolver loads separately (`~/Dokumente/QT/GRTReloaded-work/refit-2026-11-18`, `joint_fit.py`: the
  Jacobian of all powders comes from one evaluation per parameter kind, since a powder's loads depend only on its own values). Rifle held-out
  cartridges, caliber factors off (velocity ~ charge^x of the published loads 0.90):

  | rifle | v / p rms | velocity ~ charge^ |
  |---|---|---|
  | shipped constants, Ba and forms refitted | 4.57 / 14.05 % | 0.90 |
  | joint fit | 4.54 / 12.48 % | 0.75 |
  | joint fit + the velocity error inside each table x 3 (adopted) | 4.57 / 12.91 % | 0.81 |

  The unweighted fit buys its pressure with the velocity gain from the start to the maximum load (that error is small in the pooled rms);
  the weight keeps part of it. Every 0.05 of the exponent costs about half a point of pressure: no set of constants gives both. Adopted for
  rifles: burnExponent -0.155, gas share 0.266 x ((charge / bullet) / 0.3)^-0.52, chambrage 0.15, covolume 0.84 cm3/g, impetus 1.070,
  sliding stress 1.29 MPa, case-neck friction 1.67 MPa over the seating depth, ignition 0.13 ms; heat loss, gas constant, primer and spin
  inertia stay about where they were, the fill exponents at 0. Pistol and revolver cartridges were fitted the same way with their own
  values (`Calibration::...Handgun`, `handgunCalibration()`): burnExponent -0.014, no charge-ratio term, chambrage 0.06, covolume 1.04,
  capacity 1.24, impetus 1.111, sliding stress 0.74 MPa, primer 8.7 J (held-out velocity 8.65 -> 7.75 %, pressure 16.85 -> 17.76 %, velocity
  exponent 0.64 -> 0.71, published 0.92). Then, as after every model change: the load files rebuilt, the caliber factors refitted (the same
  86 calibers), the Ba of all powders refitted (`refit.sh ... "ba compare"`, adopted as a whole: better than the per-powder choice in both
  classes), the bullet start values checked (refitted they are not better on held-out powders: velocity 3.45 -> 3.37 %, pressure 10.26 ->
  10.67 %; kept). The primer start pressures per type were checked too: the rifle types make no measurable difference (held-out 4.50 / 12.61 %
  with all at 250 bar, 4.47 / 12.70 % refitted); `primers.json` stays.

  All clean published loads, velocity / pressure rms (bias), the same loads for both models:

  | | rifle | pistol and revolver |
  |---|---|---|
  | before (2026-11-17) | 4.21 / 12.57 % (-0.3 / -0.2) | 7.72 / 15.02 % (-0.7 / +2.0) |
  | joint refit per class | 4.36 / 11.64 % (-0.6 / -0.9) | 7.77 / 14.78 % (-0.1 / -2.3) |

  Velocity ~ charge^x on these loads: rifle 0.79 (before 0.88, published 0.94), pistol 0.71 (before 0.65, published 0.93); pressure ~ charge^x
  rifle 1.97 (before 2.28, published 2.08), pistol 1.38 (before 1.27, published 1.65). Measured loads (`data/measurements`): .223 Rem. and
  308 Win. pressures closer, N340 9 mm Luger 1 to 6 % low (before 0 to 6 % high), N110 .357 Magnum 24 % high (before 16 %). Shotgun loads are not
  concerned (they keep the Original equations).

  **Bearing surface from the bullet's mass** (`noseFromMass` 1, 2026-11-17). The nose is no longer one calibre: its length follows from the
  volume the bullet lacks against a cylinder of its length (jacketed 10.5, lead 10.9 g/cm3, nose filling 55 % of its cylinder, 0.5 calibre to
  60 % of the length without the boat tail). Against the one-calibre rule the bearing surface of the published rifle loads is 0.57 x (median;
  10-90 %: 0.51-0.80), that of the pistol and revolver loads 1.65 x (1.0-2.3: round and flat noses are shorter than one calibre). Everything
  fitted with the old length was checked and refitted in this order (published loads of `refit.sh build`, 87,000 rifle and 16,000 pistol loads):
  - the sliding stress (0.70 MPa for both classes): rifles 1.15 MPa (1.0 left the pressure 1.2 % low, 1.4 costs velocity), pistols and revolvers
    their own `slideStressHandgun_MPa` 0.4 (with 0.7 their pressure bias rose from +2.0 to +4.3 %);
  - the handgun start resistance (0.6, decaying over the bearing length) and the lead factor (0.45 on the Lyman cast rifle loads): unchanged,
    other values are not better;
  - the per-caliber burn factors (`fit_caliber_scales.py`, the same 86 calibers as before) and the powders' Ba (`refit.sh ... "ba compare"`,
    16 powders adopted);
  - the start values of the validated bullets (`fit_bullet_start_values.py --validated-only --holdout`): refitted under the new model they are
    not better on held-out powders (velocity 3.21 against 3.20 %, pressure 11.59 against 11.35 %), so the records keep theirs.

  All clean published loads, velocity / pressure rms (bias), against the one-calibre model with its own fitted values: rifle 4.20 / 12.38 %
  (-0.3 / -0.1 %) instead of 4.15 / 12.28 % (-0.1 / +0.5 %), pistol 7.75 / 15.05 % (-0.7 / +2.1 %) instead of 7.93 / 14.96 % (-0.6 / +2.0 %);
  measured loads (`data/measurements`) about the same. The model is at par with the old one, with bearing surfaces that are about right.

Method and numbers: `scripts/data/loaddata/README.md`, `analysis/calibrate_published.py`, `analysis/fit_physical_powders.py`,
`analysis/fit_caliber_scales.py` (and the experiments that did not help in [ACCURACY.md](ACCURACY.md)).
