# FUSE-01 — how this directory was made

Registered in report/PREREG_FUSE01.md (`67da4ef`, engine `022212e`) before any cell ran.

```
2026-10-04T01:41:50+05:30  bash scripts/fuse01_run.sh results/fuse01_2026-10-04
2026-10-04T02:12:10+05:30  (all 80 cells done, rc 0)
                           python3 scripts/fuse01_analyse.py results/fuse01_2026-10-04
```

The grid ran detached (setsid nohup), uninterrupted; no cell was re-run.

Provenance of every cell (`run_id.txt`):
```
     80 source 67da4ef dirty 0 profile optimized
```

Tracked: this file, FUSE01.md, fuse01_cells.csv, raw_manifest.sha256 (the SHA-256 of
every cell's run_id.txt, each of which digests that cell's outputs), and every cell's
`arm_summary.csv` and `run_id.txt` (the results policy in .gitignore).

Outputs (sha256):
```
8984a7b8169a853d60495775e3a4c28ae7cacb5fa15955b505905e606cae1b8b  FUSE01.md
50f05819a5deec5cfba1a273fe869537c8db1b1ee54df605e051c3d9f8246cbc  fuse01_cells.csv
9a7171534659b519faaff565f2f67bd21876580335bd36317df89ca239cc1a61  raw_manifest.sha256
```
