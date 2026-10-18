# REAL-IMU results — with a realistic IMU the 5-state filter is required; the acquisition guard is not supported

Registration: `report/PREREG_REAL_IMU.md` (`92152dc7`, pushed before any cell). 160 cells, all `rc 0`,
`source 92152dc7 dirty 0`, one frozen binary checked before every cell. Scored once, unedited, "Problems: 0".
`results/real_imu_2026-10-18/RUN.md` records a voided first run (unrelated source edits in the launch tree while it
ran; binary identical; every summary byte-identical to this run's) and a reboot with a resume.

## 1. Outcome

| # | prediction | outcome |
|---|---|---|
| P1 | ekf5 lowers CA-TBL tag 0 CEP95 against cv (guard off), A and B | **held**: A −680.0 [−951.5, −382.9] m (1545.0 → 865.1), B −114.7 [−154.3, −79.6] m (121.8 → 7.2) |
| P2 | the guard lowers CA-TBL CEP95 under ekf5 in A; ≤ +10 % in B | **not held**: A +95.3 [−201.4, +432.6] m; B mean per-seed ratio +0.529 |
| P3 | CA-TBL availability above every comparator, ekf5 with the guard | **not held**: at 5 km −0.011 [−0.020, −0.003] against `fixed_threshold`, −0.016 [−0.024, −0.007] against Poucet |
| P4 | CA-TBL − Poucet CEP95 below 0 at 5 km, above 0 in the endgame | **not held** at 5 km (+251.6 [−109.9, +668.7], interval spans 0); the endgame half held (+2.6 [+0.6, +5.2]) |

**Decision rule:** P1 held, so **ekf5 is put to the owner as the filter for a realistic IMU**. P2 failed, so the
guard is not proposed; it stays default-off with G98.

## 2. What it shows

- **The preset's headline accuracy depends on its near-perfect IMU.** With a BMI270-class gyro and 5 % per-step
  stride noise, the preset's own filter (cv) gives CA-TBL CEP95 1545 m at 5 km and 122 m in the endgame, against
  365 m and 5.0 m under DC-1's defaults (CORRW BASE). Every arm degrades alike.
- **ekf5 recovers most of it, for every arm**: CA-TBL 5 km 1545 → 865 m, endgame 122 → 7.2 m.
- **The guard's pilot gain was selection.** On 6 + 4 hand-picked seeds it lowered CA-TBL's mean 361 → 194 m; on
  the unselected 20 the 5 km change spans 0 and one seed blows up (per-seed ratio +9.6).
- **Against the goal (CA-TBL beating every comparator):**
  - The endgame under ekf5 with the guard: CA-TBL 5.3 m ties `fixed_threshold` (−1.3 [−4.3, +1.5]) and beats
    `static_2p4` and `static_868`. **Poucet is more accurate, 2.6 m, at 17× the energy** (CA-TBL − Poucet subject J
    −352 J). Poucet ranges UWB on every cycle (380 J for tag 0, against CA-TBL's 30 J at a 4 % UWB share).
  - At 5 km, CA-TBL is behind Poucet by 251.6 m (interval spans 0) and slightly behind on availability. Neither
    reaches UWB there; the gap is CA-TBL's catastrophic seeds: heading drift, and acquisition far from the truth.

## 3. Table (tag 0, mean over 20 seeds)

| config | filter | guard | `ca_tbl_full` | `fixed_threshold` | `static_868` | `static_2p4` | `poucet_fusion` |
|---|---|---|---|---|---|---|---|
| A | cv | off | 1545.0 | 1518.7 | 2005.4 | 1658.6 | 1450.2 |
| A | ekf5 | off | 865.1 | 845.3 | 1476.9 | 1312.9 | 664.8 |
| A | ekf5 | on | 960.3 | 1039.6 | 1585.1 | 1439.8 | 708.7 |
| B | cv | off | 121.8 | 145.1 | 364.8 | 94.6 | 118.4 |
| B | ekf5 | off | 7.2 | 7.9 | 56.0 | 12.7 | 2.6 |
| B | ekf5 | on | 5.3 | 6.5 | 117.9 | 15.9 | 2.6 |

Availability, coverage, NEES, energy and every interval are in `REAL_IMU.md`.

## 4. What this cannot establish

A real BMI270 beyond DC-1's model with these two parameters; whether a heading reference (a magnetometer) removes
the 5 km heading failures (none was modelled here); the ring; Wolf ToF; the default.
