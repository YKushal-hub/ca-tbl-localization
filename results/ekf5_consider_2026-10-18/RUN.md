# EKF5-CONSIDER — how this directory was made

Registered in `report/PREREG_EKF5_CONSIDER.md` (`672ecebf`, committed 2026-10-18T03:35:07+05:30)
before any cell ran, and pushed before launch. The cells ran from the worktree at `672ecebf`
(`work/2026-10-18-ekf5-consider`), built from that tree.

```
2026-10-18T03:35:40  bash scripts/ekf5_consider_run.sh results/ekf5_consider_2026-10-18   (CONSIDER_JOBS=9, detached)
2026-10-18T03:47:35  first cell done
2026-10-18T16:28:30  173rd cell done
2026-10-18T16:29     operator stop (the runner and its running cells were killed), then at 16:29:41
                     bash scripts/ekf5_consider_run.sh --resume results/ekf5_consider_2026-10-18  (CONSIDER_JOBS=10)
2026-10-18T17:14:52  189th cell done
```

**The stop.** `main` had moved (PR #14) and the campaign was judged superseded; it was stopped,
then resumed on finding 173 of 189 cells complete. No killed cell had written an `rc`, so
`--resume` re-ran each of the 16 from an emptied directory (the runner's `rm -rf` before a cell);
no cell's output mixes two runs. The 03:47–16:28 span is longer than the cells' cost implies;
the host was not rebooted (one boot since 2026-10-16), and a suspended VM is the likely reason,
as on D8-MAIN.

All 189 cells exit 0 and every `run_id.txt` reads:
```
    189 source 672ecebf dirty 0 profile optimized
```

The evaluation is `python3 scripts/ekf5_consider_analyse.py results/ekf5_consider_2026-10-18
<EKF5-PRESET raw directory>`, run once, unedited, reporting "Problems: 0" (`EKF5_CONSIDER.md`).
Its mode-0 side is EKF5-PRESET's raw cells (run at `ebb1c756`; summaries and sidecars tracked on
`work/2026-10-18-ekf5-results`); R1 checked that 9 mode-0 cells on this binary reproduce them.
`raw_manifest.sha256` holds the SHA-256 of every cell's `run_id.txt`; traces are not tracked.
