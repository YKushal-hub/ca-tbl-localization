# HANDOFF 2026-10-09 — S2 Part A (filter consistency), branch `work/2026-10-09-acc-part-a`

Done in one cloud session, overnight. Base: S3's `89118bae`, which has the 102-setting preset
and `--traceCovariance`. Plan: `plan_part_a_filter.md` on `work/2026-10-09-acc-reruns`, revised by
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
