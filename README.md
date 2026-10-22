# <img src="packaging/linux/chambersimreloaded.svg" width="48" height="48"> ChamberSim Reloaded
**Interior ballistics for handloaders — Linux 🐧 / Windows 🪟**

A Qt 6 / C++20 desktop application for **interior ballistics of handloads**: choose a cartridge, bullet and
powder, enter the charge, and see pressure, bullet energy and velocity along the barrel, with a load development
ladder, reports and printouts. It follows the workflow of *Gordon's Reloading Tool*. It saves its loads in its own
`*.csrload` format (XML, with the data the original does not know, such as the physical model's powder values and
the measurements). The original's `*.grtload` files can be opened (and are saved as `*.csrload`); *File ▸ Save as…*
with the `*.grtload` filter exports a load for the original, without the data of this program.

> **Not affiliated.** This is an independent project. It is not affiliated with, endorsed or supported by
> Gordon's Reloading Tool or its author.

📖 **Documentation:** [online user manual](https://schildkroet.github.io/ChamberSim-Reloaded/) (also as PDF there; the
program opens the same manual with *Help ▸ User manual*, F1; *Help ▸ Online documentation* opens the web site).

## ⚠️ Safety and disclaimer

**This program is still in beta.** It may contain bugs, and its results can be completely wrong, in pressure as well as
in velocity. Never rely on them for a real load.

The results are **calculated** and can differ considerably from reality. Compare every load with the current
loading tables of the component manufacturers and start low.

This software is provided "as is", without warranty of any kind, express or implied, including accuracy,
correctness or fitness for a particular purpose. The author and all contributors accept **no responsibility or
liability** for any damage, injury or death, loss, or any other consequence arising from the use of this software or
of its results, including damage to firearms or property. Reloading ammunition is dangerous and is done entirely at
your own responsibility.

## ✨ Features

- 🗃️ Cartridge, projectile and propellant records from a database; replace any component of a load; add your own
  records or changed copies of existing ones (they are stored in the program's own database)
- 📈 Pressure, energy and velocity curves with markers and pressure bands, burn-rate chart, load ladder
- 📐 Cartridge and chamber drawings with CIP dimensions (hover a table row to see the dimension in the drawing)
- ⏱️ Measurements tab: chronograph velocity and pressure per charge with statistics (mean, SD, ES), compared with the
  simulation in a table and a chart over the charge; several measurement series per load; import of Garmin Xero,
  MagnetoSpeed and Labradar files; stored in the load file like the original does
- 🎛️ **Fit to measurements**: changes the powder's model values (Ba, a0, z1, z2) until the simulation agrees with
  your chronograph (and pressure) data, and saves the result as your own propellant
- 📷 Photos of the powder (overview and detail, with a scale bar) from the database; you can add your own photos to
  your own propellant records
- 🖨️ Reports with a template editor, print preview and PDF export, adjustable scale
- 📏 Metric / imperial / original units, also per field; themes (light, dark); global font size; own title bar on Linux
- ↩️ Undo / redo of the edits of a load (Edit menu, Ctrl+Z / Ctrl+Shift+Z); a two-step New load dialog (components, then the load data)
- 🗂️ Optionally reopens the loads of the last session at start (Settings > Start-up)
- 📝 Logging (level, console, log file) that you can set in the settings
- 🌍 Translations: English, German, French, Italian

## 🎯 Accuracy

The default **Physical Model** is fitted to the published load data of powder manufacturers; **GRT Original** is calibrated against
reference runs of the original program. Neither is the algorithm of any other program.

- **Physical Model against ~42,000 published loads** (cartridges that were left out of the fit): velocity about 3-6 % rms (Vihtavuori
  3 %, Hodgdon 6 %), peak pressure about 15 % rms where the tables publish it. The scatter of the tables themselves (lots, barrels,
  components) is of that size; single loads can deviate much more.
- **GRT Original against 349 runs of the original program**: mean error 0.8 % in muzzle velocity, 2.8 % in peak pressure.
- **Shotgun loads**: velocity 2.9 %, pressure 15.6 % rms over 4,883 published loads; always check them against the powder maker's data.

Tables, method, known weak spots and how to enter a shotgun load: [docs/ACCURACY.md](docs/ACCURACY.md).

## 📐 How the Physical Model calculates

The default model integrates the shot in time (Dormand-Prince 5(4) with error control, so the result does not depend on the number of
curve points). The most important equations, with the state *x* (bullet position), *v* (velocity), *z* (burnt fraction of the powder),
*Q* (heat lost to the walls) and *W* (work against the resistance):

```text
burn rate        dz/dt  = Ba * p[bar] * (p / 1000 bar)^n * phi(z) * (1 - exp(-t / tau))
gas pressure     p      = (kappa - 1) * U / Vgas
gas energy       U      = f * mc * z + E_primer * (1 - exp(-t / t_primer)) - 1/2 M v^2 - Q - W,     f = impetusFactor * Qex
gas volume       Vgas   = V0 + A (x - x0) - mc (1 - z) / rho_c - eta * mc * z
bullet           M dv/dt = A * p - F,                                    M = mp (1 + spin) + mc / 4
resistance       F      = engraving + sliding + gfriction * A + air ahead of the bullet
heat and work    dQ/dt  = h * S * (T - Tw),   T = p * Vgas / (m_gas * R),   dW/dt = F * v
peak pressure    p_breech = p + mc * a / (4 A)                           (Lagrange gradient of the gas column; the powder burns at p_breech)
```

- **Burn:** *Ba* is the powder's vivacity, *phi(z)* its three-phase form function (*a0*, *z1*, *z2*), *n* a small pressure exponent
  and *tau* the flame spread from the primer (the grains are lit over about 0.1 ms).
- **Gas:** *mc* is the charge, *Qex* the specific explosive heat, *kappa* the ratio of the specific heats, *eta* the covolume,
  *rho_c* the powder density, *V0* the case volume (less the part of the bullet in the case) and *A* the bore area.
- **Start:** the powder burns in the closed case until the gas pressure reaches the start pressure; only then the bullet moves.
- **Resistance:** the start resistance (a fitted share of the start pressure times *A*: 1.2 for rifles decaying over 2 mm, 0.6 for pistols and
  revolvers over the bearing length), sliding friction
  (a stress times the bearing surface, which grows with the bullet's oversize over the groove), the extra bullet friction
  *gfriction* [bar] times *A*, and the air pushed out of the barrel.
- **Heat loss:** turbulent pipe flow (Dittus-Boelter) over the chamber and the bore behind the bullet.

The burn values of the powders come from fits to published loads, see [docs/SOLVER.md](docs/SOLVER.md); the code is in
[src/core/solver.cpp](src/core/solver.cpp). **GRT Original** and **GRT Tuned** keep the structure of the model in the original program's documentation (Resal energy balance, vivacity burn law, Sebert's mass) with constants fitted to its results or to published loads; they are not the original's own algorithm.

## 🛠️ Building

Requirements: Qt 6.4 or newer (Widgets, Sql, Charts, Xml, PrintSupport, Test, LinguistTools), CMake 3.16+, a C++20
compiler (GCC 13 tested), Ninja recommended.

```
tools/build.sh debug          # configure, build with all cores, run the tests
# or
cmake --preset release && cmake --build --preset release --parallel
ctest --preset release
```

All other commands (tests, screenshots, data tools, powder fits, calibration, release): [CHEATSHEET.md](docs/CHEATSHEET.md).


The build runs `clang-format -i` (see `.clang-format`: Allman braces, 140 columns) on the sources before it
compiles. It only changes files that are not yet formatted, and it is skipped when `clang-format` is not installed.
Turn it off with `-DCSR_FORMAT_ON_BUILD=OFF`. Release builds use `-O2`. For code completion in VS Code or clangd,
configure once with `cmake --preset debug` (it writes `build/debug/compile_commands.json`; `.vscode/settings.json`
and `.clangd` point to it).

Tests that need a database or loads that are not part of the repository are skipped or need them in `tests/data/` and `tools/`.

## 🗃️ Data

The program shows one database made of three parts:

- the **project database**: calibers, projectiles and propellants contributed to this repository (`data/`, JSON
  files: one per caliber, one per manufacturer for projectiles and propellants; CC BY-SA 4.0). It is built into the program, so every user has all of it; see
  [data/README.md](data/README.md) and [CONTRIBUTING.md](docs/CONTRIBUTING.md) to add records;
- your **own records** (`ChamberSimReloaded.db` in the application data folder): everything you create or copy in the
  *Database* tab; *Export…* turns one into a project file;
- optionally the records of Gordon's Reloading Tool: choose a **plain (unencrypted) copy** of its database in
  *File ▸ Settings ▸ Database*. It is only read; no database of the original is included in this repository, and
  the program does not decrypt one.

### 📚 Where the project database comes from

The records in `data/` were collected from public sources with scripts (C.I.P. data sheets for 484 calibers, the product pages and bullet files of 51
bullet makers for 3,010 projectiles, and 214 propellants fitted to the published load tables of Vihtavuori, Hodgdon, Western, Lovex,
Reload Swiss, ADI and Vectan); nothing was copied from the database of Gordon's Reloading Tool. Records made from estimates say so in
their `descr` field. Sources, quality and the commands to rebuild: [docs/DATA_SOURCES.md](docs/DATA_SOURCES.md). Please check every load
against the manufacturer's current load data.

### 🔗 Using the records of Gordon's Reloading Tool

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

## 🧪 Add a new powder from your own measurements

Fit a propellant model to your chronograph (and pressure) data directly in the program: copy a similar powder in the *Database* tab,
type the shots in the *Measurements* tab, click *Fit to measurements…* and save the result as your own propellant. Step by step, with
what you need to measure: [docs/FIT_POWDER.md](docs/FIT_POWDER.md). Reloading is dangerous: use only loads from the manufacturer's
current data and start low.

## 🤝 Add data to the project database

Calibers, projectiles and propellants are welcome as pull requests: rules, the three procedures and the CI checks are in
[docs/CONTRIBUTING.md](docs/CONTRIBUTING.md); the file format is in [data/README.md](data/README.md). The data in `data/` is licensed
CC BY-SA 4.0.

## ⚙️ Settings

Stored with `QSettings` (organization and application name `ChamberSimReloaded`): on Linux in
`~/.config/ChamberSimReloaded/ChamberSimReloaded.conf`, on Windows in the registry under
`HKEY_CURRENT_USER\Software\ChamberSimReloaded\ChamberSimReloaded`.

### 🧮 Solver model

*File ▸ Settings ▸ Solver ▸ Model* chooses the solver (`--calibration original|published|physical` for one run). Changing it
recalculates every open load.

- **Physical Model** (default): the project's own time-integrated model (flame spread, engraving, friction, air, heat loss, spin) with
  physical burn values for 183 project powders and a burn rate factor per pistol / revolver caliber; powders without them (own records) are less accurate.
- **GRT Original**: constants fitted to runs of Gordon's Reloading Tool; results like the original.
- **GRT Tuned**: the original's equations, constants fitted to published loads.

Shotshells always keep the original's constants. Details: [docs/SOLVER.md](docs/SOLVER.md), [docs/ACCURACY.md](docs/ACCURACY.md).

### 📝 Logging

*File ▸ Settings ▸ Logging* sets the log level (Debug, Info, Warning, Error or Off), whether the messages go to
the console (stderr) and whether they go to a log file (`ChamberSimReloaded.log` in the application data folder; it is
renamed to `.log.1` at 1 MB). The default is Warning, to the console only. For one run, `--log-level debug` (or
`info`, `warning`, `error`, `off`) overrides the saved level. In the code use `LOG_Debug()`, `LOG_Info()`,
`LOG_Warn()` and `LOG_Error()` (header `log.h`) like a stream: `LOG_Info() << "Load opened:" << path;`.

## 🌍 Translations

* 🇺🇸 English (American English, the source language and default)
* 🇩🇪 German
* 🇫🇷 French
* 🇮🇹 Italian

*View ▸ Language* switches the language (after a restart). How to translate: [translations/README.md](translations/README.md).

## 📜 License

GNU General Public License v3.0 — see [LICENSE](LICENSE). Qt Charts is GPL-licensed as well.

The original program's database, its sample files and any reference data derived from it are **not** part of this
repository and are not covered by this license.
