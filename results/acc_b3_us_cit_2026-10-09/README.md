# ACC-B3-US, the unknown start (2026-10-09). Citable re-run.

The evaluation registered in `report/PREREG_ACC_B3_UNKNOWN_START.md` (`ead864fb`). Results are in
`report/ACC_B3_US_RESULTS.md`.

**Provenance.** Re-run from a clean worktree at `53766acb`, outside the repository: every `run_id.txt` reads `source 53766acb dirty 0`. Every `arm_summary.csv` and `subject_summary.csv`, and `metrics.csv`, is byte-identical to the exploratory runs the results report was first computed from. It replaces `results/acc_b3_us_2026-10-09`, whose sidecars read `source unknown`.

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
