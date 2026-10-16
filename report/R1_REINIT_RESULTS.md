# R1-REINIT results — acquisition re-initialisation does not rescue the unknown start (registered, exploratory)

Registration: `report/PREREG_R1_REINIT.md` at `2c402412`, committed before any cell. 80 runs, all `rc 0`,
`source 2c402412 dirty 0`, one compile stamp (`build Oct 16 2026 11:01:38`), frozen binary with a SHA-256
manifest checked before each cell. Tracked per cell: `arm_summary.csv`, `run_id.txt`, `rc`
(`results/r1_reinit_2026-10-16/`). Traces were in `~/catbl_work/campaigns/r1_reinit` (untracked).
The full analysis output (`scripts/r1_reinit_analyse.py`) is reproduced in §3.

## 1. Verdicts
| # | prediction | outcome |
|---|---|---|
| R1a | A, `ca_tbl_full` w25 up | **not held**: −0.004 [−0.070, +0.067] |
| R1b | A, `static_2p4` w25 up | **not held**: +0.018 [−0.006, +0.043] |
| R1c | A, median error down, all arms | **not held** (`ca_tbl_full` +69 m [−18, +147]) |
| R1d | first fix unchanged | **not held**: `ca_tbl_full` A first fix 49 s *sooner* [−90, −4] |
| R1e | B not made worse (w25 above −0.05) | **not held**: `ca_tbl_full` −0.029 [−0.085, +0.027]; CEP95 +21.3 m [+5.7, +39.5] |
| R1f | `ca_tbl_full` − `fixed_threshold` under U1 contains 0 | **held**: +0.018 [−0.001, +0.039] |

## 2. Reading
- The mechanism fired: 168 re-initialisations in U1_A and 286 in U1_B (234 and 173 refused by the residual
  guard); none in the controls.
- Many accepted solutions come from **three RSSI-ranged anchors at kilometre range**: an exact fit
  (residual ≈ 0.006) with a covariance trace of about 3·10⁷ m² (σ ≈ 5 km per axis). With three ranges this
  uncertain, the circle intersection is ambiguous, and re-initialising at the wrong root is no better
  than the box-centre prior.
- **So N.54's hypothesis — that linearising the first update at the prior is what locks in — is not
  supported.** The evidence points to ambiguity and geometry of the first three-range sets (TECH_DEBT N.25's
  root ambiguity at three gateways is the same family).
- Per the master plan's rule, `--acquireReinit` stays **off** in B4 (neither R1a nor R1c held). The flag
  stays in the engine, default off, as a recorded negative.

## 3. Analysis output
# R1-REINIT results (exploratory)

## Config A: U1 − U0, paired over seeds

| arm | w25 | w25acq | med | first | never | c95 | av | J | n |
|---|---|---|---|---|---|---|---|---|---|
| ca_tbl_full | -0.004 [-0.070, +0.067] | -0.014 [-0.087, +0.065] | +69.289 [-17.528, +146.859] | -49.000 [-90.275, -4.425] | -0.040 [-0.090, +0.010] | +186.772 [-70.840, +527.933] | -0.017 [-0.038, +0.003] | +0.309 [-0.780, +1.552] | 20 |
| fixed_threshold | +0.001 [-0.070, +0.074] | -0.002 [-0.085, +0.085] | +38.212 [-82.993, +149.519] | +10.400 [-44.626, +64.752] | +0.000 [-0.050, +0.050] | +124.817 [-116.786, +459.774] | -0.008 [-0.022, +0.007] | +0.655 [-0.655, +1.989] | 20 |
| static_2p4 | +0.018 [-0.006, +0.043] | +0.015 [-0.070, +0.101] | -7.035 [-37.277, +29.039] | -22.850 [-96.057, +47.477] | -0.010 [-0.080, +0.050] | +28.071 [+2.676, +58.632] | +0.001 [-0.001, +0.005] | -0.096 [-0.311, +0.117] | 20 |

## Config B: U1 − U0, paired over seeds

| arm | w25 | w25acq | med | first | never | c95 | av | J | n |
|---|---|---|---|---|---|---|---|---|---|
| ca_tbl_full | -0.029 [-0.085, +0.027] | -0.025 [-0.083, +0.032] | +4.700 [-0.903, +13.159] | +12.650 [-14.652, +39.700] | +0.000 [+0.000, +0.000] | +21.307 [+5.695, +39.484] | -0.011 [-0.021, -0.000] | -0.195 [-2.793, +2.633] | 20 |
| fixed_threshold | -0.018 [-0.081, +0.028] | -0.019 [-0.087, +0.030] | +2.092 [-2.882, +9.690] | +13.050 [-5.825, +33.201] | +0.000 [+0.000, +0.000] | +14.140 [+1.156, +29.437] | -0.008 [-0.026, +0.006] | -0.632 [-2.729, +1.355] | 20 |
| static_2p4 | +0.017 [-0.007, +0.042] | +0.018 [-0.012, +0.046] | +1.750 [-2.077, +6.578] | -22.350 [-81.925, +37.976] | -0.010 [-0.030, +0.000] | -0.690 [-6.012, +4.487] | +0.002 [-0.003, +0.007] | +0.007 [-0.087, +0.094] | 20 |

## Predictions

- **R1a**: not held
- **R1b**: not held
- **R1c**: not held
- **R1d**: not held
- **R1e**: not held
- **R1f**: held

R1f interval (`ca_tbl_full` − `fixed_threshold`, U1_A, w25): +0.018 [-0.001, +0.039]
