# PREREG V7-REPLYPOS — finder positions carried in the ranging reply (exploratory grade)

**Registered before any V7 measurement cell runs** (rule 13). Written 2026-10-17. Engine: `c1f00fa8`
(`--rangeReplyPosition`, G144). Owner's master plan v3, stage V7; TECH_DEBT H.2.

## 1. What is disclosed
- **G144:** tag 0's finder-range median sigma 134 → 53 m with the flag.
- **Dev runs (4 seeds, 1800 s) were made on a build with a subject-entry defect in both variants**, fixed before
  commit (the preset null control caught it). They are indicative only:
  - 5 km CEP95: `ca_tbl_full` 485 → 230 m, `static_2p4` 623 → 321 m, `static_868` 1253 → 257 m;
  - endgame: `static_2p4` 39 → 18 m, `ca_tbl_full` 9.5 → 9.5 m, `static_868` 21 → 47 m;
  - tag-0 energy up 44–50 % for the hybrid arm, mostly more UWB exchanges.
- Billing: nothing extra (the position rides on the range's existing return path).

## 2. Cells
Common:
```
--specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true
--imuQuScale=10 --unknownStart=true --fixedVarThreshold=12 --foliageDepthClamp=true --rngKeyed=true
--rangeErrorNoLinkLeak=true --rssiRangeFromShadowing=true --arms=static_868,static_2p4,ca_tbl_full,fixed_threshold
```
- Configs: A (5 km) and B (`--boxHalf=750 --finderMode=1`).
- Variants: OFF, and ON = `--rangeReplyPosition=true`.
- Seeds 1–20.
- Total: 80 runs.

## 3. Predictions (tag-0 CEP95 from `arm_summary.csv`, paired over seeds, mean [95 % bootstrap])
| # | prediction | confidence |
|---|---|---|
| **V7-1** | A: `ca_tbl_full` ON − OFF interval below 0 | medium |
| **V7-2** | A: `static_868` ON − OFF interval below 0 | medium |
| **V7-3** | A: `static_2p4` ON − OFF paired mean < 0 | medium |
| **V7-4** | B: `static_2p4` ON − OFF paired mean < 0 | medium |
| **V7-5** | B: `ca_tbl_full` ON − OFF paired mean within ±25 % of OFF | low |
| **V7-6** | B: `ca_tbl_full` tag-0 energy ON >= 1.10 × OFF (paired mean; reported as a cost) | medium |

## 4. Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`, or with a differing compile stamp.
- A manifest mismatch.
- Fewer seeds than registered.
