# TDOA-01 — how this directory was made

Registered in report/PREREG_TDOA01.md (`5b839d0`, engine `58b12eb`) before any cell ran.

```
2026-10-04T05:30:51+05:30  bash scripts/tdoa01_run.sh results/tdoa01_2026-10-04
2026-10-04T05:49:34+05:30  (all 60 cells done, rc 0)
                           python3 scripts/tdoa01_analyse.py results/tdoa01_2026-10-04
```

The grid ran detached (setsid nohup), uninterrupted; no cell was re-run.

Provenance of every cell (`run_id.txt`):
```
     60 source 5b839d0 dirty 0 profile optimized
```

Tracked: this file, TDOA01.md, tdoa01_solves.csv, raw_manifest.sha256 (the SHA-256 of
every cell's run_id.txt, each of which digests that cell's outputs), and every cell's
`arm_summary.csv` and `run_id.txt` (the results policy in .gitignore).

Outputs (sha256):
```
9e8a1c78341ad2feb7620d5d0be55810b79523e3f1858f54cbd9389830b0005d  TDOA01.md
daa842c43b3dd745fe26d6a9ed63a83c04df834f5f2ba6556e0429b00f37dbfa  tdoa01_solves.csv
8a827ea578929ee4d57ef5383758e71e98da49e9561cb1676e819265c4759e5a  raw_manifest.sha256
```
