# PREREG K5 — is the 32.3% suspect rate an under-grown covariance?

**Registered before the cells run.** Rule 13.

---

## 1. What K2 left open

`ca_tbl_full` records **465 suspects in 1440 cycles — 32.3%** — against the **1%** that
χ²₀.₉₉ = 9.21 produces on a consistent filter. K2 ablated the corroboration witness and
the forced escalation, separately and together, and **none of them moved it** (465 → 457,
470, 455). The fault is in the covariance feeding the statistic, and K2 named three
candidates. This tests the second.

## 2. The hypothesis, with a mechanism

`processNoiseStdPos = 1.0`, `processNoiseStdVel = 0.5`, set in `ca-tbl-policy.cc` and
never touched by the scenario. With 2.4 GHz corrected from r80 1149.5 m to **156.0 m**,
the node coasts far longer between fixes than it did when those values were chosen. During
coasting the covariance grows only as fast as `Q` allows. **A `Q` tuned for short coasting
leaves the covariance under-grown**, so the innovation on the fix that finally arrives is
large against a covariance that is too small — which is a Mahalanobis distance that
over-fires.

If that is right, raising `Q` should drive the suspect rate down toward its design value
and close part of the accuracy gap between `ca_tbl_full` and `ca_tbl_no_integrity`.

## 3. Cells

The `k1a` configuration at `boxHalf=1500` — `k1a_box1500` is the paired control at the
same 20 seeds — varying only the process noise. Each cell runs 20 seeds as **four
processes of five seeds**, concatenated; the split is byte-identical and proved in
`PREREG_K4_SMALLBOX.md` §2.

| cell | `processNoiseStdPos` | `processNoiseStdVel` | |
|---|---|---|---|
| control `k1a_box1500` | 1.0 | 0.5 | already measured |
| `k5_q2_box1500` | 2.0 | 1.0 | ×2 in std, ×4 in variance |
| `k5_q4_box1500` | 4.0 | 2.0 | ×4 in std, ×16 in variance |

## 4. Predictions

| # | prediction | how it fails |
|---|---|---|
| V1 | The suspect rate falls **monotonically** with `Q`: 465 → fewer at ×2 → fewer again at ×4 | it does not fall, or is not monotonic |
| V2 | At ×4 the suspect count is **below 200** — under 14%, a third of the way to the 1% design rate | it stays above 200 |
| V3 | `ca_tbl_full`'s rmse gap to `ca_tbl_no_integrity` (99.76 vs 69.15 m at the control) **narrows** as `Q` rises | it does not narrow, or widens |
| V4 | Energy falls with `Q`, because fewer suspects means fewer forced escalations | energy does not fall |

## 5. Falsifier

**W1.** If V1 fails — the suspect rate does not fall materially with `Q` — then process
noise is **not** the cause, K2's second hypothesis is eliminated, and the remaining two
(A4's tier-1 sigma not surviving the correction; NLOS variance inflation not reaching the
filter) are what is left. That is a useful narrowing and is reported as such.

## 6. What is explicitly not permitted

**Raising `Q` until the suspect rate hits 1% is not the deliverable.** `Q` is a physical
statement about how the target moves, not a knob for making a test quiet. A value that
makes the Mahalanobis check behave but is inconsistent with a 1.2 m/s walker is a worse
defect than the one it hides, because it would be invisible. This campaign identifies
whether `Q` is *implicated*; choosing a defensible value is a separate exercise that has
to start from the mobility model.

The χ²₀.₉₉ = 9.21 threshold is not touched.

## 7. Abort conditions and analysis

120 rows per cell after concatenation, counted (rule 12); all four shards of a cell at the
same commit and all `dirty 0`; new paths only. Paired per-seed differences against
`k1a_box1500` on `suspects`, `rmseM`, `totalJ`, `escalations` and `fixCycles`, two-sided
sign test, with the static arms as the displacement floor.
