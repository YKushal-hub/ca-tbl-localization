# PCS redesign and the NLOS flag chain — results

Work items 1 and 2 of the technical report's §11 remediation plan, addressing
§10.2 (`nNlos` does not count NLOS links) and §10.4 (PCS is very nearly a
constant).

Status: **item 1 complete and measured. Item 2 pre-registered, not yet run.**

Baseline commit for every "before" number: `7e23ad903`. Acceptance test for every
change: `results/baseline_v3/arm_summary.csv` byte-identical at defaults.

---

## Part 0 — Pre-registration for the PCS v2 evaluation

**Written and committed before the v2 sweep was executed.** This section exists
so that the thresholds cannot be chosen after seeing which choice wins. It is
dated by its commit, not by its content.

### 0.1 The form

**AMENDED before any v2 run — see §0.1.1 for the amendment and the measurement
that forced it. The form below is the amended one; the superseded three-term
version is shown in §0.1.1 so the change is auditable rather than silent.**

```
sigma_ref  = sqrt(traceRef) = sqrt(10.0) = 3.1623 m   -- the SPECIFICATION
sigma_post = sqrt(traceSigma)                          -- EKF posterior radial sigma
sigma_geo  = sqrt(rxx + ryy) from (J^T R^-1 J)^-1      -- this cycle's
                                                          measurement-only radial sigma

sigma_eff  = sqrt(sigma_post * sigma_geo)              -- geometric mean

pcs_v2     = log10(sigma_ref / sigma_eff)
```

Equivalently `0.5 * [log10(sigma_ref/sigma_post) + log10(sigma_ref/sigma_geo)]`:
a log-ratio of a length to a length. Dimensionless, unbounded in both directions,
retaining resolution across the three orders of magnitude that separate UWB from
tier 1 — which is exactly what the saturating `traceRef/(traceRef+traceSigma)`
form destroys.

Read plainly: **the score is how many decades of position uncertainty separate
the current fix from the stated accuracy target.**

`sigma_geo` is the exact `(J^T R^-1 J)^-1`, not the `gdop × sigma` approximation.
`RawMultilaterationWeighted()` already returns it, it costs no additional RNG
draw, and GDOP is defined for equal per-range sigma — the assumption tier 1
violates hardest. There is no reason to approximate a quantity already computed
exactly.

#### 0.1.1 Amendment: term 3 removed from the operative score

**Superseded form:**

```
pcs_v2_superseded = log10(sigma_ref/sigma_post) + log10(sigma_ref/sigma_geo) + losFrac
```

The original pre-registration retained `losFrac = nLos/nTotal` on the grounds
that it was "meaningful only after item 1". **Item 1 has now shown it is not
meaningful after item 1 either.** Measured under `--nlosSource=propagation`,
seed 7, fix cycles, from the pinned single-band arms:

| tier | `losFrac` mean | sd | distinct values observed |
|---|---|---|---|
| 1 — 868 MHz | 0.9303 | 0.1008 | 14 |
| 2 — 2.4 GHz | 0.9992 | **0.0097** | 6 |
| 3 — UWB | 1.0000 | **0.0000** | **1** |

Term 3 has not been revived by fixing defect A — it has been **inverted**. It was
pinned near 0; it is now pinned near 1. On tier 3 it is exactly constant across
every fix cycle in the run. On tier 2 its standard deviation is 0.0097 against
leading terms that span decades.

A near-constant term contributes an **offset**, and an offset compared against a
threshold is absorbed by the threshold. It carries no decision information. On
tier 1 it takes a handful of discrete values drawn from roughly
`Binomial(3.7, 0.07)/3.7`, which is the only place it varies at all, and even
there its spread is ~3% of the score's range. It is also dimensionally
incoherent with the other two — a bare fraction added to log-ratios.

**It is therefore removed from the operative sum and retained as a reported
diagnostic** (§0.4a). This is an amendment made before the sweep, with the
measurement stated, not a term quietly dropped after seeing a result.

#### 0.1.2 Why the leading factor changed from a sum to a mean

The superseded form summed two log-ratios; the amended form averages them
(`sqrt` of the product inside the log). **This changes no decision at threshold
0** — `0.5x > 0` and `x > 0` are the same condition — so it is not a tuning move.

It is a fidelity fix to the pre-registered hysteresis rule. The rule is "a factor
of 2 in sigma, 0.301 on a log10 scale". Under the summed form, a factor of 2 in
the underlying uncertainty moves the score by `2 × log10(2) = 0.602`, so the
constant 0.301 would have silently meant a factor of 1.41, not 2. Under the
averaged form one unit of score is exactly one decade of `sigma_eff`, so 0.301
means a factor of 2 as stated. The formula was corrected to match the
pre-registered intent rather than the intent quietly restated to match the
formula.

### 0.2 The escalation threshold — derived, not chosen

`traceRef = 10.0 m²` is documented in `CaTblPolicyConfig` as a **SPECIFICATION**:
the target radial position accuracy, `(3.162 m)²`. It is not a tuned constant and
predates this work.

The amended form is constructed so that `pcs_v2 = 0` exactly when
`sigma_eff = sigma_ref`, i.e. when the geometric mean of the posterior and
geometric position uncertainty equals that target. The escalation threshold is
therefore

> **`pcsThresholdUp = 0.0`, for every tier.**

Read plainly: *escalate while position uncertainty exceeds the stated accuracy
target, stop when it is met or when tiers run out.* The threshold is not a free
parameter — it is the specification restated on a log scale.

**One target for all three tiers**, not three. An accuracy requirement does not
change according to which radio is being used to meet it. This removes two free
parameters relative to the legacy `1.55 / 1.70 / 2.30`, which were carried
verbatim from the Python oracle, were never calibrated even for the score they
were written for, and must not be transplanted onto a differently-scaled score.

### 0.3 The de-escalation threshold — pre-registered

Hysteresis is a second free parameter and nothing derives it. It is fixed here,
before the run, at **a factor of 2 in sigma**:

> **`pcsThresholdDown = 0.301` (= log10 2), for every tier.**

One octave of position uncertainty: the node demotes only once `sigma_eff` is
better than **half** the target. A round, principled choice, committed in
advance; not the best of several tried. Under the amended form one score unit is
exactly one decade of `sigma_eff`, so this constant means a factor of 2 as
intended — see §0.1.2. The direction preserves the legacy design's deliberate
bias: the down threshold sits above the up threshold, so a missed escalation
costs more than a wasted one, which is the correct asymmetry for search and
rescue.

`nDeescalate = 5` is unchanged from the legacy configuration.

### 0.4 Acceptance criteria, and the one that can be gamed

Stated in advance so they cannot move afterwards:

1. `--pcsForm=legacy` reproduces `baseline_v3` byte-identically.
2. ρ(PCS, realised error) over 20 seeds, materially closer to −0.78 than to −0.09.
3. Escalation rate per fix cycle reported for both forms.
4. Arm comparison, including marginal m/J against `fixed_threshold`.

**Criterion 2 is necessary but not sufficient, and it is trivially gameable.**
`traceSigma` alone predicts realised error at ρ ≈ +0.78. Therefore *any* monotone
decreasing function of `traceSigma` alone scores ρ ≈ −0.78 by construction —
while being precisely what `fixed_threshold` already does, with extra arithmetic.
Passing criterion 2 on its own would demonstrate nothing except that the score
had been reduced to its own strongest input.

Two controls are therefore pre-registered alongside v2:

- **A monotone-of-`traceSigma` control**, `−log10(sigma_post)`, reported with the
  same ρ estimator on the same population. v2 must be judged against this, not
  against zero.
- **Incremental information**: whether v2 predicts realised error better than
  `traceSigma` alone does. If it does not, the honest conclusion is that the
  confidence model reduces to the baseline's statistic.

**Escalation rate is reported beside every ρ.** A score can improve its
correlation with error while still escalating on 96% of cycles, and if it does,
the energy conclusion does not move however good ρ looks. Neither number is
sufficient alone.

### 0.4a Per-term evaluation — pre-registered

Reporting only the correlation of the sum would hide a one-term score wearing
decoration: a sum can correlate well because a single component does all the
work, and that is exactly the failure the baseline comparison is meant to expose.
So the following are reported per term, over fix cycles, per tier and pooled:

1. **Variance contributed.** `Var(term_i)` and `Var(term_i)/Var(pcs)` for each
   term, plus the covariance cross-terms, so the decomposition closes.
2. **Range and distinct-value count**, which is what caught term 3: a term with
   one distinct value on a tier is a constant, whatever its mean.
3. **ρ of each term alone** against realised error, next to ρ of the sum. If a
   single term's ρ matches the sum's, the other terms are decoration.
4. **`losFrac` is reported under all of the above despite being removed from the
   operative score**, so its exclusion stays visible and reversible rather than
   becoming an undocumented assumption of the next session.

A v2 that beats legacy on ρ but whose variance is >90% attributable to
`sigma_post` alone will be reported as what it is: the `fixed_threshold`
statistic on a log scale.

### 0.5 The threshold-parity problem

`fixedVarThresholdM2 = 100.0` means the baseline escalates at sigma = 10 m. The
derived v2 threshold escalates at sigma = 3.162 m. These are **different
operating points on the accuracy/energy curve**, so any single-point comparison
is unfair in one direction or the other, and which direction depends only on
which threshold happens to sit nearer the knee.

The comparison is therefore run as a **sweep of both**: v2 across a range of
sigma targets, `fixed_threshold` across a range of variance thresholds, compared
as Pareto fronts in (median error, joules). The question "is the confidence model
better than a flat threshold" is answerable only as "does one front dominate the
other", never as a comparison of two points. Overlapping fronts would mean the
decision layer is equivalent to a flat threshold at matched operating points —
a cleaner and more publishable negative than a single-point loss.

### 0.6 Estimator and population, stated in advance

ρ is **Spearman rank correlation**, computed over **fix cycles only**
(`hadFix == 1`), pooled across the 20 seeds, reported per arm and per active
tier as well as pooled. Confidence intervals by seed-level bootstrap
(seeds are the independent unit; cycles within a seed are not).

This is stated because the report's existing ρ figures cannot be reproduced —
see §3.2 below.

---

## Part 1 — The NLOS flag chain (§10.2)

### 1.1 What was wrong

The operative NLOS tag was

```cpp
o.nlos = (r.nlosDb > 0.0) || (r.hiddenChannelState > 0);
```

`hiddenChannelState` is a four-state Markov **fading regime**, drawn as
`(uint32_t) Sample(Uniform(0.0, 3.99))`, so `> 0` selects three of four states.

**Six occurrences, not three.** Three are in `EnginePhyProvider::Evaluate()`
(the MAC/channel path) and three in `rangeTier` (the policy path). The
`Evaluate()` three write `CaTblLinkOutcome::nlos`, which **nothing in the
codebase reads** — it is write-only. Only the `rangeTier` three were live. All
six now route through one selector so the dead field cannot silently contradict
the live definition.

### 1.2 The finding that goes beyond the report's §10.2

**In this simulation the hidden channel state has no physical effect whatsoever.**

Every coupling from the Markov state into the channel is gated behind
`FIDELITY_SDR_GRADE_REALISM`:

| Coupling | Location |
|---|---|
| +6 dB partial obstruction, +18 dB deep NLOS | `lora-physics-engine.cc:352-355` |
| terrain reflection count | `lora-physics-engine.cc:406-411` |
| delay-spread expansion | `lora-physics-engine.cc:483` |

`LoRaPhysicsConfig::ForScenario()` sets `FIDELITY_ULTRA_FAST_STATISTICAL`
(`lora-physics-parameters.cc:625`) and `ca_tbl_causal_sim.cc` never overrides it.
The state therefore **attenuates nothing, delays nothing, and biases nothing** —
it is a free-running label — while driving a 3.0× measurement-variance inflation
on three quarters of all links.

This is a stronger statement than "category error": it is not merely that the
fading regime is the wrong proxy for obstruction, it is that in this
configuration it is a proxy for nothing at all.

**Trap for later readers:** `lora-physics-engine.cc:273` sets
`result.fidelitySchedulerState = FIDELITY_SDR_GRADE_REALISM` as an unrelated
telemetry constant. Anyone grepping the symbol will find it and wrongly conclude
the engine runs SDR-grade. It is not `m_config.fidelityMode` and gates nothing.

**This partly supersedes §10.3.** That section attributes the 6/18 dB penalties
being inert to the static-node probe and a low Markov transition probability. In
the causal simulation they are inert *unconditionally*, for a different reason.
§10.3's conclusion — that every r80 figure is a static-channel measurement —
survives on its own legs; its stated mechanism is incomplete.

### 1.3 Why `--nlosSource=severe` was not implemented

The kickoff brief offered `severe` = `nlosDb > 0 || hiddenChannelState >= 2` as a
possible defensible proxy, and invited its omission if not judged defensible.

**Omitted.** Given §1.2, keying measurement-variance inflation to the Markov
state would inflate noise on the basis of a variable that does not affect the
measurement — defect A at half strength with a better name. The population is
still emitted as `nHiddenChannel` so the judgement stays checkable rather than
merely asserted, and a verifier gate now fails if that column stops being
recorded.

### 1.4 What was implemented

- `--nlosSource=legacy|propagation`, **default `legacy`**.
- `propagation` uses `PhysicsResult::isNLOS` — the engine's own determination,
  equal to `nlosDb > 0.01`, set from `ComputeNLOSPenaltyDb()`. This is the same
  field the pre-flight probe already reports as `nlos_frac`, so probe and arms
  finally agree on what the word means.
- `legacy` is spelled out literally on `nlosDb > 0.0`, not on `isNLOS`, so
  byte-identity holds by construction rather than by an argument about how
  improbable a sub-0.01 dB obstruction is.
- New trace columns `nNlosPropagation` and `nHiddenChannel`, emitted
  unconditionally in both modes.
- `nlosSigmaInflation` re-keys automatically: the policy inflates on the
  operative flag, and the harness decides what that flag means.

**No mode consumes an RNG draw.** All modes read fields off an already-computed
`PhysicsResult`, so every source shares one channel realisation and differs only
in how the EKF weights it.

### 1.5 Acceptance

```
diff results/nlos_legacy/arm_summary.csv results/baseline_v3/arm_summary.csv
→ identical
```

Verifier, both modes: **101 checks passed, 126 gates evaluated, arms_audited
6/6** — up from the 76 checks / 94 gates recorded in report §12. Gate counts are
equal between the two modes by construction (§1.8).

### 1.6 Measured NLOS fractions — seed 7, `--duration=3600`

Per-tier attribution from the pinned single-band arms. Both runs share one
channel realisation, so the propagation population is identical between them for
these arms — which is itself a check that the flag change did not perturb the
channel.

| tier | fused links | propagation NLOS | hidden state ≠ 0 | legacy flag |
|---|---|---|---|---|
| 1 — 868 MHz | 8992 | **7.00%** | 74.90% | 76.66% |
| 2 — 2.4 GHz | 13515 | **0.07%** | 75.06% | 75.08% |
| 3 — UWB | 619 | **0.00%** | 77.38% | 77.38% |

The propagation rates follow directly from `NLOSParams::DefaultsFor()`:
`P_nlos = 1 − exp(−d/scale)`, with `scale = 8000 m` for the tier-1 wide area
(`RURAL_FORESTED`) and `54000 m` at the facilities (`RURAL_OPEN`). Tier 1 ranges
over hundreds to thousands of metres; tiers 2 and 3 over tens to hundreds.

**All-LOS cycle fraction — the integrity gate's eligible population:**

| arm | legacy | propagation |
|---|---|---|
| `static_868` | 0.07% | 62.94% |
| `static_2p4` | 0.00% | 99.31% |
| `static_uwb` | 0.63% | 100.00% |
| `ca_tbl_full` | 0.00% | 90.26% |
| `fixed_threshold` | 0.00% | 85.37% |

Two to three orders of magnitude. **No claim is made here about the integrity
check.** Its eligible population changing is not evidence that it works; that
requires S7 environmental fault injection, which is not this session's work.

### 1.7 The variance inflation actually applied

Mean ratio `meanSigmaM / meanSigmaRawM` over fix cycles — what
`nlosSigmaInflation = 3.0` actually multiplied the EKF's R by:

| arm | legacy | propagation |
|---|---|---|
| `static_868` | 2.669× | 1.284× |
| `static_2p4` | 2.636× | 1.005× |
| `static_uwb` | 2.659× | 1.000× |
| `fixed_threshold` | 2.630× | 1.115× |
| `ca_tbl_full` | 2.660× | 1.070× |

The defect in one line: **`static_uwb` had its ranging variance inflated 2.659×
on average by a fading-regime label, on a tier where the propagation layer
reports an obstruction on 0.00% of links.** A parameter documented as a targeted
NLOS correction was operating as a near-global 2.6–2.7× variance multiplier.

### 1.8 Verifier changes

Four new gates per arm, and the mode is read from `run_id.txt` provenance rather
than inferred from the data being checked:

- population counts never exceed the fused link count;
- the hidden-channel population is still recorded (**anti-regression**: the cheap
  wrong way to make these numbers look right is to stop counting the Markov
  regime rather than to stop keying R to it);
- under `legacy`, the operative flag is bounded by the union of the two
  populations, below by the larger and above by their sum;
- under `propagation`, the operative flag equals the propagation population
  exactly, **and** is distinguishable from the hidden-channel population — so a
  flag that was renamed rather than re-keyed fails.

Gate counts are equal in both modes (101/126) so that changing mode cannot
quietly reduce coverage.

No existing gate was deleted or weakened.

---

## Part 2 — What fixing defect A did to the results

**Single seed (7), `boxHalf=1000`. n = 1. Twenty-seed figures pending.**

| arm | rmseM (leg → prop) | cep50M | totalJ | escalations | suspects |
|---|---|---|---|---|---|
| `static_868` | 157.15 → 115.48 | 129.37 → 88.58 | 13.59 → 13.59 | 0 → 0 | 0 → 0 |
| `static_2p4` | 2.358 → 1.779 | 1.894 → 1.449 | 16.96 → 16.96 | 0 → 0 | 0 → 0 |
| `static_uwb` | 116.73 → 669.90 | 35.39 → 53.82 | 630.51 → 630.51 | 0 → 0 | 0 → 0 |
| `fixed_threshold` | 3.365 → 3.300 | 2.361 → 2.021 | 26.55 → 25.23 | 200 → 180 | 0 → 0 |
| `ca_tbl_no_integrity` | 4.563 → **97.04** | 2.817 → 71.58 | 511.17 → 14.31 | 193 → **12** | 0 → 0 |
| `ca_tbl_full` | 4.563 → 2.811 | 2.817 → 1.726 | 511.17 → 27.66 | 193 → 218 | 0 → **220** |

**Do not read the last row as a win.** Four things must be said about it:

1. **`ca_tbl_no_integrity` collapses.** Escalations fall 193 → 12 and RMSE rises
   to 97 m. Removing the bogus variance inflation *removes most of PCS's reason
   to escalate*: smaller R → smaller `traceSigma` → larger term 1 → PCS above
   threshold. The legacy "96% of fix cycles want to escalate" was substantially
   an artefact of defect A.

2. **`ca_tbl_full` only escalates because the integrity gate forces it.** In
   `Step()`, `suspect` sets `wantEscalate = true` unconditionally. With the
   all-LOS precondition now satisfiable, `suspect` fires 220 times and becomes
   the dominant escalation driver. The gate is functioning as an escalation
   trigger, not as an integrity check. That is not the claimed mechanism, and
   the difference between the two `ca_tbl_*` rows is measuring it.

3. **`ca_tbl_full` violates the duty-cycle cap.** `duty_868` = **0.02511**
   against the WPC G.S.R. 853(E) Table-II limit of 0.025. This is a regulatory
   violation, reported and not tuned away. It is a consequence of the arm
   spending more cycles on tier 1.

4. **`static_uwb`'s unconditional RMSE gets worse (116.7 → 669.9 m) while its
   ranging gets better** (sustained-coverage median 0.154 → 0.137 m). At 11% fix
   coverage that number is dead reckoning, exactly as report §4.1 and the
   verifier's own NOTE say. It must not be quoted as a localisation result in
   either direction.

**Net reading of Part 2:** fixing defect A did not repair the confidence model.
It exposed that PCS's escalation behaviour was being driven almost entirely by
variance inflation keyed to a variable with no physical effect. This makes the
§10.4 redesign more necessary, not less.

---

## Part 3 — Contradictions found against the technical report

The report states it is accurate as of `7e23ad903`.

### 3.-1 The architecture cannot afford to signal its own handoffs — NEW RESULT, AND THE STRONGEST ONE IN THE PROJECT

This began as a bug fix (§3.7) and is not a bug. It is a regulatory constraint
dictating an architectural parameter, and it is stated here as a finding in its
own right.

**The arithmetic.** Tier 1 is SF7, BW 125 kHz, CR 4/5. Semtech time-on-air:

| quantity | value | source |
|---|---|---|
| WPC G.S.R. 853(E) Table-II budget | 2.5% | cited regulation |
| budget per `dt` = 2.5 s cycle | **62.50 ms** | `0.025 × 2.5` |
| ToA, 20-byte ranging frame | **56.58 ms** | Semtech ToA, SF7/125 kHz/CR4/5 |
| measured tier-1 airtime per cycle | **56.58 ms** | `airtime_868 / steps`, `static_868`, seed 7 |
| remaining budget | **5.92 ms** | 62.50 − 56.58 |
| ToA, 12-byte handshake frame (TX alone) | **41.22 ms** | same formula, `AccountEscalationHandshake` default payload |

The measured per-cycle airtime equals the computed ToA of a single 20-byte frame
to five significant figures, which independently confirms that the tier-1 budget
is spent on exactly one ranging exchange per cycle and nothing else.

**The escalation handshake needs 41.22 ms and 5.92 ms remain. It does not fit,
by a factor of 7.0.** This is not an artefact of ordering within the cycle — no
scheduling change can fit a 41 ms frame into a 6 ms budget. The request/grant
exchange also charges a guarded RX window on top of the TX, so 7.0× is a lower
bound on the shortfall.

**Consequence, stated as an architectural requirement:**

> At `dt` = 2.5 s, a field node under Indian WPC Table-II cannot transmit the
> escalation request that a tier-1 → tier-2 handoff requires. Either the cycle
> period rises to **3.91 s** (`(56.58 + 41.22)/0.025`), a 1.56× slower fix
> cadence, or the tier-1 ranging payload shrinks, or handoffs departing tier 1
> must be rare by design.

§3.1 of the report already documents `dt = 2.5 s` as **REGULATORY**, derived as
"0.056 s tier-1 airtime / 0.025 duty cap = 2.24 s minimum". That derivation
budgets for **ranging only**. It omits the handoff signalling the architecture
is built around. The correct floor for a system that must also signal handoffs
from tier 1 is 3.91 s, not 2.24 s, and the configured 2.5 s sits between them —
which is precisely why the defect was invisible: the system was compliant for
ranging and silently non-compliant the moment it tried to hand off.

This is a chain from a legal constraint, through a protocol parameter, to
handoff latency, to metres of position error. It is stronger than anything the
confidence model has produced, and it did not require the confidence model to
work.

**Scope and honesty about what is not yet measured.** The position-error cost of
the induced latency is quantified in §2b from `baseline_v5`. The claim above is
about signalling capacity under one regulation at one cadence; it does not
generalise to bands without a duty-cycle cap (2.4 GHz and UWB have none in this
model, which is why escalations *between* those tiers are unaffected).

### 3.0 §6 and §8's hybrid-arm accuracy figures were produced by a bug — CONFIRMED, AND THIS IS THE LARGEST ITEM ON THE LIST

The bogus NLOS inflation was **load-bearing**, not incidental. The causal chain,
measured end to end:

```
hiddenChannelState > 0 on ~75% of links   (a fading regime with no physical effect)
  -> R inflated 2.6-2.7x on essentially every link
  -> traceSigma inflated
  -> PCS suppressed below its escalation threshold
  -> escalation fires on ~96% of fix cycles
  -> the node sits on tier 2/3
  -> good median accuracy
```

Remove the defect and the escalation signal disappears — because **there never
was one**. `ca_tbl_no_integrity` falls from 193 escalations to 12 and from
4.56 m to 97.04 m RMSE. The defect was doing the work the confidence model was
credited with.

Therefore: **the ~8 m median accuracy that report §8.1 and §8.2 attribute to
`ca_tbl_full` and to `fixed_threshold` was reached via a 2.6× variance error, not
via a working confidence model.** Every hybrid-arm accuracy figure in §6 and §8
is affected. This is a larger contradiction than the two hardcoded ρ values,
because those misreport a diagnostic while this one misattributes the central
result's mechanism.

**What this does to PCS's diagnosis.** §10.4 says PCS is uninformative — ρ =
−0.09. The sharper statement is that PCS is **miscalibrated in a measurable
direction, and the direction reverses with the defect**:

| condition | PCS behaviour | realised error |
|---|---|---|
| legacy (R inflated 2.6×) | pinned low, escalates on ~96% of cycles | good, ~4.6 m |
| propagation (R correct) | pinned high, escalates on 12 of 1434 | bad, ~97 m |

In the first it reports low confidence when error is low; in the second it
reports high confidence when error is 97 m. **Uninformative in both directions
under opposite conditions** — it does not track error in either regime, and the
appearance of competence in the legacy regime was an artefact of a compensating
error. That is a considerably harder finding to wave away than a weak
correlation coefficient, and it is what item 2 exists to address.

### 3.1 §10.2's mechanism is understated — CONFIRMED AND EXTENDED
Correct that the flag is a category error. Does not state that the hidden
channel state has **no physical effect at all** under the fidelity mode actually
in use. See §1.2.

### 3.2 §6.8's ρ figures are unbacked — CONFIRMED
`scripts/make_report_figures.py:501` and `:508` set the axis titles to the
literal strings `"PCS vs Error (ρ = −0.091)"` and
`"traceSigma vs Error (ρ = +0.779)"`. **No script in the repository computes a
rank correlation over trace data.** Both headline numbers of §6.8 — including
the "8.5× information loss" that motivates the entire PCS redesign — are
currently unreproducible from the repo.

The population is also unavailable: §6.8 cites 28,798 fix cycles (≈ 20 × 1440)
but no 20-seed trace set exists on disk. Only `results/causal` (seed 7) survives.

Recomputed on `results/causal`, seed 7, six arms pooled, 7338 fix cycles,
Spearman:

| signal | report §6.8 | recomputed (different population) |
|---|---|---|
| `traceSigma` | +0.779 | **+0.749** |
| `PCS` | −0.091 | **−0.547** |
| `gdop` | −0.155 | **+0.231** |

`traceSigma` is consistent. `PCS` and `gdop` are not. The populations differ
(seed 7 / `boxHalf` 1000 here versus an unrecoverable 20-seed set at
`boxHalf` 3000), so this is not yet proof of an error — but the report's numbers
cannot currently be checked, which is the finding.

### 3.3 §10.4's "structural ceiling 1.236" is not a ceiling — LIKELY MISLABEL
Measured maximum PCS while on tier 1 is **1.964**, above the 1.55 escalation
threshold. 1.236 is close to the tier-1 *mean* (1.252). A ceiling that is
routinely exceeded is not a ceiling. The §10.4 conclusion survives — 95.6% of
`static_868` fix cycles sit below threshold, against the report's 96.2% — but
the stated mechanism ("the score cannot reach the threshold") is wrong; the
correct one is "the distribution mostly sits below it".

### 3.4 §12's audit tally does not close — CONFIRMED
"42 REPRODUCED, 1 UNTRACED, 3 CONTRADICTED, 43 checks". 42 + 1 + 3 = **46**,
not 43. The arithmetic of the independent-audit summary is internally
inconsistent.

### 3.5 §10.3's mechanism is incomplete — CONFIRMED
See §1.2. The conclusion stands; the stated cause does not account for the
fidelity gating.

### 3.6 The report undercounts the defect's call sites — MINOR
§10.2 and the kickoff brief describe three occurrences. There are six; three
are write-only. Not result-affecting, but a reader auditing the fix would have
found three of six sites and believed the job done.

### 3.7 The duty-cycle governor was never consulted for the escalation handshake — NEW DEFECT, FIXED
`ca_tbl_causal_sim.cc` charged `AccountEscalationHandshake()` unconditionally.
`CaTblEnergyModel`'s governor is **advisory** — it exposes
`CanTransmit()`/`NoteDeferred()` and requires the caller to ask — and this call
site did not, nor did it call `NoteDeferred()` on the failing path. The
request/grant round-trip therefore burnt tier-1 airtime outside the regulatory
budget.

Surfaced as `duty_868 = 0.02511` against the WPC G.S.R. 853(E) Table-II cap of
0.025 under `--nlosSource=propagation` — about 0.4 s of excess airtime over
3600 s, a handful of SF7 frames, consistent with tier-1 escalations at the
observed rate. It was latent under legacy only because escalation happened to
land differently, not because the code was correct there.

This contradicts report §7.5, "Regulatory compliance: `duty_868` = 0.0226 against
the 0.025 Table-II cap; **the governor is enforced, not assumed**." For the
escalation handshake it was assumed, not enforced. The compliant number in §7.5
is real but was reached by luck of the operating point rather than by the
mechanism claimed.

Fixed: the handshake is now gated on `CanTransmit()` with `NoteDeferred()` on the
failing path. **This changes `arm_summary.csv` for every arm that escalates**, so
it establishes `results/baseline_v4` as a new reference (see §2a).

### 3.8 The validation harness was effectively untracked — NEW DEFECT, FIXED
`scratch/.gitignore` ignores `/*`, which matches `scratch/python_handoff/`. Git
does not descend into an ignored directory, so every new file there was silently
untracked — `verify_causal_sim.py` itself required `git add -f` during this
session, and any validation script added beside it would simply not have existed
in the repository.

A validation harness that can quietly fail to exist is a worse failure mode than
anything it checks for: the gates would appear to pass on a checkout that never
contained them. Fixed with a directory negation, plus two new verifier gates that
fail if anything under that directory is untracked or ignored — a gate on the
gates. Both were confirmed to bite by planting a stray file.

---

## Part 4 — PCS v2 results

*Pending. Pre-registration is Part 0 and was committed before execution.*

---

## Part 5 — The arbitrated baseline (`v6`)

Session of 2026-08-04. Appended, not rewritten: Parts 0–4 stand as committed.

**Baseline: `results/baseline_v6`**, promoted from `results/v6_ladder/D_prop_arb`,
`run_id` `bd99a6548154dc85`, built `Aug 4 2026 16:25:41`, verifier 136 checks /
155 gates / `arms_audited 6/6`, zero failures. Source at `0be15a19b`.

### 5.0 Lead with what was wrong

Two defects were found and fixed in this session, both in the arbitration
mechanism this baseline exists to measure. Neither was in the plan.

**Defect 1 — the mechanism was uncommitted.** `--arbitrateDutyBudget` defaulted
to `true` in the working tree but did not exist at `HEAD`. `run_id.txt` hashes
the build stamp and `argv` (`ca_tbl_causal_sim.cc:3061-3070`) and contains no git
SHA, so a baseline cut from working-tree-only source is one nobody can reproduce
— the exact failure the provenance discipline exists to prevent. Committed as
`5994de53c` before any baseline was cut.

**Defect 2 — the arbiter pre-empted the wrong transmission, and D failed the
regulatory gate.** The verifier rejected the first D:

```
X seed 7/ca_tbl_full: 868 duty cycle within WPC 2.5%: duty=2.5080% exceeds the
  Table-II field-node budget -- some transmitting call site is not consulting
  CanTransmit()
```

The arbiter surrendered `rangeTier(active)`. After the tier change commits,
`active` is the **new** tier, and tiers 2 and 3 carry no duty cap in this model,
so it was sacrificing an uncapped transmission to make room on the capped one.
Meanwhile `runFloor = hybridArm && (active != CA_TBL_TIER_868)`
(`ca_tbl_causal_sim.cc:2181`) kept ranging tier 1 on exactly those cycles, and
the handshake was charged on tier 1 on top of it.

Attribution, seed 7, first D — excess tier-1 airtime over `static_868` against
`handshakePreemptions × 41.22 ms`:

| arm | airtime_868 | excess | preemptions × 41.22 ms | ratio |
|---|---|---|---|---|
| `fixed_threshold` | 88.8883 s | 7.4189 s | 7.4196 s | **0.9999** |
| `ca_tbl_no_integrity` | 82.0052 s | 0.5358 s | 0.5359 s | **0.9999** |
| `ca_tbl_full` | 90.2897 s | 8.8202 s | 8.8211 s | **0.9999** |

`ca_tbl_full` spent more tier-1 airtime (90.29 s) across 362 tier-1 cycles than
`static_868` spends across 1440 (81.47 s). Fixed at `0be15a19b`: the surrendered
slot is now the one on `pendingHandshakeTier`, the capped tier the handshake
departs. Post-fix `duty_868` = **0.021700** (78.1210 s) against
`81.4694 − 218×0.056576 + 218×0.04122 = 78.1218 s` predicted from the mechanism —
**0.8 ms**. The preemption count moved 214 → 218 under the fix, so the agreement
is at the mechanism level, not a coincidence of one number.

**Availability did not regress.** The risk was that pre-empting the tier-1 floor
converts pre-emptions into no-fix cycles, trading away the continuity result of
§6.6. Measured: `fixCycles` 1436 / `noFixCycles` 4 in D, against the
non-arbitrating reference's 1433 / 7. The node keeps the active tier's ranging
and only loses a fix when that tier is blind on the same cycle. **That
conjunction is not exercised at this operating point and remains a live exposure
elsewhere** — it is not a proof that the trade cannot occur.

**Every other pre-fix cell was compliant by luck, and that is the second
occurrence of the pattern.** With the defect fully present, `fixed_threshold`
reached `duty_868` = **0.024932** under legacy + arbitration
(`results/causal/arm_summary.csv`) and **0.024691** under propagation +
arbitration — both passing, both within 0.4% of the 0.025 cap, and both by where
the operating point landed rather than by the mechanism claimed. Only
`ca_tbl_full` under propagation crossed the line, and it is the arm that escalates
most. §3.7 recorded the identical shape. Both times the violation was invisible
until an unrelated change moved the operating point, which is an argument for the
gate rather than for the margin.

### 5.1 The ladder

Seed 7, 3600 s, `--pcsForm=legacy`, everything else default. `A` and `C` predate
the arbiter fix; `A` was re-run on the post-fix binary and is **byte-identical on
`arm_summary.csv` and on all six traces**, so the fix is confined to the
pre-emption path, `C` follows by the same argument, and the ladder is
single-binary in effect.

| | A `legacy/noarb` | B `legacy/arb` | C `prop/noarb` | D `prop/arb` |
|---|---|---|---|---|
| `static_868` rmseM | 157.15 | 157.15 | 115.48 | 115.48 |
| `fixed_threshold` escalations | 0 | 200 | 0 | 180 |
| `ca_tbl_no_integrity` escalations | 0 | 195 | 0 | **16** |
| `ca_tbl_full` escalations | 0 | 195 | 0 | **220** |
| `ca_tbl_full` escDeferredAway | 1371 | 0 | 615 | 0 |
| `ca_tbl_full` rmseM | 157.01 | 4.82 | 153.36 | 3.02 |
| `ca_tbl_full` totalJ | 13.59 | 511.07 | 13.59 | 25.86 |
| `ca_tbl_full` duty_868 | 0.02263 | 0.02263 | 0.02263 | 0.02170 |

**Gate A passes.** `A` reproduces `baseline_v5` on all 62 columns v5 has. It is
*not* a whole-file diff and cannot be: `handshakePreemptions` is inserted at
column 30, so a v6-era file has 63 columns. The comparison is
`cut -d, -f1-29,31-63` on the new file against the unmodified v5 file.
`cut -f1-62` is the trap — it keeps the new column and drops `mac_deliveries`,
shifting every field from 30 onward and reporting a divergence that did not
happen.

**Arbitration alone (A→B) is what makes the architecture escalate.**
`escalationsDeferredAway` 1371 → 0. Nothing else does this: C has the NLOS fix
and no arbitration and still defers 615 away, leaving `ca_tbl_full` at 153 m.

**The NLOS fix alone (A→C) does not touch handoffs.** It improves the single-band
arms (`static_868` 157.15 → 115.48 m) and changes no escalation count from zero.

**They interact, and not additively.** `ca_tbl_no_integrity` escalates 195 times
in B but **16** in D. Removing the variance-inflation defect removes most of PCS's
reason to escalate, confirming §0.2's prediction on the arbitrated architecture
for the first time.

### 5.2 Does the architecture escalate at all now? — yes, but not because of PCS

`escalations` / `escalationsDeferredAway`, D:

| arm | escalations | escDeferredAway | preemptions |
|---|---|---|---|
| `static_868` / `static_2p4` / `static_uwb` | 0 | 0 | 0 |
| `fixed_threshold` | 180 | 0 | 180 |
| `ca_tbl_no_integrity` | 16 | 0 | 16 |
| `ca_tbl_full` | 220 | 0 | 218 |

The v5 null-architecture control is gone: no arm defers an escalation away, and
no hybrid arm is byte-identical to `static_868` any longer.

**But cell E shows the escalations are not PCS's.** A fifth cell was run — D with
`--suspectForcesEscalation=false`, the only configuration in which the integrity
gate is measurable as a gate rather than as an escalation trigger:

| `ca_tbl_full` | D (gate as trigger) | E (gate as gate) |
|---|---|---|
| escalations | 220 | **11** |
| suspects | 226 | 642 |
| rmseM | 3.0234 | **123.0032** |
| cep50M | 1.7272 | 81.8839 |
| totalJ | 25.8639 | 14.1980 |
| accuracyPerJoule | 0.012788 | 0.000573 |

`fixed_threshold` and `ca_tbl_no_integrity` are byte-identical across D and E —
neither reads `suspect` — which is the control confirming the flag does only what
it claims.

**Under a correct NLOS model the confidence model escalates 11 times in 1440
cycles.** All 209 of D's other escalations are `suspect` firing through
`suspectForcesEscalation = true`. D's headline `ca_tbl_full` number (3.02 m at
25.86 J) is therefore **not produced by the confidence model**; it is produced by
the integrity gate wired as a trigger. The default is left alone for v6 as
instructed, and this is the statement of what that default means.

Two further readings, both negative:

- **As a gate, the integrity check costs accuracy.** E (11 escalations, 123.00 m)
  against `ca_tbl_no_integrity` (16 escalations, 100.54 m). Letting the gate
  *reject* rather than *escalate* makes the arm worse on this seed. That sharpens
  §1.3 finding 4 from "no measured benefit" to "measurably adverse".

  > **SCOPE CORRECTION, added in Part 6 (§6.9).** The sentence above is true and
  > reproduced at 20 seeds, but it is a **legacy-PCS result and must be quoted with
  > the form attached.** Under `--pcsForm=v2` the same ablation is *not significant*
  > (rmseM −0.0959 m [−0.2878, +0.0961], n=20), because v2 fires `suspect` 23.2 times
  > per run against legacy's 220 — the gate has almost nothing to act on. The
  > defensible cross-form statement is: **the integrity check has shown no benefit
  > under any form tested; under legacy it is measurably adverse, under v2 it is
  > statistically neutral.** The original wording is retained unedited above so the
  > correction is auditable rather than silent.
- **`suspects` more than doubles when the gate stops forcing escalation**
  (226 → 642): the node stays on tier 1, where the all-LOS precondition is met
  more often, so the gate fires more and helps less.

### 5.3 Claim 1 — hybrid vs single-band. **FAILS**, 0/20 seeds, all three geometries

20 seeds, paired, geometric mean of per-seed ratios, `scripts/sweeps/paired_stats.py`.

`ca_tbl_full` vs `static_2p4`, accuracy-per-joule ratio:

| geometry | ratio [95% CI] | seeds won | verdict |
|---|---|---|---|
| `boxHalf=1000` | 0.3815 [0.3376, 0.4313] | 0/20 | fails by 2.6× |
| `boxHalf=3000`, `gwGrid=3` | 0.0118 [0.0081, 0.0174] | 0/20 | fails by 84.5× |
| `boxHalf=3000`, `gwGrid=9` (constant density) | 0.0207 [0.0135, 0.0317] | 0/20 | fails by 48.3× |

Read `b3000` only beside `gw9`. The gateway grid is fixed while the area is not
(`ca_tbl_causal_sim.cc:243`), so `b1000 → b3000` at the default `gwGrid=3` varies
density as `1/boxHalf²` — the confound that voided the `box_*` directories. The
constant-density control puts the failure at 48.3×, not 84.5×; roughly half the
apparent degradation with scale was falling gateway density.

**Median error and outage, as a pair** (§0.6 requires this; the metric is
substantially an outage measure for the escalating arms):

| geometry | arm | cep50M | rmseM | outage % |
|---|---|---|---|---|
| b1000 | `static_2p4` | 1.507 | 1.845 | **0.00%** |
| b1000 | `ca_tbl_full` | 1.794 | 3.086 | 0.16% |
| gw9 | `static_2p4` | 6.032 | 57.855 | **46.25%** |
| gw9 | `ca_tbl_full` | 26.116 | 57.159 | **1.54%** |
| gw9 | `static_868` | 63.205 | 83.427 | 0.01% |

At `b1000`, `static_2p4` dominates outright — better median, zero outage, less
energy. The accuracy-per-joule failure is real and not an artefact of the metric.

At `gw9` the picture inverts and **the §6.6 conjunction result survives on its own
legs**: `static_2p4` reaches 6.03 m median but is in outage 46.25% of the time,
while `ca_tbl_full` holds 1.54% outage at 26.12 m. Equal RMSE (57.9 vs 57.2 m)
conceals opposite failure modes. This remains an argument for multi-tier
*hardware*, which is prior art, not for this decision layer.

### 5.4 Claim 2 — handoff vs flat threshold. **FAILS** on the crediting-invariant measure

Accuracy-per-joule, `ca_tbl_full` vs `fixed_threshold`:

| geometry | ratio [95% CI] | seeds won |
|---|---|---|
| b1000 | 0.9878 [0.8725, 1.1185] | 11/20 |
| b3000 | 0.5955 [0.4637, 0.7648] | 6/20 |
| gw9 | 0.5003 [0.4328, 0.5783] | 0/20 |

At `b1000` the CI spans 1.0 — a statistical tie, not a win. At both `b3000`
geometries the flat threshold wins.

**Crediting-invariant marginal m/J above the shared tier-1 floor**
(`scripts/sweeps/marginal_mpj.py`, new this session), metres of RMSE removed per
joule spent above `static_868` at the same seed. Ratio `fixed_threshold` /
`ca_tbl_full`; `>1` means the flat threshold converts joules into accuracy more
efficiently:

| geometry | ratio [95% CI] | seeds where `ca_tbl_full` wins |
|---|---|---|
| b1000 | **1.3453** [1.2495, 1.4483] | 0/20 |
| b3000 | **1.5431** [1.0787, 2.2073] | 2/20 |
| gw9 | **2.5785** [1.9940, 3.3343] | 0/20 |

The flat threshold wins on all three, as §1.3 finding 3 predicted (last measured
1.514× [1.367, 1.661]). The ranking is unchanged by the arbitration fix. What
changed is that it is now a comparison between two arms that both actually hand
off — under v5 neither did.

**The `INTEGRITY ABLATION` line printed by `paired_stats.py` must not be read as
an integrity result.** It reports 18.10× at b1000, 1.42× at b3000, 0.117× at gw9
for `ca_tbl_full` vs `ca_tbl_no_integrity`. Per §5.2 that comparison is
220 escalations against 16 — it measures the escalation, not the check. The gate
is evaluated in cell E and only there. Note also that its verdict **reverses**
between `b3000` (1.42×, "PASSES") and the constant-density `gw9` (0.117×,
"FAILS"), so even as an escalation comparison it is density-dependent.

### 5.5 Claim 3 — do handoffs trigger correctly?

`trig_*` is scored against **`visUwb >= 3`** — geometric ground truth, "could UWB
have solved this cycle", **not** "was escalating the right call". It is a
coverage oracle, not a policy oracle, and a policy could be perfect by its own
lights and score badly here.

`ca_tbl_full`, seed 7:

| cell | tp | fp | fn | tn | precision | recall | f1 |
|---|---|---|---|---|---|---|---|
| B `legacy/arb` | 155 | 966 | 4 | 315 | 0.1383 | 0.9748 | 0.2422 |
| D `prop/arb` | 14 | 0 | 145 | 1281 | **1.0000** | **0.0881** | 0.1618 |
| E `prop/arb`, gate-as-gate | 0 | 0 | 159 | 1281 | 0.0000 | 0.0000 | 0.0000 |

The NLOS fix flips the operating point rather than improving it: from escalating
almost always (966 false positives, recall 0.97) to escalating almost never but
never wrongly (0 false positives, recall 0.088). `fixed_threshold` scores 0
throughout because it escalates only to tier 2 and the metric is scored on tier-3
escalation.

**ρ, seed 7, D, `pcsForm=legacy`** (`scripts/compute_rho.py`), against the
pre-registered monotone-of-`traceSigma` control:

| arm | ρ(PCS,err) | ρ(σ,err) | ρ(1/σ,err) |
|---|---|---|---|
| `ca_tbl_full` | −0.5275 | +0.5440 | **−0.5440** |
| `ca_tbl_no_integrity` | −0.2728 | +0.5838 | −0.5838 |
| `fixed_threshold` | −0.5649 | +0.6024 | −0.6024 |
| `static_2p4` | −0.1032 | +0.1062 | −0.1062 |
| `static_868` | −0.0572 | +0.1426 | −0.1426 |
| `static_uwb` | −0.3989 | +0.4218 | −0.4218 |

**Legacy PCS fails its own pre-registered control.** §0.4 states the test: v2 must
be judged against the monotone control, not against zero, because any monotone
function of `traceSigma` scores well by construction. Legacy PCS scores −0.5275
against a control of −0.5440 — it does not beat the statistic it is built from, it
slightly loses to it.

**Scope this correctly.** This kills *legacy* PCS, which §10.4 already documented
as defective. It is **not** evidence about PCS v2, which is the pre-registered
redesign of Part 0 and remains unrun. These runs are all `--pcsForm=legacy` by
design (kickoff rule 5), precisely so the arbitration result stays interpretable.

Also note ρ(σ,err) here is +0.54 to +0.60, not the +0.78 quoted in §0.5 — a
different population (seed 7, `boxHalf=1000`, arbitrated). §6.8's −0.091/+0.779
were hardcoded string literals at `make_report_figures.py:501,508` and are
superseded by this table, as instructed.

### 5.6 `UNMEASURED`

Exhaustive, for this session's scope.

1. **PCS v2 in every respect** — Part 4 remains pending. No v2 run exists. Every
   ρ, escalation rate and arm comparison above is `--pcsForm=legacy`.
2. **ρ over 20 seeds with seed-level bootstrap CIs**, per §0.6. Only seed 7 is
   measured. *Deliberate:* §0.6 fixes that estimator and population for the **v2
   evaluation**, where ρ is the deciding statistic. For a baseline cut, seed-7 ρ
   from D is adequate provenance. The traced 20-seed set belongs to the v2
   experiment and should be produced there.
3. **Pareto fronts across matched operating points** (§0.5). `fixedVarThresholdM2
   = 100` escalates at σ = 10 m; the derived v2 threshold at σ = 3.162 m. Every
   Claim-2 number above is a single-point comparison at unmatched operating
   points and is provisional in that specific way.
4. **Per-term variance decomposition of PCS** (§0.4a) — needs v2.
5. **Integrity gate under fault injection.** Cell E measures the gate as a gate
   for the first time, at **one seed, with no faults injected**. S7 environmental
   fault injection does not exist. The all-LOS eligible population moving is not
   evidence the gate works, and E is not a fault-injection result.
6. **Cell E at 20 seeds.** E is seed 7 only. The 123.00 m vs 100.54 m comparison
   against `ca_tbl_no_integrity` has n=1 and no CI.
7. **`--suspectForcesEscalation=false` in the sweeps.** All 60 sweep runs use the
   default `true`, so every 20-seed number in §5.3–5.4 is of the
   gate-as-trigger configuration.
8. **`LoRaDistributionSampler` seed handling** (§10.1) — untouched, per kickoff
   rule 1. Remediation item 8.
9. **Mobility-state classifier / the RECEDING handshake branch.** Nothing sets
   mobility state; claim 4 remains exercised on its stationary path only.
10. **Hop-provenance confidence.** `hopWeight = 0`, `hopCount` never set. There is
    no mesh. Nothing is claimed.
11. **`searchDuty` bracket.** `searchDuty=1.0` is 99.97% of `static_uwb`'s energy
    and has no citation; the bracket spans a factor of 828. Every `static_uwb`
    energy figure above inherits that.
12. **Pre-fix D is not on disk.** The 2.5080% violation of §5.0 was destroyed when
    the cell was re-run; it is reproducible from `5994de53c` plus the arithmetic
    shown, and the legacy+arbitration pre-fix configuration survives in
    `results/causal` and `results/det_a`. This is a provenance gap in the defect
    report, not in the baseline.
13. **Duration, node count, geometry.** One 3600 s run, one field node, three
    geometries. No sensitivity to `dt`, `uwbTriad`, `nDeescalate`, or
    `fixedVarThresholdM2`.

---

## Part 6 — PCS v2, and the close of the decision-layer question

Session of 2026-08-04, continuing Part 5. Appended; Parts 0–5 stand as committed,
with one scope correction annotated in place at §5.2 (see §6.9).

**This part closes the decision layer.** Claims 1 and 2 were closed negative in
Part 5. PCS v2 — the pre-registered redesign of Part 0, and the last remaining route
to a positive decision-layer result — is measured here at 20 seeds against two
baselines and two geometries, and it loses to both.

### 6.0 Lead with what is wrong

Three things, in order of how much they should change a reader's mind.

**1. A positive v2 signal appeared at n=1 and did not replicate.** It is treated in
§6.2 as a subsection of its own, because the pre-registration surviving that event is
the most citable thing in this phase.

**2. The pre-registered hysteresis measurably failed to do what it was chosen for.**
`pcsThresholdDown = 0.301` was fixed in advance at one octave of `sigma_eff` (§0.3).
At `boxHalf=3000` it leaves `ca_tbl_full` changing tier on **29.1% of cycles**
(210.05 escalations against 208.60 de-escalations in 1440 steps), which is sustained
thrash, not convergence — and *worse* than the flat threshold it was meant to improve
on (24.1%). It is not retuned. What hysteresis would damp this is `UNMEASURED`.

**3. The integrity gate's effect is form-dependent, and Part 5 overstated its
scope.** See §6.9.

### 6.1 Verdict on v2, in the form Part 0 pre-committed to

> **PCS v2 fails.** It does not beat the flat variance threshold at matched seeds
> (0.8131 [0.6399, 1.0332] at b1000, point estimate against it; 0.2478 [0.1902,
> 0.3229] at b3000), and it loses to static 2.4 GHz provisioning on every seed at
> both geometries (0.3140 and 0.0049). At b1000 it reduces to a static tier-2
> assignment with a startup transient — 97.20% residency on one tier, 0.07% on the
> tier-1 floor it pays for. At b3000 it does not reduce to a static assignment; it
> thrashes, and 96.5% of its energy is UWB blind search.

`[PENDING: whether Part 0's specific pre-committed verdict — "a v2 that beats legacy
on ρ but whose variance is >90% attributable to sigma_post alone is the
fixed_threshold statistic on a log scale" — applies. Requires the §0.4a variance
decomposition, still running.]`

### 6.2 The n=1 signal that did not replicate

A single-seed probe (seed 7) showed `ca_tbl_full` under v2 beating `fixed_threshold`
on accuracy-per-joule by **1.33×** — the first time in this project that any
configuration of the decision layer had beaten the flat threshold on that metric.

It did not survive. At 20 seeds the geometric-mean ratio is **0.8131 [0.6399,
1.0332]**, 10/20 seeds won — a tie whose point estimate sits *below* 1.

Why the probe was misleading, stated precisely:

- Per-seed ratios span **0.2374 to 1.3968**, median **1.0019**.
- Seed 7's 1.3288 ranks **2 of 20** — the second most favourable seed in the set.
- The sweep's seed-7 rows are **byte-identical** to the standalone probe. The probe
  was not wrong. It was unrepresentative, which is a different failure and the one
  that single-seed results are specifically vulnerable to.

This is the pre-registration doing the exact job it was written for. Part 0 fixed the
form, both thresholds, the acceptance criteria, the controls and the estimator before
any v2 run, at a time when nobody knew which way the result would fall. When a
tempting number appeared, there was nothing left to adjust that would not have been
visible as an adjustment. **That is the result of this phase most worth writing
down.**

### 6.3 The two decisive comparisons

20 seeds, paired per seed, geometric mean of per-seed ratios
(`scripts/sweeps/paired_stats.py` estimator).

**v2 vs `fixed_threshold`** — the *literature* baseline:

| geometry | acc/J ratio [95% CI] | seeds won | reading |
|---|---|---|---|
| b1000 | 0.8131 [0.6399, 1.0332] | 10/20 | tie; point estimate against v2 |
| b3000 | 0.2478 [0.1902, 0.3229] | 0/20 | v2 loses by 4.0× |

**v2 vs `static_2p4`** — the *dominance* baseline:

| geometry | acc/J ratio [95% CI] | seeds won |
|---|---|---|
| b1000 | **0.3140 [0.2481, 0.3975]** | **0/20** |
| b3000 | **0.0049 [0.0032, 0.0075]** | **0/20** |

At b1000, v2 means: rmseM 2.2548 [2.0618, 2.4478], totalJ 47.1198 [38.0170, 56.2226]
against `static_2p4`'s 1.8451 and 16.6773 — **worse on both axes**. The reading:

> v2 discovers `static_2p4` and charges ~30 J for the discovery. It pays for a
> tier-1 floor it uses on 0.07% of fix cycles.

**Correction to a claim made mid-phase.** It was suggested that v2 represents
progress on Claim 1 relative to legacy. It does not. At b1000, legacy fails Claim 1
by 2.62× (ratio 0.3815 [0.3376, 0.4313]) and **v2 fails by 3.18×** (0.3140). v2
moves Claim 1 *further from* passing, because it spends 47.1 J against legacy's
27.0 J for a modest accuracy gain (2.25 m against 3.09 m). A direction was asserted
before it was checked; the check reverses it.

### 6.4 Diagnostics: what v2 is actually doing, and it differs by geometry

Fix-cycle tier residency, mean over 20 seeds:

| arm | 868 MHz | 2.4 GHz | UWB | escalations | de-escalations | first esc. |
|---|---|---|---|---|---|---|
| **b1000** `ca_tbl_full` v2 | **0.07%** | **97.20%** | 2.73% | 8.85 | 7.85 | cycle 1.4 |
| **b1000** `fixed_threshold` | 37.35% | 62.65% | 0.00% | 180.25 | 180.00 | cycle 1.4 |
| **b3000** `ca_tbl_full` v2 | 6.25% | 42.16% | **51.60%** | 210.05 | 208.60 | cycle 3.2 |
| **b3000** `fixed_threshold` | 17.62% | 68.10% | 14.29% | 174.20 | 173.20 | cycle 3.2 |

**At b1000 v2 is a static assignment with a startup transient.** It leaves tier 1 at
cycle ~1.4 and spends 97.20% of fix cycles on tier 2; its 8.85 escalations are one
promotion off tier 1 plus ~7.85 brief UWB excursions, matched by 7.85 demotions.
`pcsThresholdUp = 0.0` means "escalate while `sigma_eff` exceeds 3.1623 m"; tier 1
never meets that target and tier 2 usually does, so the threshold restates the
specification and the score does little per-cycle work. This is the structural
analogue of §10.4's finding about legacy — "escalate unless already on UWB" — with
the tier changed.

**At b3000 that reading fails.** Residency is 6.25 / 42.16 / 51.60 and the arm
thrashes. "v2 reduces to a static assignment" is a **b1000-only** statement and must
not be quoted unconditionally.

v2 does demote, near-symmetrically, at both geometries. It is not a one-way ratchet.

### 6.5 Where v2's energy goes at b3000

Mean joules, n=20, the same breakdown §6.2 of the technical report uses:

| arm | totalJ | j_uwb | **rx_uwb** | blindCycles |
|---|---|---|---|---|
| `ca_tbl_full` v2 | 456.208 | 440.399 (96.5%) | **440.354 (96.5%)** | 737.2 |
| `static_uwb` | 824.322 | 824.293 (100.0%) | 824.247 (100.0%) | 1328.3 |
| `fixed_threshold` | 165.156 | 148.265 | 148.255 (89.8%) | 268.1 |
| `static_2p4` | 6.563 | 0.008 | 0.000 | 27.0 |

**96.5% of v2's b3000 energy is `rx_uwb`** — a radio listening and hearing nothing
across 737 blind cycles. This is `static_uwb`'s failure mode differing only in
degree. One sentence covers both geometries:

> Where UWB is rarely available (b1000), v2 parks on tier 2 and becomes `static_2p4`
> with a surcharge. Where UWB is sporadically available (b3000), v2 thrashes into it
> and pays blind-search energy. In neither case does the score price the cost of
> being wrong.

**`searchDuty` caveat, restated because it bites hardest here.** `searchDuty=1.0` has
no citation and the bracket spans a factor of 828, so the *absolute* 440 J carries
that uncertainty. The **share** (96.5% in one state) and the paired comparison
against `static_2p4` do not — `static_2p4` barely touches UWB at all.

### 6.6 Cell E for v2 — the integrity gate with the gate acting as a gate

`--suspectForcesEscalation=false`, 20 seeds, b1000,
`results/sweep20_v7_v2_E_b1000/`.

**Control:** `fixed_threshold` and `ca_tbl_no_integrity` are **byte-identical** to
the corresponding `sweep20_v7_v2_b1000` rows on all 20 seeds, every column. Neither
reads `suspect`; the flag does only what it claims.

| `ca_tbl_full` v2, b1000 | gate as trigger | gate as gate |
|---|---|---|
| escalations | 8.85 | 8.90 |
| rmseM | 2.2548 | 2.2811 |
| totalJ | 47.12 | 47.49 |

Integrity ablation under v2, paired: escalations **+2.20 [+0.65, +3.75]**
(significant), rmseM **−0.0959 [−0.2878, +0.0961]** (**not significant**), totalJ
+1.50 [−1.93, +4.93] (not significant).

Under v2 the gate is **neutral**. The mechanism is that it barely fires: 23.20
suspects per run against legacy's 220.50, so `suspectForcesEscalation` has almost
nothing to act on.

### 6.7 Carried forward from Phase A

**Cell E at 20 seeds (legacy form)** — `results/sweep20_v6_E_b1000/`. The D≡E control
holds: all five arms that never read `suspect` are identical across D and E on all 20
seeds. `ca_tbl_full`: escalations 218.35 [216.81, 219.89] → **13.15 [11.28, 15.02]**;
rmseM 3.086 → **125.58 [120.18, 130.98]**. Paired differences all significant
(escalations −205.20, rmseM +122.49). Part 5's single-seed figures (11 escalations,
123.00 m) fall inside these CIs — seed 7 was representative there.

Integrity ablation under legacy, E: escalations −0.20 [−2.14, +1.74] (**null**),
rmseM **+24.64 [+18.91, +30.37]** (**adverse**).

**`ca_tbl_full` − `static_868`, paired, cell E legacy:**

| | mean [95% CI] | verdict | seeds worse |
|---|---|---|---|
| rmseM | +7.1511 [**−0.0185**, +14.3208] | CI spans zero | 14/20 |
| cep50M | −6.4115 [−13.0273, +0.2044] | CI spans zero | 6/20 |
| totalJ | +0.9010 [+0.7191, +1.0830] | **clear of zero** | **20/20** |

The rmseM lower bound is **−0.0185** and is quoted unrounded deliberately: it fails
to clear zero by 1.85 cm. The honest statement is that with its central mechanism
operating as designed, the proposed system is **statistically indistinguishable from
the single-band tier-1 baseline on accuracy, in either direction, while costing more
energy on every seed tested**. The RMSE-worse / median-better split is §0.6's
bimodality caveat behaving exactly as pre-registered.

**`trig_f1 = 0.0000` in cell E is not a failed trigger score, and not simply
"undefined".** Pooled over 20 seeds `ca_tbl_full` records tp=0, fp=0, **fn=3310**,
tn=25490. A positive class exists — 3310 cycles (165.5/seed) where `visUwb >= 3`.
What is absent is positive *predictions*. So **precision is undefined (0/0)**,
**recall is a defined, measured zero** (0/3310), and f1 is undefined via precision
and reported as 0.0000 by convention. The arm escalated to UWB on none of 3310
opportunities. That is a measurement, not a scoring artefact.

**Pre-fix D restored** — `results/v6_ladder/D_prefix_VOID/`, regenerated from
`5994de53c` after the original was destroyed by re-running in place. Every Part 5
figure reconciles: `ca_tbl_full duty_868` = **2.5080%** against the 2.5% cap, and all
three excess-airtime attribution ratios at **0.9999**. It also reproduces the
destroyed original's rmseM 3.6544 / totalJ 28.7306 / 215 escalations. The directory
carries a `README.md` marking it as evidence of a defect, never a measurement.

**`--arms` equivalence check.** `--arms=ca_tbl_full` alone (arm 6 of 6, maximum
preceding RNG stream state) and `--arms=fixed_threshold` alone (arm 4 of 6) are both
**byte-identical to `baseline_v6` on the summary row and on the full 1441-line
trace**. `RunArm()`'s per-arm stream reset behaves as `ca_tbl_causal_sim.cc:2980`
documents. This is what made the Pareto sweep affordable (~1.3 h against ~7 h).

**`sigma_geo` availability**, fix cycles only, seed 7, per active tier: 2.4 GHz
**0.00%** unavailable, UWB **0.00%**, 868 MHz **0.37%** (policy arms pooled).
`ca-tbl-policy.cc:639` substitutes `sigmaGeo = 1e6` on a failed weighted solve, which
is an **escalate-always substitution, not a dropped sample** — an unavailable
`sigma_geo` forces a strongly negative score. It is essentially never taken, so the
per-term decomposition is not running on a thin population. Failures are confined to
tier 1, where geometry is weakest, consistent with degenerate solves rather than a
defect.

### 6.8 The §0.4 criteria

`[PENDING: ρ table — criterion 2, both pre-registered controls (monotone
−log10(sigma_post), and incremental information over traceSigma alone), and the
§0.4a per-term decomposition with variance shares, covariance cross-terms, range,
distinct-value counts, per-term ρ, and losFrac reported despite exclusion.
analyse_pcs.py is running; n_boot = 2000 at ~1.5–3 h.]`

**Criterion 1 — `--pcsForm=legacy` still reproduces the legacy baseline
byte-identically: PASSES.** The rebuilt binary reproduces `results/baseline_v6` on
`arm_summary.csv` and all six traces. Additionally, `fixed_threshold` is identical
between the legacy and v2 sweeps at every seed — it does not read PCS, so the form
switch touching it would have indicated contamination.

**Criterion 3 — escalation rate per fix cycle, both forms**, `ca_tbl_full`, b1000:

| form | escalations / fix cycle |
|---|---|
| legacy | 0.1532 (218.35 / 1425) |
| **v2** | **0.0062** (8.85 / 1428) |

**Criterion 4 — arm comparison including marginal m/J against `fixed_threshold`:**
§6.3, plus `scripts/sweeps/marginal_mpj.py` for the crediting-invariant measure.

**An inverted failure mode Part 0 did not anticipate, named here as an extension of
its logic.** §0.4 warns that a score can improve ρ while still escalating on 96% of
cycles, leaving the energy conclusion unmoved. **v2 inverts this.** At an escalation
rate of 0.0062 per fix cycle, the score can correlate arbitrarily well with realised
error while almost never acting on it. ρ measures the score's *information*;
escalation rate measures whether the policy *uses* it. **v2 can pass criterion 2 and
still be a non-policy.** Both numbers must be reported together, always. This is an
extension of the pre-registration's reasoning, not a departure from it, and it is
recorded as such rather than folded in silently.

### 6.9 Correction: the integrity gate's effect is form-dependent

Part 5 §5.2 stated that "as a gate, the integrity check costs accuracy", sharpening
§1.3 finding 4 to "measurably adverse". **That is a legacy-PCS result and Part 5
stated it without the form attached.** The original wording is retained there with a
scope-correction note appended, so the correction is auditable rather than silent.

| PCS form | suspect firings/run | integrity ablation on rmseM (n=20) |
|---|---|---|
| legacy | 220.5 | **+24.64 m [+18.91, +30.37]** — adverse, CI clear of zero |
| v2 | 23.2 | −0.0959 m [−0.2878, +0.0961] — **not significant** |

The defensible cross-form statement:

> The integrity check has shown **no benefit under any form tested**. Under legacy
> PCS it is measurably adverse (+24.6 m RMSE); under v2 it is statistically neutral
> and fires on 1.6% of cycles. A mechanism that is either harmful or inert is not a
> contribution — but the two readings have different causes and must be reported
> with the form attached.

### 6.10 Pareto fronts at matched operating points

`[PENDING: §0.5 fronts and which of the three readings applies — one front dominates,
the other dominates, or the fronts overlap ("equivalent to a flat threshold at
matched operating points"). 18 points x 20 seeds, in flight.]`

Two notes recorded now so a later reader does not mistake either for a bug:

- `pareto_sweep.py`'s `collect()` reads **`ca_tbl_no_integrity`** for the v2 family,
  not `ca_tbl_full`, deliberately — "so the integrity gate's escalation side effect
  is not confounded into the comparison". That choice predates this phase and is
  correct. `--arms` was matched to it: `fixed_threshold` for `ft_*` points,
  `ca_tbl_no_integrity` for `v2_*` points.
- `static_2p4` is to be plotted as a **single fixed point**, not a swept arm. It has
  no operating-point parameter to sweep.

### 6.11 Corrections register

| # | Claim | Correction |
|---|---|---|
| 1 | "v2 is still progress on Claim 1" | **Wrong in direction.** Legacy fails by 2.62×, v2 by 3.18×. v2 moves Claim 1 further from passing. |
| 2 | `static_2p4` ratio of 1.90× | **Computed from the seed-7 probe.** At 20 seeds it is 3.18× [0.2481, 0.3975], 0/20 seeds won. |
| 3 | "the integrity check is measurably harmful" | **Scope error.** Legacy-form only; under v2 it is statistically neutral (§6.9). |
| 4 | Part 5 §5.0, `duty_868` = 0.024691 attributed to cell B | **Misattributed.** 0.024691 is pre-fix cell D; cell B was 0.024932. Corrected in Part 5 before commit. |
| 5 | `trig_f1 = 0` is "undefined, no positive class" | **Imprecise.** A positive class of 3310 pooled cycles exists; precision is undefined, recall is a defined zero (§6.7). |
| 6 | Phase A: "two E runs produced empty summaries" | **Not a failure.** The sweep was still running; the gate counted file existence, not content. Both sweep scripts now gate on `find -size +0`. |

### 6.12 `UNMEASURED`

Carried forward from Part 5 and extended.

1. **ρ and the §0.4a decomposition** — `[PENDING]`, see §6.8.
2. **Pareto fronts** — `[PENDING]`, see §6.10.
3. **v2 at the gw9 constant-density geometry.** v2 was run at b1000 and b3000 only.
   Part 5 showed the b3000 comparison is confounded by falling gateway density; the
   v2 b3000 numbers inherit that confound and have **no constant-density control**.
4. **Cell E for v2 at b3000.** Run at b1000 only.
5. **Hysteresis sensitivity.** `pcsThresholdDown = 0.301` measurably fails to damp
   thrash at b3000 (29.1% of cycles change tier). What value would damp it is
   unmeasured and deliberately not tuned.
6. **Integrity gate under fault injection.** S7 does not exist. Cells E measure the
   gate as a gate for the first time, with **no faults injected**.
7. **`suspectForcesEscalation=false` in the Pareto sweep** — all Pareto points use
   the default `true`.
8. **`LoRaDistributionSampler` seed handling** (§10.1) — untouched, remediation
   item 8.
9. **Mobility-state classifier / the RECEDING handshake branch** — nothing sets
   mobility state (`ca_tbl_causal_sim.cc:2513-2517`); claim 4 is exercised on its
   stationary path only.
10. **Hop-provenance confidence.** `hopWeight = 0`; `hopCount` is passed as a literal
    `0` and that is **correct**, not a stub — the neighbour table is one-hop by
    construction (`ca-tbl-mac.cc:353`). See `CA_TBL_PHASE8_SCOPE.md` §2.
11. **`ESC_REQUEST`/`ESC_GRANT`/`QUERY`/`RESPONSE`/`ENTRY_PUSH` on the wire.**
    Defined, sized, given access categories and ACK policies — never constructed or
    transmitted. The escalation handshake is *billed*, not sent.
12. **Multi-target arbitration.** One tag; `U(r)` scored offline only.
13. **`searchDuty` bracket.** No citation; spans a factor of 828. Every `static_uwb`
    and `rx_uwb` absolute figure inherits it.
14. **Duration, node count, geometry.** One 3600 s run, one field node. No
    sensitivity to `dt`, `uwbTriad`, `nDeescalate`, or `fixedVarThresholdM2` outside
    the Pareto grid.
