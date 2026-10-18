# REAL-BEST results — out of sample, the realistic-world stack is not beaten on accuracy by any comparator

Registration: `report/PREREG_REAL_BEST.md` (`f581e9f1`, pushed before any cell). 120 cells on **seeds 21–40, never
run by this line before**; all `rc 0`, `source f581e9f1 dirty 0`, one frozen binary; launched 2026-10-18T17:21:23Z, done
17:46:41Z. Scored once, unedited, "Problems: 0". Tracked: `results/real_best_2026-10-18/`.

The world, in every cell: a BMI270-class gyro (0.15 °/s), 5 % per-step stride noise, 2 m finder GNSS error, ekf5 and a
magnetometer (M). S adds finder GNSS offsets as consider parameters and the relay gate. F adds state bounds. Every arm
runs under the same variant.

## 1. Outcome

| # | prediction | outcome |
|---|---|---|
| P1 | F lowers CA-TBL CEP95 against M in the endgame | **held**: 10.6 → 4.0 m, −6.6 [−13.0, −1.3] |
| P2 | F costs CA-TBL no availability (lower bound > −0.01), A and B | **held**: A +0.013 [−0.001, +0.027], B +0.003 [−0.004, +0.011] |
| P3 | under F in the endgame: CA-TBL ties Poucet's CEP95, beats its availability | **held**: CEP95 −0.3 [−1.4, +0.5]; availability +0.011 [+0.002, +0.020] |
| P4 | under F no comparator significantly more accurate than CA-TBL, A or B | **held**: 8 of 8 contrasts |
| P5 | the bounds lower CA-TBL CEP95 against S at 5 km | **not held**: −16.7 [−110.4, +71.7] |

**Decision rule: P1, P2 and P4 held, so the realistic world and the full stack (S) are put to the owner together
as the preset.** The bounds (F − S) are not supported on their own: P5 failed; they stay a default-off option.

## 2. Against the goal, under F (CA-TBL − comparator, paired by seed)

| comparator | endgame availability | endgame CEP95 (m) | 5 km availability | 5 km CEP95 (m) | energy |
|---|---|---|---|---|---|
| Poucet (UWB every cycle) | **+0.011 [0.002, 0.020]** | −0.3 [−1.4, 0.5] (tie) | +0.005 (tie) | −137 [−443, 135] (tie) | CA-TBL 1/20 |
| `fixed_threshold` | **−0.010 [−0.017, −0.002]** | −0.1 (tie) | +0.006 (tie) | +15 (tie) | equal |
| `static_2p4` | **+0.319** | **−2.3 [−3.8, −1.1]** | **+0.326** | **−714 [−1108, −346]** | — |
| `static_868` | **+0.152** | **−42.8 [−60.0, −28.9]** | **+0.057** | −185 [−387, 29] (tie) | — |

- **Accuracy: no comparator is significantly more accurate than CA-TBL** in either configuration, out of sample.
  CA-TBL beats both single-band arms in the endgame and `static_2p4` at 5 km, and ties Poucet and
  `fixed_threshold` everywhere else.
- **Availability:** CA-TBL beats Poucet and both single-band arms in the endgame, and ties at 5 km. The one
  significant deficit is **`fixed_threshold` in the endgame, −0.010** — the same stack under a fixed-variance
  escalation rule in place of PCS.
- **Energy:** CA-TBL matches Poucet's endgame accuracy at about 1/20 of its tag energy.
- **The stack itself (S − M):** the endgame 10.6 → 4.2 m; 5 km 771.7 → 514.8 m (−256.9 [−568.8, −31.4]);
  coverage 0.415 → 0.823 (endgame) and 0.240 → 0.396 (5 km).

## 3. What this cannot establish

Real sensors beyond these models; the ring; Wolf ToF; seeds beyond 40; whether CA-TBL's PCS rule can close the
endgame availability gap to `fixed_threshold` (−0.010).
