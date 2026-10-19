# 📋 Cheat sheet

The commands for building, testing, running, data work and recalibration. Run them from the repository root.
`build/release/...` needs a Release build (`tools/build.sh release`), the calibration scripts need Python with numpy and
scipy (`python3 -m venv .venv && .venv/bin/pip install numpy scipy`).

## 🛠️ Build and test

```bash
tools/build.sh                       # debug: configure + build (all cores) + all tests
tools/build.sh release               # the same as Release (needed for grt_compare & co.)
tools/build.sh debug --target tst_gui   # extra arguments go to cmake --build

cmake --preset debug && cmake --build --preset debug --parallel $(nproc)
ctest --preset debug                 # tst_core, tst_gui (offscreen), projectdata (csr_datacheck)
ctest --preset debug -R gui --output-on-failure

# one test function, with screenshots of the GUI tests into a folder
QT_QPA_PLATFORM=offscreen build/debug/tests/tst_gui chartCursorShowsTheValues
CSR_SHOT_DIR=/tmp/shots QT_QPA_PLATFORM=offscreen build/debug/tests/tst_gui
build/debug/tests/tst_core -functions          # list the test functions
```

Options: `-DCSR_FORMAT_ON_BUILD=OFF` (no clang-format before the build), `-DCSR_UNITY_BUILD=ON` (faster full builds).

## 🌍 Translations

```bash
cmake --build --preset debug --target update_translations    # after changing user-visible strings
linguist translations/ChamberSimReloaded_de_DE.ts                    # translate (de_DE, fr_FR, it_IT; en_US = plural forms only)
```

## ▶️ Run the app

```bash
build/debug/ChamberSimReloaded [load.csrload ...]
build/debug/ChamberSimReloaded --help
build/debug/ChamberSimReloaded -d tools/GordonsReloadingTool.plain.db         # with the original's (plain) database
build/debug/ChamberSimReloaded --calibration original|published|physical      # solver model for this run
build/debug/ChamberSimReloaded --language de_DE --theme dark --units imperial --log-level debug
build/debug/ChamberSimReloaded -r reference/results/exp_base.json load.csrload  # overlay a reference run

# headless screenshot, without touching your settings
XDG_CONFIG_HOME=/tmp/grtcfg QT_QPA_PLATFORM=offscreen build/debug/ChamberSimReloaded \
    -d tools/GordonsReloadingTool.plain.db --screenshot out.png --tab 1 --result-tab 0 --input-tab 2 tests/data/8x57is.grtload
```

`--tab`: main tab (0 = database), `--result-tab`: 0 results, 1 measurements, 2 report; `--input-tab`: 0 cartridge & gun,
1 projectile, 2 propellant.

## 🗃️ Project data (`data/`)

```bash
build/debug/csr_datacheck data                 # check every record file (what CI runs)
python3 scripts/data/recordfiles.py data/projectiles  # merge exported single records into the manufacturers' files
python3 scripts/data/recordfiles.py data/propellants

# projectiles from the manufacturers' sites
python3 scripts/data/bulletdata/<maker>_fetch.py -o scripts/data/bulletdata/bullets_<maker>.csv [--cache dir]
python3 scripts/data/bulletdata/bullets_to_json.py scripts/data/bulletdata/bullets_*.csv [--overwrite]
python3 scripts/data/bulletdata/start_pressure.py [--write] [--overwrite]   # initial gas pressure gpressure by construction

# calibers (C.I.P. sheets, Wikipedia): see scripts/data/cipdata/README.md
python3 scripts/data/cipdata/cip_to_json.py scripts/data/cipdata/cip_calibers.csv scripts/data/cipdata/wiki_calibers.csv
```

## 🧪 Published load data (samples for the powder fits)

The sample CSVs are local only (not in the repository); make them again from the manuals:

```bash
cd scripts/data/loaddata
python3 hodgdon_extract.py manual.pdf -o hod_samples.csv [--pressure psi|all]
python3 west_extract.py guide.pdf -o west_samples.csv
python3 lovex_extract.py guide.pdf -o lovex_samples.csv          # needs pdftotext
python3 rs_extract.py -o rs_samples.csv [--html page.html]
python3 vv_extract.py N150 -o vv_N150.csv [--cache dir]
python3 hodgdon_shot_extract.py manual.pdf -o hodgdon_shot_samples.csv
python3 alliant_shot_extract.py guide.pdf -o alliant_shot_samples.csv
python3 adi_extract.py -o adi_samples.csv --cache adi_pages                    # downloads (acknowledges ADI's warnings page)
python3 vectan_extract.py catalogue.pdf -o vectan_samples.csv --ocr-cache ocr.json   # needs rapidocr-onnxruntime (OCR)
```

## 🔎 Original model against published pressures

```bash
# loads of Hodgdon / Lovex / Reload Swiss / ADI whose caliber, projectile and powder exist in the ORIGINAL's database, simulated with GRT Original
python3 analysis/compare_grt_published.py build/release/csr_dbeval [--lots best|all] [--profile original|published] [--set name=value] [--top 6]
# -> reference/grt_vs_published/grt_vs_published.csv (local only), statistics per class and publisher, the largest deviations in both directions
build/release/csr_dbeval --db tools/GordonsReloadingTool.plain.db jobs.csv   # the simulator behind it: records of the original by id
```

## ⌨️ Simulate from the command line

```bash
# one row per load and ladder step (units: g, mm, bar, m/s, J, ms); exit code 0 ok, 1 simulation failed, 2 usage/read error
build/release/csr_sim [--profile original|published|physical] [--grt-original] [--charge g] \
    [--ladder-count n --ladder-step g] [--format table|csv|json] [--out file] [--steps n] a.csrload b.grtload ...
build/release/csr_sim --format json load.csrload                  # for scripts
build/release/csr_sim --curve 'curves/{name}.csv' *.csrload       # p/v/e/z along the barrel (focused ladder step)
```

## 🔥 Powders: fit and write records

```bash
# own measurements (Measurements tab of the loads) -> a record file
build/release/csr_fitpowder [--model original|physical] [--out maker-name-lot.json] [--save] a.csrload b.csrload ...

# published data -> burn rate of one powder
build/release/csr_fitdata vv_N150.csv --params Ba [--use-pressure] [--holdout "308 Win."] [--out record.json]

# published data -> records (Ba per powder, form function 0 / 0.3 / 0.6)
python3 scripts/data/loaddata/published_fit.py lovex_samples.csv --maker Lovex --base-from-name --source "..." --write
python3 scripts/data/loaddata/vv_fit_all.py -o vv_fits.csv --fitdata build/release/csr_fitdata
python3 scripts/data/loaddata/vv_to_json.py vv_fits.csv --work vv_work

# physical burn values of all project powders -> data/physical/propellants.json
.venv/bin/python analysis/fit_physical_powders.py build/release/csr_eval \
    hod_samples.csv west_samples.csv lovex_samples.csv rs_samples.csv vv_*.csv [--split train] [--fit form|ba]
# only new records (Ba only, the other powders of the file stay): add --fit ba --form a0,z1,z2 --only N110,N120 --merge
# burn rate factor of the pistol / revolver calibers -> data/physical/calibers.json  (--holdout: fit on half of the powders, test on the rest;
# --rifles --keep-existing --min-samples 60 adds the rifle calibers and keeps the existing entries)
.venv/bin/python analysis/fit_caliber_scales.py build/release/csr_eval hod_samples.csv west_samples.csv adi_pistol_samples.csv [--holdout] [--shrink 0.75]

# bullet lengths from product photos (needs opencv-python-headless in the venv)
python3 scripts/data/bulletdata/hunting_sport_fetch.py [--out DIR]          # hunting-sport.com handgun bullets: photos + manifest.csv
python3 scripts/data/bulletdata/hn_photos_fetch.py [--out DIR]              # hn-sport.de: single (einzeln) and two-bullet (doppelt) photos
.venv/bin/python scripts/data/bulletdata/measure_side_photos.py DIR         # perfect side views: length, nose / meplat / bearing / base profile
.venv/bin/python scripts/data/bulletdata/measure_standing_bullet.py DIR --kind einzeln --phi 10 --scale 1.0256   # standing bullet seen from above
.venv/bin/python scripts/data/bulletdata/measure_standing_bullet.py DIR --phi 10 --calibrate "FILE.png:20"      # factor for a bullet of known length (mm)
python3 scripts/data/bulletdata/apply_hunting_sport_lengths.py              # measured lengths -> data/projectiles (idempotent)
```

## 🧮 Solver calibration

The analysis scripts use at most 14 cores (`CSR_JOBS=n` changes it), so the rest of the system keeps some.

```bash
# against the original program's reference runs (tools/GRTRef/jobs_all + reference/results, local only)
build/release/grt_compare [--include RE] [--exclude RE] [--set name=value] [--list-params]
CSR_STEPS=2000 .venv/bin/python analysis/calibrate.py build/release/grt_compare [param ...]   # -> calibrated.json

# against published load data (Original / Published / Physical constants)
.venv/bin/python analysis/calibrate_published.py build/release/csr_eval build/release/grt_compare \
    hod_samples.csv west_samples.csv --profile physical [--free a,b,c] [--start constants.json] [--evaluate-only]
build/release/csr_eval hod_samples.csv --profile physical [--set name=value] [--phys "Maker Name=Ba,a0,z1,z2[,shape,ld,hole,amp]"]
# effective grain geometry (ball / single-perforated cylinder) of the physical form functions, then Ba-only refit keeping the form
python3 analysis/fit_grain_geometry.py build/release/csr_eval --threshold 0.08 --write
.venv/bin/python analysis/fit_physical_powders.py build/release/csr_eval hod_samples.csv west_samples.csv \
    --split all --fit ba --keep-form --start data/physical/propellants.json --merge   # --limit-pressure for the VV limits
# published loads against the shipped powder records, rifle / pistol / publisher, optionally against an older build (--baseline) or other burn values
.venv/bin/python analysis/compare_physical_published.py build/release/csr_eval [samples.csv ...] [--baseline older/csr_eval] [--eval-arg=--physical=DIR/propellants.json] [--max-pressure-dev 45] [--worst 6]
# loads whose simulated pressure is >45 % off the published one are probably invalid: copies without them (-> sources/samples_clean), or --apply
.venv/bin/python analysis/filter_published_outliers.py build/release/csr_eval [--max-dev 45] [--apply]   # --apply: remove them from the sample files and keep them in sources/samples/invalid/invalid_samples.csv
# test constants of the physical model powder by powder (parallel, held-out cartridges, trends over loading density and bullet mass)
.venv/bin/python analysis/scan_physical.py build/release/csr_eval adi_samples.csv adi_pistol.csv [--set name=value] [--fit-form] [--drop-inconsistent 0.15] [--quiet]   # a column cls in the CSV prints the figures per class (rifle, pistol, ...)

.venv/bin/python analysis/fit_shot_factors.py build/release/csr_eval hodgdon_shot_samples.csv alliant_shot_samples.csv vectan_shot_samples.csv --out data/shotshell/parts.json   # shell, wad and primer factors (seconds)
# shotshells
.venv/bin/python analysis/calibrate_shotshell.py build/release/csr_eval <data dir> hodgdon_shot_samples.csv
```

After any solver or calibration change, refresh the accuracy table in the README:

```bash
tools/build.sh release && python3 tools/update_accuracy.py build/release/grt_compare
```

## 🎯 Reference runs of the original program (VM)

```bash
python3 tools/GRTRef/make_jobs_<kind>.py       # writes tools/GRTRef/jobs/exp_*.grtload
# copy the jobs to the shared folder /home/patrick/Vorlagen/GRTRef, run the original with the plugin
# tools/GRTRef/grtref.py (com.grt.plugin.xml) on the VM, copy its results/*.json back into reference/results/
build/release/grt_compare --include '^exp_new_'  # compare
```

## 🔑 Original database

```bash
tools/grt_decrypt_db.sh        # plain copy of the original's database (Linux; .ps1 / .cmd on Windows), see the script's header
```

## 🚀 Release

```bash
# 1. set(CSR_VERSION "x.y") at the top of CMakeLists.txt, commit
git tag -a vX.Y -m "ChamberSim Reloaded X.Y"
git push origin main && git push origin vX.Y
# CI builds the AppImage and the Windows zip and creates a draft release: publish it on GitHub (Releases)
```
