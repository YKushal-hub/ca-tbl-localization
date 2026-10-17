# PREREG C4-P833 — ITU-R P.833-10 forest foliage on the final preset (exploratory)

**Registered before any cell runs** (rule 13). Written 2026-10-17; this commit adds this file, `scripts/c4_run.sh` and
`scripts/c4_analyse.py`. The engine is the parent's (`--foliageP833`, G148; TECH_DEBT O.8).

**Disclosed:** the probe, seeds 1–3, shows 868 MHz forest loss −1.5 dB and 2.4 GHz +4.5 dB (r80 156 → 112 m, seed 1).
No scenario run was made with the flag.

## Cells
Common:
```
--specProfile=true --duration=1800 --trace=true --unknownStart=true --imuQuScale=10 --fixedVarThreshold=12
--arms=static_868,static_2p4,ca_tbl_full,fixed_threshold
```
Configs: A (5 km), and B (`--boxHalf=750 --finderMode=1`).

| variant | flags |
|---|---|
| BASE | the preset |
| P833 | `--foliageP833=true` |

Seeds 1–20: 80 cells.

## Predictions (tag 0, paired over seeds, mean [95 % bootstrap])
| # | prediction |
|---|---|
| F1 | B: `static_2p4` CEP95 P833 − BASE > 0 (less 2.4 GHz reach) |
| F2 | B: `static_2p4` fix share lower under P833 |
| F3 | A: `static_868` CEP95 P833 − BASE ≤ 0 (1.5 dB less 868 MHz loss) |
| F4 | A and B: `ca_tbl_full` \|ΔCEP95\| < 15 % of BASE (the hybrid absorbs a band's change) |

## Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`.
- A manifest mismatch.
