# HANDOFF 2026-10-09 — S2 Part A (filter consistency), branch `work/2026-10-09-acc-part-a`

Done in one cloud session, overnight. Base: S3's `89118bae`, which has the 102-setting preset
and `--traceCovariance`. Plan: `plan_part_a_filter.md` on `claude/kind-turing-csi8ok`, revised by
the owner's "PART A — Filter Consistency + CA-TBL Pending Work" (2026-10-09).

| commit | what |
|---|---|
| `81fb1485` | A1 `--imuQuScale` and gate G100. NC1-NC9 with the flag off: 41/41 identical to the S3 head |
| `681e90c8` | PREREG_ACC_A_FILTER: 240 cells, 13 predictions |
| `eadcb0b3` | LOCAL_RUN_2026_10_09: the local machine's verification task. It writes to `work/2026-10-09-local-verification` |
| `4d219963` | the analysis's abort check corrected to the registered condition (disclosed) |
| this commit's parent and this | results: `report/ACC_A_RESULTS.md`, `results/acc_a_2026-10-09/`, TECH_DEBT N.30-N.32 |

**Answers to the owner's questions:**
1. **Does correcting the covariance remove CA-TBL's disadvantage?** It improves CA-TBL's accuracy
   (endgame CEP95 −4.5 m, 5 km CEP50 −7.9 m). It improves every other arm too, so it is not a
   CA-TBL advantage. The 5 km five-node within-25 m loss to `static_2p4` remains (it is the
   subjects', N.35).
2. **Does the CA-TBL rule beat `fixed_threshold` on the same measurements?** On endgame
   availability, yes: +0.017 at k = 1 and +0.031 at k = 10. On accuracy, never. It costs energy
   (+6.1 J per node at k = 10). TOF-FIELD finds the same at measured ToF error.
3. **PCS:** the stop rule is met (N.31).
4. **Integrity:** forced escalation is redundant in the preset (N.32).

**Not done:**
- A2, the error-state filter;
- tuning k on seeds 21-30 (left to the local machine);
- CAL-03 refit at k = 10;
- A5, energy-aware escalation;
- the integrity re-fit at consistent covariance;
- the full suite on this branch (left to the local machine).

**Merge:** per WORKSTREAMS §4. Expect append-only conflicts with Part B in `SpecOptions`, the
flag block and `catbl_gates.sh` (G100 and G110). Keep both sides, and regenerate SCENARIO_FLAGS
with `tools/regen_flags.py`.

**G10 (citation audit):** one ABSENT, `report/TOF_FIELD_RESULTS.md`, until S3 (which has it) merges first.

**Local verification** (`work/2026-10-09-local-verification`, `2552bd9d`, run on the owner's machine on `eadcb0b3`):
- null control: identical to `1596cb41`;
- G100 and G120: pass;
- full suite: 81 PASS. The reds are the five expected ones, plus G10 (fixed here), plus G63, G87 and G89. Those three were already red on `1596cb41`, from S1's preset change `e308c14f`, and are S1's to fix.
- Its k sweep on seeds 21-30: k = 10 is best in B; at 5 km it is mixed (one seed carries the rise).

**Added later the same day (post-hoc diagnoses and one registered test):**
- `58f3a98c`, `report/ACC_A_DECISION_DIAGNOSIS.md`.
  - In the preset, PCS = 2.9947·10/(10 + trace P) + [0, 0.0053] on every decision row, so
    `ca_tbl_full` is a variance-threshold rule.
  - In the endgame, 85-99.5 % of escalations come from the shared URGENT-query hook.
  - The dual base makes the 868 ↔ 2.4 GHz choice nearly irrelevant to what is measured.
- `5c28eb75` PREREG, then `72c309b0`, `report/ACC_A_OPPOINT_RESULTS.md`.
  - `fixed_threshold` at T = 25 m² matches `ca_tbl_full`'s endgame availability and accuracy for
    4.19 J less per node; T = 12 gives more availability.
  - **`ca_tbl_full`'s registered operating point is inside the single-threshold curve.**
  - Disclosed correction: `f42e9ec4`. The run_id "source" line is the worktree HEAD, not the
    build commit.
- `4ece0f0f`, `report/ACC_A_STATIC2P4_DIAGNOSIS.md`.
  - Every estimate is seeded at or next to truth. At 5 km, static 2.4 GHz's within-25 m share is
    dead reckoning: 42/80 subject-runs never fix, against 9/80 for the hybrid.
  - The hybrid's subject fixes leave the estimate slightly worse 60-76 % of the time.
  - **The unknown-start case is not simulated.**

**Not done, added:**
- a `ca_tbl_full` threshold sweep (can it reach the single-threshold curve at all?);
- an unknown-start scenario;
- the subjects' NIS and measurement-weighting check (the likely cause of the 5 km within-25 m loss).
