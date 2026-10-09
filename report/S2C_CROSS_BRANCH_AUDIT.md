# S2-C cross-branch audit — branches, pull requests, integration decisions (2026-10-09)

Written before any S2-C engine change. Sources:
- `git` on `https://github.com/YKushal-hub/ca-tbl-localization`, fetched 2026-10-09;
- GitHub's pull-request list;
- the branches' own files.

Completion is judged from diffs and code, not commit messages.

## 1. State

- **`main` = `4ca0dff9`**, "Merge pull request #7 from work/2026-09-27-spec-b1". This is S1's
  102-setting preset (`e308c14f`), row 17 (`d7ada989`) and its suite evidence (`1596cb41`). GitHub
  lists PR #7 as closed with `merged: false`, but its merge commit is on `main`.
- **Every current stream branches from `1596cb41`.** It is `main`'s second parent, so `main` is
  their common integration point.
- **Target branch of this work:** `main`.
- **Working branch:** `work/2026-10-09-unknown-start-nlos`, created from `origin/main`.
- **Branch naming.** The session's tooling designated a branch carrying the tool's name (= old `main`,
  `a0829213`). It is not used, because RULES.md §2 forbids an assistant name in a branch name.
- **Open pull requests:** none. PRs #1-#7 are closed.

## 2. Inventory (relative to `main`; "ahead" counts commits not on `main`)

| branch | head | ahead | what it is | state | in this PR |
|---|---|---|---|---|---|
| `review/2026-10-08-evidence-fixes` (S3) | `5a6f4c26` | 17 | `--tofFieldSigmaM`/`--tofFieldBiasM`/`--traceCovariance` (G120), citation fixes, TOF-FIELD (registered), UWB reach, tier-1 validation, N.40-N.42 | complete; handoff says "complete except merging" | **merged** (`7c3921c8`, clean) |
| `work/2026-10-09-acc-part-a` (Part A) | `d76b4d55` | 23 (incl. S3 to `89118bae`) | `--imuQuScale` (G100), ACC-A (registered), ACC-A-OPPOINT (registered), decision and static 2.4 GHz diagnoses, N.30-N.32 | complete for A1/A3/A4/A6/A8; A2, A5, k tuning, CAL-03/integrity refit not done | **merged** (`ea5f9025`; one append-only conflict in TECH_DEBT, both sides kept, N order) |
| `work/2026-10-09-acc-part-b` (Part B, cloud) | `42294896` | 20 | F1 `--dualBaseRelayTier` (G110), F2 `--subjectPreciseOnly` (G111), ACC-B and ACC-B2 (registered), N.35-N.37 | complete for F1/F2; B1, H2-TDoA, F1 with `fixed_threshold`, B5 not done | **not merged**: conflicts with the next row |
| `work/2026-10-09-acc-reruns` (Part B, another session) | `05c0ccea` | 13 | its own N.35 `--dualBaseRelayTier`, N.36 `--relayCovScale`, N.37 `--hybridNoLone868`, N.38 `--caseDInfoRatio`, G91, its own `PREREG_ACC_B_HYBRID.md` and `ACC_B_RESULTS.md` (k = 3), plan and context docs, `analysis/accuracy_replay` | complete as an evaluation; not reconciled with the row above | **not merged**: owner decision |
| `work/2026-10-09-local-verification` | `2552bd9d` | 15 (incl. S3/A to `eadcb0b3`) | `VERIFICATION_2026_10_09_LOCAL.md`: local null control, G100/G120, full suite (81 PASS), k sweep on seeds 21-30 | complete, docs only | not merged; listed as evidence |
| `draft/camp-final2` | `c432134e` | 1 (on `e308c14f`) | DRAFT, NOT REGISTERED CAMP-FINAL-2 runner and analysis | draft | not merged |
| `ccr-4ce80826-a4ulj4` | `19cf3d6e` | 0 | PR #6 head, already in `main` | merged | — |
| `wip/b2-coverage-predicate` | `141f7a82` | 1 | 2026-08-07 coverage predicate, "ACCEPTANCE UNVERIFIED"; predates the topology correction | superseded | no |
| `IMU_SHS_V1.1` | `aa18f48b` | — | unrelated history (no merge base with `main`) | superseded by the SHS in `main` | no |
| `archive/*`, `work/2026-09-0*`, `spec/*`, `demo/*` | — | 0 | merged or archived | — | — |

## 3. Conflicts and dependencies

**The two Part B branches duplicate each other.** Both define a flag named `--dualBaseRelayTier`,
both write `report/PREREG_ACC_B_HYBRID.md` and `report/ACC_B_RESULTS.md`, and both use TECH_DEBT
N.35-N.37, with different content in each:
- `work/2026-10-09-acc-reruns` uses gate G91 and evaluates at k = 3;
- `work/2026-10-09-acc-part-b` uses G110/G111 and evaluates at k = 1.

Merging both would silently give one flag name two code paths and two registered result
documents one filename. This PR merges neither.

**Further dependencies:**
- **S3 → Part A.** Part A carries S3's commits up to `89118bae`. Merging S3 first and A second
  leaves only the TECH_DEBT append conflict.
- **This PR's work** (`--unknownStart`, `--traceSubjectConsistency`, `--measVarScale`) depends on
  neither Part B branch.
- **G10 (citation audit)** resolves once S3 is in. It is in this branch.

## 4. Results reused here, and why they are comparable

- **ACC-A** (`results/acc_a_2026-10-09`, traces on the cloud disk only): known-start controls for
  `ca_tbl_full` and `static_2p4`.
- **ACC-A-OPPOINT:** the known-start control for `fixed_threshold`@12.

Both are reused only after a registered byte-identity stage on seeds 1-3 (`PREREG_S2C` §2).
The ACC-A diagnoses' numbers are cited, not recomputed.

## 5. Recommended integration order

1. This PR, which already contains S3 and Part A.
2. **One** Part B branch, after the owner chooses. Its flag name, report names and N-numbers must be
   renumbered against the other if both are kept.
3. `work/2026-10-09-local-verification`, docs only.

## 6. Decisions the owner must make

1. **Which Part B branch to keep**, or how to renumber if both are kept.
2. **Adoption of `--imuQuScale=10` and of F1 into the preset.** This is S1's call, and neither is
   adopted here. Evidence:
   - `--imuQuScale=10`: endgame CEP95 −4.5 m filter-wide, ANEES 46 → 5.7, but k is untuned and
     the 5 km tail worsens for three arms.
   - F1: +0.030 endgame availability, no endgame accuracy gain, +5 % energy at 5 km.
3. **G63, G87, G89.** They have been red since `e308c14f`, not because of S2/S3. Their control
   cells run on `--specProfile`, which now carries the N.26 fixes; for example, G89's
   every-flag-off runs print an N.26 line. The local verification established the same check
   lines on `1596cb41`. The fix is explicit flag-off controls in those gates, and that is S1's.
4. **Branch naming:** RULES.md §2 against the tooling's designated branch name.
