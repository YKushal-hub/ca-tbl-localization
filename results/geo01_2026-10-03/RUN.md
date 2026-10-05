# GEO-01 stage 2 — how this directory was made

Registered in report/PREREG_GEO01.md (`d05992b`) before any cell ran. Every cell's
`run_id.txt` reads `source d05992b dirty 0 profile optimized`.

```
first pass: 2026-10-03T10:04:42Z  bash scripts/geo_stage2.sh results/geo01_2026-10-03
resume:     2026-10-03T10:18:14Z  bash resume.sh   (this directory's copy of the resume script)
```

**The first pass was interrupted.** The session running it ended with 52 cells finished,
12 cut off mid-run (no `rc` file) and 36 not started. The simulation is deterministic: a
finished cell (N3_star, seed 1) re-run into a scratch directory reproduced all 71 of its
files byte for byte. So the remaining 48 cells were run by `resume.sh`, which clears a
cell without an `rc` file and re-runs it with exactly the registered command line, and
never touches a finished cell. The resumed cells are listed in `resumed_cells.txt`. No
finished cell was re-run into this directory.

Only the summaries, provenance and Markdown are tracked (.gitignore: /results/**). `raw/`
regenerates from the commands; `raw_manifest.sha256` pins every cell's `run_id.txt`, and
each of those carries the SHA-256 of that cell's outputs.

Outputs (sha256):
```
faf5a8296c502802cda145d4148b4d4da414fa2732c2821e401ee5d8ce8e56f9  geo_stage2.csv
dd51b6af33e47e4df56e82d57d3cd934ca4058957cd4f110a5afc2aeadc150ac  geo_stage2.md
707bc6858816ce7faea81d69a00bdced3b396827e4ae94651063cf267738cd92  resumed_cells.txt
6982280f8023a5889f6913fdc4c8d48d6b54a342e82ab330a423b78d72473235  resume.sh
46032d2d41caca1a15fc7838ebf1f7ad0d7adec19ed085fdacc5152f51a3e26e  raw_manifest.sha256
```
