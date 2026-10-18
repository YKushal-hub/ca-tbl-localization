# PREREG B4-SWITCHING — does CA-TBL's switching earn its place under a realistic error model? (exploratory grade)

**Registered before any B4 cell runs** (rule 13). Written 2026-10-16. Engine: the commit that adds
this file, carrying R1 (`--acquireReinit`, G133), B3f (`--foliageDepthClamp`, G134), S1/S2
(`--rssiRangeFromShadowing`, `--tofErrorModel`, G135) and B2 (`--dualBaseOnDemand`, G136). Owner's
master plan v2 (kept with the thesis material outside the repository), stage B4; decisions D1–D3.

## 1. What is disclosed
- Stage A (a standalone simulation outside this repository, pre-registered, 4200 runs) found:
  attempting 2.4 GHz whenever in contact is the most accurate policy, and confidence-gated
  escalation is harmful. Under `--loraDualBase` the preset already ranges both LoRa tiers every cycle,
  so the preset IS that policy; no new mode is added for it.
- K1: `static_2p4` dominated `ca_tbl_full` in every cell (before FIXES-N26 entered the preset).
- R1-REINIT's result (registered separately) decides one flag here, by the rule in §2.
- **S3 consistency check (master plan v2), read on R1-REINIT's control cells before this registration:**
  under `--unknownStart` the filter covariance is far too small. R95 coverage at 5 km is 0.42-0.59
  (target 0.95) and the median NEES is 24-31 for `ca_tbl_full` and `fixed_threshold` (target about 2).
  In the endgame, coverage is 0.75-0.84. Rules that act on Σ (PCS, `fixed_threshold`) therefore decide
  on an overconfident quantity. B2's on-demand rule does not use Σ (it uses this cycle's 868 ranges).
  B4 reports the same check per cell and arm (`b1/nees_check.py`, copied as `scripts/nees_check.py`).
- R1-REINIT's predictions R1a and R1c did not hold, so `--acquireReinit` is **off** here (rule below).
- No B4 cell has been run.

## 2. Cells
Common flags: `--specProfile=true --duration=1800 --trace=true --traceCovariance=true
--traceSubjectConsistency=true --imuQuScale=10 --unknownStart=true --fixedVarThreshold=12
--foliageDepthClamp=true --rssiRangeFromShadowing=true`, plus `--acquireReinit=true` **if** R1-REINIT's
R1a or R1c held, otherwise not (recorded in RUN.md before launch).
Arms: `static_868,static_2p4,lora_fusion,ca_tbl_full,fixed_threshold`; and a second invocation
`--arms=ca_tbl_full --dualBaseOnDemand=true --onDemandR50M=<r50> --onDemandSigmaDb=<s>`, where <r50> and <s> are
fitted by `scripts/b4_fit_ondemand.py` (least squares of Φ(10·2.3·log10(r50/d)/s) to the 2.4 GHz PDR rows of
B3F-FOLIAGE's clamped probe, 5 seeds pooled) and recorded in RUN.md before launch. Why fitted: the S6 check
(`scripts/s6_outage_check.py`) found the engine's delivery curve flatter than the pure-shadowing outage
model with the engine's own σ (max |pdr − Φ(M/σ)| 0.17 on 2.4 GHz in `results/causal`'s probe).
Configs: A (5 km) and B (`--boxHalf=750 --finderMode=1`). Seeds 1-20.

| cell group | extra flag | runs |
|---|---|---|
| P_engine, P_gottschalk, P_muller, P_wolf | `--tofErrorModel=<preset>` | 4 × 2 configs × 20 seeds × 2 invocations = 320 |
| S1off_muller | `--tofErrorModel=muller --rssiRangeFromShadowing=false` (main invocation only) | 2 × 20 = 40 |

## 3. Metrics (tag 0 unless stated; `s2c_analyse.seed_metrics` for five-node figures)
CEP95 (tag 0), five-node w25, availability (fixCycles/steps, subjects), tag-0 totalJ. Paired over seeds,
mean [95 % bootstrap]; sign tests where stated (two-sided, α = 0.05).

## 4. Predictions
| # | prediction | confidence |
|---|---|---|
| **B4-1** | B (endgame): `ca_tbl_full` CEP95 ≤ `static_2p4` CEP95 (paired mean difference ≤ 0) under ≥ 3 of the 4 presets | medium |
| **B4-2** | B: `ca_tbl_full` five-node w25 ≥ `static_868`'s (interval above 0) under all 4 presets | high |
| **B4-3** | B: `ca_tbl_full` with `--dualBaseOnDemand` uses less tag-0 energy than `ca_tbl_full` on ≥ 15/20 seeds, with CEP95 within 25 %, under ≥ 3 presets | medium |
| **B4-4** | R5 / DEC-H: `ca_tbl_full`'s CEP95 is lower than `fixed_threshold`@12's (sign test) under ≥ 3 presets in B; if not, the switching contribution is reported as not separated from a trace-only rule | low |
| **B4-5** | A (5 km), preset muller: S1 on raises `static_868`'s CEP95 against S1 off (interval above 0) | medium |
| **B4-6** | The `static_2p4` − `static_868` CEP95 gap in B is smaller under wolf than under engine | high |
| **B4-7** | S3: R95 coverage for `ca_tbl_full` in B stays below 0.92 under every preset (the covariance remains overconfident; no B4 flag targets it) | medium |

## 5. Abort conditions
A run without an `rc` or exiting non-zero; a `run_id.txt` without `dirty 0` or with a differing compile
stamp; a manifest mismatch; fewer seeds than registered. Causes are fixed and no cell is dropped.
