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

- Cartridge, projectile and propellant records from a database; replace any component of a load
- Pressure, energy and velocity curves with markers and pressure bands, burn-rate chart, load ladder
- Cartridge and chamber drawings with CIP dimensions (hover a table row to see the dimension in the drawing)
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
| Other cartridges (caliber sweep) | 28 | 1.5 % | 4.3 % | 1.0 % | 8.2 % | 4.1 % |
| Powders (40 powders, one cartridge) | 39 | 1.8 % | 6.2 % | 0.9 % | 5.4 % | 3.9 % |
| Charge, bullet weight, start pressure, case volume | 24 | 1.9 % | 4.7 % | 0.6 % | 2.1 % | 3.4 % |
| Barrel length, twist, Sebert factor | 10 | 1.1 % | 3.5 % | 0.4 % | 2.6 % | 4.0 % |
| Powder temperature | 26 | 0.5 % | 0.7 % | 0.1 % | 0.2 % | 2.6 % |
| Bullet friction | 11 | 1.2 % | 3.3 % | 0.7 % | 2.4 % | 2.9 % |
| Almost closed vessel (very heavy bullet) | 9 | 3.7 % | 10.0 % | 1.2 % | 2.3 % | 3.7 % |
| Fast powders (burn rate, step length, form function) | 21 | 1.7 % | 6.2 % | 0.7 % | 1.2 % | 3.7 % |
| **All runs** | 168 | 1.6 % | 10.0 % | 0.7 % | 8.2 % | 3.6 % |

_Absolute errors against 168 runs of the original program; updated 2026-10-01 with `tools/update_accuracy.py`. "Curve shape" is the rms relative pressure error along the barrel (where the pressure is above 5 % of its maximum). The charge-too-high rule matches the original for all tested loads._
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

The program needs a **plain (unencrypted) SQLite database** with the tables `caliber`, `projectile`, `propellant`
and `gun` in the layout used by Gordon's Reloading Tool. Open it with *File ▸ Open database…*; the last used
database is remembered. No database is included in this repository, and the program does not decrypt one.

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
