# PREREG R1-REINIT — does re-initialising the filter at its first multilateration solution rescue the unknown-start acquisition? (exploratory grade)

**Registered before any cell runs** (rule 13). Written 2026-10-16. Engine: this commit
(`--acquireReinit`, G133), built on `80b22882` (S2-C). This commit adds this file,
`scripts/r1_reinit_run.sh` and `scripts/r1_reinit_analyse.py`. Outputs go outside the repository.
Provenance as `PREREG_S2C_ACQ`: a frozen copy of the binary with its SHA-256 manifest checked before
launch, and one compile stamp across all runs.

Context: `HANDOFF_2026_10_09_S2C.md` R1 and TECH_DEBT N.54. Also stage "R1" of the owner's master
plan (2026-10-16, kept outside the repository with the thesis material), which requires an honest
unknown-start acquisition before any switching rule is evaluated in the search scenario.

## 1. What is disclosed
- S2-C found, under `--unknownStart` at 5 km, every arm at about 0.2 within 25 m and a median error
  of about 300 m. `--acquireMinAnchors=3` did not help (S2-C-ACQ).
- The hypothesis is that the first range update is linearised at a box-centre prior kilometres off.
- No cell of this test has been run. G133's 600 s, seed-7 runs exercise the flag for the gate only;
  they are not read for any prediction below.
- **Design change disclosed:** the first G133 run showed one node re-initialising at a solution with
  weighted residual 708 952 (x = −6233 m), i.e. a solution its own ranges contradict. A residual guard
  was added before registration: a solution is accepted only if its weighted residual is within
  χ²(n − 2, 0.999) (Wilson–Hilferty); otherwise the node stays armed for its next ≥ 3-range cycle
  and an ACQREINIT_REFUSED line is printed. No prediction below was changed by that run.

## 2. Cells (`scripts/r1_reinit_run.sh`)
Common flags (as `PREREG_S2C`'s U cells): `--arms=ca_tbl_full,fixed_threshold,static_2p4
--fixedVarThreshold=12 --unknownStart=true --specProfile=true --duration=1800 --trace=true
--traceCovariance=true --traceSubjectConsistency=true --imuQuScale=10`.
- **A** is 5 km (no box flag). **B** is the endgame (`--boxHalf=750 --finderMode=1`).

| cell | extra flag | config | seeds |
|---|---|---|---|
| U0_A, U0_B | none (control, this binary) | A, B | 1-20 |
| U1_A, U1_B | `--acquireReinit=true` | A, B | 1-20 |

That is 80 runs. The control is run on this binary (paired within one binary, rule 8 and
TECH_DEBT 1.2); S2-C's tracked U cells are not reused.

## 3. Metrics and predictions
`s2c_analyse.seed_metrics` (five nodes: tag 0 and subjects 2-5), paired U1 − U0 over seeds 1-20, as
mean [95 % bootstrap], per arm and config.

| # | prediction | confidence |
|---|---|---|
| **R1a** | A, `ca_tbl_full`: five-node w25, interval above 0 | medium |
| **R1b** | A, `static_2p4`: five-node w25, interval above 0 | medium |
| **R1c** | A, all three arms: five-node median error, interval below 0 | medium |
| **R1d** | A and B, all arms: first-fix time, interval contains 0 (re-initialisation does not delay the first fix) | high |
| **R1e** | B, all arms: w25, interval above −0.05 (the endgame is not made worse) | medium |
| **R1f** | A, `ca_tbl_full` − `fixed_threshold` under U1: w25 interval contains 0 (re-initialisation does not create a CA-TBL advantage; S2-C P5's null persists) | low |

A prediction is reported as held, not held, or not evaluable (fewer than 20 paired seeds).

## 4. Abort conditions
The cause is fixed and no cell is dropped:
- a run without an `rc`, or exiting non-zero;
- a `run_id.txt` without `dirty 0`, or a compile stamp differing from the other runs';
- the binary's SHA-256 differing from the manifest written at launch;
- fewer seeds than registered.
