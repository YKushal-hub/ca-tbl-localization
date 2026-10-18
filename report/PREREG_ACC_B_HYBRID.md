# PREREG ACC-B — relay legs on 2.4 GHz under the dual base (S2 Part B, F1; exploratory grade)

**Registered before any cell runs** (rule 13). Written 2026-10-09. Engine: the build of `caf441bb`
(`--dualBaseRelayTier`, gate G110) on S3's `89118bae`, which has the 102-setting preset. The
diagnosis it acts on is TECH_DEBT N.35 (`6cd740c6`). This commit adds this file,
`scripts/acc_b_run.sh` and `scripts/acc_b_analyse.py`, and nothing else.

Outputs go to a temporary directory outside the repository. Afterwards, the tables, each cell's
`arm_summary.csv`, `subject_summary.csv`, `run_id.txt` and `rc`, and a SHA-256 manifest are copied
to `results/acc_b_2026-10-09/`. No trace is tracked.

**Grade: exploratory.** One machine (cloud), no tuning seeds (F1 has no constant to tune), no
adoption (S1's call). This deviates from S2's plan in three ways:
- **B1's range log is not built.** "Precise ranges per cycle" is replaced by two proxies:
  ranges entering the EKF per cycle, and tag 0's relay legs offered and admitted.
- **F2 (H2) is not built.**
- **`fixed_threshold` is not run.** The arms are `ca_tbl_full` and `static_2p4` only. This is
  for time.

## 1. Cells (`acc_b_run.sh`; `--specProfile=true`, 1 800 s, traces on)

| cell | flags | configuration | seeds |
|---|---|---|---|
| off_A, off_B | none | A 5 km; B endgame (`--boxHalf=750 --finderMode=1`) | 1-20 |
| on_A, on_B | `--dualBaseRelayTier=true` | A; B | 1-20 |

Arms: `ca_tbl_full,static_2p4`. That is **80 cells**. `static_2p4` is both the comparator and the
isolation check, since its files must be byte-identical on and off. Pairing is within this binary
and campaign only.

## 2. Metrics (`acc_b_analyse.py`)

- **Per seed, `ca_tbl_full` and `static_2p4`:**
  - five-node availability;
  - five-node within 25 m (S2's definition, all cycles);
  - tag 0 CEP50 / CEP95 (arm summary, per seed);
  - J per node;
  - ranges into the EKF per cycle (trace `nRangesEkf`);
  - tag 0's relay legs offered and admitted (the run log's C5 line);
  - each subject's median error.
- **Paired comparisons:** on − off for `ca_tbl_full`; on − `static_2p4`; off − `static_2p4`.

## 3. Predictions, made before any cell

**Disclosed:**
- N.35's diagnosis numbers (TOF-FIELD's D0 cells, read only).
- G110's runs: endgame, seed 7, 600 s; on gave 428 legs, 206 admitted.
- A dry run: seeds 1-2, 60 s, all 8 cells; it printed verdicts, and no prediction was changed
  after it.
- S2's plan (P-B1–P-B5), from which these are adapted.

| # | prediction | confidence |
|---|---|---|
| **Q1** | B: tag 0's relay legs offered at least double (mean on ≥ 2 × mean off), and on − off admitted has its interval above 0 | high |
| **Q2** | B: ranges into the EKF per cycle, on − off, interval above 0 | high |
| **Q3** | B: tag 0 CEP50 and CEP95 each lower, intervals below 0 and mean per-seed change ≤ −10 % (S2's P-B2) | low (the endgame is near saturation) |
| **Q4** | A: five-node within 25 m, on − `static_2p4`, no significant LOSS (interval not below 0) (S2's P-B3) | low (F1 does not touch H2) |
| **Q5** | availability on − off ≥ −0.01 (mean) in A and B, and J per node mean per-seed change ≤ +15 % in A and B (S2's P-B4) | medium |
| **Q6** | `static_2p4` files and summary rows byte-identical on and off, every seed (S2's P-B5) | high |

## 4. Abort conditions

The cause is fixed, and no cell is dropped:
- not 80 cells with an rc, or a cell exits non-zero;
- a `run_id.txt` does not read one source with `dirty 0`;
- an arm/cell has fewer than 20 seeds;
- an on cell whose B-F1 line shows no tag-0 legs (rule 4);
- a prediction cannot be evaluated.

A Q6 failure is reported as a leak, and no other result is read until it is explained.
