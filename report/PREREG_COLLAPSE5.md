# COLLAPSE-5 — pre-registration (committed before the first cell)

**Question.** What remains of the plan's experiment list (docs/PLAN_COLLAPSE.md, "Experiments")?
- the designs not yet run with the battery-fixed pipeline: each radio alone, always-on, and the ToF rubble
  correction (DD 49);
- COLLAPSE-2's remaining factors: stations 0 and 2, a crew of 10, the field 2.4 GHz error (Mueller 2021);
- the link-field comparison with published-type baselines (DD 51), planned moves, worker-via-worker location and
  rescuer height spread;
- the "range more or dig now" rule (B6).

Failures are reported as failures. Nothing is tuned after a cell runs.

## (a) Pipeline (new runs; `scripts/collapse5_run.sh <outdir> pipe`)

COLLAPSE-4's `gate` flags (report/PREREG_COLLAPSE4.md: COLLAPSE-1's set plus `--uwbFutilityCycles=4
--collapseDepthSolve=true`). Cells:
- 5 and 8 storeys, central bracket;
- seeds 821–825 (new; never used before), 1800 s.

| Config | Change from `gate` | Arms |
|---|---|---|
| `g` | none, N = 20, 4 stations | `ca_tbl_full,static_868,static_2p4,static_uwb,poucet_fusion` |
| `rubble` | `--tofRubbleCorrect=true` | `ca_tbl_full` |
| `muller` | `--tofErrorModel=muller` | `ca_tbl_full,static_2p4` |
| `st0` / `st2` | `--nGateways=0` / `2` (same ring) | `ca_tbl_full` |
| `n10` | N = 10 | `ca_tbl_full` |

60 runs. The binary is frozen at launch (manifest checked before every cell).

## (b) Link field (`scripts/collapse5_run.sh <outdir> field`)

**Re-estimation of COLLAPSE-2's link fields.**
- Data: `~/catbl_work/campaigns/collapse2_field`, 8 storeys, low / central / high, seeds 901–910; copied, not
  modified.
- Settings: coverage 0.5, local frame.
- Runs:
  - `serp`: serpentine moves, methods `tof,rss,joint,coop,rsstri,spawn`;
  - `dopt`: D-optimal moves (`--moves dopt`), methods `joint,coop`.
- Disclosure: joint and coop were seen on these seeds in COLLAPSE-2. Of the new methods, only `rsstri` and
  `spawn` on central seed 901 were run once, while the code was being checked (within 2 m 0.05 and 0.00).

**Height spread (new field runs).**
- Runs: central and low brackets, seeds 901–910, `--collapseHoldHeights=1.5` (no spread), estimated as `serp`.
- Comparison: paired with the 0.3 / 1.5 m runs above.

## (c) Dig decision (post-processing of COLLAPSE-1, no new runs)

Disclosure: the rule was run once on seed 801 while the code was being checked (DD 51).

`score_dig.py`'s range-or-dig rule (DD 51) against digging at the run's end.
- Data: N = 20, `ca_tbl_full`, every storey count and bracket, seeds 801–810.
- Sweep: teams 1 / 2, t_dig 0.5 / 2 / 8 h, radius 1 / 2 m.

## Hypotheses

Measures as COLLAPSE-1. L2 is the mean over seeds; energy is the median worker's total joules, as the mean over
seeds.

- **H17 (ToF rubble correction, DD 49):** `rubble` L2 ≥ `g` L2 + 0.05 in both cells. This is the claim of a
  gain; the single exploratory seed suggested none.
- **H18 (always-on):** in both cells, `ca_tbl_full` L2 ≥ `poucet_fusion` L2 − 0.05, and its energy is ≤ 0.5 ×
  `poucet_fusion`'s.
- **H19 (each radio alone):** `ca_tbl_full` L2 ≥ the best single radio's L2 − 0.10 in both cells.
- **H20 (stations):** L2 does not rise as stations are removed: `st0` ≤ `st2` + 0.05, and `st2` ≤ `g` + 0.05, in
  both cells. The values are reported.
- **H21 (crew of 10):** |`n10` L2 − `g` L2| ≤ 0.10 in both cells.
- **H22 (field 2.4 GHz error):** under `muller`, `ca_tbl_full` L2 ≥ `static_2p4` L2 − 0.05 at 8 storeys
  (COLLAPSE-1's direction under the engine model).
- **H23 (published-type baselines):** `joint` within 2 m ≥ `rsstri`'s and ≥ `spawn`'s in the low and central
  brackets.
- **H24 (cooperation vs SPAWN-type):** `coop` within 2 m ≥ `spawn`'s in the low and central brackets.
- **H25 (planned moves):** `coop` within 2 m with `dopt` ≥ with `serp` − 0.05 in the low and central brackets.
- **H26 (via others, descriptive):** the fraction of workers placed only through other workers (`coop_via`) and
  their median horizontal error, per bracket. No threshold.
- **H27 (height spread):** `joint`'s median vertical error with 0.3 / 1.5 m hold heights ≤ its error with 1.5 m
  only, at the central and low brackets.
- **H28 (range or dig):** the rule's expected survivors ≥ digging at the run's end − 0.1 in ≥ 90% of the
  combinations (storeys × bracket × teams × t_dig × radius, mean over seeds).

Analyser: `scripts/collapse5_analyse.py <outdir>` (committed with this file).
