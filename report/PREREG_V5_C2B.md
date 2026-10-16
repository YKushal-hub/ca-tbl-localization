# PREREG V5-C2B — escalation on reachability instead of confidence, in CA-TBL (exploratory grade)

**Registered before any V5 measurement cell runs** (rule 13). Written 2026-10-17. Engine: the commit adding
`--c2bEscalation` (G145). Owner's master plan v3, stage V5; claim C2b.

## 1. What is disclosed
- **Stage A A2 (registered):** "attempt whenever plausibly reachable" held in the default setting but in only 1 of
  10 grid cases; never gating on confidence was robust there, for 868 → 2.4.
- **In CA-TBL, 868 → 2.4 is already attempted every cycle under the dual base**, so `--c2bEscalation` acts on
  2.4 → UWB.
- **Dev runs (4 seeds, 1800 s):**

  | config | arm | UWB cycles | tag-0 J | CEP95 | CEP50 |
  |---|---|---|---|---|---|
  | B | `ca_tbl_full` | 39 → 82 | 25.7 → 40.4 | 10.3 → 18.2 m | 2.7 → 2.9 m |
  | B | `fixed_threshold` | | 25.1 → 36.6 | 7.4 → 11.4 m | |
  | A | `ca_tbl_full` | none reachable | | | 88 → 63 m |

- So the dev evidence is that C2b on 2.4 → UWB costs energy and tail accuracy. The predictions below say so.

## 2. Cells
Common:
```
--specProfile=true --duration=1800 --trace=true --imuQuScale=10 --unknownStart=true --fixedVarThreshold=12
--foliageDepthClamp=true --rngKeyed=true --rangeErrorNoLinkLeak=true --rssiRangeFromShadowing=true
--arms=static_2p4,ca_tbl_full,fixed_threshold
```
- Configs: A (5 km) and B (`--boxHalf=750 --finderMode=1`).
- Variants: OFF, and C2B = `--c2bEscalation=true`.
- Seeds 1–20.
- Total: 80 runs.
- Frozen binary, manifest, outdir outside the repository.

## 3. Predictions (tag 0, `arm_summary.csv`, paired over seeds, mean [95 % bootstrap])
| # | prediction | confidence |
|---|---|---|
| **V5-1** | B: `ca_tbl_full` CEP95 C2B − OFF > 0 (worse) | medium |
| **V5-2** | B: `ca_tbl_full` tag-0 energy C2B >= 1.3 × OFF | medium |
| **V5-3** | B: `ca_tbl_full` CEP50 C2B − OFF within ±20 % of OFF (no median gain) | medium |
| **V5-4** | A: `ca_tbl_full` CEP50 C2B − OFF < 0 | low |
| **V5-5** | `static_2p4` identical in both variants (40 pairs, trace) | high |

**C2b in CA-TBL is supported only if** B CEP95 is not worse (V5-1 fails) **and** the energy rise is under 30 % (V5-2
fails). The registered expectation is NOT SUPPORTED.

## 4. Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`, or with a differing compile stamp.
- A manifest mismatch.
- Fewer seeds than registered.
