# EKF5-PRESET — a filter that learns the gyro's errors, on the preset

Registered in `report/PREREG_EKF5_PRESET.md` (`ebb1c756`) before any cell; 189 of 189 cells, every
`run_id.txt` `source ebb1c756 dirty 0`; scored once by the registered scorer, unedited, "Problems: 0"
(`results/ekf5_preset_2026-10-17/EKF5_PRESET.md`). `--imuFilter=ekf5` against DC1-BIAS's `cv` cells,
paired by seed, at DC-1 gyro bias L0 (8.816 °/h, the preset), L2 (0.15 °/s), L3 (0.5 °/s), on A (5 km),
B (endgame), C (N8 ring), six arms. The defect is TECH_DEBT N.28.

## Registered outcomes

| | prediction | outcome |
|---|---|---|
| R1 | 9 cv cells on this binary reproduce DC1-BIAS | **HOLDS** — 9 of 9; the cross-binary pairing is sound |
| P1 | at L3, ekf5 lowers CA-TBL tag 0 CEP95 in A and B | **HOLDS** — −1210.5 m [−1871.7, −586.9] (A), −239.1 m [−323.2, −170.8] (B) |
| P2 | in B, ekf5's mean CEP95 at L3 ≤ 66.2 m | **HOLDS** — 43.9 m |
| P3 | at L0, the mean per-seed CEP95 ratio ekf5/cv − 1 ≤ +0.10 in A, B, C | **HOLDS** — −0.329, −0.448, −0.654: it does not cost, it **helps** |

## CA-TBL (`ca_tbl_full`), tag 0 CEP95, cv → ekf5

| config | L0 8.816 °/h | L2 0.15 °/s | L3 0.5 °/s |
|---|---|---|---|
| A 5 km | 57.8 → **28.1** m | 1661.9 → **394.7** m | 1976.4 → **766.0** m |
| B endgame | 12.0 → **5.1** m | 303.5 → **15.8** m | 283.0 → **43.9** m |
| C N8 ring | 44.7 → **10.1** m | 971.8 → **146.7** m | 881.6 → **127.8** m |

Every reduction has an interval entirely below 0. Availability rises with ekf5 at L2/L3 in most
arms and configurations (CA-TBL at L3: +0.040 A, +0.055 B, +0.034 C, all above 0).

**Recovered where fixes are frequent, not where they are sparse.** In the endgame and on the ring
ekf5 brings a 0.5 °/s gyro back to within a few tens of metres. At 5 km, where fixes are sparse, it
recovers about 60 % of the loss and 766 m remains — the filter can only learn the bias between fixes.

**Even with the preset's near-perfect gyro, ekf5 is better** (P3), because it also learns the stride
scale and carries heading information the constant-velocity control predict discards. One exception
at L0: at 5 km `static_868`'s CEP95 rises, 117.0 → 201.7 m (+84.8 [15.0, 183.1]); Poucet and LoRa
fusion rise too, not significantly. Where ekf5 is not better, it is a tier-1-only or always-fused
arm at the sparsest density.

## The comparison table under ekf5

CA-TBL − comparator, paired by seed, under ekf5:

| config | level | − LoRa fusion: availability / CEP95 tag 0 | − Poucet: availability / CEP95 tag 0 |
|---|---|---|---|
| A | L0 | +0.072 / −98.6 m | +0.061 / −150.6 m |
| A | L3 | +0.059 / −790.8 m | +0.049 / −399.2 m |
| B | L0 | +0.061 / −6.7 m [−17.3, 0.0] (borderline) | +0.057 / +0.1 m (n.s.) |
| B | L3 | +0.069 / **+34.4 m** [9.9, 71.4] | +0.062 / **+36.6 m** [10.3, 75.5] |
| C | L0 | +0.023 / −11.2 m | +0.023 / −6.8 m |
| C | L3 | +0.016 (n.s.) / −6.0 m (n.s.) | +0.021 (n.s.) / +73.1 m (n.s.) |

- **CA-TBL's availability lead survives under ekf5** everywhere at L0 and in A and B at every level;
  on the ring at L3 it is not significant.
- **At 5 km under ekf5 CA-TBL is far more accurate** than both comparators at every level.
- **In the endgame at 0.5 °/s, CA-TBL is now LESS accurate than both comparators** (+34 to +37 m,
  intervals above 0): ekf5 brings LoRa fusion to 9.5 m and Poucet to 7.2 m, CA-TBL to 43.9 m. Not
  measured here why; a candidate is that the fused arms give the filter more position information
  per cycle to learn the bias from than CA-TBL's one tier at a time.
- **Energy:** in the endgame CA-TBL's subject energy rises by 6–9 J under ekf5 at L2/L3
  (`fixed_threshold` likewise), while Poucet's and LoRa fusion's fall; UWB-delivered fixes for
  CA-TBL there are 535 / 512 / 403 at L0 / L2 / L3. Elsewhere energy changes are small.

## What this establishes, and what it does not

**Establishes:** a filter that learns heading offset, gyro bias and stride scale removes most of the
collapse DC1-BIAS measured, and is better than the constant-velocity filter even with the preset's
near-perfect gyro, in every configuration, for CA-TBL. **Does not establish:** behaviour with a real
IMU (the filter's assumed model *is* DC-1's error model — a best case for model fit); anything
under misspecified noise or scale; why CA-TBL loses its endgame accuracy edge under ekf5; what the
default should be (the owner's decision, which would move every preset result in the tree).
