# PREREG CAL-03 — PCS weights and thresholds re-fitted on the finished preset

**Registered before any cell runs** (rule 13). Written 2026-10-04. Engine source at
registration: `e4ae4a1`, with the preset as it stands there (97 settings: the spec's
three gateways, N3_star; half-duplex per chip; both colliding frames judged; requests over
the mesh; no fuse, FUSE-02 having failed its adoption rule). This commit adds this file, `scripts/cal03_run.sh` (the runner) and
`scripts/cal03_analyse.py` (the analysis: `scripts/cal02_analyse.py`'s computation unchanged,
its labels and paths pointing here), and nothing else. The outputs go to
`results/cal03_2026-10-04/`, which does not exist at registration.

**Why.** CAL-02 (report/CAL02_RESULTS.md) fitted the preset's PCS weights and thresholds on
the preset of 2026-10-03: eight gateways on a ring, no half-duplex, the finder oracle open
(N.21). The preset now carries the spec's three gateways and the channel-fidelity flags, so
the values it applies were fitted on a configuration it no longer is. The spec's final audit
(Gap B) asks for the weights by leave-one-out on the deployment's own data. The owner's
call, delegated on 2026-10-04 (docs/DESIGN_DECISIONS.md §9), is to re-fit them by CAL-02's
procedure unchanged.

**How the run is isolated.** The cells run from a git worktree at this registration's
commit, with a byte copy of the binary and libraries built at `e4ae4a1`
(`LD_LIBRARY_PATH` pins the copy), so the main tree can change while the grid runs and every
cell still reads `source <this commit> dirty 0`. The copy's SHA-256 is in RUN.md.

---

## 1. Data, terms, weights, thresholds, evaluation

CAL-02's §1-§5, unchanged, with one difference forced by the preset: the data-generating
configuration is the preset with `--pcsForm=legacy` given explicitly. Since `e4ae4a1`
the preset's calibrated weights and thresholds are unused, not applied, under an explicit
non-calibrated form, so the gen cells run legacy PCS with the legacy thresholds, as CAL-02's
did. The evaluation cells run the preset with `--pcsForm=calibrated` and the eight fitted
values given explicitly.

- gen: main and end (`--boxHalf=750 --finderMode=1`), seeds 1-40, arms `ca_tbl_full` and
  `fixed_threshold`, 1 800 s, traces on. Fit seeds 1-20, held-out 21-40. 80 cells.
- eval: main and end, seeds 21-40, `ca_tbl_full`. 40 cells.

## 2. Predictions, made before any cell

- **P1.** The fit again drops the trace term: w1 is clamped to 0. At 5 km 868 MHz floor
  fixes dominate and the trace ranks error backwards there (N.16), and three gateways make
  the 5 km fixes sparser than eight did.
- **P2.** On held-out seeds A (ρ_control − ρ_FIT) is BETTER, for the reason CAL-02's was:
  the trace control ranks error backwards at 5 km. Recorded as hollow if it holds.
- **P3.** `pcsThrUp2p4` and `pcsThrUpUwb` are UNFITTED or UNEXERCISED: UWB "would solve" stays
  rare even in the endgame.
- **P4.** The calibrated preset's mean availability is within 0.01 of the legacy preset's in
  both configurations.

Each is written up as held or failed in `report/CAL03_RESULTS.md`.

## 3. Abort conditions

CAL-02's §7, unchanged: a non-zero exit stops the grid; t3 outside [−0.001, 1.001] on more
than 1 % of a configuration's geometry-bearing fix cycles stops it; an existing phase
directory is refused.

## 4. What follows

CAL-02's §8 rule. The preset takes the re-fitted values, in its own commit (G24 re-run), if
the evaluation's mean availability is not lower than the legacy preset's by more than 0.01
in either configuration. Otherwise the preset keeps legacy PCS, and the report says why.
