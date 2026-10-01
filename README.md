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

The solver is an own lumped-parameter model, calibrated against reference runs; typical deviations are about 1 %
in muzzle velocity and 3 % in peak pressure, more for unusual loads. It is **not** the algorithm of any other
program.

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
