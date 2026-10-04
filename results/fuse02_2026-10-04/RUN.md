# FUSE-02 — how this directory was made

Registered in report/PREREG_FUSE02.md (`bc43d10`, engine `c10b85d`) before any cell ran.

```
2026-10-04T19:50:37+00:00  bash scripts/fuse02_run.sh results/fuse02_2026-10-04
2026-10-04T20:24:50+00:00  (all 80 cells done, rc 0)
                           python3 scripts/fuse02_analyse.py results/fuse02_2026-10-04
```

The grid ran detached (setsid nohup), uninterrupted; no cell was re-run. The analysis needed
numpy, installed in this container after the grid finished (pip, numpy 2.4.6); the script
was not changed.

Provenance of every cell (`run_id.txt`):
```
     80 source bc43d104 dirty 0 profile optimized
```

Tracked: this file, FUSE02.md, fuse02_cells.csv, raw_manifest.sha256 (the SHA-256 of every
cell's run_id.txt, each of which digests that cell's outputs), and every cell's
`arm_summary.csv` and `run_id.txt` (the results policy in .gitignore).

Outputs (sha256):
```
206289a4e959805765406eee86f63ae2e1649438af4d60b0ee9e08d5510480ef  FUSE02.md
f76c7240ce1115e3d71e2249f217b0c95f92658a8faaa304b91d0ec17c6e9dc3  fuse02_cells.csv
e54dfce457d13a0676b5afee67cf402ba945a395a8f81a6f637bac74cdc88868  raw_manifest.sha256
```
