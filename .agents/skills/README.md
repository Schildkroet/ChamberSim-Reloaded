# Skills for working on ChamberSim Reloaded

Task instructions for contributors' coding assistants, one folder per skill in the
[Agent Skills](https://agentskills.io) format (`SKILL.md` with a `name` and a `description` that says when to load it).
`AGENTS.md` at the repository root lists the hard rules and routes to these. `.claude/skills` is a symlink to this
folder (on Windows enable symlinks: Developer Mode and `git config core.symlinks true`).

| Skill | Loads when the agent |
|---|---|
| [`csr-build-test`](csr-build-test/SKILL.md) | builds, runs or adds tests, runs the app headless, takes screenshots, uses `csr_sim` |
| [`csr-ui-text`](csr-ui-text/SKILL.md) | adds or changes user-visible text or a translation |
| [`csr-project-data`](csr-project-data/SKILL.md) | adds records to `data/`, fits a powder, works on `scripts/data` |
| [`csr-solver-accuracy`](csr-solver-accuracy/SKILL.md) | changes the solver, its constants or the fitted powder values; reports accuracy |
| [`csr-pull-request`](csr-pull-request/SKILL.md) | commits, writes release notes, opens a pull request |

Keep each skill short and specific to this repository; the long explanations stay in `docs/`. A new skill gets a row
here and in `AGENTS.md`.
