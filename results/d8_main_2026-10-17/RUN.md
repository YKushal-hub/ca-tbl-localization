# D8-MAIN — how this directory was made

Registered in `report/PREREG_D8_MAIN.md` (`52b8ebac`) before any cell ran, and pushed to GitHub
before launch. The cells ran from the worktree at `52b8ebac`, built from that tree.

```
2026-10-17T01:5x  bash scripts/d8_main_run.sh results/d8_main_2026-10-17   (D8M_JOBS=11, detached)
2026-10-17T02:08  first cell done
2026-10-17T15:33  all 120 cells done
```

All 120 cells exit 0 and every `run_id.txt` reads:
```
    120 source 52b8ebac dirty 0 profile optimized
```

**Interruptions.** The VM was suspended for several hours during the run (no reboot: the
system log shows none, and `uptime` counts only awake time). Suspension pauses processes; a
discrete-event simulation's output does not depend on wall-clock time. No cell was re-run.

**Binary.** Not hashed at run time, and the build was replaced afterwards: DC1-BIAS was launched
from the same worktree at `4739849b`, which rebuilt the scenario with a new build stamp. The
commits between `52b8ebac` and `4739849b` touch documents and scripts only, so the code is the
same; the exact bytes of the D8-MAIN binary are not recorded, and this says so.

The evaluation is `python3 scripts/d8_main_analyse.py results/d8_main_2026-10-17`, run once,
unedited, reporting "Problems: 0" (`D8_MAIN.md`, `d8_main_table.csv`). `raw_manifest.sha256`
holds the SHA-256 of every cell's `run_id.txt`, whose digest block covers that cell's outputs,
traces included; the traces themselves are not tracked (`.gitignore`).
