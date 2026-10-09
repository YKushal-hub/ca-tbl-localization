# ACC-B3, citable re-run (2026-10-09)

The evaluation registered in `report/PREREG_ACC_B3_HYBRID.md` (`94a200ca`), re-run for provenance. Results are in
`report/ACC_B3_RESULTS.md` §7.

**Binary and invocation**
- **Binary:** built from `e87378cb`, sha256 prefix `f58a9e14d4b0bc87`, optimized.
- **Ran from:** a clean worktree at that commit. Every `run_id.txt` reads `source e87378cb dirty 0 profile optimized`
  (160 of 160).
- **Reference and candidate come from the same binary (rule 8):**
  - `REF_*` and `REFC_*`: `--specProfile=true`, flags off;
  - `CAND_*` and `CANDC_*`: plus `--dualBaseRelayTier=true --caseDInfoRatio=3`.

  With the flags off the binary is byte-identical to the pre-Part-B build (334/334 files, seed 1, A and B).
- **Run settings:** seeds 1–20, `--duration=1800 --trace=true`. A = 5 km; B = endgame
  (`--boxHalf=750 --finderMode=1`).
- **Arms:** `ca_tbl_full,fixed_threshold,static_2p4` in `REF_*` and `CAND_*`; `poucet_fusion,lora_fusion` in
  `REFC_*` and `CANDC_*`.

**Reproduction.** Every `arm_summary.csv` and `subject_summary.csv` (320 files) is byte-identical to the
exploratory runs that `ACC_B3_RESULTS.md` §2–§3 were first computed from.

**What is tracked**
- Per run: `arm_summary.csv`, `subject_summary.csv` and `run_id.txt`. The traces are not tracked (RULES.md §2);
  re-running a cell from its `run_id.txt` invocation regenerates them.
- `metrics.csv`: one row per (cell, seed, arm), from `scripts/acc_s2_metrics.py` over the run directories with
  traces. The column definitions are in that script.
- The verdicts: `python3 scripts/acc_s2_analyse.py results/acc_b3_2026-10-09/metrics.csv CAND_A:ca_tbl_full REF_A:ca_tbl_full ...`
