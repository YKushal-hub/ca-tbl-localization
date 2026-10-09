# s2c_acq_2026-10-09 — how this directory was made

Registered at `03473f3c` before any cell ran. Engine: the build of `c5566a27`'s sources (`--acquireMinAnchors`, G132). Binary (frozen copy, checked with `sha256sum -c` against its
manifest before launch):

```
4a61225d5e0cdfcdcb6299af33602749c9e9d99ca02d3a57e9efeeaf75d449ca  build/scratch/ns3.47-ca_tbl_causal_sim-optimized
```

Run log:

```
2026-10-09T05:11:26Z  (cd ~/campaigns/s2c_acq && bash scripts/s2c_acq_run.sh ~/campaigns/s2c_acq_out), worktree at 03473f3c, build = build_s2c_acq (sha256 verified)
2026-10-09T05:20:48Z  (all cells done, runner exit 0)
```

`run_id.txt` lines (the `source` line is the campaign worktree's HEAD, not the build commit; the compile
stamp pins the binary):

```
     12 source 03473f3c dirty 0 profile optimized
     12 build Oct  9 2026 05:07:27 
```

Each cell keeps `arm_summary.csv` and `run_id.txt`; `raw_manifest.sha256` digests every raw file
(traces untracked). Tables: the analysis output in this directory.
