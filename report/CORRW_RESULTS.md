# CORRW results: `--rssiCorrInflate` judged on accuracy (exploratory)

Registration: `report/PREREG_CORRW.md` (`73e18879`, before any cell). 120 cells, all `rc 0` and
`source 73e18879 dirty 0`, launched 2026-10-17T18:05Z from `~/catbl_c4`. Tracked: `results/corrw_2026-10-17/`.
These cells run the preset with DD 14 and DD 15.

## 1. Outcome
| # | prediction | outcome |
|---|---|---|
| W1 | W: hybrid ΔCEP95 interval below 0 | **held**: −10.5 [−18.1, −3.5] m (34.4 → 23.8; 2.4 GHz alone 28.5) |
| W2 | B: hybrid ΔCEP95 ≤ +10 % | **held**: −0.5 [−1.8, +0.5] m on 5.0 |
| W3 | A: hybrid ΔCEP95 ≤ +10 % | **not held**: +36.8 [−58.5, +167.1] m on 365.1 (+10.1 %; bound 36.5 m) |
| W4 | A: `static_868` ΔCEP95 < 0 | **held**: −71.0 [−147.6, −10.8] m |

**Adoption rule** (W1, W2 and W3 all hold): **DO NOT ADOPT.** W3 missed by 0.3 m. Its interval is wide and spans 0,
but the rule is the rule. `--rssiCorrInflate` stays default-off.

## 2. Reading
- **TECH_DEBT O.10 (the hybrid under calibrated Wolf) moved twice:**

  | preset | hybrid | 2.4 GHz alone |
  |---|---|---|
  | before DD 14 | 53.4 m | 28.3 m |
  | with DD 14 and 15 (BASE here) | 34.4 m | 28.5 m |
  | with `--rssiCorrInflate` | **23.8 m** (beats 2.4 alone) | 28.5 m |

  The remaining cost is energy: about 80 J against 8.8 J, from about 20 % UWB occupancy. Escalation fires because
  Wolf's 2.4 GHz σ is wide. That is the escalation rule working as designed, and it is an accuracy/energy choice,
  not a defect.
- **868 MHz alone gains from the flag everywhere:** 5 km −71 m; endgame post-acquisition 24.6 → 11.9 m.
- **Deployment guidance (not a preset change):**
  - With calibrated real-hardware ToF errors, or 868 MHz alone, run `--rssiCorrInflate=true`.
  - With the engine's own ToF model at 5 km, the hybrid's mean CEP95 rises about 10 % (not significant).

## 3. Table
| config | variant | arm | CEP95 (m) | CEP95 after 300 s (m) | coverage | fix share | tag-0 J |
|---|---|---|---|---|---|---|---|
| A | BASE | static_868 | 447.6 | 412.4 | nan | 0.787 | 8.16 |
| A | BASE | static_2p4 | 402.7 | 89.1 | nan | 0.046 | 13.38 |
| A | BASE | ca_tbl_full | 365.1 | 259.8 | nan | 0.851 | 19.18 |
| A | BASE | fixed_threshold | 361.7 | 239.6 | nan | 0.851 | 18.92 |
| A | CORR | static_868 | 376.7 | 338.4 | nan | 0.788 | 8.12 |
| A | CORR | static_2p4 | 402.7 | 89.1 | nan | 0.046 | 13.38 |
| A | CORR | ca_tbl_full | 401.9 | 293.0 | nan | 0.842 | 20.17 |
| A | CORR | fixed_threshold | 418.2 | 301.0 | nan | 0.836 | 19.50 |
| B | BASE | static_868 | 27.5 | 24.6 | nan | 0.964 | 9.34 |
| B | BASE | static_2p4 | 12.3 | 10.6 | nan | 0.833 | 8.98 |
| B | BASE | ca_tbl_full | 5.0 | 4.0 | nan | 0.996 | 27.17 |
| B | BASE | fixed_threshold | 4.6 | 3.9 | nan | 0.996 | 28.25 |
| B | CORR | static_868 | 23.7 | 11.9 | nan | 0.964 | 9.25 |
| B | CORR | static_2p4 | 12.3 | 10.6 | nan | 0.833 | 8.98 |
| B | CORR | ca_tbl_full | 4.5 | 3.9 | nan | 0.995 | 29.72 |
| B | CORR | fixed_threshold | 4.8 | 3.8 | nan | 0.996 | 31.85 |
| W | BASE | static_868 | 27.5 | 24.6 | nan | 0.964 | 9.34 |
| W | BASE | static_2p4 | 28.5 | 25.4 | nan | 0.763 | 8.76 |
| W | BASE | ca_tbl_full | 34.4 | 31.9 | nan | 0.995 | 83.05 |
| W | BASE | fixed_threshold | 35.7 | 33.8 | nan | 0.995 | 79.99 |
| W | CORR | static_868 | 23.7 | 11.9 | nan | 0.964 | 9.25 |
| W | CORR | static_2p4 | 28.5 | 25.4 | nan | 0.763 | 8.76 |
| W | CORR | ca_tbl_full | 23.8 | 18.2 | nan | 0.996 | 80.03 |
| W | CORR | fixed_threshold | 25.4 | 19.8 | nan | 0.996 | 78.34 |

W1 W ca_tbl_full dCEP95 -10.5 [-18.1, -3.5] m on 34.4
W2 B ca_tbl_full dCEP95 -0.5 [-1.8, +0.5] m on 5.0
W3 A ca_tbl_full dCEP95 +36.8 [-58.5, +167.1] m on 365.1
W4 A static_868 dCEP95 -71.0 [-147.6, -10.8] m on 447.6

## Predictions

- **W1**: held
- **W2**: held
- **W3**: not held
- **W4**: held

Adoption rule (W1, W2, W3 all hold): DO NOT ADOPT
