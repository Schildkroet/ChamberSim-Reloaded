# Release notes

Changes in ChamberSim Reloaded. Pre-built binaries are on the
[releases page](https://github.com/Schildkroet/ChamberSim-Reloaded/releases).

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
