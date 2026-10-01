# GRT Reloaded

A Qt 6 / C++20 desktop application for **interior ballistics of handloads**: choose a cartridge, bullet and
powder, enter the charge, and see pressure, bullet energy and velocity along the barrel, with a load development
ladder, reports and printouts. It follows the workflow of *Gordon's Reloading Tool* and reads and writes its
`*.grtload` files.

> **Not affiliated.** This is an independent project. It is not affiliated with, endorsed or supported by
> Gordon's Reloading Tool or its author.

## Safety and disclaimer

The results are **calculated** and can differ considerably from reality. Compare every load with the current
loading tables of the component manufacturers and start low.

This software is provided "as is", without warranty of any kind, express or implied, including accuracy,
correctness or fitness for a particular purpose. The author and all contributors accept **no responsibility or
liability** for any damage, injury or death, loss, or any other consequence arising from the use of this software or
of its results, including damage to firearms or property. Reloading ammunition is dangerous and is done entirely at
your own responsibility.

## Features

- Cartridge, projectile and propellant records from a database; replace any component of a load; add your own
  records or changed copies of existing ones (they are stored in the program's own database)
- Pressure, energy and velocity curves with markers and pressure bands, burn-rate chart, load ladder
- Cartridge and chamber drawings with CIP dimensions (hover a table row to see the dimension in the drawing)
- Measurement results: chronograph velocity and pressure per charge with statistics (mean, SD, ES), compared
  with the simulation in a table and a chart over the charge; stored in the load file like the original does
- Reports with a template editor, print preview and PDF export, adjustable scale
- Metric / imperial / original units, also per field; themes (light, dark); global font size
- Translations (American English is the source language)

## Accuracy

The solver is an own lumped-parameter model, calibrated against reference runs of the original program. It is
**not** the algorithm of any other program. How far off it currently is (errors of the solver against the
original's results; lower is better):

<!-- accuracy:start -->
| Test set | Runs | Peak pressure: mean error | max | Muzzle velocity: mean error | max | Curve shape: mean deviation |
|---|---:|---:|---:|---:|---:|---:|
| Other cartridges (caliber sweep) | 28 | 1.4 % | 6.1 % | 1.0 % | 9.0 % | 4.2 % |
| Powders (40 powders, one cartridge) | 39 | 1.7 % | 5.2 % | 0.9 % | 5.6 % | 4.0 % |
| Charge, bullet weight, start pressure, case volume | 24 | 1.9 % | 4.8 % | 0.7 % | 2.4 % | 3.4 % |
| Barrel length, twist, Sebert factor | 10 | 1.2 % | 3.3 % | 0.5 % | 2.3 % | 3.8 % |
| Powder temperature | 26 | 0.5 % | 0.8 % | 0.3 % | 0.3 % | 2.4 % |
| Bullet friction | 11 | 1.2 % | 3.4 % | 0.8 % | 2.5 % | 2.9 % |
| Almost closed vessel (very heavy bullet) | 9 | 3.0 % | 7.4 % | 1.2 % | 2.7 % | 3.9 % |
| Fast powders (burn rate, step length, form function) | 21 | 1.6 % | 5.4 % | 0.7 % | 1.0 % | 3.8 % |
| Loading density (charge, case volume, powder density, Qex) | 22 | 4.2 % | 15.7 % | 0.8 % | 2.4 % | 4.9 % |
| **All runs** | 190 | 1.8 % | 15.7 % | 0.8 % | 9.0 % | 3.8 % |

_Absolute errors against 190 runs of the original program; updated 2026-10-01 with `tools/update_accuracy.py`. "Curve shape" is the rms relative pressure error along the barrel (where the pressure is above 5 % of its maximum). The charge-too-high rule matches the original for all tested loads._
<!-- accuracy:end -->

Unusual loads, other powders and other cartridges than the ones tested can deviate more.

## Building

Requirements: Qt 6.4 or newer (Widgets, Sql, Charts, Xml, PrintSupport, Test, LinguistTools), CMake 3.16+, a C++20
compiler (GCC 13 tested), Ninja recommended.

```
tools/build.sh debug          # configure, build with all cores, run the tests
# or
cmake --preset release && cmake --build --preset release --parallel
ctest --preset release
```

The version is set once, at the top of `CMakeLists.txt` (`GRT_VERSION`).

Qt Charts is only available under the GPL (or a commercial license), which affects how the application may be
distributed.

## Data

The program keeps its **own database** (`GRTReloaded.db` in the application data folder, created on first start)
for every new record. The records of Gordon's Reloading Tool can be added by choosing a **plain (unencrypted)
copy** of its database in *File ▸ Settings ▸ Database*; that file is only read. Both appear as one database
everywhere in the program (own records get ids from 1,000,000 on). No database is included in this repository, and
the program does not decrypt one.

Several tests use sample loads and a database that are not part of the repository; those that need them are skipped
or have to be supplied locally in `tests/data/` and `tools/`.

## Settings

Stored with `QSettings` (organization and application name `GRTReloaded`): on Linux in
`~/.config/GRTReloaded/GRTReloaded.conf`, on Windows in the registry under
`HKEY_CURRENT_USER\Software\GRTReloaded\GRTReloaded`.

## Translations

See [translations/README.md](translations/README.md).

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE). Qt Charts is GPL-licensed as well.

The original program's database, its sample files and any reference data derived from it are **not** part of this
repository and are not covered by this license.
