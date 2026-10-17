# DC1-BIAS — how this directory was made

Registered in `report/PREREG_DC1_BIAS.md` (`4739849b`) before any cell ran, pushed before launch,
and committed after D8-MAIN's cells finished but before its scorer was run. The cells ran from
the worktree at `4739849b`, built from that tree.

```
2026-10-17T15:34  bash scripts/dc1_bias_run.sh results/dc1_bias_2026-10-17   (DC1_JOBS=11, detached)
2026-10-17T15:46  first cell done
2026-10-17T20:22  all 180 cells done
```

All 180 cells exit 0 and every `run_id.txt` reads:
```
    180 source 4739849b dirty 0 profile optimized
```
No interruption is known: no reboot in the system log, and the run did not stall. No cell was
re-run. The binary was not hashed at run time; the sidecars pin the source.

**The bias draw, checked.** At each level the per-run bias is the same standard-normal draw times
the level's sigma: for seed 1, tag 0 draws −4.43e-7 rad/s at L0 and −9.05e-5 rad/s at L3, a
ratio of 204, which is 0.5 °/s ÷ 8.816 °/h.

The evaluation is `python3 scripts/dc1_bias_analyse.py results/dc1_bias_2026-10-17`, run once,
unedited, reporting "Problems: 0" (`DC1_BIAS.md`, `dc1_bias_table.csv`). `raw_manifest.sha256` holds
the SHA-256 of every cell's `run_id.txt`, whose digest block covers that cell's outputs; traces
are not tracked (`.gitignore`).
