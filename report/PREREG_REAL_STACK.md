# PREREG REAL-STACK — the realistic world, with the filter made consistent under it

**Registered before any cell runs** (rule 13). Written 2026-10-18. Engine: `work/2026-10-18-magnetometer` at `bff86106`
(`--ekf5FinderConsider`, `10370f64`; `--ekf5RelayGate`, `dc622990`; both with their null control; gates G100,
G101). This commit adds this file, `scripts/real_stack_run.sh` and `scripts/real_stack_analyse.py`, and nothing
else. Outputs go outside the repository, to `~/ns/catbl-work/real_stack_2026-10-18/`, which does not exist at
registration.

**Why.** The most realistic world the engine models: a BMI270-class gyro (0.15 °/s), 5 % per-step stride noise, and
a finder GNSS error of 2 m per finder (`--finderPosSigma=2`, main's own "self-consistent setting"; its default 0 is,
in its own help text, "the flattering assumption"). Under it, with ekf5 and a magnetometer, CA-TBL's endgame CEP95
was 10.5 m in the pilot. Two defects were found there: repeated ranges to an offset finder were fused as
independent, and relayed ranges anchored on a peer's overconfident estimate were fused (one at z = −25). This
measures the stack that fixes both. It adopts nothing.

## 1. Cells (`real_stack_run.sh`)

`--specProfile=true --duration=1800 --trace=true --traceCovariance=true --unknownStart=true --fixedVarThreshold=12
--imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2 --imuFilter=ekf5 --imuMagSigmaDeg=3`, five arms,
A (5 km) and B (endgame), both `--reportAirtime=true`; variant M (as given) and S (`--ekf5FinderConsider=true
--ekf5RelayGate=true`); seeds 1–20. **80 cells**, one frozen binary. Every arm runs under the same variant.

## 2. Quantities (`real_stack_analyse.py`)

`lora_cmp_analyse.per_seed` (availability, tag 0 CEP95), `camp_spec_analyse.ci` paired by seed,
`v3_analyse.node_rows` coverage, subject J; S − M for CA-TBL; CA-TBL − each comparator under S.

## 3. Predictions, made before any cell

| # | prediction | basis |
|---|---|---|
| **P1** | S lowers CA-TBL's tag 0 CEP95 against M in B — interval below 0 | the pilot: 10.5 (M) → 4.4 m (S) |
| **P2** | S costs CA-TBL no availability: S − M interval lower bound above −0.01, in A and in B | the gate drops relays only; the pilot kept 0.593 |
| **P3** | under S, CA-TBL's availability exceeds Poucet's in B — interval above 0 | the pilot: +0.023 [0.008, 0.037] |
| **P4** | under S, CA-TBL − Poucet CEP95 in B — the interval contains 0 | the pilot: +0.8 [−0.1, +1.5] |

**Against them, stated now:** the pilot's S − M endgame change for consider alone was −1.2 [−7.7, +5.1]; most of
P1's expected gain comes from the gate, whose threshold (χ²₁ at 0.999) was chosen, not fitted. At 5 km no variant
in the pilot separated from another; nothing is predicted for A's accuracy.

**Reported, not predicted:** A for every arm; coverage; energy; every other comparison contrast.

**Decision rule.** P1 and P2 → the stack is put to the owner with the realistic world. P3 and P4 are the goal's
reading against Poucet, reported either way.

## 4. Abort conditions

Not 80 cells, or a cell's `rc` not 0; a `run_id.txt` not `dirty 0`, or more than one source; a manifest
mismatch; a cell whose variant or realistic-world lines do not match it; an arm with fewer than 20 seeds.

## 5. Disclosure

Read before this commit, exploratory and not citable, all on 20 seeds: the realistic world with finder GNSS error
(M); S's two parts together; relayed ranging off entirely; θ owned by the magnetometer (rejected); a sweep of the
UWB escalation thresholds in B (more UWB was less accurate); the endgame seed 3 range trace. MAG's and REAL-IMU's
results were read in full.

## 6. What this cannot establish

A real GNSS error's time structure (the model is a constant per-finder offset), a real magnetometer, the ring, Wolf
ToF, the default.
