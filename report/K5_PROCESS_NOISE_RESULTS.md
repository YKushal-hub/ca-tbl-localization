# K5 — process noise is implicated in the suspect rate, and is not the whole cause

**Registered** `report/PREREG_K5_PROCESS_NOISE.md` (`55bfb0a`), committed before the cells
ran. Two cells at `boxHalf=1500`, 20 seeds each as four processes of five, all shards at
`55bfb0a`, all `dirty 0`, 120 rows each. Control `k1a_box1500`.

## The result

| | control (Q 1.0/0.5) | Q ×2 std | Q ×4 std |
|---|---|---|---|
| **suspects / 1440** | **465 (32.3%)** | **390 (27.1%)** | **310.5 (21.6%)** |
| paired vs control | — | −71, 4+/16− p 0.0118 | **−132.5, 0+/20− p<0.0001** |
| 95% CI | — | [−117.5, −47.5] | [−185.5, −113.5] |
| `totalJ` | 15.936 J | 15.637 J (p 0.50) | 15.619 J (p 0.82) |
| `escalations` | 99 | 102.5 (p 0.65) | 101.5 (p 0.26) |

| # | prediction | outcome |
|---|---|---|
| V1 | suspect rate falls monotonically with Q | **HELD** — 465 → 390 → 310.5, and ×4 is 0+/20−, p < 0.0001 |
| V2 | at ×4 the suspect count is below 200 | **FAILED** — 310.5, still 21.6% of cycles |
| V3 | the rmse gap to `ca_tbl_no_integrity` narrows | **NOT ANSWERABLE** — see the limitation below |
| V4 | energy falls with Q | **FAILED** — 0.05–0.13 J, neither significant |

## What it means

**Process noise is implicated and is not the whole cause.** Raising `Q` by **16× in
variance** removes only about a third of the excess suspects — from 32.3% to 21.6% against
a design rate of 1%. The direction is exactly as the mechanism predicted, and the size is
not enough.

So the covariance really is under-grown during coasting, *and* something else is inflating
the innovation as well. K2's other two candidates stand:

- A4's finding that tier-1 sigma is **over-stated** ~1.7× may not survive the corrected
  propagation — and if it reversed, an under-stated measurement covariance would inflate
  the Mahalanobis distance directly.
- The **NLOS variance inflation may not be reaching the filter**. `ROOTCAUSE_ENVELOPE` Q2
  already found the operative NLOS tag firing on 75–78% of links against a
  propagation-layer determination of 0.3–12.8%, which is a 250× discrepancy at the low end.

**V4 failing is itself informative.** Escalations barely move (99 → 101.5) even as suspects
fall by a third, so on this configuration a suspect is *not* reliably converting into an
escalation — consistent with K2, where turning `suspectForcesEscalation` off changed
nothing measurable either. The suspect flag is firing constantly and doing less than its
name suggests.

## A limitation in this campaign's own design, found by its noise floor

The registration required the static arms as a displacement floor. They moved, and
**significantly**:

| cell | `static_868` drift | sign test |
|---|---|---|
| Q ×2 | 32.44 m | 18+/2− **p 0.0004** |
| Q ×4 | 39.90 m | 17+/3− **p 0.0026** |

Those arms carry no policy and cannot see `Q`, so this is not a treatment effect. **The
control was run on a different binary** — `k1a_box1500` predates the process-noise and
evidence-gate commits — and `TECH_DEBT` 4.1 and 4.2 already record a 1-ulp trace divergence
with no known cause and `-march=native` making byte-identity machine-dependent. 1.2 records
that rule 8 does not mention the cross-binary case. **This is that case.**

Consequences, stated rather than worked around:

- **The suspects result stands.** −132.5 at 0+/20− with a CI of [−185.5, −113.5] is an
  effect on a counter the static arms do not have, monotonic in the treatment, and far
  larger than any plausible ulp drift.
- **The rmse result does not.** The +24.2 m at Q ×4 is *smaller* than the 39.9 m the static
  arms drifted, so V3 cannot be answered from these cells and is not.
- **The fix is cheap and was not done:** re-run the control on the current binary. A
  same-binary control is the difference between an answerable V3 and an unanswerable one,
  and this campaign should have included one.

## What must not be concluded

**Not that `Q` should be raised to 4.0.** The registration §6 bars it in advance and the
reason holds: `Q` is a physical statement about how the target moves, and a value chosen to
quiet a test while being inconsistent with a 1.2 m/s walker is a worse defect than the one
it hides, because it would be invisible. This campaign says `Q` is *implicated*. Choosing a
defensible value starts from the mobility model and is separate work.

**The χ²₀.₉₉ = 9.21 threshold is untouched and stays untouched.**
