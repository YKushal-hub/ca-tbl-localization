# B2CONS results: filter consistency at 5 km (scope B2; exploratory)

Registration: `report/PREREG_B2CONS.md` (`a62902d1`, before any cell). 120 cells, all `rc 0` and
`source a62902d1 dirty 0`, launched 2026-10-17T17:50Z from `~/catbl_c4` (build copied from `~/catbl_switch` at
`b8c351a9`, source-identical). Tracked: `results/b2cons_2026-10-17/`. Analyser: `scripts/b2cons_analyse.py`.

## 1. Outcome
| # | prediction | outcome |
|---|---|---|
| K1 | A, Q30: `ca_tbl_full` coverage up and CEP95 not worse | **not held**: coverage 0.647 → 0.692, but CEP95 +20.9 [−36.6, +100.0] m |
| K2 | A, CORR: `static_868` coverage up, CEP95 down | **held**: 0.384 → 0.584; −49.6 [−80.1, −14.7] m |
| K3 | A, CORR: hybrid post-acquisition CEP95 worse | **held**: +34.1 [−12.4, +92.2] m |
| K4 | B, Q30: `ca_tbl_full` coverage not lower | **held**: 0.901 → 0.959 |

**Adoption rule:**
- **Q30 ADOPT:** coverage rises in A and B, and CEP95 stays within +10 % (A +20.9 m on 344; B −0.1 m). The preset now
  sets `--imuQuScale=30` (DESIGN_DECISIONS 15).
- **CORR DO NOT ADOPT:** coverage falls in B (0.901 → 0.888).

## 2. Reading
- **The hybrid's consistency** is now 0.96 in the endgame, at the 0.95 target, and 0.69 at 5 km.
  - The 5 km gap that remains is not in the motion model: Q30 moved it only 0.05.
  - Nor is it in 868 MHz correlation: CORR moved it 0.03.
  - It is **not** acquisition-era. Under Q30, coverage by time window is 0.646 (120–300 s), 0.653 (300–900 s) and
    0.723 (900–1800 s).
  - **Where it is (Q30, A, t ≥ 900 s, tag 0 and subjects):**

    | cycle class | coverage |
    |---|---|
    | 2.4 GHz fix | 0.80–0.83 |
    | 868 MHz fix | 0.59–0.64 |
    | tag 0 coasting | 0.50 |
    | subjects coasting | 0.76 |

    The residual is overconfidence in 868 MHz fixes. TECH_DEBT O.12 records it.
- **`--rssiCorrInflate` is the right physics for 868 MHz alone:**
  - median NEES 36 → 8 (5 km) and 10.6 → 2.4 (endgame);
  - coverage 0.38 → 0.58 and 0.52 → 0.83;
  - 5 km CEP95 −50 m.

  It stays default-off because the registered rule judges the hybrid. A deployment that runs 868 MHz alone should use
  it.
- **Q30 costs 2.4 GHz alone a little in the endgame** (8.7 → 12.3 m). Its coverage rises to 0.94.

## 3. Table
| config | variant | arm | coverage | median NEES | CEP95 (m) | CEP95 after 300 s (m) | tag-0 J |
|---|---|---|---|---|---|---|---|
| A | BASE | static_868 | 0.384 | 36.10 | 451.9 | 417.6 | 8.18 |
| A | BASE | static_2p4 | 0.861 | 0.41 | 396.8 | 96.6 | 13.36 |
| A | BASE | ca_tbl_full | 0.647 | 3.77 | 344.1 | 213.3 | 18.20 |
| A | BASE | fixed_threshold | 0.613 | 4.43 | 359.8 | 232.9 | 18.23 |
| A | Q30 | static_868 | 0.433 | 22.16 | 447.6 | 412.4 | 8.16 |
| A | Q30 | static_2p4 | 0.919 | 0.22 | 402.7 | 89.1 | 13.38 |
| A | Q30 | ca_tbl_full | 0.692 | 2.44 | 365.1 | 259.8 | 19.18 |
| A | Q30 | fixed_threshold | 0.718 | 1.90 | 361.7 | 239.6 | 18.92 |
| A | CORR | static_868 | 0.584 | 7.89 | 402.3 | 358.7 | 8.14 |
| A | CORR | static_2p4 | 0.861 | 0.41 | 396.8 | 96.6 | 13.36 |
| A | CORR | ca_tbl_full | 0.675 | 2.49 | 357.0 | 247.4 | 17.29 |
| A | CORR | fixed_threshold | 0.680 | 2.91 | 392.8 | 300.5 | 19.46 |
| B | BASE | static_868 | 0.520 | 10.56 | 25.1 | 21.9 | 9.33 |
| B | BASE | static_2p4 | 0.895 | 0.85 | 8.7 | 7.7 | 8.96 |
| B | BASE | ca_tbl_full | 0.901 | 1.42 | 5.0 | 4.4 | 27.56 |
| B | BASE | fixed_threshold | 0.902 | 1.43 | 4.7 | 4.2 | 27.01 |
| B | Q30 | static_868 | 0.593 | 7.85 | 27.5 | 24.6 | 9.34 |
| B | Q30 | static_2p4 | 0.939 | 0.49 | 12.3 | 10.6 | 8.98 |
| B | Q30 | ca_tbl_full | 0.959 | 0.80 | 5.0 | 4.0 | 27.17 |
| B | Q30 | fixed_threshold | 0.959 | 0.79 | 4.6 | 3.9 | 28.25 |
| B | CORR | static_868 | 0.831 | 2.37 | 27.3 | 15.5 | 9.24 |
| B | CORR | static_2p4 | 0.895 | 0.85 | 8.7 | 7.7 | 8.96 |
| B | CORR | ca_tbl_full | 0.888 | 1.34 | 4.9 | 4.2 | 25.43 |
| B | CORR | fixed_threshold | 0.889 | 1.32 | 6.1 | 4.9 | 27.63 |

K1 A Q30 ca_tbl_full coverage 0.647 -> 0.692; dCEP95 +20.9 [-36.6, +100.0] m
K2 A CORR static_868 coverage 0.384 -> 0.584; dCEP95 -49.6 [-80.1, -14.7] m
K3 A CORR ca_tbl_full d(CEP95 after 300 s) +34.1 [-12.4, +92.2] m
K4 B Q30 ca_tbl_full coverage 0.901 -> 0.959

## Predictions

- **K1**: not held
- **K2**: held
- **K3**: held
- **K4**: held

## Adoption rule (per variant)

- **Q30**: ADOPT (A: coverage 0.647 -> 0.692, dCEP95 +20.9 [-36.6, +100.0] on 344.1; B: coverage 0.901 -> 0.959, dCEP95 -0.1 [-1.0, +1.1] on 5.0)
- **CORR**: DO NOT ADOPT (A: coverage 0.647 -> 0.675, dCEP95 +12.8 [-35.0, +72.2] on 344.1; B: coverage 0.901 -> 0.888, dCEP95 -0.2 [-0.6, +0.3] on 5.0)
