# EKF5-PRESET — how this directory was made

Registered in `report/PREREG_EKF5_PRESET.md` (`ebb1c756`) before any cell ran, pushed before
launch. The cells ran from the worktree at `ebb1c756` (`work/2026-10-17-imu-filter`), built from
that tree.

```
2026-10-17T21:2x  bash scripts/ekf5_preset_run.sh results/ekf5_preset_2026-10-17   (EKF5_JOBS=11, detached)
2026-10-17T21:30  first cell done
2026-10-18T02:08  all 189 cells done
```

All 189 cells exit 0 and every `run_id.txt` reads:
```
    189 source ebb1c756 dirty 0 profile optimized
```
No reboot in the system log; no cell was re-run. The binary was not hashed at run time.

The evaluation is `python3 scripts/ekf5_preset_analyse.py results/ekf5_preset_2026-10-17
<DC1-BIAS raw directory>`, run once, unedited, reporting "Problems: 0" (`EKF5_PRESET.md`). Its
cv side is DC1-BIAS's raw cells (`results/dc1_bias_2026-10-17`, run at `4739849b`, whose summaries
and sidecars are tracked on `main`); the scorer also checked that every ekf5 cell's DC-1 drew the
same biases as its DC1-BIAS twin, and R1 that 9 cv cells on this binary reproduce DC1-BIAS's
`arm_summary.csv`. `raw_manifest.sha256` holds the SHA-256 of every cell's `run_id.txt`; traces
are not tracked.
