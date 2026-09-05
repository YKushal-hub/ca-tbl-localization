# D-1 repaired — the system works better and the confidence claim still does not stand

**Written** 2026-09-05 from `results/pcs_forms`, 80 runs, 4 forms × 20 paired seeds,
six arms, `dirty 0`. Registered in `report/PREREG_PCS_FORMS.md` (`4fc556c`) **before the
campaign ran**. Registered abort check passed: the tree still reproduces `baseline_v7`
byte-for-byte.

**Verdict: REPAIRED BUT UNINFORMATIVE** — the branch the registration named in advance as
the expected one.

## 1. The repair works, and it works on every seed

Paired per-seed differences against `legacy`, same seed, two-sided sign test.

### `ca_tbl_no_integrity` — the arm where saturation bites

| form | Δ cep50M | Δ totalJ | Δ escalations | sign test |
|---|---|---|---|---|
| v2 | **−3.414 m** | +317.3 J | +48 | 0+/20− p < 0.0001 |
| weighted | **−3.848 m** | +142.1 J | +111 | 0+/20− p < 0.0001 |
| gated | **−3.799 m** | **+28.2 J** | +92 | 0+/20− p < 0.0001 |

**All twenty seeds improve, on all three forms.** Prediction 1 holds without qualification.

### `ca_tbl_full` — the arm that already had the integrity gate

| form | Δ cep50M | Δ totalJ | sign test |
|---|---|---|---|
| v2 | +0.573 m (worse) | +277.0 J | 20+/0− p < 0.0001 |
| weighted | +0.101 m | +99.2 J | 11+/9− p = 0.82 (no effect) |
| gated | −0.050 m | −9.8 J | 5+/15− p = 0.041 |

**Prediction 2 holds.** `ca_tbl_full` barely moves while `no_integrity` moves 3.8 m,
because the integrity gate was already forcing the escalations the saturated score
suppressed. The arm with a second mechanism had less to gain from repairing the first.

### The result that was not predicted, and it is the useful one

**`gated` buys the same accuracy as `weighted` for a fifth of the energy** — −3.799 m for
28.2 J against −3.848 m for 142.1 J. And `v2` spends 317 J to make `ca_tbl_full`
*worse* on 20/20 seeds.

A veto is cheaper than a rescaling. Rescaling the terms moves the score everywhere and
changes escalation behaviour across the whole run; a floor on term 1 fires only where the
uncertainty term has actually saturated. If a repair is adopted, **`gated` is the one to
adopt**, and that is an engineering finding this campaign produced rather than assumed.

## 2. The monotone control — and it closes the confidence-score question

ρ(PCS, error) against ρ(−traceSigma, error), per seed, seed-level bootstrap 10 000
resamples. `traceSigma` is the EKF covariance trace PCS is computed from, so the control
is a monotone function of the score's own input. Advantage = |ρ_PCS| − |ρ_control|;
positive means PCS carries information the filter statistic does not.

| form | arm | ρ_PCS | ρ_control | advantage | 95 % CI | verdict |
|---|---|---|---|---|---|---|
| legacy | `ca_tbl_full` | −0.5083 | −0.5594 | −0.0511 | [−0.0630, −0.0403] | **loses** |
| legacy | `no_integrity` | −0.5500 | **−0.8331** | **−0.2832** | [−0.3038, −0.2648] | **loses** |
| v2 | `ca_tbl_full` | −0.6274 | −0.6709 | −0.0435 | [−0.0545, −0.0340] | **loses** |
| v2 | `no_integrity` | −0.6259 | −0.6637 | −0.0377 | [−0.0472, −0.0299] | **loses** |
| weighted | `ca_tbl_full` | −0.5744 | −0.5858 | −0.0114 | [−0.0156, −0.0077] | **loses** |
| weighted | `no_integrity` | −0.5596 | −0.5691 | −0.0095 | [−0.0136, −0.0059] | **loses** |
| gated | `ca_tbl_full` | −0.5264 | −0.5395 | −0.0131 | [−0.0151, −0.0112] | **loses** |
| gated | `no_integrity` | −0.5315 | −0.5501 | −0.0186 | [−0.0219, −0.0154] | **loses** |

**Every form loses to the control, in every cell, with the whole CI below zero.**
Prediction 3 holds.

The shape of the loss is the interesting part. On `no_integrity` the legacy score is
beaten by **0.283** — the raw covariance trace predicts error at ρ = −0.833 while the
saturated score manages −0.550. That gap *is* the saturation defect, measured. Repairing it
closes the gap to **−0.0095** (weighted) and **−0.0186** (gated): the score comes up
almost exactly to the control and stops there.

That is the cleanest possible statement of what PCS is. It is a **re-encoding of the
filter's own covariance trace**, and a good one once the saturation is removed — but it
never adds anything the trace did not already carry.

## 3. What this means

**Three functional forms have now failed the same information test**: legacy, v2, and both
repairs. `PREREG_PCS_FORMS.md` §6 and `WORK_PACKAGES.md` A8 both say the same thing about
this outcome — the confidence-score contribution is closed, and **a fourth form should not
be run**.

But the engineering result stands and is worth stating plainly: **the saturation defect was
real, it cost 3.8 m of median error on the arm without a second safety mechanism, and
fixing it recovers that on 20 of 20 seeds for 28 J.** A confidence score that cannot veto
is a defect whether or not the score is novel.

Those two conclusions are independent, and the honest report keeps them apart:
the mechanism is repaired, and the novelty claim does not survive.

## 4. What this campaign cannot establish

- **Whether calibrated weights would differ.** The `weighted` defaults are hand-set. CAL-01
  specifies LOOCV and it still has not been run — but a fitted weighting is a different
  point in the same family, and the family has now failed three times.
- **Whether the repair helps at other geometries.** One geometry, `boxHalf=2000`.
- **Whether `pcsGateFloor = 0.10` is well-placed.** Ours, unswept.
- **Anything about the integrity check**, which is a separate mechanism measured by A2.

## 5. A correction to this analysis, recorded

The first run of `analyse_pcs_forms.py` reported "BEATS CONTROL" in all eight cells. The
verdict logic tested whether ρ_PCS − ρ_control differed from zero, which is not the
question: both correlations are negative, so a score wins by being **more** negative, and
the raw difference inverts the sign. Corrected to |ρ_PCS| − |ρ_control| before anything was
reported. The inverted version would have claimed the confidence score beat its own control
in every cell — the exact opposite of the truth, and it would have reversed the thesis.
