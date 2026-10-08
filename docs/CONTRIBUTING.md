# Contributing

## Records for the project database

The project database (`data/`) is shared by every user of GRT Reloaded, so new calibers, projectiles and
propellants are very welcome. The procedures are below; the file format is described in
[data/README.md](../data/README.md).

Please make sure that:

- **the data is yours to give**: your own measurements, or values published by the manufacturer (data sheets,
  catalogs) or by CIP/SAAMI. Name the source in the record's `origin` or `descr` field.
- **nothing is copied from Gordon's Reloading Tool's database** (`GordonsReloadingTool.db`) or from other programs'
  databases. This includes powder models (burn rate, form function, ...) fitted by those programs. Records that are
  derived from such data cannot be accepted, because that data is not ours to license.
- propellant models come with a short description of how they were made (which loads, barrel, chronograph,
  pressure measurement) in `descr`.
- one pull request may add several records; `grt_datacheck` (run by CI) must pass.

By opening a pull request you agree that your records are published under
[CC BY-SA 4.0](../data/LICENSE.md).

## Code

Pull requests for the program are welcome as well. Please keep to the style of the surrounding code (C++20, Qt 6),
add tests for new behavior (`tests/`), and run `ctest` before submitting.

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
10. Put the file into `data/calibers/` or `data/projectiles/`. For a projectile run
    `python3 scripts/data/recordfiles.py data/projectiles`: it moves the record into the file of its manufacturer.
11. Do procedure 3.

### Procedure 2: Make a propellant model from measurements

CAUTION: Reloading is dangerous. Use only loads from the current load data of the manufacturer.

A new propellant model needs these measurements:

- 2 calibers,
- 2 bullet weights for each caliber,
- 2 charges for each bullet weight (start charge and maximum charge),
- 5 shots for each charge.

Use a good chronograph (for example MagnetoSpeed or LabRadar). Measurements of the pressure help.

1. Make one load file (`.grtreload`) for each caliber and bullet weight.
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
    | the `.json` file from step 8 (one file), then run `python3 scripts/data/recordfiles.py data/propellants` (moves the record into the file of its manufacturer) | `data/propellants/` |
    | your load files from step 7 (not the `-fitted` files) | `data/measurements/<manufacturer>-<name>-<lot>/` |

    Other users can then do step 8 again with your load files and get the same model.
13. Do procedure 3.

Use the option `--save` only for your own loads. Add it to the command of step 8. The tool then also writes a
copy of each load file with the new values (`<name>-fitted.grtreload`, in the folder of the load file). Open these
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
