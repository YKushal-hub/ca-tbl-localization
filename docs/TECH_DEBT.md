# Technical debt — to be addressed after 2026-09-07

**What this is.** Everything known to be wrong, unfinished, or unverified in this
repository, with the cost of fixing it and what it currently puts at risk. Written
2026-09-06.

**What this is not.** A wish list. Every item here is something a reader could find and
be misled by, or something that would block work later. Items that are *deliberate
design decisions* are recorded as such and are not debt — they are in §7 so nobody
"fixes" them by accident.

**Ordering is by risk to published results, not by effort.**

---

## 1. Measurement hygiene — highest risk

### 1.1 `LoRaDistributionSampler` draws from the global RNG stream
**`lora-distribution-sampler.cc:8-17` — five random variables created, zero `SetStream`
calls.**

Every submodule on the `ProcessTransmission` path draws from it: propagation, shadowing,
fading, PLL, hardware, mobility. So **any change in the number of transmissions displaces
the random stream for every link in the run.**

*Why it matters.* It makes magnitude comparisons across two binaries inadmissible unless
they perform the same number of engine evaluations. This session hit it directly: the
multi-node provider fix moved collisions by −8 000 and captures by +5 500 per arm, and
**none of that is interpretable** — it is indistinguishable from stream displacement.
Only control-flow claims (which paths execute) survived.

This is rule 8 generalised from arms to binaries, and the rule as written does not cover
it. **Anyone comparing magnitudes across a code change is currently able to publish a
stream artefact as a result.**

*Fix.* Assign explicit streams. *Cost:* small change, but **it invalidates every existing
result** because the draws move. Must be a planned regeneration campaign, same class as
the PCS change. *Status:* CLAUDE.md §5 records it as a decision not to fix, on the grounds
that determinism holds at defaults and six non-default settings. That reasoning covers
*reproducibility* and does not cover *cross-binary comparability*, which is the actual
exposure.

### 1.2 Rule 8 does not mention the cross-binary case
CLAUDE.md rule 8 says a single-seed **cross-arm** difference is never a result. The same
hazard applies across binaries and is not written down. *Fix:* one sentence in CLAUDE.md.
*Cost:* minutes. **Do this first — it is the cheapest item here and it guards the most
expensive mistake.**

### 1.3 The results tree is largely absent
28 sidecars exist; documents cite 53 results directories. `results/p10_grid` was cited for
two headline results and was **absent from the repository**, which is what withdrew §4.1
and §4.2. G10 now catches absent *documents*; the results-directory half is checked but
the underlying data is still not there.

*Fix.* Either regenerate the cited directories or mark them excluded. *Cost:* the
regeneration is uncosted and is the same blocker as A4 below.

---

## 2. Open defects in the physics engine

### 2.1 Four loss terms are computed and then discarded
`antennaGainDb` (`lora-physics-engine.cc:340` → zeroed `:341`), `dopplerPenaltyDb`
(`:536` → `:537`), `multipathPenaltyDb` (`:489` → `:596`), `soilAttenuationDb` (→ `:593`).
**In the `probe_v7` configuration 8 of 16 terms in the loss sum are structurally zero.**

*Why it matters.* Reading the submodule that computes a term is not enough to know it is
applied — the assignment that discards it is in the caller. This has already caused one
wrong attribution.

*Fix.* Either wire them in (changes every result) or delete the dead computation and say
so in the model description. *Cost:* the decision is the expensive part, not the edit.

### 2.2 Terrain hardcodes 868 MHz for every tier
`lora-terrain-submodule.cc:69-70` applies the 868 MHz wavelength to all three tiers,
**including UWB at 7.99 GHz** — a factor of ~9 in wavelength. Terrain does not scale with
frequency. *Related:* **G11b is permanently red**; terrain is transcription-only and was
never rescoped.

### 2.3 Two unattributed residuals
UWB ~3.7 dB and 868 MHz ~3.5 dB near-field. Conclusions are currently drawn by recomputing
under ±4 dB of margin offset chosen to bracket exactly these. That is a defensible
workaround and not a fix; the residuals remain unexplained.

### 2.4 Sensitivity values are still unvalidated in absolute terms
G14 (added this session) verifies the SF-index arithmetic against each table's declared
base, and its six negative fixtures all fire. It does **not** check the absolute dBm
values against `kTB + NF + required SNR`, because the per-SF required-SNR table is not
carried in these headers. **A7 item 3 is therefore not implemented and the gate is
narrower than the work package asks.** The six-decibel defect that passed every check in
the project was of the *indexing* kind, which G14 now catches; a *transcription* error in
the values themselves would still pass.

### 2.5 Tier-1 CRLB — RESOLVED 2026-09-06, kept as a record
`report/NOT_DONE_2026_09_05.md` listed **D-3 as Done** while `CLAUDE.md:177` called the
tier-1 CRLB gate an **open defect**, and no commit confirmed either.

**Verified from the code, not from either document.** `lora-ranging-submodule.cc:151`
takes the shadowing standard deviation for tier 1 and explicitly discards the ToA inputs
-- `bandwidthHz`, `snrDbInBand` and `spreadingFactor` are `(void)`-cast with the reason
stated in the comment. **D-3 is genuinely complete; CLAUDE.md was stale.** Fixed there.

Kept here because the interesting part is not the answer but that the authoritative rules
file and the register disagreed for an unknown period and nothing detected it. See
sect 5.3.

---

## 3. Unfinished mechanisms

### 3.1 C5 — hop-provenance, remaining work
Built this session: the admission layer (A1–A5) and covariance inflation
`sigma_eff^2 = sigma_range^2 + u^T P_B u`, with G15 and five negative fixtures all
observed to fire. **Not built, deliberately:**

1. **Serialise `P_B`** so a relay can transmit its covariance. Three doubles, and
   **this one changes airtime and therefore energy.**
2. **Re-establish the §6 invariant at that point** — exactly one tier transmits per cycle
   and only that tier is charged. A relay exchange adds transmissions; if they fall
   outside the per-cycle loop the energy axis stops meaning anything. The error model
   names this as the first thing to check in review.
3. **Enable tag-to-tag links** in `EnginePhyProvider` behind `--relayedRanging`.
4. **Feed admitted measurements to the EKF**, honouring A5 (one per relay per update).

*Why it stopped here:* the admission layer is complete and testable without changing what
goes on the air. Continuing would have put the energy axis at risk to add an untested
consumer.

**And there is now a named structural blocker: C5 requires C2-full.** A relay is a
pseudo-anchor, so it must report its own position estimate `x̂_B` *and* its covariance
`P_B`. The peer has **no `CaTblPolicy`** — it is an addressing and contention peer, not a
measured subject — so `P_B` does not exist to be transmitted. **Until a second tag has a
filter, there is nothing to relay.**

Note the shape of this: C4 was *believed* to need C2-full and did not. C5 *does*, and for a
reason that is visible in the type system rather than inferred from a measurement. The EKF
side is already ready — `CaTblEkf::Update(meas, varM2)` takes a per-measurement variance and
`CaTblRangeMeasurement` is already an (anchor position, range) pair, which is exactly the
pseudo-anchor shape.

**C5_INSTANT, 2026-09-29 — every `--relayedRanging` number to date predates `--relaySlots`.**
Until `--relaySlots`, every exchange a cycle made by calling an engine directly — each relay's
anchor polls, each relay-to-relay exchange, the measured tag's own — carried the cycle's one
instant. The engines' interference registry decides overlap on time and transmitter id alone,
so they collided. With only `--relayedRanging` on (static_2p4, 300 s, `--boxHalf=200`, four
tags, seeds 7/11/13), the measured tag's 3+-anchor cycles fell from 107/112/106 of 120 to
2/0/11, and relays after the first never held a fix. With `--relaySlots` the same runs give
103/104/107. `--relaySlots` (default off) moves the relays' round into its own 50 ms slots
ending before the instant (`RelayRound`, `scratch/ca_tbl_causal_sim.cc`). Under it, G17b
(L.1.2's done-when) is green: 4 fuses, depth 2 on 40 decisions, A2 at depth 2 passing 19 and
refusing 21 on the budget. **Every relayed-ranging figure recorded before it was measured
under the collision and does not compare with a `--relaySlots` run.** That covers
`MASTER_PLAN.md`'s C5 row (78/31/11/11 admitted, σ 3.0 → 39–52 m, the 0.20 relayed-variance
share, 37 vs 40 escalations), `b390ed6`'s L.1.2 sweep and `5f3ad42`'s routing counts. Those
records stay as written. **Still open:** the registry never attenuates an interferer by
distance (`lora-interference-submodule.cc:126-129` computes `dist` and does not use it). So
overlapping exchanges interfere at their own link's power wherever they are, and separating
them in time is the only protection the model has. The round is also stamped inside the slice
the event layer has already run, so the relays in it meet no MAC frame and no MAC frame meets
them. With the flag off a direct exchange meets only frames straddling the instant, at most
one airtime of look-back. In G17b's regime (~50 MAC frames/s over three tiers) that bounds
the interference the relays give up at about 16% of exchanges on 2.4 GHz.

> **2026-09-30:** `--relaySlots` is now the default in every relayed run (M.1, decided). The
> paragraph above describes the state before that. A relayed figure recorded before
> 2026-09-30 without `--relaySlots=true` was measured under the collision, which
> `--relaySlots=false` restores.

### 3.2 `hopWeight` stays 0, and the PCS hop term is the wrong shape
`ca-tbl-policy.cc:672`, `:708`. The current term is **unbounded below**, penalises a
*count* rather than an *uncertainty the covariance already carries*, and does not encode
the shared-error property that is the whole reason provenance exists. A bounded
multiplicative replacement is specified in `docs/RELAYED_RANGING_ERROR_MODEL.md` §5 and
is registered as a **separate PCS campaign**, not part of C5.

Also: **A2 caps hop depth at 1, so `hopCount` is binary.** A term linear in it has two
reachable values. Worth knowing before anyone tunes a weight expecting a gradient.

> **2026-09-30, stale references, corrected here rather than in place.**
> - The hop term now sits at `ca-tbl-policy.cc:848-851` (v2 path) and `:890-893`; the
>   `hopWeight` default is at `:101`, not `:83`.
> - A2 is no longer a fixed depth-1 cap. It admits depth up to `--maxRelayDepth` under a
>   variance budget (`ca-tbl-relay-admission.cc:120-134`). The cap is still 1 **at the
>   default** `--maxRelayDepth=1`.

### 3.3 C2-full — the peer is not a measured subject
Rebased onto current main as `wip/c2-on-main` (`7cbfead`), builds, and the C4 conflict is
resolved with `pcsHist` / `pcsDegradation` / `batteryUsedFrac` made **per tag** — with two
measured targets both issue their own `ESC_REQUEST` and a shared deque would score both
off whichever tag wrote last.

**It now blocks C5** (§3.1), which is a change from the previous entry: the provider fix
removed its role as C4's blocker, but a relay with no filter has no `P_B` to report.

**Merge decision is open and is the owner's**, against a bar rather than a judgement:
CLAUDE.md §4.2 requires byte-identical output — `arm_summary` **and every trace**. This is
result-exact and not bit-exact (§4.1), so it does not meet that bar as written.

> **2026-09-30, stale.** The merge happened at `abbe3de` (2026-09-06, "merge C2-full:
> per-tag state, so a second target can be a measured subject"). `wip/c2-on-main` and
> `wip/c2-full-refactor` have no commits ahead of `main`.
> - What exists is the **data structure**: `CaTblTagState`, `scratch/ca_tbl_causal_sim.cc:2627-2701`.
> - The **behaviour** is still tag 0's alone. The cycle body is not looped over tags. Peers
>   have no policy, and relays under `--relayedRanging` have a policy but never escalate.
>   Their energy accounts are incompletely configured.
> - Finishing it is `docs/SPEC_CONFORMANCE_PLAN.md` MS-1.

### 3.4 A4 / A5 — blocked on data that does not exist
A4 says to compare reported sigma against realised error *"from the existing traces"*.
**Those traces do not exist.** A5 is gated on A4 by an explicit ordering rule and
additionally needs a `BuildDeployment()` change — the four sites are hardcoded at
`±0.47 × boxHalf`. *Cost:* a density-sweep regeneration that is in no plan and is uncosted.

### 3.5 Anchor-to-anchor links do not exist
Every anchor is a hidden terminal to every other anchor: their CAD never detects each
other. **Beacon loss is therefore an upper bound, not an estimate.** Stated in the
provider's class comment; not a defect, but it bounds what contention results mean.

---

## 4. Verification gaps

### 4.1 The 1-ulp trace divergence has no known cause
The C3/C4 branches produce traces differing from the pre-change binary in **161 cells of
63 360, largest relative difference 3.3e-13**, on five columns printed to 13 significant
figures. `arm_summary.csv` is byte-identical and no reported quantity moves.

**The explanation previously given was withdrawn this session.** It held that *any*
addition to `RunArm` changes the stack frame and therefore the codegen under
`-march=native`. The provider fix grew a `RunArm` local by a `std::map` and left **all six
traces byte-identical**, which falsifies that as a general claim.

**Measured 2026-09-06, and it is TWO phenomena rather than one:**

| branch | differing cells | largest relative difference | affected |
|---|---:|---:|---|
| C3 / C4 | 161 / 63 360 | **3.3e-13** | 5 columns |
| C2-full | 2 379 / 380 160 | **5.35e-09** | `estXhp`, `estYhp`, `meanSigmaM`, `meanSigmaRawM`, `mahalanobis` |

**Four orders of magnitude apart, so they are not the same effect.** 3.3e-13 is last-digit
rounding at double precision. 5.35e-09 is a rounding difference **accumulated through an
iterative filter** over 3600 s: moving per-tag state from the stack into a heap vector
changes the codegen once, and the EKF then compounds it every cycle.

**`mahalanobis` is in the affected set, and that quantity is compared against a threshold**
(χ²₀.₉₉ = 9.21). A divergence in a *compared* quantity can flip a decision where a
divergence in a *reported* one cannot — rule 10's territory. So it was checked rather than
argued: `suspects`, `escalations`, `deescalations`, `noFixCycles` and `fixCycles` are
identical across all six arms, and `arm_summary.csv` is byte-identical. **No threshold
decision moved.**

*Why it still matters.* The byte-identity criterion is the project's main null-control
gate, and it currently fires on stack layout as well as on behaviour. *Suggested test:*
rebuild both binaries with `-march=x86-64` instead of `native`; if the C3/C4 divergence
disappears it is vectorisation/FMA contraction. The C2-full one probably will not, since
accumulation through the filter is not a codegen artefact once it has started.

### 4.2 `-march=native` makes byte-identity machine-dependent
Every null control compares against digests produced on a specific machine. Reproduction
on different hardware is not guaranteed by construction. *Mitigation observed:*
`baseline_v7` did reproduce byte-identically on a different machine and compiler, which is
evidence but not a guarantee.

### 4.3 G12 fires on its own documentation
The gate is red on exactly **one** commit — `3420073`, the commit that *documents the
gate's own pattern*, because its body lists `@anthropic` and `@openai` as fixture text.
Those two alternatives are **unanchored**, unlike the trailer alternatives, so they match a
subject anywhere in prose — which is precisely what the gate's own comment says
attribution is *not* ("attribution is a FORM, not a subject").

**Identity fields are clean across all commits.** Two remedies, both owner-authorised:
anchor the vendor-handle alternatives to a trailer form, or rewrite that one commit. This
gate has now fired on a commit describing it three times.

### 4.4 G11 permanently red
Closing it means deciding which of the analytic model and the engines is wrong. That is
research, not a work item.

---

## 5. Provenance and history

### 5.1 137 commits on GitHub carry the wrong author
**Recounted 2026-09-09. The heading said 106; it is now 137**, because commits kept
being pushed from the Windows clone while the entry sat unrecounted. That is section 5.3
happening inside the register that tracks 5.3, and the count is now given with the
command that produces it so the next reader can recheck rather than trust:

```bash
git log --format='%ae' | sort | uniq -c | sort -rn
git log --format='%ae' origin/main | sort | uniq -c | sort -rn
```

Current census of 318 commits: **178** `nethi.kushaal@gmail.com` (the owner), **137**
`kushalyalamanchi@gmail.com` (not the owner), **3** GitHub merge commits. Of these, 225
are on `origin/main` and **93 are unpushed** — all of the unpushed set carries the correct
identity.

*Cause.* The WSL clone held a repo-local `user.email` in `.git/config` overriding the
correct global one. The Windows clone was right throughout, which is why this stayed
invisible until the contribution graph was looked at.

*Fixed already.* The 57 then-unpushed commits were rewritten on 2026-09-06 (`SHA_MAP.md`,
second section), verified metadata-only — trees identical on every branch — and checked
against every sidecar BEFORE rewriting, since a moved SHA breaks the link between a
result and the code that produced it. The config is corrected in all three worktrees, so
nothing further goes in wrong.

*What remains.* The 137 already published. Correcting them is a **force-push to a
protected branch**. The rewrite is minutes; the risk is the sidecar citations, and the
sidecar check must be redone against the PUBLISHED set — the earlier audit covered only
the unpushed one. `SHA_MAP.md` and gate G13 exist for exactly this and the procedure is
proven.

**Nothing reaches the owner's contribution graph until commits are on the default branch**,
so the 93 unpushed ones are invisible too, correct identity notwithstanding.

### 5.2 Measurement invocations recorded only in prose
The original C3 measurement's duration exists in **no sidecar and no results directory** —
only in a commit message. It reproduced, so it cost nothing; had it not, the result would
have been unrecoverable. Same class as the §4.1 withdrawal. *Fix:* every measurement
writes a sidecar, including exploratory ones.

### 5.3 Documents go stale, and it has caused real errors
Found this session: the conformance matrix still said *"Chapter 12 has no implemented
mechanism at all"* with three built capabilities listed ABSENT. Previously:
`PCS_REDESIGN_RESULTS.md` §7.10 said S7 fault injection was unimplemented when 162 results
directories used it. *Fix:* a periodic re-audit, or a gate that flags matrix rows whose
cited code path exists.

---

## 6. Build and tooling

- **`src/lorawan` link fix** (`${liblora-physics-engine}`) — carried but never upstreamed.
- **`--build-profile=optimized` is mandatory**, not a preference: the default profile
  enables logging and runs ~10× slower. Not enforced by anything.
- **Gate suite runtime** is ~35 minutes, dominated by two 3600 s determinism runs. It
  discourages running the full suite, which is how three gates once went unobserved.

---

## 7. Deliberate — do NOT "fix" these

Recorded so nobody mistakes them for debt:

- **The PCS saturation defect** is recorded, not corrected, on purpose. A fix changes the
  engine and **invalidates every result in the tree**. It is a planned campaign
  (`MASTER_PLAN.md` A8), not a bug fix.
- **`report/REPORT_DATA_PACK.md` cites superseded campaigns on purpose** — it documents
  what those runs produced. It has a registered dirty-citation exemption in `EXCLUDED.md`.
- **`run_id.txt` sidecars are evidence.** Editing them is falsification; they resolve
  through `SHA_MAP.md`.
- **Tag-to-tag links are off by choice**, not by inability. The link budget would
  evaluate; a relayed range is inadmissible until the model that weights it is wired in.
- **A8 is closed by result** — three PCS forms failed the same monotone-of-filter-statistic
  control. Do not run a fourth.

---

## Suggested order

| # | item | why first | cost |
|---|---|---|---|
| 1 | §1.2 rule 8 cross-binary sentence | cheapest guard against the most expensive mistake | minutes |
| 2 | §2.5 reconcile D-3 / CLAUDE.md | a live contradiction in the authoritative rules file | < 1 h |
| 3 | §4.3 G12 decision | keeps the suite red, hides future regressions | owner call |
| 4 | §5.1 push + identity decision | no contribution credit until then | owner call |
| 5 | §4.1 diagnose the 1-ulp divergence | the main null-control gate has unclear meaning | half a day |
| 6 | §3.1 C5 remaining, with the §6 invariant check | finishes Chapter 12 | 1–2 days |
| 7 | §1.1 sampler streams | correct, but forces full regeneration | campaign |

---

# Addendum — 2026-09-07, after the topology correction

The model changed shape today. Every item below is new or materially changed, and this
addendum takes precedence over anything above it that contradicts it.

## A. The blocker that outranks everything else

**There is no baseline for the corrected topology.** `baseline_v7` describes the old
three-population deployment and does not reproduce, correctly. Until a new baseline
exists and is pinned, **no null control can be run for any future change** — the
project's main safety mechanism is offline.

*Cost:* one clean run at fixed flags, its digests recorded, and a sidecar. Under an
hour. **Do this before any further code change**, because every change made without it
is unverifiable after the fact.

**Consequence:** every published number in the tree is now provisional. A4, A5, C1, C3,
C4, C5, A6, A1 and the PCS forms were all measured on the old deployment.

## B. Newly built, and what each still lacks

| built today | still missing |
|---|---|
| Tri-band gateways | gateway **count/placement** (brief: 3 over 5 km; built: 9 over 2 km, and 3 is not a square number so the grid cannot express it) |
| Finders as mobile tri-band nodes | dispersal — all finders converge on the **same point**; real teams spread over the uncertainty region |
| Table entries with contents | only the **self-entry** propagates. Digest carries no sequence summary, so the anti-entropy exchange is one-directional and cannot detect what a peer is missing |
| Position dissemination | staleness is unmanaged: finders act on a **39 s-old, 36 m-wrong** position and nothing bounds that |
| Corroboration witness | constants uncalibrated — `k = 3`, assumed range sigma 30 m, floor 0.5, minimum 2 witnesses. Sensitivity untested |
| SHS inertial engine | **driver does not work** — see `docs/DESIGN_DECISIONS.md` §8 |

## C. Still unbuilt from the brief

1. **Hop weight is zero.** `hopWeight = 0.0` with the comment "no mesh yet". The
   confidence score's hop term is inert, and per-hop RSSI/SNR variance and per-hop NLOS
   likelihood — both named in the brief — never reach it. C5 built the *measurement*
   side; the *confidence* side is untouched.
2. **Gateways are not edge servers.** The brief has them performing the EKF fusion and
   NLOS/hop computation instead of the battery-constrained field node. The field node
   runs its own filter. This matters for the energy claim specifically, since that claim
   is about the field node's budget.
3. **Tier-1 TDoA is billed but not estimated.** The energy model charges a TDoA-shaped
   exchange — one broadcast, anchors timestamp it — while the position comes from RSSI
   inversion alone, and the ranging code states outright that it is not a time-of-arrival
   estimator.
4. **Multi-hop localisation.** Flood forwarding exists; the poll set is strictly one-hop
   and relayed ranging is capped at depth 1, so "mesh" means single-hop to gateways and
   finders.
5. **Self-deployable mesh.** Deployment is a fixed grid plus mobile finders.

## D. A methodological failure worth keeping

The finder-convergence result reported earlier today — UWB reachable, energy 51 J — was
an **artefact**. The finders were steering on `policy.EstX()` read straight out of the
simulator, a number no packet ever carried. The honest figure, once the position had to
travel over the air, is **80.1 J**, and the 29 J difference is the cost of dissemination
latency the artefact hid.

It was caught only because instrumentation was added to ask whether any finder actually
held the entry — the answer was 0 of 4. **A result that depends on a channel should be
checked against whether that channel exists**, and nothing in the gate suite tests for
this class of error.

## E. Where the good result actually stands

`report/INTEGRITY_CHECK_RESULTS.md` — energy improved 8/8 seeds, accuracy 7/8, paired.
That result is **corroboration alone**; the inertial witness is off and contributes
nothing. It is one configuration, at one duration, with no confidence interval. It is the
strongest thing in the repository and it is still four qualifications short of a claim.

## F. Suggested order

| # | item | why first |
|---|---|---|
| 1 | New baseline on the corrected topology | nothing is verifiable without it |
| 2 | Hop weight + per-hop variance into the score | smallest step that makes the confidence score carry information the covariance does not |
| 3 | Gateway count/placement to the brief | cheap, and every number depends on the deployment |
| 4 | Gateways as edge servers | the energy claim is about the wrong node until this moves |
| 5 | Seed sweep + CI on the integrity result | turns the best finding into a claim |
| 6 | SHS driver | needs a mobility model with finite turn rates; largest and least certain |

---

# Addendum — 2026-09-08

Takes precedence over anything above it that it contradicts.

## G. Closed since the 2026-09-07 addendum

| item | where it stood | now |
|---|---|---|
| **A. No baseline for the corrected topology** | "outranks everything else" | **Closed.** `baseline_v8` (`ae1faf5`), `dirty 0`, reproduces 7/7. Null controls work again and five changes have since used it. |
| **B. Gateway count/placement to the brief** | 9 over 2 km vs the brief's 3 over 5 km | **Closed.** `--nGateways` / `--gwRingFrac` (`089a9f1`). |
| **C.1 Hop weight is zero / provenance inert** | "the confidence side is untouched" | **Partly closed.** The provenance term reaches the score (`57ebb41`) and now has a non-zero input (`edf73a5`): share 0.20 over 180 cycles, and `--hopProvenanceForm` 0 vs 1 finally differ. **Still open:** whether it carries information the covariance does not. *2026-10-03:* per-hop RSSI/SNR variance and per-hop NLOS reach σ_eff under `--hopVarianceTerms` (NW-1 step 3, G58), and on tag-to-tag legs they swamp it (N.11). |
| **B. Only the self-entry propagates / exchange is one-directional** | listed as missing | **Closed.** `cf25341`. The casualty asks as well as answers, and a node reads the union of its own three tables. |
| **E. The integrity result has no confidence interval** | "four qualifications short of a claim" | **One qualification closed.** All three intervals exclude zero (`cf09176`). The other three stand. |
| **B. Finder entries are stale by construction** | not previously recorded | **Closed.** A finder published its spawn position once and then walked for the whole run (`5262c59`). |

## H. New, and the first two are the important ones

### H.1 Tier 3 is reached rarely, and the metric could not see it at all
**CORRECTED 2026-09-08.** This entry originally read *"Tier 3 is unreachable in the
tested configuration"*. That was too strong, and part of the zero it rested on was a
defect in the measurement rather than in the deployment: `VisibleCount()` labels
ground-truth UWB coverage from the STATIC anchor list, which does not contain finders,
so `cyclesInUwbCoverage` was structurally 0 and every UWB escalation scored a false
positive (`2af5702`).

Measured with the label corrected and the full stack on: **three UWB neighbours on 12
of 480 cycles**, four cycles labelled in coverage, trigger recall 1.0 at precision 0.19.
So tier 3 **is** reached — on roughly 2.5% of cycles — and the policy escalates on every
cycle that genuinely had coverage while over-escalating about five to one. That is a
readable result about the trigger. The original text below still describes the size of
the problem correctly; only the word "unreachable" was wrong.

### H.1 (original text) Tier 3 is scarce in the tested configuration
`--uwbEvidenceFromNeighbours` takes UWB cycles from 176.5 to **0.0** and energy from
72.03 J to 5.69 J. The saving is real and it is 92% of the budget, but the mechanism is
*not using tier 3 at all* — the node had been escalating to UWB for 176 of 240 cycles
and getting **zero UWB fixes** against 0.01–0.22 mean UWB neighbours.

**This is the architecture's central open question, not a defect to fix.** The premise is
that a finder carries UWB to the casualty. In 600 s at a 1 km box with four converging
finders, it does not get there. Until it does, the tiered arm's third tier is dead
weight and the honest headline is "the policy was wasting almost all of its energy",
not "tiering works". *Fix:* not a code change — a configuration in which tier 3 is
reachable (longer runs, closer convergence, more finders), measured. Uncosted.

### H.2 Dissemination latency is bounded at the consumer, not at the source
The pseudo-anchor now prices an entry's age into its variance (`f88e224`), so a stale
entry degrades gracefully instead of poisoning the filter. **Nothing bounds the age
itself.** Finders act on a casualty position 79–240 s old, and the casualty on finder
entries of similar age, with no maximum and no re-request when an entry goes stale. The
digest period is a fixed 30 s regardless of how fast anything is moving. *Fix:* an
age-triggered digest, or a staleness ceiling above which an entry is dropped rather than
inflated. *Cost:* small; the question of what the ceiling should be is the expensive part.

### H.3 The gate suite cannot see a mechanism that fires into a zero
`edf73a5` fixed an accumulator cleared between its two contributors. For the whole life
of `937a55a` the substitution counter reported 1513 firings while the term it fed read
0.00, and `--hopProvenanceForm` 0 and 1 produced byte-identical output. **Two counters
each looked right and disagreed about the same cycle.** No gate tests for it, and it is
the same family as the 51 J artefact: a mechanism whose stated effect depends on a
quantity nobody checked was non-zero. *Fix:* a gate asserting that a mechanism's input
is non-zero whenever its firing counter is, on at least one configuration.

### H.4 Constants introduced this session, ours and uncited
- `--finderPosSigma` default 0.0; 2.0 m is argued from the variance the entry already
  claimed, not from a receiver datasheet.
- The staleness bound uses `walkSpeedMps` as the searcher-speed bound. Defensible — it is
  the speed the deployment already assumes — but it is an assumption the consumer makes
  about the reporter, and a reporter that ran would break it.
- The casualty's digest period reuses `finderDigestPeriodS`. Not swept.

### H.5 Knowledge is unioned across tiers; the poll set deliberately is not
`cf25341` has a node read the union of its three per-tier tables. That is correct — one
node, three interfaces — but it means an entry heard on 868 can now be used to
substitute an anchor position for a UWB measurement. Physically sound (the report is
about position, not about the radio), and worth knowing before anyone reasons about
per-tier isolation. The one-hop neighbour table is **not** unioned, deliberately, because
it drives the poll set.

## I. What a flag-efficacy sweep found, 2026-09-08

`scripts/flag_efficacy.sh` runs every boolean flag both ways and compares output byte
for byte. It exists because two defects this session were invisible to the gate suite,
to the null controls and to reading the code, and both were found by one question:
**can this flag change its own output?**

### I.1 The Mahalanobis integrity check has an empty eligible population
`--mahalUseR` and `--suspectForcesEscalation` produce byte-identical output because
`suspects == 0` in every run tested. The verifier says the same thing from the other
side: *"all-LOS on 0/1431 fix cycles (0.00%). The all-LOS rate is the integrity gate's
eligible population."*

The check is specified to run on all-LOS cycles, and under the corrected topology there
are none. So the mechanism the thesis register calls the central contribution **cannot
fire at all** in the configuration everything else is measured in, and two flags that
configure it are consequently untestable.

**Distinguish it from the new one.** The *switching-point integrity check* built on
2026-09-07 — IMU and neighbour corroboration — does fire, and is the 8/8 result in
`report/INTEGRITY_CHECK_RESULTS.md`. It is the older Mahalanobis test that is dead here.
*Fix:* a configuration with a non-empty all-LOS population, or an explicit statement
that the Mahalanobis form is not exercised under this topology. Uncosted, and it needs
deciding before either is written up as a contribution.

**2026-09-30 / 10-01, DC-8: the spec's eligibility is built, and it gives the check a
population.**
- What it is: the docx's condition is "NLOS_score is LOW", not "every link tagged LOS".
  `--integrityNlosFromScore` (with `--nlosClassifier`) reads the mean score of the fix
  set's scored links instead (gate G30).
- What it does, at seed 7, 900 s, `ca_tbl_full`, defaults:
  - the tag-based control evaluates 305 fixes and finds **0** all-LOS, so this entry's
    zero reproduces;
  - read from the score, **296 of 297** scored fixes are eligible, and the check calls
    **152** of them suspect.
- **That is a new problem, not a solution.** Under the default response, which is
  reject-and-coast, half of the fix cycles are now rejected. The check's reference is the
  prior, and a tier-1 RSSI fix sits many sigma from it on most cycles (median Mahalanobis
  in the tens). So the check now fires constantly for the reason this entry's lock-out
  paragraph (`CaTblSuspectRecovery`) describes.
- Single seed: a population, not a rate (rule 8).
- The spec pairs this condition with DC-7's downweight (`--suspectRecovery=downweight`),
  and the preset turns on both. Whether that pair behaves is CAMP-SPEC's to measure.

### I.2 `--uwbEvidenceFromNeighbours` was broken on arrival, and is fixed
Recorded in full at `c236834`. It read a neighbour table belonging to a radio that is
put in deep sleep and cleared every cycle, so it was a permanent veto on tier 3 and
produced byte-identical output to the static map on all 8 seeds. **Two mechanisms
agreeing exactly is not agreement.** Now predicts from measured range on the awake tier;
tier 3 goes from 0.0 to 5.4 cycles.

### I.3 The 66 J was attributed to the wrong flag
`report/EVIDENCE_GATE_AND_EXCHANGE.md` originally credited the saving to
`--uwbEvidenceFromNeighbours`. The campaign compared *neither* against *both* and could
not separate them. The third arm shows the saving belongs to
`--escalationRequiresEvidence`. Corrected in `88a3f91`. **Rule 5, and it is now the
second time a claim in this project has needed the untested arm checked.**

### I.4 Four flags are silently conditional on `--sendHandshakeFrames`
`--bidirectionalDigest`, `--finderPseudoAnchor` and `--corroborationCheck` do nothing
without it and say so nowhere except in one flag's help text. Not a defect; a
documentation gap that costs an afternoon to anyone sweeping them. *Fix:* say it in each
flag's help, which is the house style for explaining a default.

### I.5 Suggested gate
A gate asserting that a mechanism's *input* is non-zero whenever its *firing counter*
is. Both defects above had a counter reporting healthy activity while the quantity it
fed read zero. Nothing in the suite compares the two.

### I.6 The evidence tree predates the deployment it describes — THE LARGEST ITEM
**Most of the results tree was measured on the pre-2026-09-07 topology and is
provisional.** Of the 98 directories under `results/`, only a handful carry a sidecar
whose commit post-dates the correction; the rest were produced when the model built
three disjoint anchor populations at facility sites instead of one population of
tri-band nodes.

That is not a small annotation. It means A4, A5, C1, C3, C4, C5, A6, A1, the PCS forms
and the p10 surfaces were all measured against a geometry the scenario no longer builds
— and the topology commit (`99fdf1a`) says so in its own message. Every figure derived
from them inherits the same status.

**This is compute, not design.** The campaigns are scripted, the flags are in the
sidecars, and `baseline_v8` plus `causal_v8`/`det_a_v8` establish that the current
engine reproduces byte-for-byte. Nothing has to be re-decided; it has to be re-run, and
then every document citing the old numbers has to be repointed.

*Sequence, and the order is load-bearing:*
1. Settle §H.1 first — whether tier 3 is reachable at the scale being measured, or
   whether the runs should state that it is not. Re-running 20 hours of campaigns
   against an unresolved tier-3 question produces numbers that need throwing away a
   second time.
2. Re-run to **new paths**. Never regenerate a tracked directory in place (rule 7); the
   old directories stay as the record of what those runs produced.
3. Repoint citations, and add a corrections register entry for every number this makes
   wrong. G10 will catch a citation that no longer resolves; nothing will catch a
   citation that still resolves but now points at a superseded number, so that half is
   manual.

*Cost:* uncosted, and it is the single largest remaining item in this register. The
figure most worth re-establishing first is the §7 reliability result — 82% more
deployments for 27% more energy — because it is the project's one headline number that
is not a negative, and it currently rests on the old geometry.

## J. Moved here by owner decision, 2026-09-08

### J.1 G11 — which of the analytic model and the engines is wrong
**Registered as debt rather than as a work item, and it must not be closed by tuning.**

Two independent implementations of the same physics disagree: the ns-3 engines (~35
submodules per tier) and a closed-form link budget (`linkbudget.py`, which imports only
`math` and cannot read the probe). G11 assertion 2 asks them to reconcile at a criterion
registered before the comparison — `r80` within 15%, margin within 3 dB. Measured:

| tier | disagreement |
|---|---|
| 868 MHz | model `r80` 1811.7 m vs engine 1515.3 m → **19.6%**; worst margin gap **4.95 dB** |
| 2.4 GHz | margin gap **grows monotonically with distance**: −0.51 dB at 10 m → **+11.69 dB** at 6 km |

**The growth pattern is the diagnostic and it is worth writing down.** A calibration
constant that was simply wrong would show a flat offset at every distance. A gap that
scales with range points at a **distance-dependent term** — a path-loss exponent, or
something multiplying `log10(d)` — and the two tiers disagree in opposite directions,
which a single shared constant cannot produce. That narrows the search considerably and
nobody has run it down.

**Why this must not be made green.** The gate prints its own instruction: *"Do NOT tune
the analytic model to match. Report which term differs and which of the two is more
likely wrong."* `CLAUDE.md` repeats it as a standing rule. The reason is on the record:
the six-decibel receiver-sensitivity error — which sat on the tier carrying ~95% of
active cycles and **passed every gate, verifier check and validation in the project** —
was found by this model refusing to reconcile, not by any gate. Adjusting the analytic
model to agree would destroy the only instrument that has ever caught a defect of that
class, in exchange for a green line.

*Fix:* research. Instrument both implementations term by term at two distances on
2.4 GHz, difference the loss sums, and identify which term carries the growth. *Cost:*
days, and the answer may be that the engines are wrong, which would invalidate results.
*Status:* **stays red, deliberately. 14/15 is the ceiling and that is correct.**

### J.2 Tier-1 TDoA: build the timing layer, or keep re-billing
`2a61fd2` took the cheap half of this — `--tier1ChargeReplies` now measures what the
downlink costs, and it nearly doubles tier-1 energy (13.586 → 25.860 J at seed 7). What
it does **not** do is make tier 1 a time-of-arrival estimator, which is what the
blueprint specifies (`ch09 §9.3`, TDoA multilateration by Chan/Foy) and what the energy
model's frame shape has always implied.

*What building it needs, in order:*
1. **A timing layer.** Anchors must timestamp a received frame with sub-microsecond
   resolution; 1 µs of timing error is ~300 m of ranging error, so this is the whole
   difficulty.
2. **Clock synchronisation, which this project does not have.** The GPS-PPS row in
   `MASTER_PLAN.md` §2.1 is ABSENT and marked *"not needed, because there is no TDoA"* —
   circular with this item, and the circularity is why both have sat still.
3. **A hyperbolic solver.** Chan's closed form or Foy's Taylor-series least squares over
   ≥3 anchors. The smallest piece.
4. **Re-deriving tier-1 sigma.** `A4` measured tier-1 uncertainty as over-stated ~1.7×
   against an RSSI-inversion model. That number does not survive a change of observable.

*Cost:* multi-day, and it invalidates every tier-1 result. *Recommendation on the
record:* do **not** build it unless TDoA is a claim the thesis needs to make. If it is
not, the honest resolution is to say tier 1 localises by RSSI inversion, bill it that
way, and correct `ch09 §9.3` — which is a documentation change, not an engineering one.
The DIVERGENT row already says the code and the spec disagree here; this decides which
one moves.

---

# Addendum — 2026-09-08 (second), the propagation session

Takes precedence over anything above it that it contradicts.

## K. The environment split, and what it does to everything above

### K.1 Tiers 2 and 3 are simulated in a different environment from tier 1 — DEFECT

Full write-up: `report/ENV_SPLIT_DEFECT.md`. In one paragraph: `envFacility` /
`vegFacility` put 2.4 GHz and UWB in `ENV_RURAL_OPEN` + `VEG_SPARSE_SHRUBS` while tier 1
sits in `ENV_RURAL_FORESTED` + `VEG_DENSE_FOREST`. The code says why at
`ca_tbl_causal_sim.cc:1271-1274` — tiers 2/3 "range against anchors AT a facility site" —
which was true of the three-population model and false since the tri-band correction put
all three radios on the same nodes.

**Effect, `boxHalf=2000` seed 7:** `static_2p4` mean neighbours 10.463 → 4.088, fix
cycles 1437 → 863, rmse 2.28 m → 271.96 m; `static_868` moves 0.2%. At the default,
**2.4 GHz out-ranges 868 MHz** against ~27 dB of link budget in tier 1's favour
(17 dB sensitivity + 8.8 dB path loss + 1.5 dB transmit power).

**Why it outranks I.6.** I.6 says the tree is provisional because the *geometry* moved.
This says the *link budget* moved, by tens of decibels, on two of the three tiers. A
geometry change moves which anchors are in range; this changes what "in range" means.

*Status:* measured over 20 paired seeds in cell `k1e_split1000`. Default change written
and null-controlled — see K.2.

### K.2 The default is changed; the old configuration stays reachable

`envFacility` and `vegFacility` now default to a sentinel meaning **follow the wide-area
setting**, and an explicit value still wins. `--envFacility=0 --vegFacility=1` reproduces
a pre-2026-09-08 run byte-for-byte, which is the null control.

**This changes the physics default, so no result in the tree reproduces under it.** They
were already provisional under I.6; this is the second and larger reason, and the two
compound rather than overlap.

*Order of re-running, and it is not the order of the register:*
1. The **integrity-check result** (`report/INTEGRITY_CHECK_RESULTS.md`) — 8/8 seeds on
   energy, 7/8 on accuracy, the strongest finding in the repository. It is a comparison
   *within* tier-2 behaviour and its sign could move.
2. The **PCS forms** campaign, because `gated` was adopted on its evidence.
3. Everything else, per I.6.

### K.3 `CLAUDE.md` §6 states an invariant the code does not hold

§6 says "exactly one tier transmits per cycle, and only that tier is charged". The
**tier-1 floor** breaks the literal statement: when the active tier is 2.4 GHz or UWB and
`FloorRangingWanted()` says yes, `rangeTier(CA_TBL_TIER_868)` runs in the *same* cycle and
`AccountRangingExchange` charges it. Two tiers transmit; both are charged.

The invariant's **purpose** survives — an arm that never escalates still accrues zero on
the tiers it never used, because `runFloor` is false when the active tier is 868 — and
that is the part the energy axis actually needs. But the statement as written misleads in
a specific, checkable way: a reader concludes tier-1 energy is proportional to
`cycles_868`. It is not, and the arithmetic is exact rather than approximate.

`baseline_v8`, one 868 MHz exchange costs `12.098212 / 1440 = 8.4015 mJ` from
`static_868`, where every cycle is an 868 cycle. Dividing each arm's `tx_868` by that
gives the number of 868 transmissions, against `cycles_868`, the cycles where 868 was the
**active** tier:

| arm | `cycles_868` | 868 transmissions | floor cycles | of non-868 cycles |
|---|---|---|---|---|
| `static_868` | 1440 | 1440 | 0 | — |
| `ca_tbl_no_integrity` | 1355 | 1435 | 80 | 80/85 = 94.1% |
| `fixed_threshold` | 541 | 1391 | 850 | 850/899 = 94.6% |
| `ca_tbl_full` | 334 | 1380 | 1046 | 1046/1106 = 94.6% |

**868 MHz transmits on ~96% of every arm's cycles no matter which tier is active.** So
tier-1 energy is very nearly a constant across arms — 13.19 J of `ca_tbl_full`'s 26.66 J,
49.5% — and escalation *adds* tier-2/3 energy on top rather than substituting for tier 1.
The code says as much at `:3614` ("runFloor is true on 99.9% of cycles"); §6 does not, and
§6 is the document a reviewer is told to check first.

This is also why `--floorOnDemand` is the single largest energy lever in the codebase, and
why K1's P3 is worth measuring: it turns that ~96% into "only when the active tier failed
to solve".

*Fix:* restate §6 to separate the invariant that holds — the per-cycle loop owns every
transmission, and a tier that did not transmit is not charged — from the sentence that
does not. *Cost:* a documentation change. *Risk of leaving it:* a reader reasons about
the energy axis from a false premise, which is the exact failure mode §6 exists to prevent.

### K.4 Gateways as edge servers — built, and it can only cost energy

> **2026-09-29: the default since L.2.3.** "It can only cost energy" held under the
> no-reply billing this was written against, and it still holds on tier 2. Under the reply
> billing that became the default on 2026-09-08 it saves 32.9–42.5% on every tier-1-using
> arm (`report/B3_REGEN_RESULTS.md` C6). The text below is kept as written.

The brief puts the EKF fusion on the gateway. `--gatewayEdgeServer` (default off) now
charges the field node for the downlink that returns the answer, which is what the
previous "changes where computation happens, not what is computed" note left out.

**It does not make the ranging network-side, and that is physics rather than scope.**
Tiers 2 and 3 range by two-way time-of-flight *precisely because* the endpoints share no
clock. One-way anchor-side timestamping — where the saving would be, since UWB is 99.7%
receive energy — needs the synchronisation J.2 records as ABSENT, and J.2 records that
the sync row is marked "not needed, because there is no TDoA", which is circular with the
TDoA row. Building it here would be assuming that layer into existence.

So the honest shape of the brief's split under this model is **a cost, not a saving**.
That is a finding about what this simulation can price, not a criticism of the brief: the
split buys things this simulation does not measure — a gateway that fuses across
subjects, and a field node carrying no filter state at all.

### K.5 What the K1 campaign does not settle

- **P3 is a joint effect.** `gated` PCS and floor-on-demand move together; the cell that
  would separate them was dropped for compute. Recorded in Amendment 1 §A1.4.
- **Tier 3 is barely exercised.** Even with four converging finders the smoke run showed
  `meanNbr_uwb` at 0.03 and no UWB fix cycles. H.1 says tier 3 is reached on ~2.5% of
  cycles; under the corrected propagation it may be less. **A three-tier claim resting on
  a tier that fires this rarely is not yet a three-tier claim.**
- **One duration, one mobility model, one fault model.** Every cell is 3600 s of the same
  waypoint walk.
- **The environment is now uniformly `RURAL_FORESTED`.** That is one scenario. The
  correction makes the tiers consistent with each other; it does not make the forest the
  right forest.

### K.6 G4 and G5 are red because the default deployment no longer exercises tier 2

Full write-up: `report/GATE_STATE_2026_09_08.md`. In short: with 2.4 GHz r80 corrected
from 1149.5 m to 156.0 m, the default `boxHalf=1000` with 900 m gateway spacing leaves
`static_2p4` with **41/1440 fix cycles (2.8%)**, and the verifier correctly refuses to
certify an RMSE that is dead reckoning. G5 fails only because its "healthy input" fixture
is that same run; its six negative fixtures all still fire.

Four derived defaults are now contradicted by their own comments, all of them taken from
the split-environment probe: `boxHalfM = 1000` ("868 r80 = 1000 m", measured 1515.3 m),
`t2TriadRadiusM = 150` ("2p4 r80 >= 900 m, siting-limited", measured 156.0 m and
range-limited), and the `uwbTriadRadiusM` / `gwGrid` pair alongside them.

*Fix, in order:* resize the default deployment from a fresh probe — tier 2 needs spacing
below ~312 m, which is `boxHalf` below ~350 — and then generalise the verifier's
single-band coverage exemption from "UWB is exempt" to "an arm whose measured r80 cannot
cover this deployment is a documented finding", with its negative fixture per rule 6.

*Why it is not done here:* K1 measured 924 s/seed at `boxHalf=400` against 157 s at 2500,
so a default of 350 makes the gate suite roughly five times slower on every future run.
That is a decision about the project's tooling budget, not a bug fix to be made at the end
of a session.

### K.7 `--escalationRequiresEvidence` should default true, and does not yet

K3 measured it worth **323.75 J of a 339 J arm**, 20 of 20 seeds, p < 0.0001, zero noise
floor, with no significant accuracy change (`report/K2_K3_RESULTS.md`). It is still
`false`.

*Why not landed:* it is a second physics/policy default change requiring its own
reference regeneration, stacked on a suite already red from the first. Two default changes
in flight before the first is settled is how a tree stops being verifiable. The patch is
written and its help text carries the caveat — the gate is suppress-only and was measured
where tier 3 never delivers, so where tier 3 works suppression could cost.

### K.8 PCS is uncalibrated, and measurement says it re-encodes a number we get free

**Owner decision 2026-09-08: registered as debt, not fixed.** The evidence says fixing it
is probably not the right call either.

*Uncalibrated.* `pcsW1 = 2.0`, `pcsW2 = 0.5`, `pcsW3 = 0.5` are HAND-SET — the header says
so — chosen for one structural property, that `pcsW2 + pcsW3` stays below the lowest
escalation threshold so term 1 keeps a veto. `pcsGateFloor = 0.10` is marked "OURS, not
physical". **CAL-01, the LOOCV weight calibration the blueprint specified, has never been
run**, and it is the direct cause of the saturation defect A8 closed by result.

*And measured to add nothing.* `report/PCS_FORMS_RESULTS.md` §2 ran the decisive control —
does PCS predict realised error better than the EKF covariance trace it is computed from?
**Every form loses, in every cell, with the whole 95% CI below zero.** Legacy loses by
0.283 on `no_integrity`; repairing the saturation closes the gap to −0.0095 (weighted) and
−0.0186 (gated). The best it achieves is coming back up to the control and stopping.

> "It is a re-encoding of the filter's own covariance trace, and a good one once the
> saturation is removed — but it never adds anything the trace did not already carry."

*Worse in the regime that now matters.* `ROOTCAUSE_ENVELOPE` Q2: across densities the
median falls 1.543 → 1.024 against thresholds of 1.55–2.30, so the fraction of cycles
wanting to escalate rises 67% → 98%, and above spacing/r80 = 1.29 ρ(PCS, error) turns
**positive** — higher reported confidence, higher realised error. That is the sparse
regime, which is the search scenario.

*Two of three terms are structurally dead.* The hop term has `hopWeight = 0` and an inert
provenance input; the NLOS term is pinned by an operative tag firing on 75–78% of links
against a propagation determination of 0.3–12.8%. In practice PCS ≈ term 1 ≈ a rescaling
of the covariance trace, with two dead terms attached.

*This is consistent with the campaign results rather than separate from them.*
`fixed_threshold` thresholds the covariance trace directly and beat `ca_tbl_full` on
accuracy in most cells, including 296 m against 764 m in the 4-hour probe. The simple thing
PCS is a re-encoding of works better than the re-encoding.

*Order, if it is ever picked up:* run the monotone control FIRST as an acceptance test,
then CAL-01, then give the hop and NLOS terms real inputs. Not the other way round — there
is no point calibrating a score that has already been measured not to carry information.

### K.9 The SHS gait calibration diverges where fixes are sparse — DEBT, by owner decision

`--imuFusion` gives the filter the dead-reckoned position on no-fix cycles, which is the
interval the inertial engine exists to cover. The fusion path itself is correct and
null-controlled. **What is not usable is the thing being fused.**

MEASURED, seed 7, 3600 s, `--shsTrustedFixK=5.0 --maxTurnRate=0.5`:

| arm | radio fixes | SHS position error | cep95 no fusion → fusion |
|---|---|---|---|
| `static_uwb` | 0 (never calibrates) | **13.0 m** | 749.0 → 52.6 |
| `static_868` | 1433 | 45.4 m | 334.9 → 291.1 |
| `fixed_threshold` | ~220 | 709 610 m | 269.7 → 30 694 |
| `static_2p4` | 7 | 1 897 933 m | 5 172.8 → 2 350 560 |
| `ca_tbl_full` | ~50 | 4 004 307 m | 276.6 → 317 444 |

**The arm that never calibrates is the most accurate one.** A free-running engine on its
0.7 m fallback step is merely imprecise; a mis-calibrated one is confidently, catastrophically
wrong.

WHY. Step length and heading are calibrated from the displacement between consecutive
trusted fixes. The node truly moves ~3 m per cycle and the fixes carry ~167 m of error, so
the measured displacement is roughly 1 part signal to 56 parts noise: the implied step
length is wrong by a large factor and the implied heading — the *direction* of that same
noisy vector — is close to uniformly random. Scale error and heading error then compound
multiplicatively under integration. `lora-step-length-submodule.cc:101` adopts the FIRST
calibration whole, with no EMA history, so on a sparse arm the first bad estimate *is* the
gait model permanently.

`--shsTrustedFixK` (calibrate only on informative displacement) fixed the high-availability
arms — 5–14 km → 20–206 m — but cannot fix the sparse ones, where "sparse and informative"
becomes three samples.

TWO GUARDS ARE BUILT, both default off, both positive-controlled:

- `--shsStepBounds` refuses a calibration implying a step outside 0.25–1.10 m. `chord /
  steps` is an implied step length and a 167 m chord over 3 steps implies 55 m per step. The
  bound is on step length rather than on k_w because step length is the quantity with a
  physiological range; k_w = chord / |Σ q u| has no upper bound at all. This removes the
  divergence: 1.9e6 → 301 m, 4.0e6 → 459 m.
- `--shsSnapPosition` anchors the SHS position at each clean fix. Required by `--imuFusion`
  and only correct together with it: the fusion variance is
  `imuSigmaFloor + imuDrift·(t − lastTrustedFix)`, which asserts the position error is as old
  as the last trusted fix — but a trusted fix calibrates GAIT and does not touch position, so
  unsnapped that error accumulates from seeding and is never reset. Measured 459 m of SHS
  error while the variance claimed 5.4 m, and the filter duly followed it.

**Bounded, fusion is still net-negative on the high-availability arms** (`ca_tbl_full` cep95
276.6 → 1 499.8): a 459 m measurement labelled 5.4 m degrades a filter already at 141 m RMS.
So the guards make the SHS survivable, not yet useful.

DECISION (owner, 2026-09-08): do not fix the gait calibration now. Measure the architecture
against `--imuIdeal`, a PERFECT dead reckoner anchored at the last clean fix and advanced by
true displacement. That is a **CEILING, not a system result** — every figure produced under
it is an upper bound on what a flawless IMU could contribute here, and must be reported as a
bound. It is not error-free: the anchor is the filter's estimate and carries that fix's
error, held constant rather than compounded, which is what separates a perfect inertial
system from an oracle.

WHAT CLOSING THIS WOULD TAKE: an error model for the SHS that is actually measured rather
than asserted, so the fusion variance describes the engine's real accuracy; and a
calibration that is robust to a first bad sample — at minimum, refusing to adopt a first
estimate whole, and preferring a running median over an EMA seeded by one point.

---

# L. Definition of done — 2026-09-09

**Why this section exists.** Everything above records what is wrong. Nothing above says
what "finished" would look like, so every status conversation restarts the argument about
whether the project is 32% or 60% or done. This fixes the bar.

**Read the three bars separately. They are not stages of one thing.** A system can clear
1 and fail 3 permanently, and that is an acceptable outcome — it is what a negative result
IS.

| bar | question it answers | current |
|---|---|---|
| **B1 Engine complete** | does every specified mechanism exist and execute? | **near** |
| **B2 Spec-conformant** | does the running simulator do what `finalyearprojectcontext.docx` describes? | **partial** |
| **B3 Claim-supported** | is there evidence for the claims the thesis makes? | **no, and partly by measurement** |

**2026-09-29.**

- **B1 met.** The one qualification: L.1.2 is met only with `--relaySlots`, which is
  default off.
- **B2 partial.** L.2.3 and L.2.5 are closed; L.2.1, L.2.2 and L.2.4 are partial. An
  exploratory probe corrected the coverage diagnosis for the four starved mechanisms
  (L.2).
- **B3.** The seven claim-bearing campaigns were regenerated (B3), and C1 with the evidence
  gate off and CAL-01 were run after them. Four claims are supported, PCS and tiering are
  not, and the witness cross-check is withdrawn in the shipped default (L.3).

---

## L.1 Bar 1 — engine complete

**Done when:** every mechanism the docx names has code, that code is reachable from a
running configuration, and a negative fixture has been observed to fire for each gate
guarding it.

**Where it stands:** essentially met. 152 checks across eight suites; every docx-named
mechanism greps to code; §51 (the demo scenario) is the only fully-absent item and is not
an engine feature.

**What is left for B1**

| # | item | what is wrong | what closes it | done when |
|---|---|---|---|---|
| L.1.1 | Routing unwired | `CaTblRoutingTable` + ETX + 21 checks exist; **0 scenario call sites**. The poll set is one-hop, so there is no route to choose | Multi-hop forwarding of position-fix traffic: a node that cannot reach a gateway directly relays through one that can | A run reports a next-hop that is not the destination, and `Divergences()` is non-zero between ETX and the confidence metric. **2026-09-29: MET at `5f3ad42`**, behind `--fixRouting` (default off) — see *L.1 status* below |
| L.1.2 | Net §5 depth 2 unreachable | `CaTblComposeRelayChain` and `--maxRelayDepth` exist; **finders never relay through each other**, so `hopDepth` is always 1 | Let a finder with <3 anchor ranges range against a finder that already holds a fix this cycle, inheriting depth+1 and the relay's covariance | `hopDepth == 2` appears in a run and A2 is observed both admitting and rejecting on the budget. **2026-09-29: MET only with `--relaySlots` (`d24690c`, default off); NOT met by `b390ed6` alone** — see below |
| L.1.3 | Case B / Case C unwired | library only | Call Case B where one anchor + one peer is the geometry | Case B appears in a run's verdict counts. **2026-09-29: already met at `084a7ff` (wired at `ac5766f`); this row was stale** — see below |
| L.1.4 | Anchor tie-break unwired | `CaTblGrantCoordinator` + suite; anchors do not gossip grants | Anchor-to-anchor grant gossip on the existing low-priority class | Two anchors are observed reaching opposite verdicts on one target in a run. **2026-09-29: MET at `17ab7dd`; its evidence file is digested since `ff4f8bb`** — see below |
| L.1.5 | `CA_TBL_MOB_RECEDING` never set | the receding → eager-push branch is dead code in the scenario | Drive mobility state from the walker's range trend | The branch executes and `TRIGGERED_UPDATE` count is non-zero. **2026-09-29: already met at `084a7ff` (wired at `50a229a`); this row was stale** — see below |

**B1 is worth finishing and is cheap.** None of it requires a campaign or a decision.

### L.1 status, 2026-09-29

Work on branch `work/2026-09-27-spec-b1`, local only. Every mechanism below sits behind a
default-off flag whose off state was null-controlled against the frozen `084a7ff` reference
(`~/catbl_ref/nc.sh`: NC1 7/7 and NC2 2/2 output files byte-identical). The branch review
(2026-09-29, working files `~/catbl_review/`) re-ran every done-when invocation at the branch
head with `--gatewayEdgeServer=false` and got the implementers' outputs back byte-for-byte, 8
of 8.

- **L.1.1 — CLOSED, `5f3ad42`.** `--fixRouting` and `--fixRoutingConfidence`. The routers
  are the filtered relays (ids 2+, `--relayedRanging`); the measured tag does not route,
  because its tier changes between cycles. Done-when run: `static_2p4`, seed 7, 600 s,
  `--relayedRanging --nTags=6`, default box (`~/catbl_probe/v3/L.1.1/donewhen_etx`).
  `fix_routing_static_2p4_seed7.csv`, 1981 rows: the next hop was a relay on 1116 hops, and
  781 of those were forwarded. Divergences were 576 of 1958 selections, equal to
  `CaTblRoutingTable::Divergences()` summed over the tables, and an awk recount gives the
  same numbers. Gates G18/G18b, 12 negatives observed to fail. **Caveat, from `d24690c`:**
  these counts were measured while the relays' ranging still shared the cycle instant (M.1
  below), so they do not compare with a `--relaySlots` run. The weights α, β, γ and the slack
  are ours and uncalibrated.
- **L.1.2 — MET only under `--relaySlots` (`d24690c`), which is default off. The owner has
  not yet decided its default.** `b390ed6` wired `--relayChain` and measured the done-when
  as NOT met. In G17's regime it got 112 rows, all at depth 1, A2 pass 49 / budget 63, and
  0 fuses. The cause is the simultaneous-instant defect (M.1). With `--relaySlots`, same
  flags (static_2p4, seed 7, 600 s, `--boxHalf=200 --nTags=4 --relayedRanging --relayChain
  --maxRelayDepth=2 --relayVarBudget=24336`), `relay_chain_static_2p4_seed7.csv` has 541
  rows: 4 fuse rows, 40 tag-leg rows at `hopDepth` 2, and A2 at depth 2 passing 19 and
  refusing 21 on the budget. Seeds 11 and 13 pass G17b too, with 57 and 2 fuses. G17b has
  been green since `d24690c`. `hopWeight` stays 0. The earlier probe outputs
  `~/catbl_probe/v3/L.1.2/{on_*,sw_*,sx_*,sy_*,sz_*}` came from a defective draft that set
  depth on admission rather than on a fused step, and **must not be cited**.
  *2026-09-30:* `--relaySlots` became the default in relayed runs (M.1). G17's flags as
  written now give the 541 rows above, and G17b reads G17's run.
- **L.1.3 — CLOSED before this week; the row was stale.** Case A reached the fix path at
  `c94507b`, Case B at `ac5766f`, and the census became a partition at `0031a5b` (A.7).
  Probe of 2026-09-28, EXPLORATORY (unregistered, frozen `084a7ff` binary). Invocation
  `--arms=ca_tbl_full --seed=7 --duration=1200 --sparseCaseA=true`; field: stdout
  `sect 7 CASES`; census complete, 480 of 480 cycles:
  - `--nGateways=3 --boxHalf=3000 --nFinders=4`: D 65, A 7, **B 16**, C 199.
  - `--gwGrid=2 --boxHalf=5000 --nFinders=4`: D 54, A 1, **B 11**, C 268.
  - `--nGateways=3 --boxHalf=2000 --nFinders=0`: D 39, **A 64**, B 0, C 318.

  `ac5766f`'s message gave B = 8 and 13; those were counted before the census fix. B is also
  18, 29 and 26 at seeds 7, 11 and 13 in the grid-2 5 km regime with four finders (1800 s).
  Output directories `~/catbl_probe/v3/a_case*`.
- **L.1.4 — CLOSED, `17ab7dd`; evidence digested since `ff4f8bb`.** `--anchorGrantGossipS`
  (default 0). Done-when run: NC2's flags plus `--anchorGrantGossipS=5 --vizEvents=true`,
  `ca_tbl_full`, seed 7, 1800 s. `grant_verdicts_ca_tbl_full_seed7.csv` has 22 rows: request
  OK 13, SELF_REPEAT 2, gossip WIN 4, DEFER 3. There are two mutual opposite-verdict pairs on
  target 1: 104 WIN / 103 DEFER, and 105 WIN / 104 DEFER. An awk recount equals the stdout
  counters. It reproduces byte-identically from a clean clone (`source 17ab7dd dirty 0`,
  `~/catbl_probe/v3/L.1.4/verify_clean`). Gate G16 has 7 negatives. `ff4f8bb` adds
  `grant_verdicts_*.csv` to the `run_id.txt` digest; before it, a tampered verdict passed
  `sha256sum -c`. Limits:
  - gateway 101 received no gossip;
  - one double grant stood (105 from 485 s, 102 from 490 s);
  - there is no revoke frame;
  - one seed, one deployment.
- **L.1.5 — CLOSED before this week; the row was stale.** `--mobilityFromRange`
  (`50a229a`). Probe on `084a7ff`, EXPLORATORY. Invocation `--arms=ca_tbl_full --seed=7
  --duration=1200 --mobilityFromRange=true --nTags=3 --vizEvents=true`. Stdout reads
  `sect 11.11 MOBILITY: stationary 100, approaching 177, receding 168`: 445 of 480 cycles,
  and the 35 cycles with no range are unclassified. `events_ca_tbl_full_seed7.csv` has 44
  `TRIGGERED_UPDATE` frames originated by node 1 (748 rows, including 174 relayed copies).
  The only sender of that message is the RECEDING branch of `ca-tbl-mac.cc`. The same run
  with `--mobilityFromRange=false` gives 0. With `--nTags=1` it gives 49, so `50a229a`'s
  statement that peer tags are needed no longer holds at `084a7ff`. No counter reports this
  message, and the scenario comment saying the receding branch never runs is now false (M.9).

**B1, 2026-09-29: met.** One qualification: L.1.2's done-when needs `--relaySlots`, which is
off by default until the owner decides.

> **2026-09-30:** the owner decided (M.1). `--relaySlots` is on by default in relayed runs,
> so L.1.2's done-when holds with G17's flags as written. `--relayChain` and
> `--maxRelayDepth=2` are still opt-in.

---

## L.2 Bar 2 — spec-conformant

**Done when:** for each docx section, the *running simulator* does what the section
describes, or the divergence is recorded in `MASTER_PLAN.md` §2.1 with a reason.

**The honest obstacle, and it is one fact rather than five.** Four mechanisms are correct,
reachable and have **nothing to act on**:

| mechanism | population | why |
|---|---|---|
| Covariance Intersection | 0 firings | equal-sequence entries never arrive; dedup drops the duplicate |
| Case A | ~8 attempts / 480 cycles | the tier-1 floor solves first |
| Mahalanobis integrity check | 0 eligible cycles | it runs on all-LOS cycles; there are none |
| Routing | no route to select | poll set is one-hop |

**All four are the same fact: tier 1 blankets the box.** `runFloor` is true on 99.9% of
cycles, `static_868` records zero no-fix cycles, and every mechanism built to handle
scarcity never encounters any.

**This is the single highest-value thing left in the project**, because it is what stands
between "the mechanism exists" and "the mechanism does something". It is also cheap to
test and expensive to ignore.

*What closes it:* a deployment where tier 1 does **not** cover — a larger box, a lower
tier-1 power, or an arm with `--floorOnDemand`. `TECH_DEBT` K.3 records that the floor is
the single largest energy lever in the codebase and that ~96% of every arm's cycles carry
an 868 transmission regardless of active tier. A sweep over that is one afternoon.

*Done when:* each of the four reports a non-trivial population in at least one registered
configuration, **or** the register states that it cannot and why.

**2026-09-29: the coverage probe (EXPLORATORY) corrects this diagnosis.** It was about 40
runs, unregistered, all on the frozen `084a7ff` binary (`~/catbl_ref`). Invocation
`--arms=ca_tbl_full,static_868 --sparseCaseA=true --tableFuseCI=true`, 1800 s (720 cycles),
mostly seed 7, a few at seeds 11 and 13. Outputs are in `~/catbl_probe/v3/{s_*,f_*}` and the
per-run populations in `~/catbl_probe/v3/*_pops.jsonl`. The sidecars read `source unknown`,
because the frozen binary runs outside a git tree and `run_id.txt` records the working
directory's HEAD (M.3); the runs are identified by the binary. This re-diagnoses the four
mechanisms. It does not measure an effect, and no result may be quoted from it.

**"All four are the same fact" is wrong for two of the four.** Counts are for `ca_tbl_full`,
per 720-cycle run:

| mechanism | what the probe shows | diagnosis |
|---|---|---|
| Case A / B | Default box: A 7 (9 attempts), B 0. Where tier 1 does not cover, A rises to 103 (`--nGateways=3 --boxHalf=2000`), 60 (`=3000`) and 48 (`--boxHalf=3000`). B is 10–29 with four finders in the `--nGateways=3 --boxHalf=3000` and grid-2 5 km boxes, and 0–1 elsewhere | **Coverage.** Tier-1 blanketing explains Case A and Case B, and a sparse regime produces both |
| Mahalanobis integrity check | Eligible cycles (`mahalanobis != 0 && nNlos == 0`): 0–9 in every regime under the default `--nlosSource=legacy`, sparse or dense, with or without finders. In the six runs with `--nlosSource=propagation`: 182–420 eligible, 102–288 suspects | **Structural, not coverage.** The legacy tag marks ~75 % of links NLOS, so an all-LOS fix cycle almost never occurs. The propagation source changes that |
| Covariance Intersection | With four finders, every regime from the default box to grid-2 at 5 km has 107–318 table-entry arrivals per run, each merged. Equal-sequence (CI-eligible) arrivals: 0 in 11 of 13 finder runs, 8 and 1 in the other two. With no finders: 0 arrivals | **Not coverage.** Entries arrive whatever tier 1 does. A second copy of the same sequence number almost never does. *Inferred, not counted:* an entry reaches a holder only as its subject's unicast answer to that holder's own digest (`f017da4`), so a second route rarely carries it |
| Routing | Wired since `5f3ad42` (L.1.1): 1116 relay next hops in its done-when run | A population exists under `--fixRouting` |

`runFloor` (a hybrid arm whose active tier is not 868) is true on 600 of 720 cycles at
defaults, 718 of 720 with four finders, and 25 of 720 in grid-2 at 5 km. The 99.9 % quoted
above is the finder regime, not the default. What remains for the done-when is a
*registered* configuration for each mechanism. CAL-01's `arc` regime (44 % of cycles
without a fix, `results/cal01_loocv_20260928`) is the first registered campaign in which
tier 1 does not blanket.

**Remaining B2 divergences**

| # | item | what is wrong | what closes it | done when |
|---|---|---|---|---|
| L.2.1 | Tier-1 observable | Spec says TDoA (Chan/Foy). Code does RSSI inversion. The energy model bills the **TDoA shape** — one broadcast, anchors timestamp — so the system pays for TDoA and localises with RSSI | Either build the timing layer + hyperbolic solver (needs GPS-PPS sync, absent, and circularly justified as "not needed, because there is no TDoA"), **or** re-bill as RSSI and amend ch09 §9.3 | Billing and observable agree, and the DIVERGENT row names which one moved. `--tier1ChargeReplies` already measures the gap: tier-1 energy 13.59 → 25.86 J. **2026-09-29: the observable moved, behind `--tier1Tdoa` (default off), `7377ed0`; the default is still RSSI** — see *L.2 status* below |
| L.2.2 | NLOS determination | Spec gives a CIR leading-edge/peak ratio. Code uses a probabilistic propagation state. **No CIR exists, so no classifier can** | Synthesise a CIR from the channel state and run the leading-edge test on it | The classifier produces a confusion matrix against the modelled truth. **State the circularity**: it cannot discover NLOS the channel model did not create. **2026-09-29: PARTIAL at `86e43f2`. The 868 MHz and 2.4 GHz matrices fill; the UWB CIR matrix cannot, under the channel model** — see below |
| L.2.3 | Estimator location | Spec puts the EKF on the gateway; code ran it on the tag. `--gatewayEdgeServer` exists (32–42% energy saving, accuracy byte-identical) and was **off by default** until 2026-09-29 | Default it true after its own reference regeneration | **CLOSED 2026-09-29 (`db2ac2e`) — the default is now true**, and MASTER_PLAN §2.1's CONTRADICTION row is PARTIAL, with the reason stated in the row. Evidence at the flipping commit, all on one binary: defaults against `--gatewayEdgeServer=false` differ ONLY in totalJ, accuracyPerJoule, the 868 and 2.4 GHz `j_`, `rx_`, `idle_` columns and the trace's `cumJ`, in four configurations (seed 7 × 3600 s all arms traced; the NC2 searcher regime; seeds 11–13 × 900 s; NC2 with `--uwbArbitration`); `static_uwb` is byte-identical, since it never fixes. `=false` reproduces the 084a7ff frozen reference byte-for-byte (NC1 7/7, NC2 2/2). A new frozen reference at the flipping commit, ~/catbl_ref_edge, serves later null controls. Gates that replay the pre-change configuration now pin `=false` (G6's comparison run, G19's replies-billed run), and G21 (`scripts/edge_default_gate.py`) re-asks the energy-only question on every suite run, with negative fixtures in `scripts/edge_default_gate_selftest.sh`. **Recorded, not fixed:** getting a tier-2 or tier-3 range from the exchange that measured it to the gateway's estimator is not billed. On 2.4 GHz the node closes the two-way arithmetic, so the range would have to be sent up; on UWB the final message leaves it with the responder, which may be a finder. This flatters the edge setting on tiers 2 and 3 only, where it already costs 0.2% more |
| L.2.4 | Multi-hop localisation | Flood forwarding exists; the poll set is strictly one-hop and relayed ranging is capped at depth 1 | L.1.2, then raise the cap | `hopCount` takes more than two values. **2026-09-29: PARTIAL. Met in the relay-chain log under `--relaySlots` (`d24690c`); not met where PCS reads it; the cap still defaults to 1** — see below |
| L.2.5 | Proxy staleness | *As recorded 2026-09-09:* "the proxy answers only inside 120 s and at this deployment entries are normally older, so it refuses (7 refusals / 0 answers at the spec bound)" — from an unrecorded invocation that queried a **peer**. **Measured 2026-09-29, and true of a peer, false of the casualty** (registered `report/PREREG_L25_PROXY_AGE.md`, runs at `b16531f` dirty 0, `results/l25_proxy_age_20260929`, seeds 7/11/13, the age of every entry a proxy held when a query arrived). Casualty, arc searcher regime: 458 checks of 82 queries, 265 held, age p50 11.5 s, p90 28.6 s, max 68.1 s, **265/265 inside 120 s**. Casualty, grid-2 5 km sparse regime: 42 checks of 15 queries, 39 held, p50 17.2 s, p90 49.6 s, **37/39 inside**. A peer: p50 493 s (21/130 inside) and 385 s (5/20) — an entry reaches a holder only as its subject's answer to that holder's own digest, and in the seed-7 searcher run the peer heard 13 finder digests to the casualty's 305 | Nothing, for the casualty. For a node nobody polls the lever is dissemination, not the bound; `--digestSummary`'s third-party pushes are the existing candidate and were **not** measured here | **CLOSED 2026-09-29 (`b16531f` registered, `f017da4` measured) — justified by measurement.** The spec's 120 s stays the default of `--queryStalenessBoundS`, whose help text now carries these numbers: it refuses none of the casualty's held entries in the searcher regime and 2 of 39 in the sparse one, and what it refuses is worth refusing — an estimate held 120 s is 143 m from a casualty walking 1.2 m/s, against 4.5 m fresh |

**Found while closing L.2.5, recorded and not fixed.** Fixing either changes what runs
produce, so each is a change of its own, with its own null control:

- **The C3 QueryDriver asks only 868 radios.** Its loop (`scratch/ca_tbl_causal_sim.cc`,
  the `identityQueryPeriodS > 0` block) binds every tier's driver to the *first* ANCHOR MAC
  in `macs`, with no tier test. Under the tri-band topology that is gateway 101's 868 MAC,
  so all three drivers transmit on the 868 channel and only 868 radios hear a query; a
  "tier 1" or "tier 2" query is an 868 frame evaluated with the 2.4 GHz or UWB link model,
  and a UWB-labelled one is timed as a UWB frame. Every identity-query and proxy count to
  date carries this. It does not move L.2.5: the only
  tables holding a casualty entry are the finders' 868 tables, which are the ones asked.
  - *2026-10-01, NW-3:* `--queryTierBinding=per-tier` binds each driver to its tier's
    radio (gate G32).
  - More precisely, what the old binding did: G32 decomposes the bound MAC's QUERY
    airtime exactly. The "2.4 GHz" queries were billed at 868 MHz's time on air, and the
    UWB-labelled ones at the UWB frame time, all on the 868 MHz radio.
  - The default is unchanged, so this entry stands for every result so far.
- **The casualty's entry is one cycle older than its stamp.** `PublishSelfEntry(policy.EstX
  ...)` runs before `policy.Step` in the per-cycle loop, so the entry stamped `t` carries
  the estimate of `t − dt`: 264 of 265 held entries in `searcher_t1` and 39 of 39 in
  `sparse_t1` equal the trace estimate of the cycle before their timestamp. Every entry age
  in the project is understated by 2.5 s.

### L.2 status, 2026-09-29

- **L.2.1 — PARTIAL, `7377ed0`.** Flags: `--tier1Tdoa` (default false) and
  `--tdoaPpsSigmaNs` (default 20 ns, from the u-blox M8T timing data sheet). Surveyed
  gateways stamp the node's tier-1 broadcast. Four or more gateways are solved by Chan–Ho
  seeding Foy; exactly three by a closed form that keeps every root.
  - *Done-when run:* defaults plus `--tier1Tdoa`, all arms, seed 7, 3600 s
    (`~/catbl_probe/v3/TDOA/dw2_on`). `static_868` gives 1433 rows, OK on 1428 (99.7 %),
    all fused, equal to the policy's own count; `ca_tbl_full` gives 1172 rows, 1166 fused.
    An awk recount is equal on every arm.
  - *Billing and observable agree with the flag on:* 8880 stamps on 8880 billed replies, and
    0 stamps beyond a billed reply. Under the gateway-estimator default (`db2ac2e`) the
    stamps ride the backhaul: 355 downlinks on 355 OK solves.
  - *Why not closed.* The default observable is still RSSI inversion. Making TDoA the default
    changes the tier-1 observable and, per J.2, invalidates every tier-1 result. That is a
    planned campaign (owner decision pending), not a flag flip. The accuracy the model gives
    (`static_868` rmse 167 → 11 m, seed 7) rests on a Cramér–Rao bound with no threshold
    effect and is not a result. J.2 item 4, re-deriving tier-1 σ, is not done.
- **L.2.2 — PARTIAL, `86e43f2`.** Flags: `--nlosClassifier`, `--pcsNlosClassifier` and
  `--nlosScoreThreshold` (0.5, ours).
  - *Every cell non-trivial on 868 MHz.* Searcher regime (NC2's flags, 1800 s),
    `ca_tbl_full`: tp 19, fp 358, fn 177, tn 4474. Defaults at 900 s: 9 / 91 / 147 / 1848.
    On 2.4 GHz (`static_2p4`, searcher regime): 3 / 67 / 6 / 2470. An awk recount from the
    raw rows is equal.
  - **The UWB CIR half cannot fill its TP and FN cells in any regime tried (M.2).** The
    circularity is stated in the source: the classifier can only rediscover the NLOS the
    channel model created.
    *2026-09-30:* still true with the registered IEEE 802.15.4a model (`--uwbNlosModel`,
    `57225b8`). Its NLOS frames are almost all lost, and the few decoded ones are too weak
    to score (`report/UWB_NLOS_RESULTS.md`).
  - The 868 detector barely separates LOS from NLOS (mean score 0.089 against 0.109). CAL-01
    then measured the live PCS term at |ρ| ≤ 0.06.
- **L.2.3 — CLOSED, `db2ac2e`** (row above). The branch review re-ran the null control at
  `db2ac2e` with `--gatewayEdgeServer=false`, plus 11 further configurations against the
  frozen `084a7ff` binary, and every one was byte-identical. `ff4f8bb` is null-controlled at
  defaults against the new frozen reference `~/catbl_ref_edge`. Every results directory in
  the tree, `results/regen27_*` included, was produced with the estimator on the node; to
  replay one, add `--gatewayEdgeServer=false`. An open question the review raised about
  which radio the downlink is billed on is M.10 (a DECISION since 2026-09-30).
  - *2026-10-02, SY-1:* `--edgeUplinkRanges` bills the uplink the row above records as
    unbilled: one frame on the tier that ranged, 9 header bytes plus 6 per answered anchor
    (OURS). Gate G35: only energy moves, only on tiers 2 and 3, at one TX power per radio
    (2.4 GHz: 0.0792 J per second of airtime on every arm that moved). On UWB the frame
    goes from the node too. The spec's TWR is Poucet's START/ACK/DATA_REPLY, which leaves
    the range at the initiator, so "the final message leaves it with the responder" above
    describes a different exchange. The UWB branch is not exercised today, because no
    regime returns a UWB range to the node (DC-6). The default is unchanged, so the edge
    figures above still describe an unbilled uplink.
  *Decided 2026-09-30:* the downlink is billed on the fix's tier by default
  (`--edgeDownlinkTier`). To replay an edge-server run from before that date, add
  `--edgeDownlinkTier=active`.
- **L.2.4 — PARTIAL.** `hopDepth` takes 1 and 2 on relayed legs, beside the direct anchors'
  0, in `relay_chain_*.csv` under `--relayChain --relaySlots --maxRelayDepth=2` (`d24690c`).
  So the done-when holds in the relay-chain log. It does not hold where PCS reads a hop
  count: the measured tag's `Step()` receives `hopCount` as the literal 0
  (`scratch/ca_tbl_causal_sim.cc:8009` at `ff4f8bb`; CAL-01 cites `:7874` at `c019c0a`).
  `hopWeight` stays 0, and `--maxRelayDepth` still defaults to 1, so "raise the cap" is not
  done at defaults.
- **L.2.5 — CLOSED, `b16531f` (registration and the age record) and `f017da4` (result)**
  (row above).

---

## L.3 Bar 3 — claim-supported

**Done when:** every claim in the thesis has evidence that survives its own registered
control, or is withdrawn.

**This bar is partly unreachable, and that is a finding rather than a failure.**

| claim | status | what would change it |
|---|---|---|
| Confidence-awareness (PCS) earns its place | **measured negative.** Every form loses the monotone control, whole 95% CI below zero. In the sparse regime ρ turns **positive**. **→ 2026-09-29: NOT SUPPORTED on the regenerated tree (B3 C2), and CAL-01 finds that no calibration rescues it** — see *L.3 status* | Give the hop and NLOS terms real inputs (both currently dead), then re-run the control. K.8 gives the order and warns a refit alone cannot fix it |
| Tiering beats single-band | **not supported.** Statistically indistinguishable from `static_868`; `fixed_threshold` ties or wins. **→ 2026-09-29: still NOT SUPPORTED (B3 C4). The three small-box DOMINATES verdicts are a billing effect** — see below | A regime where tier 1 does not cover — the same fix as L.2 |
| The architecture beats published estimators | **supported.** K10: 2 016 m vs MLAT 20 831 m, 20+/0−, p ≤ 0.0004. **→ 2026-09-29: REPRODUCES on the corrected environment (B3 C3)** | Re-run on the corrected environment (K.2) before quoting. *Done, B3 C3* |
| The integrity check earns its place | **supported, narrowly.** 8/8 energy, 7/8 accuracy, both CIs excluding zero — but corroboration alone; the IMU witness contributes ~nothing. **→ 2026-09-29: WITHDRAWN in the shipped default (B3 C1, no population). With the evidence gate off, the witness lowers energy on 20/20 seeds, but it acts as a near-blanket veto and no detection claim follows (C1_NOGATE)** — see below | Re-run post-K.2; calibrate the corroboration constants (k=3, σ=30 m, floor 0.5 — all ours, uncited) |
| Hop-aware provenance | **no population.** `hopWeight = 0`, depth capped at 1. **→ 2026-09-29: still no population for the claim. Depth 2 is now reachable in the relay chain, but PCS's hop input is a literal 0** — see below | L.1.2 then a registered PCS campaign |
| *(row added 2026-09-29)* The mesh carries availability | **supported (B3 C5).** `fixCycles / steps` 0.041 < 0.319 < 0.647 (WAN only < sparse < WAN + mesh); WAN + mesh minus WAN only is positive on 20/20 seeds, p = 1.9×10⁻⁶. The availability belongs to the mesh, not to tiering | — |
| *(row added 2026-09-29)* Estimation at the gateway saves node energy | **supported (B3 C6); the default since `db2ac2e`.** totalJ is 42.5 %, 32.9 % and 32.9 % lower on `static_868`, `fixed_threshold` and `ca_tbl_full`, 20/20 seeds each, with accuracy byte-identical on 20/20. It is a tier-1 mechanism: `static_2p4` costs 0.2 % more on 20/20 | Bill the delivery of a tier-2 or tier-3 range to the gateway (L.2.3). *2026-09-30:* the downlink-tier DECISION (M.10) would move the two hybrid figures, not `static_868`'s. *Decided the same day:* the downlink is billed on the fix's tier by default. The two hybrid figures describe `--edgeDownlinkTier=active` and have not been re-measured |
| *(row added 2026-09-29)* An ideal IMU collapses the error tail | **supported as a CEILING, never as a system result (B3 C7).** cep95 median: `static_uwb` 8 413 → 17 m, `static_2p4` 27 151 → 537 m, both 20+/0−. `ca_tbl_full` 11+/9−, not significant | A measured SHS error model (K.9) |

**Before any of these can be re-stated, two things must happen and they are not optional:**

1. **I.6 + K.2 — regenerate the evidence tree.** Most of the 98 results directories
   predate the topology correction, and *all* of them predate the environment-split
   default change. `TECH_DEBT` K.2: *"no result in the tree reproduces under it."*
   Order: the integrity result first (its sign could move), then the PCS forms, then the
   rest. **Compute, not design.**
2. **1.1 — the unseeded sampler.** Any change in transmission count displaces every
   link's draws, so cross-binary magnitude comparisons are inadmissible. Either assign
   streams and regenerate everything, or state the limit on every cross-binary number.

### L.3 status, 2026-09-29

**The two prerequisites.**

1. *Regenerated for the seven claim-bearing campaigns.* B3 (`report/PREREG_B3_REGEN.md`,
   registered at `7b17bd6` before any cell ran) produced 125 cells, all
   `source 7b17bd6 dirty 0`. The engine is `084a7ff`'s, byte-identical, and the cells are in
   `results/regen27_*`. Read-out: `report/B3_REGEN_RESULTS.md`. The rest of the results tree
   still predates the correction.
2. *The sampler decision is taken, for B3 and after it.* `LoRaDistributionSampler` stays
   unseeded (`report/PREREG_B3_REGEN.md` §1). Every regenerated number stands on its own
   paired control inside one binary. Cross-binary magnitudes are inadmissible; byte
   identity is admissible, because it is a control-flow fact.

**Scope of every verdict below.** B3's engine is `084a7ff`'s, and C1_NOGATE ran on the same
binary. This branch has since added the NLOS term, tier-1 TDoA and the relay changes, all
default off and null-controlled. It has also moved the estimator to the gateways by default
(`db2ac2e`), and at defaults that move changes energy columns only. So the accuracy verdicts
carry over to the current default, and every **energy** figure describes
`--gatewayEdgeServer=false`.

- **PCS — NOT SUPPORTED.**
  - *B3 C2* (`results/regen27_pcs_forms`, 20 seeds × 3600 s):
    - The monotone-control negative reproduces in 6 of 8 form × arm cells, each CI wholly
      below zero (legacy/`ca_tbl_full` −0.2349 [−0.2819, −0.1846]; weighted/`no_integrity`
      −0.0984).
    - It is undefined for `gated`, whose PCS is 0.0 on 20 752 of 20 755 and 20 765 of 20 768
      fix cycles. The analyser prints "indistinguishable" for that NaN (M.8).
    - The D-1 repair reverses (SIGN FLIPS): `gated` is worse than `legacy` on cep50 by
      +12.96 m, 15+/5−, p = 0.041.
    - "`gated` is cheaper" is withdrawn: 18 of 20 seeds are byte-identical.
    - The repaired forms saturate *low*. With `traceSigma` at 7.8×10⁴–1.2×10⁵ m², term 1 is
      about 10⁻⁴, so under each of them `ca_tbl_no_integrity` is byte-identical to
      `fixed_threshold` on 19 of 20 seeds.
    - The sparse-regime positive ρ did not appear at `boxHalf=2000`.
  - *CAL-01* (`report/CAL01_LOOCV_RESULTS.md`, `results/cal01_loocv_20260928`, registered at
    `c019c0a` before any cell ran, 60 runs `dirty 0`): **CALIBRATION DOES NOT RESCUE PCS**,
    the outcome prediction P1 named.
    - Leave-one-seed-out against ρ(−traceSigma, error), 98.33 % intervals: A =
      −0.0016 [−0.0066, +0.0030] (default), +0.0045 [−0.0129, +0.0217] (arc, 44 % of cycles
      without a fix) and −0.0011 [−0.0044, +0.0001] (searcher). All three are
      indistinguishable.
    - The calibrated weights are (3, 0, 0) to rounding, and rank agreement with the trace is
      0.984, 0.963 and 0.9997.
    - The live NLOS term ranks error at |ρ| ≤ 0.06.
    - The hand-set (2, 0.5, 0.5) loses by −0.47, −0.30 and −0.21.
    - All five predictions held, and the default weights in code are unchanged.

  The contribution claim, that PCS carries information about error beyond the trace it is
  computed from, is not supported by any linear calibration of its terms. Withdrawing the
  novelty claim is the owner's decision (`report/CAL01_LOOCV_RESULTS.md` §5). What could
  still change the conclusion: a nonlinear form; a hop count that actually reaches the
  measured tag's PCS; or an NLOS detector whose score carries information the channel
  model's own state does not hand it.
- **Tiering beats single-band — NOT SUPPORTED.** *B3 C4* (`results/regen27_k1`, 11 cells ×
  20 seeds).
  - P4.1 ("`ca_tbl_full` dominates `static_868` in no cell") is **FALSIFIED in 3 of 11**:
    `k1a_box750`, `_box1000` and `_box1500` give DOMINATES (energy 20+/0− in each; rmse
    18+/2−, 11+/9−, 13+/7−). P4.2 (`static_2p4` dominates `ca_tbl_full`) reproduces in
    11/11.
  - **The three DOMINATES verdicts are a billing effect, not evidence that escalating to a
    precision tier pays.** Tier-1 replies are billed, so a tier-1 cycle costs 18.5–24.0 mJ
    against 5.5–8.6 mJ for a tier-2 cycle. With floor-on-demand, the 78–88 % of cycles
    spent on 2.4 GHz replace tier-1 time: escalating *leaves* the dearer tier.
  - Against `fixed_threshold`, `ca_tbl_full` dominates at `k1a_box750` and `k1a_box1000`,
    is incomparable at `k1a_box1500`, and is dominated in the other eight cells.
  - C3's P3.2 agrees: `ca_tbl_full` is indistinguishable from `static_868` in every moving
    cell.
  - The billing that produces the three cells is the node-side reply bill, which the
    gateway-estimator default removes. Those cells have not been re-run under it.
- **Beats published estimators — SUPPORTED.** *B3 C3* (`results/regen27_k10`): P3.1
  reproduces in all three moving cells. cep95 medians: 6 665 vs MLAT 13 762 m (19+/1−),
  6 759 vs 38 599 m (18+/2−), and **2 016 vs 20 831 m (20+/0−, p = 1.9×10⁻⁶)**. It is a
  filter beating a memoryless solver, and not tiering: `ca_tbl_full` escalates 0 times on
  20/20 seeds in `wanonly_move`, and P3.2 holds.
- **The integrity check — WITHDRAWN in the shipped default.** Three mechanisms are kept
  distinct (`CLAUDE.md` §2):
  - *The witness cross-check* (`--imuCrossCheck`, `--corroborationCheck`; the policy block
    commented SWITCHING-POINT INTEGRITY), which is what this row's original 8/8 measured.
    - B3 C1 (`results/regen27_integrity`): **NO POPULATION → withdrawn.** Off and on rows
      are byte-identical on 8/8 seeds, although the IMU witness refused 203–232 escalations
      per seed. The inferred cause is that the evidence gate, default true since `7ec0d91`,
      absorbs every refusal.
    - C1 with the evidence gate off (`report/C1_NOGATE_RESULTS.md`,
      `results/regen27_integrity_nogate`, registered at `ddae10f`, 20 seeds × 600 s):
      energy **REPRODUCES**, ΔtotalJ −104.86 J, CI [−107.97, −101.12], 20−/0+,
      p = 1.9×10⁻⁶. Accuracy is **NOT SHOWN WORSE**, ΔrmseM −0.66 m, CI [−9.62, +11.24];
      four seeds are worse by 10–89 m.
    - The IMU witness refused on 204–235 of 240 cycles (85–98 %), against a step-and-heading
      engine 287–4 070 m from truth. It is a near-blanket veto on escalating into UWB, where
      the neighbour table held under 0.1 entries per cycle. **No integrity, trust or
      detection claim follows.**
    - The corroboration witness has never refused, in 28 runs: its effect is unmeasured, not
      zero.
  - *The Mahalanobis Switching-Point Integrity Check*: not re-tested by B3. Claim 9 remains as
    recorded, pre-correction and provisional. Its eligible population under the default
    NLOS source is structurally near-empty (L.2, exploratory probe).
  - *The evidence gate*: its K3 result was not in B3's scope.
- **Hop-aware provenance — still no population for the claim.** Depth 2 is reachable in the
  relay chain only under `--relayChain --relaySlots --maxRelayDepth=2` (L.1.2). The measured
  tag's `Step()` receives `hopCount` as a literal 0, and `hopWeight` stays 0 (`CLAUDE.md` §2).
- **Mesh availability, gateway estimation, IMU ceiling — SUPPORTED**, as in the three added
  rows (B3 C5 over C3's cells; C6 in `results/regen27_edge`; C7 in `results/regen27_imu` and
  C3's `wanmesh_move`). The IMU figure is a ceiling and is never quoted as a system result.

---

## L.4 When the system is truly done

**B1 done:** L.1.1–L.1.5 wired, each observed to execute. *Estimate: 1–2 days.*

**B2 done:** the coverage question settled (a regime where tier 1 does not blanket), plus
L.2.1–L.2.5 resolved or recorded. *Estimate: 3–5 days, one of which is compute.*

**B3 done:** the tree regenerated on the corrected environment, the sampler decision made,
and every claim either re-established or withdrawn in the corrections register.
*Estimate: 1–2 weeks, mostly compute and writing.*

**And the honest end state.** B3 will most likely conclude with **PCS and tiering
withdrawn** and the architecture's value resting on the mesh, the filter, the integrity
check and the measurement apparatus. That is a complete and defensible project. It is not
the project the novelty claims describe, and the gap between those two is the thing to
decide deliberately rather than discover at a viva.

> **2026-09-29.** B3 and CAL-01 ended where this paragraph predicted for PCS and tiering:
> neither is supported on the regenerated tree (L.3 status). Four things survive
> regeneration:
>
> - the mesh (C5);
> - the filter against snapshot estimators (C3);
> - the gateway estimator's tier-1 energy saving (C6);
> - the IMU ceiling, as a bound (C7).
>
> The integrity check **did not** survive in the shipped configuration. The witness
> cross-check adds nothing with the evidence gate on, and the Mahalanobis check has almost
> no eligible population under the default NLOS source. Whether to withdraw the PCS and
> tiering novelty claims is the owner's decision. The claim-by-claim evidence for it is in
> `CA-TBL_Technical_Report_SelfContained.md` §0.

**The system is done when the register is empty or every remaining entry is a stated
decision.** Not when the gates are green — G11 is red on purpose and closing it by tuning
would destroy the only instrument that has ever caught a defect of its class.

---

# M. Found 2026-09-27 – 2026-09-29, recorded

Found while building Bar 1 and Bar 2 and regenerating Bar 3 on
`work/2026-09-27-spec-b1`. Each item says whether it is fixed, and where. Items that need
an owner decision are marked **DECISION**.

### M.1 Relays ranged at the cycle's one instant, and collided — fixed behind a flag; DECIDED 2026-09-30: on by default in relayed runs

Every exchange a cycle made by calling an engine directly carried the cycle's one `now`:
each relay's anchor polls, every relay-to-relay exchange, and the measured tag's own
exchange. The engines' interference registry decides overlap on time and transmitter id
alone, at the interferer's own link power, so those exchanges collided with each other.
Measured at `86e43f2`, with only `--relayedRanging` on (`static_2p4`, 300 s,
`--boxHalf=200`, four tags, seeds 7/11/13): the measured tag's 3+-anchor cycles fell from
107/112/106 of 120 to 2/0/11, and under `--relayChain` no relay-to-relay exchange ever
delivered. That is why `b390ed6` could not meet L.1.2.

**Fixed behind `--relaySlots` (`d24690c`), default off.** With it, the same runs give
103/104/107 and G17b is green. **Every `--relayedRanging` figure recorded before it was
measured under the collision:** `MASTER_PLAN.md`'s C5 row, `b390ed6`'s sweep and
`5f3ad42`'s routing counts. They do not compare with a `--relaySlots` run (§3.1). No
`results/` sidecar uses `--relayedRanging=true`, so no tracked result moves.

**DECISION:** whether `--relaySlots` becomes the default for `--relayedRanging` runs. It
moves every relayed result.

**DECIDED 2026-09-30 (owner: "fix them all"): the relays' round is the default in every
relayed run.** The flag is now `--relaySlots=auto|true|false`, default `auto`. `auto` is on
exactly when `--relayedRanging=true` and `--nTags >= 2`, and off in every other run, where
nothing is constructed. `true` without a relay is still refused. A round that does not fit
inside `--dt` is refused under `auto` as under `true`: at the 2.5 s default that is
`--nTags >= 9` under `--relayChain`, or `>= 51` without it. `--relaySlots=false` keeps the
old behaviour. Checked at the change against `7a36b63` (the commit before it), frozen at
`~/catbl_prev_relayslots`:

- *Plain defaults move nothing.* NC1 and NC2 against `~/catbl_ref_edge` (`db2ac2e`) are
  byte-identical: 7/7 and 2/2 files, stderr identical, stdout identical but for the outdir.
  The same holds with `--relaySlots=false` and with `--relaySlots=auto` appended.
- *The old value reproduces the old commit.* `--relaySlots=false` against `7a36b63` with no
  flag is byte-identical in 13 configurations: every output file, stdout and stderr. The
  configurations are the regime above at seeds 7/11/13 with and without `--relayedRanging`;
  868 MHz relays (`--relayTier=0`); G17's, G18's and G18b's flags; a UWB-relay chain with
  finders; and NC1's and NC2's flags with `--relayedRanging --nTags=3`.
- *The default is the round.* The default and `--relaySlots=true` write identical files in
  every relayed configuration. They differ only in a stderr NOTE that the default engaged.
- *What moves at the default in a relayed run* is everything downstream of the relays'
  exchanges, in every relayed file. These are populations, one seed each, and not effects.
  Engine evaluations differ between the two settings (`directCalls`), so no magnitude
  compares across them (rule 8).
  - The regime above: the measured tag's 3+-anchor cycles go from 2/0/11 to 103/104/107 of
    120 (107/112/106 without relayed ranging). Relay 3 now holds a fix at seeds 7 and 11.
  - G17's flags: relay-to-relay exchanges go from 0 to 7, fuses from 0 to 4, and depth-2
    decisions from 0 to 40. `relay_chain_*.csv` goes from 112 rows to 541.
  - NC2's flags with relays: `fixCycles` go from 665 to 713 of 720.
  - NC1's flags with relays: `ca_tbl_full`'s `fixCycles` go from 291 to 303 of 360.
  - G18's flags: the measured tag's fix cycles go from 0 to 6 of 240.
- *Gates.* G17 and G17b no longer name the flag, so they run at the default, and G17b reads
  G17's run. G18 and G18b pass a new `--require-round`, so that the log must show the round.
  Their self-tests hold the old value red: N14 in each is the gate's own run with
  `--relaySlots=false`. No other gate runs relayed ranging.
- *Results before it.* No `results/` sidecar uses `--relayedRanging=true`, so no tracked
  result moves. Every relayed figure recorded before 2026-09-30 without `--relaySlots=true`
  describes the old default. `--relaySlots=false` restores it. Any other default that moved
  since that figure was recorded is a separate flag, for example `--gatewayEdgeServer=false`
  before `db2ac2e`.

Still open:

- The registry never attenuates an interferer by distance
  (`lora-interference-submodule.cc:126-129` computes `dist` and does not use it). That is
  an engine change with broad impact.
- The round sits in the event slice the MAC layer has already run, so its exchanges meet no
  MAC frame. That gives up at most about 16 % of exchanges' interference on 2.4 GHz in
  G17b's regime.
- Relay 4 held no fix in any 300 s run. Not investigated.

### M.2 The UWB CIR confusion matrix cannot fill under the channel model — DECISION

`86e43f2` synthesises a CIR for every decoded UWB link and scores the spec's leading-edge
ratio. Its TP and FN cells are empty in every regime tried:

- Legacy topology, `static_uwb`, 3600 s: tp 0, fp 0, fn 0, tn 104.
- Default topology: no UWB link is polled at all.
- Outdoors, the engine's NLOS probability at UWB range is about 0.1 % (1 − exp(−d / 8000 m)).
- Where NLOS is not rare (`--envFacility` 5/6/7), every NLOS UWB link polled was lost,
  6 of 6. Both NLOS terms add +9.64 dB at 7.99 GHz, which is beyond UWB's margin.

The classifier is not what is missing: the unit suite exercises both classes on
engine-like states (outdoor tp 157, fp 0, fn 277, tn 495). **DECISION:** filling the matrix
needs a channel-model or regime change, for example a smaller UWB NLOS penalty or an NLOS
scale at UWB distances. That changes the UWB physics every UWB result rests on.

**2026-09-30: decided and registered, then built.** Owner decision "fix them all". The
model is registered in `report/PREREG_UWB_NLOS.md` (`166d431`). It is built exactly as that
registration's §2 specifies, behind `--uwbNlosModel=legacy|ieee802154a`, **default
`legacy`**:
- The state comes from the 3GPP TR 38.901 InF-SL law.
- On an NLOS draw, the excess is the IEEE 802.15.4a NLOS-minus-LOS path loss.
- It makes one sampler draw per evaluation, in the UWB engine only.

Evidence at the change:
- *The default reproduces the build before it.* The default and `=legacy` are byte-identical
  to the frozen `53e5c38` binary (`~/catbl_prev_uwbnlos`) in 7 configurations, in every
  output file, stdout and stderr. They are NC1, NC2, seed 11 with all arms, both
  legacy-topology UWB cells at seed 101, the office class, and a UWB-active finder regime.
- *The formula, on every evaluation.* The unit suite drives the engine directly, 4 000
  evaluations at each of 2, 10 and 20 m. Every NLOS evaluation carries ΔPL(d_3D) to 1e-9 dB,
  every LOS one 0. The NLOS fraction matches P_NLOS(d_2D) to 4σ, and `nlosProbability`
  reports it. Legacy stays below 1 % as the negative control.
- *In the scenario.* Gate G20b asserts the model's NLOS share of polled UWB links (≥ 0.10),
  and the formula on decoded NLOS rows (the registration's P4). A legacy run's share was
  1/110. Its self-test has 9 fixtures.

**A finding against the registration's own expectation.** §2 "Randomness" expected the
868 MHz and 2.4 GHz engines, the MAC, and every non-UWB static arm to be byte-identical
between the two models, and asked the code commit to check it. **They are not.** Under
`ieee802154a` at NC1's flags, `static_868` moves in 28 `arm_summary` columns, and its trace
diverges at t = 10 s.
- The network-wide MAC counters move: `mac_tx` 11 353 → 12 143, and `mac_deliveries`
  5 227 → 6 381.
- The mechanism: the UWB model changes which UWB-channel MAC frames are delivered. The one
  shared MAC random stream is then consumed differently, and every tier's frame timing,
  and so every engine's draw sequence, shifts. This is rule 8 in its MAC form.
- It is not a defect in the model: the physics change is confined to the UWB engine.
- It does not touch the registered design. Its predictions are populations inside each cell,
  and its §8 already forbids cross-model magnitudes.

The results file records it as a finding. The registered cells run next, at the code
commit.

**2026-09-30, measured: the cited model does not fill the matrix.** The 40 registered cells
(`results/uwb_nlos_20260930`, all `57225b8 dirty 0`) were read in `report/UWB_NLOS_RESULTS.md`.
- **P1 is FALSIFIED.** In open ground at 1–4 m, `A_802` decodes 30 NLOS UWB frames of 1 475
  NLOS draws. 28 of them are unscored, because the CIR peak is under the noise floor at
  accumulated SNRs of about +11 to −6 dB. The 2 scored ones score 0, so they read as LOS:
  tp 0, fn 2.
- **P4 HOLDS**: the model is as registered.
- **P2's count clause failed** because the registration assumed polls at 2 m. The legacy
  poll set reaches tens of metres. Legacy's per-poll NLOS rate, 0.084 % and 0.13 %, confirms
  this entry's "about 0.1 %".
- **P3 failed on one frame at 4.38 m.**

By the registration's §6, M.2 stands for every cited NLOS excess. Filling the matrix needs
an obstruction model of our own (a blocked direct path with the scattered energy intact),
and that is a separate owner decision. The read-out adds one design constraint: the SNR
headroom that model would have to leave. The default stays `legacy`.

### M.3 `run_id.txt` records the working directory's commit, not the binary's

The simulator writes `source` from `git rev-parse --short HEAD` run in the **working
directory** (`scratch/ca_tbl_causal_sim.cc:10973` at `ff4f8bb`). If a run is launched from
one clone with another clone's binary, its sidecar gets a clean, wrong SHA.

- It has happened. The C1 original's sidecars read `source deb52da dirty 0`, and `deb52da`
  does not define the flags that run used (`report/B3_REGEN_RESULTS.md` §C1).
- The coverage probe's sidecars read `source unknown`, because the frozen reference runs
  outside any git tree.

B3 is not exposed: every cell ran from the worktree that built the binary, and the driver
hashed the binary before every launch. *Mitigation until fixed:* launch only from the tree
that built the binary. *Fix:* stamp the build's commit into the binary at build time. That
is its own change, with its own null control, because it changes `run_id.txt`.

### M.4 Output files the sidecar does not digest

`CLAUDE.md` §4.5 asks for a SHA-256 per output file. `grant_verdicts_*.csv` (L.1.4's evidence)
was the gap on this branch, and it has been digested since `ff4f8bb`. **Still written and
not digested:** the viz outputs `events_*`, `nodes_*`, `nbrtable_*`, `loctable_*` and
`deployment.json`. L.1.5's only on-disk evidence lives in `events_*` (M.9). The fix is to
add the patterns at the digest call site, as every mechanism on this branch did for its own
file.

### M.5 `--tier1ChargeReplies` help text said "FALSE by default" — CLOSED at `db2ac2e`

It was stale from `4779bf7` (2026-09-08), when the default became true, until `db2ac2e`.
The B3 registration recorded it and could not correct it without touching `scratch/`. It
now says the flag is read only with `--gatewayEdgeServer=false`.

### M.6 The evidence-gate default was missing from the B3 registration's list of moved defaults

`report/PREREG_B3_REGEN.md` §2 lists the defaults that moved between the originals and the
B3 binary: the environment and the tier-1 reply billing. It omits
`--escalationRequiresEvidence`, which went from false to true at `7ec0d91` (2026-09-08). So
C1, C2 and the five `k1b` cells ran with the evidence gate on, where their originals ran
with it off. No verdict changes, because the verbatim invocation binds. What changes is what
C1's verdict is about, and that is why C1_NOGATE was registered (`ddae10f`). The
registration is not amended after the fact; `report/B3_REGEN_RESULTS.md` is its erratum.

### M.7 The CAL-01 machinery's acceptance test is not a fair control

`CaTblCalibrateLoocv` (`ca-tbl-loocv.cc`) returned OK in all six CAL-01 cells, and the OK
is an artefact. It fits the term-1-only control without an intercept, while
t3 ≈ 0.92 gives the three-term model one for free. With an intercept on both sides the gain
vanishes: 77.47 against 77.50 m on `default`, and 80.17 against 80.18 m on `searcher`
(`report/CAL01_LOOCV_RESULTS.md` §4). *Fix:* fit the control with the same intercept, or
accept on rank. That is a `src/` change with its own test. Until then, `CA_TBL_CAL_OK` is
not evidence.

*Fixed 2026-10-03:* the control is now fitted as c0 + c1 · term1. A new unit case reads
NO_IMPROVEMENT where the old code read OK: an error of 4 · term1 + 5, with term 3 near
0.92. On the old code that case fails (8 passed, 1 failed); on the new it passes (9 of 9).
CAL-01's verdicts are untouched: its registered analysis is `scripts/cal01_loocv.py`,
which never called this function.

### M.8 `analyse_pcs_forms.py` reads an undefined statistic as "indistinguishable"

Under `gated`, PCS is constant on 99.98 % of fix cycles, so the Spearman correlation is NaN.
The NaN interval satisfies neither sign test, and the script prints `indistinguishable` and
reads that as "carries nothing the covariance trace does not"
(`report/B3_REGEN_RESULTS.md` §C2). It should print *undefined*. Its docstring also promises
a per-cell hadFix count that it does not print.

### M.9 `TRIGGERED_UPDATE` has no counter, and a scenario comment says its branch never runs

L.1.5's eager push is visible on disk only as rows of `events_*` (M.4). No stdout or CSV
counter reports that message type. The comment at `scratch/ca_tbl_causal_sim.cc:8361-8362`
(at `ff4f8bb`) still says "Nothing here sets mobility state … the receding branch never
runs". It has been false since `50a229a`.

- *2026-09-30:* the comment was corrected (P1, `af492ca`).
- *2026-10-02, MC-2:* under `--triggeredUpdates` the scenario's own triggers (large
  correction, last gasp) are counted on stdout and in `triggered_<arm>_seed<N>.csv`
  (gate G34). The RECEDING push is still uncounted; it moves into `subject_summary.csv`
  with MS-1.
- *2026-10-03, MS-1:* `subject_summary.csv` exists under `--measuredSubjects > 1`, but
  triggered updates still run for tag 0 only (a start-up WARNING says so, N.3), so the
  RECEDING push has nowhere per-subject to be counted yet. Open.

### M.10 Which radio the gateway-estimator downlink is billed on — DECISION; DECIDED 2026-09-30: on the fix's tier

*Filed on 2026-09-29 as "an open question, not an established defect"; the review's
finding is filed below, 2026-09-30.*

The branch review reported a defect in the flipped `--gatewayEdgeServer` default that it did
not fix. That report is not in the tree. The review's working files (`~/catbl_review/`)
examine this question.
The downlink is charged to the **active** tier (`AccountEdgeServerDownlink(active)`,
`scratch/ca_tbl_causal_sim.cc:8076`), including on floor-solved cycles where the fix came
from tier-1 measurements. At NC1 (defaults, seed 7, 900 s), 230 of `ca_tbl_full`'s 305 fix
cycles are active 2.4 GHz, fix tier 868, `usedFloor` 1 (`~/catbl_review/nc_on`).

The §6 invariant holds either way, because the 2.4 GHz radio is awake on those cycles and
is the tier in use. What is not established is whether a 2.4 GHz downlink is the right
charge on a cycle where 2.4 GHz could not reach three anchors. Recorded so that the review's
finding is not lost. Its exact wording should be filed here from the review's report.

**The review's finding, filed 2026-09-30.** The review called it a DEFECT: with the
estimator on the gateways (the default since `db2ac2e`), a fix that came from the 868 floor
is billed as a 2.4 GHz downlink.

- *Per downlink:* 0.2005 mJ on 2.4 GHz against 0.9042 mJ on 868 MHz, so each floor-solved
  cycle is billed 0.7037 mJ less than a fix-tier bill would charge.
  - Both figures are recomputed here from the energy model's constants
    (`ca-tbl-energy-model.cc:36-63`, `:150-156` and `:346-363`): a 12-byte payload at SF7,
    3.3 V, and wake-up and turnaround at the idle current. On 868 MHz that is 125 kHz,
    41.2 ms on air plus a 10 ms guard at 5.3 mA. On 2.4 GHz it is 800 kHz, 6.4 ms plus 2 ms
    at 7.0 mA.
  - The 2.4 GHz figure is also recounted on disk, from NC1 with the edge setting on minus
    NC1 with it off (`~/catbl_review/nc_on`, `nc_off`, `arm_summary.csv` column `j_2p4`).
    `ca_tbl_full` gains 0.049726 J over its 248 fix cycles with 2.4 GHz active (230
    floor-solved and 18 with a 2.4 GHz fix), and `static_2p4` gains 0.002206 J over its 11
    fixes. Both work out to 0.2005 mJ per fix.
- *What it biases:* B3 C6's hybrid figures. C6 ran with the downlink on the active tier
  (`report/B3_REGEN_RESULTS.md` §C6: "one downlink per fix on the active tier"; Δ `j_2p4`
  −0.20 J on each policy arm). Billed on the fix's tier, the −32.9 % on `fixed_threshold`
  and `ca_tbl_full` would be a smaller saving. By how much is not computed here: that needs
  a run with the billing changed, not arithmetic on this one. `static_868`'s −42.5 % is
  unaffected, because its active tier is always 868 MHz.
- *Why it is a decision and not a fix:* either bill charges a radio that is awake on the
  cycle. 2.4 GHz is the active tier, and 868 MHz ranged for the floor. Moving the bill
  changes energy on every hybrid arm under the default.

**DECISION:** bill the downlink on the tier the fix came from, or on the active tier as
now. Until it is taken, every hybrid-arm energy figure under the edge setting carries this
caveat. The accuracy figures do not.

**DECIDED 2026-09-30 (owner: "fix them all"): the downlink is billed on the tier the fix
came from.** The new flag `--edgeDownlinkTier=fix|active` defaults to `fix`. It is read only
with `--gatewayEdgeServer=true`. `active` keeps the old bill, and any other value is FATAL.
The two differ only on a hybrid arm's floor-solved fix cycle. On one of those, the downlink
moves off the active tier and onto 868 MHz. Checked at the change against `fab313c` (the
commit before it), frozen at `~/catbl_prev_m10`:

- *The old value reproduces the old commit.* `--edgeDownlinkTier=active` against `fab313c`
  with no flag is byte-identical in 11 configurations, in every output file, stdout and
  stderr:
  - NC1's and NC2's flags, and seed 11 × 900 s with all arms;
  - NC2's flags with `--uwbArbitration`;
  - NC1's flags with `--tier1Tdoa`, with `--relayedRanging --nTags=3`, and with
    `--batteryDepleteJ=4.5`;
  - two finder regimes, and a third with the evidence gate off, where the hybrid arms are
    UWB-active;
  - C6's edge cell at seed 7 × 3600 s. This run also reproduces the seed-7 rows of
    `results/regen27_edge/edge/arm_summary.csv` byte-for-byte.

  `~/catbl_ref_edge/nc.sh` passes with `--edgeDownlinkTier=active` (NC1 7/7, NC2 2/2).
- *The node-side setting does not move.* `--gatewayEdgeServer=false` passes
  `~/catbl_ref/nc.sh` against `084a7ff`, with and without `--edgeDownlinkTier=active`.
- *What moves at the default.* Only energy, only on hybrid arms, and only on floor-solved
  fix cycles:
  - `arm_summary.csv`: `totalJ`, `accuracyPerJoule`, and the `j_`, `rx_` and `idle_`
    columns on 868 MHz and on the active tier (2.4 GHz, or UWB where it was active);
  - the traces: `cumJ`;
  - stdout: the arm table's energy and acc/J columns, and the tier-1 rx count under
    `--tier1Tdoa`.

  `static_868`, `static_2p4` and `static_uwb` are byte-identical in every configuration,
  which control flow requires. `fixTier` differs from `active` only when `usedFloor` is
  set, and `runFloor` is false on a static arm. The bill is accounting only, so both
  settings make the same engine evaluations and their magnitudes compare (rule 8 does not
  bite).
- *Recounted from the traces (rule 3).* In every traced configuration, `cumJ` at `fix` minus
  `cumJ` at `active` steps on exactly the rows with `usedFloor=1` and `hadFix=1`, and on no
  other row.
  - The step is 0.703675 mJ where 2.4 GHz was active and 0.581780 mJ where UWB was.
  - Per moved downlink, `arm_summary.csv` gives +0.904183 mJ on 868 MHz, −0.200508 mJ on
    2.4 GHz and −0.322403 mJ on UWB.
  - At NC1, 230 downlinks move on `ca_tbl_full` (the review's figure) and 234 on
    `fixed_threshold`.
- *The §6 invariant on those cycles.* Every row whose bill moved has `floorRanged=1`, which
  means 868 MHz had transmitted its broadcast that cycle. That is 3 568 of 3 568 rows over
  the nine traced configurations. The downlink now lands on the radio whose ranges made the
  fix, and never on a radio that stayed silent.
- *Where energy is an input, more moves.* With `--batteryDepleteJ=4.5` on NC1's flags, the
  hybrid arms' batteries run out earlier:
  - `fixed_threshold` at 780.0 s instead of 812.5 s;
  - `ca_tbl_full` at 657.5 s instead of 672.5 s.

  Their accuracy, tier and MAC columns then move too. With `--uwbArbitration` and one
  requester, nothing but energy moved.
- *C6: one seed, an illustration only.* At seed 7 × 3600 s, C6's edge cell moves 943
  downlinks on `fixed_threshold` and 946 on `ca_tbl_full`. Against node-side estimation,
  their totalJ saving goes from −32.30 % to −30.13 % and from −31.64 % to −29.53 %.
  `static_868` (−42.15 %) and `static_2p4` (+0.13 %) do not move. The registered 20-seed
  figures (−32.9 %) describe the old bill. Re-measuring them under the new default needs a
  registered C6 re-run, which has not been done.
- *Gates.* G21 gains a third input: G3's seed and duration at `--edgeDownlinkTier=active`.
  Against G3's run, only energy may move, only as a transfer onto 868 MHz from the active
  tier, and only on rows where 868 MHz transmitted. At least one row must move, so a
  reverted default goes red. The self-test gains P1 and N8–N14. N8 is the active-tier run
  offered as the defaults. All 17 fixtures behave as expected. No other gate bills the
  downlink on a floor-solved fix:
  - G6 pins `--gatewayEdgeServer=false`;
  - G19's edge run uses `static_868` only;
  - G2–G5 compare runs of one binary, never a tracked edge-server result.
- *Results before it.* No `results/` directory moves. The only tracked edge-server cells,
  `results/regen27_edge/edge`, describe `--edgeDownlinkTier=active`, which reproduces them.
  Every hybrid-arm energy figure with the edge setting recorded before 2026-09-30
  describes the old bill.

### M.11 Stale numbers in the rules and setup documents

- `CLAUDE.md` §4 and §8 place the `cmd.AddValue` block at
  `scratch/ca_tbl_causal_sim.cc:3178-3450`. At `ff4f8bb` it is `:9316-10762` (142 flags),
  with `RunArm` at `:4905` and its call at `:11365`.
- `CLAUDE.md` §3 says G1-G11, and the header of `scripts/catbl_gates.sh` says G1-G13. The
  suite runs 23 gates (G1–G21 with G17b and G18b). At `ff4f8bb` it gives 19 PASS and
  4 FAIL, the four documented reds G4, G5, G6 and G11.
- `SETUP_NEW_MACHINE.md` expects 11/15 gates and eight suites of 152 checks. There are now
  ten suites and 235 checks: packet_validator 38, session 29, routing 27, lifecycle 16,
  netlayer 23, relay_admission 22, sparse_anchor 11, loocv 8, tdoa 33 and nlos 28.

Documentation only. It is recorded here rather than edited in the rules file, because
`CLAUDE.md` changes are the owner's.

*2026-10-03 (P7):* the numbers have moved again, and `CLAUDE.md` still waits for the
owner.
- **The gate suite:** 58 gates, from G1 to G60 with G17b, G18b and G20b; G22-G23 and
  G25-G27 were never built. At `cef0fc6` it gives 53 PASS and 5 FAIL: the documented G4,
  G5, G6 and G11, and G24 FAIL BY FINDING (N.15).
- **The unit suites:** twelve, 356 checks:
  - packet_validator 62, nlos 41, integrity 38;
  - sparse_anchor 33, tdoa 33, session 32, netlayer 30;
  - routing 27, relay_admission 22, lifecycle 16, imu 13, loocv 9.

*2026-10-03, CLOSED (the owner approved the edit):* `CLAUDE.md`, `SETUP_NEW_MACHINE.md` and
the header of `scripts/catbl_gates.sh` now carry these numbers, each dated with the commit it
was counted at. Where a line number is cited, the grep that finds it is given as well, so
the next drift can be re-read rather than trusted. Corrected with them:
- G3 passes now (it separates RNG contamination from the treatment effect on causal order);
- "G11b" is G11;
- the reds are G4, G5, G6, G11 and G24.
Line numbers will drift again with the next code change. That is expected and is not
reopened as debt.

# N. Found 2026-10-02 to 2026-10-03, while building the spec-conformance programme

Index (P7 of `docs/SPEC_CONFORMANCE_PLAN.md`). Every entry is open unless it says otherwise:

| entry | in one line | status |
|---|---|---|
| N.1 | a silenced radio still forwards other nodes' floods | open |
| N.2 | Case A with relays counts one pair twice | fixed under `--sparseInStep` only |
| N.3 | subjects 2..N run a core of tag 0's cycle, not the whole cycle | open (MS-1's deviation) |
| N.4 | a finder tasked to a subject it never hears stands still | open |
| N.5 | what `--grantGatedUwb` decides that the spec does not | recorded (OURS) |
| N.6 | a radio can put two frames on the air at once | open |
| N.7 | PCS does not fit the byte an entry gives it | open (with CAL-02 and N.16) |
| N.8 | what `--arbitrationV2` costs and decides | recorded (OURS) |
| N.9 | queue gossip is built and rarely needed | recorded |
| N.10 | what `--requesterEscalation` decides that the spec does not | recorded (OURS) |
| N.11 | per-hop variance swamps a relayed leg's sigma | DECIDED 2026-10-03: out of the preset (owner); the term itself open |
| N.12 | `--fixRouting` with subjects breaches sect 6; `--fixRoutingSubjects` replaces it | CLOSED 2026-10-03: refused (owner decision) |
| N.13 | Case C shapes are coarse on 868 MHz | recorded |
| N.14 | a UWB range never improves a fix the floor can make | remedy built behind `--fuseFloorActive` (2026-10-04); adoption by FUSE-01 |
| N.15 | the preset's differences from the plan, and G24's zero populations | open (G24 FAIL BY FINDING) |
| N.16 | at 5 km the covariance trace ranks error backwards | open, cause not established |
| N.17 | what `--radioBasis=lr1121` decides that the datasheet does not | recorded (OURS) |
| N.18 | what `--subjectRelayChain` decides that the plan does not | recorded (OURS) |

### N.1 A silenced radio still forwards other nodes' floods

`CaTblMac::SetSilent(true)` gates `Send()` and nothing else. Flood forwarding goes through
`MaybeRebroadcast()` → `AttemptCad()`, which never consults it, and so does a frame already
queued when the radio was silenced (its CAD, its repeats). So a node whose battery is
spent (`--batteryDepleteJ`), or one silenced by `--injectSilentId`, goes on relaying
DISCOVERY and every other flooded type.

- *Measured,* G34's default run (seed 7, 900 s, `--batteryDepleteJ=3`): after the battery
  died at 505 s, node 1's radios rebroadcast 322 distinct DISCOVERY frames that other
  nodes had originated (1 277 receptions of them), and originated none.
- *Effect.* A dead node still extends the flood, so every `--batteryDepleteJ` and
  `--injectSilentId` result overstates how much of the mesh the fault removes. Its own
  traffic does stop, which is what the fault counters measure.
- *Why not fixed here.* Fixing it changes every run under those two flags. It needs its
  own flag value and null control, like any behaviour replaced under an existing flag
  (plan §0 rule 12).
- MC-2's last gasp RELIES on the second half: the gasp is queued, then the radio is
  silenced, and the queued frame still leaves. A fix must keep that, by gating forwarding
  only, or by checking silence at the moment of queueing.


### N.2 Case A with relays counts one pair of ranges twice — fixed only under `--sparseInStep`

With `--sparseCaseA --relayedRanging`, the scenario solves Case A from the active tier's two
ranges and fuses that position. C5 then appends the relays, and `Step()` range-updates on the
same two ranges plus the relays, so one measurement goes in twice (plan §1.H item 9). The
same path records every Case A cycle as a no-fix and counts it toward demotion: 64 of 64 at
NC3's seed 7 (G39, OLD).

- *2026-10-02, DC-11:* `--sparseInStep` solves inside `Step()`, on the final set after C5.
  Two ranges are fused once, as a position. Two plus relays take the full path, and no
  sparse point is fused. G39 recounts 0 cycles that do both.
- The default is unchanged, so every `--sparseCaseA` result so far carries both effects.

### N.3 Subjects 2..N run a core of tag 0's cycle, not the cycle itself

The plan (MS-1 step 1) was to move the per-tag body of the loop into one `RunSubjectCycle`
and call it for every subject, in a commit of its own with no behaviour change. That was not
done. `--measuredSubjects` (2026-10-03, G46) instead grows C5's relay mini-pipeline into a
subject pipeline for tags 2..N and leaves tag 0's code untouched, so the null control holds
by construction rather than by a byte-identical move. The cost is two copies of the core:
ranging on the active tier, the tier-1 floor, fix selection, the IMU control, `Step()`, the
handshake and its duty-budget pre-emption, the downlink, the C4 terms, listen/sleep, and mesh
relay billing. A change to one copy that is not made to the other drifts silently. G46's
subject-1 = arm_summary check pins tag 0's row only, not the equivalence of the two copies.

What tag 0 has and subjects 2..N do not, each named in a start-up WARNING when on:
SHS and the SHS witness (`--imuCrossCheck`/`--imuFusion`), `--imuWitnessSource=model`,
`--tier1Tdoa`, `--nlosClassifier`, `--sessionLayer`, `--batteryDepleteJ`,
`--triggeredUpdates`, `--edgeUplinkRanges` and `--entryVelocity`/`--entryPrediction`.
Finder tasking (MS-1's `--finderTasking`) is not built: finders still steer to tag 0.
- *2026-10-03:* built, `--finderTasking=roundrobin` (G47). See N.4.

Two measured facts from building it (five subjects, NC9's regime with frames and
arbitration, seed 7, 600 s):
- Ranged at tag 0's instant, the subjects collided with it in the engines' interference
  model, and tag 0's static_868 fixes fell from 212 to 2. Each subject now ranges in its own
  50 ms slot before the instant (the relay round under `--relaySlots`, else a round of its
  own). The slots sit in the slice the MAC has already run, M.1's known limit.
- With a refused duty budget aborting the escalation, as relays never needed, subjects did
  not escalate at all (subject 2 deferred 137 times). They now commit it and pre-empt the
  next cycle's ranging with the handshake, as tag 0 does under `arbitrateDutyBudget`.

Turning subjects on changes the number of engine evaluations, so every tag-0 magnitude moves
with it (rule 8): at five subjects tag 0's static_868 fixes are 225, not 212. Only
control-flow claims compare across N.

*2026-10-04:* the relay chain is open to subjects behind `--subjectRelayChain` (G62, N.18).
That was G24's hop-depth-2 zero. The rest of this entry stands: the two copies of the core
remain, and so does the list of what tag 0 has and subjects do not.

### N.4 A finder tasked to a subject it never hears stands still

Under `--finderTasking=roundrobin` (2026-10-03, G47) finder k steers on subject
(k mod N) + 1's table entry. A finder learns entries only by anti-entropy with nodes in range,
and a finder with no entry for its subject holds position in mode 1 (the honest behaviour the
converge branch has always had) or keeps sweeping in mode 2. In G47's regime (G43's
converging finders, five tags, seed 7, 600 s), with one finder per subject:
- finder 501 (tag 0) converged for 216 cycles;
- finders 502-504 (subjects 2-4) were blind for all 240 cycles and never left base camp.
With two subjects, the finders on subject 2 learned its entry from the pair converging on tag
0 and converged for 206 and 188 cycles.

So round-robin tasking can leave a subject with a searcher that never moves, where the
default would have sent every finder toward tag 0. That is a property of the assignment rule
(OURS) and the dissemination it rides on, not a defect in the code. Whether a blind finder
should fall back to its neighbours' targets, or the base station should task finders
(TA-1), is an open design choice. No result depends on it yet.

### N.5 What `--grantGatedUwb` decides that the spec does not

NW-7 (2026-10-03, G49) gates the escalation into UWB on a delivered grant. Three choices in
it are OURS, and each changes something a reader could otherwise assume:
- **A denial ends the request.** MC-1 leaves a denied request pending, so its ACK timeout
  retransmits and the arbiter scores the retry afresh. The plan says a denial clears the
  pending state, and a retry the policy no longer waits for would earn a grant nobody uses.
  So under the flag a denial also cancels the MAC's retries (`CancelPendingRequests`), and
  the policy asks again, with fresh terms, the next cycle it wants UWB. In G49's regime tag
  0 drew 24 denials against 29 grants.
- **The first verdict settles a broadcast request.** Several anchors can hear one
  ESC_REQUEST, and each answers. Whichever verdict arrives first decides; a grant arriving
  after an earlier denial finds no outstanding request and is ignored, though its anchor has
  reserved a slot. Multi-anchor reconciliation is NW-5 and NW-6.
- **The trace's `escalated` flag changes meaning on 2.4 GHz.** It marks the request (with
  `nextTier` still 2.4 GHz), while `escalations` in arm_summary counts the grant. Any
  analysis that equates the two needs the flag's state.

A process note from the same day: `~/catbl_tmp_gates/run_suites.sh` ran the unit-suite
binaries without rebuilding them. After NW-7's policy-header change,
`ca_tbl_sparse_anchor_test` segfaulted on its stale binary, and the 311 checks reported at
NW-8 (`184d688`) included binaries built before that commit's MAC-header change. Rebuilt
and re-run on the NW-7 tree, which contains NW-8, all 321 pass. The helper now rebuilds
first.

### N.6 A radio can put two frames on the air at once

`CaTblChannel::IsBusy()` skips the observer's own transmissions
(`f.txId == observerId`), so a node's CAD never sees its own frame in flight. A MAC with two
queued frames can start the second while the first is still on air. A third receiver then
counts the two as a collision, since the interference loop skips only the receiver itself.
A half-duplex radio cannot do this.

It surfaced building NW-9 (2026-10-03). In a four-node line, node 2 answered part of a batch
query and relayed the rest within the default jitter; the two frames destroyed each other at
node 3, and the query never reached node 4. The unit case widens the relay jitter to step
around it.

It is not new to NW-9. Any node that answers and rebroadcasts at once has always done it (a
target answering a legacy QUERY still rebroadcast it), as has any node whose ACK, beacon or
relay overlaps its own frame. How often it happens in the scenario is not measured. A fix
changes every run's channel and so every result (rule 8): it would be a default-off flag
with its own null control, not a bug fix. Open.

### N.7 PCS does not fit the byte a table entry and a beacon give it

NW-9 carries the producer's PCS in its entry, quantised 0..255 over [0, 1], the same mapping
as the C4 request terms and the 1-byte pcs_q DISCOVERY already bills. Under the default PCS
form PCS is not bounded by 1: values of 1.96 appear in G50's regime, and 22 of 53 answers
carried 255. A requester can therefore tell "high" from "low" but not grade within the
saturated range. This is K.8 (PCS uncalibrated) seen from the wire. `--pcsForm=calibrated`
(DC-6) is where a bounded score comes from. Open, with CAL-02.

### N.8 What `--arbitrationV2` costs and decides

NW-5 (2026-10-03, G51) implements Network sect 5's arbiter. Measured in G51's regime (five
subjects, four finders converging, URGENT queries every 30 s, `--grantGatedUwb`, seed 7,
1 200 s):
- **Every anchor that hears a UWB request it has no reach evidence for answers DENY.**
  That is 2 872 out-of-range refusals in 1 200 s, all from gateways too far away for UWB,
  as the plan specifies ("answers DENY with reason out-of-range"). Each is a frame on 2.4
  GHz. Silence would cost nothing on the air; the plan chose an explicit answer. Open.
- **An out-of-range refusal does not settle the request at the node.** It was first built
  as final, and under NW-7's first-verdict rule a far gateway's DENY then cancelled
  requests a nearby finder would have granted. Tag 0 confirmed 5 against 29 under C4
  before this was changed. It is now non-final: MC-1's timeout retries the request, and
  only exhausted retries end it.
- **A request queued by two anchors.** A broadcast request can sit in several queues.
  The first build read the second anchor's queued verdict as a drop: the subject cleared
  its pending state, re-requested every cycle, and missed both grants from the queue.
  Queued requests are now tracked per anchor, and dropped only when every anchor holding
  one drops it. Two anchors can still grant the same request from their queues; the second
  grant is ignored, its slot is held until it expires, and NW-6's gossip of the queue is
  what would reconcile it.
- **Most requests time out.** Tag 0: 138 requests, 8 confirmed, 129 timed out. The NW-4
  hook escalates whenever an URGENT query arrives, and with no anchor in UWB reach no
  arbiter with evidence answers. Under C4 every gateway granted, so the node believed it
  held a slot nothing could serve. The evidence gate (off in G51's regime, on in the
  preset) is what keeps the hook out of places without UWB.
- OURS, stated in code and help: T_batt's normalisation (e = 0.05), the 10 s evidence
  window, the 30 s queue lifetime, the 29.5 m 2.4 GHz proxy for UWB reach (this channel
  model's UWB r50), URGENT = priority byte 1, and finders as arbiters.

### N.9 Queue gossip is built and rarely needed in the five-subject regimes

NW-6 (2026-10-03, G52) gossips the --arbitrationV2 queue and extends the lower-id
tie-break to it. Two arbiters holding the same request is what it settles, and in this
scenario that is rare. Fourteen probe runs varied seeds, slot holds (10, 30, 60 s), finder
counts (4, 6, 8) and converging finders; queue items reached another arbiter in three of
them, and one (seed 1, 1 800 s) yielded. Two reasons, both measured:
- Without digests, finders never learn tag 0's entry and stand at their spawn points, so
  no arbiter is in range of another. G52's own gossip log shows queue items reaching only
  field nodes in such runs.
- Five subjects walk apart. Two arbiters both have UWB-reach evidence for the same subject
  and both hold slots only when subjects and finders cluster.

So the mechanism's reachability is shown in the packet validator ([12]: a double grant
and a double queue between two arbiters, settled by gossip frames). G52 shows the
scenario wiring, and one justified yield. A clustered team -- the spec's "five-person
search" -- would exercise it; no flag places the subjects together today.

Also OURS, or a consequence of the rules:
- The queue item layout (4 bytes: target, U byte as U/4 over 0..255, queued-for byte).
- Finders gossip only over --meshLinks=all, because the gateway pair class places the
  transmitter at a 15 m surveyed mast. They are unbilled like everything a finder does.
- Finder ids (501+) are above every gateway's (101+), so under "the lower id defers" a
  finder always keeps a contested request against a gateway. The rule is the spec's; the
  id assignment makes the outcome systematic.

### N.10 What `--requesterEscalation` decides that the spec does not

TA-2 (2026-10-03, G57). Transport sect 4: a requester that sees a low-precision result
drives escalation for its target, routing the request to an anchor in the target's fresh
neighbour set. Measured in G57's regime (G47's converging finders, five subjects, seed 7,
1 200 s): 54 requests, 5 sent, 4 granted, 4 accepted.
- **The request reaches the arbiter instantly and unbilled.** The base station hands it to
  the arbiter's MAC directly. No frame carries it across the mesh: routing a frame to a
  named anchor is NW-2's (routing per subject). Only the arbiter's on-behalf GRANT is a
  frame, billed like any GRANT.
- **The neighbour set is one arbiter, named by the target.** The target's entry carries the
  id of the arbiter it last had UWB-reach evidence with: a UWB exchange, or a 2.4 GHz range
  under 29.5 m, within 10 s. That is the NW-5 evidence rule (N.8), and the 10 s window is
  OURS. The id travels in memory with the entry and is not serialised or billed (2 bytes
  if it were). A list would serve the spec's wording, but a requester routes to one anchor,
  and it cannot check the list either way.
- **Most requests are unroutable.** 49 of 54 found no arbiter named, because the target had
  no fresh reach when it published. Those requests are not sent. The plan's review ruled
  out the nearest gateway, and G57's fixture N3 fails a request routed there.
- **Every clean answer from a node not on UWB asks.** The rule compares the PCS in the
  delivered entry with the 2.4 GHz up-threshold. That PCS is a byte over [0, 1] (N.7), and
  under the default PCS form the thresholds (1.55 to 2.30) lie above anything the byte can
  hold. So the test always passes, and only the per-target rate limit (one request per
  `--arbSlotHoldS`, OURS) spaces the requests. `--pcsForm=calibrated` (DC-6, CAL-02) is
  what would make the comparison mean something.
- **Only the base station asks.** TA-1b's finder sessions do not submit requests; that is
  not built. Open.
- At the target, an on-behalf grant confirms a pending request of the node's own.
  Otherwise it is the node's escalation 2.4 GHz -> UWB on a hybrid arm, effective the next
  cycle. It is refused on 868 MHz (never a jump to UWB) and on a static arm. Both outcomes
  are counted. In G57 none were refused.

### N.11 Per-hop variance swamps a relayed leg's sigma

NW-1 step 3 (2026-10-03, G58). `--hopVarianceTerms` adds a leg's RSSI/SNR variability to its
sigma. The variability is the von Neumann sigma of its readings, converted to range by the
tier's nominal path-loss slope. Measured in G53's regime (four tags in a 400 m box, relays
chained to depth 2, seed 7, 600 s; 2.4 GHz, path-loss exponent 2.3, so slope x range =
9.99 dB):
- **The term is large.** The median leg variability is 5.5 dB (static 2.4 GHz arm) and
  6.2 dB (full arm), at a median range of 137 m. Converted, that is a median sigma_var of
  64 and 83 m (90th percentile 127 and 159 m), against the engine's raw sigma of 3.5 m. The
  relayed share of the measured tag's variance rises from 0.62 to 0.98.
- **So a relayed leg carries almost no weight.** The measured tag's RMSE barely moves
  (3.528 -> 3.494 m and 9.898 -> 9.884 m, same binary and seed). A short relay that fuses
  through the chain coasts: relay 3's error on the static arm went from 65 to 122 m.
- **Why.** The engine draws fading and NLOS independently for each transmission, so
  successive readings on a link differ by several dB even when nothing moves, and the
  statistic measures exactly that. Tiers 2 and 3 range by time of flight, so RSSI
  variability is not a term of their range error at all. The conversion is OURS: the spec
  names the term, not how to put it in metres.
- **The NLOS half rarely fires.** 16 of 1 040 scored legs were at or above 0.5, and two of
  those were admitted. The 2.4 GHz leg score is the ToF/RSSI excess against the legs' own
  pool, and per-transmission fading moves that pool as much as NLOS does.

The preset turns the flag on (plan sect 3.1). Whether it should is for CAL-02 and the
comparator arms to say; the "LoRaWAPS-style" baseline runs with it off. It is not a defect
to tune away here. Open.

*2026-10-03, DECIDED by the owner:* `--hopVarianceTerms` leaves `--specProfile`. The flag
stays, default off, unchanged. What the preset loses is a term that, where it fired, gave a
relayed leg almost no weight. At 5 km it had no population at all (N.15: no relay leg had
the three readings a variance term needs). The conversion of RSSI variability into a range
sigma for time-of-flight tiers is still OURS and still open. A tier-aware form, or none on
ToF tiers, would be its own flag.

### N.12 Routing subjects: what `--fixRoutingSubjects` decides, and the breach it replaces

NW-2 (2026-10-03, G59). Measured in G59's regime: full arm, five measured subjects and one
plain relay, the confidence selector, seed 7, 600 s.
- **The breach.** `--fixRouting` with `--measuredSubjects` > 1 routes subjects 2..N on
  `--relayTier` whatever tier they ranged on. In G59's control, 671 of 1 204 frames on the
  air came from a subject that ranged on another tier that cycle: a second tier
  transmitting in one cycle, which sect 6 forbids. No gate ran that combination before.
  The breach still runs, warned on stderr. It is not refused, because refusing would
  change what an existing flag combination does.
- **The fix.** Under `--fixRoutingSubjects` there is one router per tier. Each cycle a node
  takes part only in the router of the tier it ranged on. Routes learned on a tier persist
  for their three-cycle lifetime while the node is elsewhere. A hop whose next hop has
  since moved tier is refused: 46 of them in G59.
- **868 MHz routes almost nothing.** A subject on 868 MHz has spent its 2.5 % duty budget
  on ranging, so its routed frames are refused by the governor (1 053 refusals, 24
  forwards, nothing delivered). No subject reached UWB in this regime, so that router had
  no participants. Neither outcome is wrong, but the per-tier routing that works is
  2.4 GHz.
- **Links are learned passively on every tier.** A node notes gateway beacons on all three
  radios whether or not it is on that tier, as the MAC's neighbour tables already do. Only
  transmission and the receive charge are tied to the cycle's tier. Whether a node on
  868 MHz can hear 2.4 GHz beacons at all is the model's existing assumption, not NW-2's.
- The row's pcs field stays 0, as neither metric reads it. The weights are exposed and
  untuned (OURS); the sweep is CAMP-SPEC's.

*2026-10-03, CLOSED by owner decision:* the breach is now refused. `--fixRouting` with
`--measuredSubjects` > 1 and without `--fixRoutingSubjects` exits 1 with the reason, where it
warned. Nothing at defaults moves (the combination needs two non-default flags), the null
control passes, and no `run_id.txt` under `results/` used it. Two gates read it:
- G59's control is now `--measuredSubjects=1` (the relays on `--relayTier`), and the old
  combination is its third refusal, with a fixture that accepts it.
- G24 can no longer turn `--fixRoutingSubjects` off under the preset, which has subjects
  and `--fixRouting`. Its efficacy run became a refusal check (R12), with a fixture.

### N.13 Case C shapes: what `--caseCRelative` decides, and how coarse they are

DC-3 (2026-10-03, G60). Measured in G54's regime: five subjects in a 4 km box, five gateways
on a 140-degree arc, finders, mesh links, the base station's batch queries every 30 s,
seed 7, 900 s.
- **0-anchor cycles are common and mostly on 868 MHz.** There were 736 node-cycles with no
  anchor range, and a peer was heard on all but 11 of them. Peer exchanges numbered 2 159,
  of which 1 759 returned a range. A node with no anchor has usually been demoted to
  868 MHz, where ranging is RSSI inversion. Peer ranges there err by about 190 m (median;
  90th percentile about 730 m) at separations of a few hundred metres.
- **So the shapes are coarse.** The 13 shapes of three or more miss the true
  configuration by about 360 m (median, after the best rotation and reflection), at a
  median cohesion of about 1.4 km. MDS reproduces its input; the input is the problem.
  G60 checks the reconstruction, not its accuracy, as the plan's verification asks.
- **Most sets never reach the base station.** Gateway tables in this model never merge a
  field node's entry; a gateway holds only its own. So a node's entry, and the peer
  ranges riding it, reach the base station only in an answer to its query (TA-1a). With
  answers every 30 s and a 10 s window (OURS), 14 sets arrived and 677 did not. A first
  build looked in gateway tables and delivered nothing. A shorter query period, or a
  window as long as the query period, would deliver more; both are choices, so they were
  not made here.
- **Missing pairs are completed by shortest path (OURS):** 34 completions over 13 shapes.
  A completion overstates its distance whenever the path bends. Three members with one
  completed pair come out collinear (lambda2 = 0).
- **The carry is billed, not modelled.** "3 bytes a range" is billed as extra airtime on
  the node's active tier when the ranges are taken, as if they rode an entry sent that
  cycle. No frame grows.

### N.14 A UWB range never improves a fix the floor can make

The MS-3 checkpoint (report/MS3_UWB_CHECKPOINT.md, 2026-10-03) failed: no UWB-delivered
fix on any seed of the preset. Two structural reasons.
- **One finder per subject at most.** `--finderTasking=roundrobin` gives each subject one
  finder; with 4 finders and 5 subjects, one subject has none. A UWB multilateration
  needs three anchors in UWB reach, and gateways are never that close. In a 750 m box
  tag 0 had two UWB neighbours on 16 of 2 160 cycles and three on none.
- **Floor-first fix selection.** When the active tier returns fewer than three ranges and
  the 868 MHz floor solves, the cycle uses the floor's set, and the active tier's ranges
  are not fused at all (`nRangesEkf` = `nAnchors` on every such UWB cycle). Case A and
  Case D on the active tier run only when the floor fails. The rule was measured as right
  for 2.4 GHz Case A against the floor (the census comment in the scenario). For UWB it
  throws away a range three orders of magnitude better than the RSSI ranges it keeps.

The remedy, not built and waiting on the owner: fuse the active tier's 1-2 ranges with the
floor's set (a flag, its own null control and gate). Open.

*2026-10-03, GEO-01 stage 2 (report/GEO01_RESULTS.md §2, P4):* the same rule costs the
subjects availability at sparse geometry. The sparse cases (Case A, two ranges) run on the
active tier only. A subject that escalates to 2.4 GHz and finds 0-1 anchors there gets no
fix, because the 868 MHz floor needs three ranges. On `static_868` the same subject stays
on 868 MHz and fixes from two ranges (75 cycles against 0 for subject 3, N8_ring seed 1).
So `static_868` beat `ca_tbl_full` on availability in every GEO-01 cell, by 0.03-0.06. Applying
the sparse cases to the floor's set as well would be part of the same remedy.


*2026-10-04, the remedy built (owner decision: a flag, then a registered campaign):*
`--fuseFloorActive` (G63). When the active tier has fewer than three ranges and the floor
ranged, the fix set is the union: the active tier's ranges, relays included, then the
floor's. Step()'s sparse cases act on it below three. The floor alone at three or more keeps
the old path. Tag 0 and every measured subject. What it decides, each OURS:
- **A union holding active ranges is labelled with the active tier** in the traces. The
  cycle used them, and they are the precise ones. This overrides the rule written above the
  fix selection, that each cycle's fix comes from one tier, which is the point of the
  remedy. So a "UWB fix" under the flag can hold 868 MHz ranges. `fusefloor_*.csv` says
  which, and how many of each.
- **Under `--subjectRelayChain`, a union of three is a fix on direct ranges** (the subject
  holds), and relays may complete a shorter union.
- **TDoA still runs only on a pure floor set** of three or more.

In G63's preset runs (seed 7, 900 s):
- the endgame formed 666 unions, 91 of them on UWB, with 19 UWB-delivered fixes against 0
  without the flag;
- at 5 km it formed 872, of which 644 had no active range and a short floor, the case
  GEO-01 found.

Whether the preset takes it is FUSE-01's registered rule, not this entry.

### N.15 The preset: what `--specProfile` sets that the plan's table did not say, and its zero populations

P6 (2026-10-03, G24). `--specProfile` sets 68 flags. It differs from the plan's §3.1 table
in these ways, each forced by the build:
- `--sparseCaseA` is left out. It is FATAL together with `--sparseInStep`, which replaces
  it.
- `--relayVarBudget=24336` ((156 m)^2, G17's value, OURS) is added. With A2's budget at 0
  the chain is inert ("budget 0.0 m^2" in the first probe).
- `--identityQueryPeriodS=30` (OURS) is added. `--queryV2` is FATAL without a query
  period.
- `--fixRoutingSubjects`, `--hopProvenance`, `--reversePathHop`, `--requesterEscalation`
  and `--sessionPreciseOnGrant` were built after the table was written, and are in it.
- **Not yet in it:**
  - `--radioBasis=lr1121` (SY-2: the LR1121 datasheet, which needs poppler-utils or the
    owner's tables);
  - ~~the calibrated PCS~~: in it since CAL-02 (`--pcsForm=calibrated`, weights
    (0, 1.92, 1.08) and five thresholds; report/CAL02_RESULTS.md, N.16).
- *2026-10-03:* the geometry is now in it: `--nGateways=8`, a ring, GEO-01 stage 2's
  registered fallback. No candidate met the criterion (report/GEO01_RESULTS.md). The
  preset with it reproduces the GEO-01 N8_ring seed-1 cell byte for byte.

G24 runs the preset (5 km), its endgame variant (750 m box, finders converging; the
owner's decision after MS-3), the last-gasp run, and the preset with one flag off per
mechanism. Every efficacy run reads 0, the preset reproduces its own flags byte for byte,
and the refusal fires. But **six mechanisms have a zero population**, and G24 lists each
as FAIL:
- **Case A and Case B fixes.** Floor-first selection (N.14): the 868 MHz floor solves
  whenever the active tier has fewer than three ranges, so the sparse cases are tried only
  when it does not. That was once or twice a run, refused as degenerate each time.
- **UWB-delivered fixes and sessions reaching LOCATED_PRECISE.** N.14 again: at most one
  finder is in a subject's UWB reach, and a single UWB range loses to the floor. In the
  endgame, 6 grants were confirmed and no UWB fix formed. PRECISE needs the delivered
  entry to name a grant the node still holds, and its tier has already fallen back by
  then.
- **Fixes at hop depth 2.** The relay chain (L.1.2) runs for plain C5 relays. Under the
  preset all five field nodes are measured subjects, which step through the MS-1 pipeline
  (N.3), so the chain never steps a short relay ("short relays stepped 0" in every run).
  Depth 1 appears in the endgame: 243 fixes.
- **Grant tie-break verdicts.** Grant gossip runs among the gateways on tier 1, while
  finders make 27 of the 31 grants. No two anchors granted the same request, so there was
  never a WIN or DEFER.

Each needs a change to the estimator or the network, not a setting: the N.14 fusion, the
relay chain for subjects, and finders taking part in grant gossip. They wait on the owner,
and G24 stays FAIL until they are made or the owner accepts the zeros. Open.

*2026-10-03, with GEO-01's ring (8 gateways):* the zero set moved. It is still six, and
they are populations of a sparse geometry, not defects of a mechanism:
- **Back:** Case B fixes (2 in the main run) and sessions reaching LOCATED_PRECISE (4 in
  the endgame, where the ring's gateways now sit 450 m from the centre and 10 grants were
  confirmed).
- **Still zero:** Case A, hop depth 2, grant tie-break verdicts, UWB-delivered fixes.
- **Newly zero at 5 km:**
  - triggered updates: no large correction fired;
  - per-hop variance terms: 3 relay legs reached admission, and none had the three
    readings a variance term needs. With the old default grid the subjects met often
    enough for both.

*2026-10-03, with CAL-02's calibrated PCS:* five zeros. Grant tie-break verdicts appear:
the calibrated thresholds move when subjects ask for UWB, and two anchors now granted one
request. The other five stand: Case A, triggered updates, hop depth 2, per-hop variance
terms, UWB-delivered fixes.

*2026-10-03, the routing weights (owner decision):* the preset sets CAMP-SPEC's registered
sweep pick, `--routeAlpha=0 --routeBeta=0 --routeGamma=1 --routeSlack=1` (82 settings). It
reproduces the sweep's `D_route_a0_b0_g1_s1` seed-1 cell byte for byte (37 files).
- **One choice is OURS.** The weights need `--fixRouting`, and a weight given without it is
  refused. A weight the preset applied is exempt: with `--fixRouting=false` given
  explicitly, as CAMP-SPEC's single-hop cells do, the preset's weights are unused, not
  refused. G24 runs that shape (x_froff) and requires exit 0.
- **The zero set moved, still five.** The routes changed, so the channel did, and every
  magnitude moved with it (rule 8).
  - Case A fixes are back: 1 in main.
  - Grant tie-break verdicts are zero again in the endgame: 0, where CAL-02's preset gave 2.
  - Both populations are one or two events in G24's single seed.
  - The five zeros: triggered updates, hop depth 2, per-hop variance terms, grant tie-break
    verdicts, UWB-delivered fixes.

*2026-10-03, `--hopVarianceTerms` out of the preset (owner decision, N.11):* 81 settings. G24
no longer asks for its population. It checks instead that the preset neither sets the flag
nor prints its NW-1 line (NOHV). Still four zeros, one of them new:
- triggered updates, hop depth 2 and UWB-delivered fixes, as before;
- requester escalations accepted in the endgame. No request was granted: 2 refused and 44
  unroutable, where the routing-weight preset had 1 accepted.
- Grant tie-break verdicts are back (1).

Like the moves before it, these are populations of a few events in one seed.

*2026-10-03, SY-2:* the preset sets `--radioBasis=lr1121` (82 settings). The same four zeros.

*2026-10-04, `--subjectRelayChain` (N.18; owner decision: preset entry if it shows a
population):* the preset sets it (83 settings).
- **Hop depth 2 has a population:** 137 of tag 0's fixes in the endgame, where subjects
  fused through the chain 39 times; with the flag off, 0.
- **A preset-applied `--subjectRelayChain` is unused, not refused,** when `--relayChain` is
  turned off explicitly, as the routing weights are with `--fixRouting`. Otherwise
  CAMP-SPEC's single-hop command line would be refused.
- **Case A and Case B fixes read 0 at 5 km,** where they were 1 and 3. The subjects' chain
  exchanges at 5 km are few, and none is admitted, but they are transmissions: the channel
  moved, and with it these one-event populations (rule 8).
- **The five zeros:** Case A, Case B, triggered updates, UWB-delivered fixes, requester
  escalations accepted.

### N.16 At 5 km the covariance trace ranks error backwards

CAL-02 (2026-10-03, report/CAL02_RESULTS.md). Under the preset at 5 km, ρ(−traceSigma,
error) over tag 0's fix cycles is +0.315 on the held-out seeds: a smaller trace goes with
a larger error. The facts behind it, read from the 40 seeds:
- 98 % of the fixes are 868 MHz floor fixes, and nearly all are SUSPECT, so DC-7 fuses
  almost every one at R × 4.
- Within the 868 MHz fixes the trace still ranks error backwards (+0.22).
- In the endgame (2.4 GHz fixes, a 750 m box) it ranks error correctly (−0.53 overall,
  −0.55 within 2.4 GHz).

**The consequence.** CAL-02 fitted the weights where the trace is least informative. FIT-R
wanted a negative trace weight, and the clamp to non-negative weights dropped it, leaving
(0, 1.92, 1.08). The calibrated score then "beats" the inverted control by the registered
test, while ranking error only weakly (ρ = −0.10).

**Not established:** why the RSSI-ranged, mostly-SUSPECT fixes invert the trace. Candidates
are:
- range-proportional σ near gateways;
- DC-7's inflation applied to almost every fix;
- the integrity check firing on nearly every preset fix (N.15's populations).

Each would need its own measurement. Open, for the owner, together with N.14.

### N.17 What `--radioBasis=lr1121` decides that the datasheet does not

SY-2 (2026-10-03, G61). The currents come from Semtech's LR1121 datasheet, DS.LR1121.W.APP
Rev 2.0 (Dec 2023), sect 3.4.1, Tables 3-4, 3-5 and 3-6, read from the PDF. Two
distributors' copies (sha256 `ede17ac3...` and `f36bcf2d...`) carry the same tables; both are
kept outside the repository in `~/catbl_work/datasheets/`. The choices:
- **The revision.** Semtech's product page lists a datasheet dated 2025-04-26. Its download
  is a script-driven viewer that would not serve the file, and a Rev 2.1 mirror is behind a
  browser challenge. Rev 2.0 is the one read, so it is the one cited. A later revision
  could move a figure.
- **TX at 2.4 GHz:** 26 mA, the +13 dBm row. The scenario transmits 12.5 dBm; the table
  has +13 and +10 dBm. Taking the setting that delivers 12.5 dBm is OURS.
- **RX:** the RxBoosted figures, 7.8 mA (sub-GHz, 125 kHz) and 7.5 mA (2.4 GHz, 812 kHz,
  the LR1121 setting nearest the scenario's 800 kHz). The SX1262 profile was boosted too,
  and the sensitivity tables assume it.
- **CAD:** at the RX current. The datasheet gives no CAD figure; the SX1262 profile's
  "CAD draws RX-equivalent current" is kept. OURS.
- **Idle and sleep:** STBY_RC (1.25 mA) and SLEEP with the RC-based RTC, no RAM retained
  (1.6 µA), the counterparts of the SX profiles' choices.
- **Timing:** CAD duration, turnaround, wake-up, guards and timeouts are not tabulated. They
  are kept from the SX profiles, so a basis change moves currents only. That is what makes
  G61's ratio check exact; it is not a claim that the LR1121's timing equals the SX parts'.
- **What it changes beyond energy.** At defaults, nothing (G61: 6 arms and 6 traces, no
  non-energy value moved). Under `--specProfile` energy feeds the escalation request's
  battery term, and so arbitration. G24's populations at 82 settings: the same four zeros
  as before.
- **Seen while building G61:** `subject_summary.csv` carries only the tx and sleep buckets
  of each tier, not rx, CAD or idle. So a subject's energy can be checked only through
  those buckets and the tier totals. Recorded, not changed: adding columns would change the
  file every subject run writes.

### N.18 What `--subjectRelayChain` decides that the plan does not

2026-10-04, G62; the owner's decision of 2026-10-03 to build it behind a flag. Measured
subjects stepped through the MS-1 pipeline (N.3) and never ranged another node, so their
depth stayed 0. A range through a subject was then depth 1, and with every field node a
subject depth 2 could not arise (G24's zero). The flag runs L.1.2's short-relay exchange for
subjects. The choices, each OURS:
- **The tier is the subject's active tier,** not `--relayTier`. A subject escalates, and it
  transmits on one tier a cycle (sect 6), which pays for the exchange.
- **Never on 868 MHz.** A1 refuses every relayed range on tier 1, so a subject on 868 MHz
  makes no exchange. A first build did make them, and all 21 in G62's hybrid run were
  refused and billed.
- **A holder must be awake on that tier:** its active tier, or 868 MHz while a hybrid arm
  keeps the floor. A sleeping radio does not answer (NW-0's rule).
- **Holders are nodes stepped earlier in the cycle with a fix:** plain relays and lower-id
  subjects. Tag 0 steps after every subject, so it is never one.
- **Relayed ranges only complete a full set.** They join the active set only if it then
  has three or more. The sparse cases never take one, and the floor's 868 MHz ranges are
  never mixed with them (the fix selection's rule; N.14's flag changes that).
- **Depth is carried across cycles,** as L.1.2's is. A subject re-grounds only on three
  direct ranges.

**Measured.**
- G62's regime (static_2p4, 200 m box, four subjects, 600 s): 4 subject fuses at depth 1,
  and 16 of tag 0's fixes at depth 2.
- In the hybrid arm of the same regime, no exchange. On 2.4 GHz the floor solves, so the
  subject holds a fix; on 868 MHz A1 admits nothing (554 short subject-cycles).
- Under the preset (900 s, seeds 7, 1 and 2; the preset sets the flag since 2026-10-04):
  - at 5 km, no admitted exchange: subjects are kilometres apart;
  - in the endgame, subjects fused through the chain 39-67 times a run, and tag 0 had 50,
    137 and 183 fixes at depth 2.

