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
| Other cartridges (caliber sweep) | 28 | 1.3 % | 4.0 % | 1.2 % | 10.9 % | 4.5 % |
| Powders (40 powders, one cartridge) | 39 | 2.0 % | 6.4 % | 0.9 % | 6.3 % | 4.0 % |
| Charge, bullet weight, start pressure, case volume | 24 | 1.6 % | 4.4 % | 0.7 % | 3.4 % | 3.4 % |
| Barrel length, twist, Sebert factor | 10 | 1.0 % | 2.8 % | 0.4 % | 1.8 % | 2.6 % |
| Powder temperature | 26 | 0.5 % | 1.0 % | 0.3 % | 0.4 % | 2.3 % |
| Bullet friction | 11 | 1.2 % | 3.9 % | 0.8 % | 2.4 % | 2.8 % |
| Almost closed vessel (very heavy bullet) | 9 | 2.8 % | 7.2 % | 1.1 % | 2.4 % | 3.6 % |
| Fast powders (burn rate, step length, form function) | 21 | 1.4 % | 5.2 % | 0.6 % | 0.9 % | 3.6 % |
| Fast-powder probes on the 8x57 (Ba 1..8, form function, start pressure, gas, charge) | 28 | 2.1 % | 8.4 % | 0.8 % | 3.9 % | 3.1 % |
| Pistol and revolver loads with fast powders | 28 | 3.0 % | 8.1 % | 0.9 % | 2.8 % | 4.1 % |
| Loading density (charge, case volume, powder density, Qex) | 22 | 3.4 % | 13.9 % | 0.7 % | 3.4 % | 4.8 % |
| **All runs** | 246 | 1.9 % | 13.9 % | 0.8 % | 10.9 % | 3.6 % |

_Absolute errors against 246 runs of the original program; updated 2026-10-02 with `tools/update_accuracy.py`. "Curve shape" is the rms relative pressure error along the barrel (where the pressure is above 5 % of its maximum). The charge-too-high rule matches the original for all tested loads._
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

## Data

The program shows one database made of three parts:

- the **project database**: calibers, projectiles and propellants contributed to this repository (`data/`, one JSON
  file per record, CC BY-SA 4.0). It is built into the program, so every user has all of it; see
  [data/README.md](data/README.md) and [CONTRIBUTING.md](CONTRIBUTING.md) to add records;
- your **own records** (`GRTReloaded.db` in the application data folder): everything you create or copy in the
  *Database* tab; *Export…* turns one into a project file;
- optionally the records of Gordon's Reloading Tool: choose a **plain (unencrypted) copy** of its database in
  *File ▸ Settings ▸ Database*. It is only read; no database of the original is included in this repository, and
  the program does not decrypt one.

### Using the records of Gordon's Reloading Tool

Gordon's Reloading Tool keeps its database encrypted. A script makes a plain copy of it; put it into the folder of
GRT and run it there, then choose the resulting `GordonsReloadingTool.plain.db` in *File ▸ Settings ▸ Database*.

- **Linux**: copy `tools/grt_decrypt_db.sh` next to the `GordonsReloadingTool` program of the unpacked **Linux**
  version and run `./grt_decrypt_db.sh`. It works on Debian/Ubuntu, Fedora, Arch and openSUSE and installs what is
  missing after asking (gdb and the 32-bit GTK2 runtime; Xvfb when there is no desktop).
- **Windows**: copy `tools/grt_decrypt_db.cmd` and `tools/grt_decrypt_db.ps1` next to `GordonsReloadingTool.exe` and
  double-click `grt_decrypt_db.cmd`. It installs what is missing after asking (Python with winget, the Python
  package frida into a private environment).

Both start a copy of GRT and let it remove the encryption from a copy of its database; GRT and its database stay
unchanged. `--check` / `-Check` only report what would be installed.

Several tests use sample loads and a database that are not part of the repository; those that need them are skipped
or have to be supplied locally in `tests/data/` and `tools/`.

## Add data to the project database

The project database is in the folder `data/`. All users get this data with the program. You can add calibers,
projectiles and propellants. Use a pull request on GitHub to send the data.

### Rules for the data

- Use only data from your own measurements, or data that a manufacturer, CIP or SAAMI published.
- Do not copy data from the database of Gordon's Reloading Tool or from other programs. This includes their
  propellant models (Ba, a0, z1, z2).
- Write the source of the data in the field `origin` or `descr`.
- The data in `data/` has the license CC BY-SA 4.0. When you send a pull request, you agree to this license.

### Procedure 1: Add a caliber or a projectile

1. Start GRT Reloaded.
2. Open the tab *Database*.
3. Select the tab *Calibers* or *Projectiles*.
4. Click *New…*. Or select a similar record and click *Copy…*.
5. Type the values. Use the values from the data sheet of the manufacturer, or measure them.
6. Click *Save*.
7. Make a load with the new record. Make sure that the results are correct.
8. Select the record in the tab *Database*.
9. Click *Export…*. The program writes a `.json` file.
10. Put the file into `data/calibers/` or `data/projectiles/`.
11. Do procedure 3.

### Procedure 2: Make a propellant model from measurements

CAUTION: Reloading is dangerous. Use only loads from the current load data of the manufacturer.

A new propellant model needs these measurements:

- 2 calibers,
- 2 bullet weights for each caliber,
- 2 charges for each bullet weight (start charge and maximum charge),
- 5 shots for each charge.

Use a good chronograph (for example MagnetoSpeed or LabRadar). Measurements of the pressure help.

1. Make one load file (`.grtload`) for each caliber and bullet weight.
2. Measure the volume of a fired case with water. Write the value into the field *Case Volume*.
3. Measure the length of the bullets. Write the mean value into the field *Projectile Overall Length*.
4. Measure the length of the cartridges. Write the mean value into the field *Cartridge Overall Length*.
5. Open the tab *Measurements*.
6. Add each charge. Type the velocity of each shot. If you have pressure values, type them too.
7. Save the load files.
8. Run the tool `grt_fitpowder` with all load files of the propellant. Give the name of the record file with
   `--out`. Use the name `<manufacturer>-<name>-<lot>.json`:

   ```
   grt_fitpowder --out vihtavuori-n150-lot123.json 8x57-196gr.grtload 8x57-170gr.grtload 308-150gr.grtload 308-168gr.grtload
   ```

   The tool changes the values Ba, a0, z1 and z2 of the propellant. The simulation then agrees with your
   measurements. The tool shows the difference for each charge before and after the change. The tool does not
   change your load files.
9. Read the warnings of the tool. Correct the load files if necessary. Then do step 8 again.
10. Make sure that the difference for each charge is small (less than approximately 2 %).
11. Open the `.json` file. Make sure that the values Qex, k, pc and pcd come from the manufacturer or from your
    measurements. The tool copies these values from the load file without a change. Make sure that `mname`,
    `pname` and `lotid` are correct.
12. Copy the files into the repository:

    | File | Copy to |
    |---|---|
    | the `.json` file from step 8 (one file) | `data/propellants/` |
    | your load files from step 7 (not the `-fitted` files) | `data/measurements/<manufacturer>-<name>-<lot>/` |

    Other users can then do step 8 again with your load files and get the same model.
13. Do procedure 3.

Use the option `--save` only for your own loads. Add it to the command of step 8. The tool then also writes a
copy of each load file with the new values (`<name>-fitted.grtload`, in the folder of the load file). Open these
files to continue your load development with the new model. You do not have to wait until the record is in the
program. Do not copy the `-fitted` files into `data/`.

Note: The program contains the records of `data/` when it is built. After step 12, build the program again to see
the new propellant in the tab *Database*.

### Procedure 3: Send the data

1. Make a fork of the repository on GitHub.
2. Add your files to the fork (commit).
3. Build the program and run `ctest`. Make sure that the test `projectdata` passes.
4. Make a pull request. Write the source of the data into the description.

The CI checks all files in `data/`. If a check fails, the pull request shows the problem. Correct the file and
push again.

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
