# PREREG PRESET-2 — Amendment 1, before any amended cell runs

**What happened.** PRESET-2 (`report/PREREG_PRESET2.md`, `05e8417f`) launched 2026-10-20T11:21:02Z. Every N and R
cell exited 1 at start-up:

    FATAL: --deadBatteryNoDraw needs --batteryDepleteJ > 0 (nothing dies without it) and --silenceStopsRelay

N.20's flag acts only when a battery can run out, and no PRESET-2 cell sets a battery budget. The registered abort
condition applied ("a cell's rc not 0"). The runner was stopped, and the output directory
`~/ns/catbl-work/preset2_2026-10-20/` is kept as the record of the abort (`RUN.md`). No cell completed with rc 0, so
no outcome was seen.

**The amendment.** N and R drop `--silenceStopsRelay=true --deadBatteryNoDraw=true`, and nothing else changes. Both
flags correct the fault model: a dead node relays nothing and draws nothing (N.1, N.20). They act only under
`--batteryDepleteJ` or `--injectSilentId`, so a fault-free campaign cannot measure them.
- They are not adopted here.
- Every future fault-injection campaign sets them in its runner. That is recorded in `docs/DESIGN_DECISIONS.md` with
  this amendment's commit.
- The preset cannot carry them, because the dependency check refuses them without a battery budget.

**Checked before this commit,** with a 5 s dependency run of every configuration × variant on the amended flags
(A, B, C × S, N, R): all exit 0. These runs measure nothing.

**Unchanged:** the cells otherwise, seeds 101–120, the predictions P1–P5, the decision rules, the abort conditions
and `scripts/preset2_analyse.py`. The amended output directory is `~/ns/catbl-work/preset2_a1_2026-10-20/`, which
does not exist at this commit.
