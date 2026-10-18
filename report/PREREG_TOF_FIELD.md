# PREREG TOF-FIELD — tier-2 ToF ranging at measured outdoor error, on the 102-setting preset

**Registered before any cell runs** (rule 13). Written 2026-10-08/09. Engine: the build of
`342d33b8`: S3's flags and gate G120 (`d761eeb9`) and the citation comments (`342d33b8`, no
output changed), on S1's `1596cb41`, whose preset (`e308c14f`) carries FIXES-N26's four fixes. This commit
adds this file, `scripts/tof_field_run.sh` and `scripts/tof_field_analyse.py`, and nothing else.

Cell outputs go to a temporary directory outside the repository, which does not exist at
registration. Afterwards these are copied to `results/tof_field_2026-10-09/`, which does not
exist either:
- the tables;
- each cell's `arm_summary.csv`, `subject_summary.csv`, `run_id.txt` and `rc`;
- a SHA-256 manifest of every raw file.
No trace is tracked.

**Why.**
- Tier 2's ranging sigma floor is 3.0 m LOS / 3.6 m NLOS (`RangingParams`). It rests on ranges
  measured within tens of metres, yet tier 2 is used out to r80 = 1149.5 m.
- Mueller et al., ICL-GNSS 2021 (doi 10.1109/ICL-GNSS51451.2021.9452277), measured outdoor
  SX1280 ranging:
  - rural, to 1400 m, SF 8 / 400 kHz, ~25 000 ranges: SD **83.6 m**, mean error **+46.4 m**;
  - suburban, to 500 m: SD **45 m**, mean **+39 m**;
  - their linear correction (0.95 d + 1) brings the mean below 10 m.
- The campaign asks which of the project's comparisons depend on the 3 m floor, re-running
  every arm that consumes tier-2 ranges at the field error.

Scope (`WORKSTREAMS_2026_10_09.md` §2): filter consistency (NEES) is S2's and is not measured
here. "CA-TBL⁺" is now the preset itself.

## 1. Settings

| tag | flags | what it stands for |
|---|---|---|
| D0 | none | the engine as every result so far used it |
| R1 | `--tofFieldSigmaM=83.6 --tofFieldBiasM=46.4` | Mueller rural, uncorrected |
| R0 | `--tofFieldSigmaM=83.6 --tofFieldBiasM=0` | rural spread with the bias calibrated away |
| S | `--tofFieldSigmaM=45 --tofFieldBiasM=39` | Mueller suburban, uncorrected |

- The sigma is applied as max(floor, value) to both tier-2 floors. The EKF sees it through each
  range's sigma.
- The bias is added to every tier-2 range with no random draw (G120 shows no draw moves). The
  EKF is not told.
- Using the measured SD as a floor at every distance is OURS. Mueller report one SD per route,
  not per distance.

## 2. Cells (`tof_field_run.sh`; `--specProfile=true`, 1 800 s, traces on)

| cell | arms | configuration | seeds |
|---|---|---|---|
| X_main_<tag> | `ca_tbl_full,fixed_threshold,static_2p4,lora_fusion,poucet_fusion` | A, the preset at 5 km | 1-20 |
| X_end_<tag> | the same | B, the endgame (`--boxHalf=750 --finderMode=1`) | 1-20 |

That is 4 settings × 2 cells × 20 seeds = **160 cells**, five arms each. Each arm resets its
own random stream, so an arm's run inside a cell is the same as a run of that arm alone.

**Why `fixed_threshold` (added at the owner's request, 2026-10-08, before registration).**
FIXES-N26 compared CA-TBL with all four fixes against a `fixed_threshold` that had none of them.
Two of the fixes are measurement fixes, not decision-rule fixes, and they carry most of the gain:
- D, the dual LoRa base;
- T, the TDoA gate.
So that comparison could not separate the decision rule from the fixes. Under the 102-setting
preset, `--specProfile` sets those flags for every arm. D (`hybridArm`), U and T (mode-agnostic)
are live in `fixed_threshold` exactly as in `ca_tbl_full`. M is not: the integrity check exists
only in `ca_tbl_full`. So preset − `fixed_threshold` compares the decision rules on equal
measurements: PCS with its hysteresis and gates, against escalation on the raw position
variance.

**Pairing is within this binary only.** Every comparison is between settings, or between arms,
of this campaign, paired by seed. No cell is compared with CAMP-FINAL, CAMP-FINAL-2, LORA-CMP-2
or FIXES-N26. This build is on a different machine and compiler (g++ 13.3) from theirs, and
the build uses `-march=native` (TECH_DEBT 4.2).

Rule 8 applies within the campaign too. A setting changes the tier-2 ranges, which changes
escalations and transmissions and so displaces the shared stream. No single-seed difference is
a result; only the paired distributions over 20 seeds are.

**Isolation.** The cells run on the cloud, from a git worktree at this commit holding a byte copy
of the engine build. Its SHA-256 goes in the results directory's RUN.md.

## 3. Metrics (`tof_field_analyse.py`)

**Per configuration, setting and arm:** LORA-CMP's per-seed quantities
(`lora_cmp_analyse.per_seed`):
- availability;
- tag 0 CEP50 / CEP95;
- subject CEP95;
- subject J.

**Paired differences:**
- each setting − D0, per arm;
- within each setting:
  - preset − `fixed_threshold`;
  - preset − dual-band LoRa fusion;
  - preset − Poucet;
  - static 2.4 GHz − preset.

## 4. What the campaign decides

Nothing is adopted. The settings are a sensitivity analysis, not a candidate default.

It says which tier-2-dependent comparisons survive measured field error. A comparison that fails
under R1, R0 or S is reported as **conditional on the 3 m ToF floor** wherever it is cited. Its
result is an input to S2's WP-A4 registration (`WORKSTREAMS_2026_10_09.md` §2 D).

## 5. Predictions, made before any cell

**Disclosed:**
- G120's runs (seed 7, 600 s, default deployment; the probe), made while building the flags.
- FIXES-N26's results (the ALL rows, which the preset now equals in its flags), on another
  binary.
- A dry run of this pipeline on a throwaway grid outside the repository (seeds 1-2, 60 s, every
  cell; all 16 exit 0, and the analysis's abort checks fire on the undersized grid as they should).
  The analysis printed verdicts on that grid; they are 60 s of two seeds, and no prediction was
  changed after it. The rule-4 check was changed after it, from "TDoA fused > 0" to "TDoA judged
  (fused or refused) > 0": a gate that refuses every position is live, not absent.
- Two timing cells (seed 1, 1 800 s, A and B, all five arms): 191 s and 222 s wall, two at a time;
  43 and 62 MB each.

None of these is data for this registration.

| # | prediction | confidence |
|---|---|---|
| **Q1** | static 2.4 GHz, A and B: R1 − D0 tag 0 CEP95 has its interval above 0 | high |
| **Q2** | static 2.4 GHz, A and B: R0 − D0 tag 0 CEP95 has its interval above 0 (the spread alone degrades it) | high |
| **Q3** | static 2.4 GHz, LoRa fusion and Poucet, A and B: \|mean Δ availability\| (R1 − D0) ≤ 0.02 (ranging error does not change delivery) | medium |
| **Q4** | the preset, B: R1 − D0 tag 0 CEP95 has its interval above 0 | medium |
| **Q5** | the preset, A: R1 − D0 tag 0 CEP95 has its interval above 0 (the dual base fuses 2.4 GHz ranges every cycle) | medium |
| **Q6** | preset − LoRa fusion availability has its interval above 0 under all four settings, A and B | medium |
| **Q7** | preset − Poucet availability has its interval above 0 under all four settings, A and B | medium |
| **Q8** | B, R1: preset − LoRa fusion tag 0 CEP95 has its interval below 0 (UWB, which LoRa fusion lacks, carries the preset's endgame accuracy) | low |
| **Q9** | D0: preset − `fixed_threshold` availability interval contains 0, in A and in B (the decision rule adds nothing once both have the fixes; PCS carries no information beyond the covariance trace, `report/PCS_FORMS_RESULTS.md` §2) | medium |
| **Q10** | D0: the same comparison's tag 0 CEP95 interval contains 0, in A and in B | medium |
| **Q11** | R1: Q9's and Q10's intervals both contain 0, in A and in B | low |
| **Q12** | B, D0: static 2.4 GHz − preset tag 0 CEP95 has its interval below 0 (static 2.4 GHz's accuracy lead exists at the 3 m floor) | medium |
| **Q13** | R1, A and B: that interval is not below 0 (the lead does not survive field error) | medium |
| **Q14** | B: the preset's mean tag 0 CEP95 under R1 is more than twice its D0 value (its endgame accuracy does not survive) | low |

Each is written up as held or failed in `report/TOF_FIELD_RESULTS.md`, with TECH_DEBT N.40. The
analysis prints the verdicts mechanically, as registered.

## 6. Abort conditions (the analysis's "Problems")

Any of the following stops the write-up. The cause is fixed, and no cell is dropped:
- not 160 cells with an rc, or a cell exits non-zero;
- a `run_id.txt` does not read one source with `dirty 0`;
- an arm in a cell has fewer than 20 seeds;
- in any configuration and setting, `ca_tbl_full`'s or `fixed_threshold`'s N.26 lines sum to zero
  dual-base node-cycles, or to zero TDoA positions judged by the gate, fused or refused (rule 4:
  the preset's fixes did not reach that arm);
- a registered prediction cannot be evaluated.
