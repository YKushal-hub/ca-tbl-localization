# LORA-CMP — how this directory was made

Registered in report/PREREG_LORA_CMP.md (`0a542ec`) before any R cell ran. The cells ran
from a git worktree at `0a542ec` (~/campaigns/lora_cmp) holding a byte copy of CAMP-FINAL's
frozen build of `d49f99c`, pinned by LD_LIBRARY_PATH (scripts/lora_cmp_run.sh). The copy
verified against CAMP-FINAL's list (results/camp_final_2026-10-04/RUN.md) before the run.

```
2026-10-05T04:10:21+00:00  (cd ~/campaigns/lora_cmp && bash scripts/lora_cmp_run.sh /home/user/ca-tbl-localization/results/lora_cmp_2026-10-05)
2026-10-05T04:23:02+00:00  (all 61 cells done)
```

The run was detached (setsid nohup) and uninterrupted; no cell was re-run. All 61 cells
exit 0, and every cell's `run_id.txt` reads:
```
     61 source 0a542ecc dirty 0 profile optimized
```

The analysis is `python3 scripts/lora_cmp_analyse.py results/lora_cmp_2026-10-05
results/camp_final_2026-10-04`, which reports "Problems: 0". X_repro's `static_868` row is
byte-identical to CAMP-FINAL's A_main seed 1 row.

Tracked: this file, LORA_CMP.md, lora_cmp_table.csv, raw_manifest.sha256 (the SHA-256 of
every cell's run_id.txt, each of which digests that cell's outputs), and every cell's
`arm_summary.csv` and `run_id.txt`.

Outputs (sha256):
```
8f5805f87d80aa608509ca31f70e6cbe7d0b09d0fdf0603a81575a8057674159  LORA_CMP.md
f3b33d5452792d9e140f94a6a747cc2475d45eab24bef1a7ce2689db391bf463  lora_cmp_table.csv
f7a6fc9e831cc428e4de1e47d574faeb4b613f7a7ba3a33abfb5bd0340b5e120  raw_manifest.sha256
```

The analysis first wrote lora_cmp_table.csv with CRLF line endings (Python csv's default).
The repository stores it as LF (.gitattributes eol=lf), so the file was converted to LF, and
its hash above is the LF file's (CRLF original: `6fef22f5e2c4c8ec2d51c8a88859d5251d0e9311098cd34628596c6626df90dc`). The script now writes LF.
Re-running it on these cells reproduces LORA_CMP.md and lora_cmp_table.csv byte-for-byte.
