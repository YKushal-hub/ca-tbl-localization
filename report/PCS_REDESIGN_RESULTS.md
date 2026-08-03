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

```
sigma_ref = sqrt(traceRef) = sqrt(10.0) = 3.1623 m
sigma_post = sqrt(traceSigma)                     -- EKF posterior radial sigma
sigma_geo  = sqrt(rxx + ryy) from (J^T R^-1 J)^-1 -- this cycle's measurement-only
                                                     radial sigma
losFrac    = nLos / nTotal                        -- meaningful only after item 1

pcs_v2 = log10(sigma_ref / sigma_post)
       + log10(sigma_ref / sigma_geo)
       + losFrac
```

Both leading terms are log-ratios of a length to a length: dimensionless,
unbounded in both directions, and therefore retaining resolution across the
three orders of magnitude that separate UWB from tier 1 — which is exactly what
the saturating `traceRef/(traceRef+traceSigma)` form destroys.

`sigma_geo` is the exact `(J^T R^-1 J)^-1`, not the `gdop × sigma` approximation.
`RawMultilaterationWeighted()` already returns it, it costs no additional RNG
draw, and GDOP is defined for equal per-range sigma — the assumption tier 1
violates hardest. There is no reason to approximate a quantity that is already
computed exactly.

### 0.2 The escalation threshold — derived, not chosen

`traceRef = 10.0 m²` is documented in `CaTblPolicyConfig` as a **SPECIFICATION**:
the target radial position accuracy, `(3.162 m)²`. It is not a tuned constant and
predates this work.

The form above is constructed so that `log10(sigma_ref / sigma_post) = 0` exactly
when the posterior radial sigma equals that target. Taking the geometry term at
its own reference and the LOS term at its clean value, the escalation threshold
is therefore

> **`pcsThresholdUp = 0.0`, for every tier.**

Read plainly: *escalate while the fix is worse than the stated accuracy target,
stop when it is met or when tiers run out.* The threshold is not a free
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

One octave of position uncertainty. A round, principled choice, committed in
advance; not the best of several tried. The direction preserves the legacy
design's deliberate bias — the down threshold sits above the up threshold, so a
missed escalation costs more than a wasted one, which is the correct asymmetry
for search and rescue.

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

---

## Part 4 — PCS v2 results

*Pending. Pre-registration is Part 0 and was committed before execution.*
