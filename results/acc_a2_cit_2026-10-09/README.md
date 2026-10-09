# ACC-A2, the error-state filter (2026-10-09). Citable re-run.

The evaluation registered in `report/PREREG_ACC_A2_ERROR_STATES.md` (`9da4bd71`), with its tuning cells. Results
are in `report/ACC_A2_RESULTS.md`.

**Provenance.** Re-run from a clean worktree at `88b0a101`, outside the repository: every `run_id.txt` reads `source 88b0a101 dirty 0`. Every `arm_summary.csv` and `subject_summary.csv`, and `metrics.csv`, is byte-identical to the exploratory runs the results report was first computed from. It replaces `results/acc_a2_2026-10-09`, whose sidecars read `source unknown`.

**Binary**
- Built from the code of `88b0a101`, sha256 prefix `be255612534998e6`, compile stamp `Oct  9 2026 04:27:14`.
- With `--imuErrorStates` off it is byte-identical to the pre-change build (334/334, seed 1, A and B).

**Cells**
- **Evaluation**, seeds 1–20:
  - `IE_*` and `IEC_*`: `--imuErrorStates=true`;
  - `IEB3_*` and `IEB3C_*`: plus `--dualBaseRelayTier=true --caseDInfoRatio=3`.

  `*C_*` cells run `poucet_fusion,lora_fusion`; the others run `ca_tbl_full,fixed_threshold,static_2p4`.
- **Tuning**, seeds 21–30, `ca_tbl_full,static_2p4`: `IEQ0_*`, `IEQ3e7_*` and `IEQ1e6_*` are q_b = 0, 3e-7 and 1e-6.
- **Reference:** the citable `results/acc_b3_2026-10-09` (`REF_*`, `REFC_*`).

**What is tracked**
- Per run: `arm_summary.csv`, `subject_summary.csv` and `run_id.txt`.
- `metrics.csv`, from `scripts/acc_s2_metrics.py`.
- No traces.

**Incident.** The seed-15 endgame cells `IEB3_B/s15` and `IEB3C_B/s15` were first corrupted: the runner script was
edited while bash was executing it, and they exited 127. Both were re-run whole before any analysis; the tracked
copies are the re-runs.
