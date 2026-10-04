# CAL-02 — how this directory was made

Registered in report/PREREG_CAL02.md (`7af04f5`, engine `995eeb5`) before any cell ran.

```
2026-10-03T11:35:32Z  bash scripts/cal02_run.sh gen results/cal02_2026-10-03
                      python3 scripts/cal02_analyse.py fit results/cal02_2026-10-03
2026-10-03T12:08:07Z  bash scripts/cal02_run.sh eval results/cal02_2026-10-03
                      python3 scripts/cal02_analyse.py eval results/cal02_2026-10-03
```

Provenance of every cell (`run_id.txt`):
```
    120 source 7af04f5 dirty 0 profile optimized
```

Tracked: this file, FIT.md, fit.json, EVAL.md, raw_manifest.sha256, and every cell's
`arm_summary.csv` and `run_id.txt` (the results policy in .gitignore).

Outputs (sha256):
```
5a6f8a22236623cf8a829b164bd151168e8bb41cb9c246ee4d0a245e0b701bc3  FIT.md
0fd8a8019a4da923f7d2d9310228161dd69029d9816aad9d762c17748d742a3d  fit.json
d754a651e0cd753bd4fd4a6f29276f60d63e1339c1e56162a1a5d2795dd372e7  EVAL.md
805b5da1f50a0cf857857f512461c312deea236332d4d65d75620fd5116cb5a8  raw_manifest.sha256
```
