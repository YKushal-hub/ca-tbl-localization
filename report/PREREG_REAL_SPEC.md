# PREREG REAL-SPEC — the Switching-Point Integrity Check at its specified threshold, on the consistent stack, out of sample

**Registered before any cell runs** (rule 13). Written 2026-10-18. Engine: `work/2026-10-18-magnetometer` at `b218e608` (no
engine change: `--integrityChi2` is main's own flag). This commit adds this file, `scripts/real_spec_run.sh` and
`scripts/real_spec_analyse.py`, and nothing else. Outputs go outside the repository, to
`~/ns/catbl-work/real_spec_2026-10-18/`, which does not exist at registration.

**Why.** The Switching-Point Integrity Check is specified at Mahalanobis χ²₀.₉₉ = 9.21 (CLAUDE.md §2). The preset
runs it at 1992.9, an owner override (FIXES-N26) made because the old filter, overconfident, locked itself out at
the specified value — a falsely tight covariance makes honest fixes look anomalous. The realistic-world stack (S:
ekf5, magnetometer, finder consider, relay gate) makes the covariance honest (endgame coverage ~0.82, REAL-BEST).
On seeds 1–20 (exploratory, read before this commit), 9.21 on S raised CA-TBL's availability (endgame +0.008
[+0.001, +0.015]) and closed the endgame gap to `fixed_threshold` (−0.010 significant → +0.006 [−0.003, +0.015]),
accuracy unchanged. This tests that on **seeds 41–60, never run**. It adopts nothing.

## 1. Cells (`real_spec_run.sh`)

The realistic world under S in every cell: `--specProfile=true --duration=1800 --trace=true --traceCovariance=true
--unknownStart=true --fixedVarThreshold=12 --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2
--imuFilter=ekf5 --imuMagSigmaDeg=3 --ekf5FinderConsider=true --ekf5RelayGate=true`, five arms, A and B (both
`--reportAirtime=true`); variant S (the preset's 1992.9) and N (`--integrityChi2=9.21`); seeds 41–60. **80 cells**,
one frozen binary. Only `ca_tbl_full` runs the integrity check; the other arms are the controls they were.

## 2. Predictions, made before any cell

| # | prediction | basis (seeds 1–20) |
|---|---|---|
| **P1** | N raises CA-TBL's availability against S in B (interval above 0), and costs none in A (lower bound > −0.01) | +0.008 [+0.001, +0.015]; A +0.015 [−0.000, +0.030] |
| **P2** | N costs CA-TBL at most 10 % CEP95 (mean per-seed ratio N / S − 1 ≤ +0.10) in A and in B | A −9.2, B +0.5 m, both n.s. |
| **P3** | under N, no comparator has significantly higher availability than CA-TBL in A or B | every interval spanned or exceeded 0 |
| **P4** | under N, no comparator is significantly more accurate than CA-TBL in A or B | every interval spanned or was below 0 |
| **P5** | under N, CA-TBL's availability exceeds Poucet's in B (interval above 0) | +0.031 [+0.018, +0.044] |

**Against them, stated now:** B's CEP95 against Poucet under N reached +1.3 [−0.2, +3.2] m in sample, close to
significance; A's availability gain was borderline; a gate at 9.21 can still lock a filter out on a seed whose
covariance is wrong.

**Decision rule.** P1, P2, P3 and P4 → the specified threshold, with the stack, is put to the owner in place of
the 1992.9 override. P5 is the goal's reading against Poucet.

## 3. Abort conditions

Not 80 cells, or a cell's `rc` not 0; a `run_id.txt` not `dirty 0`, or more than one source; a manifest mismatch;
a cell whose stack lines or integrityChi2 do not match it; an arm with fewer than 20 seeds.

## 4. Disclosure

Seeds 41–60 have not been run. Read before this commit: the seeds 1–20 run of N described above, and everything
in REAL-STACK, REAL-BEST and N.57–N.61.
