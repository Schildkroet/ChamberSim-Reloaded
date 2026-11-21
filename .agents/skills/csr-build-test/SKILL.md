---
name: csr-build-test
description: "Use when building ChamberSim Reloaded, running or adding tests (tst_core, tst_gui, projectdata, sim_cli), running the app headless or taking screenshots, or using the csr_sim command-line simulator."
---

# Build, test and run

Commands in full: `docs/CHEATSHEET.md`.

## Build

```bash
tools/build.sh [debug|release]            # configure + build + ctest, all cores; extra args go to cmake --build
tools/build.sh debug --target tst_gui
cmake --preset debug && cmake --build --preset debug --parallel $(nproc)
```

- Presets: `debug`, `release` (`CMakePresets.json`, Ninja). Tools such as `grt_compare`, `csr_eval`, `csr_fitpowder`
  are used as Release builds (`build/release/...`): the analysis scripts run them thousands of times.
- The build runs clang-format on `src/`, `tests/`, `tools/*.cpp` first (`-DCSR_FORMAT_ON_BUILD=OFF` skips it). A
  diff that contains only formatting of code you did not touch: revert it.
- Must compile with Qt 6.4.2 (see AGENTS.md); `-DCSR_UNITY_BUILD=ON` speeds up full builds.
- New source files go into the source list of `src/core/CMakeLists.txt` or `src/app/CMakeLists.txt` (relative paths).

## Tests

```bash
ctest --preset debug                                    # core, gui, projectdata, sim_cli, sim_cli_missing_file
ctest --preset debug -R gui --output-on-failure
QT_QPA_PLATFORM=offscreen build/debug/tests/tst_gui someTestFunction
build/debug/tests/tst_core -functions                   # list the test functions
CSR_SHOT_DIR=<dir> QT_QPA_PLATFORM=offscreen build/debug/tests/tst_gui   # screenshots of the GUI tests
```

- Core logic (solver, files, units, reports, data) → a private slot in `tests/tst_core.cpp`; widgets and dialogs →
  `tests/tst_gui.cpp` (offscreen; ctest sets `QT_QPA_PLATFORM`). No new test executables per feature.
- Tests isolate `QSettings` in a temp dir; a new test must not read or write the user's settings or files.
- Sample loads used by tests go in `tests/fixtures/` (tracked). `tests/data` is local only: CI does not have it.
- A solver test asserts a tolerance band around a value you can explain, not the current output to the last digit.
- CI (`.github/workflows/build.yml`) builds Release with Qt 6.5.3 and runs the whole ctest on Ubuntu.

## Run the app

```bash
build/debug/ChamberSimReloaded [load.csrload ...]
build/debug/ChamberSimReloaded --calibration physical --language de_DE --theme dark --units imperial
# headless screenshot; isolated settings so the user's are not touched
XDG_CONFIG_HOME=<scratch dir> QT_QPA_PLATFORM=offscreen build/debug/ChamberSimReloaded \
    --screenshot out.png --tab 1 --result-tab 0 --input-tab 2 tests/fixtures/8x57is.grtload
```

`--tab` 0 = database, 1.. = loads; `--result-tab` 0 results, 1 measurements, 2 report; `--input-tab` 0 cartridge &
gun, 1 projectile, 2 propellant. Look at the screenshot after a UI change; check a dark theme and a small window too.

## Headless simulation

```bash
build/release/csr_sim [--profile original|published|physical] [--charge g] [--format table|csv|json] load...
```

Exit code 0 ok, 1 simulation failed, 2 usage/read error. Units: g, mm, bar, m/s, J, ms.
