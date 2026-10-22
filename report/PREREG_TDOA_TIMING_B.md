# PREREG TDOA-TIMING-B — better tier-1 arrival timing on the paper's comparators

Registered 2026-10-21, before any campaign cell. The runner (`scripts/tdoa_timing_b_run.sh`) and the analysis
(`scripts/tdoa_timing_b_analyse.py`) are committed with this file and are not edited after the first cell.

## 1. Question

TDOA-TIMING (`report/PREREG_TDOA_TIMING.md`) measured CA-TBL and the 868 MHz-only arm at 5 km and in the endgame:
- CA-TBL gained nothing;
- static_868 gained in the endgame.

This run asks how much the same two no-hardware proposals give the **other methods in the paper's tables**:
- nanosecond gateway sync (S0);
- arrival-time refinement such as two-pass CFO/STO (T0, a ceiling).

It also adds the 8-gateway configuration, where TDoA solves are more frequent. CA-TBL is included there as the
control.

## 2. Mechanism

The same flags as TDOA-TIMING:
- `--tdoaPpsSigmaNs`;
- `--tdoaToaScale` (G168: byte-identical at 1, refused at 0);
- `--tier1Tdoa`.

Tier-1 TDoA is built per arm whenever `--tier1Tdoa` is on (`Tier1Tdoa`, one instance per arm run), so every arm with
an 868 MHz tier is exposed.

## 3. Design

| | |
|---|---|
| binary | `build/scratch` from `22af153e`'s source (src/ and scratch/ unchanged since); copied with a SHA-256 manifest, verified before every cell |
| common flags | `--specProfile=true --duration=1800 --trace=true` |
| **A** (5 km, 3 gateways) | arms fixed_threshold, poucet_fusion, lora_fusion, static_868, static_2p4 |
| **B** (endgame) | the same arms, `--boxHalf=750 --finderMode=1` |
| **C** (8 gateways) | arms ca_tbl_full, static_868, lora_fusion, `--nGateways=8 --gwLayout=legacy`, as CAMP-FINAL's C_n8 |
| configurations | **L**: the preset; **S0**: `--tdoaPpsSigmaNs=0`; **T0**: `--tdoaPpsSigmaNs=0 --tdoaToaScale=0.01`; **N**: `--tier1Tdoa=false` |
| seeds | 361–380 (fresh; the highest registered before is 360) |
| cells | 3 × 4 × 20 = 240 |

The arms map to the paper's rows:

| arm | paper row |
|---|---|
| fixed_threshold | Fixed threshold |
| poucet_fusion | Poucet-style fusion |
| lora_fusion | LoRa dual-band fusion |
| static_868 | LoRa-868 RSSI+TDoA (L); LoRa-868 RSSI (N) |
| static_2p4 | LoRa-2.4 ToF |

## 4. Metrics and statistics

- **Metrics:** the paper's own per-seed quantities, `lora_cmp_analyse.per_seed`:
  - availability, the subject mean of fixCycles/steps;
  - CEP95;
  - energy, the subject mean of totalJ.
- **Statistics:** paired over seeds, bootstrap with 10 000 resamples (NumPy seed 20261021, one resample matrix per
  geometry), 95 % percentile intervals.

**Contrasts:**

| contrast | what it measures |
|---|---|
| SYNC = S0 − L | the ceiling on gateway-sync improvement |
| TIMING = T0 − L | the ceiling on arrival-timing improvement |
| TDOA VALUE = L − N | what tier-1 TDoA contributes now |

**Verdict on Δ availability, per geometry, arm and contrast:**
- **MATERIAL** if the interval lies above 0 and the mean is ≥ +2 pp;
- **SMALL** if the interval lies above 0 and the mean is < +2 pp;
- **ADVERSE** if the interval lies below 0;
- **NONE** otherwise.

CEP95 and energy are reported with their intervals and carry no verdict.

## 5. Checks and abort conditions

The analysis exits 1, and no verdict is read, if any of these holds:
1. any `rc` is not 0;
2. the count is not 240 cells, or 20 seeds per geometry, configuration and arm;
3. any `run_id.txt` is not `dirty 0`, or more than one source appears;
4. the `TDOA TOA SCALE` line appears in a cell other than T0, or is missing from a T0 cell.

## 6. Predictions (before any cell)

These are made knowing TDOA-TIMING's result, which is on a different metric (five-node A(25 m)) and different
seeds.

- **E1.** static_868, TIMING in B: MATERIAL. This replicates TDOA-TIMING on the paper's metric.
- **E2.** SYNC is NONE for every arm in A and B. 20 ns is 6 m of range difference, small against the arrival-time
  bound.
- **E3.** In C, TIMING is MATERIAL for static_868 and lora_fusion, and NONE or SMALL for ca_tbl_full.
- **E4.** static_2p4 is identical under L, S0, T0 and N in every seed. It has no 868 MHz tier.
- **No directional prediction** for fixed_threshold, poucet_fusion and lora_fusion in A and B.

## 7. Limitations

- **T0 is a ceiling.** It is not a model of any receiver. Arrival sigma at 1 % of the one-way bound is better than
  any implementable estimator, so a T0 gain bounds what refinement could buy and does not predict it.
- **Overconfidence is not corrected.** The solver is told the reduced sigma while the NLOS excess delay τ_nlos is
  unchanged, so T0 fixes may be overconfident. That would bias T0 against the arms that fuse TDoA.
- **Simulator only**, with the channel and energy models of the paper's tables.

# RESULT — after the registration above (`a9174090`)

## Runs

**First run** (22:28–00:52 UTC, 240 cells, all `rc` 0): **not scored.**
- Check 3 failed: its cells name four sources, `a9174090` and three commits made during the run.
- Those commits touched only `report/`, `scripts/` and `results/`. The simulator records HEAD at each cell's launch.
- Its `RUN.md` is kept as `results/tdoa_timing_b_2026-10-22/FIRST_RUN_NOT_SCORED_RUN.md`.
- Its launch was also disturbed: two runners overlapped for about 20 s; both were stopped before any cell finished,
  then one was relaunched.

**Rerun** (00:54–03:12 UTC):
- **Setup:** a detached worktree at `91dd6e65`, so commits elsewhere could not move its HEAD. The binary and
  libraries were byte-identical to the first run's manifest.
- **Outcome:** 240 of 240 cells, every `rc` 0, one source, every section 5 check passed.
- **Scored once:** `results/tdoa_timing_b_2026-10-22/TDOA_TIMING_B.md`.
- **Determinism:** the two runs' `arm_summary.csv` and `subject_summary.csv` are byte-identical, 480 of 480 files.
  The first run's numbers were not read before this comparison.
- **Housekeeping:** each cell was verified against its `run_id.txt` digest, then pruned to the files the analysis
  reads (`prune_cells.py`, `prune.log`).

## Registered verdicts (Δ availability)

**Every geometry, arm and contrast is NONE except one:**

| geometry | arm | contrast | Δ availability |
|---|---|---|---|
| 8 gateways | static_868 | TIMING | **ADVERSE**, −2.1 pp [−3.4, −0.9] |

## Intervals that exclude 0 (no verdict attached)

**CEP95:**

| geometry | arm | contrast | Δ CEP95 |
|---|---|---|---|
| endgame | static_868 | SYNC | −18.8 m [−33.9, −4.0] |
| endgame | static_868 | TDOA VALUE | −160.2 m [−189.6, −129.7] |
| endgame | poucet_fusion | TIMING | −0.2 m [−0.4, −0.1] |
| 8 gateways | static_868 | TDOA VALUE | −508.8 m [−616.6, −396.5] |
| 8 gateways | ca_tbl_full | TIMING | −1.9 m [−3.1, −0.6] |
| 8 gateways | lora_fusion | TIMING | −1.8 m [−2.7, −1.0] |

**Energy:**

| geometry | arm | contrast | Δ energy |
|---|---|---|---|
| endgame | static_868 | TIMING | +4.43 J [+3.43, +5.27] |
| endgame | lora_fusion | TIMING | +0.62 J [+0.36, +0.97] |
| endgame | poucet_fusion | TDOA VALUE | +2.30 J [+0.20, +5.31] |
| 8 gateways | static_868 | TIMING | +5.06 J [+4.30, +5.69] |
| 8 gateways | lora_fusion | TIMING | +1.96 J [+0.94, +3.03] |
| 8 gateways | ca_tbl_full | TIMING | +1.11 J [+0.47, +1.66] |

Three small TDOA VALUE energy intervals at 5 km sit at +0.02 to +0.03 J, at the edge of 0.

## Predictions

| prediction | outcome | evidence |
|---|---|---|
| E1 | **Fails** | static_868 endgame TIMING −0.6 pp [−1.8, +0.4] |
| E2 | Holds | SYNC NONE for every arm at 5 km and in the endgame |
| E3 | **Fails** for static_868 and lora_fusion | static_868 is ADVERSE, lora_fusion is NONE |
| E3 | Holds for ca_tbl_full | NONE |
| E4 | Holds exactly | static_2p4's per-seed values are identical under L, S0, T0 and N in all 20 seeds of both geometries |

## The T0 energy cost: mechanism, checked

`results/tdoa_timing_b_2026-10-22/TRIGGERED_UPDATE_CHECK.md` reruns one endgame static_868 cell (seed 341) under L
and T0.

| | L | T0 |
|---|---|---|
| triggered updates (`large_correction`) | 8 | 544 |
| tag-0 868 MHz airtime | 45.7 s | 176.6 s |

**Mechanism.** The MC-2 triggered update (on under `--specProfile`) sends a 3-frame flooded update whenever the
estimate moves more than √trace(P). T0 tells the solver the stamps are near-exact, so P, and with it the threshold,
collapses.

**Reading.** The energy cost belongs to that rule's interaction with an overconfident covariance, not to better
timing as such.

**Not tested.**
- That the same flood causes static_868's ADVERSE availability at 8 gateways, through collisions with ranging
  traffic, is a hypothesis.
- A real receiver with refined timing would report a sigma above the bound, not 1 % of it.

## Reading

1. **Availability does not move.** Availability, the share of cycles with a fix, was registered as the verdict
   metric to match the paper's tables. Tier-1 timing changes where a fix lands, not whether one exists. That is why
   E1 fails here although TDOA-TIMING found an accuracy gain for static_868 on five-node A(25 m).
2. **Where better timing does act: the 868 MHz-only system's CEP95.**
   - Perfect sync cuts its endgame CEP95 by 18.8 m.
   - Tier-1 TDoA itself cuts it by 160 m in the endgame and 509 m at 8 gateways.
3. **Fusion methods, CA-TBL included, gain little.** The ceiling buys about 2 m of CEP95 at 8 gateways, for 1–2 J.
4. **No comparator's availability improves under either option, and one worsens.** So neither nanosecond sync nor
   timing refinement gives the paper's comparators a gain on the paper's headline metric. The 868 MHz-only
   system's CEP95 is the exception, and there sync alone already delivers it.
