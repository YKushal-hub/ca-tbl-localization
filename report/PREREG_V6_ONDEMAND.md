# PREREG V6-ONDEMAND — a calibrated on-demand 2.4 GHz rule (exploratory grade)

**Registered before any V6 measurement cell runs** (rule 13). Written 2026-10-16. Engine: `e1295066`
(`--traceOnDemand`, `--onDemandCalibrate`, G143). Owner's master plan v3, stage V6.

## 1. What is disclosed
- **B4-3 (registered):** the raw rule (pSkip 0.05) slept on 1.3–1.5 % of cycles at 5 km and 1 in 14 400 in the
  endgame.
- **Dev reliability (6 seeds, `--traceOnDemand`):**
  - 5 km: 56 % of attempts succeed; the predictor over-states (0.66 → 0.41 in the 0.5–0.8 bin).
  - Endgame: 97 % succeed; the predictor is calibrated.
- **Dev energy:** 2.4 GHz is 22–44 % of tag-0 energy, and UWB dominates in the endgame.
- **Dev calibrated rule (pSkip 0.25, 4 seeds):**
  - attempted 99.7 % of cycles at 5 km and 97 % in the endgame;
  - tag-0 energy −0 % and −1.5 %;
  - CEP95 unchanged (5 km) and 10.3 → 11.0 m (endgame).

## 2. Cells
Common:
```
--specProfile=true --duration=1800 --trace=true --traceOnDemand=true --imuQuScale=10 --unknownStart=true
--fixedVarThreshold=12 --foliageDepthClamp=true --rngKeyed=true --rangeErrorNoLinkLeak=true
--rssiRangeFromShadowing=true --arms=ca_tbl_full --onDemandR50M=328 --onDemandSigmaDb=9.1
```
- Configs: A (5 km) and B (`--boxHalf=750 --finderMode=1`).
- Variants:
  - ALWAYS: no on-demand;
  - RAW: `--dualBaseOnDemand=true` (pSkip 0.05);
  - CAL: `--dualBaseOnDemand=true --onDemandCalibrate=true --onDemandPSkip=0.25`.
- Seeds 1–20.
- Total: 120 runs.
- Frozen binary, manifest, outdir outside the repository.

## 3. Predictions
| # | prediction | confidence |
|---|---|---|
| **V6-1** | A, CAL, attempted cycles after t = 300 s, pooled: \|mean pCal − observed success\| <= 0.05 | medium |
| **V6-2** | CAL tag-0 energy saving against ALWAYS is under 5 % (paired mean) in both configs (immaterial) | medium |
| **V6-3** | CAL CEP95 within 25 % of ALWAYS (paired mean) in both configs | medium |
| **V6-4** | B, RAW: the 2.4 GHz radio sleeps on under 2 % of tag-0 cycles | high |

## 4. Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`, or with a differing compile stamp.
- A manifest mismatch.
- Fewer seeds than registered.
