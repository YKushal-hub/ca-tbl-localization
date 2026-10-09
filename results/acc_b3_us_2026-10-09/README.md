# ACC-B3-US, the unknown start (2026-10-09). Exploratory, NOT citable as is.

The evaluation registered in `report/PREREG_ACC_B3_UNKNOWN_START.md` (`ead864fb`). Results are in
`report/ACC_B3_US_RESULTS.md`.

**Why it is not citable.** The runs were made from the session's scratch directory, so every `run_id.txt` reads
`source unknown dirty 0`. To make them citable, re-run each cell's `run_id.txt` invocation from a clean worktree, as
`results/acc_b3_2026-10-09` was.

**Binary**
- Built from the scenario code of `53766acb`, sha256 prefix `6eca39913459d5a2`, compile stamp `Oct  9 2026 04:19:17`.
- With the flag off it is byte-identical to the pre-change build (334/334, seed 1, A and B).

**Cells**
- **US:** `--unknownStartSigmaM=100`.
- **USB3:** `--unknownStartSigmaM=100 --dualBaseRelayTier=true --caseDInfoRatio=3`.
- **Arms:** `US_*` and `USB3_*` run `ca_tbl_full,fixed_threshold,static_2p4`; `USC_*` and `USB3C_*` run
  `poucet_fusion,lora_fusion`.
- **Run settings:** seeds 1–20, 1800 s, A = 5 km and B = endgame.
- **The s = 0 reference:** the citable `results/acc_b3_2026-10-09` (`REF_*`).

**What is tracked**
- Per run: `arm_summary.csv`, `subject_summary.csv` and `run_id.txt`.
- `metrics.csv`, from `scripts/acc_s2_metrics.py`.
- No traces.
