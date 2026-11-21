<!-- Title: "Area: what a user now gets", e.g. "Input panels: tooltips that describe each field". No feat:/fix:, no emoji. -->

## Change

What this is and the problem it solves, in plain words for someone who has not seen the code or your chat.
Link the issue when there is one (`Fixes #N`).

## Validation

- [ ] `tools/build.sh debug` (build + ctest) passes
- [ ] New behaviour has a test in `tests/tst_core.cpp` or `tests/tst_gui.cpp`
- [ ] Changed UI strings: `update_translations` run, `.ts` files included
- [ ] Solver/calibration change: velocity and peak pressure, before and after, **per class (rifle / pistol / shotgun)**

What you ran and what came out.

## Data (only for changes in `data/`)

- Source of every record (manufacturer data sheet, C.I.P./SAAMI, own measurement):
- [ ] Nothing is taken from Gordon's Reloading Tool's database or another program's database (including powder models)
- [ ] I agree that the records are published under CC BY-SA 4.0

## Screenshots (for anything visible)

Before and after, from a small public example (`--screenshot`, see `docs/CHEATSHEET.md`). No private loads, home
folders or user names.
