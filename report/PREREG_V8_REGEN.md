# PREREG V8-REGEN — the adopted fixes together, each one removed in turn (exploratory grade)

**Registered before any V8 measurement cell runs** (rule 13). Written 2026-10-17. Engine: `ed2230e1`.
Owner's master plan v3, stage V8.

## 1. What is disclosed
Registered results this builds on: V1 (keyed draws), N.55 / V2 (the leak fix), V3 (consider biases: mixed, NOT
adopted), V4 (particle acquisition: consistency gain), V5 (C2b: NOT SUPPORTED, NOT adopted), V6 (on-demand: nothing
to save, NOT adopted), V7 (reply positions: the largest 5 km gain).

**Adopted set FULL:**
```
--rngKeyed --rangeErrorNoLinkLeak --rssiRangeFromShadowing --nlosRangeBias --rangeReplyPosition --acquireParticles
```
- `--rssiRangeFromShadowing` (realistic 868 MHz error) and `--rangeErrorNoLinkLeak` (its leak fixed) are
  physics-correctness flags.
- `--nlosRangeBias` is realism (NLOS is rare here).
- The other two are the improvements.

**Dev evidence:** the V4 and V7 campaigns individually. The two have never run together; that is the first thing this
campaign tests.

## 2. Cells
Common:
```
--specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true
--imuQuScale=10 --unknownStart=true --fixedVarThreshold=12 --foliageDepthClamp=true
--arms=static_868,static_2p4,ca_tbl_full,fixed_threshold
```

**Variants:**

| variant | flags |
|---|---|
| BASE | `--rngKeyed=true` only (the default physics with keyed draws) |
| FULL | the adopted set |
| NO_REPLY | FULL without `--rangeReplyPosition` |
| NO_PF | FULL without `--acquireParticles` |
| NO_NLOS | FULL without `--nlosRangeBias` |

**Configs:**

| config | flags |
|---|---|
| A | 5 km, the preset |
| B | `--boxHalf=750 --finderMode=1` |
| R8 | 5 km, `--nGateways=8 --gwLayout=legacy` (the ring, N.16) |
| BS | B with a static lost node, `--speed=0` |

**ToF robustness:** `--tofErrorModel=wolf`, BASE and FULL, configs A and B.

**Seeds** 1–20. Total: 5 × 4 × 20 + 2 × 2 × 20 = 480 runs. Frozen binary, manifest, outdir outside the repository.

## 3. Metrics (tag 0 unless stated; paired over seeds, mean [95 % bootstrap])
- CEP95 from `arm_summary.csv`.
- **CEP95 after acquisition:** the 95th percentile of tag-0 error over fix rows with t >= 300 s, per seed, then
  averaged. Added because the per-run CEP95 is dominated by the first minutes (V7 §2).
- Pooled tag-0 and subject R95 coverage.
- Fix-cycle share; tag-0 J.

## 4. Predictions (`ca_tbl_full` unless stated)
| # | prediction | confidence |
|---|---|---|
| **V8-1** | A: CEP95 FULL − BASE interval below 0 | high |
| **V8-2** | A: CEP95 NO_REPLY − FULL interval above 0 (reply positions are the main contributor) | high |
| **V8-3** | A: coverage FULL > NO_PF | medium |
| **V8-4** | R8: CEP95 FULL − BASE paired mean < 0 | medium |
| **V8-5** | BS: coverage FULL >= BASE | medium |
| **V8-6** | B: CEP95 after acquisition FULL − BASE paired mean <= 0 | medium |
| **V8-7** | wolf, A: CEP95 FULL − BASE paired mean < 0 | medium |
| **V8-8** | NO_NLOS vs FULL: \|CEP95 change\| < 10 % of FULL in every config | medium |
| **V8-9** | A: `static_2p4` CEP95 FULL − BASE paired mean < 0 | medium |

## 5. Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`, or with a differing compile stamp.
- A manifest mismatch.
- Fewer seeds than registered.
