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
`CLAUDE.md` §2. **Messages only:** every rewritten commit has the same tree as its original
(checked commit by commit), the count is unchanged at 81, and the first 55 carried no trailer
and keep their SHAs. The 26 below changed. Sidecars ported from the local line cite the
**old** SHA; this table is how they resolve (G13). The originals remain on
`work/2026-10-16-imu-d8-audit` until the owner decides whether to delete that branch.

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
