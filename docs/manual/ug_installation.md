# Installation and first start

## Installing

Release builds are available from the project’s GitHub page: an AppImage
for Linux and a zip file for Windows. Unpack or make the AppImage
executable and start it; no installation is needed. Both contain this
manual (**Help** → User manual, F1); the Windows zip also contains the
Visual C++ runtime, so nothing else has to be installed. To build the
program yourself you need Qt 6.4 or newer, CMake 3.16 or newer and a
C++20 compiler:

    tools/build.sh release
    build/release/ChamberSimReloaded

## Safety notice

At every start a safety notice is shown. Read it, then choose I
understand; Quit closes the program. With Do not show this again ticked,
I understand also switches the notice off for later starts. There is no
entry in the settings to bring it back.

## Command-line options

    ChamberSimReloaded [options] [load.csrload ...]
      -d, --database <file> the original's (plain) database
      -r, --reference <f>   overlay a reference result (JSON) on the chart
      --calibration <m>     solver model: original|published|physical
      --language <code>     de_DE, en_US, fr_FR, it_IT
      --theme <t>           light | dark
      --units <system>      metric | imperial | original
      --log-level <level>   debug | info | warning | error
      --help                all options

Files given on the command line are opened as loads. `--theme` applies
to this run only; `--units` and `--calibration` are saved like the
choices in the settings. `--screenshot` saves an image of the window and
exits (for documentation; see `--help` for `--tab`, `--input-tab` and
`--result-tab`).

## Simulating without the window

The program `csr_sim` runs simulations without a graphical interface,
for scripts and batch jobs. It reads `.csrload` and `.grtload` files and
prints one row per load and ladder step.

    csr_sim [options] load.csrload|load.grtload ...
      --profile <m>         original | published | physical (default)
      --grt-original        use the Original model for .grtload files
      --charge <g>          override the charge of the load
      --ladder-count <n>    override the number of ladder steps
      --ladder-step <g>     override the charge difference of the ladder
      --format <f>          table (default) | csv | json
      --out <file>          write the results to a file
      --curve <file.csv>    pressure/velocity curve of the focused ladder step;
                            {name} is replaced by the load's file name
      --steps <n>           position steps of the solver (default 2000)

The program’s project data (physical burn values of the powders and the
factors of the pistol and revolver calibers) are built in, so the
results equal those of the program. Each row holds the charge, maximum
and muzzle pressure, muzzle velocity, energy and time, burnt fraction,
efficiency, burnout in the barrel, load ratio and the status. Units are
g, mm, bar, m/s, J and ms, whatever the unit settings of the program.
The exit code is 0 when all simulations succeeded, 1 when at least one
failed (for example because the charge does not fit into the case) and 2
for a usage or file error.
