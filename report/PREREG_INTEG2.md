# PREREG INTEG2 — re-fit the integrity threshold on the final preset (T21; exploratory)

**Registered before any cell runs** (rule 13). Written 2026-10-18. This commit adds this file, `scripts/integ2_run.sh`
and `scripts/integ2_analyse.py`; the engine is the parent's.

**Why.** FIXES-N26 fitted `--integrityChi2` = 1992.9 when tag 0's covariance was 20–50× overconfident (N.30, acc line).
At that threshold false alarms vanished, and so did detection (recall 0.009–0.031; N.15). Since then DD 14 (exact
log-normal 868 σ) and DD 15 (IMU covariance ×30) have made the covariance far more consistent: endgame coverage 0.96,
5 km 0.69. The fault-free distance should therefore be smaller, and a re-fit threshold should detect more.

**Procedure: FIXES-N26's, unchanged** (`scripts/fixes_n26_analyse.py fit`). The threshold is
max(9.21, the 99th percentile of tag 0's Mahalanobis distance over judged fix cycles at t ≥ 120 s), pooled over 5 km
and the endgame. Common flags:
```
--specProfile=true --duration=1800 --trace=true --unknownStart=true --fixedVarThreshold=12
```

**Phase 1 (cal):** CAL_main and CAL_end (`--boxHalf=750 --finderMode=1`), `ca_tbl_full`, fault-free, seeds 21–40.
The fitted value is recorded in `fit.json` before phase 2 runs.

**Phase 2 (eval):** CAMP-FINAL2's fault matrix at 5 km: `rb_common`, `rb_single`, `txdrop`, `antenna`, recovery
`downweight`, arms `ca_tbl_full,ca_tbl_no_integrity`, seeds 1–10. Run twice:
- **base/**: the preset's 1992.9;
- **fit/**: `--integrityChi2=<fit>`.

120 cells.

## Predictions
| # | prediction |
|---|---|
| I1 | the fitted threshold is below 1992.9 |
| I2 | mean recall over the four faults is higher at the fit than at the base |
| I3 | mean precision at the fit is ≥ the base's − 0.05 |

**Adoption:** owner (TECH_DEBT_STATUS O-list). The registered rule recommends the fit if I1–I3 all hold.

## Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`.
- No judged fix cycle in phase 1.
