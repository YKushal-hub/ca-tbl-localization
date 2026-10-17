# PREREG SIG868 — honest 868 MHz RSSI range σ (`--rssiSigmaLogNormal`) on the final preset (exploratory)

**Registered before any cell runs** (rule 13). Written 2026-10-17. This commit adds this file, `scripts/sig868_run.sh` and
`scripts/sig868_analyse.py`. The engine is unchanged; the flag is V3's (G141).

**Why.** CAMP-FINAL2's W_end showed the hybrid at 53 m against 2.4 GHz alone at 28 m (TECH_DEBT O.7). A diagnosis on
W_end's traces:
- the hybrid ranges the 868 MHz floor on 95 % of endgame cycles;
- its 2.4 GHz σ under Wolf is about 70 m (honest);
- so the overconfident 868 MHz RSSI ranges (NEES about 60, V8) dominate the fusion.

**Disclosed:** a 6-seed diagnostic on the W_end configuration, outside results/:

| variant | CEP95 | energy |
|---|---|---|
| BASE | 57.4 m | 82.8 J |
| `--rssiSigmaLogNormal` | 34.0 m | 97.2 J |
| `--dualBase868OnNeed` | 49.5 m | |
| `--loraDualBase=false` | 33.0 m | 51.1 J |

V3 (TECH_DEBT N.53 area) found the flag mixed under the old preset: `static_868` coverage up, `ca_tbl_full` CEP95 worse.

## Cells
Common:
```
--specProfile=true --duration=1800 --trace=true --unknownStart=true --imuQuScale=10 --fixedVarThreshold=12
--arms=static_868,static_2p4,ca_tbl_full,fixed_threshold
```
Configs: A (5 km); B (`--boxHalf=750 --finderMode=1`); W (B plus `--tofErrorModel=wolf --tofCalibrate=true`).

| variant | flags |
|---|---|
| BASE | the preset |
| SIG | `--rssiSigmaLogNormal=true` |

Seeds 1–20: 120 cells.

## Predictions (tag 0, paired, mean [95 % bootstrap])
| # | prediction |
|---|---|
| S1 | W: `ca_tbl_full` CEP95 SIG − BASE interval below 0 |
| S2 | A: `static_868` coverage (pooled tag 0 and subjects) higher under SIG |
| S3 | B: `ca_tbl_full` CEP95 SIG − BASE paired mean ≤ +15 % of BASE |
| S4 | A: `ca_tbl_full` CEP95 SIG − BASE paired mean ≤ +15 % of BASE |

**Adoption rule** (fixed now): the preset adopts `--rssiSigmaLogNormal` if S1, S3 and S4 all hold.

## Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`.
- A manifest mismatch.
