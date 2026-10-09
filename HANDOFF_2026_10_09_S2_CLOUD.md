# HANDOFF 2026-10-09 — the S2 cloud branch

The S2 accuracy stream's cloud branch: its state, how it overlaps the two other S2 branches
(`work/2026-10-09-acc-part-a`, `work/2026-10-09-acc-part-b`), and what it needs from S1. Read
`WORKSTREAMS_2026_10_09.md` (on those branches) first. Check `git log` before trusting this file.

## 1. What is on this branch, in order

| commit | what |
| --- | --- |
| `100cc650` | merge of the main line (102-setting preset) |
| `24387cd2` | N.34 `--dualBaseRelayTier` (independently the same as acc-part-b's F1 / N.35 / G110) |
| `5e6fd01c` | N.33a `--relayCovScale`: measured, kept at 1 |
| `b00e1f40` | N.33b `--hybridNoLone868`: measured, not adopted |
| `a1da0201` | N.38 `--caseDInfoRatio`: refuse a Case D range far less informative than the prior |
| `1c1886c7` | gate G112 (then numbered G91) |
| `94a200ca` → `05c0ccea` | ACC-B3: pre-registration, then results (`report/ACC_B3_RESULTS.md`) |
| `e87378cb` | renumbering into S2's reserved ranges (G91 → G112, N.35-37 → N.33/N.34), report files renamed ACC_B3 |
| `53766acb` | N.39 `--unknownStartSigmaM`, gate G113 |
| `ead864fb` → `12960c3c` | ACC-B3-US: pre-registration, then results |
| `88b0a101` | **Plan A, A2: `--imuErrorStates`**, the error-state IMU filter, gate G101 |
| `9da4bd71` → `7dc74b13` | ACC-A2: pre-registration, then results |
| `a6ea951e` | ACC-B3 citable re-run (since replaced by `results/acc_b3_cit_2026-10-09`, see below) |
| `f1562b59` | **preset: `--imuErrorStates` adopted (103 settings)**, at the owner's request |

## 2. Results in one line each

- **ACC-B3** (`--dualBaseRelayTier --caseDInfoRatio=3` vs the 102-setting preset):
  - endgame availability +0.020 WIN;
  - 5 km within 25 m +0.035 tie;
  - 5 km energy +1.24 J LOSS;
  - endgame CEP did not move (P2 failed).

  This one is citable.
- **ACC-B3-US** (unknown start, s = 100):
  - every arm loses about a third of its 5 km within-25 m share; `static_2p4` loses 0.222;
  - `ca_tbl_full` ties `static_2p4` at 5 km (U2 failed);
  - with the B3 flags, `ca_tbl_full` has no accuracy LOSS against any arm.
- **ACC-A2** (`--imuErrorStates`):
  - endgame CEP50 4.80 → 1.39 m and CEP95 (per seed) 12.0 → 5.1 m;
  - 5 km CEP50 16.2 → 6.5 m;
  - no accuracy LOSS against any arm.

  **With the B3 flags added, the endgame CEP worsens** (E7 failed): the B3 flags were tuned against the old,
  over-confident filter.

## 3. Requests to S1 (`kSpecProfile` and `RULES.md` are S1's)

1. **The preset changed on this branch** (`f1562b59`), on the owner's instruction to this stream. It is
   one added setting, `imuErrorStates=true`.
   - Equivalence: 167/167 files byte-identical to the old preset plus the explicit flag, apart from
     `spec_profile.txt`.
   - Every preset-based result before it describes the setting off.
   - CAMP-FINAL-2 should be registered on whichever preset S1 decides is current.
2. **`RULES.md` gate counts are stale** (58 there; 91 `finish` lines on this branch).
3. **`scripts/publication_ready.sh` is NOT READY on this branch's base** (`100cc650`) for one reason that
   predates this stream: the authoritative report does not cite `baseline_v11`.

## 4. Numbering, and one number this stream could not get

- **Gates used here:** G101 (A2), G112 (B3 flags) and G113 (unknown start).
  - acc-part-a uses G100; acc-part-b uses G110 and G111.
- **TECH_DEBT:** across the three S2 branches, S2's reserved range N.30–N.39 is used up:
  - acc-part-a: N.30–N.32;
  - acc-part-b: N.35–N.37;
  - here: N.33, N.34, N.38, N.39.

  **The A2 outcome has no TECH_DEBT number**; it is recorded in `report/ACC_A2_RESULTS.md`. Request: a number
  for it (Plan A's own text reserved N.31 for it, which acc-part-a used for the PCS stop rule).
- **Report names:** this branch's are `PREREG_ACC_B3_*`, `ACC_B3_*`, `PREREG_ACC_A2_*` and `ACC_A2_*`. acc-part-b
  already has `ACC_B_RESULTS.md` and `PREREG_ACC_B_HYBRID.md`; these were renamed so they don't collide.

## 5. Merge notes (not done; the owner asked for no merge in this session)

- **`--dualBaseRelayTier` exists on both this branch and acc-part-b**, under the same name, written
  independently. Keep one implementation. acc-part-b's is the registered ACC-B one; this branch's G112 RELAY and
  SCALE checks then need pointing at it.
- **The two flag blocks.** acc-part-a's `--imuQuScale` and this branch's `--imuErrorStates` both act on the IMU
  covariance. They should not be combined without a test: the error-state mode ignores the increment covariance
  that `--imuQuScale` scales.
- **`ca-tbl-policy.{h,cc}`.** This branch adds `caseDInfoRatio` and the error-state mode (`CaTblEkf` `Ie*`
  members, branches at the top of the mutating methods). Append-only against acc-part-a's changes as far as is
  known; check on merge.
- **After merging:**
  - regenerate `docs/SCENARIO_FLAGS.md` with `tools/regen_flags.py`;
  - re-run NC1–NC9 and the suite.

## 6. Not done

- Plan A's A3: the IMU-quality stress flags (a mis-declared prior, a drifting bias).
- Re-tuning the B3 flags under the error-state filter.
- The unknown start under the error-state filter.
- Done: all three campaigns re-run from clean worktrees outside the repository, into
  `results/acc_b3_cit_2026-10-09` (`e87378cb`), `results/acc_b3_us_cit_2026-10-09` (`53766acb`) and
  `results/acc_a2_cit_2026-10-09` (`88b0a101`). Every summary and `metrics.csv` is byte-identical to the runs the reports
  were computed from (1 080 files). The earlier directories were removed from tracking: two read `source unknown`
  (380 G13 entries), and all three recorded the session's scratch paths.

## 7. Suite after the preset change

Run on `f1562b59`. The session's disk filled at G89, so G89, G90, G112, G113, G101 and G24 were re-run once space
was freed. Every red is accounted for:

| gate(s) | why red |
| --- | --- |
| G4, G5, G6, G11 | expected reds, `RULES.md` §3 |
| G24 | FAIL BY FINDING. Now **five** zero populations: subjects' TDoA solves went 2 → 0 over seeds 7–9 with the error-state filter (`d459c25a`) |
| G10, G63, G87, G89 | already red on the base (`1596cb41` local verification; S1's preset change `e308c14f`) |
| G13, G72 | artifacts of this container's **shallow clone**: they resolve SHAs (`c019c0a`, `d82b865`, ...) that are not in the 320 fetched commits |

**G13 also counted 380 sidecars reading `source unknown`.** Those were this branch's exploratory ACC-B3-US and ACC-A2
runs; they are replaced by citable re-runs (§6).

G101, G112, G113 and G90 pass.
