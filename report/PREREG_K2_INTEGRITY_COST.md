# PREREG K2 — which part of the integrity check is costing accuracy and energy?

**Registered before K2 runs.** Rule 13. Written while K1 was still executing, from K1
cells that were already complete; the K1 result it responds to is stated below so the
reader can see exactly what was known when this was written.

---

## 0. What K1 found, and why it demands this campaign

At every completed K1 cell, `ca_tbl_no_integrity` beats `ca_tbl_full` on **both** axes:

| cell | rmse: full → no_integrity | energy: full → no_integrity | sign test (rmse / energy) |
|---|---|---|---|
| `k1a_box1500` | 99.8 → 69.2 m | 15.94 → 14.94 J | 4+/16− p 0.0118 / 6+/14− p 0.115 |
| `k1a_box2000` | 206.7 → 90.8 m | 19.69 → 16.15 J | 2+/18− p 0.0004 / 3+/17− p 0.0026 |
| `k1a_box3000` | 276.9 → 166.5 m | 20.69 → 19.80 J | 7+/13− p 0.263 / 6+/14− p 0.115 |
| `k1c_brief` | 344.4 → 135.5 m | 21.60 → 15.88 J | 2+/18− p 0.0004 / 1+/19− p 0.0000 |

**The integrity check is not merely failing to help. It is actively costing** — up to
209 m and 5.7 J in the brief's own deployment, on 18 and 19 seeds of 20.

That inverts `report/INTEGRITY_CHECK_RESULTS.md`, which measured energy improved 8/8 and
accuracy 7/8. That campaign ran at `--duration=600 --boxHalf=1000` **on the split
environment**, where 2.4 GHz had r80 1149.5 m instead of 156.0 m. The two results are not
in contradiction; they are the same mechanism in two different propagation regimes, and
only one of those regimes is the one the scenario now builds.

## 1. The hypothesis this tests

`ca_tbl_full` differs from `ca_tbl_no_integrity` by the Switching-Point Integrity Check
and what fires from it. Three candidate mechanisms, and they are separable by flag:

1. **The corroboration witness** (`--corroborationCheck`) — `k = 3`, assumed range sigma
   30 m, floor 0.5, minimum 2 witnesses. `TECH_DEBT` B records these constants as
   uncalibrated and their sensitivity as untested.
2. **The forced escalation on a suspect fix** (`--suspectForcesEscalation`, default
   **true**) — a suspect fix escalates. Under the corrected propagation, escalation
   frequently goes to a tier that cannot deliver, so this spends energy *and* loses the
   fix.
3. **The Mahalanobis test itself** (χ²₀.₉₉ = 9.21), which has no off-switch and is what
   remains once 1 and 2 are accounted for.

**Stated mechanism.** χ²₀.₉₉ = 9.21 presumes a consistent filter. A4 already measured
tier-1 uncertainty as over-stated by ~1.7× in sigma, and the corrected propagation makes
coasting longer and innovations larger. A mis-scaled innovation statistic marks good
fixes suspect; `suspectForcesEscalation` then converts each into an escalation that
cannot solve. The predicted signature is **more escalations and fewer fix cycles**, and
K1 shows both: `k1c_brief`, 170 escalations against 101, and 1236 fix cycles against 1320.

## 2. Cells

All at the `k1a` configuration — corrected environment, designed deployment, gated PCS,
floor on demand, evidence gate, bidirectional digest — `--seedCount=20`, seeds 7–26,
`--duration=3600`, `--trace=false`. Only the named flag moves.

| cell | change | isolates |
|---|---|---|
| `k2_nocorr_box1500` | `--corroborationCheck=false` | the corroboration witness |
| `k2_nosusesc_box1500` | `--suspectForcesEscalation=false` | the forced escalation |
| `k2_both_box1500` | both of the above | whether the two interact |

`boxHalf=1500` because it is the cheapest completed K1 cell that shows the effect
(~366 s/seed), and because `k1a_box1500` is its paired control, already measured, at the
same flags but for the one under test.

## 3. Predictions, committed in advance

| # | prediction | how it fails |
|---|---|---|
| Q1 | `--suspectForcesEscalation=false` recovers **more** of the gap than `--corroborationCheck=false` | corroboration recovers more, or neither does |
| Q2 | Turning off the forced escalation reduces `escalations` **and** raises `fixCycles` toward `ca_tbl_no_integrity`'s | escalations fall but fix cycles do not follow |
| Q3 | Neither flag alone closes the gap to `ca_tbl_no_integrity` — the Mahalanobis test carries some of it | either flag alone closes it, which would clear the test itself |
| Q4 | `k2_both` still does not fully reach `ca_tbl_no_integrity`, for the same reason | it reaches or beats it |

Q4 failing would be informative rather than embarrassing: it would mean the whole cost is
in the two flagged mechanisms and the Mahalanobis test is neutral.

## 4. Falsifiers and what is not permitted

- **G1.** If no cell improves on `k1a_box1500`, the integrity check's cost is not in
  either flagged mechanism, and the finding is that the Mahalanobis test itself is
  mis-scaled under the corrected propagation. That is reported as-is.
- **The threshold must not be tuned.** χ²₀.₉₉ = 9.21 is a distributional constant, not a
  parameter. If the statistic is mis-scaled the fix is to the covariance that feeds it —
  A4's over-stated tier-1 sigma — not to the quantile. Fitting 9.21 to whatever makes
  this campaign look better would destroy the one thing the check is for.
- **No arm is dropped from the report** because it is unflattering.

## 5. Abort conditions

Same as K1: 120 rows exactly per cell, counted (rule 12); `dirty 0` in every sidecar;
new paths only, nothing tracked regenerated in place (rule 7). Cells failing any of these
are excluded **by name**.

## 6. Analysis

Paired per-seed differences against `k1a_box1500`, same seeds, two-sided sign test, on
`rmseM`, `totalJ`, `escalations`, `fixCycles` and `suspects`. `ca_tbl_full` is the
subject in every cell; the other five arms are unaffected by these flags and serve as the
RNG-displacement noise floor — **if a static arm moves materially between cells, the
per-seed pairing is measuring stream displacement and not the flag**, and that check runs
before any conclusion is drawn.
