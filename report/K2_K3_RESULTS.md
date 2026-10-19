# K2 and K3 — what costs, and what saves

Two registered follow-ups to `report/K1_DOMINATION_RESULTS.md`, run at `boxHalf=1500` on
the corrected propagation, 20 paired seeds (7–26), `dirty 0`, 120 rows each.

**Both have a zero noise floor.** The static arms are **byte-identical** between every
cell and its control — `0+/0−` non-zero differences, median drift 0.0000 m — because
these flags touch only the policy arms and displace no shared RNG. Unlike K1's
cross-configuration comparisons, where the displacement floor reached 35.7 m, every
difference below is a pure treatment effect.

---

# K3 — the evidence gate is worth 324 J, and it is off by default

**Registered** `report/PREREG_K3_EVIDENCE_GATE.md` (`0a060dc`), committed before the cell
ran. One flag moves: `--escalationRequiresEvidence`.

| quantity | gate ON | gate OFF | median Δ | sign test | 95% CI |
|---|---|---|---|---|---|
| `totalJ` | 15.94 J | **339.11 J** | **+323.75 J** | 20+/0− p<0.0001 | [+298.0, +361.4] |
| `j_uwb` | 0.008 J | **322.65 J** | +322.64 J | 20+/0− p<0.0001 | [+297.5, +361.1] |
| `j_2p4` | 7.31 J | 4.12 J | −2.96 J | 0+/20− p<0.0001 | [−3.51, −2.66] |
| `escalations` | 99 | 191 | +91.5 | 20+/0− p<0.0001 | [+76.0, +102.5] |
| `rmseM` | 99.76 m | 104.74 m | +7.85 m | 13+/7− p 0.263 | [−43.0, +15.6] |
| `fixCycles` | 1357.5 | 1365.5 | +11.5 | 13+/5− p 0.096 | [0.0, +22.5] |

**The gate saves 95% of the arm's energy at no measurable accuracy cost**, and
`j_uwb` accounts for 322.6 J of the 323.8 J. The mechanism is exactly what the gate is
for: without it the policy escalates to UWB 92 more times per run, into a tier that —
per K1 — obtains **zero** fix cycles in every cell of that campaign. It is paying full
UWB receive energy for nothing.

## R3 failed, and the failure is the interesting part

R3 predicted the gate alone would **not** reach `k1b_box1500`'s 320.99 J, on the reasoning
that the other four flags in the repaired bundle carry some of the energy. Measured:
turning off the gate alone gives **339.11 J — more than the whole unrepaired bundle.**

So the evidence gate does not merely carry most of the bundle's 305 J energy effect; it
carries **all** of it, and the other four flags together are a small net *negative* on
energy. K1 §4's inference is confirmed and then some.

- **R1 HELD** (predicted > 100 J; measured 323.75 J).
- **R2 HELD** — the rise is UWB, and escalations rise with it.
- **R3 FAILED** — and S1, the falsifier that would have withdrawn K1 §4's sentence, does
  not fire. The sentence stands as measured rather than inferred.

## The recommendation this supports

**`--escalationRequiresEvidence` should default to `true`.** It is currently `false`.
The evidence is 20 of 20 seeds, p < 0.0001, a 95% energy reduction, no significant
accuracy change, and a zero noise floor.

**The one caveat, stated because it is real:** the gate is suppress-only, and this
measurement is in a regime where tier 3 never delivers a fix. Where tier 3 *does* work,
suppressing escalation could cost. K1 found no such regime in its sweep, and the
untested small-box regime (Amendment 2 §A2.4) is where one might exist. The
recommendation is made on the evidence available and is falsifiable by that cell.

---

# K2 — the integrity check's cost is the Mahalanobis test itself

**Registered** `report/PREREG_K2_INTEGRITY_COST.md`, committed before the cells ran.
Control `k1a_box1500`. Target to reach: `ca_tbl_no_integrity` at **69.15 m / 14.94 J**
against `ca_tbl_full`'s 99.76 m / 15.94 J.

| cell | flag removed | rmse | energy | sign test (rmse) |
|---|---|---|---|---|
| control `k1a_box1500` | — | 99.76 m | 15.94 J | — |
| `k2_nocorr` | `--corroborationCheck=false` | 93.69 m | 16.35 J | 7+/4− p 0.549 |
| `k2_nosusesc` | `--suspectForcesEscalation=false` | 111.69 m | 15.97 J | 10+/10− p 1.000 |
| `k2_both` | both | 113.08 m | 16.23 J | 10+/10− p 1.000 |
| target `ca_tbl_no_integrity` | whole check off | **69.15 m** | **14.94 J** | — |

**None of the three ablations recovers the gap.** Not one is significant, and two are
numerically worse than the control. G1 of the registration fires:

> If no cell improves on `k1a_box1500`, the integrity check's cost is not in either
> flagged mechanism, and the finding is that the Mahalanobis test itself is mis-scaled
> under the corrected propagation.

Q3 and Q4 both **held**: neither flag alone closes the gap, and neither does both
together. The cost is in the test.

Worth noting on `k2_nocorr`: **9 of 20 seeds show a difference of exactly zero.** The
corroboration witness does not fire at all on nearly half the seeds, which is consistent
with `TECH_DEBT` B recording its constants — `k = 3`, assumed range sigma 30 m, floor 0.5,
minimum 2 witnesses — as uncalibrated and its sensitivity as untested.

## The measurement that names the defect

`ca_tbl_full` at the control records **465 suspects in 1440 cycles — 32.3%.**

The Switching-Point Integrity Check compares a Mahalanobis distance against
χ²₀.₉₉ = 9.21. **On a consistent filter that threshold is exceeded on 1% of cycles.**
Observed is 32.3%, and turning off corroboration barely moves it (457) and turning off
the forced escalation does not either (470).

**The innovation statistic is mis-scaled by a large factor**, and that single number
explains the whole of K2: no flag helps because no flag is the problem. A third of all
cycles are being called suspect, their covariance inflated `suspectInflateFactor = 4.0`×,
and — until `--suspectForcesEscalation=false`, which changes nothing else — escalated.

**This must not be fixed by moving 9.21.** The registration bars it and `RULES.md`
repeats the reason: the analytic model refusing to reconcile is the only instrument that
has ever caught a defect of this class in this project, and a quantile fitted to make a
campaign look better destroys it. 9.21 is a distributional constant. **The fault is in
the covariance that feeds the statistic, and finding which term is the open question.**

One direction is already ruled out by sign. A4 measured tier-1 uncertainty as
**over-stated** ~1.7× in sigma; an over-stated covariance makes the Mahalanobis distance
*smaller* and would produce **fewer** suspects, not 32× more. So whatever is wrong, it is
not A4's finding carried forward — either that finding does not survive the corrected
propagation, or the process noise `Q` is too small for the longer coasting the corrected
tier-2 range imposes, or the NLOS variance inflation is not reaching the filter. Those
are three testable alternatives and none of them has been tested.

---

## What these two change about the K1 picture

- The energy half of the repaired bundle is now **attributed to one flag** rather than
  inferred from five, and that flag is default-off.
- The accuracy gap between `ca_tbl_full` and `ca_tbl_no_integrity` is **localised to the
  Mahalanobis test**, with a quantitative signature — a 32.3% suspect rate against a 1%
  design rate — that says what to look at next.
- Neither result changes K1's answer. `ca_tbl_full` is still dominated by `static_2p4` at
  every scale, and the fixed threshold still beats it from `boxHalf=1500` upward.
