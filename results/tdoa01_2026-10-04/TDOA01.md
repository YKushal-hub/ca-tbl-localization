# TDOA-01 -- the registered analysis, applied unchanged

NEES = e' R^-1 e over OK solves (chi^2_2 if R is right; median 1.386, mean 2). C95 = share with NEES <= 5.991, seed-bootstrap 95 % interval (10 000 resamples, seed 20261006).

| config | OK solves | NEES median / mean | C95 | C95, exactly 3 stamps | C95, 4+ stamps | share of OK with 3 stamps | (AMBIGUOUS + NO_ROOT) / 3-stamp attempts |
|---|---|---|---|---|---|---|---|
| p8 | 7543 | 1.352 / 4.125 | 0.941 [0.932, 0.950] | 0.944 [0.932, 0.954] | 0.938 [0.928, 0.947] | 0.605 | 0.016 (4639 attempts) |
| p3 | 370 | 1.341 / 2.079 | 0.930 [0.910, 0.950] | 0.930 [0.910, 0.951] | n/a | 1.000 | 0.000 (370 attempts) |
| end | 14967 | 1.456 / 3.296 | 0.915 [0.911, 0.920] | 0.904 [0.867, 0.944] | 0.915 [0.911, 0.920] | 0.011 | 0.228 (215 attempts) |

Attempts by verdict:

| config | OK | TOO_FEW | COLLINEAR | SINGULAR | NO_CONVERGE | BAD_INPUT | NO_ROOT | AMBIGUOUS |
|---|---|---|---|---|---|---|---|---|
| p8 | 7543 | 11301 | 0 | 0 | 5 | 0 | 31 | 43 |
| p3 | 370 | 15344 | 0 | 0 | 0 | 0 | 0 | 0 |
| end | 14967 | 267 | 0 | 0 | 81 | 0 | 18 | 31 |

Pooled over all configurations: C95 exactly 3 stamps 0.942 [0.930, 0.952], 4+ stamps 0.919 [0.915, 0.923].

## Predictions (report/PREREG_TDOA01.md sect 3)

- P1 (p8: the C95 interval inside [0.90, 0.99]): HELD
- P2 (p3: >= 80 % of OK solves with exactly 3 stamps): HELD (1.000)
- P3 (p3: AMBIGUOUS + NO_ROOT >= 10 % of 3-stamp attempts): FAILED (0.000)
- P4 (pooled: C95 of 3-stamp solves below 4+): FAILED

