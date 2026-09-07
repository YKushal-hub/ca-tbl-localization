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

### H.1 Tier 3 is unreachable in the tested configuration, and the gate now makes that visible
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
