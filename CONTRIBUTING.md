# Contributing

CA-TBL is a research simulator, and its results are only worth something if they can be reproduced and audited. The
rules below exist because each was earned by a real defect; `RULES.md` records the defects. Read it before changing
anything.

## Workflow

1. Branch from `main`: `work/YYYY-MM-DD-short-topic`.
2. Make one change per commit, so each can be reverted alone. The message states the change and carries its evidence
   (the measured number, the gate, the null control). History is not rewritten once published.
3. Open a pull request into `main`. `main` is the canonical line.

## What a change must include

| change | requirement |
|---|---|
| a new mechanism | a **default-off flag** (`cmd.AddValue` in `scratch/ca_tbl_causal_sim.cc`, with help text that argues why the default is what it is), a **null-control test** showing byte-identical output with the flag off, and a gate with a **negative fixture** |
| a measurement campaign | a pre-registration (`report/PREREG_*.md`) committed before the first cell runs: predictions, falsifiers, abort conditions; then the runner and scorer committed with it |
| a gate | its negative fixture, in the same commit, observed to fail |
| a citable run | `run_id.txt` with commit, `dirty 0`, build profile and a SHA-256 per output file |
| a documentation change | the source of every number it quotes; a statement that a thing is "not implemented" is checked against the flag registry and the sidecars first |

## Things not to do

- Do not regenerate a tracked `results/` directory in place; regenerate into a temporary directory.
- Do not edit `run_id.txt` sidecars. They are evidence, and old commit identifiers resolve through `SHA_MAP.md`.
- Do not track `trace_*.csv` (tens of GB; they regenerate from each cell's `run_id.txt`).
- Do not change the preset or the engine casually: a change moves every result in the tree. It is a planned
  campaign, not a fix.
- Do not add a co-author trailer or any tool-generated attribution to a commit, pull request or tracked file
  (`RULES.md` section 2).

## Checks before opening a pull request

```bash
./ns3 build                                  # optimized profile, see docs/INSTALL.md
bash scripts/catbl_gates.sh                  # runs every gate and summarises; expect only G11 and G93 red
bash scripts/publication_ready.sh            # manifest and citation assertions
git status --short                           # must be unchanged by the checks
```

Run gates in a worktree that is not the one a campaign is running from, and never edit a script while it runs.

## Style

C++ follows the repository's `.clang-format`; comments say why, not what, and mark any constant that is ours rather than
cited. Python scripts read their inputs from the paths they are given, write to a directory the caller names, and do not
modify tracked files. Documents state their date and are corrected, not silently edited, when a result changes.
Upstream ns-3 files are not modified except where `README.md` says so.

## Conduct and licence

See `CODE_OF_CONDUCT.md`. Contributions are licensed under GPL-2.0-only, as the project is (`LICENSE`).
