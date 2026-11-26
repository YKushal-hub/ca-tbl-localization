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

## Results (recorded 2026-11-22; hypotheses unchanged)

### (a) Pipeline

60 runs, all exit 0, binary `6bd87e333` (`results/collapse5/collapse5_pipe_analysis.md`).

| Hypothesis | Verdict | Detail (5 storeys / 8 storeys) |
|---|---|---|
| H17 (ToF rubble correction) | **holds, 2 of 2** | L2 0.85 vs 0.78; 0.70 vs 0.62. The single exploratory seed (DD 49) did not show this; five seeds do |
| H18 (always-on) | **holds, 2 of 2** | always-on is 0.04 / 0.05 better in L2, at 13× / 12× the energy (469 / 489 J vs 35 / 42 J) |
| H19 (each radio alone) | **holds, 2 of 2** | ours 0.78 / 0.62; best single radio (868 MHz) 0.78 / 0.66; 2.4 GHz 0.31 / 0.14; UWB 0.00 / 0.00 |
| H20 (stations) | **not tested as registered** | a zero-station deployment is outside the CA-TBL architecture (the base station runs the session behind the gateway backhaul, docs/SPEC_CONFORMANCE_PLAN.md l. 28, 755). Registration error: `--nGateways=0` does not remove the stations. It keeps the default 3 × 3 grid of 9, one of them over the pile (documented flag behaviour, missed when registering). The 9-grid gives 0.70 / 0.38. Two stations (opposite on the ring, collinear through the pile) give **0.15 / 0.09**, against 0.78 / 0.62 with four: two stations leave a mirror ambiguity. A finders-only run cannot be expressed with the current flags |
| H21 (crew of 10) | **1 of 2** | 0.82 vs 0.78 holds; at 8 storeys 0.50 vs 0.62 fails (−0.12) |
| H22 (Mueller 2.4 GHz error) | **holds** | ours 0.51 vs 2.4 GHz alone 0.09. Under Mueller, ours loses 0.16 / 0.11 against the engine model |

### (b) Link field

30 re-estimated COLLAPSE-2 link fields, plus 20 new 1.5 m-only field runs. All exit 0
(`results/collapse5/collapse5_field_analysis.md`). Coverage 0.5, local frame.

| Hypothesis | Verdict | Detail (low / central bracket) |
|---|---|---|
| H23 (published-type baselines) | **holds, 4 of 4** | joint within 2 m 0.48 / 0.45; LoRaAid-style RSS trilateration 0.06 / 0.03 (median 16 / 43 m); SPAWN-type 0.05 / 0.04 (16 / 42 m). Neither baseline models the rubble's excess loss, so its ranges read long |
| H24 (coop vs SPAWN-type) | **holds, 2 of 2** | 0.53 / 0.47 vs 0.05 / 0.04 |
| H25 (D-optimal moves) | **holds, 2 of 2** | coop 0.50 / 0.46 with D-optimal moves vs 0.53 / 0.47 with serpentine moves: no gain, no loss beyond 0.05 |
| H26 (via others) | descriptive | at half coverage every worker in the low and central brackets has a direct rescuer link, so none is placed only through others. In the high bracket, with D-optimal moves: 1% of workers, median 21.9 m |
| H27 (height spread) | **1 of 2** | joint median vertical error, with spread vs without: 2.57 vs 2.60 m (low, holds); 2.79 vs 2.42 m (central, fails). A 1.2 m spread in hold height does not measurably improve depth from the link field |

### (c) Dig decision

`results/collapse5/collapse5_dig_analysis.md`.

**H28 fails: 62 of 108 combinations.**
- **Where it loses:** the rule starts digging at 6–29 min. In the high bracket and at 8 storeys central it loses up
  to 0.89 expected survivors.
- **Why:** the workers' true errors keep falling after the windowed fall of the system's own σ stops paying for the
  teams' time, so the rule stops too early.
- **Where it holds:** within 0.1 in the low brackets (34 of 36).
- **Consequence:** ranging for the whole first half hour is the better default. The rule is reported as a negative
  result.
