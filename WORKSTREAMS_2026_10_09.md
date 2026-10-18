# Parallel work on CA-TBL — who owns what (2026-10-09)

Three lines of work are running at the same time. This file says what each owns, where they
touch, and the rules that keep them from overwriting each other. **Read it before starting work
on any of them.** If your work needs something owned by another stream, write it in your own
handoff as a request and do not do it yourself.

Check `git log` on each branch before trusting this file, as with every status document.

---

## 1. The three streams

| | S1 — spec, preset, campaigns | S2 — accuracy plan | S3 — evidence fixes |
|---|---|---|---|
| **Branch** | `work/2026-09-27-spec-b1` (the main line), plus `draft/camp-final2` | `claude/kind-turing-csi8ok` (cloud) | `review/2026-10-08-evidence-fixes` |
| **Where it runs** | Local machine, main checkout `~/projects/ca-tbl-localization`; suite worktree `~/catbl_suite` | Cloud | Cloud (local worktree `~/catbl_evfix` exists, but the local machine is S1's) |
| **State document** | `HANDOFF_2026_10_05.md` and the commit messages after it | `plan.md`, `context.md` | `HANDOFF_2026_10_08.md` |
| **Head when written** | `1596cb41` | `d4033ffb` | `493d00c1` |
| **Base** | — | `a0829213` (before row 17 and the preset change) | `b98c4913`, the **pre-amend** row 17 commit (now `d7ada989`) |

### S1 — what it has done and what is next
- `d7ada989` row 17: `--pcsTerm1Inverse`, gate G90.
- `e308c14f` **the preset now includes FIXES-N26's four fixes** (owner override): `--specProfile`
  sets `--loraDualBase`, `--uwbCrossTierPoll`, `--tdoaInnovationGate`, `--mahalUseR` and
  `--integrityChi2=1992.9`. That is 102 settings.
- `1596cb41` full suite evidence: 80 PASS, expected reds only.
- **Next:** register and run CAMP-FINAL-2 (`draft/camp-final2`, `c432134e`:
  `report/PREREG_CAMP_FINAL2.md`, `scripts/camp_final2_{run.sh,analyse.py}`). That is CAMP-FINAL's
  runner with `lora_fusion` as an eighth arm. Paper tables are regenerated from it.

### S2 — what its plan covers (`plan.md` §2)
WP-0 instruments (`--rangeLog`, `--imuBiasScale/--imuScaleScale/--imuArwScale`, `--arms=dr_only`,
`--maxTier`, replay fidelity), WP-A1a `--imuQuScale`, WP-A2 TDoA gate adoption, WP-A1b
error-state IMU filter, WP-A3 accuracy-based availability and the dead-reckoning comparator,
WP-A4 2.4 GHz on demand, WP-A5 UWB cap, WP-A6 tier-1 SF/BW sweep, WP-A7 TDoA root choice, and
the ML track.

### S3 — the evidence fixes (`HANDOFF_2026_10_08.md`)
1. Tier-2 (2.4 GHz ToF) error at the level Müller et al. 2021 measured: flags
   `--tofFieldSigmaM` and `--tofFieldBiasM` (in `13c5a23`, null control passed locally), then a
   registered sensitivity campaign.
2. Citation fixes in the physics engine's comments and in `docs/CONTEXT_PACK.md`.
3. One tier-1 validation plot against a public LoRaWAN dataset.
4. `--traceCovariance`: the instrument only (see §2, overlap B).
7. The UWB reach gap: a write-up from `scripts/uwb_reach_ablation.py`, and the missing ×5.28 UWB
   foliage scaling in `src/catbl-propagation-study/linkbudget.py`.

Items 5 and 6 (rewording the contribution, finding a SAR accuracy requirement) are thesis prose.
They stay **outside this repository** (`CLAUDE.md`).

---

## 2. Where the streams overlap, and what is decided

**A. The TDoA gate is already in the preset.** S2's WP-A2 ("adopt the TDoA innovation gate") was
written before `e308c14f`, which put it there. **S2:** rebase onto S1's head and re-scope WP-A2
as a removal ablation (gate off against the preset). Its registered 2 × 2 with `--imuQuScale`
still stands, with the gate's default now ON.

**B. Filter consistency (NEES) is S2's.** S2 has already measured the covariance as wrong by
3–54× in variance (`context.md` §6.5) and owns the fix (WP-A1a, WP-A1b). **S3 does not run its
own NEES study.**
- S3 delivers only the instrument, `--traceCovariance` (pxx, pxy, pyy on every trace row,
  default off).
- S2 uses it for the NEES it reports in WP-A1a, instead of writing its own covariance dump.
  `--rangeLog` (WP-0) is a separate instrument and stays S2's.
- Until S3's flag merges, S2 keeps using its replay.

**C. Two campaigns on the preset.** S1's CAMP-FINAL-2 and S3's TOF-FIELD both run on
`--specProfile`, which changed at `e308c14f`.
- S3 rebases onto S1's head **before** registering, so "the preset" means the 102-setting one.
  "CA-TBL⁺" is now just the preset: no separate ALL-flags arm.
- S3 runs its own default cells in its own binary (rule 8). It does **not** pair against
  CAMP-FINAL-2's cells.
- S3's campaign runs on the cloud, never on the local machine while S1's campaign or suite is
  running. The local machine has 12 cores and 7 GB.

**D. Tier 2 is touched by both S2 and S3.** S3 changes how a 2.4 GHz range is *measured*
(the ranging submodule, `RangingParams`). S2's WP-A4 changes *when* 2.4 GHz is ranged (the
scenario loop and the policy). These are different files and do not conflict. S3's result
(whether 2.4 GHz still carries the accuracy at a field error level) is an input to WP-A4's
registration. **S2:** read S3's result before registering WP-A4.

**E. Tier 1.** S3's validation checks the 868 MHz *path-loss model* against measured data. S2's
WP-A6 sweeps the tier-1 *PHY* (SF/BW). If S3 finds the path-loss model wrong, WP-A6's numbers
move, so S3's item 3 should come first.

**F. The link-budget model.** S3 owns `src/catbl-propagation-study/` for the foliage-scaling fix.
This candidate cause for the open "UWB ~3.7 dB residual" (`CLAUDE.md` §5) is S3's to record. The
line in `CLAUDE.md` is S1's, and S1 changes it only on the owner's say-so.

---

## 3. Files that more than one stream edits

| file | owner | rule for the others |
|---|---|---|
| `scratch/ca_tbl_causal_sim.cc` — `kSpecProfile` | S1 only | Nobody else adds to the preset. Propose it in your handoff |
| `scratch/ca_tbl_causal_sim.cc` — `SpecOptions` fields and the `cmd.AddValue` block | shared | Add your fields and flags as **one contiguous block** headed `// --- S2 accuracy flags ---` or `// --- S3 evidence-fix flags ---`. Never reorder or edit another stream's entries. S3's three flags sit just before `tdoaInnovationGate` |
| `scratch/ca_tbl_causal_sim.cc` — IMU call sites (`SetImuControl`, `PredictWithControl`) | S2 | — |
| `src/lora-physics-engine/model/submodules/lora-ranging-submodule.cc`, `RangingParams` in `lora-physics-parameters.h` | S3 | — |
| `ca-tbl-imu-model.{h,cc}`, `ca-tbl-policy.{h,cc}` (filter, Q, gating) | S2 | S1 touches the policy only for spec rows. Say so in the commit, and rebase S2 after |
| Physics-engine **comments** citing papers (`lora-physics-parameters.{h,cc}`, `submodules/lora-{foliage,delay-spread,nlos,hardware}-submodule.*`, the three engines' thermal comment) | S3 | Comment-only commit; no code moves |
| `scripts/catbl_gates.sh` | shared, **append only** | Use only your reserved gate numbers (§4). Add your `finish` lines at the end. On a merge conflict keep both sides |
| `docs/TECH_DEBT.md` | shared | Use only your reserved N numbers (§4). Add whole new sections; do not edit another stream's section |
| `docs/SPEC_CONFORMANCE.md`, `docs/DESIGN_DECISIONS.md` | S1 | Others write the change they need into their handoff |
| `docs/SCENARIO_FLAGS.md` | generated | **Never hand-merge.** After any rebase that adds flags, regenerate with `tools/regen_flags.py` from the merged binary's `--PrintHelp`. Whoever merges last regenerates |
| `docs/CONTEXT_PACK.md`, `Architecture.md` §13 (references, PROV rows) | S3 | S2 adds new references in its own subsection only |
| `CLAUDE.md` | S1, and only on the owner's say-so | — |
| `HANDOFF_*.md` | each stream its own file | Do not edit another stream's handoff |
| `report/paper_tables/`, `scripts/paper_tables.py` | S1 | — |

### Reserved numbers
| | gates | TECH_DEBT sections | report files | results directories |
|---|---|---|---|---|
| S1 | G91–G99 | N.27–N.29 | `PREREG_CAMP_FINAL2.md`, `CAMP_FINAL2_RESULTS.md` | `results/camp_final2_*` |
| S2 | G100–G119 | N.30–N.39 | `PREREG_ACC_*.md`, `ACC_*_RESULTS.md` | `results/acc_*` |
| S3 | G120–G124 | N.40–N.44 | `PREREG_TOF_FIELD.md`, `TOF_FIELD_RESULTS.md`, `UWB_REACH.md`, `TIER1_VALIDATION.md` | `results/tof_field_*`, `results/tier1val_*` |

S3's planned use:
- **G120:** the tier-2 field-error flags and `--traceCovariance`, with a negative fixture.
- **G121:** the tier-1 validation script, if it becomes a gate.
- **N.40:** tier 2's 3.0 m floor against Müller 2021.
- **N.41:** UWB reach and the link-budget model's missing foliage scaling.
- **N.42:** the citations that could not be confirmed.

---

## 4. Bases, rebases and merge order

1. **S3 rebases first.** Its base `b98c4913` was amended away (now `d7ada989`, same tree). Rebase
   with
   `git rebase --onto 1596cb41 b98c4913 review/2026-10-08-evidence-fixes`.
   The one conflict to expect is in `scratch/ca_tbl_causal_sim.cc`: `kSpecProfile` changed on S1's
   side and the flag block on S3's. Keep both. Then rebuild, re-run NC1–NC9 against a rebuilt
   reference, and push with `--force-with-lease`. The branch is S3's alone.
2. **S2 rebases onto S1's head** before any WP-0 code. Its branch predates row 17 and the preset
   change (§2 A).
3. **Merge order into the main line:**
   - S1's CAMP-FINAL-2 registration;
   - S3's flag commit (squashed from `13c5a23`, gate G120 included) and its citation commit;
   - S2's WP-0;
   - then each stream's campaigns, one at a time.
   After every merge, run the full suite and NC1–NC9. Each campaign is registered on the
   then-current main line before its first cell.
4. **Null-control references are per machine** (the build uses `-march=native`). A cloud session
   rebuilds its own with `tools/nullcontrol/rebuild_reference.sh`. A pass on one machine does not
   carry to another.

---

## 5. Rules every stream keeps (from `CLAUDE.md`, restated because three agents share a tree)

- **Never run the simulator binary without `--outdir=<temp dir>`.** The default writes into the
  tracked `results/causal`. Check `git status --short` for `results/` lines before any suite.
- Never `pkill` with an unanchored pattern. It kills the tool's own shell.
- Never use bare `git stash`; the stash stack is shared across worktrees. Use a WIP commit.
- Commit as the owner (Kushaal KN). **No attribution trailer or "generated with" line**, anywhere.
- One change per commit, null-controlled, with a gate and its negative fixture. Pre-register
  before the first measured cell.
- Do not regenerate a tracked results directory in place. Do not edit `run_id.txt`.
- When you finish a step, update your own handoff file and push your branch, so the other
  streams see it.

---

## 6. S3 step list in detail (for whoever runs the evidence-fix branch)

1. **Rebase** as in §4.1. Build optimized. Rebuild the null-control reference on that machine.
   Run NC1–NC9: every output must be IDENTICAL with the three flags off.
2. **G120**, `scripts/tof_field_gate.{sh,py}` and `_selftest.sh`:
   - **Off:** byte-identical to a run without the flags.
   - **`--tofFieldSigmaM=83.6`:** every tier-2 `rangingSigmaM` is at least 83.6. Read it from
     the trace's `meanSigmaM` on 2.4 GHz cycles.
   - **`--tofFieldBiasM=46.4`:** the mean tier-2 range error moves by +46.4 m ± noise. This
     needs the range log or a direct engine test. If neither exists, write a unit check in a
     scratch test like `ca_tbl_integrity_test.cc`.
   - **`--traceCovariance`:** three extra columns; pxx + pyy equals `traceSigma` to print
     precision on every row.
   - One negative fixture per check. Then squash `13c5a23` into one atomic commit with the gate.
3. **Citation commit**, comments and docs only, with no code change. Null control: the binary's
   outputs are byte-identical. Write the comment changes:
   - Petäjäjärvi: 2.2 and 2.75 are OURS. Rademacher 2021 quotes the Oulu fit as n = 2.65.
   - Rademacher: 1.58 confirmed. It is a single-slope fit, used here as the far slope.
   - Boano, Cattani & Römer 2018 replaces "Oliveira 2017".
   - Cheffena & Ekman 2009 replaces "Meng 2009" for the wind fading; the values stay OURS.
   - Lim 2018, Hofer 2015 and Robyns 2017 values become OURS.
   - Update `CONTEXT_PACK.md` PROV-01 and PROV-02, and Architecture §13. Add TECH_DEBT N.42.
4. **`report/PREREG_TOF_FIELD.md`**, committed before any cell:
   - **Cells:** A (5 km) and B (endgame) on the 102-setting preset, seeds 1–20.
   - **Levels:** {default; 83.6 / 46.4; 83.6 / 0; 45 / 39}.
   - **Arms:** `ca_tbl_full`, `static_2p4`, `lora_fusion`, `poucet_fusion`.
   - **Predictions:** whether `static_2p4`'s CEP95 lead, and the preset's endgame accuracy,
     survive at field error.
   - Use the runner pattern of `scripts/fixes_n26_run.sh`. Run on the cloud and write
     `report/TOF_FIELD_RESULTS.md`. Add TECH_DEBT N.40.
5. **Tier-1 validation**, `analysis/tier1_validation/`:
   - **Data:** Aernouts et al. 2018 LoRaWAN Antwerp (doi 10.3390/data3020013, Zenodo), or
     Rademacher 2021's released data.
   - **Comparison:** RSSI against distance, binned, overlaid on the engine's probe sweep for the
     matching environment class.
   - **Output:** `report/TIER1_VALIDATION.md`. It states which profile is compared, and that
     Antwerp is urban while the SAR scenario is forest.
6. **UWB reach write-up**, `report/UWB_REACH.md`:
   - The DW3000 datasheet v1.1 numbers, the ablation table from `scripts/uwb_reach_ablation.py`,
     and the free-space ranges.
   - Fix `linkbudget.py` to carry the engine's per-tier foliage scaling (×2.17 at 2.4 GHz, ×5.28
     at UWB) as its own commit. Re-run `validate.py` and record whether the UWB residual closes.
   - Add TECH_DEBT N.41.
7. After each step: update `HANDOFF_2026_10_08.md`, then push.
