# PREREG ACC-A-OPPOINT — is `ca_tbl_full`'s gain over `fixed_threshold` only an operating point? (exploratory grade)

**Registered before any cell runs** (rule 13). Written 2026-10-09. It tests reading 4 of
`report/ACC_A_DECISION_DIAGNOSIS.md` (`58f3a98c`):
- in the preset, PCS is a function of trace(P) alone;
- `ca_tbl_full` therefore differs from `fixed_threshold` only in its thresholds (≈ 8-12 m²
  against 100/50 m²);
- its endgame gain (availability +0.031, +6.1 J per node at k = 10; ACC-A §4) is an operating
  point.

If so, `fixed_threshold` with its one threshold lowered into that region should reproduce the gain.

This commit adds this file, `scripts/acc_a_oppoint_run.sh` and `scripts/acc_a_oppoint_analyse.py`,
and nothing else.

## 1. Engine and pairing

- **Engine.** The frozen ACC-A build, unchanged: binary SHA-256 `38c306df…`, as listed in that
  campaign's build manifest. It is built from `681e90c8`, so every `run_id.txt` will read
  `source 681e90c8 dirty 0`. This commit and `58f3a98c` change only `report/` and `scripts/`.
- **Running the same binary is deliberate.** It lets each `fixed_threshold` run pair per seed with
  ACC-A's own `ca_tbl_full` run (`M_{A,B}_k10`).
- **Pairing across runs is valid here.** ACC-A showed `ca_tbl_full` run alone byte-identical to its
  row in the seven-arm run (E against M, ACC-A §3). Control C below repeats that check for
  `fixed_threshold`.
- **Outputs** go to a temporary directory outside the repository. Afterwards, each cell's
  `arm_summary.csv`, `run_id.txt` and the tables are copied to
  `results/acc_a_oppoint_2026-10-09/`, with a SHA-256 manifest. No trace is tracked.

## 2. Cells (`acc_a_oppoint_run.sh`)

Common flags: `--arms=fixed_threshold --specProfile=true --duration=1800 --trace=true
--traceCovariance=true --imuQuScale=10`.

| cell | configuration | `--fixedVarThreshold` (m²) | seeds |
|---|---|---|---|
| C_B_T100 | B (`--boxHalf=750 --finderMode=1`) | 100 (= the default, explicit) | 1-20 |
| O_B_T8, O_B_T12, O_B_T25 | B | 8, 12, 25 | 1-20 |
| O_A_T12 | A (5 km) | 12 | 1-20 |

That is **100 runs**.

**Why these thresholds.** `fixed_threshold` escalates above T and counts as healthy below T/2.
`ca_tbl_full`'s equivalents (diagnosis §1) are:
- escalate above 8.3 (868 MHz) and 11.8 (2.4 GHz);
- healthy below 9.4 (2.4 GHz) and 5.9 (UWB).

T = 12 is the single value closest to all four. 8 and 25 bracket it.

## 3. Metrics (`acc_a_oppoint_analyse.py`)

The per-seed metrics are ACC-A's own (`acc_a_analyse.seed_metrics`):
- five-node availability;
- J per node;
- tag 0 CEP50 / CEP95;
- five-node within 25 m;
- escalations;
- UWB cycles;
- UWB-solved fixes.

Each comparison is **`fixed_threshold`@T − `ca_tbl_full`**, paired over seeds 1-20, as mean
[95 % bootstrap]. The share of `fixed_threshold`'s escalation decisions made by its own rule
(trace > T on a decision row) is reported descriptively.

## 4. Predictions, made before any cell

**Disclosed:**
- ACC-A's results, and the diagnosis, including its numbers at T = 100: availability −0.031
  [−0.041, −0.021] and J −6.11 [−8.23, −4.05] in B.
- No cell of this design has been run.

| # | prediction | confidence |
|---|---|---|
| **R1** | control: every `fixed_threshold` file in C_B_T100, its `arm_summary.csv` row and its `subject_summary.csv` rows are byte-identical to ACC-A M_B_k10's, 20 of 20 seeds | high |
| **R2** | B, T = 12: availability interval contains 0 | medium |
| **R3** | B, T = 12: J per node interval contains 0 | medium |
| **R4** | B, T = 12: CEP50 and CEP95 intervals each contain 0 | high |
| **R5** | B: `fixed_threshold`'s mean availability and mean J per node are each non-increasing in T across 8, 12, 25, 100 | medium |
| **R6** | B: at least one T in {8, 12, 25} has availability and J intervals that both contain 0 | medium |
| **R7** | A, T = 12: availability, CEP50 and J intervals each contain 0 | medium |
| **R8** | B, T = 12: UWB cycles per run, interval contains 0 | medium |

**Falsifier of the reading.** R6 fails if, at every T, availability and J cannot both be matched.
In that case `ca_tbl_full`'s per-tier thresholds or hysteresis do something a single variance
threshold cannot, and diagnosis reading 4 is wrong as stated.

R2-R4 failing at T = 12 while R6 holds means only that T = 12 was not the matching point.

## 5. Abort conditions

The cause is fixed, and no cell is dropped:
- not 100 runs with an `rc`, or a run exits non-zero;
- a `run_id.txt` that does not read `source 681e90c8 dirty 0`;
- a cell with fewer than 20 seeds;
- a prediction that cannot be evaluated.

An R1 failure is reported as a leak, and no other result is read until it is explained.
