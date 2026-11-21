---
name: csr-pull-request
description: "Use when committing to ChamberSim Reloaded, writing release notes, or opening or updating a pull request: what to run first, the title and body, screenshots, and what never goes in a commit."
---

# Commits and pull requests

## Before you commit

- `tools/build.sh debug` passes (build + full ctest), including `projectdata` for data changes.
- Changed strings: `update_translations` run and the `.ts` files included (csr-ui-text skill).
- Solver change: accuracy per class (rifle / pistol / shotgun) in the message or PR (csr-solver-accuracy skill).
- `git status`: no build output, no `*.db`, `reference/`, GRT job files, sample CSVs or private `*.csrload`/`*.grtload`
  (only `tests/fixtures/` and `data/measurements/` load files are tracked). No unrelated reformatting.
- One topic per commit. Don't push, tag, create a release or open a PR unless you were asked to.

## Commit message / PR title

Form: `Area: what the user now gets, in plain words` — present tense, behaviour not mechanism, under ~80 characters.
Areas people recognise: Solver, Physical Model, Database, Measurements, Report, Chart, Input panels, Translations,
Data, CI, Docs. Examples from the history:

- `Input panels: tooltips that describe each field`
- `Translations: Spanish, Swedish and Norwegian (Bokmål); refresh the others`

No `feat:`/`fix:` prefixes, no emoji. The body says why, and for a fix what was wrong.

## Release notes

`RELEASE_NOTES.md` has a section per version (`## 0.19`, `### Added and changed`, `### Fixed`). User-facing changes get
a bullet written for handloaders (what changes in their results), solver changes with the per-class figures. Edit
only what is there; text that was removed stays removed. The version itself is `set(CSR_VERSION ...)` in
`CMakeLists.txt`; releases are tagged `vX.Y` and CI builds the AppImage and Windows zip.

## The pull request

- Fill `.github/pull_request_template.md`: what it is and the problem it solves, how you validated it (commands and
  results), screenshots for anything visible (headless `--screenshot`, see csr-build-test).
- Data PRs name the source of every record and state that nothing comes from another program's database.
- Write the body in your own words: no chat transcripts. Check pictures and logs for home folders, user names and
  private loads.
