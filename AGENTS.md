# ChamberSim Reloaded: instructions for agents

Qt 6 / C++20 desktop app for interior ballistics of handloads, following the workflow of *Gordon's Reloading Tool*
(reads and exports its `*.grtload` files, can use its database). This file is a router: the hard rules, how to build,
the layout, then the skill for each task. Humans: [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md), commands in
[docs/CHEATSHEET.md](docs/CHEATSHEET.md).

## Hard rules (and what enforces them)

- **Modern C++20** for application code (ranges, `std::span`, designated initializers, `[[nodiscard]]`, ...); match the
  surrounding code (comment density, naming). Scripts in `tools/`, `analysis/`, `scripts/` may stay Python/shell.
- **Qt 6 only** (Widgets, Sql, Charts, Xml, PrintSupport, Test, LinguistTools), and it must build with **Qt 6.4.2**:
  `QXmlStreamAttributes::value` needs a `QString` literal, `QChar(int)` is ambiguous, no newer APIs. CI uses 6.5.3.
- **Formatting**: the build runs `clang-format -i` (`.clang-format`: Allman, 140 columns) on `src/`, `tests/`,
  `tools/*.cpp`. Write in that style, never hand-format, never commit a reformat of untouched code.
- **Every user-visible string is translatable** (`tr()`, `QCoreApplication::translate`, `QT_TR_NOOP`, `%n` plurals),
  US English source: [.agents/skills/csr-ui-text/SKILL.md](.agents/skills/csr-ui-text/SKILL.md).
- **Nothing from Gordon's Reloading Tool's database or other programs' databases goes into `data/`**, not even fitted
  powder models; records name their source. `csr_datacheck` (ctest `projectdata`) checks the format:
  [.agents/skills/csr-project-data/SKILL.md](.agents/skills/csr-project-data/SKILL.md).
- **Never commit** the original's database, `*.db`, `reference/`, GRT job folders or sample loads: `.gitignore` lists
  them. Published load-data CSVs (`sources/`, `*_samples.csv`) are local only too.
- **The GRT Original solver profile stays unchanged**; a solver or calibration change reports accuracy **split into
  rifle, pistol (pistol + revolver, C.I.P. table IV) and shotgun**, never one pooled figure:
  [.agents/skills/csr-solver-accuracy/SKILL.md](.agents/skills/csr-solver-accuracy/SKILL.md).
- **No reading of the app's own old save formats** (e.g. `<GRTReloaded>` from 0.4); the original's `.grtload` is
  supported.
- **New behaviour gets a test** in the existing `tests/tst_core.cpp` / `tests/tst_gui.cpp`; run the full ctest before a
  commit: [.agents/skills/csr-build-test/SKILL.md](.agents/skills/csr-build-test/SKILL.md).
- **UI stays usable**: readable labels, capitalised property names, visible buttons, settings remembered in `QSettings`.
- The version lives only in `set(CSR_VERSION ...)` at the top of `CMakeLists.txt`.
- Temporary files and extra builds go outside the repository tree (a scratch dir), not into it.
- Don't push, tag or open PRs unless asked; one topic per commit/PR:
  [.agents/skills/csr-pull-request/SKILL.md](.agents/skills/csr-pull-request/SKILL.md).

## Build and test

```bash
tools/build.sh [debug|release]        # configure + build (Ninja, all cores) + ctest
cmake --preset debug && cmake --build --preset debug --parallel $(nproc)
ctest --preset debug                   # core, gui (offscreen), projectdata, sim_cli
```

## Layout

- `src/core` — static lib `csrcore`, no widgets: `loadfile` (own `.csrload` = `<ChamberSimReloaded format="2"><load>`;
  `.grtload` read, and written only as an export chosen by suffix, `formatForPath`; appendix kept verbatim),
  `chronograph` (MagnetoSpeed CSV / Labradar Report.csv), `measurement` (`<Measurement>` blocks: kg, m/s, bar),
  `simconfig` (`SimInput::fromLoad`), `solver` (profiles Original / Published / Physical, `Calibration` constants,
  ladder), `physicalpowders` (`data/physical/propellants.json`, `calibers.json`), `loadbuilder`, `database` (own
  read-write DB ids >= 10,000,000; project records from `data/*/*.json` built in as `:/data`, ids 1,000,000..; the
  original's plain DB ATTACHed read-only, ids < 1,000,000; TEMP views = UNION ALL; writes touch only the own DB),
  `projectdata`, `powderfit`, `burnfactors`, `units`, `report`/`wiki`/`reportformat`, `reference`, `language`.
- `src/app` — static lib `csrui` + executable: `MainWindow`, `LoadView`, `PropertyPanel`, charts, `MeasurementView`,
  `CartridgeView`/`ChamberView`, `ReportView`, dialogs, `theme`, `unitprefs`, `translation`, `TitleBar` (own title
  bar on Linux: use `MainWindow::mainMenu()`, not `menuBar()`).
- `tools/*.cpp` — CLIs (`csr_sim`, `csr_fitpowder`, `csr_datacheck`, `csr_eval`, `grt_compare`, ...);
  `analysis/*.py` — calibration and fits; `scripts/data` — data pipelines; `data/` — project records (CC BY-SA 4.0);
  `tests/` — `tst_core.cpp`, `tst_gui.cpp`, fixtures in `tests/fixtures`; `translations/` — `.ts` files;
  `docs/` — SOLVER, ACCURACY, DATA_SOURCES, the manual.
- Base units of the model = the file's units (mm, g, bar, ...); display units come from `UnitPreferences`
  (presets Metric/Imperial/Original plus per-field overrides, in `QSettings`).
- Settings keys: `ui/hideDisclaimer`, `ui/fontSize`, `ui/customTitleBar`, `units/...`, `solver/calibration`,
  `solver/originalForGrtFiles`, `report/template`, `report/scale`, `layout/...`, `window/geometry`, `files/recent`,
  `session/reopen`, `session/loads`, `session/current`, `database/path` (the original's database).

## Open work

Measurement tab extras (Caldwell and Labradar `.lbr` import; the report shows only the selected series), ladder
semantics, remaining tools of the original (unit calculator, shot group analysis, OBT, black powder calculator,
cartridge designer, parametric powder search, plugin API), hollow-base drawing, notes/files tabs.

## Skills: read the one for your task

Skills live in `.agents/skills/` (`.claude/skills` is a symlink to it).

| Skill | When |
|---|---|
| [`csr-build-test`](.agents/skills/csr-build-test/SKILL.md) | building, running tests, adding a test, running the app headless, screenshots |
| [`csr-ui-text`](.agents/skills/csr-ui-text/SKILL.md) | any label, message, tooltip, menu entry or translation |
| [`csr-project-data`](.agents/skills/csr-project-data/SKILL.md) | records in `data/` (calibers, projectiles, propellants), powder fits, data pipelines |
| [`csr-solver-accuracy`](.agents/skills/csr-solver-accuracy/SKILL.md) | solver, calibration constants, physical powder values, `docs/ACCURACY.md` |
| [`csr-pull-request`](.agents/skills/csr-pull-request/SKILL.md) | committing, release notes, opening a PR |
