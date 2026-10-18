# UPG-SCOPE results: the scope-2026-10-17 upgrades on the adopted preset (exploratory)

Pre-registration: `report/PREREG_UPG_SCOPE.md` (commit `b62340c2`, before any cell ran). Engine `b62340c2` (the source is
`be9b8f21` plus the prereg). Campaign `~/catbl_work/campaigns/upg`; 320 cells, all `rc 0` and `dirty 0`; launched
2026-10-17T02:03Z, done 02:30Z. Tracked summaries: `results/upg_2026-10-17/`. Analyser: `scripts/upg_analyse.py`.
No abort condition fired.

## 1. Predictions
| # | prediction | outcome | figure |
|---|---|---|---|
| U1 | A, C1 (`--acquireNegInfo`): `ca_tbl_full` CEP95 not worse, coverage not lower | **not held** | CEP95 +0.3 [−184.5, +221.8] m; coverage 0.577 → 0.533 |
| U2 | A, C2 (`--tofRecal`): `static_868` CEP95 lower | **not held** (mis-specified) | +0.0 exactly: see 2.2 |
| U3 | B, C3 (`--hold24AfterK=3`): tag-0 energy ≤ 0.95×, CEP95 within +20 % | **not held** (narrowly) | energy 0.900×; CEP95 +1.2 [−0.8, +3.9] m on 5.6 m (+21 %) |
| U4 | A, B1 (`--relayNisInflate`): coverage higher | **held** | 0.577 → 0.587 |
| U5 | B7 (`--fadingPerBand`): \|ΔCEP95\| < 10 % in A and B | **held** | A −4.6 m on 333; B −0.4 [−0.9, +0.1] m on 5.6 |
| U6 | B, WC5 (`--tofCalibrate` under Wolf): `static_2p4` CEP95 interval below 0 | **held** | −100.3 [−117.4, −83.7] m (130.3 → 30.1 m) |

## 2. Paired effects on `ca_tbl_full` (tag 0, 20 seeds, mean [95 % bootstrap])
| config | variant | ΔCEP95 (m) | ΔCEP95 after 300 s (m) | energy ratio | Δcoverage |
|---|---|---|---|---|---|
| A | C1 | +0.3 [−184.5, +221.8] | −36.4 [−246.9, +221.8] | 1.031 | −0.048 [−0.109, +0.015] |
| A | C2 | −24.3 [−166.4, +113.1] | −21.3 [−169.5, +123.6] | 1.037 | −0.038 [−0.092, +0.014] |
| A | C3 | +0.3 [−1.3, +2.6] | −0.7 [−2.9, +1.8] | 0.998 | −0.008 [−0.032, +0.012] |
| A | B1 | −5.0 [−13.9, +1.8] | −14.5 [−47.7, +8.2] | 1.000 | +0.011 [−0.009, +0.035] |
| A | B7 | −4.6 [−77.1, +72.4] | +22.0 [−33.7, +89.2] | 0.982 | −0.033 [−0.101, +0.038] |
| A | WC5 vs WBASE | −13.4 [−106.9, +64.7] | −10.0 [−100.8, +65.4] | 1.263 | +0.035 [−0.018, +0.094] |
| B | C1 | +1.4 [−1.3, +6.3] | −0.3 [−0.8, +0.2] | 0.998 | +0.015 [−0.006, +0.042] |
| B | C2 | +2.8 [−1.1, +9.6] | +0.3 [−0.5, +1.4] | 1.042 | +0.008 [−0.024, +0.044] |
| B | C3 | +1.2 [−0.8, +3.9] | +0.2 [−0.6, +1.2] | **0.900** | +0.006 [−0.032, +0.048] |
| B | B1 | +0.0 [−1.1, +1.5] | +0.5 [−0.4, +1.8] | 0.950 | +0.015 [−0.001, +0.032] |
| B | B7 | −0.4 [−0.9, +0.1] | −0.2 [−0.7, +0.2] | 0.969 | +0.024 [−0.006, +0.061] |
| B | WC5 vs WBASE | −2.6 [−24.8, +13.8] | +5.0 [−7.0, +16.9] | **2.038** | +0.055 [+0.015, +0.092] |

### 2.1 Reading
- **No upgrade moves 5 km accuracy measurably.** Every config-A interval spans zero. C1 and C2 have the largest point
  gains after acquisition (−36 and −21 m), but with ±200 m intervals, and both lower coverage by 0.04–0.05. Per-run
  CEP95 at 5 km is dominated by the acquisition tail, as V7 §2 found.
- **C3 is a real energy lever** in the endgame: −10 % tag-0 energy with post-acquisition CEP95 unchanged
  (+0.2 [−0.6, +1.2] m). The registered criterion was per-run CEP95, which moved +21 % against a +20 % bound.
- **B1 and B7 are consistency and correctness fixes with no accuracy cost.** B1 raises coverage at both scales and
  cuts endgame energy 5 %. B7 removes cross-band fading coupling, changes accuracy by less than 1 % in the endgame,
  and cuts energy 2–3 %.
- **Calibrated Wolf ToF (WC5):** 2.4 GHz alone goes from 130 to 30 m in the endgame. `ca_tbl_full` does not follow
  (58.8 → 56.2 m) and its energy doubles (42.7 → 85.0 J).
  - So the hybrid under Wolf errors is limited by something other than the 2.4 GHz bias. The energy doubling says
    calibrated 2.4 GHz ranges keep the policy escalating rather than settling.
  - This is a new finding, recorded as TECH_DEBT O.7, not diagnosed here.

### 2.2 U2 was mis-specified
C2 acts in the policy's `Step` on tier-0 ranges, using same-anchor pairs of 868 MHz and 2.4 GHz ranges. `static_868`
never holds a 2.4 GHz range, so it cannot learn a bias, and its output is byte-identical to BASE by construction.
The prediction should have named `ca_tbl_full`. There, A gives −24.3 [−166.4, +113.1] m and B gives +2.8 [−1.1, +9.6] m:
no supported effect either way.

## 3. Decisions (DESIGN_DECISIONS 13)
- **Adopted into the preset:**
  - `--fadingPerBand=true` (B7): a defect fix; U5 held.
  - `--relayNisInflate=true` (B1): a consistency fix; U4 held; no accuracy or energy cost.
- **Not adopted:**
  - C1 and C2: predictions not held, no supported gain, lower 5 km coverage.
  - C3: its prediction was not held. It stays available as an endgame energy mode, at −10 % energy with
    post-acquisition accuracy unchanged.
- **C5 `--tofCalibrate`:** a property of calibrated hardware, not of the preset. Any campaign that runs
  `--tofErrorModel=wolf` should pair it with `--tofCalibrate=true`, as real SX1280 ToF is calibrated (Müller eq. 5).

## 4. Full table (all arms)
| config | variant | arm | CEP95 (m) | CEP95 after 300 s (m) | coverage | fix share | tag-0 J |
|---|---|---|---|---|---|---|---|
| A | BASE | static_868 | 490.1 | 444.8 | 0.343 | 0.780 | 8.27 |
| A | BASE | static_2p4 | 395.9 | 88.3 | 0.844 | 0.062 | 13.36 |
| A | BASE | ca_tbl_full | 333.2 | 242.6 | 0.577 | 0.856 | 17.24 |
| A | BASE | fixed_threshold | 315.6 | 208.9 | 0.580 | 0.845 | 18.32 |
| A | C1 | static_868 | 471.6 | 354.9 | 0.311 | 0.775 | 8.30 |
| A | C1 | static_2p4 | 396.6 | 88.7 | 0.850 | 0.062 | 13.37 |
| A | C1 | ca_tbl_full | 333.5 | 206.2 | 0.533 | 0.866 | 17.64 |
| A | C1 | fixed_threshold | 336.5 | 219.2 | 0.545 | 0.859 | 17.45 |
| A | C2 | static_868 | 490.1 | 444.8 | 0.343 | 0.780 | 8.27 |
| A | C2 | static_2p4 | 395.9 | 88.3 | 0.844 | 0.062 | 13.36 |
| A | C2 | ca_tbl_full | 309.0 | 221.2 | 0.541 | 0.853 | 17.77 |
| A | C2 | fixed_threshold | 309.5 | 225.9 | 0.570 | 0.861 | 18.27 |
| A | C3 | static_868 | 490.1 | 444.8 | 0.343 | 0.780 | 8.27 |
| A | C3 | static_2p4 | 395.9 | 88.3 | 0.844 | 0.062 | 13.36 |
| A | C3 | ca_tbl_full | 333.6 | 241.9 | 0.568 | 0.853 | 17.28 |
| A | C3 | fixed_threshold | 316.1 | 207.7 | 0.575 | 0.845 | 18.20 |
| A | B1 | static_868 | 490.1 | 444.8 | 0.343 | 0.780 | 8.27 |
| A | B1 | static_2p4 | 395.9 | 85.7 | 0.852 | 0.061 | 13.37 |
| A | B1 | ca_tbl_full | 328.3 | 228.1 | 0.587 | 0.850 | 17.21 |
| A | B1 | fixed_threshold | 329.6 | 220.8 | 0.605 | 0.840 | 18.27 |
| A | B7 | static_868 | 493.5 | 464.7 | 0.283 | 0.794 | 8.38 |
| A | B7 | static_2p4 | 396.8 | 96.0 | 0.863 | 0.056 | 13.33 |
| A | B7 | ca_tbl_full | 328.6 | 264.6 | 0.544 | 0.857 | 16.48 |
| A | B7 | fixed_threshold | 318.6 | 215.3 | 0.549 | 0.856 | 17.51 |
| A | WBASE | static_868 | 490.1 | 444.8 | 0.343 | 0.780 | 8.27 |
| A | WBASE | static_2p4 | 419.1 | 172.2 | 0.663 | 0.041 | 13.38 |
| A | WBASE | ca_tbl_full | 374.9 | 324.9 | 0.319 | 0.847 | 18.77 |
| A | WBASE | fixed_threshold | 374.8 | 324.5 | 0.322 | 0.848 | 18.78 |
| A | WC5 | static_868 | 490.1 | 444.8 | 0.343 | 0.780 | 8.27 |
| A | WC5 | static_2p4 | 413.1 | 164.1 | 0.872 | 0.053 | 13.39 |
| A | WC5 | ca_tbl_full | 361.5 | 314.9 | 0.351 | 0.851 | 22.81 |
| A | WC5 | fixed_threshold | 361.5 | 315.5 | 0.351 | 0.849 | 22.83 |
| B | BASE | static_868 | 51.9 | 40.8 | 0.398 | 0.964 | 9.47 |
| B | BASE | static_2p4 | 8.9 | 6.4 | 0.906 | 0.838 | 8.98 |
| B | BASE | ca_tbl_full | 5.6 | 4.8 | 0.858 | 0.995 | 28.48 |
| B | BASE | fixed_threshold | 6.2 | 5.0 | 0.843 | 0.995 | 25.32 |
| B | C1 | static_868 | 59.7 | 47.3 | 0.385 | 0.966 | 9.46 |
| B | C1 | static_2p4 | 7.5 | 5.8 | 0.911 | 0.840 | 8.99 |
| B | C1 | ca_tbl_full | 7.1 | 4.5 | 0.873 | 0.995 | 27.76 |
| B | C1 | fixed_threshold | 8.1 | 5.2 | 0.844 | 0.994 | 26.53 |
| B | C2 | static_868 | 51.9 | 40.8 | 0.398 | 0.964 | 9.47 |
| B | C2 | static_2p4 | 8.9 | 6.4 | 0.906 | 0.838 | 8.98 |
| B | C2 | ca_tbl_full | 8.4 | 5.1 | 0.866 | 0.995 | 28.81 |
| B | C2 | fixed_threshold | 8.7 | 5.5 | 0.856 | 0.994 | 28.63 |
| B | C3 | static_868 | 51.9 | 40.8 | 0.398 | 0.964 | 9.47 |
| B | C3 | static_2p4 | 8.9 | 6.4 | 0.906 | 0.838 | 8.98 |
| B | C3 | ca_tbl_full | 6.8 | 5.0 | 0.864 | 0.994 | 24.63 |
| B | C3 | fixed_threshold | 6.6 | 5.0 | 0.851 | 0.995 | 23.22 |
| B | B1 | static_868 | 51.9 | 40.8 | 0.398 | 0.964 | 9.47 |
| B | B1 | static_2p4 | 8.5 | 6.5 | 0.908 | 0.836 | 8.96 |
| B | B1 | ca_tbl_full | 5.7 | 5.3 | 0.873 | 0.995 | 26.58 |
| B | B1 | fixed_threshold | 5.2 | 4.7 | 0.843 | 0.995 | 25.94 |
| B | B7 | static_868 | 52.4 | 42.1 | 0.398 | 0.966 | 9.52 |
| B | B7 | static_2p4 | 8.5 | 7.4 | 0.897 | 0.835 | 8.95 |
| B | B7 | ca_tbl_full | 5.2 | 4.6 | 0.882 | 0.994 | 26.79 |
| B | B7 | fixed_threshold | 5.7 | 5.5 | 0.850 | 0.995 | 26.70 |
| B | WBASE | static_868 | 51.9 | 40.8 | 0.398 | 0.964 | 9.47 |
| B | WBASE | static_2p4 | 130.3 | 113.7 | 0.252 | 0.793 | 8.78 |
| B | WBASE | ca_tbl_full | 58.8 | 48.6 | 0.337 | 0.997 | 42.71 |
| B | WBASE | fixed_threshold | 56.6 | 47.2 | 0.341 | 0.997 | 41.74 |
| B | WC5 | static_868 | 51.9 | 40.8 | 0.398 | 0.964 | 9.47 |
| B | WC5 | static_2p4 | 30.1 | 24.8 | 0.887 | 0.776 | 8.79 |
| B | WC5 | ca_tbl_full | 56.2 | 53.7 | 0.391 | 0.996 | 85.04 |
| B | WC5 | fixed_threshold | 56.4 | 54.8 | 0.400 | 0.996 | 89.49 |

U1 A C1-BASE ca_tbl_full CEP95 +0.3 [-184.5, +221.8] m
U2 A C2-BASE static_868 CEP95 +0.0 [+0.0, +0.0] m
U3 B C3/BASE ca_tbl_full energy 0.900; CEP95 +1.2 [-0.8, +3.9] m on 5.6
U5 B7-BASE ca_tbl_full CEP95 A -4.6 [-77.1, +72.4], B -0.4 [-0.9, +0.1] m
U6 B WC5-WBASE static_2p4 CEP95 -100.3 [-117.4, -83.7] m

## Predictions

- **U1**: not held
- **U2**: not held
- **U3**: not held
- **U4**: held
- **U5**: held
- **U6**: held
