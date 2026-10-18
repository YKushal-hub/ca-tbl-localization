# PREREG ACC-B2 — subjects fuse precise ranges without 868 MHz (S2 Part B, F2; exploratory grade)

**Registered before any cell runs** (rule 13). Written 2026-10-09. Engine: the build of `6c0c3194`
(`--subjectPreciseOnly`, gate G111) on this branch. The hypothesis is H2 of TECH_DEBT N.35. This
commit adds this file, `scripts/acc_b2_run.sh` and `scripts/acc_b2_analyse.py`, and nothing else.
Outputs go to a temporary directory outside the repository. Afterwards the tables, the per-cell
`arm_summary.csv` and `run_id.txt`, and a manifest are copied to `results/acc_b2_2026-10-09/`.

**Why.** ACC-B (F1) left the 5 km five-node within-25 m loss to static 2.4 GHz in place (−0.064).
That loss is the subjects'. N.35 found the hybrid subjects' error the same on either active tier,
while static 2.4 GHz's subjects, which fix rarely, are more accurate. H2 says the 868 MHz (RSSI)
information the dual base fuses pulls the subjects' estimates off. F2 removes it from the fused
set whenever two or more precise ranges exist. The threshold of two is OURS. Ranging and billing
are unchanged, so this is a pure estimator-input test.

## Cells (`acc_b2_run.sh`; `--specProfile=true`, 1 800 s, traces on)

| cell | flags | configuration | seeds |
|---|---|---|---|
| off_A, off_B | none | A 5 km; B endgame | 1-20 |
| on_A, on_B | `--subjectPreciseOnly=true` | A; B | 1-20 |

The arms are `ca_tbl_full,static_2p4`: **80 cells**. The metrics are those of ACC-B, plus `smed`,
the per-seed mean over subjects 2-5 of each subject's median error. The off cells are run again
here rather than reused from ACC-B. The binary differs, and pairing stays within one binary.

## Predictions, made before any cell

**Disclosed:**
- N.35;
- ACC-B's results (F1, same branch);
- G111's runs: endgame, seed 7, 600 s; 207 subject-cycles fused without 868 MHz;
- a dry run: seeds 1-2, 120 s, all 8 cells. F2 never fired in 2 of the 4 on cells there, and
  no prediction was changed.

| # | prediction | confidence |
|---|---|---|
| **Q1** | A: subjects' median error (smed), on − off, interval below 0 (H2) | low |
| **Q2** | A: five-node within 25 m, on − off, interval above 0 | low |
| **Q3** | A: five-node within 25 m, on − static 2.4 GHz, no significant LOSS | low |
| **Q4** | A: five-node availability, on − off, interval below 0 (fewer subject fixes once 868 MHz is left out) | medium |
| **Q5** | tag 0 CEP95, on − off, interval contains 0, A and B (tag 0 is untouched; only interactions through relays) | medium |
| **Q6** | `static_2p4` byte-identical on and off | high |

## Abort conditions

- not 80 cells with an rc, or a cell exits non-zero;
- a `run_id.txt` not reading one source with `dirty 0`;
- an arm/cell with fewer than 20 seeds;
- F2 firing in no on cell at all (rule 4). The number of on cells where it never fired is
  reported.
- a prediction that cannot be evaluated.
