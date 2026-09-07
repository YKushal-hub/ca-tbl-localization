# Relayed ranging — the error model, written before any code

**2026-09-06.** C5 (hop-provenance) is gated on this document by an explicit ordering
rule: the error model is written and reviewed **before** the mechanism is built. This is
that document. Nothing in it is implemented yet, and `hopWeight` stays 0 until it is.

The reason for the ordering is recorded in CLAUDE.md §2: a non-zero weight on a hop count
that is structurally always zero changes nothing, and applied to an admitted-but-
unreachable peer it degrades the score for an **energy** defect rather than a provenance
one — a wrong result that looks like a working mechanism. The same trap applies to the
error model. A relayed measurement admitted under the wrong noise model does not fail
loudly; it produces a fix with a plausible covariance that is simply wrong.

---

## 1. What a relayed measurement is — and the category error to avoid

**A relayed range is not a range.** If tag A ranges to tag B at `r_AB`, and B ranges to
anchor G at `r_BG`, then `r_AB + r_BG` is **not** a measurement of `|A − G|`. By the
triangle inequality it is an **upper bound** on it, and a loose one: the bound is tight
only when A, B and G are collinear with B between them. Feeding a sum of hop ranges into
the EKF as if it were a range to G is a category error of the same family as applying a
time-of-arrival Cramér–Rao bound to RSSI inversion (`lora-ranging-submodule.h:17`, and the
open CRLB-on-tier-1 defect in CLAUDE.md §5).

**What a relay actually provides is a pseudo-anchor with a covariance.** B has a position
*estimate* `x̂_B` with covariance `P_B` from its own filter. A ranges to B directly. The
measurement is therefore an ordinary range to a reference point whose location is
uncertain — which is a solved problem, and a different one from range summation.

This is the whole content of the model. Everything below follows from it.

---

## 2. The measurement equation

For a range from A to relay B:

```
z = ||x_A − x_B|| + ν,        ν ~ N(0, σ²_range)
```

`σ_range` is returned by `ComputeRangingErrorM()` (`lora-ranging-submodule.h:39`) — it is
already the value used for the EKF R-matrix diagonal on direct measurements, and it is
tier-dependent: RSSI inversion on 868, CRLB-derived two-way ToF on 2.4 GHz and UWB.

`x_B` is not known, only `x̂_B` with covariance `P_B`. Writing `x_B = x̂_B + δ`, and
linearising about `x̂_B`:

```
z ≈ ||x_A − x̂_B|| − û·δ + ν,        û = (x_A − x̂_B) / ||x_A − x̂_B||
```

so the effective measurement variance is

```
σ²_eff = σ²_range + û^T P_B û                                              (1)
```

`û^T P_B û` is the relay's position variance **projected onto the range direction**. That
projection matters: a relay whose uncertainty is large across the line of sight and small
along it contributes almost nothing to a range measurement, and a scalar "relay is ±5 m"
would wrongly inflate it. `P_B` is already maintained per tag by `CaTblPolicy`'s EKF, so
the quantity exists; it is not currently exposed across nodes.

**Validity of the linearisation.** First-order propagation is accurate while
`tr(P_B) ≪ ||x_A − x̂_B||²`. It degrades exactly when the relay is close relative to its
own uncertainty — the geometry in which the range direction `û` is itself poorly
determined. This is a **hard admission condition**, not a refinement: see §4.

---

## 3. The correlation problem — the real reason provenance is not just a bigger sigma

Equation (1) handles one measurement. It does **not** handle two.

Two relayed measurements taken through the **same relay** share the same `δ`. Their errors
are correlated with covariance `û₁^T P_B û₂`, which is of the same order as the inflation
term itself. A filter that receives both and treats them as independent will:

- reduce its covariance as if it had two independent observations,
- and converge to a **biased** estimate, because the shared `δ` is a bias common to both,
  not noise that averages out.

**This is the failure mode that a per-measurement variance inflation cannot catch, and it
is the reason hop provenance needs to exist at all.** If relayed measurements were merely
noisier, `σ²_eff` would be the entire story and no provenance term would be needed. What
provenance encodes is that the *errors are shared*, and a scalar noise model has no way to
say so.

Two admissible treatments:

**(a) One relay, one measurement per cycle — the conservative choice.** Admit at most one
relayed measurement per distinct relay per update. Correlation between measurements
through *different* relays is second-order (their `P` are driven by disjoint anchor sets,
though not perfectly — see the shadowing note below) and is neglected, with that neglect
stated.

**(b) Carry the cross-covariance.** Correct, and it requires the filter to hold a joint
state over A and every relay it uses, or to run a covariance-intersection update. This is
a substantially larger change and is **not** proposed for C5.

**(a) is the specified treatment.** It is conservative in the right direction: it discards
information rather than inventing it.

### A second correlation, specific to this simulator

On **tier 1 the errors are not independent even across different relays**. RSSI inversion
derives its error from the shadowing field (`lora-ranging-submodule.h:17`), and shadowing
is spatially correlated — `lora-correlation-engine` models exactly that. Two tags near
each other inverting RSSI to the same anchor see *correlated* shadowing, so their position
errors are correlated before any relaying occurs. Treatment (a) does not fix this.

**Consequence, stated as a limit rather than solved:** relayed ranging on tier 1 is
specified as **not admissible**. This is a physical property of RSSI inversion in a
correlated shadowing field, not a simplification adopted for convenience — the
independence the filter requires is absent in the medium. Tiers 2 and 3 use two-way
time-of-flight, whose error is dominated by receiver SNR and hardware timing rather than
by a spatially correlated field, and are admissible.

---

## 4. Admission conditions

A relayed measurement is admitted only if **all** hold. Each has a falsifiable test.

| # | condition | why it is a limit, not a preference |
|---|---|---|
| A1 | Tier is 2.4 GHz or UWB | Tier-1 error is a correlated shadowing field (§3). Independence is absent in the medium. |
| A2 | `hopDepth == 1` | At depth 2 the relay's own position is itself relayed, so `P_B` understates its error by exactly the term this model adds. Correcting it requires the joint state of treatment (b). |
| A3 | `tr(P_B) < 0.25 · ‖x_A − x̂_B‖²` | The linearisation in §2 fails when the relay is close relative to its own uncertainty. The constant is **ours**, set so the neglected second-order term is below 10 % of `σ²_eff`; it is not cited. |
| A4 | `P_B` is fresher than the relay's own fix interval | A stale covariance describes a position the relay has since left. |
| A5 | One measurement per relay per update | The correlation of §3. |

**A2 is what makes `hopCount` almost binary in this system.** With depth capped at 1,
`hopCount ∈ {0, 1}`, and a PCS term linear in hop count has two reachable values. That is
worth saying plainly before anyone tunes a `hopWeight` expecting a gradient.

---

## 5. What this means for PCS, and why the current hop term is the wrong shape

The current form (`ca-tbl-policy.cc:672`, `:708`) is

```
pcs −= hopWeight · hopCount
```

Three objections, in increasing order of importance.

1. **It is unbounded below.** With a hop count and a weight both free, PCS can be driven
   arbitrarily negative. Every other PCS term is bounded; this one is not.
2. **It penalises a count, not an uncertainty.** If `σ²_eff` from (1) is applied to the
   EKF, the resulting larger covariance **already** degrades the PCS uncertainty term. A
   separate linear penalty on hop count double-counts the part of the effect that is
   already modelled.
3. **It does not encode the thing that actually distinguishes a relayed fix** — that its
   errors are *shared* with other measurements through that relay, and that the relay's
   own fix may be wrong in a way its covariance does not express.

**Specified replacement.** The provenance term should represent *unverifiability*, which
is what survives after (1) has done its work:

```
term_prov = 1                     if the fix used no relayed measurement
          = ρ                     if it used one, with ρ ∈ (0, 1]
```

applied **multiplicatively** to the PCS quality factor rather than subtracted, so it is
bounded by construction and cannot drive PCS negative. `ρ` is **ours** and must be
declared as ours in the source comment, per CLAUDE.md §9.

**This is a PCS change, and PCS changes are a planned campaign, not a bug fix**
(CLAUDE.md §2, `MASTER_PLAN.md` A8). It does not ride along with C5. C5 implements the
measurement model and the admission conditions; the PCS term is registered here as a
consequence and is sequenced separately, behind its own default-off flag.

---

## 6. What must be built, in order

1. **Expose `P_B` across nodes.** The relay must transmit its position estimate *and* its
   covariance. `ca-tbl-header.h` currently carries fields it does not serialise
   (`m_queryManifestId`, `m_reqDegradation`, `m_reqBattery`); a covariance is 3 doubles
   for the 2-D symmetric case and **must** be serialised, so this one changes airtime and
   therefore energy. That is a real cost and must appear in the energy accounting, not be
   waved through as a header field.
2. **Enable tag-to-tag links in `EnginePhyProvider`.** Reachable today — `ProcessTransmission`
   auto-registers an unknown receiver as a gateway with `fineTimestamp=false`
   (`lora-physics-engine.cc:298`), the correct hardware class for a tag. Held back only by
   the absence of this model, which now exists.
3. **Implement (1) and the A1–A5 admission conditions** behind `--relayedRanging`,
   default off, byte-identical when off.
4. **A gate with its negative fixture** (rule 6): a fixture in which a relayed measurement
   violating each of A1–A5 is offered, and the gate must observe it rejected. Five
   fixtures, all forced to fire.
5. **Pre-registration** before the first measurement run (rule 13).

**The §6 invariant is at risk in step 1 and must be checked there.** Exactly one tier
transmits per cycle and only that tier is charged. A relay exchange adds transmissions; if
they are not inside the per-cycle loop and charged to the active tier, the energy axis
stops meaning anything. This is the first thing to check in review of that step.

---

## 7. Acceptance criteria — how this model is falsified

Written now so they cannot be chosen after seeing results.

- **F1.** With `--relayedRanging` off, output is byte-identical to `baseline_v7` on its
  own recorded invocation, all seven digests.
- **F2.** In a geometry where the relay's position is known exactly (`P_B = 0` by
  construction in a fixture), a relayed measurement must produce the **same** filter update
  as a direct measurement to that point. If it does not, (1) is implemented wrongly.
- **F3.** As `tr(P_B)` grows, the admitted measurement's influence on the estimate must
  decrease monotonically, and must reach zero at the A3 boundary rather than jumping.
- **F4.** Feeding the *same* relayed measurement twice must not reduce the filter
  covariance twice. This is the direct test of §3, and it is the one that would catch the
  failure that a variance-only model cannot express.
- **F5.** Reported σ must remain calibrated against realised error once relayed
  measurements are admitted — the A4 check, applied to relayed fixes. **This depends on
  A4 (tier-1 calibration), which is itself open and blocked on trace regeneration.**
  F5 is therefore **not satisfiable today**, and that is stated here rather than
  discovered later.

---

## 8. Honest positioning

None of this is novel and it is not claimed to be. Uncertainty propagation through relayed
position references is the DV-Hop family and its successors (Huang et al. 2013, cited in
`Architecture.md` §13.2); pseudo-anchor formulations with covariance are standard in
cooperative localisation. The contribution here is the same as the project's throughout:
the assembly, the quantification, and the admission conditions being *stated and gated*
rather than assumed.

The fault model remains **environmental, not adversarial** (CLAUDE.md §9). A relay whose
covariance is wrong here is a cold-sagged battery skewing RSSI or a drifting sensor, not a
node lying about its position. Nothing in this document should be read as a defence
against a dishonest relay, and no such defence is specified.
