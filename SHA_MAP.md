# SHA map — attribution rewrite of 2026-09-01

The repository owner directed that an assistant attribution trailer be removed from
commit messages entirely, not merely avoided going forward. A history rewrite was
authorised **once, for that purpose only**, overriding the standing never-rewrite
rule — that rule was adopted as hygiene around a settled vendored import, and an
owner instruction about attribution in his own private repository outranks it.

**Metadata only.** `git diff <old-tip> <new-tip>` was empty: no file content changed,
commit count unchanged at 83, order and dates preserved, linear no-merge shape intact.
Author and committer identity were already correct on all 83 commits — only 27 message
bodies carried a trailer.

30 commits were rewritten; the 53 before them are untouched and keep their SHAs.

| old | new | subject |
|---|---|---|
| `aeeb1cfd7c12` | `e02e45ad9984` | fix: EXCLUDED.md lists handoff v5, and the HIDDEN assertion coun |
| `f3aa980eb7c6` | `7e91d73f726d` | docs: cross-document consistency pass, and withdraw a wrong defe |
| `5d590e4fc1bd` | `f87aaf9c7836` | docs: mark the data pack as a historical record of superseded ca |
| `4ee3c0c3b8d0` | `da328d19d0ba` | docs: restructure the report around what stands |
| `5e616d735a86` | `068e5d5f05ed` | evidence: recovery-policy ablation on clean provenance -- the la |
| `22ee9f90297a` | `ccaedb0746c5` | pre-register: abl_clean, the recovery-policy ablation on clean p |
| `6906ad50880e` | `e44e8e227308` | chore: untrack 12 traces and record the deliberately-ignored fil |
| `27ed203cb141` | `9f2aff65b938` | evidence: Claim 9 re-measured on citeable provenance -- falsific |
| `57dcadf8ea7a` | `cd594c90c362` | pre-register: c9_clean, Claim 9 re-measured on citeable provenan |
| `2f0f0ac55852` | `3e1a270a0dd9` | fix: degeneracy screen is per-mechanism; propagate two withdrawn |
| `2ffa0cc90196` | `55aee233d77e` | docs: withdraw a wrong finding -- the trigger pair WAS reproduci |
| `1601cfb935f9` | `1ac680259c03` | fix: G6 compared a real file against a path that never existed |
| `a0ccb4fd1c51` | `683416334b12` | evidence: baseline_v7 -- regenerate under the corrected engine,  |
| `72bcb7dcb4ea` | `c651743cb48a` | docs: record the provenance gap in the cited evidence base |
| `4ba6f334d655` | `3ce0618955b0` | docs: track the two report documents the citation audit reads |
| `2ca905c50a65` | `cb02c32f516e` | tools: run every gate, and close the three cited-script gaps |
| `4a717a1b56d1` | `b9459c23a418` | fix: track Simulation.md, untrack the third report, and gate bot |
| `a3690c21b374` | `3edfec8fb486` | fix: make the must-stay-untracked rule mechanical, and record wh |
| `aa8328bbdfdc` | `03c15dd607a8` | chore: untrack CA_TBL_HANDOFF_v4.md, which must stay untracked |
| `35466c019062` | `2ab38c2e71fe` | docs: withdraw W1 and N3, correct r80, and document the delibera |
| `302d32f92694` | `1057f6f811dc` | tools: commit the analysis scripts the gate suite requires |
| `e21666dc4b51` | `d34eeb7c474a` | fix: gate suite must not overwrite the evidence it verifies |
| `13a62bb788f9` | `0a489df392d7` | docs: withdraw the reliability and Claim 3 entries after the v7  |
| `7c0c5e513753` | `63bba4fd2c5e` | chore: land the gate, manifest and report state from phases 9-10 |
| `6de0049507ea` | `bd9d76bb2127` | fix: NaN sentinel for undefined mean SNR |
| `846dd80cab81` | `c0313f3f214f` | diag: emit unconditional SNR alongside the conditioned accumulat |
| `7ac9cd58b057` | `dd07cf7efa78` | feat: Phase 10 evidence predicate and suspectRecovery ablation |
| `60605e4fc786` | `976bbefaa369` | fix: record the blast radius of a4e23ad (D1 SF index) |
| `a4e23adc2930` | `0d5e74794d37` | fix: correct SF index base in 2.4GHz and UWB sensitivity tables |
| `b9e650f6b1aa` | `26721ef444d7` | study: add the closed-form propagation module and its defect rec |

## Where old SHAs still appear

**`run_id.txt` sidecars are evidence and were NOT edited.** Rewriting them would be
falsification. 1283 sidecars cite six SHAs that changed:

| old SHA | sidecars | new SHA |
|---|---|---|
| `13a62bb` | 938 | `0a489df392d7` |
| `7c0c5e5` | 181 | `63bba4fd2c5e` |
| `22ee9f9` | 120 | `ccaedb0746c5` |
| `57dcadf` | 40 | `cd594c90c362` |
| `72bcb7d` | 3 | `c651743cb48a` |
| `4ba6f33` | 1 | `3ce0618955b0` |

Those sidecars resolve through this table. The verifier's four-outcome resolution
already has a *does not resolve (history rewritten)* branch; these land in it, and this
file is what turns that outcome from a dead end into a lookup.

---

# SHA map — attribution rewrite of 2026-09-05

Second authorised rewrite, same purpose and same authority as the 2026-09-01 one
above: the owner directed that the assistant attribution trailer be removed from
commit messages entirely rather than merely avoided going forward.

**Why it was needed.** Gate G12, added by the 2026-09-01 rewrite precisely to stop
attribution returning, went red on 21 commits made during the 2026-09-05 session.
The identity fields were already clean on all of them; only the `Co-Authored-By`
trailer in the message body tripped it. G12 states its own remedy is a history
rewrite authorised only by the owner, and that authorisation was given.

**Metadata only.** `git diff <old-tip> <new-tip>` is **empty**: no file content
changed. Commit count unchanged at **131**. Order, dates and subjects preserved,
merge topology intact. Author and committer identity were correct throughout.

**25 commits were rewritten** — the 21 carrying a trailer plus 4 descendants whose
SHAs move because their ancestors did. The 106 before them are untouched.

**Sidecars were not edited.** 793 `run_id.txt` files written during that session
cite the SHAs below in their `source` field, chiefly `6d44895` (512 cells),
`9723dfc` (169) and `1e8e527` (60). They keep citing what they recorded; this table
is what resolves them. Editing evidence to match a rewrite would be falsification.

| old | new | subject |
|---|---|---|
| `1b938fedead2` | `1eb09f0aa563` | lorawan links the physics engine its phy already calls |
| `170cd209773e` | `09b13c74fe77` | docs: agent working rules, master plan, and work packages |
| `134d3e6972c4` | `83f47de9f741` | pre-register C9-ESCALATE before the first cell runs |
| `3c3c5f4aaaf7` | `ef78f8307736` | A2 stage 1 runner, with the fault label mapping corrected |
| `6d4489553dae` | `e282f4d310dd` | pre-register A0, the joint availability/energy criterion |
| `5f1ff7870624` | `590d6ca736e7` | the confidence score gets a form that can veto |
| `f578f16bce8a` | `2817213da066` | the hysteresis band becomes a swept parameter instead of a const |
| `5e8d2b6a2f08` | `894caf561b4a` | the section 4.1 reliability result has no sidecar in the tree |
| `c1868ef6f897` | `e346f99ce96e` | CAL-05 exists: the integrity check is precise and insensitive |
| `4fc556c679b3` | `d27ff99b3b11` | pre-register the PCS form comparison before running it |
| `aa76e818a679` | `7d8ad4f32e92` | the PCS form campaign runner, with the registered abort check wi |
| `d07acdf8a7d0` | `e170377eada1` | A0 returns FAIL in 64 of 64, and the reason is degenerate |
| `ac41f6c39e56` | `dcc355a854e3` | the escalation handshake becomes a frame, not just a charge |
| `4c0b1753d763` | `60932657a6a2` | C1 verified: the handshake, once it contends, costs 41 percent m |
| `8fddef99f921` | `64da52cbfc81` | untrack the partial pcs_forms output that git add -A swept into  |
| `9723dfc70f0c` | `64277c489937` | the PCS abort check was asking the wrong question |
| `1e8e527709b5` | `5f6fa316848b` | CAL-05 as a curve: the integrity check is a working detector |
| `ea900d244ef5` | `0279f6b8d67a` | terrain stops applying an 868 MHz wavelength to every tier |
| `df67f2d305f0` | `b55db5774d7d` | D-1 repaired: the system works better, the confidence claim does |
| `e5741a50d4b8` | `caac6214d760` | evidence: the pcs_forms and c9_stage2 campaigns, complete this t |
| `cf1709de49b8` | `77b8e9b2d020` | evidence: c9_escalate, p10_grid and p10_reliability, with their  |
| `89b9aaefbb60` | `39e931e12404` | Merge branch 'wip/lane-b' into main |
| `36d6e90a0ae1` | `d85c10a4050f` | Merge branch 'wip/c1-handshake' into main |
| `b1138820a429` | `1098ee3063aa` | session record: five work items, five campaigns, six errors |
| `a67389618d23` | `7dc9f02a5162` | G12 is red because of this session commit trailers, and that is  |


---

# SHA map — identity rewrite of 2026-09-06

**Why.** Every commit in this repository was authored `Kushal Yalamanchi
<kushalyalamanchi@gmail.com>`. That is not the repository owner. The WSL clone carried a
**repo-local** `user.name`/`user.email` in `.git/config` that overrode the correct global
identity, so work done in that clone was attributed to someone else and earned the owner
no contribution credit on GitHub. The owner identified this and authorised the rewrite.

**Scope — unpushed commits only.** Every commit reachable from a local ref but **not**
from `origin/main`: 57 commits across 8 branches, 45 of them on `main`. Nothing already
published was touched, so **no force-push is required** and the 106 commits already on
GitHub keep their SHAs and their old authorship. Making those the owner's too is a
separate, larger operation against a protected branch and was deliberately not done here.

**Blast radius on evidence: none.** No `run_id.txt` sidecar cites any rewritten commit —
checked before rewriting, all 28 sidecars against all 57 rewritten SHAs, zero hits. G13
(sidecar source SHAs resolve) passed before and after.

**Metadata only.** Author and committer identity changed; nothing else. The acceptance
test was per-branch tree equality: for all 8 branches `<old-tip>^{tree}` equals
`<new-tip>^{tree}`, so no file content, no commit count, no ordering and no date changed.
Every old/new pair was additionally verified to share its subject line and author date.

**Citations updated in the same commit** — `report/A6_HYSTERESIS_RESULTS.md`,
`report/C4_PROVIDER_BLOCKER.md`, `report/NOT_DONE_2026_09_05.md`,
`report/PREREG_A6_HYSTERESIS.md`, `report/PREREG_PROVIDER.md`. This file is **not**
rewritten: the map above records the 2026-09-01 rewrite and its SHAs are historical.

Pre-rewrite tips are retained locally under `refs/original/refs/heads/*`.

| old | new | subject |
|---|---|---|
| `7d8ad4f` | `003241c` | the PCS form campaign runner, with the registered abort check wired in |
| `64277c4` | `048b7bb` | the PCS abort check was asking the wrong question |
| `67ef735` | `0c383a4` | a second target on the channel, as a protocol peer |
| `3420073` | `1164e5d` | revert the generated-with widening: it broke the gate on its own documentation |
| `ef78f83` | `161ef57` | A2 stage 1 runner, with the fault label mapping corrected |
| `10a3f3b` | `16b70d9` | Merge branch 'wip/c4-arbitration' into wip/c34-on-provider |
| `b55db57` | `1c0ddb2` | D-1 repaired: the system works better, the confidence claim does not survive |
| `77b8e9b` | `2444a13` | evidence: c9_escalate, p10_grid and p10_reliability, with their runners |
| `7401f82` | `250bf81` | write the provider blocker report the register was already citing |
| `39e931e` | `26c541b` | Merge branch 'wip/lane-b' into main |
| `f5e51ea` | `2c4ff7d` | no assistant attribution anywhere in this project, as a standing rule |
| `c05d48d` | `36199b5` | A6: hysteresis is a live axis, and the dwell is an energy/accuracy dial |
| `70e9329` | `3c95a86` | the PHY provider carries a position per field node, so a peer tag can be heard |
| `09b13c7` | `3d1cc3e` | docs: agent working rules, master plan, and work packages |
| `57b3151` | `4aa38aa` | pre-register A6, the hysteresis sweep, before running it |
| `27b2cef` | `4d1d349` | C4: fix the peer MAC binding, and narrow why arbitration still sees one requester |
| `590d6ca` | `5751945` | the confidence score gets a form that can veto |
| `ce7e13f` | `59767b8` | a tier-1 handoff stops costing a ranging slot |
| `f4d42ce` | `5d88faf` | open items: the PHY provider re-scopes C2, C4 and C5 |
| `dcc355a` | `5dc96ed` | the escalation handshake becomes a frame, not just a charge |
| `0279f6b` | `614912b` | terrain stops applying an 868 MHz wavelength to every tier |
| `b8e1b3f` | `6662df2` | A7: a gate that reads the sensitivity tables, with six negative fixtures |
| `d85c10a` | `689aa49` | Merge branch 'wip/c1-handshake' into main |
| `9b26515` | `789349c` | the register of what this session did not do, and why each |
| `7dc9f02` | `7899015` | G12 is red because of this session commit trailers, and that is a policy conflict |
| `6093265` | `7bdc576` | C1 verified: the handshake, once it contends, costs 41 percent more collisions |
| `5f6fa31` | `7dc8ba0` | CAL-05 as a curve: the integrity check is a working detector |
| `deb52da` | `801b1a7` | record the 2026-09-05 attribution rewrite, and update the plan it changed |
| `321d249` | `8364e96` | C4: the blocker is the PHY provider, which models exactly one field node |
| `5ad4284` | `8461d5d` | evidence: the A6 hysteresis sweep and the A1 dwell sweep, both complete |
| `2af7444` | `88d0e49` | G12: stop matching co-authored-by in message bodies, and close a hole beside it |
| `1098ee3` | `9d3136f` | session record: five work items, five campaigns, six errors |
| `e170377` | `a1f6e76` | A0 returns FAIL in 64 of 64, and the reason is degenerate |
| `e553f39` | `a59c29d` | withdraw sections 4.1 and 4.2: the directory they cite is not in this repository |
| `535eb55` | `a7308bc` | register: C4 closes, C3 re-evidenced, and the trace divergence is now a number |
| `5f0b128` | `ab38a80` | the handshake contention effect is 33.7 percent, not the 41 percent one seed showed |
| `83f47de` | `b368948` | pre-register C9-ESCALATE before the first cell runs |
| `141f7a8` | `b82f43e` | wip: coverage predicate for tier-3 escalation -- ACCEPTANCE UNVERIFIED |
| `64da52c` | `bb0e43f` | untrack the partial pcs_forms output that git add -A swept into d07acdf |
| `ec922ac` | `c855ae4` | C4: the UWB arbiter, ported and running, with its deny path unexercised |
| `9193202` | `c862842` | the relayed-ranging error model, written before any code, as C5 requires |
| `caac621` | `d3bb518` | evidence: the pcs_forms and c9_stage2 campaigns, complete this time |
| `34b3b2f` | `d6240d7` | A1: dwell drives coverage, and coverage does not rescue the conjunction |
| `a496532` | `d64aab5` | fold the duplicate C3/C4 report into one, keeping the provenance-gap note |
| `83bb388` | `d789e09` | PARKED: full per-tag refactor, result-exact but not bit-exact |
| `894caf5` | `e014992` | the section 4.1 reliability result has no sidecar in the tree |
| `4c6ff54` | `e431eff` | register: A7 done, the provider unblocked C4, and two of my own claims withdrawn |
| `b877030` | `e7fc994` | pre-register the C4 re-measurement on the fixed provider, before running it |
| `8bc1c08` | `eaf45df` | C3 reproduces and C4 deny/pre-empt execute, both on the fixed provider |
| `8b99a99` | `eb1eaf8` | C4 deny and pre-empt execute, and C3 survives the substrate change |
| `d27ff99` | `ec07660` | pre-register the PCS form comparison before running it |
| `e346f99` | `eda6826` | CAL-05 exists: the integrity check is precise and insensitive |
| `ec78e97` | `ede157f` | pre-register the multi-node provider measurement, before running it |
| `e282f4d` | `f7ccb76` | pre-register A0, the joint availability/energy criterion |
| `7842c21` | `f8a6ed2` | identity-scoped QUERY and RESPONSE, answered only by the target named |
| `2817213` | `fbcca17` | the hysteresis band becomes a swept parameter instead of a constant |
| `1eb09f0` | `fc1f715` | lorawan links the physics engine its phy already calls |

---

## Local-line archive — attribution stripped, 2026-10-16

The local line (81 commits after `56d47fb`, never merged) was copied to
`archive/local-line-2026-10-16` with `Co-Authored-By` trailers removed, per the standing rule in
`RULES.md` §2. **Messages only:** every rewritten commit has the same tree as its original
(checked commit by commit), the count is unchanged at 81, and the first 55 carried no trailer
and keep their SHAs. The 26 below changed. Sidecars ported from the local line cite the
**old** SHA; this table is how they resolve (G13). The originals, trailers included, were on
`work/2026-10-16-imu-d8-audit`; that branch was deleted on the owner's decision (2026-10-16),
so the old SHAs survive only here, each beside its tree-identical rewrite.

| old | new | subject |
|---|---|---|
| `c620352` | `067921f` | pre-register D-8 stage 1: tier 2 on its 812.5 kHz row, probe + 2 configs x 20 seeds |
| `4332e89` | `ee7dcee` | G20: the IMU filter stays honest through a long outage -- the population G17 missed |
| `21d181b` | `7c00612` | IMU: moment-matched coasting, so the filter's covariance stays honest through long outages |
| `395cd64` | `d81a497` | D-8: --t2SensitivityRowKHz, tier 2 on its own bandwidth's data-sheet row; default unchange |
| `3b983d8` | `a12a630` | labels that said false things: two sensitivity captions, the D1 comment, verifier warnings |
| `42b76d8` | `ebee74b` | A3 measured: zero pre-emptions, and nothing else changes -- the same slot is lost either w |
| `4c6843f` | `b6c98f8` | audit: describe the G12b hits without reproducing the name G12b scans for |
| `f6b9782` | `c053877` | audit 2026-10-15: what the documents report as true that is not -- annotated, not erased |
| `36ee51c` | `66197c1` | evidence: the IMU bias sensitivity, 20 invocations at e94d04d |
| `aad1919` | `6c2ce51` | C3 result: the inertial prior, measured -- and why it fails on long outages |
| `e94d04d` | `b9b1451` | docs: stop the front pages contradicting the ledger; record C3, D-8 and the plan |
| `dd53ddf` | `8dc4048` | runners: the registered queue at one commit, and the cited-evidence reconstruction |
| `3a17f20` | `469b00d` | runner and scorer for the IMU bias sensitivity registered at cb60c71 §8 |
| `a28b591` | `35f4113` | pre-register A3: the intra-cycle handshake, with its blocked exchanges measured |
| `bbbb49e` | `84f8a93` | pre-register A1, re-specified: coverage fraction against dwell, scored by A0 v2 |
| `e9970ed` | `46fdf74` | pre-register E3: CAL-05 with the inertial prior, before any imu cell |
| `08c726b` | `ceadc7e` | A7 / D-5: a receiver-sensitivity check the D1 defect could not have passed -- and it fails |
| `f0b9f24` | `90cc829` | G18: every results/ path a tracked document cites is held -- red, 14 paths |
| `e5474d9` | `2ca7e52` | A3: the escalation handshake inside the cycle -- with the off-time it obliges enforced |
| `89ac3b6` | `d880ee8` | C3 measured: the inertial prior helps on short outages and is far worse on long ones |
| `cb60c71` | `f5a3ffb` | pre-register the inertial-prior campaign, with its runner and scorer, before any cell |
| `a4d6bf4` | `2f2945a` | pre-register A0 v2: A0's rule plus the branch it was missing, on new cells only |
| `9f5085a` | `6067671` | C3: --imuPrior drives the predict step from the IMU; off is byte-identical |
| `c7f12d7` | `7f01258` | C3: an IMU error model, checked against closed forms by code it shares nothing with |
| `8f121da` | `3fdc550` | Merge remote-tracking branch 'origin/main' |
| `9a636d7` | `b4d18b8` | record what results/ regenerates before it is deleted -- 2004 of 3277, and D1 is why |

## Six sidecars with one path segment redacted, 2026-10-19

By owner decision (no tool or vendor name in the repository), the six `report/b8/m{false,true}/s{1,2,3}/run_id.txt`
sidecars were edited once, on line 2 only: in the recorded `--outdir=`, the first path component under
`/tmp/` — a tool-named scratch directory with uid suffix `-1000` — is replaced by `[redacted]`. Nothing
else changed: line 1, the `source`/`dirty`/`profile` line, and every SHA-256 digest are byte-identical,
and the digests cover the run's outputs, not the sidecar, so every output still verifies. The output
directory was a scratch path on another machine and is not needed to regenerate the run (the
invocation's flags are intact). This is the only edit ever made to a tracked sidecar; RULES.md §2
forbids editing sidecars, and the owner overrode that rule for this redaction only.

Pre-redaction git blob ids, so the originals stay checkable against the pre-rewrite backup bundle
the owner holds:

| sidecar | blob before redaction |
|---|---|
| `report/b8/mfalse/s1/run_id.txt` | `b99c8408c626` |
| `report/b8/mfalse/s2/run_id.txt` | `2b5129c61f33` |
| `report/b8/mfalse/s3/run_id.txt` | `09c4e5628049` |
| `report/b8/mtrue/s1/run_id.txt` | `3fd35d8f54a6` |
| `report/b8/mtrue/s2/run_id.txt` | `2e3209bae628` |
| `report/b8/mtrue/s3/run_id.txt` | `2cef85c65532` |

# SHA map — name rewrite of 2026-10-19

The repository owner directed that no tool or vendor name appear anywhere in the repository,
history included, and authorised this rewrite for that purpose. Rewritten with
`git filter-repo --prune-empty never --prune-degenerate never`, every branch at once:

- **Path:** the rules file was renamed `RULES.md` throughout history.
- **Text and messages:** one fixed list of substitutions, applied identically to file contents and
  commit messages. It covers the rules file's name; the Part B branch name, now
  `work/2026-10-09-acc-reruns`; one unused tool-designated branch name; hosted-page links; the
  tool's settings-directory ignore entries; the identity pattern in G12, now a character class;
  work-order filename patterns; and phrases naming the tool.
- **Trailers:** two co-author trailers naming the tool were removed (one on `IMU_SHS_V1.1`, one
  quoted in a G12 commit).
- **Sidecars:** a `/tmp/` scratch-directory segment in `--outdir=` was replaced by `[redacted]`. This
  covers the six `report/b8` sidecars (see the section above) and, in history only, the sidecars of
  the 2026-10-09 ACC-A2, ACC-B3 and ACC-B3-US directories that the Part B branch later replaced.
  Digests are untouched.

**The current tree is unaffected.** `main`'s tree was byte-identical before and after
(`27f7d8c8`): the substitutions had already been made in the tree by PRs #16 and #17. 876 branch
commits before and after, none pruned, and the merge shape is preserved. 843 commits changed SHA; the
34 earlier ones (upstream ns-3) did not. The rewrite left zero matches for the name in any message,
identity, path, ref or blob on any branch.

**Resolving an old SHA.** The tables in the older sections give pre-2026-10-19 SHAs. Where such a SHA
names a commit that was rewritten again here, follow it into this table. GitHub's `refs/pull/*`
cannot be rewritten and still hold the old commits. The owner holds the full pre-rewrite history as a
bundle.

| old | new | subject |
|---|---|---|
| `002908407aa8` | `7d1c1c1b3b6c` | CaTblImuEkf: SetPositionCov and NIS tracking, for main's acquisition and relay mechanisms |
| `0031a5bbd84b` | `9cc3749a7dea` | A.7: the case census partitions the run, and says so where it can be checked |
| `003241c0d27c` | `9496e1377f9d` | the PCS form campaign runner, with the registered abort check wired in |
| `0037ea2decf5` | `a3f3ca33acc1` | tools/: the per-item workflow's helpers move into the tree for the cloud continuation -- t |
| `003eaac53be3` | `8fd791c6cf2c` | docs/SCENARIO_FLAGS.md regenerated (tools/regen_flags.py): --ekf5StateBounds |
| `00da84047a54` | `95278e6d337d` | close G10's two known-bad entries; sixteen superseded files say so in their headers |
| `00ee3f54605a` | `e072ed147756` | NW-2: every measured subject routes its own position-fix traffic on its own tier -- --fixR |
| `00fb4664df84` | `5344ce4900c8` | SY-1: the uplink that carries a tier-2/3 range to the gateway estimator can be billed -- - |
| `01abbed05933` | `aff62f6b04ad` | audit the baselines: three of five load-bearing directories predate D1 |
| `01c852da2a49` | `7288839f9c4d` | the dirty-provenance assertion gets an exception it can still fail |
| `022212ea1930` | `515bff788a6a` | N.14: the active tier can be fused with the floor -- --fuseFloorActive makes a cycle whose |
| `0279f6b8d67a` | `3e2c5fc8b38d` | terrain stops applying an 868 MHz wavelength to every tier |
| `02b6b1aa0e7f` | `7d0aaa9b8812` | N.20: --deadBatteryNoDraw -- a node whose battery has run out draws nothing more; by defau |
| `032290a92f18` | `2afcc1e8523d` | the lost node starts where it was last seen and wanders out of coverage, and register K9 |
| `0341444aeca0` | `242d46a56d1b` | the beacon-contention account is withdrawn, and the anomaly it explained was one seed |
| `03473f3cde8c` | `96bdd3569f95` | PREREG S2-C-ACQ, registered before any cell (exploratory): --acquireMinAnchors=3 under --u |
| `03c15dd607a8` | `30439ef194cc` | chore: untrack CA_TBL_HANDOFF_v4.md, which must stay untracked |
| `043e0d7a087a` | `148ee163dd91` | TECH_DEBT N.50-N.54 (truth-seeded filters; the subjects' localised overconfidence; NLOS ha |
| `045539d79c84` | `89a8b25cdd54` | the filter had no door for a position, only for a range |
| `048b7bbaac26` | `384798387265` | the PCS abort check was asking the wrong question |
| `04dcb117c6df` | `5f3e731a703e` | the proxy and reverse-path controls pass, and the first two attempts failed for the same r |
| `053d57027d6a` | `0e572480493c` | N.6 per chip: --halfDuplexChip -- on the LR1121 basis a node's 868 MHz and 2.4 GHz radios  |
| `05c0cceaedba` | `064fca401487` | ACC_B_RESULTS and TECH_DEBT N.35-N.38: the Part B evaluation, seeds 1-20 |
| `05cc6d7083c1` | `fb22fb4b29b5` | setup and handoff for a fresh agent on a new machine |
| `067921f072fb` | `114a56fc2a53` | pre-register D-8 stage 1: tier 2 on its 812.5 kHz row, probe + 2 configs x 20 seeds |
| `06853d19098b` | `ca5ec555d758` | peer targets published nowhere, so nothing could ever answer for them |
| `068e5d5f05ed` | `6999509463a4` | evidence: recovery-policy ablation on clean provenance -- the last gap closes |
| `06b9beffc1bf` | `22e59e7abfd5` | Network Layer 1/4 and Transport Layer 1: proxy query answering and a route home |
| `06cc21694b16` | `e0fcc2f87016` | HANDOFF 2026-10-05 sect 0: progress -- G89, the policy-input fix, the integrity-distance f |
| `06ea438631b2` | `54f7bffcad89` | PREREG CAMP-FINAL2 (scope D1), registered before any cell: CAMP-FINAL's cells on the final |
| `0774474680c7` | `09a93891e39d` | point RULES.md at the debt register and the C5 error model |
| `07a25704f05b` | `6fb4e2a8c43f` | M.4: --digestVizOutputs -- the sidecar digests events_*, nodes_*, nbrtable_*, loctable_* a |
| `0827f4cdf263` | `ec2eed8ee654` | FUSE-01b results: with the subjects' finder oracle closed, the fuse still raises availabil |
| `083541987156` | `2a9268a8e5ff` | V3 --rangeBiasStates (Schmidt consider biases for 868 MHz ranges in every filter) and --rs |
| `084a7ff60a37` | `afa0db153203` | demo/: the console, the run it was built from, and how to rebuild it |
| `089a9f1203dc` | `ba6bf38ffc5b` | explicit gateway placement, so the deployment can match the brief |
| `094da7de0b78` | `494c28732266` | sect 49: a flat battery stops the ranging, not just the chatter |
| `09b13c74fe77` | `08c670d0432e` | docs: agent working rules, master plan, and work packages |
| `09c8fdd081d8` | `65c27ac9a880` | Publication tables, colour-coded: every paired difference green where it favours CA-TBL (+ |
| `0a060dcc4b3b` | `3712cbdd5089` | register K3 before running it: is the evidence gate actually the 300 J? |
| `0a489df392d7` | `2e27b5a3c348` | docs: withdraw the reliability and Claim 3 entries after the v7 regeneration |
| `0a542ecc5c6c` | `f2435a964f31` | PREREG LORA-CMP: CA-TBL against outdoor LoRa-based localisation on our simulator -- plain  |
| `0a814cc21ce3` | `6fc4ebfb76d7` | the topology, stated correctly and in one place |
| `0a9ef942bd2e` | `fe0c18a9e5a6` | M.3: --stampBuildCommit -- run_id.txt records the commit that BUILT the binary, stamped in |
| `0b563fbb2137` | `52b903da21af` | port the SHS inertial engine, and report honestly that its driver does not work |
| `0bd936b390b9` | `ebcd31d5dc61` | Name scrub (#16): rules file renamed to RULES.md; branch, link and sidecar references neut |
| `0c383a463475` | `ae5c0de056b4` | a second target on the channel, as a protocol peer |
| `0c8e638f274d` | `288d4565ba2d` | T36 (N.42): the 'Hofer' urban NLOS branch is 3GPP TR 38.901 Table 7.4.1-1 UMa (verified on |
| `0d5e74794d37` | `c2e4ff4876a1` | fix: correct SF index base in 2.4GHz and UWB sensitivity tables |
| `0d89214bb79b` | `8de2d2b44342` | Citable re-runs of ACC-B3, ACC-B3-US and ACC-A2 replace the earlier results directories |
| `0da098c97ea9` | `9d81a76f8f3b` | P6 (owner decision): the preset takes --subjectRelayChain -- with every field node a measu |
| `0dac638fe765` | `6f92be22d6ba` | B3F-FOLIAGE results (registered; exploratory): with the clamp, UWB r80 rises from 0.0 m to |
| `0f68d4547359` | `7717e744e84f` | PREREG ACC-B, registered before any cell (S2 Part B, exploratory grade): --dualBaseRelayTi |
| `0f836c902997` | `03136b9e1cca` | the escalation predicate is self-defeating: wanting to escalate and being able to are anti |
| `100cc6504ab0` | `a14e5797a8c3` | Merge the main line (work/2026-09-27-spec-b1, 102-setting preset) into the accuracy branch |
| `10370f64fd9a` | `bfef08678771` | --ekf5FinderConsider: each finder's GNSS offset is a Schmidt consider parameter of the 5-s |
| `103c2554c6eb` | `eeac686e6039` | C4 --foliageP833 (default off): forest foliage gamma from ITU-R P.833-10 Table 1 (log-log; |
| `104538869e94` | `8e8950613eb5` | DEBT-A results (D1, D4, D6 held; D2, D3, D5, D7 not): T26 and T33 closed; T12, T27, T29, T |
| `1057f6f811dc` | `00bdf98eddc5` | tools: commit the analysis scripts the gate suite requires |
| `1098ee3063aa` | `e61eed7341fe` | session record: five work items, five campaigns, six errors |
| `1164e5de5690` | `a971006842af` | revert the generated-with widening: it broke the gate on its own documentation |
| `11b4dce82e2a` | `3e2848d97924` | Preset adopts the master-plan-v3 fixes (owner decision 2026-10-17): --rngKeyed --rangeErro |
| `11da4fc9b160` | `347506ebeb24` | TECH_DEBT L.3-L.4 status: four claims survive regeneration, PCS and tiering do not, and th |
| `11f4be0e3fc7` | `266d8db7ac98` | V2A2-NLOS3GPP results (registered at b9ed29bb; exploratory): N1, N3, N4 held; N2, N5 not h |
| `12958f791e9e` | `2fd088a50225` | T46 code: --rssiRangeFromRxPower (default off) makes the 868 MHz RSSI range carry the link |
| `12960c3c7a20` | `593c635a9c01` | ACC-B3-US results and TECH_DEBT N.39: the unknown start at s = 100 (registered, 41be7959;  |
| `12a4b3b8144a` | `860b022d7ec0` | register Claim 4's test on the mediator, not the endpoint |
| `1312f171541b` | `444610cb7031` | Network sect 5 and sect 6: relay chain composition, and the anchor tie-break |
| `136a2af978bb` | `16cb2282f8be` | C5 done: 12 of 15, and Chapter 12 has no ABSENT row left |
| `13ddfc60fb48` | `53b632fe0fea` | G10: allow the per-run output name subject_summary.csv -- report/PREREG_GEO01.md (d54b68b) |
| `1420cedbac06` | `a304cdc5c270` | T19 (2.3) explained: the UWB/868 near-field residuals sit in the engine's outcome-dependen |
| `142593707184` | `0248baac1f4b` | RULES.md: the merged main's suite (111 PASS; reds G6, G11, G24, G92, G93) |
| `149627447630` | `ea1b7b9c06a2` | refresh the conformance matrix: Chapter 12 is no longer empty |
| `14d86ba50a89` | `cdb62f62a8b2` | MC-2: the spec's triggered update and last gasp exist behind --triggeredUpdates -- a corre |
| `1552325dbaef` | `03f3c2787000` | TECH_DEBT N.26: CA-TBL's endgame UWB escalation is 64.6 % of subject energy for 0 UWB fixe |
| `1596cb41e5ee` | `65224c021d8f` | Evidence: full suite on row 17's tree (b8785f1e, run on b98c4913 before a message-only ame |
| `161ef57c05b5` | `3b9c8dbfcfc0` | A2 stage 1 runner, with the fault label mapping corrected |
| `16325af0d6b0` | `e307bc19038b` | HANDOFF 2026-10-05 sect 0: FIXES-N26 outcome (beats Poucet and LoRa fusion; not adopted on |
| `165df16b64f3` | `26da794a23ff` | HANDOFF_2026_10_08: the local verification's result, and a request to S1 (G63, G87, G89 re |
| `166d431ad065` | `3eaba15e9b3d` | PREREG UWB-NLOS: a cited IEEE 802.15.4a NLOS model registered before it exists -- the fill |
| `16b70d9ec9b6` | `d5d34f066f7d` | Merge branch 'wip/c4-arbitration' into wip/c34-on-provider |
| `17073d19ccd6` | `b3233462f13e` | P6: the preset takes GEO-01's geometry -- --specProfile now sets --nGateways=8 (a ring), s |
| `17196047a128` | `6939182f3f88` | HANDOFF_2026_10_04.md: evening addendum -- row 55 finished (every subject runs tag 0's TDo |
| `17472fe761dc` | `adfa3bdd6c4e` | PREREG TOF-FIELD, registered before any cell: tier-2 ToF error at Mueller et al. 2021's me |
| `17ab7ddfe84a` | `e68f8063505d` | two gateways that both grant one field node now settle it by gossip: the lower id yields |
| `17d1a1129483` | `b5cacb9ad505` | branch references: the Part B cloud branch is now work/2026-10-09-acc-reruns |
| `180d63a91709` | `f0ea2a0f267c` | C4-P833 results: F1, F2 held, F3, F4 not; the hybrid's endgame is sensitive to the 2.4 GHz |
| `184d688bb34b` | `619fc40bcf8b` | NW-8: the reverse path can exist -- --reversePathHop caches the previous hop a QUERY came  |
| `185d8d1c5150` | `a229f97ed534` | N.26 fixes, four default-off flags: --loraDualBase (both LoRa tiers ranged and fused every |
| `186990ff7f13` | `6ff7bf5abfcc` | correct the record: 622.92 J is a directory mean, not a missing number |
| `18d14f6d9d92` | `6c9ea93711eb` | Handoff and TECH_DEBT N.34: refer to the S2 cloud branch by role, not by its name |
| `19211fd21483` | `bd16943a3650` | the local line, compared with main and archived without attribution |
| `194706721773` | `35700b916ba5` | B4-SWITCHING results (registered at 9a14df6a; exploratory): 360 runs, source 6f92be22 dirt |
| `19cf3d6e58a1` | `7c1faa9d9eca` | Publication tables, compact main-text form (report/paper_tables/tables.{tex,md}, preview.h |
| `19d9131eeca2` | `dfbec8dd8d08` | register time to first fix -- the total burial budget is sourced, a phase budget is not |
| `1a1524043c2f` | `ef4924eac733` | A.6: a session that recovered into TRACKING is not one that climbed to it |
| `1ab35ac5a35c` | `9da176c26bc5` | S2-C-ACQ results (registered, 96bdd356; exploratory): 12 runs exit 0; reuse check 4/4 byte |
| `1ac680259c03` | `e83f1c3424eb` | fix: G6 compared a real file against a path that never existed |
| `1bae0ebc2008` | `8e0cdf651733` | a stale entry may be reported, but it must not steer a searcher |
| `1c0ddb254d4e` | `6d1e47ca4986` | D-1 repaired: the system works better, the confidence claim does not survive |
| `1c1886c79230` | `0ead49bfdec1` | G91: gate for the S2-B accuracy flags (N.35-N.38), with negative fixtures |
| `1c45463a2d10` | `7af1aa503c6d` | RUN.md (S2-C, S2-C-NLOS, S2-C-ACQ) and the handoff: the run logs' 'runner exit $?' follows |
| `1d2f8333c42a` | `1d0bdd1a054c` | seventeen citations land on a case label or a blank line; twelve were off by one |
| `1d444eaa07db` | `0d6816b6f6a6` | B3 read-out: static_868 at k1a_box750 seed 7 is 34.7 J, not 34.6 J |
| `1df3e6fca54f` | `489b058e0962` | the trace divergence is two phenomena, and C5 has a named blocker |
| `1e0d9aaac933` | `47520a724f53` | TECH_DEBT N.32 (measured, remedy rejected) and N.33 (built, default off, not established) |
| `1eb09f0aa563` | `7fbbe080ecbc` | lorawan links the physics engine its phy already calls |
| `1eb6a655a61a` | `bc77aa89fb61` | G102: --ekf5StateBounds is reached (147 projections on 5 km seed 12), holds the scale insi |
| `1f10cf72a91d` | `e956c28425c6` | M.3: --stampBinaryHash -- run_id.txt records the SHA-256 and path of the binary that ran,  |
| `1f5a17ff6820` | `26576ae32c8e` | context.md: refer to the S2 cloud branch by role |
| `1f73930cc0a9` | `1cce9361c299` | register the check on Claim 3's only PASS before running it |
| `2036d04d93a5` | `0455a4aca039` | B2CONS results (K2-K4 held, K1 not) and DESIGN_DECISIONS 15: the preset sets --imuQuScale= |
| `20ccc932d3ab` | `f934a73ffb35` | T9: G1 refuses a tree not configured with the optimized profile (build_profile_check.sh, d |
| `215e2d83c6ff` | `6d1baa7c26a5` | add exclusion as a suspect-response policy, default off, and register its test |
| `21962189c5b1` | `cd766cb5557d` | UWB reach write-up (report/UWB_REACH.md, TECH_DEBT N.41): the datasheet budget, the one-te |
| `221e747ba52f` | `8986731e6ed6` | NW-6: anchors can gossip their queue as well as their grants -- --gossipQueue carries the  |
| `232241bd1226` | `39e78eb5342d` | the technical debt register, ordered by risk to results rather than by effort |
| `233f98aa3704` | `7507e9e8cade` | anchor G12's vendor handles to trailer form, which its own comment already argued for |
| `23499d10b5de` | `756fb1f299ca` | TECH_DEBT N.60: the last gap to fixed_threshold (endgame availability -0.010) is an availa |
| `23821c966f7e` | `4eba6faf535b` | G10: the lost-evidence note cites the full results/ path (recorded lost) instead of a bare |
| `24387cd24b5d` | `e2bf31636d1f` | N.35 --dualBaseRelayTier: under the dual base, relayed (C5) legs on the awake 2.4 GHz radi |
| `2438c719fbc7` | `03a58f4ecda2` | HANDOFF_2026_10_09_ACC_B: Part B's state -- F1 built and gated, N.35 diagnosis, ACC-B regi |
| `2444a13bd77e` | `06cfe31b72ab` | evidence: c9_escalate, p10_grid and p10_reliability, with their runners |
| `24eb6ba729c1` | `fdd2cd74ee9e` | P6 (N.21, tag 0's half): the preset takes --finderPseudoAnchorStrict -- no node in the spe |
| `250bf8150fa8` | `048d3e07504c` | write the provider blocker report the register was already citing |
| `2552bd9df4f4` | `a45ef4bb3b24` | Local verification of 07486c4c: null control, G100/G120, full suite, exploratory k sweep ( |
| `255bd4959ae6` | `dfad7b41b396` | HANDOFF_2026_10_08 and scripts/uwb_reach_ablation.py: the evidence-fix branch's state as a |
| `25887d065d52` | `61d9976486a2` | K10: CA-TBL beats the published estimators on a moving subject, and fails on a still one |
| `26721ef444d7` | `124c8b0959ad` | study: add the closed-form propagation module and its defect record |
| `268d020567df` | `78507ab24c0d` | Docs follow DESIGN_DECISIONS 14: RULES.md preset list and headline note, Technical Report  |
| `26c541bc559e` | `98999d00f09e` | Merge branch 'wip/lane-b' into main |
| `26d002e1b273` | `85763e9e65d5` | CAL-01: the LOOCV machinery the blueprint specified, and the control that guards it |
| `26fc908ef071` | `b1261948ad3d` | Row 55 / N.3: subjects can send MC-2's large correction -- --subjectTriggeredUpdates pushe |
| `276209cb5b56` | `ce5119fd8ead` | TA-3 + TA-4: a session can become precise on a confirmed grant, and a batch reports its pr |
| `276a3aafa098` | `53d3bc603c84` | terrain is unvalidatable; both headline conclusions are invariant to it |
| `279f0c388e99` | `908bab1786b5` | Merge pull request #3 from YKushal-hub/work/2026-09-07-topology-docs |
| `27ccd3a62225` | `9fbaffe647b0` | TECH_DEBT N.27-N.30: what the local line found that is true on main too |
| `2817213da066` | `f51c80f58928` | the hysteresis band becomes a swept parameter instead of a constant |
| `293dc3aa07c0` | `1f6c94cae91a` | add the D5 PCS repair and complementary siting, both flag-gated and default off |
| `29dad31324dc` | `b3822dbd8c67` | R1-REINIT results (registered at f324e1bc; exploratory): --acquireReinit does not rescue t |
| `29e41f8bccf0` | `4f0f178016cd` | sweep fault multiplicity: only the single-fault case resolves at 20 seeds |
| `2a011e0feac6` | `684b320bf4b5` | the table entries now carry contents, so a position can reach a searcher |
| `2a61fd2020e4` | `64d7e89d93bd` | tier 1 is billed for an exchange one message short of closing, and now that gap is measura |
| `2ab38c2e71fe` | `a145016cb437` | docs: withdraw W1 and N3, correct r80, and document the deliberate red G6 |
| `2af570256d13` | `f41286ca2cb6` | the UWB coverage label could not see the only source of UWB, so every escalation scored fa |
| `2af7444f144a` | `ee93cdff5427` | G12: stop matching co-authored-by in message bodies, and close a hole beside it |
| `2b27af971439` | `3c482a81cc46` | the evidence gate is worth 324 J and is off by default; the integrity check's cost is the  |
| `2bac44ffb0cd` | `72be495a786b` | figures: six-figure set for the positive-results document, recomputed and asserted |
| `2becdaa3fdae` | `16bb612d2a41` | G10 read a quoted verifier message as a citation |
| `2c1f888ed586` | `507175a573e6` | chore: ship every scratch source, not just the CA-TBL ones |
| `2c40241298d2` | `f324e1bc5d1c` | R1 (TECH_DEBT N.54): --acquireReinit re-initialises each field-node filter at its first th |
| `2c4ff7d8d1bb` | `159d9e40141b` | no assistant attribution anywhere in this project, as a standing rule |
| `2ca7e52a9185` | `c281c7547282` | A3: the escalation handshake inside the cycle -- with the off-time it obliges enforced |
| `2d6170bd1a9b` | `28d89a256441` | TECH_DEBT M.10 is a DECISION: at the edge default a floor-solved fix is billed as a 0.2005 |
| `2d93287e67ff` | `01e25b671973` | Merge pull request #12 from YKushal-hub/work/2026-10-17-d8-main-results |
| `2dbbc3c52141` | `5bdc075f5e62` | P6 (owner decision): the preset adopts CAMP-SPEC's routing-weight pick -- --specProfile no |
| `2dde56b66e3f` | `e6a0a9d0f043` | a packet event says what the packet is carrying, not only what type it is |
| `2f2945a6d66d` | `7d4667338c87` | pre-register A0 v2: A0's rule plus the branch it was missing, on new cells only |
| `2f60b75c137f` | `dd6b61f45104` | G95 checks the wiring on main's preset: the acquisition cloud's handover, ekf5's own NIS,  |
| `316fbf8cd6ac` | `575bf610b38c` | S2-C-NLOS results (registered, d37e0e97; exploratory): 60 runs exit 0, one compile stamp;  |
| `31935e36221a` | `10020dd5903b` | Merge work/2026-10-09-acc-part-b into main: S2 Part B -- --dualBaseRelayTier (G110), --sub |
| `31db16c4f359` | `6ccf8c0904ae` | register the foliage Jensen fix before applying it |
| `322576010733` | `e32cf4c93822` | P6 (decision 8, both requesters): the preset takes --finderRequesterEscalation -- a finder |
| `32c5909a454e` | `9353aa096567` | N.3 / N.15: measured subjects can take part in the relay chain -- --subjectRelayChain lets |
| `3363f854ac7a` | `bf0305cbd3fd` | sect 24-27: a node that comes back is not a node found for the first time |
| `341a824862b3` | `78ff2fedd079` | S2-C cross-branch audit (report/S2C_CROSS_BRANCH_AUDIT.md): branch and PR inventory agains |
| `342d33b85971` | `ad330b8c0298` | Citations: the physics engine's comments and the provenance rows now say what each source  |
| `34b7a4b668d6` | `a8eef237fd29` | the citation checker read one line of every range and measured its own parser |
| `350fa493668b` | `77785ad254c2` | Scope 2026-10-17 batch (all default off): --probeMonotoneTime, --fadingPerBand, --agcPerRe |
| `35da73476c6c` | `22cbfeac1e57` | T24 (O.9) diagnosed: the RSSI inversion has no foliage term; P.833 admits 11 % more 868 MH |
| `35f4113e14fd` | `6d720a5c1dc1` | pre-register A3: the intra-cycle handshake, with its blocked exchanges measured |
| `36199b587d51` | `426a8c606146` | A6: hysteresis is a live axis, and the dwell is an energy/accuracy dial |
| `362fbe2192cf` | `77f953838590` | D2: Technical Report §0a (claim record restated 2026-10-17 against CAMP-FINAL2), Architect |
| `36521fd8a0e7` | `3eee747eb95c` | Citation audit (G10): the same allowlist lines as S3 (49647391) for the four paths that ca |
| `376d718b29fd` | `7436d4bac1df` | finderMode 2: search outward from the last known position until there is contact |
| `37e42e3d8953` | `1501b2bb9515` | DC-6: the spec's calibrated PCS is expressible -- per-tier thresholds from the CLI, a weig |
| `38a84d1eaf5c` | `f6c9349577da` | V1 --rngKeyed (default off): keyed random draws per (seed, band, link, time, occurrence) a |
| `38d2fb4812f5` | `6cd731f7786c` | T2: --publishAfterStep (default off) re-publishes tag 0's entry after Step at the same sta |
| `38faea0ade53` | `447c078014d6` | chore: record the attribution rewrite, add SHA_MAP.md, and gate it with G12 |
| `39483a0258e6` | `6f802b585808` | an empty detector population cannot convict, and G4 was failing on one |
| `39e931e12404` | `b1259b45b383` | Merge branch 'wip/lane-b' into main |
| `3a46cb655b96` | `5b0bfe69e073` | Merge pull request #2 from YKushal-hub/work/2026-09-05-defect-repairs |
| `3aba1b75ba77` | `f67ea7f1a10b` | finders spread over the reported uncertainty instead of stacking on its centroid |
| `3b410a0e3435` | `2081309fc30c` | G100 and G101: --ekf5FinderConsider (reached on 1612 finder ranges; R lost exactly the 4.0 |
| `3b5fffba30b8` | `70b468d73a5d` | the frontier's winning points are not allocatable; the premise survives constrained |
| `3b90a75b6ff6` | `37725c874fc8` | P6 (row 56; owner call, DESIGN_DECISIONS sect 9): the preset takes the spec's deployment,  |
| `3c95a8661910` | `d77496971de0` | the PHY provider carries a position per field node, so a peer tag can be heard |
| `3ce0618955b0` | `8ce92d19fdde` | docs: track the two report documents the citation audit reads |
| `3d16739fc139` | `8596c2eca8d4` | the residual explains 71% of the r80 gap, and tier 3 was never blocked |
| `3d1cc3e17849` | `51f0757fde16` | docs: agent working rules, master plan, and work packages |
| `3d637f47e229` | `00eb5a0bcf49` | --arms=lora_fusion: dual-band LoRa fusion (poucet_fusion without UWB) so the outdoor-LoRa  |
| `3dbc3a80dd1b` | `8ea623002ca3` | DESIGN_DECISIONS 16 (owner decision): code-default --boxHalf 1000 -> 340 so tier 2 covers  |
| `3dd69c9cdf45` | `dc510b4f1f6d` | CAMP-SPEC results: at 5 km the preset localises a third of the subjects' cycles, forms no  |
| `3e1a270a0dd9` | `f2c16e2615f6` | fix: degeneracy screen is per-mechanism; propagate two withdrawn findings |
| `3e40aff16b96` | `cc68297178c7` | PREREG LORA-CMP-2 amendment: the registered reproduction check fired on poucet_fusion's rm |
| `3ebb3eb0521e` | `076f128b9e53` | Claim 1 withdrawn as W12: the trigger is inert under the corrected NLOS source |
| `3edfec8fb486` | `7d120924568b` | fix: make the must-stay-untracked rule mechanical, and record why history stands |
| `3f0f618f4a46` | `dfacb80f896d` | DC-1: --imuGyroBiasSigmaDps, so its gyro bias can be a turn-on residual instead of bias in |
| `3f14e35dea6b` | `e07c2d634c5e` | V2a-2 --nlos3gppRMa (default off): 3GPP TR 38.901 v17.0.0 RMa LOS probability and NLOS exc |
| `3f3e12e914d1` | `a401a4391655` | S2-C analysis, corrected after the run: (1) verdicts are cast to bool before rendering --  |
| `3f87ee26a494` | `8e0232f35464` | W11: Claim 3's only PASS was an artefact of the legacy NLOS source |
| `3fdc550de2e9` | `e6249ac7fefc` | Merge remote-tracking branch 'origin/main' |
| `400a7ae6636b` | `288ff7ca3d04` | plan_6h.md: two independent parts (filter consistency; hybrid range supply and subjects) t |
| `40e3258264b5` | `868e2c48b3aa` | the UWB evidence gate reads what is audible, not a map of a deployment we no longer have |
| `410c99809d72` | `770a07a50e57` | register K7 before running it: the integrity gate's innovation covariance is missing its R |
| `4112a19a8588` | `36a7138531ce` | docs/TECH_DEBT_STATUS.md: every TECH_DEBT entry classified (open T1-T45 by class ENG/CAMP/ |
| `4116b64c5fe4` | `8ddb5d10ec74` | The IMU line on main (#15): the 5-state filter, a realistic sensor world, and its consiste |
| `41ba903a6547` | `f2282fe20f5c` | fuse dead reckoning on no-fix cycles, and measure the ceiling while the SHS is debt |
| `422948967202` | `6ef0a5ae5544` | ACC-B2 results (registered, 25c118a0; exploratory): leaving the dual-base subjects' 868 MH |
| `427dabd16939` | `3bbbbeca2f6c` | T41 (5.3): gate G152 -- current documents name only registered flags, and RULES.md's prese |
| `42c7e30868f3` | `60e53329f793` | instrument the exclusion branch census, flag-gated to stderr |
| `42db438a5058` | `24fd58b20004` | census: every configuration where tier 3 serves depends on a documented defect |
| `4302918e919d` | `6164fb6a7a81` | declare the anchor's own error in the fusion variance -- it was being hidden |
| `4412fac3d847` | `a359def9f4ee` | pre-register MAG: a magnetometer heading reference (--imuMagSigmaDeg=3) on REAL-IMU's real |
| `4450a771b301` | `2cd0886f8409` | DESIGN_DECISIONS sect 9: the owner's open calls, delegated on 2026-10-04 and decided again |
| `449f397448a9` | `c81cd4c00f11` | DC-1: the filter's prediction can take a synthetic-IMU control input, as Physical 2.2 says |
| `44b041177b7e` | `4d9842b0e7ae` | TA-1a: the base station can run a real search -- --sessionOverMesh opens one session for r |
| `457957188bbd` | `09f476375ea6` | RULES.md: final suite on 240255d1 verified (107 PASS; reds G6, G11, G24 only) |
| `469b00d7f64f` | `4df61eabe1c5` | runner and scorer for the IMU bias sensitivity registered at cb60c71 §8 |
| `46fdf7423658` | `0d8b2bf5cec6` | pre-register E3: CAL-05 with the inertial prior, before any imu cell |
| `470458599d3e` | `c384c5244978` | register: 14 of 15, and every work package is now done or closed with a reason |
| `472e1d93c5f5` | `9403c94a4704` | fix: make a fresh clone buildable, and fail loudly instead of silently |
| `4739849b0694` | `94346576e402` | PREREG DC1-BIAS: the preset with a turn-on gyro bias -- before any cell |
| `4779bf7b5ea7` | `bf76d073ad97` | gateways as edge servers: the estimator moves, and tier 1 costs one broadcast again |
| `478930513a95` | `c2f7b9310e51` | HANDOFF 2026-10-04: second addendum -- the owner's delegated calls as decided against the  |
| `47e69ba474d9` | `7144aaad0135` | INTEG2 results (T21): the re-fit threshold is 9303.8 and recall falls to 0.012; the fault- |
| `481e1de7edb3` | `28a03c063ba9` | PREREG CAMP-FINAL: the finished 97-setting preset measured for the spec's comparison table |
| `489fa0465bea` | `1b1ec450c669` | v10 gate references, generated with the evidence gate on, on a clean tree |
| `4a2bd037e003` | `2fac64eee8f7` | T6 (H.3/I.5): gate G151 -- every adopted preset setting (12) changes the preset's output w |
| `4aa38aae6bb3` | `ce901a4d0bd0` | pre-register A6, the hysteresis sweep, before running it |
| `4ab5d5d12010` | `e984476a2c49` | fix: the PDR criterion was an artefact; add the sourced foliage term |
| `4b99c0be3a44` | `b971e8e977c6` | TECH_DEBT N.15: CAMP-FINAL's integrity-check base rate with no fault injected (0.775 at 5  |
| `4c826cc036ec` | `9ba4d6017d86` | B1: anchor-to-anchor ranging, and a prediction whose premise the channel forbids |
| `4ca0dff91539` | `e160dde771d3` | Merge pull request #7 from YKushal-hub/work/2026-09-27-spec-b1 |
| `4d1d3495ec13` | `6298a521902d` | C4: fix the peer MAC binding, and narrow why arbitration still sees one requester |
| `4d219963a448` | `521c8a0b1f2f` | ACC-A analysis, abort check corrected after the run to test what PREREG sect 6 registers:  |
| `4d9e5fcc0301` | `f976c61f1ebf` | NW-1 (steps 1-2): the hop count can reach the decision and the RESULT -- --hopProvenance p |
| `4dbdae4e6710` | `5a0bfd53e55b` | tools/regen_flags.py: the count of added names past ninety-nine falls back to digits inste |
| `4ddabe7e3d4f` | `40c57b2828d1` | T39: the preset stamps the binary hash and build commit into run_id.txt (NC10 identical);  |
| `4dec767e3591` | `48428deffd34` | HANDOFF_2026_10_04.md: state after the owner's 2026-10-03 decisions, the N.21 oracle and t |
| `4e087627b30a` | `8adea5b103eb` | S2-C engine, default off: --nlosRSource=operative/detector/none chooses which NLOS tag key |
| `4e7fb0540d9a` | `0c626c4932f7` | the registered criterion cannot be met; register G11a before computing it |
| `4ece0f0f3b3e` | `d5032c9b1c76` | ACC-A static 2.4 GHz diagnosis (post-hoc, exploratory, from ACC-A's traces; no cell run):  |
| `4f54a39b73c1` | `54d50c929d71` | register G11 and the tier-1 TDoA build as debt, and correct H.1 |
| `5018c7aa92fd` | `466893e4d1f2` | linkbudget.py carries the engine's per-tier foliage scaling as the engine applies it -- th |
| `504b6122aafa` | `795fa1e925db` | GEO-01 stage 1 results: ten shapes go to stage 2 -- star and cluster at N=3, arcs from N=4 |
| `50637167a99d` | `0ed294c112da` | Citation audit (G10): the same allowlist lines as S3 (49647391) for the four paths that ca |
| `50a229a013bb` | `b6253d668046` | sect 11.11: the mobility state is driven, so the eager-push branch stops being dead code |
| `5102c8d52bda` | `c2280c518c6e` | --tableFuseCI: give the CI branch a flag, and report that it still never fires |
| `51081f374030` | `f134c2eceff9` | G93: each tier's receiver against the data sheets -- FAIL BY FINDING on tier 2 (D-8) |
| `513f55807dcc` | `a0d82684570f` | derive the requirement from the operation -- three of four supplied figures do not survive |
| `51a2e5ef5f43` | `775a8ee9939a` | rival localisation systems, computed from the same measurements in the same cycle |
| `522e933e051a` | `523848fd5808` | the Transport and Application layers run in the simulator, not only in their suite |
| `5262c5987be5` | `1b549e6d985e` | a finder knows where it is to within its GPS error, and republishes it as it walks |
| `528752f47994` | `c8aae1e1f10b` | PREREG FUSE-01b: FUSE-01 re-measured with the subjects' finder oracle closed (TECH_DEBT N. |
| `52b8ebacc5b7` | `20c9c47537d3` | PREREG D8-MAIN: tier 2 on its 812.5 kHz data-sheet row, measured on the preset -- before a |
| `52cfb0773d32` | `8ae6bedf09d1` | D8-MAIN results: the corrected tier 2 strengthens CA-TBL's availability lead and removes i |
| `52dc7a022325` | `966f9fcdc50a` | TECH_DEBT N.28: the remedy is built, behind --imuFilter=ekf5, and is not yet measured |
| `5328f13c012c` | `db960c8aab12` | DC-9: the IMU witness can check the escalation against the synthetic IMU and the raw radio |
| `535922732e3d` | `dfd2ac57dc05` | runner for the registered h = 1 campaign |
| `53766acb6216` | `2f15a8c55c05` | N.39 --unknownStartSigmaM: start every filter off its seed point, under an honest prior; g |
| `537add05ab44` | `eb1640f97742` | FIXES-N26 phase 1, the registered integrity-threshold fit: even with --mahalUseR=true the  |
| `53b95cd19943` | `d60314ce2697` | B3 read-out: C4 P4.1 is FALSIFIED in 3 of 11 cells, the outcome the registration names |
| `53c429ca26fd` | `bd6de96e9fb0` | exclusion helps a single-anchor fault and HARMS a common-mode one |
| `53e5c38b8d35` | `7d3feea785aa` | M.10: the gateway downlink is billed on the fix's tier by default (--edgeDownlinkTier=fix) |
| `55aee233d77e` | `4cbcf72095b0` | docs: withdraw a wrong finding -- the trigger pair WAS reproducible |
| `55bfb0a8f07a` | `e781b8d99619` | register K5 before running it: is the 32.3 percent suspect rate an under-grown covariance? |
| `56d47fbd7a83` | `77d7d53aa75e` | Merge pull request #1 from YKushal-hub/work/2026-09-05-defects-and-campaigns |
| `56dcd89c1c31` | `6cf4f91f60f4` | io: check every output stream before reporting success |
| `56ec9b6a8640` | `ee19dfb5518a` | register the siting repair -- and the pre-flight reverses one of its predictions |
| `57225b8439dd` | `7e80a32462e7` | M.2: the registered UWB NLOS model exists behind --uwbNlosModel=ieee802154a; the default s |
| `5751945a1a78` | `aa0e546bdb14` | the confidence score gets a form that can veto |
| `57ebb41024ab` | `6acf9f72d650` | hop provenance reaches the confidence score, in the shape the error model asks for |
| `58556edd7b5d` | `e8c39d4bb154` | TECH_DEBT N.25: at 5 km with three gateways gateway TDoA worsens tier-1 accuracy (static_8 |
| `589a797cb625` | `5ffaffe9981d` | Merge draft/camp-final2 into main: the unregistered CAMP-FINAL-2 draft (lora_fusion as an  |
| `58b12eb61d95` | `0b55048d46d0` | P6 (decision 4, all five measured): the preset takes --subjectEdgeUplink -- every subject' |
| `58cb5d318108` | `e8b0480f5c16` | docs: publication README; correct the Pareto front-point count to 13/13 |
| `58f2d0419b68` | `b67fe24823c8` | C5 end to end: relayed measurements formed, admitted, and fed to the filter |
| `58f3a98c3f15` | `29a23e8818f4` | ACC-A decision diagnosis (post-hoc, exploratory, from ACC-A's traces; no cell run): in the |
| `590d6ca736e7` | `4d38ad2336fa` | the confidence score gets a form that can veto |
| `592c5595030d` | `a3cc0371b018` | a definition of done, and a register for behaviour nobody intended |
| `59767b817c7a` | `9a14aef4094d` | a tier-1 handoff stops costing a ranging slot |
| `5a6f4c26d647` | `9cb1e29deb44` | TOF-FIELD addendum: the hybrid's resistance to the two-range failure stated as a hypothesi |
| `5a733446ea59` | `37c191d28bb1` | K1: the domination claim fails as tested, and the integrity check is why it fails hardest |
| `5add3b0d4a38` | `248f645f6a3f` | C1_NOGATE read-out: the B3 coverage counter reads 0-73 where the flag is set, not 10-19 |
| `5ae26fd4e7e1` | `91f69873f9a2` | PREREG INTEG2 (T21), registered before any cell: re-fit --integrityChi2 by FIXES-N26's unc |
| `5b20993b98a3` | `c50f805ecee8` | NW-1 step 3: per-hop variance and NLOS enter a relayed range's sigma -- --hopVarianceTerms |
| `5b839d0de123` | `dbdad80392cb` | PREREG TDOA-01: is tier-1 TDoA's reported covariance calibrated, and how often does the ex |
| `5bbac5dd2786` | `6db20e791acb` | the topology correction never reached the propagation, and 2.4 GHz out-ranges 868 because  |
| `5bcb98cc5b41` | `e0ec26b1df20` | DC-4: Case D exists -- --caseD: a single range is applied as a constraint on the propagate |
| `5c28eb7506b7` | `94eac1abdecb` | PREREG ACC-A-OPPOINT, registered before any cell (exploratory): tests the diagnosis's read |
| `5c5ab1b98998` | `05b65d37fb1d` | PREREG V1-RNGKEYED, registered before any measurement cell (master plan v3 stage V1): dist |
| `5c9e6a4725fa` | `49647391d803` | Citation audit (G10) clean on S3's documents: allowlist the four paths that cannot resolve |
| `5d1534e0c0f9` | `7d366dcd1c13` | item B pre-flight: the deployment cannot support B4's fault on the tier it targets |
| `5d20de9156a7` | `1b1c9d1ea725` | the filter starts where the subject was last seen, not at the truth |
| `5d22e8383978` | `0ba688207484` | PREREG V5-C2B, registered before any measurement cell (master plan v3 stage V5): escalatio |
| `5d88faf6d90f` | `817ac9007708` | open items: the PHY provider re-scopes C2, C4 and C5 |
| `5da316501d04` | `3202f86b874a` | context.md: what CA-TBL is, what it measured, and where the localisation error comes from  |
| `5dc96ed44ddf` | `d0291bd7d6a9` | the escalation handshake becomes a frame, not just a charge |
| `5def42956784` | `26c89dcfd3b7` | sect 24-27 and Application sect 5 reach the scenario, and the fault they detect is injecte |
| `5e6fd01ce68e` | `1c94b6435feb` | N.36 --relayCovScale: scale a relay's covariance in tag 0's relayed legs (measured, kept a |
| `5f0b128db7f4` | `7d94efb604ed` | the handshake contention effect is 33.7 percent, not the 41 percent one seed showed |
| `5f1ee44ceff4` | `d6b6e122ec0e` | T7 (N.29): the validation harness's Metric 2 spaces frames at twice their time on air, so  |
| `5f3ad42b60a2` | `0c8feacc9909` | a relay out of a gateway's reach now routes its fix through one in reach, and ETX and the  |
| `5f6fa316848b` | `7abda97a555c` | CAL-05 as a curve: the integrity check is a working detector |
| `5f8bbf1e7dfe` | `b7783a59495e` | Tier-1 validation script (analysis/tier1_validation/tier1_validation.py): the engine's 868 |
| `6067671150b3` | `53274c93d7c6` | C3: --imuPrior drives the predict step from the IMU; off is byte-identical |
| `60932657a6a2` | `ea30f32c318b` | C1 verified: the handshake, once it contends, costs 41 percent more collisions |
| `60c3b84d9c4c` | `5114d9d32e01` | plan_part_a_filter.md and plan_part_b_hybrid.md: the 6-7 h plan as two self-contained, ind |
| `60d2dfba54c4` | `317f723a559f` | a sweep that asks whether every flag can change its own output |
| `614912b109a7` | `a4672b60c555` | terrain stops applying an 868 MHz wavelength to every tier |
| `61d2f89aa100` | `ad9827cfd94f` | --ekf5Robust: the 5-state filter's update mode as a scenario flag, default 0 (N.33) |
| `62743b40bf5d` | `bebaab78d6dc` | rename the repository rules file to RULES.md, and every reference to it |
| `63bba4fd2c5e` | `8a082cf32d64` | chore: land the gate, manifest and report state from phases 9-10 |
| `640f57b5b462` | `e84069cf7489` | docs: archive superseded working documents, keeping code-cited ones at root |
| `64277c489937` | `a82473cb2e4a` | the PCS abort check was asking the wrong question |
| `64833016ae71` | `e338f27675ee` | G96 and --ekf5Robust's help text say what EKF5-CONSIDER found: the mode is rejected |
| `648e659337aa` | `c542be9c2440` | T12 code: --foliageScaleBounds (default off) rebuilds the 2.4 GHz/UWB foliage distribution |
| `64a623381c24` | `ca7632b257d9` | gates: G7 heading now names the population it checks |
| `64da52cbfc81` | `769e36cfed65` | untrack the partial pcs_forms output that git add -A swept into d07acdf |
| `655fac7f224f` | `b5ab98a23770` | PREREG V4-ACQUIRE, registered before any measurement cell (master plan v3 stage V4): parti |
| `65e95da92c2c` | `1fb4745cce02` | N.19: G24 reads each population over three seeds (7, 8, 9) and calls a mechanism zero only |
| `66197c131f60` | `ec5a80f8a41d` | evidence: the IMU bias sensitivity, 20 invocations at e94d04d |
| `66338f26a692` | `25c118a04679` | PREREG ACC-B2, registered before any cell (S2 Part B, F2, exploratory): --subjectPreciseOn |
| `6662df212ae8` | `fe64e7d4e74d` | A7: a gate that reads the sensitivity tables, with six negative fixtures |
| `667dc44d38dd` | `3d890bec4f10` | merge C3 and C4 onto main: identity-scoped query and UWB arbitration |
| `66899b1c9a3f` | `9b6085d28146` | the documents now agree, and the front page no longer asserts withdrawn claims |
| `668e418fec15` | `979cdcb0c98e` | Tool settings directory ignored per clone (#17); the tree names no tool |
| `66c138df9efe` | `ea43a8c1d310` | MAC sect 2: --toaTable -- the time-on-air table the spec asks to be reported (each message |
| `672ecebf2700` | `fe40ad4784b3` | pre-register EKF5-CONSIDER: --ekf5Robust=8 on the preset, 3 configs x 3 bias levels x 20 s |
| `679c7339f1ca` | `8adedbf9a07d` | HANDOFF_2026_10_08 progress: S3 complete except merging -- TOF-FIELD results in 9e7986e9;  |
| `67da4ef0e467` | `50b3860e6c4a` | PREREG FUSE-01: the active tier fused with the floor (TECH_DEBT N.14), on the preset -- -- |
| `681e90c886d1` | `818b301b84a2` | PREREG ACC-A, registered before any cell (S2 Part A, exploratory grade): --imuQuScale at 1 |
| `683416334b12` | `033a82fee7ea` | evidence: baseline_v7 -- regenerate under the corrected engine, to new paths |
| `689aa497c8e3` | `069c5ce94439` | Merge branch 'wip/c1-handshake' into main |
| `68aaef17aeee` | `9acf877f4a3d` | CAL-03 results: at the spec's three gateways the fit keeps the trace term, weights (2.995, |
| `68c69967193e` | `695273d1f2cb` | the switching-point integrity check, with two independent witnesses |
| `68f1c588f048` | `217b889b2280` | v9 gate references, generated at the unified environment on a clean tree |
| `693088a23954` | `cdac7638983f` | C1_NOGATE read-out: the witness lowers energy 20/20 with the evidence gate off; accuracy n |
| `694be3fb53f7` | `49afcc6b420a` | Citation audit (G10): allow the three files report/S2C_CROSS_BRANCH_AUDIT.md names on unme |
| `69ca0051f207` | `274810e532aa` | Merge remote-tracking branch 'origin/work/2026-10-18-ekf5-on-main' into work/2026-10-18-ma |
| `6b61432ec4e1` | `9b2d6c668fbd` | V8-REGEN results (registered at 7a4c4fd9; exploratory): 6 of 9 held -- FULL beats BASE in  |
| `6b7b64308931` | `7a4c4fd9db0a` | PREREG V8-REGEN, registered before any measurement cell (master plan v3 stage V8): BASE /  |
| `6bd216a4aad3` | `40a9b24ba502` | T1, T4, T5: RULES.md §6 states the invariant that holds (K.3); stale counts replaced by th |
| `6bfac76e766d` | `df1157a9de1e` | Network Layer 2/3: the routing table, an ETX baseline, and the confidence metric |
| `6bfeefc07ac7` | `6e92c39e5c9b` | P6 (FUSE-01's registered rule): the preset takes --fuseFloorActive -- a short active tier  |
| `6c0c3194c3b9` | `624848c34a2d` | Part B, F2: --subjectPreciseOnly -- a dual-base subject with two or more ranges on 2.4 GHz |
| `6c2ce5152de1` | `adcc5e148ee1` | C3 result: the inertial prior, measured -- and why it fails on long outages |
| `6c5e95107204` | `9b47f5b980ea` | P6 (M.4): the preset takes --digestVizOutputs -- the visualiser files --specProfile writes |
| `6c77443ea32f` | `4251e8114250` | v11 gate references: the summary schema grew nine baseline columns |
| `6c9595eed54a` | `3172b7b62f31` | G95: --imuFilter=ekf5 is wired, learns, and is off by default -- with nine negative fixtur |
| `6cd740c6721c` | `2762d29a668d` | TECH_DEBT N.35 (S2 Part B, B2): under the dual base tag 0 is offered 204 relay legs per en |
| `6ee6e9e1de7b` | `75e41cd23965` | G89: the four N.26 fixes, each off and on -- RUNS, OFF, STATIC, DUAL, CROSS, CHI, GATE all |
| `6f1b4ab7f8c5` | `73c3cb6367b3` | the continuation form is not mechanically resolvable, and here is the counterexample |
| `6f600b666416` | `57eec89185ec` | confirm the final gate state at 16bb612: 12/15, three explained reds |
| `6f68d7a77bec` | `e203d7955d9a` | two-ray is verified, not refuted -- the defect was in my implementation |
| `6f8e94c3cef1` | `6b01358dc1ce` | Repository hygiene: a truncated script, 37 hardcoded home paths, a wrong cd, stale gate co |
| `6fa3b2e3ae28` | `5205df3f66c6` | WORKSTREAMS_2026_10_09: the three parallel streams (spec/preset and CAMP-FINAL-2; the accu |
| `70360bc68751` | `7a6766c264ad` | FUSE-02 results: the narrowed fuse raises availability (+0.105 at 5 km, +0.127 in the endg |
| `7053eba04b59` | `afd5982e1241` | register Claims 1 and 2b from the mechanism, before looking at the columns |
| `722515ea3a78` | `db0e73d6f25d` | Merge work/2026-10-18-ekf5-consider into latest main: the 5-state IMU filter line |
| `7262296b45f7` | `27e3f1353028` | scope every pre-correction document rather than rewriting it |
| `729375f9eb65` | `f38e766de36e` | track both reports -- two tier-1 deliverables were not in the repository |
| `72c309b0d9f0` | `5c9ab9adfe2d` | ACC-A-OPPOINT results (registered, 94eac1ab; exploratory): fixed_threshold alone on ACC-A' |
| `72ee8969528f` | `c108a32f3f14` | V4 --acquireParticles (default off): particle-filter acquisition before the EKF takes over |
| `7377ed034de4` | `c4a808ebe9ec` | tier 1 now localises by TDoA from the stamps its bill always described: 1428 of 1433 tier- |
| `73b5bd3129ca` | `23557a326a5b` | PREREG LORA-CMP-2: the LoRa comparators on both LoRa bands -- dual-band LoRa fusion (lora_ |
| `73e18879f562` | `3efe88a9e2f2` | PREREG CORRW, registered before any cell: --rssiCorrInflate judged on accuracy (owner's ac |
| `75179f4f5527` | `e51c50965073` | TA-1b: every finder can run its own search -- --finderSessions gives each finder a session |
| `759fe4815a6c` | `5ba107e38fad` | MS-2: the gateway shapes GEO-01 compares can be built -- --gwLayout=perimeter/cluster/star |
| `762a4a90867b` | `f988274e9011` | REAL-BEST results (out of sample, seeds 21-40): the realistic-world stack holds -- P1 endg |
| `76d336037089` | `184b46b86188` | B3 S1/S2, B2 and B1, all default off (master plan v2): --rssiRangeFromShadowing (868 MHz r |
| `77127e97531c` | `1084a0e9e738` | N.21: subjects ranged finders against their true positions -- --subjectFinderPseudoAnchor  |
| `779935aaac03` | `f931a381302e` | results: refresh the causal/det_a/det_b sidecars again |
| `77a889a8bf98` | `e0feb8160c9e` | register K6 before running it: is there a deployment large enough that no single band cove |
| `77b8e9b2d020` | `5f752cdbb0d5` | evidence: c9_escalate, p10_grid and p10_reliability, with their runners |
| `77dd45ee8f9d` | `479969f843f6` | results: ship every arm_summary and run_id sidecar, no traces |
| `789349cad53e` | `cae40443b7e2` | the register of what this session did not do, and why each |
| `78990153bd4f` | `7d923570261f` | G12 is red because of this session commit trailers, and that is a policy conflict |
| `78c913fd0c48` | `36558b60b8e9` | Row 55 / MS-1 step 2: --subjectNlosClassifier -- every measured subject scores its own lin |
| `78edc67f3838` | `e2d37f7e6b60` | pre-register the bidirectional digest before running it |
| `798e543fb93a` | `f6caa3a2ea09` | MAC sect 2 / Transport sect 7: --reportAirtime -- airtime per tier and message type from e |
| `79eb7c5e2c24` | `527a241022b7` | Merge work/2026-10-09-local-verification into main |
| `7a36b63c6a11` | `ecc4e52514a9` | HANDOFF_2026_09_29: Bar 1 met, Bar 2 partial, Bar 3 regenerated -- four claims stand, PCS  |
| `7ac8a32b2a53` | `db2b42b5bb8a` | amend Claim 4's registration: one arm, because five of six change nothing |
| `7af04f59d871` | `bba10831bbb9` | PREREG CAL-02: PCS weights by FIT-R on the preset (fit seeds 1-20, held-out 21-40) and per |
| `7b17bd604d3d` | `a29035489860` | B3 registered before it runs: seven claim-bearing campaigns on one commit, and their drive |
| `7b50ab9d1118` | `6fb494b92254` | Transport and Application layers: session lifecycle, result provenance, batch search |
| `7bb6aaa9f4ac` | `605fb17ce8a8` | P6: --specProfile, the spec preset, and gate G24 -- 68 settings applied unless given expli |
| `7bdc57633dd2` | `564bfe6ef9a4` | C1 verified: the handshake, once it contends, costs 41 percent more collisions |
| `7c006121be8d` | `b786e2c187d5` | IMU: moment-matched coasting, so the filter's covariance stays honest through long outages |
| `7c2cf44068ba` | `0cc20d970440` | docs/SCENARIO_FLAGS.md regenerated (tools/regen_flags.py; 286 flags): --imuStrideSigmaPct, |
| `7c36084e4a9d` | `05b48936f59a` | G12b: gate file contents, not only commits -- and the fixture caught it passing blind |
| `7c3921c87db5` | `e7ff5b02540f` | Merge S3 (review/2026-10-08-evidence-fixes, 9cb1e29d) into the integration candidate: tier |
| `7cbfeadc35a8` | `d199791f2d65` | C2-full rebased onto current main, and the divergence finally quantified |
| `7d4ebeae60e6` | `507a8b50835e` | MASTER_PLAN 2.1: twelve rows brought up to this week's code -- TDoA, the CIR classifier an |
| `7d8ad4f32e92` | `9152457806a5` | the PCS form campaign runner, with the registered abort check wired in |
| `7daa5e66f7e0` | `8eeea832b6d2` | V4-ACQUIRE results (registered at b5ab98a2; exploratory): V4-3, V4-5 held; V4-1, V4-4, V4- |
| `7dc74b137728` | `d6ec5c506a17` | ACC-A2 results: the error-state filter (registered, 3a558fb6; exploratory) |
| `7dc8ba018035` | `c7c34448fbc0` | CAL-05 as a curve: the integrity check is a working detector |
| `7dc9f02a5162` | `9f111f35b133` | G12 is red because of this session commit trailers, and that is a policy conflict |
| `7e5164a63755` | `aaa1b2ac8807` | MS-3: the P5 UWB-reach checkpoint fails -- the preset (5 km, evidence gate on) delivers no |
| `7e91d73f726d` | `9ca658824c4a` | docs: cross-document consistency pass, and withdraw a wrong defect claim |
| `7eabe44680dc` | `ba8889bef227` | TECH_DEBT O.6 (B12 remaining reds classified; G4 re-checked: anchors in view, delivery-lim |
| `7ec0d919dd11` | `b0ffe56fc27b` | the evidence gate defaults on, because it is worth 324 J and costs nothing measurable |
| `7f0125818e79` | `799ad2ff8f26` | C3: an IMU error model, checked against closed forms by code it shares nothing with |
| `7f5bd595509e` | `baa0f25d6cac` | price the repair -- and the per-cycle cost is not a constant, established before predictin |
| `7fbf648f12d7` | `792459eb199a` | redact one path segment in six report/b8 sidecars, and disclose it |
| `801b1a79dd2a` | `1e4cc96c0f43` | record the 2026-09-05 attribution rewrite, and update the plan it changed |
| `808885a12bf3` | `33f6bb0caf1c` | record the SHS gait-calibration divergence as debt, with the measurement that forced it |
| `80b228823762` | `6ce29066bec1` | HANDOFF_2026_10_09_S2C: the remaining work as a self-contained section for the next sessio |
| `8103c09dd971` | `8a46faf28f02` | rebuild the tier-2 evidence gate as a predictor, because K6 met the precondition for it |
| `8190ef1ed296` | `b9ed29bbae32` | PREREG V2A2-NLOS3GPP, registered before any measurement cell (master plan v3 stage V2a-2): |
| `81fb1485a016` | `647d09c63df0` | Part A, A1: --imuQuScale multiplies the IMU increment covariance handed to every filter (t |
| `82eb33c7b59e` | `74fe6db2c242` | recount 5.1: the heading said 106 and the answer is 137 |
| `83191b15e570` | `337fbd6fb6d9` | G98: --acquireHandoverCurvature is reached (held 8 cycles at 5 km seed 5), changes the run |
| `8364e969ac9e` | `5709e13cd367` | C4: the blocker is the PHY provider, which models exactly one field node |
| `8396f3b2dbbe` | `ec6827b59ca3` | B3f (report/FOLIAGE_DEPTH_DEFECT.md, owner decision D-16): --foliageDepthClamp clamps the  |
| `83bb6e1fe33d` | `3987c94cd5a3` | sect 49: a node that cannot talk, and the honest name for that fault |
| `83f47de9f741` | `8bc1bdec169b` | pre-register C9-ESCALATE before the first cell runs |
| `841be3ca192d` | `0544f60face8` | B3 results: the 125 regenerated cells, all complete, all on a290354 dirty 0 |
| `8461d5de7aa6` | `f53a91c08197` | evidence: the A6 hysteresis sweep and the A1 dwell sweep, both complete |
| `84f8a93c8f97` | `87658bb6445e` | pre-register A1, re-specified: coverage fraction against dwell, scored by A0 v2 |
| `85b7a0da01d4` | `165fb9585f54` | pre-register REAL-SPEC: the Switching-Point Integrity Check at its specified chi^2_0.99 =  |
| `86e376cf45ff` | `9e0a476a444b` | --imuFilter=ekf5: the 5-state filter as every node's policy filter, on DC-1's own output |
| `86e43f20aeb5` | `c19970759c42` | every ranged link now carries the spec's NLOS score and PCS term 3 can read it, but the CI |
| `87413fa80b6c` | `68ff4e30f9cd` | PREREG DEBT-A amendment 1, before the amended cells run: EDGEOFF and ARBOFF were refused a |
| `87692a230d65` | `1cb32e396647` | TECH_DEBT N.57: O.12's 868 MHz short-read at range is selection; a censoring correction pr |
| `87f77a06660b` | `866ec4fa08d6` | record P12 and P16 as analytic, so their ROBUST class is not overread |
| `88769554845d` | `1d8a18b8e5b3` | Final suite on efac9d12: 97 PASS, the five known reds, and five stale preset expectations  |
| `889f7df8ac6c` | `ea0a33e8a151` | reconcile the rewritten history with the published one |
| `88a3f910c919` | `0108e04851d3` | correct the attribution, and put the analysis where a citation can resolve it |
| `88b0a1018a67` | `b33b4897102f` | Plan A, A2: --imuErrorStates, the error-state IMU filter (psi, b, s as states); gate G101 |
| `88d0e493e22c` | `86d39bd7694e` | G12: stop matching co-authored-by in message bodies, and close a hole beside it |
| `89108ac191cb` | `b9fe0dc70848` | analysis: localise the residual to two-ray; refute the Markov hypothesis |
| `89118bae81cd` | `40992d6cf8cb` | HANDOFF_2026_10_08 progress: items 2, 3 and 7 committed, TOF-FIELD registered (adfa3bdd) a |
| `8943b2190ae9` | `1f29c5c8b591` | Merge pull request #11 from YKushal-hub/work/2026-10-17-gate-repairs |
| `894caf561b4a` | `5a065b015918` | the section 4.1 reliability result has no sidecar in the tree |
| `897dbe68a4ed` | `c23ee8503dce` | rule 8 gains the cross-binary case, and the tier-1 CRLB entry is closed |
| `8ac1c31ea615` | `b7788c9e3f80` | invert G9: every .md must be tracked, not a hardcoded list must be |
| `8b7be35929ca` | `3b9002ee07f0` | fix: validate.py computes the link budget it validates; G11 wired in red |
| `8bd47c283ff1` | `af726afc9294` | G16: a detector for the class geometry caught by luck -- and the adjacent link was content |
| `8be693261036` | `b297aed1834c` | the sweep radius was bounded by the subject's speed, so a still subject got a 1 m search |
| `8bfd04b2bad0` | `cf1ddff1c9d4` | Row 55 / MS-1 step 2: --subjectTier1Tdoa -- every measured subject runs tier-1 TDoA; each  |
| `8c2a2237f1a1` | `8d8659dc8663` | merge work/2026-09-27-spec-b1 into the session branch; the merged tree equals that branch' |
| `8cb688815c32` | `992651a38f9e` | TECH_DEBT N.15: the integrity check's misfiring is a misspecified distance -- the preset's |
| `8d0043a84f58` | `9c30f743cc9f` | census the loss sum instead of guessing the next term |
| `8d1f07ccba94` | `3d543ffecacf` | V2a --nlosRangeBias and V2b --rangeErrorNoLinkLeak, --probeTerms (all default off); gates  |
| `8d441fe494e5` | `96fa65731c54` | PREREG S2-C, registered before any cell (exploratory): unknown start (U, seeds 1-20, A and |
| `8d870aa40bda` | `2d4b6e7daa9f` | A5: complementary tier-3 siting, behind --sitingMode, default unchanged |
| `8dc4048e2d6e` | `4cc7bef8ef6d` | runners: the registered queue at one commit, and the cited-evidence reconstruction |
| `8f46378b7c1b` | `886502d6f892` | N.21 (tag 0's half): --finderPseudoAnchorStrict -- no finder range uses the finder's true  |
| `8f90fd2c08ee` | `cf21abbb6174` | CaTblImuEkf: robust update modes behind SetRobust(), default 0 (N.33) |
| `8fc91dc90110` | `facf24b7fac7` | pre-register REAL-STACK: the realistic world (0.15 deg/s gyro, 5 % stride noise, 2 m finde |
| `90a1fc7da52c` | `4478218fe967` | register h = 1 at 40 seeds, and the winner's curse that makes 40 optimistic |
| `90cc829b2d9a` | `7c5eeabb1ad0` | G18: every results/ path a tracked document cites is held -- red, 14 paths |
| `91eadfcd6f47` | `f9d18738c608` | T18 (J.1/G11) diagnosed: 2.4 GHz path loss agrees within 0.04 dB; the growing gap is the e |
| `92152dc78e2a` | `2d02a9248948` | pre-register REAL-IMU: the final preset with a realistic IMU (0.15 deg/s gyro, 5 % stride  |
| `92bc8427abcd` | `d496ce15a398` | LORA-CMP: lora_cmp_table.csv in LF as the repository stores it (eol=lf); RUN.md's hash is  |
| `931a5a4ee25c` | `e96be2b5b926` | figures: figure 1 as four per-tier panels, r80 computed from the probe sweep |
| `93695ac2f7bc` | `e52606889e2d` | Merge pull request #10 from YKushal-hub/integrate/local-line-2026-10-16 |
| `937a55a638b1` | `f18128574d74` | a finder is a pseudo-anchor, not a perfect one -- built, and it does not yet fire |
| `937e87351195` | `5d93de06bd63` | --imuMagSigmaDeg: a magnetometer heading reference for DC-1, fused by ekf5 -- default 0, g |
| `93f68e600ded` | `2fc36ffe8648` | HANDOFF_2026_10_09_S2_CLOUD: this stream's state, results, overlaps with acc-part-a/b, req |
| `944d85712559` | `9c7f5d4aace5` | N.22: --collisionBothLost -- of two overlapping frames both are judged by the capture rule |
| `946f02ba3876` | `f73462f3b033` | SHA_MAP: the trailer-bearing copy of the local line is deleted, on the owner's decision |
| `949f18861f38` | `bf685ff8cd11` | the local line's IMU sensor model and 5-state filter, ported as a pure addition -- G91, G9 |
| `94a200ca11cc` | `79c90497c942` | PREREG_ACC_B_HYBRID: register the N.35 + N.38 (k = 3) evaluation on seeds 1-20 before its  |
| `95b793516d78` | `a3f54d15a9e7` | PREREG V6-ONDEMAND, registered before any measurement cell (master plan v3 stage V6): alwa |
| `974bffcd7a29` | `5a9154edc1fd` | N.31: G63, G87 and G89 are red on main itself, since the preset took the N.26 fixes |
| `976a36afcf23` | `4fead4f8f66b` | G12b was failing on its own fixture, and the r80 conclusion reverses a second time |
| `976bbefaa369` | `4db72958792a` | fix: record the blast radius of a4e23ad (D1 SF index) |
| `97cb1878984d` | `e07dce712ad7` | DC-11: the sparse cases can be solved inside Step(), after the prediction, as Physical 2.4 |
| `97d9844f7bba` | `747ed826748e` | a per-frame event log and the console that replays it |
| `9805817187b6` | `45a6ad2c72b0` | register h = 1 at 40 seeds, and the winner's curse that makes 40 optimistic |
| `9818ed8baeeb` | `1ccadde1a83f` | P7: the documentation sweep -- the register reviewed, MASTER_PLAN 2.1 updated and recounte |
| `995eeb504eb2` | `90dedb248f50` | M.7: CaTblCalibrateLoocv fits its control with an intercept -- the term-1-only control was |
| `99853730564d` | `1f22d174fd94` | --ekf5StateBounds: the 5-state filter's gyro bias and stride scale are projected onto +/-  |
| `999f6763c4e3` | `5449dd6d98fa` | N.26 fixes 1-2, the policy's inputs: under --loraDualBase a 1-2 range active set that join |
| `99b2a50f6aa1` | `ac7de6036738` | register PCS as debt: uncalibrated, and measured to re-encode a number we get for free |
| `99d4caaebde5` | `4eff6ab29b88` | census: the remainder of 9c30f74's message, truncated by a quoting fault |
| `99fdf1a4732e` | `483ac805970b` | the topology was wrong: every node carries all three radios, and finders exist |
| `9a0333c58638` | `e7cc28119c6c` | UPG-SCOPE results (U4, U5, U6 held; U1, U2, U3 not) and DESIGN_DECISIONS 13: the preset ad |
| `9a7e23b0bb18` | `2870b2c91822` | K9: the mesh is the story, and the tier switching is not. F1 fires. |
| `9a9ec3c6a89f` | `ff9cd4ef0d1a` | a ridge of gateways and a walker who leaves the trail |
| `9ae6b3d92c15` | `f7f927db2f8d` | FIXES-N26 results: with all four N.26 fixes CA-TBL beats Poucet (availability +0.047 / +0. |
| `9aebbba04251` | `76016c905885` | P6 (N.21; MS-1's finders as pseudo-anchors, for the subjects too): the preset takes --subj |
| `9b26515f04c9` | `d683cbd81c11` | the register of what this session did not do, and why each |
| `9b8850ac327a` | `1dec0e9cd250` | expose the EKF process noise, so K2's second hypothesis can be tested |
| `9c162fd604b2` | `b5083da58e21` | EKF5-CONSIDER results: Schmidt updates for one-range fixes make the 5-state filter worse - |
| `9c57c2b8ae19` | `ff70a94647b0` | debt register: the state after the topology correction, and the one blocker first |
| `9c619f0c166d` | `d56f7d293a88` | G12, fourth time: a trailer sits at column 0, and a quoted example does not |
| `9c7140b0d149` | `4cefe555cc06` | widen G16: its population excluded the contention it exists to catch |
| `9ce8192471dd` | `0d811a9253ee` | N.12: --fixRouting with measured subjects and without --fixRoutingSubjects is refused, whe |
| `9d140835f1f3` | `47337fa7475a` | RULES.md: the IMU-line merge's suite -- every gate PASS but the five known reds (G6, G11,  |
| `9d3136f0b7bf` | `6d85b070a979` | session record: five work items, five campaigns, six errors |
| `9da4bd71b4ab` | `3a558fb698ae` | PREREG ACC-A2: the error-state filter (q_b = 0, picked on seeds 21-30 by the plan's rule)  |
| `9db40afd7a0c` | `f7440fa077f4` | V5 --c2bEscalation (default off): escalate when the next tier is observed reachable, never |
| `9e5a5599ed0a` | `b5f93cea17d9` | A5: complementary siting forfeits tier 3, and co-location is now vindicated |
| `9e6b5c7e04cc` | `8186955ed557` | C5: a relay is now a measured subject, so it has an x_hat and a P_B to report |
| `9f23638197da` | `8c6bed9e924f` | EXCLUDED.md: review CA_TBL_HANDOFF_v6.md as an intentional exclusion |
| `9f2aff65b938` | `2f03360b8161` | evidence: Claim 9 re-measured on citeable provenance -- falsification reproduces |
| `9f34ccefc9a1` | `0ca63f2b6c39` | MC-1: the escalation handshake can run as the spec's MAC sect 8 says -- --handshakeFixes:  |
| `9f514bea6401` | `302edc29139d` | CAMP-FINAL2 results (scope D1): the headline tables on the final preset, 221 cells; Q3, Q4 |
| `9fce49cc6690` | `7549cb487d27` | B3 read-out: C5's every-arm check now shows every arm |
| `a025bbc55aa8` | `27c0f563da4c` | Merge remote-tracking branch 'origin/work/2026-10-18-ekf5-consider' into work/2026-10-18-e |
| `a0592039051c` | `4c982c4f3a95` | phase 10 amendment: split claim 1, derive (A,R) and the power floor |
| `a0829213bd86` | `7caf3298d68c` | Merge pull request #6 from YKushal-hub/ccr-4ce80826-a4ulj4 |
| `a129111509a5` | `16ac1f89b65e` | ACC-A results (registered, 818b301b; exploratory): --imuQuScale=10 cuts ca_tbl_full's endg |
| `a12a630e051d` | `6066b3b12653` | labels that said false things: two sensitivity captions, the D1 comment, verifier warnings |
| `a144a76565eb` | `07c1014e2309` | G99: --imuMagSigmaDeg is fused on every IMU cycle (720/720) and bounds the heading where t |
| `a1bd7292cb3f` | `da41fc7bd78f` | TECH_DEBT M: eleven things found this week, two of them waiting on the owner -- relays tha |
| `a1c2074c583c` | `e734bbb27938` | G77 runs on N8_ring, the geometry it was built on: at the preset's three gateways seed 7 h |
| `a1da02013469` | `15490886f518` | N.38 --caseDInfoRatio: refuse a Case D range far less informative than the prior |
| `a1f6e76cd322` | `0485cb1eaa44` | A0 returns FAIL in 64 of 64, and the reason is degenerate |
| `a2c32f5245b4` | `3c303e34b590` | results: refresh the tracked sidecars for causal, det_a and det_b |
| `a2d9586564e6` | `b56520a95287` | the robustness classifier gets a negative fixture; six rows relabelled |
| `a39696f37f2f` | `61960de62b53` | MS-1: every field node can be a measured subject -- --measuredSubjects=N runs the core of  |
| `a3b468e8d2ad` | `4581b479c252` | the fault injector never tests the case its comment documents |
| `a3f3665c3967` | `f9e132e95b6f` | DC-10: table entries can carry the producer's velocity, as the spec's entry does -- the EK |
| `a3f69c25ab55` | `351753f92270` | docs: restore the rewritten technical report to main; correct 13/13 in all four places |
| `a4177c15bf0c` | `3b0a52d5faf8` | K4: the small-box regime measured, and the gap to one 2.4 GHz radio does not close |
| `a4ae196b2ade` | `59df9114fe63` | Spec conformance, recorded before it is built: the docx audited row by row (57 rows, 16 co |
| `a52bd45066fe` | `2113885843e1` | the console template could not read what the packer writes |
| `a576020aec3c` | `6dbbf07c6d72` | verify: a missing sidecar must demand the mode, not assume it |
| `a59c29d86a16` | `53c5cc7478f9` | withdraw sections 4.1 and 4.2: the directory they cite is not in this repository |
| `a5c58c85b3c7` | `536b407a82e1` | G12: spell the vendor-identity pattern as a character class, same matches |
| `a5dfeb97820b` | `a6097f2a5c80` | CAL-02 results: at 5 km the trace ranks error backwards, so FIT-R drops it (weights 0, 1.9 |
| `a62902d10ddd` | `f3b5a8b42833` | PREREG B2CONS, registered before any cell: Q30 (--imuQuScale=30) and CORR (--rssiCorrInfla |
| `a637204ade04` | `616d3c8225a6` | the grid holds the no-PASS prediction, and the effect size tracks dwell |
| `a64c3a1e026a` | `97ea91dea62c` | DC-8: the integrity check can read its eligibility from the NLOS score, as the spec says - |
| `a6b2414d2acb` | `f450a797eee8` | S2-C engine, default off: --unknownStart seeds every field-node filter (tag 0, subjects, r |
| `a6b6e9ea9319` | `3f391a0c2a55` | P6 (FUSE-01b's registered rule): the preset drops --fuseFloorActive -- the fuse is no long |
| `a6cb593b1dda` | `d39c04143a95` | NW-4 + NW-5: an URGENT query can escalate its target, and UWB arbitration can follow Netwo |
| `a6ea951efb9f` | `5b4235eaaf99` | ACC-B3 citable re-run: results/acc_b3_2026-10-09 (160 runs, source 5ce8a013 dirty 0) |
| `a7158ae5215e` | `3a5985acd75a` | sect 7: two anchors are not nothing -- Case A/B/C, and the refusals that make them safe |
| `a718bf5f4342` | `efb77a36e0e7` | T8 (4.1/4.2) diagnosed: a portable (non-native) build of the same commit leaves every filt |
| `a7308bcf8605` | `027f23e8c3b6` | register: C4 closes, C3 re-evidenced, and the trace divergence is now a number |
| `a7491d114062` | `dea1497bda7d` | withdraw the 0.16% agreement -- a retraction propagates along data, not argument |
| `a78015a6e4eb` | `465750b95dd8` | V3-CONSIDER results (registered at 103d4801; exploratory): V3-1, V3-5, V3-6 held; V3-2, V3 |
| `a78de51411fa` | `fb57ec95a147` | repo: vendor src/lorawan, invert two ignore traps, assert the inverse |
| `a7d8e90e1ce6` | `149fa7fb413e` | PREREG CAL-03: the PCS weights and thresholds re-fitted on the finished 97-setting preset  |
| `a80b202feaf8` | `103d48019d0e` | PREREG V3-CONSIDER, registered before any measurement cell (master plan v3 stage V3): cons |
| `a89aed8d55ef` | `1502e04ce557` | Merge pull request #14 from YKushal-hub/integrate/2026-10-18-kushaal-kn |
| `a8ab3bd94429` | `240255d1089a` | G4/G5 green under DD 16; G28 and G30 pin --boxHalf=1000 (their populations were defined at |
| `a93a2aaca6ca` | `db0cdb15c60c` | GEO-01 stage 2: track every cell's arm_summary.csv and run_id.txt, as the results policy i |
| `a9f0f74f0226` | `87665ff5d910` | pre-register A5 before running it |
| `aa088643638c` | `d24d87e5a80e` | Merge work/2026-10-16-switching into main: the S2C unknown-start/NLOS line, ACC part A, th |
| `aa18f48b4e4a` | `218ae91258be` | lorawan: Add backpack-mounted IMU Step-and-Heading System |
| `aa538bb813d8` | `b0e6e535a8f8` | --imuStrideSigmaPct: per-step stride-length noise in DC-1, default 0 -- a pedestrian IMU,  |
| `aa6ebbb590b5` | `59acc560cd05` | NW-3: each tier's identity-query driver can send on its own radio -- --queryTierBinding=pe |
| `aaa875e0a620` | `aa205e3915e8` | chore: publication gitignore, and close the scratch new-file trap |
| `ab38a808f346` | `7801ac10030b` | the handshake contention effect is 33.7 percent, not the 41 percent one seed showed |
| `abbe3de7ef7f` | `a91d75d2f725` | merge C2-full: per-tag state, so a second target can be a measured subject |
| `abf81e3ba1ad` | `1e025efbf77c` | Docs follow DD 15 and CORRW (RULES.md current figures, Technical Report §0a) |
| `ac5766f57730` | `a4c3e327f78d` | sect 7 Case B reaches the fix path, and all four cases now have a regime that produces the |
| `ace53dd356c2` | `44af83e9353a` | fix: restore the run_id source-hash provenance writer and its verifier gate |
| `adebc714ba85` | `0c8708c3fecf` | attribute the debt: four of the five defects that mattered today were not in the register |
| `ae1faf53cad0` | `9182d14a6f30` | baseline_v8: the reference for the corrected topology |
| `ae5f933b4ccc` | `9056e06142bb` | T15 (N.30 main line, G92): 13 lost cited results paths recorded in RESULTS_NONREGENERABLE. |
| `ae6d84b82629` | `d1d5b2d46501` | gates: an empty population is VACUOUS, not a pass |
| `af492ca7d743` | `e9b85186b4c9` | Spec P1, no behaviour moved: three policy counters initialised, two wrong comments and fiv |
| `af609bf7e939` | `ee285a003520` | the integrity check holds across eight seeds, on both axes at once |
| `af80c081c92d` | `9268c847d5e6` | merge Bar-3: C3, C5, C6 and C7 reproduce, C1 and the PCS repair do not, and with the evide |
| `af89ffa88a99` | `a5b8b331c04c` | Merge 6b01358dc1ce755e1a3799803c775633c173f223 into e160dde771d36b2a87cace1badb0c0681e1128 |
| `afa46d41b70b` | `001b6b78fd90` | ACC-B results (registered, 7717e744; exploratory): --dualBaseRelayTier lifts tag 0's endga |
| `afb8862bce4b` | `89c22d7098c1` | PREREG FIXES-N26: the four N.26 fixes on the preset, singly and together -- phase 1 fits t |
| `afcae9112f75` | `45cd1a93d3a9` | debt register: what closed today, and the question the energy win exposes |
| `aff5b7aab489` | `ea65ae364d01` | C5: relayed-range admission and covariance inflation, with G15 and five fixtures |
| `b00e1f402203` | `00d85c425750` | N.37 --hybridNoLone868: step a lone 868 MHz cycle as empty on the hybrid arms (measured, n |
| `b01960dd80b3` | `a5f11b71dc70` | the claim record, restated on the regenerated evidence: four claims stand, PCS and tiering |
| `b114e0e5ee8b` | `177c28f9169e` | scripts/lora_cmp_table_image.py: renders LORA-CMP-2's paired-difference table as HTML (PNG |
| `b1473abc9630` | `a8f213378bec` | A4: tier-1 sigma is over-stated by ~1.7x, and the baseline does not reconcile |
| `b16531f127e1` | `769b73a6eb58` | register L.2.5 before running it: the proxy now records how old every entry it is asked ab |
| `b199ba590f38` | `98cef5b5d6e8` | C1_NOGATE results: the 40 cells, all complete, all on bd20cf8 dirty 0 |
| `b218e608e93e` | `b057bc67567c` | TECH_DEBT N.61: the magnetometer disturbance as an estimated state is consistent (theta NE |
| `b26d0c85d23a` | `0e8084a871df` | pre-register EKF5-MAIN: the 5-state filter on the final preset, cv vs ekf5 x preset gyro v |
| `b2807f3d40f3` | `dd55cee844c4` | Row 52 / N.10: --requestOverMesh -- a requester's escalation request reaches a finder arbi |
| `b34377738a97` | `22962d5de297` | M.9: --reportRecedingPushes -- L.1.5's RECEDING eager push is counted; in G75's run tag 0  |
| `b3663ff58ab5` | `28dccd44cac4` | Row 55 / Physical 2.6: --subjectIntegrityWitness -- every measured subject's escalation is |
| `b3689487567c` | `f253aa2533e4` | pre-register C9-ESCALATE before the first cell runs |
| `b390ed6531e0` | `f8fba24f53f8` | relays can now chain to depth 2, and the cycle's one shared instant is why none ever does |
| `b3e8f9429f02` | `7222257f8d40` | merge A5: complementary tier-3 siting, and the negative result it produced |
| `b440a4e213b0` | `db8bb2d48570` | Row 55 / N.3: subjects can run the battery fault -- --subjectBatteryDeplete silences a sub |
| `b450b3bf9a6c` | `e73b6548af1a` | Docs: dated corrections where the code default was described as the preset (hopCount, rela |
| `b47996fdb777` | `8cf9334f557b` | Technical Report §0a: energy ratio range corrected (1.2-2.9x) |
| `b49df4674e88` | `f4036e06f92a` | phase 10: pre-register the testability predicate, requirement and metrics |
| `b4d18b86af21` | `80c95a88ae4e` | record what results/ regenerates before it is deleted -- 2004 of 3277, and D1 is why |
| `b55db5774d7d` | `fd73f83aa8a9` | D-1 repaired: the system works better, the confidence claim does not survive |
| `b62340c281c8` | `bb2df9aa8592` | PREREG UPG-SCOPE, registered before any measurement cell: the scope-2026-10-17 upgrades (C |
| `b6c98f815d4b` | `77d3ad2b9005` | audit: describe the G12b hits without reproducing the name G12b scans for |
| `b6e3dca3dddd` | `82da6df68ab0` | CaTblImuEkf gains the rest of the interface CaTblPolicy uses on its filter |
| `b6e6567f0de7` | `a30d3119d328` | Comparison table: --arms=poucet_fusion -- the spec's Poucet-style fusion baseline: every r |
| `b6eab9f26c2b` | `19d6b72c72f6` | N.6: --halfDuplex -- every radio half-duplex; count-only mode (1) finds 56 frames started  |
| `b763b32da930` | `2555fa6c1c78` | SY-5: the evaluation's missing fault classes exist -- --injectTxPowerDropDb (a cold-sagged |
| `b78e48216848` | `7337dad1b0a7` | D-8: --t2SensitivityRowKHz, tier 2 on its own bandwidth's SX1280 row; SY-3's shift not sta |
| `b828add07513` | `a94bf3a2fa58` | The finder-consider gate is G150, not G100: main already had a G100 (--imuQuScale), and bo |
| `b8c351a9f1df` | `81b28d9759f0` | B2 --rssiCorrInflate (default off): 868 MHz RSSI range variance x the AR(1) long-run facto |
| `b909458e56a5` | `be677d03910b` | close Region 3 as a gap, add the independent CRLB reason, register the OFAT campaign |
| `b9459c23a418` | `2d99463f94c4` | fix: track Simulation.md, untrack the third report, and gate both directions |
| `b973f7e0ee42` | `ac12a1a5f12b` | ca_tbl_imu_validator --robustDiag: N.33's synthetic robustness study (diagnostic, not a ga |
| `b9b1451e6e83` | `9f586f4b4c22` | docs: stop the front pages contradicting the ledger; record C3, D-8 and the plan |
| `ba2c00332760` | `c3913907c91a` | register K8 before running it: does the architecture pay in a search? |
| `bb0e43f70c9b` | `90d4644d5574` | untrack the partial pcs_forms output that git add -A swept into d07acdf |
| `bb83e4f5b8fc` | `6325d9395ff8` | register the no-PASS prediction before scoring the propagation grid |
| `bbad01c7d212` | `10b399a91e97` | session record: the answer is no, and the two reasons it could not have been yes |
| `bbfc0de863e7` | `c5014553e7cf` | --acquireHandoverCurvature: the acquisition cloud hands over only once every range is line |
| `bc38069a493c` | `7fb29b067980` | PREREG SIG868, registered before any cell: --rssiSigmaLogNormal on the final preset, confi |
| `bc43d10492b8` | `a5575036a6dc` | PREREG FUSE-02: the fuse narrowed to a short floor (--fuseFloorActive with --fuseFloorShor |
| `bc8a430d977d` | `2a32643c0037` | the OFAT campaign: sixteen parameters, each with its own robustness verdict |
| `bc996e4f87be` | `390c89d24aae` | G10 gains its negative fixture, and G92 asks whether cited evidence is held -- FAIL BY FIN |
| `bce252fd3297` | `5cc9b07e93d2` | repair session record section 7: a heredoc reached bash unquoted and ate every backtick |
| `bd88741168a4` | `9d297e9a8d8c` | MS-1 finder tasking: finders can search for specific people -- --finderTasking=roundrobin  |
| `bd9d76bb2127` | `db96b81c1741` | fix: NaN sentinel for undefined mean SNR |
| `be0a962c4ad1` | `a7bb0b4a07c6` | B2CONS report and TECH_DEBT O.12 corrected: the 5 km consistency gap is not acquisition-er |
| `be1bd239def5` | `ad3bf3e9362d` | T3 (M.9): closed as moot -- the RECEDING push lives only in StartHandshake, which only tag |
| `be51a5bcbdda` | `ad55b5762684` | EKF5-PRESET results: a filter that learns the gyro's errors removes most of the preset's c |
| `be9b8f2174c3` | `a2deb1976290` | Preset: Tier 2 = SX1280 as two chips (owner decision 2026-10-16): --radioBasis=sx (SX1262  |
| `bec676b1ac28` | `4651e289122c` | M.11 (owner approved): RULES.md, SETUP_NEW_MACHINE.md and the gate-suite header carry toda |
| `bff861066700` | `6f3ea33ee638` | docs/SCENARIO_FLAGS.md regenerated (tools/regen_flags.py): --ekf5FinderConsider, --ekf5Rel |
| `c019c0aceb8f` | `e304b1d6792d` | register CAL-01 before running it: can any weighting of the three PCS terms rank error bet |
| `c02c050c2aa4` | `32bb3ecc4550` | PREREG C4-P833, registered before any cell: --foliageP833 against the final preset, config |
| `c0313f3f214f` | `9d277b043a6c` | diag: emit unconditional SNR alongside the conditioned accumulator |
| `c053877397e5` | `095291c8e175` | audit 2026-10-15: what the documents report as true that is not -- annotated, not erased |
| `c101cc384a0b` | `9caafb5b7225` | two counters were lying, and the zeros were properties of the counters |
| `c10b85d3180b` | `b0fbe72c08f1` | N.14 / FUSE-02: --fuseFloorShort -- the fuse narrowed to cycles whose floor is short too,  |
| `c1b374bd9a52` | `435f0a6969ef` | LORA-CMP-2 results: M1-M5 held; dual-band LoRa fusion beats CA-TBL on availability in ever |
| `c1ba5ffb22b2` | `790ffab0861e` | DC1-BIAS results: with a realistic gyro, every preset accuracy figure collapses -- DC-1 ne |
| `c1dfb926571e` | `06c9a77ede72` | docs/SCENARIO_FLAGS.md: the scenario had no flag listing, so this week's ten flags and the |
| `c1f00fa8b2b9` | `8c9ec390c193` | V7 --rangeReplyPosition (default off): a finder's ranging reply carries its current positi |
| `c1f83c3df49b` | `179de1403700` | repoint two citations the identity rewrite missed |
| `c1fd61b66698` | `4154a14faf9f` | TECH_DEBT L.1-L.2 status: Bar 1 is met, two of its rows were already met at afa0db1, and t |
| `c236834c4398` | `d6a1572cb635` | the UWB evidence gate asked a sleeping radio, so it could never say yes |
| `c2e66f2068b2` | `5f59ceff6342` | hosted-page links and session wording: no vendor name in the tree |
| `c314bde3b51a` | `476c2227c863` | sect 21 and 23: the table entry gains the three-branch rule and an age it can act on |
| `c432134edb7b` | `1543e3f2e458` | DRAFT, NOT REGISTERED: CAMP-FINAL-2 -- CAMP-FINAL's runner with lora_fusion as an eighth a |
| `c44a0913c402` | `726aab5fa114` | pre-register: the foliage term, its sourced values, and four predictions |
| `c469139c5c8c` | `04f8f6fbc400` | time to first fix: zero censoring everywhere, and the question closes |
| `c527d6a2899e` | `1d13b4d63b6b` | CAMP-FINAL results: the finished preset at three gateways -- P1, P3, P4, P6, P8 held; P2 f |
| `c5566a270d25` | `8de7edd21125` | S2-C engine, default off: --acquireMinAnchors=N, an acquisition rule for a filter seeded w |
| `c5a361fd03b9` | `a15a1658f042` | MS-2 step 5: docs/DESIGN_DECISIONS.md sect 5 records how the spec configuration's gateway  |
| `c5bb233153df` | `1bee67ad87a3` | refresh the conformance matrix from the table, and register the re-run as the largest item |
| `c628d83e2598` | `9ae620af285a` | the local line's evidence, as a record: six campaigns, their results, registrations and sc |
| `c62aa4250e19` | `debc0cc0b3e9` | DC-13: a held entry can be carried forward before it is used, as Physical 2.2 says -- --en |
| `c651743cb48a` | `99a6e6ce244d` | docs: record the provenance gap in the cited evidence base |
| `c6d40eb3b89b` | `0a746a7d63fb` | analysis/accuracy_replay: exploratory range-log patch and open-loop replay scripts behind  |
| `c72af43843af` | `ff7ad1da9617` | B3 read out: four claims reproduce, the integrity cross-check and the PCS repair do not |
| `c79fffd93470` | `baaa8fcbe285` | P6 (owner decision, TECH_DEBT N.11): --hopVarianceTerms leaves the preset -- --specProfile |
| `c82a2a9b2688` | `8aff9322f359` | MAG results: a magnetometer halves the heading failures (10 -> 5 of 40, every arm fewer);  |
| `c855ae4413e5` | `178bef7cdfc2` | C4: the UWB arbiter, ported and running, with its deny path unexercised |
| `c86284217a63` | `7584cf534788` | the relayed-ranging error model, written before any code, as C5 requires |
| `c8758533f978` | `93d8a356f922` | FUSE-01 results: fusing the active tier with the floor raises availability by 0.118 at 5 k |
| `c8df8484364a` | `a660ac20843e` | --speed froze the search party as well as the subject, and every still cell measured that |
| `c9248c77ce3f` | `4396390b699a` | docs/SCENARIO_FLAGS.md regenerated (tools/regen_flags.py): --imuMagSigmaDeg, --imuMagGmSig |
| `c94507b21c6a` | `98dcb4a752ae` | sect 7 Case A reaches the fix path, and is a last resort rather than a competitor |
| `c967d56f9038` | `fc5b86fe164d` | CORRW results: W1, W2, W4 held, W3 missed by 0.3 m -> --rssiCorrInflate not adopted; calib |
| `c996a4a2c944` | `d579d3ba56c6` | register the residual-versus-r80 test before computing it |
| `c9db8959c8ff` | `8fb80d0f3e22` | PREREG CAMP-SPEC: the spec configuration measured -- the preset x six arms x 20 seeds at 5 |
| `caac6214d760` | `341f122127e5` | evidence: the pcs_forms and c9_stage2 campaigns, complete this time |
| `cac9a28cefae` | `79bce878201d` | SY-3: tier-1 and tier-2 spreading factor and bandwidth are configurable, as Physical 1.1 s |
| `cac9a8436f18` | `3e79a81bc4ca` | docs/SCENARIO_FLAGS.md regenerated from the merged binary (tools/regen_flags.py; 282 flags |
| `caf441bb0032` | `396639f1fda8` | Part B, F1: --dualBaseRelayTier forms relay legs on 2.4 GHz under the dual base when the a |
| `cb02c32f516e` | `9464a72aec8c` | tools: run every gate, and close the three cited-script gaps |
| `cba0df532e48` | `d3e1c639ab07` | A.10: the link outlives the fix, so contact loss is not an observable event |
| `cc04a233c04e` | `0edea77a976b` | P6 (row 52 / N.10): the preset takes --requestOverMesh -- under --specProfile no requester |
| `cc058eb943cb` | `8f30302e32e0` | PREREG GEO-01 stage 1: which gateway shapes are worth simulating at the spec's 5 km scale, |
| `cc6811084478` | `485a324fd134` | K6+K7: the envelope exists and 868 owns it; the integrity gate's missing R term was the de |
| `cc8aebbc6197` | `a523582a7116` | register K4, and prove that splitting a cell by --seedStart is byte-identical |
| `ccaedb0746c5` | `f71d466ce13f` | pre-register: abl_clean, the recovery-policy ablation on clean provenance |
| `cd594c90c362` | `cbf743e3ed66` | pre-register: c9_clean, Claim 9 re-measured on citeable provenance |
| `cdfee42d2e61` | `7e9aa96fda5f` | HANDOFF_2026_10_08 progress: step 1 done on the cloud -- rebased onto 65224c02, the WIP co |
| `ce155fe3ebba` | `2e8c458be58e` | digest: a source sha that will not resolve is not a pre-digest source |
| `ceadc7e7e306` | `6595251df4cf` | A7 / D-5: a receiver-sensitivity check the D1 defect could not have passed -- and it fails |
| `ceed5ee07544` | `126d2e078893` | G10 fixture: the unlisted work-order case uses a neutral filename |
| `cef0fc6a163d` | `3c233bff7df7` | P6: the preset adopts CAL-02's calibrated PCS -- --specProfile now sets --pcsForm=calibrat |
| `cef40037cdd2` | `45ab066999c4` | T29 code: --interferenceDistance (default off) corrects a registered interferer's power to |
| `cf09176fbd2c` | `0714c482fc09` | an interval on the integrity result, and the write-up for the gate and the exchange |
| `cf157648c601` | `a33df1e431e4` | PREREG V7-REPLYPOS, registered before any measurement cell (master plan v3 stage V7): repl |
| `cf22d6d67120` | `3b3afc38805c` | a stray comma made finderMode a one-byte buffer, and the flag stopped accepting values |
| `cf253419744a` | `35da95166783` | the anti-entropy exchange runs both ways, and a node reads what all three of its radios he |
| `cfb69ea40757` | `47b98276450c` | a search rather than a monitored enclosure: --baseCampFrac |
| `cfcfe8c8f35b` | `a8eca7041169` | TECH_DEBT P: state at the end of the 2026-10-18/19 pass -- what is done, the owner decisio |
| `d05992bfa6ad` | `d54b68b2c763` | PREREG GEO-01 stage 2: the ten pruned geometries simulated under the preset minus geometry |
| `d05e6c21a720` | `50fc86203b69` | one node cannot be in two environments at once, so stop putting it there |
| `d24690cdfe69` | `70f5f76a1893` | the relays now range in slots of their own before the cycle instant, and chains fuse to de |
| `d27ff99b3b11` | `890ff97b23d7` | pre-register the PCS form comparison before running it |
| `d31dd0a3a084` | `2dedc842f0c9` | DC-7: a SUSPECT fix can be downweighted instead of rejected, as the spec says -- --suspect |
| `d34eeb7c474a` | `f736b7274ad9` | fix: gate suite must not overwrite the evidence it verifies |
| `d38c7a7ea3fd` | `c61c925b0058` | REAL-SPEC results (out of sample, seeds 41-60): the specified integrity threshold 9.21 on  |
| `d3a3bb9bf35e` | `e41c0a768307` | SY-2: the LR1121 radio basis -- --radioBasis=lr1121 bills tiers 1 and 2 on one Semtech LR1 |
| `d3bb518b1f06` | `feb00d854d7e` | evidence: the pcs_forms and c9_stage2 campaigns, complete this time |
| `d4033ffb2d8f` | `75257d8753b7` | plan.md: ranked accuracy work packages (IMU process-noise scale, TDoA gate adoption, error |
| `d42e1360ccf3` | `ca82d3e54310` | G6: compare the current deployment against evidence from the current deployment |
| `d459c25af899` | `398774dc2d02` | G24: a fifth zero population under the preset since --imuErrorStates joined it |
| `d495f43b75ed` | `07e7a67c9380` | the repair costs 4.34 J, and the term I did not write down is 16x the one I did |
| `d49f99c49f0e` | `f0b5ad4158f9` | P6 (CAL-03's registered adoption): the preset's PCS weights and thresholds are CAL-03's, r |
| `d6240d7d9b80` | `a642a25a85c2` | A1: dwell drives coverage, and coverage does not rescue the conjunction |
| `d64aab5f7092` | `8fedb74c2ca3` | fold the duplicate C3/C4 report into one, keeping the provenance-gap note |
| `d656a27baacf` | `92df8eb38151` | P6 (row 55 / MS-1 step 2): the preset gives every subject tag 0's components -- --subjectT |
| `d74adca761f9` | `ba1a39909a0a` | K13: searching works, it is cheaper, and it is not enough -- the constraint is search capa |
| `d761eeb9175f` | `f0bf2a9e5c85` | Tier-2 field ranging error and the covariance trace, default off: --tofFieldSigmaM raises  |
| `d76b4d5597c7` | `2a0db9fbb41c` | HANDOFF ACC-A: the decision diagnosis, the operating-point test and the static 2.4 GHz dia |
| `d789e09c95cd` | `647cc3e54909` | PARKED: full per-tag refactor, result-exact but not bit-exact |
| `d7ada9892e12` | `b8785f1edd78` | Row 17: the spec's literal PCS term 1 = 1/trace(Sigma) is built behind --pcsTerm1Inverse - |
| `d7dab92c101e` | `357aea532118` | HANDOFF_2026_10_04.md and SETUP_NEW_MACHINE.md: the cloud session's addendum -- row 52 (G8 |
| `d7e99f8ae7ed` | `77693af8e7b9` | T16: G6 evidence regenerated to new paths results/causal_v12 and det_a_v12 (clean d36133f2 |
| `d81a497d8d02` | `95704a98a84a` | D-8: --t2SensitivityRowKHz, tier 2 on its own bandwidth's data-sheet row; default unchange |
| `d82b8650c680` | `b9cdfcb6eda4` | N.1: --silenceStopsRelay -- a silenced radio relays no other node's frame; by default a de |
| `d85c10a4050f` | `39fe5fc6ef2b` | Merge branch 'wip/c1-handshake' into main |
| `d880ee8794aa` | `69338409f980` | C3 measured: the inertial prior helps on short outages and is far worse on long ones |
| `d96c7852094f` | `9aa08434e8fa` | NW-7: a UWB escalation can wait for its grant -- --grantGatedUwb turns the escalation into |
| `da328d19d0ba` | `b3885cf6f5d0` | docs: restructure the report around what stands |
| `dac3dc70db33` | `fc1ee6d51814` | Publication tables (report/paper_tables/, generated by scripts/paper_tables.py from the tr |
| `dafc0aedda4d` | `921ec3265d5c` | TA-1b: a finder's session can drive escalation -- --finderRequesterEscalation lets each fi |
| `db2ac2e24000` | `5ef19ae4f607` | L.2.3: the estimator is on the gateways by default, and at defaults only energy moves -- t |
| `db422dac4c77` | `c048b7559c41` | gates: pre-push completeness manifest, with its own failure tests |
| `dc274de40357` | `cb025c309a34` | the SHS was never broken -- it was being calibrated on noise, and the recorded diagnosis w |
| `dc622990abed` | `7d0855093a1b` | --ekf5RelayGate: a chi^2(1, 0.999) gate on relayed ranges only, in the 5-state filter -- d |
| `dc8e701affff` | `4af9075348e5` | K8: in a search the architecture stops hurting, and the margin is 0.6% |
| `dcbeb3f9d767` | `c2962bd87706` | K5: process noise is implicated in the 32.3% suspect rate, and is not the whole cause |
| `dcbf2df9e04b` | `8aa62e349076` | Row 55 / N.3: subjects can bill their range uplink -- --subjectEdgeUplink makes subjects 2 |
| `dcc355a854e3` | `93bca23cdcc2` | the escalation handshake becomes a frame, not just a charge |
| `dcce376b31de` | `45493d21c17f` | Merge pull request #5 from YKushal-hub/demo/arc-containment-and-handoff |
| `dd07cf7efa78` | `11dcc85a3317` | feat: Phase 10 evidence predicate and suspectRecovery ablation |
| `dd28c7e1225e` | `a831f05f10cf` | M.8: analyse_pcs_forms.py --strict-undefined -- a score with no defined correlation reads  |
| `ddae10f047d9` | `bd20cf8a742b` | C1 with the evidence gate off, registered before it runs |
| `ddc0173b115c` | `d37e0e978076` | S2-C NLOS audit and PREREG S2-C-NLOS, registered before any cell (exploratory): the preset |
| `de8435896fc4` | `7c03f6adbeb3` | provenance: make "does this sidecar belong" answerable, and record the profile |
| `deb52daa60f9` | `cb493d109e6e` | record the 2026-09-05 attribution rewrite, and update the plan it changed |
| `dee4e969c515` | `50b2301910c5` | docs/SCENARIO_FLAGS.md regenerated from the branch's binary (tools/regen_flags.py; 284 fla |
| `df23182fb649` | `c5315f77d568` | TOF-FIELD addendum (post-hoc, exploratory): Q3's static_2p4 availability loss at field err |
| `e01499251940` | `485ed5f794b1` | the section 4.1 reliability result has no sidecar in the tree |
| `e02e45ad9984` | `8ac10e146ad5` | fix: EXCLUDED.md lists handoff v5, and the HIDDEN assertion counted zero |
| `e0478b13f454` | `4084a2c9d425` | tool settings directory: ignored per clone in .git/info/exclude, not in the tracked tree |
| `e1295066a1cc` | `cf64805e0c22` | V6 --traceOnDemand and --onDemandCalibrate (default off); gate G143 |
| `e138a689aa35` | `0911cbd33b01` | RULES.md: final suite on a64e3bce verified (105 PASS, the five known reds only) |
| `e170377eada1` | `3a6d62c0d301` | A0 returns FAIL in 64 of 64, and the reason is degenerate |
| `e1c90578446f` | `9a14df6a35d3` | PREREG B4-SWITCHING, registered before any cell (master plan v2 stage B4; owner decisions  |
| `e226c6d4a72b` | `62e15c4aa821` | B3 read-out: static_uwb has no fixes in C6, not almost none, so the edge path cannot run |
| `e282f4d310dd` | `b6f497a8fa3f` | pre-register A0, the joint availability/energy criterion |
| `e308c14feef4` | `804e7ac52345` | P6 (owner override): the preset takes FIXES-N26's four fixes -- --specProfile now sets --l |
| `e346f99ce96e` | `20a6bec820d7` | CAL-05 exists: the integrity check is precise and insensitive |
| `e3b67a225ce2` | `bc76608af488` | GEO-01 stage 2 results: no geometry meets the criterion (availability <= 0.329 against 0.9 |
| `e3c53399d53b` | `0f6eac614837` | G11a fails and falsifies the census; register the terrain sensitivity test |
| `e431effd4063` | `6cab790ca78a` | register: A7 done, the provider unblocked C4, and two of my own claims withdrawn |
| `e44e8e227308` | `392da6a69cb7` | chore: untrack 12 traces and record the deliberately-ignored files |
| `e4ae4a109340` | `add4b7c9daab` | DC-6 / CAL-03 prerequisite: under --specProfile an explicit non-calibrated --pcsForm leave |
| `e4fd6d5c5606` | `57f5d0abe438` | Claim 4 survives with its magnitude halved, and the mediator I registered was wrong |
| `e553f395f166` | `27839228e85a` | withdraw sections 4.1 and 4.2: the directory they cite is not in this repository |
| `e6b3da9d8789` | `739708887a39` | UWB-NLOS read-out: the cited 802.15.4a model does not fill the UWB CIR matrix -- P1 FALSIF |
| `e7225bff9176` | `a709a4d28e60` | C1_NOGATE read-out: the analyser ran four times, not three |
| `e772a19c8dce` | `6cda40d78063` | G3: separate "the arms diverged" from "why", and stop dropping columns |
| `e77c9847c0f3` | `ab107ccf779a` | an arc of gateways, and an overdue report, because nothing we had was a fair test |
| `e7ab66a5ec69` | `d921b64fba08` | the anti-correlation is a property of the predicate, not the geometry -- my diagnosis is w |
| `e7fc994b808a` | `f77e4aebd20d` | pre-register the C4 re-measurement on the fixed provider, before running it |
| `e87378cb00ec` | `5ce8a01340b5` | Renumber the cloud branch's Part B work into S2's reserved ranges (WORKSTREAMS_2026_10_09  |
| `e8a56a263db3` | `da1c229ea05f` | G97: --imuStrideSigmaPct's noise is the stated model (E[e^2]/model 0.853 over 239 cycles,  |
| `e8e8a94d19db` | `1c9d1e66119d` | G5: derive the fixture's capabilities instead of assuming them |
| `e91c8380e9d7` | `52921928ba86` | HANDOFF_2026_10_04.md: the cloud continuation -- setting up a new machine with tools/ (the |
| `e93f35c0da3b` | `d36133f2a422` | T11 closed (the preset already sets --terrainPerTierFreq); PREREG DEBT-A, registered befor |
| `e9c26323e9c6` | `8efbb0ff2ff3` | DC-2: Case B's orientation comes from the IMU heading when the synthetic IMU drives the pr |
| `e9f218f79bd0` | `f5b7b439e959` | expand 101 of 111 continuations; the carry-over heuristic is wrong 42% of the time |
| `ea38dd2bf4b9` | `a64e3bcec977` | Suite on fc5b86fe: 102 PASS, the five known reds, and G136/G142/G144 stale against the pre |
| `ea5f90255738` | `c110860182f1` | Merge Part A (work/2026-10-09-acc-part-a, 2a0db9fb): --imuQuScale (G100), ACC-A, the decis |
| `ea60af83a375` | `0590c873493b` | the party deploys when the network loses the subject, not when it loses the link |
| `ead864fb3b46` | `41be7959367a` | PREREG ACC-B3-US: the unknown start (N.39) at s = 100 on the preset, with and without the  |
| `eadcb0b3cf52` | `07486c4c4019` | LOCAL_RUN_2026_10_09: the local machine's end-to-end task (owner, 2026-10-09: no communica |
| `eaf45dfd8893` | `7dd134a1b6f0` | C3 reproduces and C4 deny/pre-empt execute, both on the fixed provider |
| `eafcb3bb867f` | `f4aef6f63d2f` | REAL-IMU results: with a realistic IMU the 5-state filter is required; the acquisition gua |
| `eb1eaf81bfac` | `8b0b8bedbba9` | C4 deny and pre-empt execute, and C3 survives the substrate change |
| `ebb1c756f7ab` | `c164f731d01e` | PREREG EKF5-PRESET: does a filter that learns the gyro's errors recover the preset -- befo |
| `ebee74b24339` | `5dd7b5612829` | A3 measured: zero pre-emptions, and nothing else changes -- the same slot is lost either w |
| `ec076604a5b9` | `d75cccfb9d6b` | pre-register the PCS form comparison before running it |
| `ec0eca3f9e86` | `b807adf4ba1a` | the redundancy account of Claim 9 is falsified by data already in the tree |
| `ec48e90f1ebd` | `c6c965106056` | the 3.92 J is occupancy, not overhead -- and the joint criterion is registered prospective |
| `eca2d4783800` | `186379758099` | G63, G87, G89: each runner states the flags its checks were written for (TECH_DEBT N.31) |
| `ed2230e14cbb` | `63dff9caf6b0` | V5-C2B results (registered at 0ba68820; exploratory): V5-1..V5-5 held -- C2b in CA-TBL NOT |
| `ed5445fdf098` | `2544c1eaa62f` | register: C5 moves to partial, and the debt moves to its own file |
| `eda68268a7df` | `80773e7002f8` | CAL-05 exists: the integrity check is precise and insensitive |
| `eda70f5f4877` | `6c618d09db97` | Claim 2 re-scoped 25.68x -> 6.53x: the defect inflated the baseline, not the result |
| `edcf452430fa` | `2275bcafdf94` | S2-C results report (report/S2C_RESULTS.md): unknown start, subjects' weighting (training, |
| `ede157ff891d` | `a1e469114f8b` | pre-register the multi-node provider measurement, before running it |
| `edf73a529d8f` | `74bc14c3b887` | the relayed-variance accumulator was cleared between its two contributors |
| `ee7dceee451b` | `72e4b5c94837` | G20: the IMU filter stays honest through a long outage -- the population G17 missed |
| `eef62db90864` | `f768abeaf98b` | register a negative fixture for the robustness classifier |
| `ef78f8307736` | `da258c41b14b` | A2 stage 1 runner, with the fault label mapping corrected |
| `ef9f8d66dc38` | `eb171bb6a39f` | expose the 2x2 position covariance, which C5 needs and TracePos discards |
| `f017da42f1d7` | `a07c58e3abd3` | L.2.5: the proxy's 120 s bound refuses none of the casualty's entries -- the finders keep  |
| `f04e817de859` | `e5f1e32a69fd` | pre-register A9 before running it: the switching cost of a tiered architecture |
| `f062b5ea615c` | `402557ed30f0` | DC-3: Case C's relative shape -- --caseCRelative lets a measured subject with no anchor ra |
| `f1562b5943b3` | `67ab479d4788` | Preset: adopt --imuErrorStates (Plan A, A2) into --specProfile (103 settings); G101 follow |
| `f1a159ced38e` | `bb8fb27829ea` | V6-ONDEMAND results (registered at a3f54d15; exploratory): V6-1..V6-4 held -- calibrated p |
| `f1bdf63badd3` | `7206795bfeca` | CAL-01: calibrated PCS is the covariance trace, and nothing more -- no weighting of its th |
| `f1c137071720` | `f01334d0ea00` | ekf5 on main: the acquisition cloud, acquisition re-initialisation and relay NIS reach the |
| `f23fd5e24866` | `6c2953391fb1` | HANDOFF 2026-10-05: why Poucet beats CA-TBL (LoRa band fusion, not UWB), the four measured |
| `f272d0469eaf` | `034200e5a29a` | refuse a step-length calibration that implies a gait no human has |
| `f280dc4c062b` | `6295a5ce9ed5` | EKF5-MAIN results: ekf5 wins under a realistic gyro, collapses at 5 km under a near-perfec |
| `f2b3a21217b9` | `77d728580550` | 12/15: G4 and G5 go red on the environment correction, and both are true findings |
| `f2e2136554b9` | `5f24d22cc230` | pre-register the domination test, including the prediction that it loses at small scale |
| `f300fea885cc` | `07c700c3f983` | TA-2: the requester drives escalation -- --requesterEscalation makes the base station ask  |
| `f310c3cebc19` | `c6dc86bcf6ad` | P6 (owner call, DESIGN_DECISIONS sect 9): the preset takes --halfDuplex=2, --halfDuplexChi |
| `f42e9ec4ff49` | `a01042c1d8ff` | ACC-A-OPPOINT analysis, provenance check corrected after the run: PREREG sect 1 expected r |
| `f443ed40e157` | `f19ffd33429e` | Technical Report §0a: 2.4 vs 868 figures on the final preset (CORRW BASE rows) |
| `f4d1a866d03e` | `76974878ee06` | the reliability advantage does not reproduce -- 52/52 for both arms, M1 2.17x, FAIL |
| `f4defb505030` | `569d33c394c1` | the ladder: no band exists, and the precision tier proves both its accuracy and its irrele |
| `f581e9f1d96d` | `9e7363aac39c` | pre-register REAL-BEST: the realistic-world stack out of sample on seeds 21-40 -- M, S, F  |
| `f5a3ffbc8822` | `ae1cae74950d` | pre-register the inertial-prior campaign, with its runner and scorer, before any cell |
| `f5e51eaf136f` | `f43427c219db` | no assistant attribution anywhere in this project, as a standing rule |
| `f60b052579ba` | `0720d4225f41` | the integration's record: null control, what was ported and what was not, two new expected |
| `f67efc4afbff` | `4c5372945ad9` | V7-REPLYPOS results (registered at a33df1e4; exploratory): V7-1..V7-4 held -- 5 km CEP95 c |
| `f6c691e63371` | `c37c5f31ed93` | NW-0: the mesh can exist -- --meshLinks=all links every radio pair on a shared tier (tag-t |
| `f74640aa445d` | `9f15ab5af55e` | S2-C results (registered, 96fa6573; exploratory): 116 runs exit 0; stage C 18/18 byte-iden |
| `f7ccb7607999` | `ca8ef35f0b63` | pre-register A0, the joint availability/energy criterion |
| `f80291431f16` | `9e7986e951ae` | TOF-FIELD results (registered, adfa3bdd): at Mueller et al.'s measured outdoor 2.4 GHz err |
| `f8527ed37f63` | `062d3fd4c527` | anti-entropy proper: the digest carries the summary it has been billed for since S4 |
| `f87aaf9c7836` | `930886bbeb9e` | docs: mark the data pack as a historical record of superseded campaigns |
| `f883ec281e1f` | `a505d3112425` | Merge pull request #4 from YKushal-hub/spec/network-transport-imu |
| `f88e2242a273` | `7ee5d3d8e012` | a stale anchor is an uncertain anchor, and the substitution never said so |
| `f89158e56b22` | `b104243ceff2` | record the identity rewrite, and repoint the citations it moved |
| `f8a6ed2fd6ab` | `c76456e1eaf2` | identity-scoped QUERY and RESPONSE, answered only by the target named |
| `f91b4b826023` | `cb798c0b7a68` | no assistant vendor named in the prose |
| `f99c4ef5b522` | `fc408e6b56b4` | NW-9: QUERY and RESPONSE can carry what the spec says -- --queryV2 sends a manifest list w |
| `f9baabb00ccc` | `50df7f33df48` | --rangeBiasStates is refused under --imuFilter=ekf5 |
| `fa39262a4507` | `efac9d12cf0b` | SIG868 results (S1, S3, S4 held; S2 not evaluable, disclosed) and DESIGN_DECISIONS 14: the |
| `fab313c57e24` | `eab9ddb60cd8` | M.1: the relays' round is the default in every relayed run (--relaySlots=auto); at plain d |
| `fac5da67a0b9` | `b3c91a8369a9` | N.21 follow-up: the subjects' digest questions are staggered -- asked at tag 0's instant,  |
| `fb3826f487be` | `1eded7fb2857` | TECH_DEBT N.15 / N.25 / N.26 after FIXES-N26: the integrity check at the fitted 1992.9 has |
| `fbaffa7e0680` | `2c2c12b2dbd3` | CAMP-FINAL2 report: correct the comparison with CAMP-FINAL (known start there; endgame onl |
| `fbb37814d235` | `5a3675b2a820` | export both of a node's tables over sim time, as two files because they are two things |
| `fbcca17ed8a6` | `c2519fff33f3` | the hysteresis band becomes a swept parameter instead of a constant |
| `fbf21538548a` | `bf0a7d2178d6` | fix: narrow G12 to the form attribution takes, and add G13 for sidecar SHAs |
| `fc1f715fc02c` | `04f7a4d5d6c2` | lorawan links the physics engine its phy already calls |
| `fc3cc0ef3de1` | `b8d5972f265e` | V1-RNGKEYED results (registered at 05b65d37; exploratory): V1-1 not held on 10 seeds (868  |
| `fcccdbaa0b71` | `00380898bfb4` | REAL-STACK results: in the realistic world the consistency stack lowers the endgame CEP95  |
| `fcf8f497bda0` | `371dee388e1e` | TDOA-01 results: tier-1 TDoA's reported covariance holds the error in its bulk -- 95 %-ell |
| `fd6b5075ed63` | `5a995af7b012` | pre-register A4 before running it, and withdraw the claim that it is blocked |
| `fd7c8214530c` | `975fa677c293` | B8 diagnosed: the delivery curve is Phi(M/sigma_margin) (868 within 0.002, 2.4/UWB within  |
| `fdb22b221781` | `3c7eec5a5667` | G10 was failing on a placeholder that looked like a citation |
| `fef249d1b1c2` | `323dff814650` | session record: K4 answers the question section 4 said round 2 should ask first |
| `ff252803fb1b` | `bc2955662f6b` | Tier-1 validation (report/TIER1_VALIDATION.md): the engine's urban 868 MHz class matches R |
| `ff4f8bbd1538` | `8c21d2ce8f89` | the sidecar now vouches for grant_verdicts_*.csv: L.1.4's done-when evidence was the one m |
| `ff6faaf02ee7` | `54836e2789d8` | LORA-CMP results: CA-TBL against outdoor LoRa localisation on our simulator -- L1-L5 held; |
| `ffd5e72741b7` | `bd578474fa90` | G96: --ekf5Robust=8 is wired, reached, decides its motivating cell, and is off by default |
