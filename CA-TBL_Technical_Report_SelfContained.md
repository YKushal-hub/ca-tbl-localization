# CA-TBL — Technical Report

**Confidence-Aware Tier-Based Localization for GPS-Denied Search and Rescue**

Status **2026-08-05**, after Parts 0–8 of `report/PCS_REDESIGN_RESULTS.md`.
Baseline `results/baseline_v6` · `run_id` `bd99a6548154dc85` · source `0be15a19b` ·
verifier 136 checks / 155 gates / `arms_audited 6/6`, zero failures.

**This revision replaces the previous one wholesale.** That version was written
against `7e23ad903`, before the NLOS defect was fixed, and its §1, §6 and §8 were
produced by a 2.6× measurement-variance error that was doing the work the confidence
model was credited with. Its numbers must not be quoted. The full correction chain is
preserved in `report/PCS_REDESIGN_RESULTS.md` and summarised in the Appendix here.

---

## 0. How to read this document

Organised **claim by claim**. Each claim states what was **intended**, how it was
**implemented**, what it **measured**, and how we **proceed**.

Three reading rules, each earned by a specific error in this project's history:

1. **Every comparison is paired within a seed.** Single-seed figures are marked `n=1`
   and are not conclusions — one n=1 result here reversed at 20 seeds.
2. **Geometry scope is part of every claim.** `boxHalf=1000`/`gwGrid=3` is
   2.25 gw/km²; `boxHalf=3000`/`gwGrid=3` is 0.25 gw/km². Results at the second
   without a constant-density control are confounded.
3. **`cep50M` and `rmseM` diverge for the escalating arms** — the two halves of a
   bimodal error distribution. Quoting one alone chooses a winner.

### 0.1 Verdicts

| # | Claim | Verdict | Evidence |
|---|---|---|---|
| 1 | Hybrid beats single-band on accuracy-per-joule | **FAILS** | 0/20 seeds · 3 geometries · 4 environments · 5 `searchDuty` settings |
| 2 | The handoff is worth its complexity | **FAILS** | 0/20 crediting-invariant at 2 of 3 geometries; static tier 2 dominates 13/13 Pareto points |
| 3 | Handoffs trigger correctly | **FAILS** | recall 0.088 as trigger; a defined 0.000 as gate |
| 4 | Custom handshake | **NOT ON THE WIRE** | billed, never transmitted |
| 5 | Specialized packet taxonomy | **PARTIAL** | 4 of 9 types transmitted |
| 6 | Real physics on real packets | **WORKS** | 14.5k emergent captures; confounded by beacon flooding |
| **N3** | **Integrity check at the switching seam** (central novelty) | **NO BENEFIT** | null in 3 of 4 configurations, adverse in 1 |
| N1 | Self-deployable mesh | **NO SUBSTRATE** | not implemented |
| N2 | Hop-aware confidence degradation | **NO SUBSTRATE** | `hopWeight = 0`, one-hop by construction |
| N4 | Identity-scoped multi-target arbitration | **NO SUBSTRATE** | one tag; scored offline only |

### 0.2 What survives as a contribution

Not the confidence model. Three findings and one apparatus:

- **A regulatory chain** from a duty-cycle limit to metres of position error, holding
  under both PCS forms and independent of the decision layer working (§8).
- **A mechanism**: absolute-target tier escalation converts infrastructure
  improvement into accuracy regression (§9).
- **A deployment requirement**: continuity and accuracy are bought with *different*
  infrastructure, and accuracy is governed by precision-tier coverage fraction, not
  gateway density (§10).
- **The measurement apparatus** — determinism, provenance, gates, and a
  pre-registration that caught an n=1 false positive before it was published (§12).

---

## 1. Claim 1 — a hybrid escalation architecture beats single-band

### 1.1 Intended
A node running a cheap wide-area tier continuously and escalating to a precision tier
only when position confidence justifies the energy should beat any single band on
accuracy per joule, because it pays for precision only when precision is needed.

### 1.2 Implemented
Six arms against identical channel realisations: three single-band controls
(`static_868`, `static_2p4`, `static_uwb`), a literature baseline (`fixed_threshold`,
flat variance trigger), and two proposed arms (`ca_tbl_full`, `ca_tbl_no_integrity`).
Energy charged per radio action from datasheet currents. Metric
`accuracyPerJoule = 1/(rmseM × totalJ)`, paired per seed, geometric mean of per-seed
ratios.

### 1.3 Measured — fails everywhere tested

`ca_tbl_full` vs `static_2p4`, 20 seeds paired. Ratios below 1.0 mean the single band
wins:

| geometry | density | legacy PCS | PCS v2 | seeds won |
|---|---|---|---|---|
| `boxHalf=1000` | 2.25 gw/km² | 0.3815 [0.3376, 0.4313] | **0.3140 [0.2481, 0.3975]** | **0/20** |
| `boxHalf=3000`, `gwGrid=3` | 0.25 gw/km² | 0.0118 [0.0081, 0.0174] | 0.0049 [0.0032, 0.0075] | **0/20** |
| `boxHalf=3000`, `gwGrid=9` | 2.25 gw/km² | 0.0207 [0.0135, 0.0317] | **0.0147 [0.0104, 0.0209]** | **0/20** |

**The redesigned score is worse than the one it replaced** — v2 spends 47.1 J against
legacy's 27.0 J for a modest accuracy gain (2.25 m vs 3.09 m), moving the claim
*further* from passing.

The verdict does not move under any parameter tested:

| axis | result |
|---|---|
| `searchDuty` (uncited, spans 828×) | loses by **6.6× to 202.9×**; **0/20 at all five settings**. Changes magnitude 31×, ranking at not one seed. |
| Environment (4 settings) | 0/20 in RuralForested/dense, RuralOpen/sparse, Suburban/light, UrbanModerate/none |
| Density de-confounding | loss moved 202.9× → 67.9×. Two-thirds of the apparent degradation with scale was falling gateway density. **No conclusion reversed.** |

### 1.4 The one place the picture is not one-sided

At `boxHalf=3000`/`gwGrid=9`, equal RMSE conceals opposite failure modes:

| arm | cep50M | rmseM | outage |
|---|---|---|---|
| `static_2p4` | **6.03 m** | 57.86 m | **46.25%** |
| `ca_tbl_full` | 26.12 m | 57.16 m | **1.54%** |
| `static_868` | 63.21 m | 83.43 m | 0.01% |

`static_2p4`'s excellent median is scored **only on the cycles where it works**. This
is the continuity question, treated as its own finding in §10.

### 1.5 Proceeding
**Stop testing Claim 1 as stated.** Closed negative from five independent directions.
Write it up as a quantified negative architectural result:

> A hybrid escalation architecture does not beat well-sited single-band 2.4 GHz on
> accuracy-per-joule at any geometry, density, environment or energy-model setting
> tested. Where tier 2 has coverage, pin the radio to it.

---

## 2. Claim 2 — the handoff is worth its complexity

### 2.1 Intended
A confidence-based policy should beat a naive flat variance threshold — the prior-art
baseline — because it fuses geometry, LOS quality and posterior uncertainty rather
than reading one number.

### 2.2 Implemented
`ca_tbl_full` against `fixed_threshold` (`traceSigma > 100 m²`), under two PCS forms:

- **legacy** — `traceRef/(traceRef+traceSigma) + 1/max(gdop,1) + nLos/nTotal`,
  thresholds `1.55/1.70/2.30` carried verbatim from a Python oracle, never calibrated.
- **v2**, pre-registered before any run — `log10(sigma_ref / sigma_eff)` with
  `sigma_eff = sqrt(sigma_post × sigma_geo)` and `sigma_geo` from the exact
  `(JᵀR⁻¹J)⁻¹`. Threshold **derived**: `pcsThresholdUp = 0.0`, one target for all
  tiers, because the score is zero exactly when uncertainty meets the stated 3.1623 m
  specification. This removed two free parameters.

Three measures, because acc/J alone is contestable:

1. `accuracyPerJoule` ratio, paired per seed.
2. **Marginal m/J above the shared tier-1 floor** — metres of RMSE removed per joule
   spent above `static_868` at the same seed. Crediting-invariant.
3. **Pareto fronts at matched operating points** — 9 points per family on a shared
   sigma grid, because a single-point comparison at unmatched thresholds is unfair in
   whichever direction the knee happens to lie.

### 2.3 Measured — fails on all three

**Accuracy-per-joule**, v2 vs `fixed_threshold`:

| geometry | ratio [95% CI] | seeds won |
|---|---|---|
| b1000 | 0.8131 [0.6399, 1.0332] | 10/20 — tie, point estimate against |
| b3000, gw3 | 0.2478 [0.1902, 0.3229] | 0/20 |
| b3000, gw9 | 0.3559 [0.3015, 0.4202] | 0/20 |

**Marginal m/J above the floor** — ratio `fixed_threshold` / `ca_tbl_full`; `>1` means
the flat threshold converts joules into accuracy more efficiently:

| geometry | ratio [95% CI] | seeds `ca_tbl_full` wins |
|---|---|---|
| b1000 | **1.3453 [1.2495, 1.4483]** | 0/20 |
| b3000 | **1.5431 [1.0787, 2.2073]** | 2/20 |
| gw9 | **2.5785 [1.9940, 3.3343]** | 0/20 |

**Pareto fronts**, b1000, 18 points × 20 seeds. The sweep script's built-in verdict
string says the v2 front dominates. **It must not be quoted**, for three reasons: the
fronts are in `cep50M` while §2.3's ratios are in `rmseM` and the two disagree; the
dominance rests on **one** point with margins of 0.9 cm and 1.1 J; and —

> **`static_2p4` (cep50 1.5066 m, 16.68 J) dominates 13 of 13 points on both
> fronts.** Not one operating point of either decision rule achieves both a better
> median and lower energy than statically pinning the radio to 2.4 GHz.

Arguing which decision rule has the better front is arguing about the ordering of two
dominated sets. **Scope:** b1000, where `static_2p4`'s outage is 0.00%. At b3000 it
dominates the same axes while in 45.98% outage — the axes exclude availability.

### 2.4 Why the confidence model does not work

PCS carries no information beyond the EKF trace it is computed from. 20 seeds,
Spearman, fix cycles only, seed-level bootstrap:

| arm | esc/fix | ρ(PCS) [95% CI] | ρ(monotone control `−log10 σ_post`) |
|---|---|---|---|
| `ca_tbl_full` v2 | 0.0062 | **−0.199 [−0.232, −0.165]** | **−0.200 [−0.235, −0.166]** |
| `ca_tbl_no_integrity` v2 | 0.0047 | −0.181 | −0.182 |
| `fixed_threshold` | 0.1254 | −0.531 | −0.539 |

Pre-registered acceptance was ρ "materially closer to −0.78 than to −0.09."
**v2 scores −0.199 — and is marginally worse than the trivial control.** Its two terms
correlate −0.200 and −0.195 against the sum's −0.198: not decoration on each other but
**redundant**, tracking the same quantity.

*Do not rank arms by these ρ values.* `fixed_threshold`'s −0.531 is largely range
restriction — it spends 37% of fix cycles on tier 1 and spans far more uncertainty,
while v2 parks on tier 2.

The per-term variance decomposition **did not** fire the pre-committed ">90% from
`sigma_post`" verdict (measured 11.0%), and it was not stretched to fit. A different
pre-registered instrument caught the failure instead.

**An inverted failure mode the pre-registration did not anticipate:** at an escalation
rate of 0.0062 per fix cycle, a score can correlate arbitrarily well with realised
error while almost never acting on it. ρ measures the score's *information*;
escalation rate measures whether the policy *uses* it. **Both must always be reported
together.**

### 2.5 The pre-registration earning its keep

A single-seed probe showed v2 beating `fixed_threshold` by **1.33×** — the first time
any configuration of the decision layer had beaten the flat threshold. At 20 seeds:
**0.8131**, 10/20 won. Per-seed ratios span 0.2374–1.3968; seed 7 ranks **2 of 20**.
The sweep's seed-7 rows are byte-identical to the probe — it was not wrong, it was
unrepresentative.

Form, both thresholds, acceptance criteria, controls and estimator were all fixed
before any v2 run. When the tempting number appeared there was nothing left to adjust
that would not have been visible as an adjustment. **This is the most citable
methodological result in the project.**

### 2.6 Proceeding
Claim 2 is closed negative. Two items remain worth doing, neither of which reopens it:

- **Hysteresis.** `pcsThresholdDown = 0.301` was pre-registered at one octave and
  measurably fails — at b3000 the arm changes tier on **29.1% of cycles**, worse than
  the flat threshold's 24.1%. Not retuned. `pcsThresholdDown` and `nDeescalate` have
  no CLI flag; exposing them is the prerequisite.
- **Intra-cycle handshake scheduling** (§8.4), the only change with a derived
  mechanism predicting a real improvement.

---

## 3. Claim 3 — handoffs trigger correctly

### 3.1 Intended
Escalation should fire when the precision tier can actually deliver, and not
otherwise.

### 3.2 Implemented
Ground truth is **geometric**: `shouldUwb := visUwb >= 3` — "could UWB have solved
this cycle". Confusion matrix per cycle; `trig_f1` per arm.

**This is a coverage oracle, not a policy oracle.** A policy could be right by its own
lights and score badly. The limitation is structural and was never resolved.

### 3.3 Measured

`ca_tbl_full`, seed 7:

| configuration | tp | fp | fn | precision | recall | f1 |
|---|---|---|---|---|---|---|
| legacy + arbitration | 155 | 966 | 4 | 0.1383 | 0.9748 | 0.2422 |
| propagation + arbitration (gate as trigger) | 14 | 0 | 145 | **1.0000** | **0.0881** | 0.1618 |
| gate as gate (`suspectForcesEscalation=false`) | 0 | 0 | 159 | undefined | **0.0000** | 0.0000 |

Fixing the NLOS defect **flipped the operating point rather than improving it** — from
escalating almost always (966 false positives) to escalating almost never but never
wrongly.

The third row is not a scoring artefact. Pooled over 20 seeds: tp=0, fp=0,
**fn=3310**, tn=25490. A positive class of 3310 cycles exists; precision is undefined
(0/0), **recall is a defined, measured zero**. With its central mechanism acting as
designed, the arm escalated to UWB on **none of 3310 opportunities**.

### 3.4 Proceeding
Two separable problems:

- **The oracle.** `visUwb >= 3` cannot distinguish "should have escalated" from "could
  have". An error-based oracle needs a counterfactual per cycle — expensive but
  well-defined.
- **The trigger.** Its behaviour is a consequence of §9's absolute-target mechanism,
  not an independent defect. Fixing it means moving the threshold from absolute to
  relative, which reintroduces the free parameter the pre-registration removed.

---

## 4. Claim 4 — the custom handshake

### 4.1 Intended
Mobility state selects which messages a node sends: stationary/approaching gets
Beacon → Digest → targeted Entry-Push at `CW_normal`; receding skips the digest for a
one-shot urgent push at `CW_high`, because the link is degrading and a round-trip may
not complete.

### 4.2 Implemented
`CaTblMac::StartHandshake()` exists, branches on mobility state, and is called on
escalation. The round-trip is **charged** via `AccountEscalationHandshake()`.

### 4.3 Measured — the handshake is billed, not sent

- **`ESC_REQUEST`, `ESC_GRANT`, `QUERY`, `RESPONSE`, `ENTRY_PUSH` are never
  constructed or transmitted.** Defined, sized, given access categories and ACK
  policies — and never reaching the wire.
- **Nothing sets mobility state** (`ca_tbl_causal_sim.cc:2513-2517`). Every node
  reports `STATIONARY`, so the RECEDING branch — the interesting half — never runs.
- The energy model charges a 12-byte frame plus a guarded RX window per escalation, so
  the *cost* is modelled while the *behaviour* is not.

**A real finding came out of the billing anyway** — §8 — and it was only discoverable
because the cost was charged honestly.

### 4.4 Proceeding
Claim 4 needs two things that do not exist: a mobility-state estimator, and the
ESC/QUERY path on the wire. Both are Phase 8 scope. Until then the claim is exercised
on its stationary path only, and that must be stated rather than implied.

---

## 5. Claim 5 — the specialized packet taxonomy

### 5.1 Intended
Nine message types across seven classes, each with an access category, an ACK policy
and a payload sizing rule, so message type drives contention window drives channel
access.

### 5.2 Implemented
Fully specified in `ca-tbl-header.{h,cc}`: `DISCOVERY` through `ACK`, four access
categories with CW bounds, four ACK policies, payload sizing (28 B per table entry,
4 B per digest entry, 9 B MAC header). S4 unit suite passes 38/38.

### 5.3 Measured — 4 of 9 types reach the wire

| type | transmitted | note |
|---|---|---|
| `DISCOVERY` | **yes** | ~60,840 beacons/run plus ~140k flood relays |
| `ACK` | yes | explicit-ACK path |
| `DIGEST`, `ENTRY_PUSH` | via `StartHandshake` only | not on escalation in practice |
| `ESC_REQUEST`, `ESC_GRANT`, `QUERY`, `RESPONSE`, `TRIGGERED_UPDATE` | **no** | never constructed |

The priority separation the taxonomy exists to demonstrate — URGENT 1.89 ms against
LOW 15.96 ms, an 8.4× access-delay ratio — is measured **in the S4 unit test**, not in
the causal simulation, because the types that would exercise it are not sent.

**And `DISCOVERY` floods.** `CaTblIsFlooded(DISCOVERY)` is true, so beacons get
`hop_limit = 3`. Anchors cannot hear each other, but the node hears them and
rebroadcasts and anchors hear the node. **~140,000 of ~200,000 transmissions are flood
relays**, so the collision statistics measure flood traffic, not discovery contention.

### 5.4 Proceeding
- Decide whether a one-hop "I am here" announcement should be a flooded type at all.
  `--discoveryHopLimit=1` would make the MAC statistics measure what claim 6 wants.
- The remaining five types are Phase 8 scope alongside claim 4.

---

## 6. Claim 6 — real physics applied to real packets

### 6.1 Intended
Collisions should be an emergent consequence of who transmitted when — which follows
from CSMA/CA backoff, which follows from the access category the message type
selected — rather than a probability drawn per link.

### 6.2 Implemented
`CaTblChannel` tracks in-flight transmissions and arbitrates at each receiver by
capture threshold. `EnginePhyProvider` backs it with the three real physics engines.
CAD-then-backoff with per-category contention windows, sliding-window duty-cycle
enforcement, APC.

### 6.3 Measured — this claim works

| quantity | value |
|---|---|
| transmissions | ~200,000 / run |
| collisions | ~40,000 (20%) |
| **captures** | **~14,500** |
| deliveries | ~240,000 |
| `duty_868` | 0.02170 against the 0.025 Table-II cap |

**Captures were exactly 0 with the stub PHY** and ~14,500 with the real one, because
capture arbitration needs a received-power spread and a stub supplies one value for
every link. That is direct evidence the physics reaches the MAC.

Two structural limits, stated rather than hidden:

- **Anchors are hidden terminals to each other.** Only the field node is registered as
  a node on the engines, so anchor↔anchor links do not exist and anchor CAD never
  detects another anchor. Beacon loss is therefore an **upper bound**.
- **Collision counts are dominated by flood relays** (§5.3).

### 6.4 Proceeding
The one claim that can be written up as working. Both limits go in the limitations
paragraph, not the results paragraph.

---

## 7. Central novelty — the integrity check at the switching seam

### 7.1 Intended
A trust check at the LoRa→UWB switching seam, framed as **environmental fault
tolerance**: cold-sagged battery skewing RSSI, antenna detune from ice or damage,
canopy multipath. A fix that is statistically clean (all links LOS) but violates
physical continuity is the failure the confidence score cannot see.

### 7.2 Implemented
Filter-free multilateration from this cycle's ranges, compared against the EKF prior by
Mahalanobis distance, gated at χ²₀.₉₉ (9.21, 2 DOF), additionally requiring
`nNlos == 0`. On firing, either the measurement is rejected (`RevertToPrior`) or —
under the default `suspectForcesEscalation = true` — escalation is forced.

Two mis-specifications were found and fixed under `--mahalUseR`:
`MahalanobisFromPrior` omitted `R`, normalising by the prior covariance alone; and
`RawMultilateration` was unweighted least squares, so the tested point was itself
biased. Correcting both collapsed firings 89 → 5 — from 61% to 3.2% of the eligible
population, which is what a χ²₀.₉₉ gate should do.

### 7.3 Measured — no benefit in any configuration

Ablation `ca_tbl_full` − `ca_tbl_no_integrity` with the gate acting **as a gate**
(`--suspectForcesEscalation=false`), 20 seeds:

| form | geometry | suspects/run | ablation on rmseM |
|---|---|---|---|
| legacy | b1000 | 220.5 | **+24.64 m [+18.91, +30.37]** — adverse |
| v2 | b1000 | 23.2 | −0.096 m [−0.288, +0.096] — n.s. |
| legacy | b3000 | 362.5 (25.2%) | −6.97 m [−53.21, +39.27] — n.s. |
| v2 | b3000 | 382.5 (26.6%) | +53.34 m [−3.36, +110.05] — n.s. |

> The integrity check shows **no significant benefit in any of the four
> configurations tested**. It is significantly adverse in exactly one, and
> statistically null in the other three.

An earlier explanation — "v2 barely fires" — **does not survive the second geometry**:
at b3000 v2 fires on 26.6% of cycles, sixteen times more often, and the ablation is
still null.

### 7.4 The scope problem underneath the result

Two things make this not yet a test of the claim as written:

1. **It never fires at the seam it is named for.** The check runs on every fix and
   empirically fires on tier-1 fallback fixes. The claim describes a seam check; the
   code implements a general innovation gate.
2. **It has never been shown a fault.** S7 environmental fault injection does not
   exist. Every firing measured is on natural channel variation. The all-LOS eligible
   population moving from 0.00% to 90.26% after the NLOS fix is **not** evidence the
   gate works.

There is also a novelty exposure: a Mahalanobis innovation gate at a χ² threshold is
textbook validation gating (Bar-Shalom). Re-scoping the claim to match the code
collapses part of it into standard practice.

### 7.5 Proceeding — S7 is the only route

Build fault injection around five constraints the measurements have established:

- **Inject at the physics/scenario boundary** so the policy cannot see it.
- **Canopy multipath as a common-mode range bias that does not set the NLOS flag** —
  `suspect` requires all-LOS, so any fault tripping the NLOS flag can never fire it.
- **Score against damage** (`inducedErrM > τ`, faulted minus unfaulted fix), not
  against fault presence. A fault can be active and the fix still fine.
- **Report ROC (TPR vs FPR)**, not precision/recall — fault prevalence is an uncited
  stress parameter, and precision bakes an arbitrary prevalence into the headline.
- **Build the residual-threshold control in the same pass** — a one-parameter
  threshold on raw multilateration residual RMS. If it matches the χ² gate's ROC, the
  contribution is the formalism only.

Also required: gate the check to the seam so the code implements the claim.

---

## 8. Finding A — the architecture cannot afford to signal its own handoffs

**Independent of the confidence model, and holds under both PCS forms.** The strongest
result in the project.

### 8.1 The arithmetic
Tier 1 is SF7 / 125 kHz / CR 4/5:

| quantity | value |
|---|---|
| WPC G.S.R. 853(E) Table-II budget | 2.5% |
| budget per `dt = 2.5 s` cycle | **62.50 ms** |
| ToA, 20-byte ranging frame | **56.58 ms** |
| measured tier-1 airtime per cycle | **56.58 ms** |
| remaining budget | **5.92 ms** |
| ToA, 12-byte handshake frame, TX alone | **41.22 ms** |

Measured airtime equals computed ToA to five significant figures, independently
confirming the tier-1 budget is spent on exactly one ranging exchange.
**The handshake needs 41.22 ms and 5.92 ms remain**, and that is a lower bound because
the exchange also charges a guarded RX window.

### 8.2 Raising the cycle period does not fix it — the prediction failed

Derived compliance floor: `(56.58 + 41.22)/0.025 = 3.91 s`. Pre-registered
expectation: `handshakePreemptions → 0` between 3.0 and 3.91 s.

| `dt` | escalations | **preemptions** | duty_868 | cep50M | rmseM |
|---|---|---|---|---|---|
| 2.5 s | 218.35 | **216.45** | 0.02171 | 1.794 | 3.086 |
| 3.0 s | 182.40 | **181.95** | 0.01808 | 1.833 | 3.122 |
| 3.91 s | 140.25 | **139.90** | 0.01386 | 1.927 | 3.791 |
| 5.0 s | 109.80 | **109.60** | 0.01085 | 1.959 | 3.915 |

**Preemptions track escalations at ≈0.99 at every period**, including 5.0 s where duty
is 0.0108 against a 0.025 cap.

**Mechanism** — `ca-tbl-energy-model.cc:226`:
`m_nextAllowedTxS = nowS + airtimeS/limit`. A 56.58 ms frame arms a **2.263 s**
lockout, and the handshake is attempted at the *same cycle timestamp*, immediately
after ranging. `dt` sets when the next cycle begins; it does not separate two
transmissions inside one cycle. **The binding constraint is intra-cycle scheduling,
not the average budget.**

### 8.3 The result

> Moving to the derived 3.91 s compliance floor costs **+0.13 m median and +0.71 m
> RMSE**, saves 33% energy, and **does not eliminate the handoff's ranging-slot
> cost** — which was the entire reason for the figure. You pay the accuracy price and
> get nothing for it.

At 1.5 m/s the target moves 3.75 m between fixes at 2.5 s and 5.87 m at 3.91 s,
against a 3.1623 m accuracy target: the prediction step dominates.

### 8.4 Proceeding — the strongest remaining candidate
Because the constraint is ordering rather than rate, **scheduling the handshake
≥2.263 s after the ranging within the same cycle should eliminate the pre-emption at
any `dt ≥ ~2.32 s`, at no accuracy cost.** This is arithmetic from the lockout rule,
**not a run**, and it is the highest-value unmeasured item in the project.

---

## 9. Finding B — absolute-target escalation inverts the value of infrastructure

### 9.1 The measurement
`boxHalf=3000`, legacy form, 20 seeds per point:

| gateways | gw/km² | `ca_tbl_full` cep50 | `fixed_threshold` cep50 | `static_868` cep50 |
|---|---|---|---|---|
| N=4 | 0.44 | 11.86 m | 9.51 m | 168.36 m |
| N=5 | 0.69 | 11.49 m | 8.94 m | 119.02 m |
| N=6 | 1.00 | 12.86 m | 8.67 m | 102.27 m |
| N=7 | 1.36 | 15.77 m | 9.37 m | 87.51 m |
| N=9 | 2.25 | **26.12 m** | 9.07 m | 63.20 m |

As the network improves by every conventional measure — tier-1 median 168 → 63 m,
tier-1 outage 5.66% → 0.01% — **the proposed system's median error more than doubles.**

### 9.2 The mechanism
PCS v2 is `log10(sigma_ref/sigma_eff)` with `pcsThresholdUp = 0.0`, which reads as
*escalate while `sigma_eff` exceeds 3.1623 m*. As density rises, tier 1's `sigma_eff`
falls, the policy is satisfied on tier 1 more often, escalates less, and converges to
tier 1's own accuracy. **The system behaves exactly as specified; the specification
produces the pathology.**

`fixed_threshold` is flat because its trigger (σ > 10 m) sits outside the range where
density moves the decision — tier 1's median is 63 m even at the densest point tested.
**It is not immune; a tighter flat threshold would show the same effect.** This is not
a win for flat thresholds.

### 9.3 The uncomfortable corollary
The pre-registration defended `pcsThresholdUp = 0.0` as *derived, not chosen*,
removing two free parameters. **That derivation is what produces the pathology**: a
threshold restating the specification is by construction insensitive to how the
specification is met. A principled threshold and a working policy turned out to be in
tension — and the pre-registration is what makes that statement credible rather than
post-hoc, because the threshold was fixed before the pathology was found.

### 9.4 Proceeding
A **relative** target — escalate when the next tier would improve `sigma_eff` by some
factor — would not have this property, but reintroduces the free parameter that was
removed. **Unmeasured**, and the trade is explicit.

The effect is invisible at any single density. That is the general lesson: a
single-point evaluation would not have found it.

---

## 10. Finding C — continuity and accuracy are bought with different infrastructure

### 10.1 The claim, and its scope
> Only a hybrid achieves both continuity and sub-10 m median accuracy.

Established at `boxHalf=1000`. At `boxHalf=3000`, **no arm achieves outage < 5% AND
cep50 < 10 m at any of six densities**, including both hybrids. They fail in opposite
directions: `ca_tbl_full` reaches 1.54% outage but degrades to 26.12 m median;
`fixed_threshold` holds ~9 m but plateaus at ~6% outage.

### 10.2 The correction — density is not the explanatory variable

| geometry | area | gateways | **density** | UWB coverage | `ca_tbl_full` cep50 |
|---|---|---|---|---|---|
| b1000, gwGrid=3 | 4 km² | 9 | **2.25 gw/km²** | **11.49%** | **1.53 m** |
| b3000, gwGrid=3 | 36 km² | 9 | 0.25 gw/km² | 3.39% | 13.45 m |
| b3000, gwGrid=9 | 36 km² | 81 | **2.25 gw/km²** | **3.39%** | **12.05 m** |

**Rows 1 and 3 have identical density and differ 8× in median error.** The variable
tracking the outcome is **precision-tier coverage fraction**, unchanged by gateway
count because facilities are placed independently of `gwGrid`.

**The mechanism is a dwell/transit ratio.** Sites sit at `±0.47 × boxHalf`, so
inter-site separation scales with the box while anchor rings keep a fixed radius.
Coverage fraction ≈ `dwell/(dwell + transit)`:

| boxHalf | separation | transit @ 1.5 m/s | predicted | **measured** |
|---|---|---|---|---|
| 1000 | 940 m | 627 s | 12.6% | **11.49%** |
| 3000 | 2820 m | 1880 s | 4.6% | **3.39%** |

Two independent predictions within 1–2 percentage points.

### 10.3 The restated claim
> **Gateway density governs continuity** — tier-1 outage falls 45.06% → 0.01% across
> the sweep, and tier 1 alone becomes a complete floor at **0.69 gw/km²**.
> **Precision-tier coverage fraction governs accuracy** — median 1.53 m at 11.49%
> coverage, 12–13 m at 3.39%, *at any density*. The conjunction requires both, and
> they are bought with different infrastructure.

This converts "the conjunction fails at scale", which is fatal, into a **deployment
requirement**, which is useful.

### 10.4 Proceeding — the cheap test exists and was missed
`--dwell` already exists (default 90 s). Restoring b1000's coverage fraction at b3000
needs `dwell/(dwell + 1880) ≈ 0.115`, i.e. **dwell ≈ 245 s**. Sweeping `--dwell` at
b3000 varies the candidate explanatory variable directly, **over the existing binary**.

**Caveat stated in advance:** raising dwell and adding facility sites both raise
coverage fraction but are not the same intervention. If the conjunction returns under
one and not the other, coverage fraction is not the whole story. The facility-count
sweep is the cleaner test and needs a `BuildDeployment()` change; the dwell sweep runs
first because it is free.

---

## 11. Claims with no substrate

Stated plainly rather than deferred, because three of the four locked novelty
components are here.

| Component | Status |
|---|---|
| **N1 — self-deployable mesh** | Not implemented. No routing, no multi-hop, no route table. |
| **N2 — hop-aware confidence degradation** | `hopWeight = 0`; `hopCount` passed as literal `0`, which is **correct not stubbed** — the neighbour table is one-hop by construction. Requires N1 first. |
| **N4 — identity-scoped multi-target arbitration** | One tag in the simulation; `U(r)` scored offline only. Identity classes defined and unused. |

**Two of the four claimed novelty components have no substrate at all**, and the third
(N3) has no measured benefit. Only the tier architecture itself is exercised, and that
is prior art (Poucet).

---

## 12. What works — the verified apparatus

A genuine contribution, and it should be written up as one.

| Property | Evidence |
|---|---|
| Determinism from seed | Byte-identical repeat runs at defaults and six non-default flag settings |
| Arms share a channel realisation | Null ablation byte-identical; `ResetNextStreamIndex()` per arm |
| Energy traceable to datasheet | `rx_uwb` reconciles to `1015 × 2.0 s × 94 mA × 3.3 V` within 0.61 J |
| Independent re-derivation | 42 of 43 audited numbers reproduced under code sharing nothing with the originals |
| Gate coverage | 136 checks / 155 gates / `arms_audited 6/6`, zero failures |
| Gate self-test | G5 confirms the verifier fails on all five evidence defects it must catch |
| Provenance | `run_id.txt` per output directory; verifier fails on mismatch |
| Batch integrity | 518/518 jobs at full row count; binary fingerprint re-asserted unchanged after |
| Pre-registration | Form, thresholds, controls, estimator fixed before the v2 sweep; caught an n=1 false positive |

**Five defects found by the independent audit were all in validation code, none in the
simulator** — because the gates test the simulator on every run and, until the audit,
nothing tested the gates.

---

## 13. Limitations

### 13.1 Defects with known result impact

| # | Defect | Effect | Status |
|---|---|---|---|
| 1 | `hiddenChannelState` conflated with NLOS | Inflated R 2.6–2.7× on ~75% of links; was doing the work the confidence model was credited with | **FIXED** |
| 2 | The Markov fading state has **no physical effect** under the fidelity mode in use | Variance inflation keyed to a variable that attenuates nothing | **FIXED** — emitted as a diagnostic only |
| 3 | `MahalanobisFromPrior` omitted `R`; `RawMultilateration` unweighted | 84 of 89 integrity firings were artefacts | **FIXED** |
| 4 | Duty governor not consulted for the escalation handshake | `duty_868 = 0.02511` against a 0.025 cap — a regulatory violation | **FIXED** |
| 5 | Arbiter pre-empted the wrong transmission | Surrendered an uncapped slot to relieve a capped one | **FIXED** |
| 6 | `scratch/.gitignore` ignored the validation harness | Gates could appear to pass on a checkout that never contained them | **FIXED** |
| 7 | CRLB gate is a category error on tier 1 | Reports a ratio for an RSSI-inversion estimator a ToA bound does not describe | **OPEN** |
| 8 | `LoRaDistributionSampler` ignores its seed | None while `ResetNextStreamIndex()` holds | **DECISION NOT TO FIX** — fixing invalidates ~400 result files; every comparison is paired within a seed and the defect is common-mode |
| 9 | `--uwbTriad` also sets `dwellRadiusM` | Compound knob; degeneracy at 25 m, not 30 | **OPEN** — correct guard is `≤ 15` |
| 10 | `DISCOVERY` floods at hop_limit 3 | ~140k of 200k transmissions are relays | **OPEN** — design question |

### 13.2 `UNMEASURED`

1. **Intra-cycle handshake scheduling** (§8.4) — derived, not run. Highest value.
2. **S7 environmental fault injection** — does not exist. The integrity gate has never
   been shown a fault it was designed to catch.
3. **`--dwell` sweep at b3000** (§10.4) — tests the coverage-fraction hypothesis on the
   existing binary.
4. **Facility-count sweep** — the cleaner test of the same hypothesis; needs a
   `BuildDeployment()` change.
5. **Hysteresis / `nDeescalate`** — no CLI flag; requires code.
6. **Pareto at b3000/`gwGrid=9`** — the existing b3000 Pareto carries the density
   confound.
7. **ρ and per-term decomposition at b3000** — b1000 only.
8. **Cell E for v2 at b3000** — b1000 only.
9. **Mobility-state estimator** — claim 4's RECEDING branch never runs.
10. **`searchDuty` citation** — spans a factor of 828. The ranking is invariant to it;
    magnitudes are not.
11. **Probe figures are static-channel** — the probe runs a static node, so every
    r80/r50 was measured under a regime the scenario never operates in.
12. **Environment coverage** — three settings is a sensitivity check, not a
    generalisability proof.

---

## 14. Remediation queue

| # | Work | Rationale | Cost |
|---|---|---|---|
| 1 | **Intra-cycle handshake scheduling** | Derived mechanism predicting elimination of pre-emption at no accuracy cost; the only positive prediction outstanding | ~2 h |
| 2 | **`--dwell` sweep at b3000** | Tests §10's hypothesis on the existing binary; converts a fatal scope failure into a deployment requirement | ~1.5 h |
| 3 | **S7 fault injection + residual control** | The only route to the central novelty claim; must score against damage and report ROC | ~4 h |
| 4 | **Gate the integrity check to the seam** | Makes the code implement the claim; ships with item 3 | ~1 h |
| 5 | **Facility-count sweep** | Cleaner test of §10; needs `BuildDeployment()` change | ~2 h |
| 6 | **Expose `pcsThresholdDown` / `nDeescalate`** | Prerequisite for the thrash finding (§2.6) | ~1 h |
| 7 | **Pareto and ρ at b3000/gw9** | Removes the last density confound | ~2 h |
| 8 | **CRLB gate string + tier-1 exclusion; `--uwbTriad` decoupling; `--discoveryHopLimit`** | Small correctness items | ~1.5 h |

**Items 1–3 are the only ones that can change a verdict.** 5–8 close scope gaps.

---

## 15. Provenance

| | |
|---|---|
| Baseline | `results/baseline_v6` · `run_id` `bd99a6548154dc85` |
| Source | `0be15a19b` |
| Verifier | 136 checks / 155 gates / `arms_audited 6/6`, zero failures |
| Overnight batch | 518 jobs · `results/overnight_20260805_003718/` · binary fingerprint re-asserted unchanged · 518/518 at full row count |
| Determinism | Verified at defaults and six non-default flag settings |
| Full detail | `report/PCS_REDESIGN_RESULTS.md` Parts 0–8 |
| Audit | `CA_TBL_AUDIT_REPORT.md` |

---

## Appendix — corrections register

Every claim stated and later corrected, so the record is auditable rather than tidy.

| # | Claim as stated | Correction |
|---|---|---|
| 1 | v2 is progress on Claim 1 | **Wrong in direction.** Legacy fails by 2.62×, v2 by 3.18×. |
| 2 | `static_2p4` ratio of 1.90× | From an n=1 probe; 3.18× at 20 seeds, 0/20 won. |
| 3 | The integrity check is measurably harmful | **Scope error** — legacy form only; neutral under v2. |
| 4 | `duty_868 = 0.024691` attributed to cell B | Misattributed; that is pre-fix cell D. |
| 5 | `trig_f1 = 0` is undefined | Imprecise. Precision undefined; **recall a defined zero** over 3310 opportunities. |
| 6 | Two E runs produced empty summaries | Not a failure — the gate counted file existence, not rows. |
| 7 | "v2 barely fires" explains the gate's form-dependence | **Geometry-scoped**; at b3000 it fires 16× more and is still null. |
| 8 | Static provisioning dominates the design space | **b1000-scoped**; at b3000 it dominates while in 45.98% outage, and the axes exclude availability. |
| 9 | `dt ≥ 3.91 s` fixes handshake capacity | Average-rate calculation; the binding constraint is intra-cycle ordering. |
| 10 | Only a hybrid delivers both continuity and accuracy | **Geometry-scoped**; fails at all six densities at b3000. |
| 11 | `find -size +0` is a completeness gate | Insufficient for `--seedCount=N`; all gates now count rows. |
| 12 | The conjunction does not transfer to b3000 | **Framing wrong** — identical density, 8× median difference. Coverage fraction is the variable. |
| 13 | The facility sweep needs a code change | Partly wrong — `--dwell` varies the same variable on the current binary. |
| 14 | An earlier "the handshake does not fit by 7.0×, no scheduling change can help" | **Retracted twice** — first to an off-time framing, then to §8.2's intra-cycle finding, which is the measured one. |
