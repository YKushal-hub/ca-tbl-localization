# S2-C-NLOS — how this directory was made

Registered in `report/PREREG_S2C_NLOS.md` (`ddc0173b`, committed 2026-10-09T04:35:18Z) before any
cell ran. Engine: the build of `4e087627`'s sources (SHA-256 of the three changed sources recorded
at build time), frozen as `build_s2c_nlos` and checked with `sha256sum -c` before launch:

```
94d4a5a19f53aa3d5d8b3d92542704a9acc8c804f86213300ac757663c56af01  build/scratch/ns3.47-ca_tbl_causal_sim-optimized
```

The cells ran detached from a clean worktree at `ddc0173b` on the cloud machine (4 cores, g++ 13.3),
concurrently with the S2-C campaign and the gate suite:

```
2026-10-09T04:35:22Z  (cd ~/campaigns/s2c_nlos && bash scripts/s2c_nlos_run.sh ~/campaigns/s2c_nlos_out), worktree at ddc0173b, build = build_s2c_nlos (sha256 verified)
2026-10-09T05:04:35Z  (all cells done, runner exit 0)
```

All 60 runs exit 0. Every `run_id.txt` reads `source ddc0173b dirty 0` (the worktree HEAD) and one
compile stamp. The analysis (`python3 scripts/s2c_nlos_analyse.py <outdir>`) ran once on the complete
set, "Problems: 0"; `S2C_NLOS.md` is its output. (A read of the registered analysis at 58 of 60 runs
was taken while the last two ran; it reported the two missing seeds as problems and was superseded.)

Each cell keeps `arm_summary.csv` and `run_id.txt`; `raw_manifest.sha256` digests every raw file
(traces untracked).
