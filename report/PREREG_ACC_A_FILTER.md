# PREREG ACC-A — IMU increment covariance and every consumer of P (S2 Part A, exploratory grade)

**Registered before any cell runs** (rule 13). Written 2026-10-09. Engine: the build of `81fb1485`
(`--imuQuScale`, gate G100) on S3's `89118bae`, which carries:
- the 102-setting preset (`e308c14f`), with FIXES-N26's four fixes, `suspectRecovery=downweight`
  and `suspectForcesEscalation=false`;
- S3's `--traceCovariance`.

This commit adds this file, `scripts/acc_a_run.sh` and `scripts/acc_a_analyse.py`, and nothing
else. Cell outputs go to a temporary directory outside the repository. Afterwards, the tables,
each cell's `arm_summary.csv`, `subject_summary.csv`, `run_id.txt` and `rc`, and a SHA-256
manifest are copied to `results/acc_a_2026-10-09/`. No trace is tracked.

**Grade: exploratory.** It is one machine (cloud, g++ 13.3, `-march=native`), with no seeds held
out for tuning, and no adoption into the preset (S1's call). It answers the Part A plan
(owner, 2026-10-09).

**The questions:**
1. Does correcting the EKF covariance change what `ca_tbl_full` achieves?
2. Which consumers of P change with it: the innovation gates, PCS, integrity and escalation?
3. Is the integrity trigger worth what it costs?
4. At corrected covariance, does CA-TBL's decision rule beat `fixed_threshold` when both get the
   same measurements?

## 1. The intervention

`--imuQuScale=k` multiplies the IMU increment covariance handed to every filter (G100). It is a
stand-in for the run-persistent bias and scale error the EKF treats as independent increments.
S2 measured the variance 3-54× too small (`context.md` §6.5 on `claude/kind-turing-csi8ok`).

**k = 10** is registered as the plan's "validated starting intervention": an exploratory
env-var patch measured it. **k ∈ {3, 30}** runs on `ca_tbl_full` only, as a dose-response, so k = 10
is not taken on faith.

**Deviation from S2's plan:** no tuning sweep on seeds 21-30, for time. So k is not tuned. It is
the a-priori 10, bracketed by 3 and 30.

## 2. Cells (`acc_a_run.sh`; `--specProfile=true`, 1 800 s, traces and `--traceCovariance` on)

| cell | arms / flags | k | configurations | seeds |
|---|---|---|---|---|
| M_{A,B}_k{1,10} | `ca_tbl_full, fixed_threshold, ca_tbl_no_integrity, static_2p4, static_868, lora_fusion, poucet_fusion` | 1, 10 | A 5 km; B endgame (`--boxHalf=750 --finderMode=1`) | 1-20 |
| E_{A,B}_k{1,10} | `ca_tbl_full --suspectForcesEscalation=true` (integrity-triggered escalation) | 1, 10 | A, B | 1-20 |
| K_{A,B}_k{3,30} | `ca_tbl_full` | 3, 30 | A, B | 1-20 |

That is **240 cells**. Each arm resets its own random stream, so the arms of a cell are separate
runs.

**The integrity ablation, under the preset as it is.** The preset already turns forced
escalation OFF and downweights SUSPECT fixes. So "integrity-triggered escalation" is the E cells,
which turn it back ON explicitly, and "no integrity check" is the `ca_tbl_no_integrity` arm.
Both are compared with the preset's `ca_tbl_full`, paired by seed, at k = 1 and at k = 10. The
check's distance reads P, so its behaviour can change with k.

**Pairing is within this binary only.** No comparison is made with S2's exploratory cells,
FIXES-N26, CAMP-FINAL or TOF-FIELD. Rule 8 holds: a different k changes the estimates, so the
escalations, so the stream. Only paired distributions over 20 seeds are results.

## 3. Metrics (`acc_a_analyse.py`)

**Per arm and seed:**
- five-node fix availability;
- five-node within 25 m: the share of all cycles, coasting included, with error ≤ 25 m, averaged
  over tag 0 and subjects 2-5. This is S2's definition.
- tag 0 CEP50 / CEP95: the arm summary's per seed. Pooled CEP over all cycles of all seeds
  (S2's convention) is reported beside it.
- J per node: the subjects' mean `totalJ`.

**Consumers of P** (tag 0, fix rows at t ≥ 120 s, the FIXES-N26 warm-up, OURS):
- **NEES:** e′P⁻¹e. Per-seed ANEES, its median, and the share above χ²₂,₀.₉₅ = 5.991.
- **NIS:** the integrity distance, a 2-DOF NIS under the preset's `--mahalUseR`. The share above
  9.21 and the median.
- **SUSPECT:** the share of fix rows SUSPECT (fault-free, so every one is false).
- **TDoA innovation gate:** refusal rate = refused / (refused + fused), from the run log's N.26
  line, tag 0 and subjects. This is the preset's innovation gate. The trace's `rejected` column
  (a revert) is reported, but under `downweight` it is expected to be 0.
- **PCS informativeness:** −Spearman(pcs, error) against Spearman(traceSigma, error), paired by
  seed. Positive Δ means PCS predicts error better than the trace it is computed from. This is
  the `PCS_FORMS` control.
- **Escalation:** escalations, UWB cycles and UWB-delivered fixes.
- **Divergence:** max error, and the share of rows with error > 1 km.

**Energy (tag 0):**
- J per tier (868 / 2.4 GHz / UWB) and the total;
- J per fix;
- J per cycle within 25 m;
- ΔJ per metre of CEP95, `ca_tbl_full` against `fixed_threshold`.

TDoA overhead is billed inside the 868 MHz figures and is not separable in the summaries. That
is stated, not estimated.

**Scoreboard** (S2 §7, reported): `ca_tbl_full` − each arm at the same k, WIN / tie / LOSS on the
95 % interval. Energy is reported, not scored.

## 4. What it decides

Nothing is adopted. The results note must attribute the outcome separately to six causes:
covariance, the measurement fixes, the decision policy, the integrity trigger, energy, and the
propagation assumptions (`report/ACC_A_RESULTS.md`, TECH_DEBT N.30-N.32). **Stop rule for PCS
(Part A §11):** if Q8 holds, no further PCS form is developed.

## 5. Predictions, made before any cell

**Disclosed:**
- S2's exploratory ×10 runs on another machine. `ca_tbl_full` went from 16.2 / 85.0 to
  11.7 / 61.7 m at A and from 4.8 / 16.1 to 2.7 / 8.0 m at B, pooled CEP50 / CEP95. Fix
  availability was a WIN over every arm, and accuracy tied `fixed_threshold` and `static_2p4`.
  These are on `claude/kind-turing-csi8ok` and are not data for this registration.
- G100's runs (seed 7, 600 s).
- A dry run of this pipeline (seeds 1-2, 60 s, all 24 cells). It printed verdicts on that grid,
  and no prediction was changed after it. After it, the E-cell reachability check was corrected
  to read the run's invocation and `spec_profile.txt`'s "explicit" line, which is how that file
  records an explicit flag.

| # | prediction | confidence |
|---|---|---|
| **Q1** | replication: at k = 10, `ca_tbl_full` pooled CEP50 ≤ 13 m and CEP95 ≤ 70 m in A; ≤ 3.2 m and ≤ 10 m in B | medium |
| **Q2** | k = 10 − k = 1, `ca_tbl_full` per-seed CEP95: interval below 0 in A and in B | medium |
| **Q3** | NEES: at k = 1 the median ANEES exceeds 4 (overconfident) in A and B, and k = 10 − k = 1 ANEES is below 0 in both | medium |
| **Q4** | NIS: k = 10 − k = 1 share above 9.21 is below 0 in A and B (fewer inappropriate innovations) | medium |
| **Q5** | the gate still discriminates at k = 10: share of NIS above 9.21 > 0.005 in A and B (not every measurement passes) | high |
| **Q6** | no arm's mean availability moves by more than 0.01 (k = 10 − k = 1), A and B | low |
| **Q7** | no divergence: no arm's pooled CEP95 at k = 10 exceeds its k = 1 value by more than 10 %, A and B | medium |
| **Q8** | PCS stop condition: at k = 10, PCS − trace informativeness is not above 0, in A or in B | high |
| **Q9** | integrity: at k = 1, the fault-free SUSPECT share is ≤ 0.02 in A and B, and k = 10 − k = 1 is not above 0 | medium |
| **Q10** | forced escalation at k = 1: \|Δ availability\| ≤ 0.01 and Δ CEP95 interval contains 0, A and B (SUSPECT is rare at χ² 1992.9) | medium |
| **Q11** | integrity off − preset: availability and CEP95 intervals contain 0, at k = 1 and k = 10, A and B | medium |
| **Q12** | policy at corrected covariance (k = 10): `ca_tbl_full` − `fixed_threshold` availability interval above 0 and CEP95 interval contains 0, A and B | low |
| **Q13** | escalation: k = 10 − k = 1 `ca_tbl_full` escalations interval above 0, A and B (a larger, honest P lowers PCS term 1, so it escalates more, not less) | low |

Energy is reported, not predicted.

## 6. Abort conditions (the analysis's "Problems")

Any of these stops the write-up. The cause is fixed, and no cell is dropped:
- not 240 cells with an rc, or a cell exits non-zero;
- a `run_id.txt` does not read one source with `dirty 0`;
- an arm/cell has fewer than 20 seeds, or a seed with no covariance rows;
- an E cell where any seed does not show `--suspectForcesEscalation=true` in its invocation and
  as explicit in `spec_profile.txt`;
- a registered prediction cannot be evaluated.
