---
name: csr-solver-accuracy
description: "Use when changing the interior-ballistics solver (src/core/solver*), its Calibration constants, the physical powder or caliber values, the analysis/*.py calibration scripts, or docs/ACCURACY.md / docs/SOLVER.md."
---

# Solver, calibration and accuracy

Long form: `docs/SOLVER.md` (models and what was tried), `docs/ACCURACY.md` (current figures).

## The three profiles

- **Original** (`--profile original`): constants fitted to runs of Gordon's Reloading Tool. **Keep it unchanged**: its
  purpose is to give the original's results. It is not the original's exact algorithm (~0.7 % muzzle velocity,
  ~1.6 % peak pressure against the reference runs). Mechanisms it reproduces: ignition ramp
  `1 - exp(-ignitionRate*(step+lead))` on the energy reaching the gas, covolume `c0 + c1/pc` (eta ignored), solid
  powder `s/pc`. Known gaps: charge-mass and powder-density trends in peak pressure, burnt fraction at the muzzle.
- **Published** (GRT Tuned): the original's equations, constants fitted to published load tables.
- **Physical** (default, `SolverModel::Physical`): own time-integrated model; separate constant sets for rifle and for
  handgun (`...Handgun`); per-powder burn values in `data/physical/propellants.json`, per-caliber burn factor for
  pistol/revolver cartridges in `data/physical/calibers.json` (`SimInput::caliberBurnScale`).

## Reporting accuracy

- **Always split into rifle, pistol and shotgun**, never one pooled number. Pistol = pistol and revolver cartridges
  (C.I.P. table IV, `cippdf` contains `/tab-iv/`), rifle = the rest, shotgun = the shotshell loads. Pistol is by far
  the least accurate class; a change can help one class and hurt another.
- Give velocity and peak pressure rms (held-out where possible) and the bias, before and after.
- `analysis/scan_physical.py` prints per class when the CSV has a `cls` column;
  `analysis/compare_physical_published.py --baseline <older csr_eval>` compares two builds.
- Many knobs trade velocity against pressure: report both, and say what got worse.

## Running the analysis

- Release builds of `grt_compare` / `csr_eval`; Python with numpy + scipy in a venv
  (`python3 -m venv .venv && .venv/bin/pip install numpy scipy`). Scripts use at most 14 cores (`CSR_JOBS=n`).
- The inputs are local only: the reference runs of the original (`reference/results`, `tools/GRTRef/jobs_all`) and the
  published-load sample CSVs. Without them you cannot measure accuracy: say so in the PR and the maintainer runs it.
- After a solver/calibration change or new reference runs, the maintainer refreshes the table:
  `tools/build.sh release && python3 tools/update_accuracy.py build/release/grt_compare`, committed with the change.
- Refitted powder values: run the fit, compare per class, and adopt only what improves held-out figures; don't hand-edit
  the generated JSON. Document an adopted change and its numbers in `docs/SOLVER.md` and `RELEASE_NOTES.md`.
