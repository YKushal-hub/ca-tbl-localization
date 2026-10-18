# PREREG REAL-BEST — the realistic-world stack, out of sample on seeds 21–40

**Registered before any cell runs** (rule 13). Written 2026-10-18. Engine: `work/2026-10-18-magnetometer` at `003eaac5`
(the realistic-world mechanisms: `--imuStrideSigmaPct`, `--imuMagSigmaDeg`, `--ekf5FinderConsider`,
`--ekf5RelayGate`, `--ekf5StateBounds`, each default-off with its null control and gate G97, G99–G102; ekf5, G95).
This commit adds this file, `scripts/real_best_run.sh` and `scripts/real_best_analyse.py`, and nothing else.
Outputs go outside the repository, to `~/ns/catbl-work/real_best_2026-10-18/`, which does not exist at registration.

**Why out of sample.** Every exploratory run behind these mechanisms used seeds 1–20, as did REAL-STACK. The engine
is deterministic, so a "registered" rerun of those seeds would reproduce numbers already read. This uses **seeds
21–40, never run in this line**, so the predictions below are tested on data that did not shape them.

## 1. Cells (`real_best_run.sh`)

The realistic world in every cell: `--specProfile=true --duration=1800 --trace=true --traceCovariance=true
--unknownStart=true --fixedVarThreshold=12 --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2
--imuFilter=ekf5 --imuMagSigmaDeg=3`, five arms, A (5 km) and B (endgame), both `--reportAirtime=true`.
Variants: **M** (as given); **S** (`--ekf5FinderConsider=true --ekf5RelayGate=true`); **F** (S + `--ekf5StateBounds=4`).
Seeds 21–40. 3 × 2 × 20 = **120 cells**, one frozen binary; every arm runs under the same variant.

## 2. Quantities (`real_best_analyse.py`)

`lora_cmp_analyse.per_seed` (availability, tag 0 CEP95), `camp_spec_analyse.ci` paired by seed,
`v3_analyse.node_rows` coverage, subject J; variant contrasts for CA-TBL; CA-TBL − each comparator under F.

## 3. Predictions, made before any cell

| # | prediction | basis (seeds 1–20) |
|---|---|---|
| **P1** | F lowers CA-TBL's tag 0 CEP95 against M in B — interval below 0 | REAL-STACK P1: 10.5 → 4.4 m |
| **P2** | F costs CA-TBL no availability against M: lower bound above −0.01, in A and in B | REAL-STACK P2 missed at 5 km by 0.001 for S; bounds added +0.006 |
| **P3** | under F in B: CA-TBL − Poucet CEP95 interval contains 0, and CA-TBL's availability interval is above 0 | REAL-STACK P3, P4; the bounds pilot |
| **P4** | under F, no comparator is significantly more accurate than CA-TBL in A or B (no CA-TBL − comparator CEP95 interval entirely above 0) | the bounds pilot: every interval spans or is below 0 |
| **P5** | F lowers CA-TBL's CEP95 against S in A — interval below 0 | the bounds pilot: −86 [−244, +52], **not significant** |

**Against them, stated now:** P5's basis is not significant; P3's accuracy half rested on intervals reaching +1.4–1.5
m; 5 km outcomes are dominated by a few seeds per twenty, and new seeds bring new ones.

**Decision rule.** P1, P2 and P4 → the realistic world and the full stack are put to the owner together as the
preset. P5 decides the bounds alone. P3 is the goal's reading against Poucet, reported either way.

## 4. Abort conditions

Not 120 cells, or a cell's `rc` not 0; a `run_id.txt` not `dirty 0`, or more than one source; a manifest mismatch;
a seed outside 21–40; a cell whose variant or realistic-world lines do not match it; an arm with fewer than 20 seeds.

## 5. Disclosure

Seeds 21–40 have not been run by this line. Read before this commit: everything on seeds 1–20 (REAL-IMU, MAG,
REAL-STACK and every exploratory run of the stack, the gate, the bounds, θ ownership, fading, the threshold sweep).

## 6. What this cannot establish

Real sensors beyond these models (constant finder GNSS offsets, a Gauss-Markov magnetometer disturbance); the
ring; Wolf ToF; anything about seeds beyond 40.
