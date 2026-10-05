# UWB-NLOS registered cells: analysis

Directory `/home/nethi/catbl_uwbnlos/registered`; registration `report/PREREG_UWB_NLOS.md`.

## Completeness and provenance (sect 7)

- A_802: 10 of 10 seeds read; steps per seed [1440] (1440 expected)
- A_leg: 10 of 10 seeds read; steps per seed [1440] (1440 expected)
- B_802: 10 of 10 seeds read; steps per seed [1440] (1440 expected)
- B_leg: 10 of 10 seeds read; steps per seed [1440] (1440 expected)

Every cell complete; every sidecar `dirty 0` with verified digests.

## Pooled UWB matrix per cell (10 seeds)

| cell | tp | fp | fn | tn | unscored LOS | unscored NLOS | lost LOS | lost NLOS | decoded rows | decoded NLOS |
|---|---|---|---|---|---|---|---|---|---|---|
| A_802 | 0 | 7 | 2 | 6936 | 310 | 28 | 512 | 1445 | 7283 | 30 |
| A_leg | 0 | 6 | 0 | 7912 | 1044 | 0 | 2988 | 10 | 8962 | 0 |
| B_802 | 0 | 0 | 0 | 696 | 334 | 2 | 981 | 459 | 1032 | 2 |
| B_leg | 0 | 0 | 0 | 936 | 492 | 0 | 1635 | 4 | 1428 | 0 |

## Predictions (sect 5)

- **P1** (A_802: tp >= 1 and fn >= 1): tp 0, fn 2 -> **FALSIFIED**
- **P2** (A_leg, B_leg: tp = fn = 0; NLOS polls <= 3 and <= 15): A_leg tp 0 fn 0 NLOS polls 10; B_leg tp 0 fn 0 NLOS polls 4 -> **FALSIFIED**
- **P3** (B_802: lostNlos >= 50; <= 2 decoded NLOS, each within 3 m): lostNlos 459, decoded NLOS 2 at d_3D [4.38, 1.56] m -> **FALSIFIED**
- **P4** (both _802: NLOS rows carry dPL(distM) to 1e-6 dB, LOS rows 0): violations A_802 0, B_802 0 over 30 + 2 NLOS rows -> **HOLDS**
- **P5** (descriptive; A_802 mean score NLOS > LOS): NLOS 0.0000 over 2 scored rows, LOS 0.0008 over 6943 -> **FALSIFIED**

## Decoded NLOS rows (seed, d_3D m, SNR dB, nlosDb, score)

- A_802: 30 rows
  - seed 7: 2.803 m, -29.88 dB, 30.7528 dB, score -1.0000
  - seed 7: 3.800 m, -33.92 dB, 31.7307 dB, score -1.0000
  - seed 7: 1.504 m, -30.85 dB, 28.7531 dB, score -1.0000
  - seed 7: 1.544 m, -23.92 dB, 28.8373 dB, score -1.0000
  - seed 7: 2.346 m, -21.11 dB, 30.1816 dB, score -1.0000
  - seed 7: 1.616 m, -18.73 dB, 28.9824 dB, score 0.0000
  - seed 8: 3.218 m, -35.60 dB, 31.1971 dB, score -1.0000
  - seed 8: 4.002 m, -33.56 dB, 31.8979 dB, score -1.0000
  - seed 9: 1.902 m, -20.71 dB, 29.5064 dB, score -1.0000
  - seed 9: 1.602 m, -22.32 dB, 28.9549 dB, score -1.0000
  - seed 9: 3.756 m, -32.71 dB, 31.6936 dB, score -1.0000
  - seed 9: 1.654 m, -19.99 dB, 29.0575 dB, score -1.0000
  - seed 10: 2.387 m, -30.51 dB, 30.2371 dB, score -1.0000
  - seed 11: 2.030 m, -25.73 dB, 29.7167 dB, score -1.0000
  - seed 11: 1.987 m, -29.71 dB, 29.6479 dB, score -1.0000
  - seed 11: 2.195 m, -28.25 dB, 29.9674 dB, score -1.0000
  - seed 12: 2.994 m, -34.96 dB, 30.9648 dB, score -1.0000
  - seed 12: 2.511 m, -26.19 dB, 30.3994 dB, score -1.0000
  - seed 12: 1.467 m, -33.72 dB, 28.6720 dB, score -1.0000
  - seed 13: 2.036 m, -27.44 dB, 29.7253 dB, score -1.0000
  - seed 13: 1.323 m, -30.16 dB, 28.3412 dB, score -1.0000
  - seed 13: 2.983 m, -33.70 dB, 30.9530 dB, score -1.0000
  - seed 14: 3.540 m, -36.23 dB, 31.5030 dB, score 0.0000
  - seed 14: 2.731 m, -28.61 dB, 30.6696 dB, score -1.0000
  - seed 14: 3.144 m, -23.16 dB, 31.1225 dB, score -1.0000
  - seed 14: 2.912 m, -25.38 dB, 30.8756 dB, score -1.0000
  - seed 15: 1.355 m, -22.67 dB, 28.4161 dB, score -1.0000
  - seed 16: 2.765 m, -29.50 dB, 30.7093 dB, score -1.0000
  - seed 16: 1.945 m, -20.44 dB, 29.5794 dB, score -1.0000
  - seed 16: 2.730 m, -33.75 dB, 30.6685 dB, score -1.0000
- B_802: 2 rows
  - seed 8: 4.375 m, -46.08 dB, 32.1840 dB, score -1.0000
  - seed 13: 1.559 m, -41.96 dB, 28.8670 dB, score -1.0000
