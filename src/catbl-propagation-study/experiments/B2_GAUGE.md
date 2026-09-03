# B2 — the gauge decision, and the degeneracy pre-flight that changed the build order

Written **before implementing B1 or B2**, as the work order requires, and committed before any
calibration cell runs.

## 1. The gauge decision: partial survey

Anchor-free ranging determines positions only up to rotation, translation and reflection. Three
options were on the table; the choice is **partial survey**, for the reasons the work order
states: perimeter anchors sit where GNSS still works, interior ones are under canopy or in
rubble, the covariance comes out in an absolute frame the tag's fix can actually consume, and
the number of surveyed references becomes a swept parameter that is itself a deployment result.

**That decision stands. It is also not the binding constraint, and the pre-flight is how that
was found.**

## 2. The degeneracy pre-flight — mechanism named, before spending a seed count

The deployment is not one anchor graph. It is **four isolated clusters per tier**, measured from
the deployment builder (`ca_tbl_causal_sim.cc:292-319`), not assumed:

| | tier 2 (2.4 GHz) | tier 3 (UWB) |
|---|---|---|
| anchors per site | 3 | 5 |
| ring radius | 150 m | 10 m |
| intra-site anchor–anchor range | 259.8 m (×3) | 11.76 m (×5), 19.02 m (×5) |
| **r80, computed from the model** | **1580.6 m** | **25.2 m** |
| intra-site links inside r80 | **3 of 3** | **10 of 10** |
| modelled PDR on those links | 0.9974 | 0.9774 / 0.9021 |
| inter-site separation | 1880 m / 2659 m | same |
| cross-site links inside r80 | **0** | **0** |

So anchor–anchor ranging can only ever connect anchors **within one facility site**. The
calibration problem is four independent small problems per tier, not one network.

**Two corrections, in opposite directions. The second was found by grepping for every r80 in the
tree rather than trusting the first.**

*First correction.* The r80 figures were initially read from the constant's own comment —
`uwbTriadRadiusM = 10.0; // UWB r80 = 10 m` (`ca_tbl_causal_sim.cc:3251`) — against which none of
tier 3's ten intra-site links is in range. Computed from the analytic model, UWB r80 is 25.2 m,
under which **all ten** are in range. That looked like a stale comment corrected.

*Second correction, which reverses it.* The tree carries **four** UWB r80 figures, and the 10 m
comment is not stale — it is a rounded restatement of an engine measurement:

| source | figure | what it is |
|---|---|---|
| `ca_tbl_causal_sim.cc:220` | **12.9 m** | the 64-site decorrelated probe, 2000 trials/point — **the authoritative engine measurement** |
| `ca_tbl_causal_sim.cc:3278` | ~13.5 m | a looser restatement of the same |
| `ca_tbl_causal_sim.cc:3251` | 10 m | the siting constant, rounded down from it |
| `ca_tbl_causal_sim.cc:1187` | 10 → 5 m | the *unstable* 400-trial estimate, explicitly superseded at `:1191` |
| `linkbudget.py`, this session | **25.2 m** | the analytic model |

**The model and the engine disagree by roughly 2× on UWB r80**, and that disagreement decides the
answer:

| criterion | tier-3 links retained | solvable at 2 surveyed? |
|---|---|---|
| model, 25.2 m | 10 of 10 | yes, redundancy **+3**, 5 m fault leaves a 3.34 m residual |
| **engine, 12.9 m** | **5 of 10** (adjacent only) | **SINGULAR — rank 4 of 6** |

Under the engine's own number only the five *adjacent* links survive, leaving a 5-cycle, and **a
cycle is not rigid in 2-D.** Tier 3 goes from "redundancy to spare" to "no solution at all"
depending on which r80 is used.

**So §5's caveat, as first written, was wrong to call the exposure bounded.** A ≈3.7 dB margin
residual sounds small; expressed as range it is a factor of two, and it flips the conclusion
rather than perturbing it.

The fair reading is not that the across-ring links are absent. At 19.02 m they sit between the
engine's r80 (12.9 m) and its r50 (29.5 m, same probe, `:220`), so their PDR is roughly 0.6–0.7
**by interpolation between two measured points — an interpolation, not a measurement.** They are
*lossy*, not missing. Calibration averages over many epochs, so a lossy link degrades the solve
far less than an absent one. But the topology is then **intermittent**, the redundancy is
**stochastic**, and on any epoch where the across-ring ranges happen to drop together the cluster
is momentarily singular. A solve that assumes a fixed graph is the wrong solve.

Robustness of the *model's* answer to its own residual, swept as a margin offset both ways —
retained because it bounds the model's internal sensitivity, not the model-versus-engine gap:

| link | −4 dB | 0 dB | +4 dB |
|---|---|---|---|
| UWB adjacent, 11.76 m | 0.9075 | 0.9774 | 0.9958 |
| UWB across, 19.02 m | **0.7112** | 0.9021 | 0.9758 |
| 2.4 GHz, 3/site, 259.8 m | 0.9965 | 0.9974 | 0.9985 |
| 2.4 GHz, 5/site, 176.3 m | 0.9987 | 0.9995 | 0.9999 |

The across-ring link was already the fragile one under the model's own uncertainty, and it is the
one the engine puts out of r80. The two disagreements point at the same link.

**Tier 2 is unaffected by any of this.** Its intra-site chord is 259.8 m against a measured
tier-2 r80 of ≥ 900 m (censored lower bound, `:217-218`) — an order of magnitude of headroom, so
no plausible model-engine gap touches it. Tier 2's problem is redundancy, and redundancy alone.

### The result that changes the build order

All anchors of a tier share one height, so each cluster is a 2-D problem. Unknowns are
`2(n − s)`; the gauge absorbs 3 continuous DOF with nothing surveyed, 1 with a single anchor
surveyed, 0 with two or more; a range between two surveyed anchors carries no information about
anything and must not be counted as an observation.

| tier | n | surveyed | unknowns | gauge | informative ranges | **redundancy** |
|---|---|---|---|---|---|---|
| **2** | 3 | 0 | 6 | 3 | 3 | **0** |
| **2** | 3 | 1 | 4 | 1 | 3 | **0** |
| **2** | 3 | 2 | 2 | 0 | 2 | **0** |
| 3 | 5 | 0 | 10 | 3 | 10 | +3 |
| 3 | 5 | 1 | 8 | 1 | 10 | +3 |
| 3 | 5 | 2 | 6 | 0 | 9 | +3 |

**Tier 2 has exactly zero redundancy at every survey count.** A three-node cluster in 2-D is
*minimally* rigid: three ranges determine three shape degrees of freedom exactly. There is no
survey count that fixes this, because the deficiency is in the anchor count, not the gauge.

Three consequences, and the third is the one that matters:

1. **The residual vector is identically zero.** A weighted least-squares solve on an exactly
   determined system reproduces its input. There is no consistency check to be had.
2. **`(JᵀWJ)⁻¹` still returns a covariance, and it would look perfectly reasonable.** It
   propagates range noise through an exact inverse. Nothing about it announces that it came from
   a solve with no over-determination.
3. **Leave-one-out FDE is impossible.** Drop any one range and the cluster is no longer rigid.
   B4 proposes to inject a mis-calibrated anchor and let exclusion find it — on tier 2, which is
   exactly where the `h = 1` fault lives and where §2's whole campaign is measured. **The fault
   B4 exists to test cannot be detected by the mechanism B4 exists to test it with, in the
   deployment as configured.**

## 2b. The claim demonstrated, not argued

A degrees-of-freedom count is an argument. `scripts/calib_prototype.py` makes it a measurement:
a weighted least-squares solve, written outside the simulator precisely so the C++ is written
against an established result rather than a hoped-for one.

**Noiseless recovery, free anchors started 5% of the ring radius away from truth:**

| tier | n | surveyed | outcome |
|---|---|---|---|
| 2 | 3 | 2 | max coordinate error **7.6e-14 m** |
| 3 | 5 | 2 | max coordinate error **4.8e-15 m** |
| 3 | 5 | 3 | max coordinate error **1.8e-15 m** |
| 2 | 3 | **1** | **SINGULAR** — rank 3 of 4 |
| 3 | 5 | **1** | **SINGULAR** — rank 7 of 8 |

**One surveyed anchor is not enough, on either tier, and the deficiency is exactly one.** That
is the rotation gauge, unfixed. It is not a numerical difficulty to be regularised away; there
is no information in the ranges about global orientation. **At least two surveyed anchors per
cluster is a hard requirement** — a deployment constraint, discovered before it could be
mistaken for a solver bug.

**And the decisive one. Inject a 5 m error into a single range, then look at the residual:**

| tier | surveyed | redundancy | residual after the fault | position error |
|---|---|---|---|---|
| **2** | 2 | **+0** | **8.0e-14 m** | **5.79 m** |
| 3 | 2 | +3 | **3.34 m** | 3.05 m |
| 3 | 3 | +3 | 3.37 m | 3.01 m |

**On tier 2 the fault is completely invisible.** The residual is numerically zero — the solve
reproduces its corrupted input exactly — while the estimate moves 5.79 m. The error goes
somewhere, and where it goes is straight into the position, silently, with a covariance that
reports nothing unusual. On tier 3 the same fault leaves a 3.34 m residual and is detectable.

This is consequence 2 above, realised: **an exactly determined solve returns an arithmetically
correct covariance for an estimate that is 5.79 m wrong.** Anything downstream that reads that
covariance — which is all of B3 — would be consuming a confident number about a wrong position.

**A defect found in the prototype's own accounting, worth recording because it produced the
error this section exists to prevent.** The first version counted redundancy as
`len(ranges) - n_free_params` and reported **+1** for the tier-2 two-surveyed case — a solve
with a residual of 8e-14 against a 5 m fault, labelled as having redundancy to spare. The cause:
a range between two *surveyed* anchors contributes no row to the Jacobian, because both its
endpoints are fixed, yet it was counted as an observation. Only informative ranges count. The
corrected counter returns +0 and agrees with the table in §2.

## 2c. SUPERSEDED — the tier-3 blocker was an artefact of treating r80 as a cutoff

Everything in §2 about tier 3 being SINGULAR under the engine's r80 **is withdrawn.** See
`DEFECTS.md`, "Tier 3 is not blocked: I read a percentile as a cutoff".

Two errors, compounding:

1. **The condition was too strong.** Rigidity of a 5-node 2-D graph needs a Laman subgraph of
   7 edges, so **two** of the five across-ring chords suffice, not all five. Measured: 5 adjacent
   → rank 4/6; +1 across → 5/6; **+2 across → 6/6, rigid.**
2. **r80 is a percentile, not a boundary.** A 19.02 m chord "beyond" a 12.9 m r80 is not an absent
   link; under the residual-corrected model it delivers **73.0%** of the time.

Enumerating all 1024 edge subsets weighted by corrected per-link PDR:

    P(solvable in a single epoch, 2 surveyed) = 0.9191
    P(>=2 across-ring chords observed within 2 epochs) = 0.9999

**Tier 3 self-calibrates on ~9 epochs in 10 and effectively always within two.** What survives is
that the edge set is *intermittent*, so the solve must accept a varying topology rather than
assume a fixed graph. The ring-radius rule `R <= 6.78 m` is not applied: it is sufficient rather
than necessary, and derived from the withdrawn cutoff premise.

The r80 to design against is settled by `experiments/UWB_R80_RESIDUAL_RESULT.md`: the engine's
**12.9 m**, with the residual-corrected model at 16.44 m recorded as the better model where the
two are not in conflict.

## 3. What this changes

The anchor count per site stops being deployment furniture and becomes **the** swept parameter,
ahead of the survey count:

    redundancy for n anchors, s surveyed, 2-D:  C(n,2) - C(s,2) - [2(n-s) - gauge]

      n=3  ->  +0 at every s          (the current tier-2 configuration)
      n=4  ->  +1
      n=5  ->  +3                    (the current tier-3 configuration)

`--t2Anchors` already exists as a command-line parameter. At n = 5 the intra-site chord is
2·150·sin 36° = 176.3 m — modelled PDR 0.9995, still far inside tier-2 range. So the sweep is
available without new geometry — but it is **not free**, and this is the trap:

> **Changing the anchor count changes the tag's fixes too.** More anchors per site means more
> ranges into the EKF whether or not anything is self-calibrated. The byte-identity null control
> for B1 must therefore be taken **at the same anchor count**, against a baseline re-run with
> that count — never against the committed 3-anchor baseline. A null control that compares
> across a changed deployment is not a null control, it is the experiment.

## 4. Registered acceptance for B2, before any run

1. **Identity case.** With every anchor surveyed the solve returns the surveyed positions to
   numerical precision — max coordinate error < 1e-9 m. Until this passes, no partial case is
   believed. (Demonstrated in §2b at 1e-13–1e-15 m under partial survey with a displaced start;
   the fully-surveyed case has zero free parameters by construction and is trivially exact.)
2. **Rank check, gating.** The solve reports the redundancy of the system it just solved, and
   **refuses to emit a covariance when redundancy is zero**, rather than emitting one that looks
   fine. This is the direct remedy for consequence 2 above, and §2b shows exactly the covariance
   it would otherwise have silently emitted.
3. **Reflection.** With fewer than two surveyed anchors in a cluster the solution is
   rank-deficient (§2b: SINGULAR, not merely reflection-ambiguous, because the rotation gauge is
   also unfixed at one surveyed anchor). The solve must detect and report this, not silently
   pick a branch or fail opaquely.

## 5. What is measured here, and what is left to B1

**Not answered here, contrary to the first draft of this section: tier-3 link feasibility.** It
depends on a model-versus-engine disagreement of about 2× in UWB r80 (§2), and the two sides of
that disagreement give opposite answers — +3 redundancy versus a singular system. Nothing in this
document settles it.

**What B1 must therefore measure, before any B2 claim is believed:** the engine's own PDR on the
tier-3 intra-site links at 11.76 m and 19.02 m, and on the tier-2 links at 259.8 m. That is not a
confirmation step. It is the step that decides whether B2 has a problem to solve on tier 3 at all.

**What is settled here and does not depend on that disagreement:**

- Tier 2 has zero redundancy at every survey count, and its links have an order of magnitude of
  range headroom, so the finding is geometric and not propagational (§2, §2b).
- One surveyed anchor is never enough on either tier — the rotation gauge is unfixed and the
  system is rank-deficient by exactly one (§2b).
- An exactly determined solve returns a confident covariance for a position that is 5.79 m wrong
  (§2b). That is the failure B3 would silently consume, and acceptance criterion 2 exists to
  prevent it.
