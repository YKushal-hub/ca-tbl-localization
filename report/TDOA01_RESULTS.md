# TDOA-01 — results: tier-1 TDoA's reported covariance is calibrated at 5 km to within the registered band, a little overconfident in the endgame, with a heavy tail everywhere; the exactly-3 closed form never refused at three gateways

**Registered in `report/PREREG_TDOA01.md` (`5b839d0`, engine `58b12eb`) before any cell
ran.** All 60 cells exit 0 and read `source 5b839d0 dirty 0`. The outputs, commands and
digests are in results/tdoa01_2026-10-04/ (`RUN.md`). The analysis is
`scripts/tdoa01_analyse.py`, committed with the registration and applied unchanged. Its
table is `TDOA01.md` there.

**In one paragraph.** Over 22 880 TDoA solves for tag 0 (both arms, 20 seeds, three
configurations), the solve's own reported covariance R holds the error about as often as it
claims:
- at 5 km with eight gateways, 94.1 % of solves fall inside their 95 % ellipse;
- with three gateways, 93.0 %;
- in the endgame, 91.5 %.

The median NEES is 1.35-1.46, against χ²₂'s 1.39. But the mean is 2-4, against 2: the tail
is heavy. In the endgame 2.5 % of solves are beyond χ²₂'s 0.999 point, where 0.1 % should
be. With three gateways the closed form solved 370 times and never refused. The binding
limit there is that all three gateways rarely stamp at once: 15 344 attempts were TOO_FEW.

---

## 1. The table (OK solves; C95 = share with NEES ≤ 5.991, 95 % seed bootstrap)

| config | OK solves | NEES median / mean | C95 | C95, 3 stamps | C95, 4+ stamps | share with 3 stamps | (AMBIGUOUS + NO_ROOT) / 3-stamp attempts |
|---|---|---|---|---|---|---|---|
| p8 (preset, 8 gateways, 5 km) | 7 543 | 1.352 / 4.125 | **0.941** [0.932, 0.950] | 0.944 | 0.938 | 0.605 | 0.016 (4 639) |
| p3 (3 gateways, 5 km) | 370 | 1.341 / 2.079 | 0.930 [0.910, 0.950] | 0.930 | — | 1.000 | **0.000** (370) |
| end (750 m, finders converging) | 14 967 | 1.456 / 3.296 | 0.915 [0.911, 0.920] | 0.904 | 0.915 | 0.011 | 0.228 (215) |

Attempts that did not solve, by verdict:

| config | TOO_FEW | NO_CONVERGE | NO_ROOT | AMBIGUOUS |
|---|---|---|---|---|
| p8 | 11 301 | 5 | 31 | 43 |
| p3 | 15 344 | 0 | 0 | 0 |
| end | 267 | 81 | 18 | 31 |

None was COLLINEAR, SINGULAR or BAD_INPUT.

## 2. Predictions

- **P1 held.** p8's C95 interval, [0.932, 0.950], lies inside [0.90, 0.99].
- **P2 held.** All 370 of p3's OK solves had exactly three stamps.
- **P3 failed.** No AMBIGUOUS or NO_ROOT in p3's 370 three-stamp attempts. With three
  gateways on a 3 km ring at 5 km, when all three stamp, the two roots are always told apart
  by the RSSI ranges or only one is real. The refusals happen elsewhere: 1.6 % of p8's
  three-stamp attempts, 22.8 % of the endgame's (where three-stamp attempts are rare, 215).
- **P4 failed.** Pooled, three-stamp solves are no worse calibrated than four-or-more: C95
  0.942 against 0.919. The closed form's root choice did not cost calibration where it ran.

## 3. What else the data say (descriptive)

- **The heavy tail.** Beyond χ²₂'s 0.999 point (13.8) lie 0.9 % of p8's solves and 2.5 % of
  the endgame's, where 0.1 % would be expected. The CRLB-based R holds the bulk and misses
  the outliers. J.2 item 4 predicted that: the bound has no threshold effect.
- **By arm.** C95 is 0.941 / 0.942 (`ca_tbl_full` / `static_868`) in p8, 0.909 / 0.938 in
  p3, and 0.870 / 0.920 in the endgame. In the endgame the full arm's TDoA solves are its
  rarer floor fixes, and they are more often overconfident.
- **The errors themselves.** Median 53 m and 95th percentile 334 m at 5 km (p8); 66 / 273 m
  with three gateways; 7.4 / 35 m in the endgame. TDoA at 5 km is coarse, and its R says
  so.

## 4. What follows

- **J.2 item 4, for the preset's geometry:** the CRLB σ is calibrated in its bulk at 5 km
  (C95 0.94) and in the registered band in the endgame (0.915). It is overconfident in the
  tail. Recorded in TECH_DEBT J.2.
- **No configuration fell below 0.90**, so the registration's "inflate σ" item is not
  triggered. The endgame's full-arm subset (0.870) is descriptive, not the registered test.
  An outlier-robust treatment of the tail (an NIS gate on TDoA, or a heavier-tailed R) would
  be its own flagged item.
- **Nothing is adopted.** `--tier1Tdoa` stays off by default; that is the owner's J.2
  decision.
