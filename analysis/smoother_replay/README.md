# Smoother replay — EXPLORATORY, not citable (TECH_DEBT N.62)

Asks whether a trajectory-level estimator recovers tag 0 at 5 km where the 5-state EKF locks wrong, given the same
information. Both scripts read one run's logs and solve least squares in Python (scipy). They compute nothing the
scenario reports.

- `replay.py <run dir> <seed>` — full batch over every cycle. An upper bound: it uses future measurements.
- `window.py <run dir> <seed> <W> <every>` — causal: each estimate is solved from the trailing W cycles only.

Inputs (one `ca_tbl_full` run): `--trace=true --traceRanges=true`, giving `trace_`, `ranges_`, `imu_`, `nodes_` and
`deployment.json`. The runs used (2026-10-20, `~/ns/catbl-work/finder868/s1..s6`, not tracked) are:

    --specProfile=true --duration=600 --trace=true --traceRanges=true --unknownStart=true --fixedVarThreshold=12
    --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2 --imuFilter=ekf5 --imuMagSigmaDeg=3
    --ekf5FinderConsider=true --ekf5RelayGate=true --arms=ca_tbl_full --seed=<1..6>

Factors: the ranges the EKF fused (`inSet`), with their logged sigma; IMU odometry rotated by
theta(t) = theta0 + b t and divided by (1 + s); a magnetometer synthesised from the logged true heading error with
the run's model (white 3 deg plus Gauss-Markov 3 deg / 120 s); priors on b, s and a constant 2 m offset per finder.

What it cannot show: a real magnetometer, real finder GNSS, a filter that runs inside the scenario's feedback loop
(finders steer on the estimate), or cost.
