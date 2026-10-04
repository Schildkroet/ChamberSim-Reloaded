# <img src="packaging/linux/grtreloaded.svg" width="48" height="48"> GRT Reloaded
**Interior ballistics for handloaders — Linux 🐧 / Windows 🪟**

A Qt 6 / C++20 desktop application for **interior ballistics of handloads**: choose a cartridge, bullet and
powder, enter the charge, and see pressure, bullet energy and velocity along the barrel, with a load development
ladder, reports and printouts. It follows the workflow of *Gordon's Reloading Tool* and reads and writes its
`*.grtload` files.

> **Not affiliated.** This is an independent project. It is not affiliated with, endorsed or supported by
> Gordon's Reloading Tool or its author.

## ⚠️ Safety and disclaimer

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
  simulation in a table and a chart over the charge; several measurement series per load; import of MagnetoSpeed
  and Labradar files; stored in the load file like the original does
- 🎛️ **Fit to measurements**: changes the powder's model values (Ba, a0, z1, z2) until the simulation agrees with
  your chronograph (and pressure) data, and saves the result as your own propellant
- 📷 Photos of the powder (overview and detail, with a scale bar) from the database; you can add your own photos to
  your own propellant records
- 🖨️ Reports with a template editor, print preview and PDF export, adjustable scale
- 📏 Metric / imperial / original units, also per field; themes (light, dark); global font size; own title bar on Linux
- 📝 Logging (level, console, log file) that you can set in the settings
- 🌍 Translations: English, German, French, Italian

## 🎯 Accuracy

The solver has two main models (File > Settings > Solver, see below). The default **Physical Model** is fitted to published
load data of powder manufacturers; **GRT Original** is calibrated against reference runs of the original program. Neither
is the algorithm of any other program.

### Physical Model against published loads

Simulated with the project's powder records and compared with the manufacturers' tables (velocity, and measured peak
pressure where published). Hodgdon and Western: only cartridges that were left out of the fit (fitted on the other 3/4).
The other rows are the loads the powders' burn rates were fitted to, so they are not independent:

| Published data | Powders | Velocity: rms (median) | Peak pressure: rms (median) |
|---|---:|---:|---:|
| Hodgdon and Western, all | 59 | 5.9 % (2.5 %) | 12 % (6.1 %) |
| of these rifle powders | | 4.2 % | 9.6 % |
| of these pistol and shotgun powders | | 19 % | 39 % |
| Vihtavuori (velocities only) | 16 | 3.25 % | - |
| Lovex | 8 | 2.5 - 4.2 % | 6.5 - 11 % |
| Reload Swiss | 6 | 5 - 7 % | 13 - 20 % |
| Shotgun loads (Hodgdon, Alliant; 5,198 loads) | | 3.1 % | 13.9 % |

Single loads can deviate much more. Details of the method: `tools/loaddata/README.md`.

### Against the original program's runs

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
| Other cartridges (caliber sweep) | 28 | 55.7 % | 129.3 % | 9.7 % | 25.6 % | 39.6 % |
| Powders (40 powders, one cartridge) | 39 | 52.5 % | 195.5 % | 9.9 % | 43.1 % | 30.0 % |
| Charge, bullet weight, start pressure, case volume | 24 | 54.6 % | 196.4 % | 6.7 % | 20.6 % | 32.3 % |
| Barrel length, twist, Sebert factor | 10 | 51.6 % | 67.0 % | 8.1 % | 24.6 % | 37.4 % |
| Powder temperature | 26 | 49.6 % | 60.5 % | 5.6 % | 7.9 % | 29.4 % |
| Bullet friction | 11 | 54.3 % | 64.0 % | 6.0 % | 15.6 % | 36.0 % |
| Almost closed vessel (very heavy bullet) | 9 | 3.8 % | 14.2 % | 6.7 % | 15.7 % | 11.9 % |
| Fast powders (burn rate, step length, form function) | 21 | 27.3 % | 160.9 % | 4.7 % | 10.0 % | 11.9 % |
| Fast-powder probes on the 8x57 (Ba 1..8, form function, start pressure, gas, charge) | 32 | 20.2 % | 66.5 % | 5.1 % | 10.8 % | 7.1 % |
| Powder probes on a 9 mm Luger (Ba 1..32, form function, start pressure, gas, charge) | 34 | 37.9 % | 194.8 % | 6.4 % | 14.8 % | 19.9 % |
| Pistol and revolver loads with fast powders | 28 | 30.7 % | 63.5 % | 6.5 % | 14.2 % | 18.9 % |
| Slow powders at low pressure (9 mm Luger: bullet mass, barrel, charge, start pressure, ...) | 34 | 118.9 % | 226.8 % | 2.1 % | 23.0 % | 77.8 % |
| Bullet jump (0.25 ... 4 mm, 8x57 and 9 mm) | 18 | 34.5 % | 50.7 % | 6.4 % | 10.1 % | 15.6 % |
| Powder and start-pressure variations of the 8x57 (shotshell batch, flag off) | 12 | 56.3 % | 160.9 % | 5.8 % | 13.6 % | 34.4 % |
| Loading density (charge, case volume, powder density, Qex) | 22 | 59.3 % | 253.0 % | 6.8 % | 20.6 % | 30.6 % |
| Other | 1 | 61.9 % | 61.9 % | 2.9 % | 2.9 % | 61.8 % |
| **All runs** | 349 | 49.9 % | 253.0 % | 6.4 % | 43.1 % | 30.0 % |

_Absolute deviations against 349 runs of the original program; updated 2026-10-04 with `tools/update_accuracy.py`. "Curve shape" is the rms relative pressure error along the barrel (where the pressure is above 5 % of its maximum). The charge-too-high rule matches the original for all tested loads._
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
(lot "shotshell", their burn rates fitted to 3,448 loads of the Hodgdon Reloading Manual: 12 gauge 2 3/4", 1 and 1 1/8 oz) the error against these loads is
3.1 % rms in velocity and 13.9 % in pressure over 5,198 loads of Hodgdon and Alliant (the pressure of one charge varies with wad and primer by about that much, which
the model does not know). Almost all of these loads are 12 gauge 2 3/4"; the few 20 gauge loads come out 16 % low in pressure, the 16 gauge ones 26 % high. For comparison,
the original program with the same 50 Vihtavuori loads: velocity +3 % on average (typically 4.8 % off) - closer - but
pressures of the maximum loads up to 1800 bar, where the guide's loads are pressure-tested within the CIP limit of
1050 bar (its shotshell setting changes little). Always check a shotgun load against the powder maker's data. Enter a shotgun load like this, the fit depends on it:

- *Projectile*: mass = shot + wad (about 2.5 g for a plastic wad), length = height of the shot column
  (shot weight / (shot density x 0.62) / bore area) + height of the wad, diameter = bore diameter;
- seated completely inside the hull: *Cartridge Overall Length* = hull length, *Seating depth* = projectile length;
- *Initial Gas Pressure* (start pressure) = crimp release, about 20 bar.

## 🛠️ Building

Requirements: Qt 6.4 or newer (Widgets, Sql, Charts, Xml, PrintSupport, Test, LinguistTools), CMake 3.16+, a C++20
compiler (GCC 13 tested), Ninja recommended.

```
tools/build.sh debug          # configure, build with all cores, run the tests
# or
cmake --preset release && cmake --build --preset release --parallel
ctest --preset release
```

All other commands (tests, screenshots, data tools, powder fits, calibration, release): [CHEATSHEET.md](CHEATSHEET.md).

## 🗃️ Data

The program shows one database made of three parts:

- the **project database**: calibers, projectiles and propellants contributed to this repository (`data/`, JSON
  files: one per caliber, one per manufacturer for projectiles and propellants; CC BY-SA 4.0). It is built into the program, so every user has all of it; see
  [data/README.md](data/README.md) and [CONTRIBUTING.md](CONTRIBUTING.md) to add records;
- your **own records** (`GRTReloaded.db` in the application data folder): everything you create or copy in the
  *Database* tab; *Export…* turns one into a project file;
- optionally the records of Gordon's Reloading Tool: choose a **plain (unencrypted) copy** of its database in
  *File ▸ Settings ▸ Database*. It is only read; no database of the original is included in this repository, and
  the program does not decrypt one.

### 📚 Where the project database comes from

The records in `data/` were collected from public sources with scripts; nothing was copied from the database of Gordon's
Reloading Tool. The scripts, the collected tables (CSV) and the notes on quality are in the repository, so the data can be
rebuilt and checked. Records made from estimates say so in their `descr` field.

- **Calibers (467)**: the C.I.P. TDCC data sheets (public PDFs of bobp.cip-bobp.org, tables I-V) are read with
  `tools/cipdata/cip_extract.py`: dimensions of cartridge and chamber, pressures, angles. The sheets give no case capacity;
  `cip_estimate_v.py` estimates it from the outline (about 6 % rms against known capacities, 10 % for rimfire). 25 more
  cartridges that have no sheet (SAAMI, wildcats, newer than the sheets: .224 Valkyrie, 6.8 Western, .22-.33 Nosler, ...) come
  from Wikipedia's cartridge infoboxes (`wiki_calibers.py`); they lack body and shoulder lengths and tolerances. `cipname` is the
  C.I.P. name, `altname` other names. Shotshells are not included. `Pmax` is the C.I.P. maximum pressure; `PmaxSAAMI` (a column the
  project added, shown next to it) the SAAMI maximum average pressure of 97 cartridges, from Wikipedia's infoboxes
  (`tools/cipdata/saami_pressures.py`). Load data makers use the lower of the two.
- **Projectiles (1761)**: the product pages of 16 manufacturers (Berger, Nosler, Speer, Hornady, Sierra, Barnes, Sellier & Bellot,
  Lapua, Norma, RWS, PPU, Federal, Winchester, Remington, H&N, Sako), read with `tools/bulletdata/*_fetch.py`: weight,
  diameter and, where published, ballistic coefficients. Only Berger, Nosler and Speer publish the bullet length; for the others
  it is estimated from mass, diameter and bullet type (`estimate_length.py`, about 5-10 % uncertain). Fiocchi's site refuses
  automated requests and is not included.
  The initial gas pressure (`gpressure`, the pressure at which the bullet starts and engraves) is an estimate from the construction
  (`start_pressure.py`): lead (cast, plated) 150 bar, jacketed 250 bar, brass monolithic 290 bar, copper monolithic (Barnes TSX/TTSX/LRX,
  Hornady CX/GMX, lead-free ...) 350 bar. Checked with the physical model against published pressures: copper loads median peak
  pressure error 7.6 -> 6.3 % (no longer 5 % too low), lead loads bias +4 -> -2 %; the original's 150 bar for jacketed handgun bullets
  made those results worse and is not used.
- **Propellants (112)**: Vihtavuori N130-N170 and N530-N570 (16), Lovex S040, S053, D063, S060-S071 and D073.4-D073.6 (11), Reload Swiss RS50-RS76 (6), Vectan Tubal 3000-8000 (4: from the Nobel Sport catalogue, which publishes no test barrel and no pressures, so a 600 mm barrel is assumed; its handgun powders fit too poorly and are not included), and
  55 rifle powders of Hodgdon, IMR, Winchester, Accurate and Ramshot (the Hodgdon and Western Powders manuals; Accurate 4100, Enforcer, H110 and W296 fit
  less well, 6-7 % in velocity, and the Lovex S040, S053 and D063 rest on 9-20 loads: their descriptions say so), and 20 shotgun powders
  (Hodgdon Clays, Titewad, International, 700-X, Universal, Longshot, Titegroup, Winchester Super Handicap and WST, Ramshot Competition; Alliant Red Dot,
  Green Dot, Blue Dot, Unique, Herco, Promo, American Select, Clay Dot, Pro Reach and 20/28; lot "shotshell", for shotshell loads only: 3,448 loads of the Hodgdon
  manual's 12 gauge tables and 1,993 of Alliant's Reloader's Guide).
  Vihtavuori: explosion heat (the "energy content") and bulk density are taken from its published powder table; the burn rate
  Ba was fitted to its published load data (start and maximum loads, about 4,000 samples), which contain no pressures. Lovex
  (Explosia's Reloading Guide 2019), Reload Swiss (online loading data of Nitrochemie), the Hodgdon Reloading Manual 2024 and the
  Western Powders Handloading Guide 8.0 publish pressures as well; the fit uses velocities and pressures (velocity error 3-6 % rms,
  pressure error 10-20 % rms; each record's `descr` states its numbers). None of them publishes the explosion heat or the bulk
  density: Qex is a typical value of a single-base (3800 kJ/kg) or double-base (4300 kJ/kg) powder (Lovex: S = single, D = double
  base by name; the others: the type that fits the velocities better), pcd is 900 kg/m3.
  k and the material density are typical values everywhere; the form function (a0, z1, z2) is not published, it is the combination
  that reproduces the velocities best and the data hardly determine it. Pistol and shotgun powders fitted too poorly and are not
  included, nor are powders with too few samples. `tools/grt_fitdata` and `tools/loaddata` do the work, see
  `tools/loaddata/README.md` for the method and the results.

The details, the commands to repeat each step and the limits of the data are in `tools/cipdata/README.md` and
`tools/loaddata/README.md`. Please check every load against the manufacturer's current load data.

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

The build runs `clang-format -i` (see `.clang-format`: Allman braces, 140 columns) on the sources before it
compiles. It only changes files that are not yet formatted, and it is skipped when `clang-format` is not installed.
Turn it off with `-DGRT_FORMAT_ON_BUILD=OFF`. Release builds use `-O2`. For code completion in VS Code or clangd,
configure once with `cmake --preset debug` (it writes `build/debug/compile_commands.json`; `.vscode/settings.json`
and `.clangd` point to it).

Several tests use sample loads and a database that are not part of the repository; those that need them are skipped
or have to be supplied locally in `tests/data/` and `tools/`.

## 🧪 Add a new powder from your own measurements

You can make a propellant model for your own use directly in the program. The model values (Ba, a0, z1, z2) are
fitted to your chronograph data. The result is saved as an own record, so you can use the powder in every load.
(To contribute a powder to the project database, see [Procedure 2](#-procedure-2-make-a-propellant-model-from-measurements).)

CAUTION: Reloading is dangerous. Use only loads from the current load data of the manufacturer, and start low.
The result is only as good as the measurements and the data of the powder.

What you need: a chronograph (for example MagnetoSpeed or Labradar) and, if you can, a pressure measurement. The more
different loads you measure (2 or more calibers, 2 or more bullet weights, 2 or more charges each, at least 5 shots
per charge), the better the model fits other loads. One load with few charges only determines Ba.

1. **Start from a similar powder.** Open the tab *Database*, select the *Propellants* list, select a powder with
   a similar burn rate and click *Copy…*. (Or click *New…* if you know all values.) Type the name, the
   manufacturer and the lot. Check the values that you know: explosion heat `Qex`, ratio of specific heats `k`,
   material density `pc`, bulk density `pcd` and the reference temperature `pt`. These do not come from the fit.
   Optionally add an overview photo and a detail photo (*Choose file…*, with the scale in px/mm if you know it).
   Click *Save*.
2. **Make a load with the new powder.** Create a load with *File ▸ New load…* (or open a `.grtload` file). On the
   tab *Propellant* click *Select Propellant…* and choose your new powder.
3. **Measure the case and the cartridge.** Measure the volume of a fired case with water and type it into *Case
   Volume*. Measure the length of the bullets and the cartridges and type the mean values into *Projectile Overall
   Length* and *Cartridge Overall Length*. A good case volume makes the fit much better.
4. **Type the measurements.** Open the tab *Measurements*. Click *Add charge* for each charge (or *Add ladder
   charges*). Select a charge and type the velocity of each shot in the shot list (and the pressure, if you
   measured it). You can also use *Paste shots* (one shot per line, velocity and optionally pressure), or click
   *Import chronograph file…* and choose a MagnetoSpeed `.csv` or a Labradar `Report.csv`. The files do not contain
   the powder charge: type it for each series in the import dialog. The table shows the mean velocity, SD and ES
   and the difference to the simulation.
   Use *New series* if you have several measurement series for the load (for example one for each range day).
5. **Fit the model.** Click *Fit to measurements…*. Check which values to fit (the default depends on the data: Ba alone for fewer than 3 measured
   charges, Ba and a0 for 3 to 5, all four values for 6 or more charges from at least 2 load files) and click *Fit*. The dialog shows the velocity and
   pressure of each charge before and after the fit. Read the warnings, for example when the case volume is only the
   database value or when there are fewer charges than values. With the physical model (the default) and a project powder, the fit
   changes the load's physical burn values (*Physical Model: ...* in the propellant table) and keeps them in the load; *Save as own
   propellant…* is then not offered.
6. **Check the result.** The differences after the fit should be small (less than approximately 2 % in velocity).
   If they are not, check the measurements, the case volume and the bullet and powder data.
7. **Save the model.** Click *Save as own propellant…* in the dialog. The program makes a new record
   "<name> (fitted)" in your own database and keeps the photos of the powder. Select it in other loads with
   *Select Propellant…*. You can also click *Apply to this load* to put the fitted values into this load only.
8. Fit again later with more loads: the fit uses all measurement series of the load, and the command-line tool
   `grt_fitpowder` (see Procedure 2) fits one model to several load files.

The fit is made for one load at a time. A powder that is fitted to one caliber and one bullet only can give wrong
results in other cartridges. Check every new load with a measurement before you trust the model.

## 🤝 Add data to the project database

The project database is in the folder `data/`. All users get this data with the program. You can add calibers,
projectiles and propellants. Use a pull request on GitHub to send the data.

### 📏 Rules for the data

- Use only data from your own measurements, or data that a manufacturer, CIP or SAAMI published.
- Do not copy data from the database of Gordon's Reloading Tool or from other programs. This includes their
  propellant models (Ba, a0, z1, z2).
- Write the source of the data in the field `origin` or `descr`.
- The data in `data/` has the license CC BY-SA 4.0. When you send a pull request, you agree to this license.

### 📦 Procedure 1: Add a caliber or a projectile

1. Start GRT Reloaded.
2. Open the tab *Database*.
3. Select the tab *Calibers* or *Projectiles*.
4. Click *New…*. Or select a similar record and click *Copy…*.
5. Type the values. Use the values from the data sheet of the manufacturer, or measure them.
6. Click *Save*.
7. Make a load with the new record. Make sure that the results are correct.
8. Select the record in the tab *Database*.
9. Click *Export…*. The program writes a `.json` file.
10. Put the file into `data/calibers/` or `data/projectiles/`. For a projectile run
    `python3 tools/recordfiles.py data/projectiles`: it moves the record into the file of its manufacturer.
11. Do procedure 3.

### 🔥 Procedure 2: Make a propellant model from measurements

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
    | the `.json` file from step 8 (one file), then run `python3 tools/recordfiles.py data/propellants` (moves the record into the file of its manufacturer) | `data/propellants/` |
    | your load files from step 7 (not the `-fitted` files) | `data/measurements/<manufacturer>-<name>-<lot>/` |

    Other users can then do step 8 again with your load files and get the same model.
13. Do procedure 3.

Use the option `--save` only for your own loads. Add it to the command of step 8. The tool then also writes a
copy of each load file with the new values (`<name>-fitted.grtload`, in the folder of the load file). Open these
files to continue your load development with the new model. You do not have to wait until the record is in the
program. Do not copy the `-fitted` files into `data/`.

Note: The program contains the records of `data/` when it is built. After step 12, build the program again to see
the new propellant in the tab *Database*.

### 📨 Procedure 3: Send the data

1. Make a fork of the repository on GitHub.
2. Add your files to the fork (commit).
3. Build the program and run `ctest`. Make sure that the test `projectdata` passes.
4. Make a pull request. Write the source of the data into the description.

The CI checks all files in `data/`. If a check fails, the pull request shows the problem. Correct the file and
push again.

## ⚙️ Settings

Stored with `QSettings` (organization and application name `GRTReloaded`): on Linux in
`~/.config/GRTReloaded/GRTReloaded.conf`, on Windows in the registry under
`HKEY_CURRENT_USER\Software\GRTReloaded\GRTReloaded`.

### 🧮 Solver model

*File ▸ Settings ▸ Solver ▸ Model* chooses the solver (`--calibration original|published|physical` for one run: GRT Original, GRT Tuned, Physical Model):

- **GRT Original**: constants fitted to runs of Gordon's Reloading Tool. Results like the original (the accuracy
  table above is for this choice).
- **GRT Tuned**: the original's equations, constants fitted to the loads and measured pressures of the Hodgdon Reloading Manual
  and the Western Powders Handloading Guide (22 powders, the burn rates of the records unchanged). Against the published tables of
  held-out cartridges and of other powders it is off by about 3.5 % in velocity and 10 % in peak pressure (the original's constants:
  4.5 % and 16 %), but its results differ from the original program (against its reference runs 27 % in peak pressure). Some of its
  constants are empirical without a physical meaning (covolume about 0, case capacity x 0.74).
- **Physical Model**, the default: the project's own model, not the original's. Integrated in time to convergence (the result does not depend on the
  number of curve points); the primer lights the powder, the burning spreads over the grains (flame spread, about 0.75 ms) and the bullet
  starts at the start pressure; covolume of the gas (x eta) and volume of the unburnt powder (1 / pc); engraving, friction of the bearing
  surface (bullet length), the extra friction gfriction [bar] and the air ahead of the bullet act on the bullet; heat loss to the chamber
  and bore walls; the spin (twist) takes energy. The peak pressure no longer depends on the barrel length. Results show heat and friction
  loss; a bullet that cannot leave the barrel is reported.
  Its constants and the **physical burn values** of 92 project powders (Ba, a0, z1, z2 fitted to each powder's published loads with
  pressures: Hodgdon, Western, Lovex, Reload Swiss; Vihtavuori publishes no pressures, its 16 powders have a fitted Ba and the median
  form function of the others) come from `analysis/calibrate_published.py --profile physical` and
  `analysis/fit_physical_powders.py` (file `data/physical/propellants.json`). A load stores them in its propellant section (`physBa`,
  `physA0`, `physZ1`, `physZ2`, `physRef` = the record values they belong to); they apply only while Ba, a0, z1, z2 of the load are
  unchanged, otherwise (other powders, edited values) the load's burn rate is scaled and the deviations are larger. A load saved
  before these values existed gets them when it is opened. The curves and the barrel time begin when the bullet moves (it waits at
  the start pressure until the gas overcomes the engraving and friction).
  Held-out cartridges (fitted on the other 3/4), the 59 Hodgdon and Western powders: velocity 5.9 % rms (median 2.5 %), peak pressure 12 % rms (median 6.1 %);
  the Published constants 6.0 % (2.3 %) and 19 % (6.4 %). Rifle powders: velocity 4.2 % (Published 3.6 %), pressure 9.6 % (10.1 %);
  pistol and shotgun powders: velocity 19 % (22 %), pressure 39 % (79 %). Velocity of rifle loads is still a little less accurate than
  with the Published constants, pressure is better everywhere. Vihtavuori (16 powders, velocities only): 3.25 % (Published 3.16 %,
  whose record burn rates were fitted to all of these loads). Lovex (8 powders): velocity 2.5-4.2 %, pressure 6.5-11 %;
  Reload Swiss (6): 5-7 % and 13-20 %.
  Shotshells always keep the original's constants.

Method and numbers: `tools/loaddata/README.md`, `analysis/calibrate_published.py`, `analysis/fit_physical_powders.py`.

Changing the setting recalculates every open load.

### 📝 Logging

*File ▸ Settings ▸ Logging* sets the log level (Debug, Info, Warning, Error or Off), whether the messages go to
the console (stderr) and whether they go to a log file (`GRTReloaded.log` in the application data folder; it is
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
