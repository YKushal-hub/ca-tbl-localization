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

### 3.2 `hopWeight` stays 0, and the PCS hop term is the wrong shape
`ca-tbl-policy.cc:672`, `:708`. The current term is **unbounded below**, penalises a
*count* rather than an *uncertainty the covariance already carries*, and does not encode
the shared-error property that is the whole reason provenance exists. A bounded
multiplicative replacement is specified in `docs/RELAYED_RANGING_ERROR_MODEL.md` §5 and
is registered as a **separate PCS campaign**, not part of C5.

Also: **A2 caps hop depth at 1, so `hopCount` is binary.** A term linear in it has two
reachable values. Worth knowing before anyone tunes a weight expecting a gradient.

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

### 5.1 106 commits on GitHub carry the wrong author
Fixed for the **57 unpushed** commits on 2026-09-06 (`SHA_MAP.md`, second section). The
106 already published still say `Kushal Yalamanchi <kushalyalamanchi@gmail.com>`, which is
not the owner. Correcting them means a **force-push to a protected branch**. *Cost:* the
rewrite itself is minutes; the coordination is the cost. **Nothing is credited to the
owner's contribution graph until commits reach the default branch.**

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
| **C.1 Hop weight is zero / provenance inert** | "the confidence side is untouched" | **Partly closed.** The provenance term reaches the score (`57ebb41`) and now has a non-zero input (`edf73a5`): share 0.20 over 180 cycles, and `--hopProvenanceForm` 0 vs 1 finally differ. **Still open:** whether it carries information the covariance does not, and per-hop RSSI/SNR variance, which still never reaches the score. |
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
