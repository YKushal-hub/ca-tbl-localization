# MAG results — a magnetometer halves the heading failures; the endgame gain is not significant; Poucet still leads there

Registration: `report/PREREG_MAG.md` (`4412fac3`, pushed before any cell). 42 cells, all `rc 0`, `source 4412fac3
dirty 0`, one frozen binary; launched 2026-10-18T15:32:23Z, done 15:40:44Z. Scored once, unedited, "Problems: 0".
Tracked: `results/mag_2026-10-18/`. Paired by seed with REAL-IMU's ekf5 cells (guard off).

## 1. Outcome

| # | prediction | outcome |
|---|---|---|
| R1 | 2 of 2 no-magnetometer cells reproduce REAL-IMU | **held** |
| P1 | the magnetometer lowers CA-TBL CEP95 in B | **not held**: 7.2 → 4.3 m, −2.9 [−7.0, +0.5] — the interval crosses 0 |
| P2 | fewer CA-TBL heading divergences (> 0.3 rad), A and B | **held**: 10 → 5 of 40 |
| P3 | B, CA-TBL − Poucet upper bound below +1.0 m | **not held**: +1.7 [+0.4, +3.5] m |

**Decision rule: not proposed** (P1 failed). `--imuMagSigmaDeg` stays a default-off option with G99.

## 2. What it shows

- **It bounds heading for every arm.** Divergences fall for all five arms: CA-TBL 10 → 5, `fixed_threshold`
  10 → 4, `static_868` 19 → 0, `static_2p4` 15 → 1, Poucet 8 → 3. 5 km CEP95 falls for every arm, CA-TBL by
  279 m (865.1 → 586.1, interval [−657.8, +32.7]) and `static_868` by 971 m.
- **Against the goal, with the magnetometer:**
  - **Availability:** CA-TBL is not below any comparator in either configuration (5 km: +0.003 to +0.338;
    endgame +0.001 to +0.327; the two smallest intervals span 0).
  - **5 km accuracy:** CA-TBL ties Poucet (−0.4 [−411.2, +427.1] m) at 1/24 of its energy, and trails
    `fixed_threshold` by +90.2 [−53.6, +281.7] (interval spans 0).
  - **Endgame accuracy:** CA-TBL beats `fixed_threshold` (−4.6 [−9.3, −0.7]) and `static_868`, ties
    `static_2p4`, and trails Poucet by 1.7 m (4.3 against 2.6), which ranges UWB every cycle at 17× the energy.
- **What remains at 5 km:** 5 of 40 CA-TBL seed-cells still end with heading > 0.3 rad off despite the reference.
  The range innovations drive the heading state against a reference weighted at the independent-equivalent
  variance (≈ 29° per reading). Coverage at 5 km is unchanged (0.329 → 0.332).

## 3. What this cannot establish

A real magnetometer's error (the disturbance model is ours), indoor or near-metal operation, the ring, Wolf ToF,
the default.
