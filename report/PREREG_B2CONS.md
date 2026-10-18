# PREREG B2CONS — filter consistency at 5 km (scope B2; exploratory)

**Registered before any cell runs** (rule 13). Written 2026-10-17. The parent commit adds `--rssiCorrInflate`
(G149, default off; NC1–NC10 byte-identical). This commit adds this file, `scripts/b2cons_run.sh` and
`scripts/b2cons_analyse.py`.

**Why.** The final preset's `ca_tbl_full` covers the truth with its 95 % ellipse on about 0.6–0.7 of cycles at 5 km.
`static_868` covers it on about 0.07, with median NEES about 240. Two candidate causes:
- **(a)** the IMU increment covariance is too small. It is scaled by `--imuQuScale`, 10 in every campaign since S2,
  while TECH_DEBT measured the true factor as 3–54.
- **(b)** successive 868 MHz RSSI ranges share their link's shadowing (Gudmundson, d_corr 30 m) and are fused as
  independent.

**Disclosed.** A 6-seed dev grid at A (outside results/):

| variant | arm | coverage | median NEES | CEP95 | post-acq CEP95 |
|---|---|---|---|---|---|
| BASE | `ca_tbl_full` | 0.672 | 3.6 | 134.5 m | 32.3 m |
| BASE | `static_868` | 0.068 | 239 | 245 m | |
| Q30 | `ca_tbl_full` | 0.851 | | 95.2 m | 16.4 m |
| CORR | `ca_tbl_full` | 0.601 | | | 73.3 m |
| CORR | `static_868` | 0.289 | 26 | 187 m | |

## Cells
Common:
```
--specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true
--unknownStart=true --fixedVarThreshold=12 --arms=static_868,static_2p4,ca_tbl_full,fixed_threshold
```
Configs: A (5 km) and B (`--boxHalf=750 --finderMode=1`).

| variant | flags |
|---|---|
| BASE | `--imuQuScale=10` |
| Q30 | `--imuQuScale=30` |
| CORR | `--imuQuScale=10 --rssiCorrInflate=true` |

Seeds 1–20: 120 cells.

## Predictions (coverage pooled over tag 0 and subjects after their first fix and t ≥ 120 s; CEP95 tag 0, paired)
| # | prediction |
|---|---|
| K1 | A, Q30: `ca_tbl_full` coverage > BASE, and CEP95 paired mean ≤ BASE |
| K2 | A, CORR: `static_868` coverage > BASE and CEP95 paired mean < BASE |
| K3 | A, CORR: `ca_tbl_full` CEP95 after 300 s paired mean > BASE (the hybrid loses 868 MHz information it was using) |
| K4 | B, Q30: `ca_tbl_full` coverage ≥ BASE |

**Adoption rule** (fixed now; per variant): adopt if `ca_tbl_full` coverage rises in both A and B **and** its CEP95
paired mean is within +10 % of BASE in both. Q30 is adopted by making `--imuQuScale=30` the preset's value, so the
preset sets it, and campaigns stop passing it. CORR is adopted into the preset as `--rssiCorrInflate=true`.

## Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`.
- A manifest mismatch.
