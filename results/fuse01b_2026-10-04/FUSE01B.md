# FUSE-01b -- the registered analysis, applied unchanged

Paired by seed (on - off), 95 % seed-bootstrap intervals (10 000 resamples, seed 20261005).

## main

| arm, flag | availability | CEP50 / CEP95 tag 0 (m) | UWB-delivered fixes (sum; seeds > 0) | sessions with a defined spread | subject J |
|---|---|---|---|---|---|
| ca_tbl_full, off | 0.317 [0.295, 0.339] | 22.5 / 48.0 | 0; 0 | 0 of 20 | 13.68 |
| ca_tbl_full, on | 0.433 [0.406, 0.459] | 21.8 / 48.9 | 0; 0 | 0 of 20 | 15.61 |
| static_868, off | 0.397 [0.372, 0.421] | 19.1 / 34.9 | - | - | 6.08 |
| static_868, on | 0.397 [0.371, 0.421] | 19.1 / 34.9 | - | - | 6.08 |

- paired availability difference (on - off), ca_tbl_full: 0.116 [0.100, 0.133] over 20 seeds
- paired relative CEP95 change (on / off - 1), tag 0: 0.026 [-0.070, 0.124]
- availability, ca_tbl_full on - static_868: 0.036 [0.018, 0.056]
- fused sets (on): 39312; with no active range and a short floor: 18864; became fixes: 18252; UWB fixes from a fused set: 0

## end

| arm, flag | availability | CEP50 / CEP95 tag 0 (m) | UWB-delivered fixes (sum; seeds > 0) | sessions with a defined spread | subject J |
|---|---|---|---|---|---|
| ca_tbl_full, off | 0.652 [0.626, 0.679] | 5.2 / 9.5 | 0; 0 | 14 of 20 | 39.70 |
| ca_tbl_full, on | 0.773 [0.752, 0.793] | 5.4 / 10.3 | 525; 20 | 15 of 20 | 48.58 |
| static_868, off | 0.589 [0.560, 0.615] | 5.4 / 9.9 | - | - | 7.57 |
| static_868, on | 0.589 [0.561, 0.615] | 5.4 / 9.9 | - | - | 7.57 |

- paired availability difference (on - off), ca_tbl_full: 0.120 [0.109, 0.131] over 20 seeds
- paired relative CEP95 change (on / off - 1), tag 0: 0.137 [0.016, 0.279]
- availability, ca_tbl_full on - static_868: 0.184 [0.169, 0.199]
- fused sets (on): 27448; with no active range and a short floor: 6581; became fixes: 19931; UWB fixes from a fused set: 518

## Predictions (report/PREREG_FUSE01B.md sect 4)

- P1 (main: availability rises, interval above 0): HELD
- P2 (main: ca_tbl_full on no longer below static_868, interval reaching 0): HELD
- P3 (end: UWB-delivered fixes on >= 15 of 20 seeds): HELD (20)
- P4 (end: CEP95 not worse by more than 10 %, upper bound <= 0.10): FAILED

## Adoption (sect 6)

Mean availability difference >= -0.01 and mean relative CEP95 change <= +0.10 in both configurations: DO NOT ADOPT

