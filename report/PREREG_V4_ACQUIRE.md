# PREREG V4-ACQUIRE — particle-filter acquisition against wrong-root lock (exploratory grade)

**Registered before any V4 measurement cell runs** (rule 13). Written 2026-10-16. Engine: `72ee8969`
(`--acquireParticles`, G142). Owner's master plan v3, stage V4; TECH_DEBT N.54.

## 1. What is disclosed
- **R1 (registered):** re-initialising one Gaussian at a multilateration root did not help.
- **Unit gate G142:** two anchors hold both mirror roots 40/40; a third resolves to the true root 40/40.
  - Without progressive correction, 30/40 trials collapsed onto one root.
  - The first handover rule (unimodality alone) handed over after one cycle with a km-wide cloud. That was
    fixed by the size guard before this registration.
- **Dev runs** (4 seeds, 1800 s; final code; consider states ON in dev, OFF here):

  | config | arm | CEP95 | median error | consistency |
  |---|---|---|---|---|
  | A | `ca_tbl_full` | 518 → 253 m | 59 → 15 m | median NEES 30 → 6 |
  | A | `static_2p4` | 623 → 300 m | 187 → 24 m | coverage 0.46 → 0.93 |
  | A | `static_868` | 2153 → 1276 m | | |
  | B | `static_2p4` | 39 → 11 m | | coverage 0.80 → 0.96 |
  | B | `ca_tbl_full` | 7 → 14 m (one seed 35.5 m) | | coverage 0.83 → 0.74 |

- **V3 (registered)** showed dev runs can reverse at 20 seeds; the predictions below allow for that.

## 2. Cells
Common:
```
--specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true
--imuQuScale=10 --unknownStart=true --fixedVarThreshold=12 --foliageDepthClamp=true --rngKeyed=true
--rangeErrorNoLinkLeak=true --rssiRangeFromShadowing=true --arms=static_868,static_2p4,ca_tbl_full,fixed_threshold
```
- Configs: A (5 km) and B (`--boxHalf=750 --finderMode=1`).
- Variants: OFF, and ON = `--acquireParticles=true`.
- Seeds 1–20.
- Total: 80 runs.
- Frozen binary, manifest, outdir outside the repository.

## 3. Metrics
- Tag-0 CEP95 (`arm_summary.csv`), paired over seeds, mean [95 % bootstrap].
- Tag-0 median error over fix rows, t >= 120 s.
- Pooled tag-0 and subject median NEES and R95 coverage (`node_rows` as in V3).

## 4. Predictions
| # | prediction | confidence |
|---|---|---|
| **V4-1** | A: `ca_tbl_full` CEP95 ON − OFF interval below 0 | medium |
| **V4-2** | A: `static_2p4` tag-0 median error ON < OFF | medium |
| **V4-3** | A: `ca_tbl_full` coverage ON > OFF and median NEES ON < OFF | medium |
| **V4-4** | A: `static_868` CEP95 ON < OFF (paired mean) | medium |
| **V4-5** | B: `static_2p4` coverage ON >= 0.90 | medium |
| **V4-6** | B: `ca_tbl_full` CEP95 ON − OFF paired mean within +30 % of OFF | low |

## 5. Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`, or with a differing compile stamp.
- A manifest mismatch.
- Fewer seeds than registered.
