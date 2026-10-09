# PREREG S2-C — unknown start, and the subjects' measurement weighting (exploratory grade)

**Registered before any cell runs** (rule 13). Written 2026-10-09. Engine: `a6b2414d`, the S2-C commit that
adds `--unknownStart`, `--traceSubjectConsistency` and `--measVarScale` (G130). It is built on
`main` (`4ca0dff9`) with S3 (`5a6f4c26`) and Part A (`d76b4d55`) merged.

This commit adds this file, `scripts/s2c_run.sh` and `scripts/s2c_analyse.py`. Outputs go outside
the repository; afterwards only tables, per-cell `arm_summary.csv` / `run_id.txt` and a SHA-256
manifest are tracked.

**Grade: exploratory.** One machine (cloud, 4 cores), and a 3-hour session. The measurement-weighting
part has a training/held-out split: seeds 21-30 select, seeds 1-20 evaluate.

**Provenance rule, learned from ACC-A-OPPOINT.** `run_id.txt`'s `source` line is the git HEAD of
the directory the binary runs from: this commit's worktree. The engine is pinned by:
- the binary's compile stamp (`build <date>` line), identical in every run;
- the binary's SHA-256, checked against `build_s2c.sha256` before launch.

The binary is compiled from the working tree that the S2-C engine commit records. `RUN.md` lists the
SHA-256 of its three changed source files.

## 1. Hypotheses

- **H-U (unknown start).** ACC-A's static 2.4 GHz is within 25 m on 69 % of 5 km subject-cycles
  while 42 of 80 subject-runs never fix. If that is dead reckoning from a known start, then:
  - with the filter told only the search box, static 2.4 GHz's within-25 m share collapses;
  - the hybrid's fix rate turns into a within-25 m lead.
- **H-W (weighting).** The hybrid's subject fixes leave the estimate worse 60-76 % of the time
  because the filters trust their ranges too much. If so:
  - the subjects' 2-DOF NIS exceeds χ²₂(0.99) = 9.21 far more often than 1 %;
  - inflating R (`--measVarScale`) lowers that share and improves the subjects' error.

## 2. Cells (`scripts/s2c_run.sh`)

Common flags: `--specProfile=true --duration=1800 --trace=true --traceCovariance=true
--traceSubjectConsistency=true --imuQuScale=10`.
- **A** is 5 km (no box flag).
- **B** is the endgame (`--boxHalf=750 --finderMode=1`).
- `fixed_threshold` runs at `--fixedVarThreshold=12`, the single operating point ACC-A-OPPOINT
  tested in both A and B. There it was at least as good as `ca_tbl_full` (`72c309b0`).

| stage | cells | flags | arms | seeds |
|---|---|---|---|---|
| T (training) | T_A_s{1,3,10} | `--measVarScale` 1, 3, 10 | `ca_tbl_full` | 21-30 |
| C (reuse check) | C_A, C_B | none | `ca_tbl_full,fixed_threshold,static_2p4` | 1-3 |
| U (unknown start) | U_A, U_B | `--unknownStart=true` | `ca_tbl_full,fixed_threshold,static_2p4` | 1-20 |
| H (held-out) | H_A, H_B | `--measVarScale=s*` | `ca_tbl_full,static_2p4` | 1-20 |

That is 30 + 6 + 40 + 40 = 116 runs (H is skipped if s* = 1).

**Selection rule for s\*, applied by the runner after stage T** (`s2c_analyse.py --select`):
- the statistic is the subjects' share of fix rows with NIS > 9.21, for `ca_tbl_full` (subjects
  2-5, rows with `hadFix` 1, `mahalanobis` > 0 and t ≥ 120 s), pooled over seeds 21-30;
- s\* is the smallest s in {1, 3, 10} whose share is ≤ 0.05;
- if no s reaches it, s\* is the s with the smallest share.

**Controls.**
- **Known-start controls for seeds 1-20** are ACC-A's M_k10 cells (`ca_tbl_full`, `static_2p4`)
  and ACC-A-OPPOINT's O_{A,B}_T12 cells (`fixed_threshold`).
- **They are reused only if stage C passes.** In C, every per-arm file of seeds 1-3 must be
  byte-identical to the reused run's. The exception is subject traces, which must equal it after
  the 5 appended columns are stripped.
- **If C fails,** no U-against-control or H-against-control difference is reported. Only
  comparisons within a stage are.

## 3. Metrics (`scripts/s2c_analyse.py`), per seed, then paired, as mean [95 % bootstrap]

Five nodes means tag 0 and subjects 2-5. Truth is used only here, for evaluation.

- **First fix:** the first row with `hadFix` 1, censored at 1 800 s. Reported as the five-node
  mean, plus the share of node-runs that never fix.
- **Fix rate:** availability, `subject_summary` fixCycles/steps, five-node mean.
- **Within 25 m, all cycles (w25)**, and **within 25 m after the node's first fix (w25acq)**:
  - w25 is ACC-A's metric, and under a known start it includes dead reckoning from that start;
  - w25acq counts only cycles after the node has localised by radio.
- **Error:** five-node median error over all cycles, and tag 0 CEP95 over all cycles.
- **Recovery:**
  - a loss is the first cycle above 25 m after a cycle within it;
  - recovery is the number of seconds until the error is back within 25 m;
  - reported as the five-node median per seed, over completed episodes. Episodes truncated by
    the run's end are counted separately and not timed (rule 1, censoring).
- **Energy:** J per node, `subject_summary` totalJ.
- **Consistency (subjects 2-5):**
  - NIS > 9.21 share on fix rows (`ca_tbl_full` only; the other modes do not compute it);
  - ANEES (mean NEES over fix rows, t ≥ 120 s, against χ²₂ mean 2).

## 4. Predictions, made before any cell

**Disclosed:**
- ACC-A, ACC-A-OPPOINT, and the two diagnoses (`ACC_A_STATIC2P4_DIAGNOSIS`,
  `ACC_A_DECISION_DIAGNOSIS`);
- the G130 runs, seed 7, 300 s, endgame. There, unknown start put three of five nodes more than
  100 m off at the first row, and nothing else was read.

| # | prediction | confidence |
|---|---|---|
| **P1** | U_A: w25, `ca_tbl_full` − `static_2p4`, interval above 0 (the 5 km loss reverses) | medium |
| **P2** | U_A: share of node-runs never fixing — `static_2p4` ≥ 0.40 and `ca_tbl_full` ≤ 0.15 | medium |
| **P3** | U_A and U_B: five-node first-fix time, `ca_tbl_full` − `static_2p4`, interval below 0 | high |
| **P4** | U_A and U_B: w25acq, `ca_tbl_full` U − C (reused), interval within ±0.05 | low |
| **P5** | U_A and U_B: w25 and availability, `ca_tbl_full` − `fixed_threshold`@12, intervals contain 0 | medium |
| **P6** | stage C byte-identity holds on all 18 seed-arm pairs | high |
| **P7** | T at s = 1: subjects' NIS > 9.21 share > 0.05 | medium |
| **P8** | T: that share decreases strictly from s = 1 to 3 to 10 | high |
| **P9** | H_A: `ca_tbl_full` w25, s\* − s = 1, interval above 0 | low |
| **P10** | H_B: `ca_tbl_full` tag 0 CEP95, s\* − s = 1, mean ≤ +10 % of the s = 1 mean | medium |
| **P11** | T (training seeds; it describes the selection, not a held-out test): subjects' \|ANEES − 2\| for `ca_tbl_full`, smaller at s\* than at s = 1 (mean over seeds). ACC-A's subject traces carry no covariance, so the held-out s = 1 side does not exist. | low |

## 5. Abort conditions

The cause is fixed and no cell is dropped:
- a run without an `rc`, or exiting non-zero;
- a `run_id.txt` without `dirty 0`, or whose `build` stamp differs from the other runs';
- a cell with fewer seeds than registered;
- a prediction that cannot be evaluated.

A P6 failure is reported as a leak, and the fallback in §2 applies.
