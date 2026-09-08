# PREREG K7 — is the suspect rate an innovation covariance missing its R term?

**Registered before the cell runs.** Rule 13. This is the owner's step 2.

---

## 1. The chain that leads here

- **K2:** `ca_tbl_full` records **465 suspects in 1440 cycles — 32.3%** — against the 1%
  that χ²₀.₉₉ = 9.21 gives on a consistent filter. Ablating the corroboration witness and
  the forced escalation moved it to 457 and 470. Neither is the cause.
- **K5:** raising process noise by 16× in variance took it to 21.6%. `Q` is implicated and
  is not the whole cause.
- **This:** the remaining candidate is the statistic itself, and the code already says
  what is wrong with it.

## 2. The defect, in the flag's own words

`--mahalUseR` (`ca_tbl_causal_sim.cc:5285`), default **false**:

> "Use the **correctly specified** integrity gate: weighted multilateration for the test
> point and **S = P_prior + (JᵀR⁻¹J)⁻¹** for the distance. false reproduces the
> pre-existing baseline."

With the flag false, `ca-tbl-policy.cc:963` takes the `MahalanobisFromPrior` branch, whose
innovation covariance is **`P_prior` alone**. The measurement term `(JᵀR⁻¹J)⁻¹` is absent.

A Mahalanobis distance is `dᵀ S⁻¹ d`. Dropping a positive-definite term from `S` makes
`S⁻¹` larger, so **every distance is inflated and the test over-fires** — which is the
32.3%-against-1% signature exactly. The correctly specified version is already implemented;
it is switched off because switching it on changes every published number.

**This is not a new mechanism and needs no new code.** It is a flag that exists, with an
implementation that exists, described in the source as the correct one.

## 3. The cell

`k7_mahalR_box1500` — the `k1a` configuration at `boxHalf=1500`, `--mahalUseR=true`,
nothing else changed. `k1a_box1500` is the paired control at the same 20 seeds. 20 seeds as
four processes of five; the split is byte-identical (`PREREG_K4_SMALLBOX.md` §2).

## 4. Predictions

| # | prediction | how it fails |
|---|---|---|
| Z1 | The suspect count falls **below 200** — under 14%, which K5's Q sweep could not reach with a 16× variance increase | it stays above 200 |
| Z2 | It falls **further than K5's best** (310.5 at Q ×4), i.e. the missing R term explains more of the excess than process noise does | it does not |
| Z3 | `ca_tbl_full`'s rmse improves relative to the control and closes at least half the 30.6 m gap to `ca_tbl_no_integrity` (99.76 vs 69.15 m) | it does not close half |
| Z4 | Energy does not rise — fewer suspects should mean no more escalations, not more | energy rises significantly |

## 5. Falsifier

**Za.** If Z1 fails and the suspect rate stays above 14%, then the missing R term is not
the dominant cause either, and all three of K2's named candidates have been tested and
found partial. The honest conclusion would be that the statistic is mis-scaled by a
combination no single term explains, and the next step is to instrument `S` directly rather
than to keep switching flags.

## 6. What is not permitted

**The χ²₀.₉₉ = 9.21 threshold is not touched.** If this cell succeeds it will be because a
term was restored to the covariance, which is a correctness fix; if it had been achieved by
moving the quantile it would have been tuning. `CLAUDE.md` and `TECH_DEBT` J.1 both bar the
latter and the reason stands.

**A pass here does not license flipping the default in the same breath.** `mahalUseR=true`
changes the fix point as well as the test — it uses weighted multilateration — so it moves
accuracy for two reasons at once, and a default change needs the same treatment the other
two got this session: null control, reference regeneration, gate run.

## 7. Abort conditions and analysis

120 rows after concatenation, counted; all four shards at one commit; all `dirty 0`; new
path. Paired per-seed differences against `k1a_box1500` on `suspects`, `rmseM`, `totalJ`,
`escalations`, `fixCycles`, two-sided sign test, static arms as the displacement floor.

**The cross-binary caveat applies and is stated in advance:** `k1a_box1500` was produced on
an earlier binary, and K5 measured static-arm drift of 32–40 m between binaries. So the
**suspects** comparison — a counter the static arms do not carry, expected to move by
hundreds — is admissible, and any **rmse** difference smaller than ~40 m is not. Z3 is
therefore reported with that floor beside it, and if the improvement is under 40 m it is
reported as **not resolvable**, not as absent.
