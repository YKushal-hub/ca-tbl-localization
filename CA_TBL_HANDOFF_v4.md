# CA-TBL — Handoff v4

**Read this file completely before taking any action. It supersedes every earlier handoff,
plan and status document in this repository.**

Date: 2026-08-10. Owner: Kushal Yalamanchi <kushalyalamanchi@gmail.com>.
Repository: `github.com/YKushal-hub/ca-tbl-localization` (**private**).
Working tree: `~/ns/ns-3-dev` in WSL.

This file is **untracked and must never be committed**.

---

# PART 0 — Context refresh: do this first

You have worked on this project before. **Your context is stale.** Substantial work has
happened since, including two defect classes fixed, a repository history rewrite, a
pre-registration, and a rewritten technical report. Do not act on remembered state.

Before doing anything else, read these in order and reconcile them against what you
remember:

| # | file | what it tells you |
|---|---|---|
| 1 | this file, entire | current status, constraints, next steps |
| 2 | `CA-TBL_Technical_Report_SelfContained.md` | the claims as they now stand — **rewritten 2026-08-10** |
| 3 | `report/REPORT_DATA_PACK.md` | every number in the report, with provenance |
| 4 | `report/PHASE10_PREREGISTRATION.md` | metric definitions, `(A,R)`, decision rule |
| 5 | `CA_TBL_PHASE10_PLAN.md` + `CA_TBL_PHASE10_AMENDMENT.md` | the current experimental plan |
| 6 | `report/PCS_REDESIGN_RESULTS.md` | the full correction chain, Parts 0–8 |

Then run these and confirm the state matches §1 below:

```bash
cd ~/ns/ns-3-dev
git log --oneline -12
git status --porcelain --untracked-files=no
git log --format='%ae' | sort -u
git branch -a && git remote -v
ls results/ | head -40
```

**If anything disagrees with §1, stop and report the discrepancy before proceeding.**

---

# PART 1 — Current status

## 1.1 Repository

| property | value |
|---|---|
| remote | `origin` → `github.com/YKushal-hub/ca-tbl-localization`, **private** |
| history | vendored-import structure: one `vendor: import ns-3.47 upstream source` root commit, then project commits |
| authors | `Kushal Yalamanchi <kushalyalamanchi@gmail.com>` **only** — verified across all refs |
| refs | `main`, `wip/b2-coverage-predicate`, plus their `origin/` tracking refs. Nothing else. |
| tags | **0**. All 97 upstream ns-3 tags deleted from local and remote. |
| upstream remote | **removed**. Re-add with `git remote add upstream https://gitlab.com/nsnam/ns-3-dev.git` if ever needed. |
| `.git` size | ~50 MB after gc |
| backups | `~/catbl-pre-rewrite.bundle`, `~/catbl-upstream-master.bundle`, `~/catbl-upstream-tags.bundle` — the only copies of pre-rewrite history |
| GitHub Actions | **disabled** at repository level |

## 1.2 Verification state

> ### THE GATE SUITE EXITS FAIL AT G6. THIS IS CORRECT. DO NOT "FIX" IT BY REGENERATING IN PLACE.
>
> G6 compares the tracked evidence in `results/causal` and `results/det_a` against a
> fresh run, **read-only**. Those files were produced by the **pre-D1 engine**; the
> six-decibel SF-index defect (`a4e23ad`) has since been corrected, so a fresh run
> legitimately differs. G6 is reporting real evidence drift that was previously
> invisible.
>
> **Until 2026-08-29 the suite regenerated those directories IN PLACE**, silently
> overwriting the evidence it was verifying and re-dating their sidecars to the last
> gate run rather than the run that produced the cited numbers. `e21666d` fixed that:
> regeneration now goes to `mktemp -d`, the tracked copies are read and never written,
> and G7 fails the suite if `git status --porcelain --untracked-files=no -- results/`
> is non-empty at exit.
>
> **Making G6 green by overwriting the tracked files reintroduces exactly the defect
> `e21666d` removed.** The remedy is §3: regenerate under the corrected engine to
> **new paths** as `baseline_v7`, keep the existing directories with superseded notes,
> and move the citations. G6 then goes green because the evidence was regenerated
> deliberately and the citations moved with it.

- Gate suite `scripts/catbl_gates.sh`: **G1–G5 pass; G6 FAILS by design** (see the box
  above), G7 passes — the suite leaves the tree clean.
- ~~Gate suite: G1–G5 all pass, exit 0.~~ **Superseded 2026-08-29:** G6 and G7 were
  added by `e21666d`; the suite now has seven gates and exits non-zero at G6.
- Verifier `verify_causal_sim.py`: 141 checks, 161 gates, 6/6 arms audited.
- G5 verifier self-test: 11 cases, all ok.
- `scripts/publication_ready.sh`: **NOT READY, for a known and recorded reason** —
  cited directories carry pre-D1 provenance, cleared by `baseline_v7`. See §3 and
  `DEFECTS.md`.
- Clone test: fresh clone builds `optimized` (1711/1711) and reproduces
  `baseline_v6/arm_summary.csv` byte-for-byte at seed 7 **with the flags recorded in that
  baseline's own sidecar** — `--nlosSource=propagation --arbitrateDutyBudget=true
  --pcsForm=legacy`. Default flags do **not** reproduce it.

## 1.3 Established results — Phase 10, `boxHalf=2000`, `gwGrid=5`, 20 seeds

Requirement: availability `A = 0.95` at accuracy `R = 25 m`. Availability denominator is
**all 1440 cycles**, not fix-cycles.

| arm | deployments meeting spec | avail | CEP50 | energy |
|---|---|---|---|---|
| ~~`ca_tbl_full`~~ | ~~19/20~~ | ~~0.9917~~ | ~~2.69 m~~ | ~~24.26 J~~ |
| ~~`ca_tbl_no_integrity`~~ | ~~18/20~~ | ~~0.9903~~ | ~~2.69 m~~ | ~~24.24 J~~ |
| ~~`fixed_threshold`~~ | ~~18/20~~ | ~~0.9828~~ | ~~3.09 m~~ | ~~22.77 J~~ |
| ~~`static_2p4`~~ | ~~14/20~~ | ~~0.9426~~ | ~~2.73 m~~ | ~~9.68 J~~ |

> **W1 — THIS TABLE IS WITHDRAWN (2026-08-29).** Measured with tier 2 six decibels
> deaf: `GetSensitivityThreshold` read the SF5 cell for SF7 on the 2.4 GHz and UWB
> tables (`a4e23ad`). Regenerated under the corrected engine
> (`results/p10_plane_v7`, 20 seeds, `source 7c0c5e5 dirty 0`):
>
> | arm | meets spec, pre-D1 → post-D1 | mean totalJ |
> |---|---|---|
> | `ca_tbl_full` | 19/20 → **20/20** | 24.260 → 27.094 |
> | `static_2p4` | 14/20 → **19/20** | 9.677 → 12.679 |
>
> **The deployment-reliability argument does not survive.** A five-seed gap became one
> seed, at 2.1× the energy. `static_2p4` also wins M1 — 12.679 J against 27.094 J.
| `static_868` | 0/20 | 0.0691 | 100.60 m | 14.81 J |
| `static_uwb` | 0/20 | 0.0551 | 0.16 m | 769.45 J |

**Claim 3 — rescued.** With `--escalationRequiresEvidence=true`:
precision 0.0777 → **0.9343**, F1 0.1439 → **0.8030**, false positives 21,116 → **90**
(20 seeds pooled). Recall falls 0.9780 → 0.7041 — the deliberate cost of the quality gate.

**Energy.** `ca_tbl_full` 622.92 J → **24.26 J** (25.68×), with accuracy and availability
*improving* simultaneously. Escalation efficiency 0.071 → 0.827.

**Claim 2.** `ca_tbl_full` > `fixed_threshold` on availability at **20/20 seeds**, no ties,
no reversals, mean margin +0.0089. Sign test p ≈ 1×10⁻⁶.

**Integrity ablation (N3). WITHDRAWN 2026-08-29 — pre-D1, not wrong.**

~~`ca_tbl_full` − `ca_tbl_no_integrity`: positive 4, negative 1, tied 15.~~

It reproduces **exactly** from the directory it cites (`results/p10_plane_fix`, 20 seeds).
That established **provenance, never validity** — reproducibility was never the question.

Withdrawn because the six-decibel tier-2 sensitivity defect (`a4e23ad`) reaches the
quantity N3 measures, and does **not** cancel in the `full − no_integrity` difference.
The mechanism, recorded so it is not re-litigated: the two arms differ only at
`ca-tbl-policy.cc:877`, and the gate's trigger is `mahal > threshold && nNlos == 0`.
In this campaign **tier-2 occupancy is 99.6%** (27 322 cycles against 108 on tier 1),
so `nNlos`, the geometry and the Mahalanobis residuals are computed almost entirely
from the tier-2 links the 6 dB error changes. Measured consequence: the gate's eligible
population is **151/28 718 all-LOS fix cycles = 0.53%**, producing **9 suspects over 20
seeds**. A sensitivity error that moves which anchors are heard moves `nNlos`, which
moves eligibility, which moves the difference.

Re-measure under the corrected engine before quoting anything of this kind.

## 1.4 Defects found, and their status

| # | defect | status |
|---|---|---|
| 1a | escalation to tier 3 ignored target-tier availability | **FIXED** — `escalationRequiresEvidence`, suppress-only, default off, enabled by flag |
| 1b | same for tier 1 → tier 2 | **BROKEN, DELIBERATELY UNWIRED** — see §4.2 |
| 2 | concurrent-tier fallback hardwired to the 868 floor rather than `active − 1` | **MEASURED, NOT FIXED** — blast radius shrank to near-nothing once 1a landed. See §4.3 |
| 9 | unchecked `std::ofstream` — ran full simulation, wrote nothing, exited 0 | **FIXED at all four sites**: `arm_summary.csv`, `probe_range_sweep.csv`, trace writer, `run_id.txt` |

## 1.5 Provenance mechanism

`run_id.txt` in every result directory now carries:

```
<16-hex rid>
<full invocation>
source <sha> dirty <n> profile <optimized|default|debug>
digest sha256
<64-hex>  <filename>          (one line per file the run produced)
```

The digest block is written **after the last output stream closes**, so its presence is
also a completeness signal. The verifier resolves the recorded source sha to decide whether
a missing digest is excusable, with **four** outcomes — resolves+has-writer, resolves+lacks
writer, does not resolve (history rewritten), not a git checkout. Do not collapse the last
two.

---

# PART 2 — What must be achieved

In priority order. Do not reorder without instruction.

## 2.1 PRIMARY — the deployment envelope

**The single most valuable outstanding item.** It converts a result scoped to one geometry
into a general deployment rule, and it is the difference between "the architecture works at
`boxHalf=2000`" and "tiering is required above infrastructure spacing X".

**Goal.** Map every arm's availability against the dimensionless axis `spacing / r80`, and
identify the crossover — the density above which no single band meets the joint requirement
and the tiered architecture does.

**Why dimensionless.** `spacing = 0.47 × boxHalf`; measured tier-2 `r80 = 909.5 m`. A result
stated as "above `spacing/r80` ≈ X" transfers to any radio pair. A result stated as "above
`boxHalf=2200`" is a fact about this simulator and nothing else.

| boxHalf | spacing | spacing/r80 |
|---|---|---|
| 1500 | 705 | 0.78 |
| 2000 | 940 | 1.03 ← current results |
| 2500 | 1175 | 1.29 |
| 3000 | 1410 | 1.55 |
| 4000 | 1880 | 2.07 |
| 5000 | 2350 | 2.58 |
| 6000 | 2820 | 3.10 |

**Method.** 7 box sizes × 6 arms × 20 seeds, `gwGrid=5`,
`--escalationRequiresEvidence=true`, `--trace=true`, one output directory per
(boxHalf, seed). Runs take ~40 s each single-arm; budget accordingly and **run one gate
suite or sweep at a time** (§5.4).

**Report per (boxHalf, arm):** `avail_mean`, `avail_min`, `seeds_meeting_spec`, `cep50M`,
`rmseM`, `outage`, `totalJ`, plus per-seed availability so distributions are available.

**Then, per arm — the crossover, read off the curves.** Plot `seeds_meeting_spec` and
`avail_mean` against `spacing/r80` for every arm and state where the **ordering between
arms changes**. That is the envelope: "where does the advantage begin and end" is a
question about two curves crossing, not about a cutoff applied to one.

Report, per box size:

- the pre-registered **PASS / FAIL / INCONCLUSIVE** verdict (`b49df46` §4, amended
  `a059203`) — seven verdicts per arm, under the rule already committed, no amendment;
- raw `seeds_meeting_spec` as a **reported observable, not a threshold** — 14/20 beside
  19/20 across seven densities tells the reader everything a cutoff would;
- the crossover location with the ordering stated at each point;
- the **sign test** across the seven box sizes for the pairwise comparisons.

> **WITHDRAWN 2026-08-10: the "≥15/20 seeds" rule this section previously specified.**
>
> It is **not pre-registered**. `report/PHASE10_PREREGISTRATION.md` and amendment
> `a059203` were searched for `15/20`, `15 of 20`, `>= 15`, `at least 15` and `fifteen` —
> zero hits in both. The pre-registered decision rule (§4) is PASS/FAIL/INCONCLUSIVE on
> `(A,R)` plus an M1 energy comparison by geometric mean of per-seed ratios with a
> seed-level bootstrap CI. There is no seed-count threshold anywhere in it.
>
> **Why it could not simply be adopted.** At the one geometry already measured,
> `static_2p4` scores 14/20 and `ca_tbl_full` 19/20. ≥15 is the only round number in that
> gap: it separates exactly the two arms under comparison, in the direction of the
> proposed system, and the numbers were already known. §3.1 condition 1 requires a defect
> in the current rule demonstrable **without reference to which results it admits** — and
> there is no defect, because ≥15/20 was an *addition* someone wanted, not a repair.
>
> **How it got in:** through this handoff document, and then through the session prompt
> that passed it along with an instruction to verify it was pre-registered. It entered the
> plan through documents rather than through the pre-registration. The verification step
> caught it, which is what it was for.
>
> Nothing above needs an amendment, because nothing is being added. If a threshold is
> genuinely wanted later for a headline sentence, it is pre-registered **before** the next
> run it could affect, with all three §3.1 conditions met.

**The prediction to be falsified, recorded before the run:** at `spacing/r80 = 1.03`,
`static_2p4` already fails 6/20 while `ca_tbl_full` fails 1/20, so the window is open there.
As spacing grows, tier-2 gaps widen — but tier-3 anchors ring the *same* facility sites, so
in the gaps only tier 1 reaches, and tier 1 fails `R=25 m` outright. The architecture's
advantage should therefore be **bounded above** as well as below: a *window*, not a
threshold. If that is what the data shows, the upper edge is a property of **co-located
siting** (§2.3), not of tiering, and the report must say so.

**Three outcomes, all publishable.** A window at a plausible SAR density → claim scoped and
proven with a design rule. A window only at implausible density → honestly bounded negative
result. No window → 2.4 GHz dominates everywhere tested, and the report says that.

## 2.2 SECONDARY — repository completeness

Kushal's requirement, stated verbatim: *"I don't want a half built repo that only runs a
subset of what I have built, I want whoever wants to access the repo have access to every
file that I have and the ability to change anything in the repo."*

**Current gap.** `results/` is ~13 GB and only partially tracked. Nine
`physics_scale_sweep_*.csv` at ~135 MB each exceed GitHub's **100 MB hard per-file limit**.
Trace CSVs are excluded entirely.

**Method — measure before choosing.**

```bash
du -sh results/; find results -type f | wc -l
find results -type f -size +45M -printf '%s %p\n' | sort -rn | head -20
f=$(find results -name 'physics_scale_sweep_*.csv' | head -1)
ls -l "$f"; gzip -c "$f" | wc -c        # measure the real ratio
```

If the compression ratio is ≥10× (likely — these are numeric CSVs), 13 GB → ~1 GB, every
file lands under 100 MB, and **everything ships in ordinary git with no LFS and no paid
tier**. `pandas.read_csv` and `zcat` read `.gz` transparently.

If <10×, report the numbers and present the options — Git LFS with a paid data pack, or
GitHub Releases for bulk data — and **let Kushal choose**. Do not choose for him.

**Add as new commits. Do not rewrite history again** (§5.1).

**Then extend `scripts/publication_ready.sh`:**
- assertion 12 — `git ls-files -z | xargs -0 git check-ignore` must be empty
- assertion 13 — every non-`build/` path is tracked or listed in a reviewed `EXCLUDED.md`
  with a stated reason
- assertion 14 — fresh-clone file count and size match the working tree

**Extend the clone test** with a fifth step: modify a policy constant, rebuild, confirm the
output changes. That is the only direct test of "can someone change anything," and it has
caught missing-source defects twice.

## 2.3 TERTIARY — the two scenario extensions

Both are real limitations documented in report §8. Neither is required for the current
claims; both would strengthen them.

**Multi-tag (§8.1).** Sweep `nTags ∈ {1,5,10,20}`. Gives N4 a substrate, exercises the MAC
contention layer, and should *strengthen* the duty-cycle finding — tier 1's 2.5% limit arms
a 2.263 s lockout per 56.58 ms frame, so contention becomes binding well before the radio
does. Emit offered load, collisions, capture events, per-node lockout time.

**Complementary siting (§8.2).** Tier-2 and tier-3 anchors currently ring the same four
facility sites (150 m and 10 m radii). This is a *deliberate control* — it makes an
escalation failure unambiguously a decision error rather than a siting artefact, which is
what makes the §5.1 trigger result clean. But it means the tiers fail together in the gaps.

**Do this only if §2.1's envelope shows a binding upper edge.** If it does, add
complementary siting as a **second treatment arm alongside co-located**, never as a
replacement — otherwise siting is confounded with everything else that changed.

## 2.4 QUATERNARY — tier-2 escalation gate

See §4.2. Requires a *predicted*-coverage gate symmetric with the tier-3 predictor, at a
radius derived from the measured 909.5 m r80. The ratio is favourable: 909.5 m against a
tier-1 position error of 63–83 m is ~11:1, versus 20 m against the same error for tier 3.
The quality gate would be satisfied almost everywhere.

Only matters where tier 2 actually fails — i.e. the regime §2.1 is looking for. **Build it
after the envelope, not before.**

---

# PART 3 — How it must be achieved

## 3.1 Pre-registration discipline

`report/PHASE10_PREREGISTRATION.md` (`b49df46`) and its amendment (`a059203`) fix the
metrics, the requirement `(A,R)`, the testability predicate and the decision rule. Both
were committed **before** every run they govern, and that ordering is verifiable from git
timestamps. Preserve it.

**To amend, all three conditions must hold:**

1. The defect in the current version is **demonstrable without reference to which results
   it admits**. The predicate amendment qualified because `boxHalf=1500` was admitted at 57
   decision cycles while `boxHalf=2200` was rejected at 589 — an inversion that holds
   regardless of outcome.
2. The amendment is **recorded with both versions and the disclosure that the outcome was
   visible when the defect was found.** Not "the predicate was revised" — state what was
   seen and when.
3. Results are **reported under both versions**. If they agree, the amendment is
   immaterial and may be described as such. If they differ, that difference is reported,
   not reconciled.

## 3.2 Every claim states its population

The project's most repeated failure, in six distinct instances. A count, rate or sum is not
an observable until its population is stated — denominator and observation window.

Applies to your own good news first. The `boxHalf=2000` acceptance claim was stated for
weeks without naming its flags and passed the whole time; it surfaced only when a
legitimate change made it fail.

**Separately: a truncated observation still settles any threshold it has already crossed.**
Censoring limits precision, not conclusions already reached. Do not merge this into the
population rule — they point opposite ways, and merging licenses discarding valid censored
inference.

## 3.3 Design checks against the failure, not the expectation

Six instances this project: a truncation scan for a defect whose mode is *absence*; mtime
dating where mtimes were demonstrably unreliable; a guard tested without `pipefail` when its
caller sets it; a `cp`-built fixture that reset every mtime; a contradiction test asserted
against a directory whose recorded mode was never read; a `*.csv` glob that vouched for
files the run never wrote.

**Before writing a check, write down what a firing looks like on disk.** For an unchecked
`ofstream`, that is "a file that isn't there" — which rules out a truncation scan before it
is run.

**Exercise guards through the harness that will run them.** An interactive `bash -c` has no
`pipefail`; the suite does.

## 3.4 Verification must not mutate its subject

Earned twice in one session. A `git reset --hard` inside a revert-test loop discarded
uncommitted `results/*/run_id.txt` updates and manufactured a sidecar that lied about its
own directory — the exact defect class the digest mechanism exists to detect.

Use `git worktree add` for anything that needs a clean tree. **Never `reset --hard` in the
live tree.**

The wider form: **gate suites hold an exclusive lock on the results tree.** Two suites
running concurrently against `results/causal`, `det_a`, `det_b` produced a `G3 FAIL` that
was pure cross-contamination, and a binary was rebuilt underneath a running suite. One at a
time. Never rebuild under a running suite.

## 3.5 Definitions live in code, and numbers are quoted against them

Four mutually incomparable numbers exist for two quantities:

| number | what it actually is |
|---|---|
| `trig_*` in `arm_summary.csv` | `activeTier == UWB` predicting `visUwb >= 3`, all cycles — **canonical** |
| `0.088` in the report and `PCS_REDESIGN_RESULTS.md:891` | the **integrity gate** scored as a trigger, seed 7 only, `boxHalf=1000`, propagation+arbitration flags |
| `0.941` on `wip/b2-coverage-predicate` | `recall(deliverable)` — scored against cycles where UWB *delivered*, not geometric solvability |
| `0.843` measured this session | geometric solvability at R=20, `boxHalf=1000` |

**`0.088` must never be quoted against the `trig_*` figures.** They are different
quantities. The report as rewritten does not do this; do not reintroduce it.

## 3.6 Commits

One change per commit, revertible without collateral. The test is mechanical: revert each
in isolation and confirm neither leaves a broken state. Thematic similarity ("both are
correctness fixes") is **not** sufficient — that argument was tried and rejected twice.

Commit message states what changed and why, with the evidence. Forced-failure evidence for
any fix to a silent-failure path.

## 3.7 Report writing is suspended

Until §2.1 reports. The technical report was rewritten 2026-08-10 and is current. Do not
edit it, do not "improve" prose, do not correct wording. If the envelope changes a claim,
that is a rewrite of substance and it happens then.

---

# PART 4 — Specific technical state you will need

## 4.1 The evidence predicate as implemented

`src/lora-physics-engine/model/ca-tbl-policy.{h,cc}`

- `escalationRequiresEvidence` — CLI flag, default **false**, gates **2.4 → UWB only**
- `uwbPredictRadiusM = 20.0`, `uwbPredictMaxSigmaFrac = 0.25`
- Quality gate runs **first**: `sqrt(max(traceSigma,0)) > 0.25 × radius` → suppress
- Coverage test: count `uwbAnchorX/Y` within radius of the EKF estimate; `< 3` → suppress
- **Suppress-only** — applied after `wantEscalate` is formed, can never create an escalation
- Counters: `m_predictBlockedQuality`, `m_predictBlockedCoverage`, `d.predictedVisUwb`
  (−1 when the predicate did not run)

Null control verified: **flag off is byte-identical to the pre-change binary.**

## 4.2 `escalationRequiresEvidenceT2` — DO NOT ENABLE

A separate field, defaults false, **deliberately not plumbed to the CLI**.

The tier-2 radio sleeps while another tier is active, so `GetNeighbourCount(TIER_2P4)`
returns 0 from tier 1 and the evidence bit is never set. Requiring it is unsatisfiable —
the node latches on tier 1 permanently. **Measured: 1440/1440 cycles on 868 MHz, 0
escalations, availability 4.9% at R=25 m.**

The field exists only to carry the record of why it does not work. The fix is §2.4 — a
*predicted*-coverage gate, not an observed-evidence one.

## 4.3 Defect 2 — measured, deliberately not fixed

`const bool runFloor = hybridArm && (active != CA_TBL_TIER_868);`

Only two tiers are ranged per cycle: the active tier and the 868 floor. When active is UWB,
tier 2 is never polled, so there is no tier-2 measurement to fall back to.

**Blast radius before defect 1a was fixed:** 79.5% UWB occupancy, 86.9% of on-UWB fixes
taken from 868 MHz.
**After:** UWB occupancy 4.8%, `usedFloor` firings 1106.6 → 84.5.

The correct fix is `active − 1` rather than the floor, and it costs tier-2 polling on every
UWB cycle. At 4.8% occupancy that is likely not worth it. **Re-measure before building it.**
Note that when active is tier 2, `active − 1` is already 868 — the current code is correct
there, so the entire question is confined to UWB cycles.

## 4.4 Seed semantics — critical for any cross-arm claim

`RngSeedManager::SetSeed(seed)`, `SetRun(1)` fixed, `ResetNextStreamIndex()` at the head of
every arm (`RunArm:1848`, `RunProbe:1090`).

**Without the reset no two arms share a channel realisation.** Measured: two arms with
identical EKF state reached 334 vs 336 engine evaluations by t=2.5 s, before either policy
had decided anything.

**Even with the reset, arms do not see identical channel realisations.** They read the same
stream but consume it at arm-dependent rates, because which tier is polled and how many
anchors are addressed depend on the arm's own behaviour. **Common at a seed:** geometry
(deterministic, no RNG) and trajectory (explicitly streamed). **Not common:** the alignment
of draws to links in time.

**Therefore the 20-seed sweep is the minimum evidence for any cross-arm claim, not a
refinement of a single-seed one.** Never report a single-seed cross-arm difference as a
result.

## 4.5 Geometry

- Facilities: 4, at `(±f, ±f)` where `f = boxHalf × 0.47`
- `t2TriadRadiusM = 150.0`, `uwbTriadRadiusM = 10.0` — **absolute**, so local anchor density
  is invariant under `boxHalf`. This is why R=20 transfers across scales.
- Gateways: `gwGrid²` on a grid spanning `±0.9 × boxHalf`
- **`gwGrid` does not affect tier-2 coverage at all** — verified identical to six decimals
  across all `gwGrid` values at every `boxHalf`. It moves tier-1 gateways only.
- `dt = 2.5 s`, duration 3600 s, **1440 cycles per run**

## 4.6 Known stale artefacts

- `scratch/ca_tbl_causal_sim.cc:216-218` claims tier-2 r80 is censored at 900 m. **It is
  not** — the probe extension ran, rows exist to 6000 m, r80 = 909.5 m interpolated. The
  comment is stale. Fix it when documentation work resumes, not before.
- `results/baseline_v6/run_id.txt` predates the `source`/`profile`/digest fields.
- `wip/b2-coverage-predicate` carries `CA-TBL_Technical_Report_SelfContained.md` and
  `report/PCS_REDESIGN_RESULTS.md` at their **pre-correction** state — "12 of 12" where
  main says 13 of 13, with the correction-#14 register entry deleted. Dropping those two
  files from the branch is not a content decision; it declines to re-litigate a settled
  correction. A rebase onto current main will conflict on both, and resolving in the
  branch's favour silently reinstates the error.

---

# PART 5 — What must NOT be done

## 5.1 Repository

- **Never rewrite history again.** The vendored-import structure is settled and verified.
  Add commits.
- **Never `git push --all` or `--mirror`.** Currently harmless — only `main` and `wip`
  exist — but do not create refs that would make it harmful.
- **Never delete the three bundles** in `~/`. They are the only copies of pre-rewrite
  history, including 495 upstream author identities.
- **Never re-enable GitHub Actions.** The inherited ns-3 workflows fail permanently and
  bill against a metered quota. ~1606 estimated minutes were consumed in one afternoon.
- **Never run git as root.** It re-owns files (2215 last time, 145 inside `.git`) and breaks
  the next gate run. Use `runuser -u kushal -- bash -lc '...'` for **every** command.
- **Never `git add` `results/` wholesale.** 13 GB, nine files over GitHub's 100 MB limit.
- **Do not modify `utils/coverity-report.sh`.** It contains an upstream ns-3 hardcoded
  credential, already public. Rewriting history over it is not warranted.

## 5.2 Results and evidence

- **Never `git reset --hard` in the live tree.** Use a worktree.
- **Never run two gate suites concurrently**, and never rebuild under a running one.
- **Never discard modified `results/*/run_id.txt`.** They are regenerated by gate runs;
  reverting them restores stale content over a current run and manufactures a lying
  sidecar. Commit them.
- **Never enable `escalationRequiresEvidenceT2`** (§4.2).

## 5.3 Claims and reporting

- **Never fabricate, tune toward, or selectively omit a result** that changes a conclusion.
  Emphasis and ordering are legitimate authorial choices; removing a comparator that makes
  a result meaningful is not. `static_2p4`'s 14/20 stays in the report — without it,
  19/20 means nothing.
- **Never attribute a measured outcome to a cause the data contradicts.** Specifically:
  do not write that results were limited by "too small a deployment" or "too few nodes"
  where the measurement shows the opposite. At `boxHalf=2000`, 2.4 GHz performs well
  *because* `spacing/r80 = 1.03` — an examiner can check this against the repo in minutes,
  and the repo contains the full correction chain.
- **Never quote `0.088` against the `trig_*` figures**, or the branch's `0.941` against the
  measured `0.843` (§3.5).
- **Never report a single-seed cross-arm difference as a result** (§4.4).
- **Never report the N3 integrity ablation as a finding** at current evidence — 4 positive,
  1 negative, 15 tied.
- **Never amend the pre-registration without all three conditions** in §3.1.

## 5.4 Process

- **Do not edit `CA-TBL_Technical_Report_SelfContained.md`** until §2.1 reports.
- **Do not do prose, wording or commit-message polish.** Documentation work is suspended.
- **Do not build a fix before measuring its magnitude.** This saved the three-tier fallback
  (§4.3) and would have saved the complementary-siting work.
- **Do not report a check as passing without confirming it can fail.** A green suite once
  concealed a guard that reported a match as a non-match, and a promoted gate shipped with
  no test proving it fails.

---

# PART 6 — Execution prompt: start here

Work in this order. Report at each **STOP**.

### Step 1 — Reconcile context

Read the six files in Part 0. Run the state checks. **Report any discrepancy against §1
before proceeding.** Do not begin Step 2 until the state is confirmed.

**STOP. Report: state matches / discrepancies found.**

### Step 2 — Launch the envelope

Pre-register nothing new — `a059203` already governs this. Record the falsifiable
prediction from §2.1 in the run's own directory before launching, so it is timestamped
ahead of the result.

```
7 box sizes {1500,2000,2500,3000,4000,5000,6000}
x 6 arms x 20 seeds, gwGrid=5, --escalationRequiresEvidence=true, --trace=true
one output directory per (boxHalf, seed)
```

Run it **alone** — nothing else touching the results tree (§3.4). Verify each run wrote its
digest block; a directory without one is an incomplete run, not a result.

**STOP. Report the full table** — per (boxHalf, arm): `avail_mean`, `avail_min`,
`seeds_meeting_spec`, `cep50M`, `rmseM`, `outage`, `totalJ`, plus per-seed availability.
Then per arm, the largest `spacing/r80` still meeting spec on ≥15/20 seeds.

**Do not interpret. Do not edit the report. Do not proceed to Step 3.**

### Step 3 — After the envelope is read

Direction depends entirely on what Step 2 shows:

| envelope result | next |
|---|---|
| window exists at plausible density | report rewrite with the design rule; §8.3 leaves limitations |
| window bounded above by co-location | §2.3 complementary siting as a second treatment arm |
| no window | report the negative honestly; §2.2 repository completeness becomes primary |

In all three cases §2.2 (repository completeness) remains outstanding and is Kushal's
explicit requirement.

---

## Contact points in the code

| what | where |
|---|---|
| scenario, arms, sweep harness | `scratch/ca_tbl_causal_sim.cc` |
| policy, PCS, predicate | `src/lora-physics-engine/model/ca-tbl-policy.{h,cc}` |
| energy, duty cycle | `src/lora-physics-engine/model/ca-tbl-energy-model.{h,cc}` |
| MAC, neighbour cache | `src/lora-physics-engine/model/ca-tbl-mac.{h,cc}` |
| verifier | `scratch/python_handoff/verify_causal_sim.py` |
| gate suite | `scripts/catbl_gates.sh`, `scripts/catbl_g3_ablation.py` |
| publication manifest | `scripts/publication_ready.sh` |
| self-test | `scripts/catbl_verifier_selftest.sh` |
