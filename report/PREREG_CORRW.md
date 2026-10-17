# PREREG CORRW — `--rssiCorrInflate` on accuracy, including calibrated Wolf ToF (exploratory)

**Registered before any cell runs** (rule 13). Written 2026-10-17. This commit adds this file, `scripts/corrw_run.sh`
and `scripts/corrw_analyse.py`. The engine is the parent's: the preset includes DD 14 and DD 15.

**Disclosed, and why this is a separate registration.**
- B2CONS judged CORR by a *consistency* rule, and it was not adopted: endgame hybrid coverage went 0.901 → 0.888.
- The owner's standing criterion is accuracy-first (STATE §5). This registration judges CORR on accuracy, with the
  rule fixed below.
- **Dev, 6 seeds, the current preset, calibrated-Wolf endgame:**

  | variant | hybrid CEP95 | energy |
  |---|---|---|
  | BASE | 40.6 m | 82 J |
  | CORR | 17.6 m | 84 J |
  | `--dualBase868OnNeed` | 31.4 m | |
  | 2.4 GHz alone | 21.5 m | 8.7 J |

- B2CONS (pre-DD-15, engine ToF): CORR's `ca_tbl_full` ΔCEP95 was A +12.8 [−35.0, +72.2] m and
  B −0.2 [−0.6, +0.3] m.

## Cells
Common:
```
--specProfile=true --duration=1800 --trace=true --unknownStart=true --fixedVarThreshold=12
--arms=static_868,static_2p4,ca_tbl_full,fixed_threshold
```
Configs:
- A (5 km);
- B (`--boxHalf=750 --finderMode=1`);
- W (B plus `--tofErrorModel=wolf --tofCalibrate=true`).

Variants: BASE (the preset), CORR (`--rssiCorrInflate=true`). Seeds 1–20: 120 cells.

## Predictions (tag 0, paired, mean [95 % bootstrap])
| # | prediction |
|---|---|
| W1 | W: `ca_tbl_full` ΔCEP95 interval below 0 |
| W2 | B: `ca_tbl_full` ΔCEP95 paired mean ≤ +10 % of BASE |
| W3 | A: `ca_tbl_full` ΔCEP95 paired mean ≤ +10 % of BASE |
| W4 | A: `static_868` ΔCEP95 paired mean < 0 |

**Adoption rule** (fixed now): adopt `--rssiCorrInflate=true` into the preset if W1, W2 and W3 all hold.

## Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`.
- A manifest mismatch.
