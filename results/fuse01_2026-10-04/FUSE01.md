# FUSE-01 -- the registered analysis, applied unchanged

Paired by seed (on - off), 95 % seed-bootstrap intervals (10 000 resamples, seed 20261005).

## main

| arm, flag | availability | CEP50 / CEP95 tag 0 (m) | UWB-delivered fixes (sum; seeds > 0) | sessions with a defined spread | subject J |
|---|---|---|---|---|---|
| ca_tbl_full, off | 0.333 [0.314, 0.355] | 21.9 / 47.3 | 0; 0 | 0 of 20 | 12.93 |
| ca_tbl_full, on | 0.451 [0.425, 0.478] | 20.1 / 47.6 | 4; 3 | 0 of 20 | 14.84 |
| static_868, off | 0.385 [0.361, 0.409] | 20.1 / 35.8 | - | - | 5.91 |
| static_868, on | 0.385 [0.361, 0.409] | 20.1 / 35.8 | - | - | 5.91 |

- paired availability difference (on - off), ca_tbl_full: 0.118 [0.103, 0.132] over 20 seeds
- paired relative CEP95 change (on / off - 1), tag 0: 0.030 [-0.080, 0.152]
- availability, ca_tbl_full on - static_868: 0.066 [0.055, 0.079]
- fused sets (on): 40345; with no active range and a short floor: 19171; became fixes: 19535; UWB fixes from a fused set: 4

## end

| arm, flag | availability | CEP50 / CEP95 tag 0 (m) | UWB-delivered fixes (sum; seeds > 0) | sessions with a defined spread | subject J |
|---|---|---|---|---|---|
| ca_tbl_full, off | 0.648 [0.620, 0.675] | 5.0 / 9.0 | 1; 1 | 15 of 20 | 37.92 |
| ca_tbl_full, on | 0.775 [0.748, 0.799] | 4.7 / 9.3 | 552; 20 | 14 of 20 | 46.58 |
| static_868, off | 0.586 [0.563, 0.607] | 5.4 / 9.1 | - | - | 7.41 |
| static_868, on | 0.586 [0.563, 0.607] | 5.4 / 9.1 | - | - | 7.41 |

- paired availability difference (on - off), ca_tbl_full: 0.127 [0.118, 0.136] over 20 seeds
- paired relative CEP95 change (on / off - 1), tag 0: 0.081 [-0.006, 0.184]
- availability, ca_tbl_full on - static_868: 0.189 [0.178, 0.200]
- fused sets (on): 27262; with no active range and a short floor: 6480; became fixes: 19867; UWB fixes from a fused set: 538

## Predictions (report/PREREG_FUSE01.md sect 5)

- P1 (main: availability rises, interval above 0): HELD
- P2 (main: ca_tbl_full on no longer below static_868, interval reaching 0): HELD
- P3 (end: UWB-delivered fixes on >= 15 of 20 seeds): HELD (20)
- P4 (end: CEP95 not worse by more than 10 %, upper bound <= 0.10): FAILED

## Adoption (sect 7)

Mean availability difference >= -0.01 and mean relative CEP95 change <= +0.10 in both configurations: ADOPT

