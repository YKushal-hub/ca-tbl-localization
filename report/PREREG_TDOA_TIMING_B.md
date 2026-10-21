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
