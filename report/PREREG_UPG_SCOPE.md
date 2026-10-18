# PREREG UPG-SCOPE — the scope-2026-10-17 upgrades on the adopted preset (exploratory grade)

**Registered before any measurement cell runs** (rule 13). Written 2026-10-17. Engine: `be9b8f21`. The preset is
DESIGN_DECISIONS 10 + 11: the v3 fixes, SX1262 + SX1280.

## 1. What is disclosed
Dev runs (6 seeds, endgame, `ca_tbl_full`): `--hold24AfterK=3` gives tag-0 energy 30.3 → 27.6 J at CEP95 4.7 → 5.0 m.
Gate G147:
- wolf 2.4 GHz mean range error 53.8 → −2.1 m under `--tofCalibrate`;
- C2 learns from 83 pairs in 600 s;
- C1 applied 10 silent-gateway factors in 600 s.

No other measurement was made. B6 (`--agcPerReceiver`) is inert in the scenario (no link above the AGC knee; unit test
in G147) and is not measured here.

## 2. Cells
Common:
```
--specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true
--imuQuScale=10 --unknownStart=true --fixedVarThreshold=12 --arms=static_868,static_2p4,ca_tbl_full,fixed_threshold
```
- Configs: A (5 km) and B (`--boxHalf=750 --finderMode=1`).

**Variants:**

| variant | flags |
|---|---|
| BASE | the preset |
| C1 | `--acquireNegInfo=true` |
| C2 | `--tofRecal=true` |
| C3 | `--hold24AfterK=3` |
| B1 | `--relayNisInflate=true` |
| B7 | `--fadingPerBand=true` |
| WBASE | `--tofErrorModel=wolf` |
| WC5 | `--tofErrorModel=wolf --tofCalibrate=true` |

Seeds 1–20. Total: (6 × 2 + 2 × 2) × 20 = 320 runs.

## 3. Predictions (tag 0, paired over seeds, mean [95 % bootstrap]; coverage pooled over tag 0 and subjects)
| # | prediction | confidence |
|---|---|---|
| **U1** | A, C1: `ca_tbl_full` CEP95 − BASE paired mean <= 0 and coverage >= BASE | low |
| **U2** | A, C2: `static_868` CEP95 − BASE paired mean < 0 | low |
| **U3** | B, C3: `ca_tbl_full` tag-0 energy <= 0.95 × BASE (paired mean ratio) with CEP95 within +20 % | medium |
| **U4** | A, B1: `ca_tbl_full` coverage > BASE | medium |
| **U5** | B7: `ca_tbl_full` \|CEP95 − BASE\| < 10 % of BASE in both configs (a correctness fix, small effect) | medium |
| **U6** | B, WC5: `static_2p4` CEP95 − WBASE interval below 0 | high |

## 4. Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`, or with a differing compile stamp.
- A manifest mismatch.
- Fewer seeds than registered.
