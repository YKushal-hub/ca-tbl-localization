# PREREG S2-C-ACQ — does an acquisition rule rescue the unknown-start filter at 5 km? (exploratory grade)

**Registered before any cell runs** (rule 13). Written 2026-10-09. Engine: `c5566a27`
(`--acquireMinAnchors`, G132). This commit adds this file, `scripts/s2c_acq_run.sh` and
`scripts/s2c_acq_analyse.py`. Outputs go outside the repository. Provenance as `PREREG_S2C_NLOS`:
the build manifest `build_s2c_acq.sha256`, checked before launch, and one compile stamp.

## 1. What is disclosed

This test is motivated by an exploratory read of the registered S2-C campaign's U_A cells (11 seeds)
while that campaign ran. The read changed none of `PREREG_S2C`'s predictions, analysis or cells.

At 5 km under `--unknownStart`:
- `ca_tbl_full`'s first fixes were two-range solves in 31 of 47 node-runs, with a median error at
  first fix of 611 m;
- five-node within 25 m was 0.18;
- static 2.4 GHz's three- and four-anchor fixes had median errors of 328-333 m after a wrong
  acquisition.

## 2. Cells (`scripts/s2c_acq_run.sh`)

Flags as `PREREG_S2C` U_A (A, 5 km; `--unknownStart=true --specProfile=true --duration=1800
--trace=true --traceCovariance=true --traceSubjectConsistency=true --imuQuScale=10`), arms
`ca_tbl_full,static_2p4`:

| cell | extra flag | seeds |
|---|---|---|
| R (reuse check) | none | 1-2 |
| Q3 | `--acquireMinAnchors=3` | 1-10 |

That is 12 runs.
- **Control:** S2-C's U_A cells (seeds 1-10, same flags, the `a6b2414d` build).
- **Reuse condition:** R's `ca_tbl_full` and `static_2p4` files must be byte-identical to U_A's
  (subject traces included) on both seeds. If not, no Q3 − U_A difference is reported.

## 3. Metrics and predictions

`s2c_analyse.seed_metrics`, paired Q3 − U_A over seeds 1-10, as mean [95 % bootstrap].

| # | prediction | confidence |
|---|---|---|
| **S1** | `ca_tbl_full`: five-node w25, interval above 0 | medium |
| **S2** | `ca_tbl_full`: five-node median error, interval below 0 | medium |
| **S3** | `ca_tbl_full`: first-fix time, mean ≥ 0 (acquisition waits for three ranges) | high |
| **S4** | static 2.4 GHz: share of node-runs never fixing, mean ≥ U_A's | high |
| **S5** | R reuse check holds on 4 of 4 seed-arm pairs | high |

## 4. Abort conditions

The cause is fixed and no cell is dropped:
- a run without an `rc`, or exiting non-zero;
- a `run_id.txt` without `dirty 0`, or with a compile stamp differing from the other runs';
- fewer seeds than registered.
