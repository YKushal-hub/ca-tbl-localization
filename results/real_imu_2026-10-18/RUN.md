2026-10-18T14:06:50Z launch; source 92152dc7 dirty 0
2026-10-18T14:42:55Z resume; source 92152dc7 dirty 0
2026-10-18T15:01:12Z done

## How this directory was made (added at recording, not by the runner)

- **A first run was voided.** Launched 2026-10-18T13:30:41Z into this path. 148 of its 160 cells recorded
  `dirty 7` or `dirty 8`, because unrelated source edits were made in the launch worktree while it ran. Its
  binary was verified by manifest before every cell, and a clean rebuild hashes identical (`08313ad0…`). It is
  kept outside the repository as `real_imu_2026-10-18.void-dirty`, with a VOID line in its own RUN.md, and is
  not cited.
- **This run** was launched 14:06:50Z from a clean tree, the same binary by manifest. The host rebooted after
  80 cells (76 complete, 4 cut off). It was resumed 14:42:55Z: the four were re-run from emptied directories,
  completed cells skipped. Done 15:01:12Z. All 160 cells read `source 92152dc7 dirty 0`.
- All 160 `arm_summary.csv` are byte-identical to the void run's.
