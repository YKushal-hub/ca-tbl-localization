# PREREG V2A2-NLOS3GPP — 3GPP RMa NLOS on the adopted preset (exploratory grade)

**Registered before any measurement cell runs** (rule 13). Written 2026-10-17. Engine: `11b4dce8` (preset with the
master-plan-v3 fixes, DESIGN_DECISIONS 10; `--nlos3gppRMa`, G146). Master plan v3, stage V2a-2.

## 1. What is disclosed
- **Unit gate G146:** NLOS fraction at 1 km 0.629 (3GPP 0.628); excess 22–35 dB.
- **G146 scenario (600 s, `static_868`, preset before this commit's adoption):** NLOS share of tag 0's gateway ranges
  0.10 → 0.47, and their count 241 → 15.
- This is a realism change, not a fix. The predictions say it makes 868 MHz-dependent positioning worse.

## 2. Cells
Common:
```
--specProfile=true (now including the v3 fixes) --duration=1800 --trace=true --traceRanges=true
--traceCovariance=true --imuQuScale=10 --unknownStart=true --fixedVarThreshold=12
--arms=static_868,static_2p4,ca_tbl_full,fixed_threshold
```
- Configs: A (5 km), B (`--boxHalf=750 --finderMode=1`), R8 (`--nGateways=8 --gwLayout=legacy`).
- Variants: OFF, and ON = `--nlos3gppRMa=true`.
- Seeds 1–20.
- Total: 120 runs.
- Frozen binary, manifest, outdir outside the repository.

## 3. Predictions (tag 0, paired over seeds, mean [95 % bootstrap])
| # | prediction | confidence |
|---|---|---|
| **N1** | A: share of tag 0's gateway ranges (`sourceKind` 1) that are NLOS: ON > OFF + 0.2 (pooled) | high |
| **N2** | A: `static_868` CEP95 ON − OFF interval above 0 | medium |
| **N3** | R8: `ca_tbl_full` CEP95 ON − OFF paired mean > 0 | medium |
| **N4** | A: `ca_tbl_full` fix-cycle share ON < OFF (paired mean) | medium |
| **N5** | B: `static_2p4` CEP95 ON − OFF paired mean within ±25 % of OFF (endgame links are mostly low and keep the old model) | low |

## 4. Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`, or with a differing compile stamp.
- A manifest mismatch.
- Fewer seeds than registered.
