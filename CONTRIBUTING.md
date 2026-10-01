# Contributing

## Records for the project database

The project database (`data/`) is shared by every user of GRT Reloaded, so new calibers, projectiles and
propellants are very welcome. How to create a record file is described in [data/README.md](data/README.md).

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
[CC BY-SA 4.0](data/LICENSE.md).

## Code

Pull requests for the program are welcome as well. Please keep to the style of the surrounding code (C++20, Qt 6),
add tests for new behavior (`tests/`), and run `ctest` before submitting.
