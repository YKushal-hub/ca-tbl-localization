# REAL-STACK results — in the realistic world the consistency stack ties Poucet in the endgame at 1/17 the energy, with more availability

Registration: `report/PREREG_REAL_STACK.md` (`8fc91dc9`, pushed before any cell). 80 cells, all `rc 0`, `source 8fc91dc9
dirty 0`, one frozen binary; launched 2026-10-18T16:46:38Z, done 17:03:15Z. Scored once, unedited, "Problems: 0".
Tracked: `results/real_stack_2026-10-18/`.

The world, in every cell: a BMI270-class gyro (0.15 °/s), 5 % per-step stride noise, 2 m finder GNSS error, ekf5 and a
magnetometer. M is that as given. S adds finder GNSS offsets as consider parameters and the relay gate.

## 1. Outcome

| # | prediction | outcome |
|---|---|---|
| P1 | S lowers CA-TBL CEP95 against M in the endgame | **held**: 10.5 → 4.4 m, −6.1 [−13.4, −0.8] |
| P2 | S costs CA-TBL no availability, lower bound above −0.01, in A and B | **not held** at 5 km: −0.011 [−0.028, +0.004] (the endgame +0.008 [−0.003, +0.021]) |
| P3 | under S, CA-TBL availability above Poucet in the endgame | **held**: +0.023 [+0.008, +0.037] |
| P4 | under S, CA-TBL − Poucet CEP95 in the endgame contains 0 | **held**: +0.8 [−0.1, +1.5] m |

**Decision rule: not proposed** (P2 failed narrowly; the 5 km availability interval spans 0 but its lower bound
crosses the registered −0.01). Both flags stay default-off with G100 and G101.

## 2. Against the goal, under S

| | endgame (B) | 5 km (A) |
|---|---|---|
| **vs Poucet** (UWB every cycle, 17× / 24× the energy) | availability **+0.023 [0.008, 0.037]**; CEP95 +0.8 [−0.1, 1.5]: tied | availability −0.004 [−0.030, 0.022]; CEP95 −63.5 [−510, 388]: tied |
| vs `fixed_threshold` | availability −0.002; CEP95 +0.1 [−0.5, 0.6]: tied | availability −0.010; CEP95 −98.6 [−240, 19]: tied |
| vs `static_2p4` | availability +0.337; CEP95 **−3.4 [−6.2, −1.2]** | availability +0.326; CEP95 −444 [−966, 94] |
| vs `static_868` | availability +0.181; CEP95 **−27.5 [−37.1, −19.6]** | availability +0.042; CEP95 +132 [−132, 475] |

In the endgame CA-TBL beats both single-band arms on both axes, ties `fixed_threshold`, and against Poucet has
higher availability at equal accuracy for a seventeenth of the energy. At 5 km every accuracy interval spans 0.
Coverage of CA-TBL's 95 % ellipse rises 0.460 → 0.803 in the endgame and 0.276 → 0.386 at 5 km.

## 3. What this cannot establish

A real GNSS error's time structure (a constant per-finder offset here), a real magnetometer, the ring, Wolf ToF, the
default; why S moves 5 km availability by −0.011 (the gate drops relays, which buy availability; not traced).
