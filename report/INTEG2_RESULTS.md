# INTEG2 results: the integrity threshold re-fit on the final preset (T21; exploratory)

Registration: `report/PREREG_INTEG2.md` (`5ae26fd4`, before any cell). 120 cells, all `rc 0` and `source 5ae26fd4 dirty 0`.
Tracked: `results/integ2_2026-10-18/`.

## 1. Outcome
| # | prediction | outcome |
|---|---|---|
| I1 | the fit is below 1992.9 | **not held**: 9303.83 |
| I2 | mean recall higher at the fit | **not held**: 0.046 → 0.012 |
| I3 | mean precision ≥ base − 0.05 | **held**: 0.182 → 0.168 |

The registered recommendation is **do not recommend**. The preset keeps 1992.9.

## 2. Reading
The covariance consistency gained since FIXES-N26 (DD 14, DD 15) did not shorten the fault-free tail of the check's
distance. It lengthened it.

| cell | p50 | p90 | p99 |
|---|---|---|---|
| fault-free, 5 km | 6.4 | 2055 | 21 322 |
| fault-free, endgame | 1.8 | 122 | 2406 |

The p50s are near χ²₂'s 1.39 in the endgame, so the bulk is consistent. The tail is not: a few fault-free fixes per
run sit thousands of χ² units out (acquisition-era and NLOS-ranged fixes, whose error is not in R; N.15's hypothesis).
Any threshold quiet on fault-free runs is therefore blind to the injected faults (recall 0.003–0.023 at the fit,
0.03–0.10 at 1992.9).

**Conclusion for T21:** no re-fit makes the Mahalanobis integrity check a useful detector under this preset. It is
consistent with its withdrawal as a claim (Technical Report §0a). What could change it is the tail's cause (N.15): the
measurement covariance would have to carry NLOS and acquisition-state error. Recorded as a limit.

## 3. Fit
# FIXES-N26 — integrity threshold fit (scripts/fixes_n26_analyse.py fit)

| cell | eligible fix cycles | p50 | p90 | p99 |
|---|---|---|---|---|
| CAL_main | 5931 | 6.40 | 2055.47 | 21322.30 |
| CAL_end | 13327 | 1.81 | 122.10 | 2405.88 |

Pooled p99 9303.828 over 19258 cycles at t >= 120 s -> integrityChi2 = max(9.21, p99) = **9303.83**.


## 4. Fault tables
Fitted integrityChi2 = 9303.83 (preset 1992.9).

## base (1992.9)

## E: the SY-5 fault matrix (tag 0; label = the fault window)

| fault | recovery | arm | integrity precision | integrity recall | CEP95 in window (m) | CEP95 outside (m) | seeds |
|---|---|---|---|---|---|---|---|
| antenna | downweight | ca_tbl_full | 0.037 | 0.010 | 27.4 | 201.4 | 10 |
| antenna | downweight | ca_tbl_no_integrity | nan | 0.000 | 86.7 | 200.2 | 10 |
| rb_common | downweight | ca_tbl_full | 0.275 | 0.071 | 67.1 | 242.1 | 10 |
| rb_common | downweight | ca_tbl_no_integrity | nan | 0.000 | 93.2 | 198.8 | 10 |
| rb_single | downweight | ca_tbl_full | 0.292 | 0.074 | 90.8 | 200.3 | 10 |
| rb_single | downweight | ca_tbl_no_integrity | nan | 0.000 | 114.0 | 199.0 | 10 |
| txdrop | downweight | ca_tbl_full | 0.126 | 0.029 | 30.9 | 192.4 | 10 |
| txdrop | downweight | ca_tbl_no_integrity | nan | 0.000 | 89.0 | 198.8 | 10 |

## fit

## E: the SY-5 fault matrix (tag 0; label = the fault window)

| fault | recovery | arm | integrity precision | integrity recall | CEP95 in window (m) | CEP95 outside (m) | seeds |
|---|---|---|---|---|---|---|---|
| antenna | downweight | ca_tbl_full | 0.041 | 0.003 | 85.9 | 199.5 | 10 |
| antenna | downweight | ca_tbl_no_integrity | nan | 0.000 | 86.7 | 200.2 | 10 |
| rb_common | downweight | ca_tbl_full | 0.290 | 0.023 | 91.4 | 198.8 | 10 |
| rb_common | downweight | ca_tbl_no_integrity | nan | 0.000 | 93.2 | 198.8 | 10 |
| rb_single | downweight | ca_tbl_full | 0.246 | 0.018 | 114.9 | 207.1 | 10 |
| rb_single | downweight | ca_tbl_no_integrity | nan | 0.000 | 114.0 | 199.0 | 10 |
| txdrop | downweight | ca_tbl_full | 0.095 | 0.006 | 83.8 | 198.8 | 10 |
| txdrop | downweight | ca_tbl_no_integrity | nan | 0.000 | 89.0 | 198.8 | 10 |


mean over faults, ca_tbl_full: precision 0.182 -> 0.168; recall 0.046 -> 0.012

## Predictions

- **I1**: not held
- **I2**: not held
- **I3**: held

Registered recommendation (I1-I3 all hold): do not recommend
