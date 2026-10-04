# FUSE-02 -- the registered analysis, applied unchanged

Paired by seed (on - off), 95 % seed-bootstrap intervals (10 000 resamples, seed 20261005).

## main

| arm, flag | availability | CEP50 / CEP95 tag 0 (m) | UWB-delivered fixes (sum; seeds > 0) | sessions with a defined spread | subject J |
|---|---|---|---|---|---|
| ca_tbl_full, off | 0.166 [0.147, 0.186] | 89.3 / 193.8 | 1; 1 | 0 of 20 | 12.63 |
| ca_tbl_full, on | 0.271 [0.245, 0.298] | 52.9 / 161.8 | 3; 2 | 0 of 20 | 13.38 |
| static_868, off | 0.165 [0.143, 0.188] | 289.2 / 559.2 | - | - | 6.10 |
| static_868, on | 0.165 [0.143, 0.189] | 289.2 / 559.2 | - | - | 6.10 |

- paired availability difference (on - off), ca_tbl_full: 0.105 [0.085, 0.126] over 20 seeds
- paired relative CEP95 change (on / off - 1), tag 0: 1.139 [-0.016, 2.735]
- availability, ca_tbl_full on - static_868: 0.106 [0.077, 0.136]
- fused sets (on): 30308; with no active range and a short floor: 17023; became fixes: 8441; UWB fixes from a fused set: 3

## end

| arm, flag | availability | CEP50 / CEP95 tag 0 (m) | UWB-delivered fixes (sum; seeds > 0) | sessions with a defined spread | subject J |
|---|---|---|---|---|---|
| ca_tbl_full, off | 0.417 [0.403, 0.431] | 8.4 / 17.2 | 0; 0 | 10 of 20 | 24.96 |
| ca_tbl_full, on | 0.543 [0.524, 0.563] | 8.1 / 17.7 | 65; 15 | 14 of 20 | 34.37 |
| static_868, off | 0.371 [0.356, 0.386] | 9.1 / 20.6 | - | - | 6.43 |
| static_868, on | 0.371 [0.356, 0.386] | 9.1 / 20.6 | - | - | 6.43 |

- paired availability difference (on - off), ca_tbl_full: 0.127 [0.111, 0.143] over 20 seeds
- paired relative CEP95 change (on / off - 1), tag 0: 0.043 [-0.046, 0.129]
- availability, ca_tbl_full on - static_868: 0.173 [0.159, 0.185]
- fused sets (on): 22010; with no active range and a short floor: 8490; became fixes: 10512; UWB fixes from a fused set: 64

## Predictions (report/PREREG_FUSE02.md sect 4)

- P1 (main: availability rises, interval above 0): HELD
- P2 (main: ca_tbl_full on no longer below static_868, interval reaching 0): HELD
- P3 (end: UWB-delivered fixes on >= 5 of 20 seeds): HELD (15)
- P4 (end: CEP95 not worse by more than 10 %, upper bound <= 0.10): FAILED

## Adoption (sect 6)

Mean availability difference >= -0.01 and mean relative CEP95 change <= +0.10 in both configurations: DO NOT ADOPT

