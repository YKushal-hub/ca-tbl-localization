# FUSE-01b — how this directory was made

Registered in report/PREREG_FUSE01B.md (`528752f`, engine `9aebbba`) before any cell ran.

```
2026-10-04T08:57:59+05:30  bash scripts/fuse01b_run.sh results/fuse01b_2026-10-04
2026-10-04T09:30:22+05:30  (all 80 cells done, rc 0)
                           python3 scripts/fuse01b_analyse.py results/fuse01b_2026-10-04
```

The grid ran detached (setsid nohup), uninterrupted; no cell was re-run.

Provenance of every cell (`run_id.txt`):
```
     80 source 528752f dirty 0 profile optimized
```

Tracked: this file, FUSE01B.md, fuse01b_cells.csv, raw_manifest.sha256 (the SHA-256 of
every cell's run_id.txt, each of which digests that cell's outputs), and every cell's
`arm_summary.csv` and `run_id.txt` (the results policy in .gitignore).

Outputs (sha256):
```
1b23719e801c475f07b5901e11cdc86e39b16c2b624506376466a553ed4ff7ce  FUSE01B.md
bf85bff701a083e04146b4250f6f3750648607811efdac1d81fb75ffdbba2ec1  fuse01b_cells.csv
d4a2277ba8cd7c6fc1a930d94e121798b89d9e948cd698653eece67b91d76f1b  raw_manifest.sha256
```
