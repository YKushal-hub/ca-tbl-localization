# HANDOFF 2026-10-09 — S2 Part B (hybrid range supply), branch `work/2026-10-09-acc-part-b`

Done in one cloud session, overnight, end to end. Base: S3's `89118bae`, which has the 102-setting
preset. Plan: `plan_part_b_hybrid.md` on `work/2026-10-09-acc-reruns`.

| commit | what |
|---|---|
| `caf441bb` | F1 `--dualBaseRelayTier` and gate G110. NC1-NC9 with the flag off: 41/41 identical to the e6b3da9 reference; 40/41 against the S3 head, the exception being the 5 TECH_DEBT 4.1 cells, which flip from build to build |
| `6cd740c6` | TECH_DEBT N.35, the diagnosis (B2), from TOF-FIELD's D0 cells |
| `0f68d454` | PREREG_ACC_B_HYBRID, 80 cells, 6 predictions |
| `afa46d41` | results: `report/ACC_B_RESULTS.md`, `results/acc_b_2026-10-09/`, TECH_DEBT N.36 |

**The answer:**
- The dual base kept tag 0's relay legs on the active tier, which is 868 MHz on 64 % of endgame
  cycles.
- F1 restores the legs (198 → 894 per run) and adds +0.030 endgame availability.
- It does not improve endgame accuracy, and it does not close the 5 km five-node within-25 m loss
  to static 2.4 GHz. That loss is the subjects' (H2), which is not built.

**Not done, for the next session:**
- F2 / H2 (`--subjectFloorGate`, or replaying the subjects' tier-1 / TDoA fixes);
- B1's range log;
- F1 with the `fixed_threshold` arm;
- B5 (dual base on demand, energy);
- the full suite on this branch.

Adoption of F1 into the preset is S1's call. Evidence for: +0.030 endgame availability, energy not
significant there. Against: no endgame accuracy gain, +5 % energy at 5 km.

**Merge:** per WORKSTREAMS §4. Expect append-only conflicts with Part A in `SpecOptions`, the
`cmd.AddValue` block (the S2-A and S2-B blocks both sit after S3's) and `scripts/catbl_gates.sh`
(G100 and G110 both appended). Keep both sides. Regenerate SCENARIO_FLAGS with
`tools/regen_flags.py` after merging.
