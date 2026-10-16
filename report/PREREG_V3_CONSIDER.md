# PREREG V3-CONSIDER — consider biases for correlated 868 MHz range errors (exploratory grade)

**Registered before any V3 measurement cell runs** (rule 13). Written 2026-10-16. Engine: the commit adding
`--rangeBiasStates` and `--rssiSigmaLogNormal`. Owner's master plan v3, stage V3; TECH_DEBT N.51.

## 1. What is disclosed
Development runs (6 seeds, config B, 900 s; not measurements) were read before this registration. All used
`--rngKeyed --rangeErrorNoLinkLeak --rssiRangeFromShadowing`.
- **Per-range log, z = error / reported sigma:**
  - 868 MHz gateway ranges: E[z^2] 2.3–2.5, mean z −0.4 (log-normal error against a first-order sigma; selection
    toward favourable shadowing).
  - Finder ranges: E[z^2] about 1.1.
  - 2.4 GHz direct ranges: about 0.9.
  - Relayed 2.4 GHz ranges: 12–37.
- **Subject filters:** on `static_868`, median NEES 12–37.
- **`--rangeBiasStates` (dev):**

  | arm | median NEES | coverage | CEP95 |
  |---|---|---|---|
  | `ca_tbl_full` | 3.18 → 1.90 | 0.69 → 0.75 | 11.4 → 8.4 m |
  | `static_868` | 12.3 → 7.4 | 0.43 → 0.57 | 11.9 → 8.7 m |
  | `fixed_threshold` | 4.47 → 1.90 | 0.62 → 0.79 | 8.7 → 9.6 m |
  | `static_2p4` | unchanged | unchanged | unchanged |

- **Adding `--rssiSigmaLogNormal` (dev):** `static_868` coverage 0.66, but `ca_tbl_full` CEP95 14.5 m and coverage
  0.69. Mixed, so it is **not** in this campaign's treatment; it stays off.
- **Remaining overconfidence** sits in the early-run tail (2.4 GHz acquisition) and in relayed ranges. These belong to
  V4; no V3 flag targets them.

## 2. Cells
Common:
```
--specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true
--imuQuScale=10 --unknownStart=true --fixedVarThreshold=12 --foliageDepthClamp=true --rngKeyed=true
--rangeErrorNoLinkLeak=true --rssiRangeFromShadowing=true --arms=static_868,static_2p4,ca_tbl_full,fixed_threshold
```
- Configs: A (5 km) and B (`--boxHalf=750 --finderMode=1`).
- Variants: OFF, and ON = `--rangeBiasStates=true`.
- Seeds 1–20.
- Total: 2 × 2 × 20 = 80 runs.
- Frozen binary, SHA-256 manifest, outdir outside the repository.

## 3. Metrics
Tag 0 and subjects 2–5 pooled (`scripts/nees_check.py` rows: after the node's first fix, t >= 120 s):
median NEES, R95 coverage. Tag-0 CEP95 (`arm_summary.csv`), paired over seeds, mean [95 % bootstrap].

## 4. Predictions
| # | prediction | confidence |
|---|---|---|
| **V3-1** | B: `ca_tbl_full` R95 coverage ON > OFF, and ON median NEES within [1.0, 3.0] | medium |
| **V3-2** | B: `static_868` R95 coverage ON − OFF >= 0.08 | medium |
| **V3-3** | B: `ca_tbl_full` CEP95 ON − OFF paired mean <= +10 % of OFF (not materially worse) | medium |
| **V3-4** | A: `static_868` R95 coverage ON > OFF | medium |
| **V3-5** | B: no arm reaches the S3 band (coverage 0.95 ± 0.03) in either variant (the remaining tail is V4's) | high |
| **V3-6** | `static_2p4` outputs are identical ON and OFF in both configs (no 868 MHz range enters it) | high |

## 5. Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`, or with a differing compile stamp.
- A manifest mismatch.
- Fewer seeds than registered.
