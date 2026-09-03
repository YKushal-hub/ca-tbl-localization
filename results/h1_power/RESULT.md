# h = 1 at adequate power — result

Registered `5359227`, before the first cell. 80 cells, `source 5359227 dirty 0 profile
optimized`, zero failures. Estimator: fraction of 1440 cycles with a fix whose `errM ≤ 25 m`,
unchanged from `hsweep`.

## Reproduction check

Seeds 1–20 are byte-comparable with `hsweep/h1_*` by construction. All 40 shared cells
(20 seeds × 2 policies) are **byte-identical** in `arm_summary.csv` digest. This is not a new
measurement layered on the old one — it is confirmation that nothing in the simulator moved
between the two runs, which the pre-flight established from `git diff --name-only a3b468e HEAD`
(only `.md` and gate-script files changed) and this check now confirms empirically.

## Question 1 — does `exclude` beat `none`? Registered: yes, CI excluding zero

| n | advantage | 95% CI | per-seed sd | seeds positive | verdict |
|---|---|---|---|---|---|
| 20 (`hsweep`, retrospective power 0.53) | +0.0053 | [+0.0002, +0.0105] | 0.0117 | 13/20 | held, marginal |
| **40 (registered, power 0.80–0.82)** | **+0.0037** | **[+0.0005, +0.0068]** | 0.0102 | 23/40 | **held** |

**HELD.** The interval excludes zero at the power the design was built for. The point estimate
moved from +0.0053 to +0.0037 — a 30% drop — which is the winner's-curse caveat registered in
`PREDICTION.md` resolving in the direction it warned about: the 20-seed estimate was inflated by
having been the one point of seven that reached significance in `hsweep`. The corrected estimate
is smaller and the interval is narrower (0.0102 sd against 0.0117), and it still excludes zero.
**This is the outcome the registration explicitly said would count as a real confirmation, and
it is reported as one — not upgraded past what the interval says.** The lower bound is +0.0005,
essentially at the boundary; this is a real, adequately-powered effect at the edge of what a
40-seed design can resolve, not a large one.

## Question 2 — does `ca_tbl_full` beat `ca_tbl_no_integrity`? No prediction registered

| policy | advantage | 95% CI | seeds positive | verdict |
|---|---|---|---|---|
| none | −0.0011 | [−0.0039, +0.0018] | 15/40 | not distinguishable from zero |
| exclude | +0.0026 | [−0.0014, +0.0067] | 19/40 | not distinguishable from zero |

**Neither interval excludes zero.** Unlike question 1, adequate power for question 1 does not
imply adequate power for question 2 — it was never powered, because no effect size for it was
ever measured to power against; PREDICTION.md priced a *positive* result at +0.0025 availability
without forecasting one. The measured advantage under `exclude` is +0.0026, almost exactly that
priced figure, with an interval that contains zero. **Reported as registered: this is not
evidence for or against `ca_tbl_full` beating `no_integrity` at `h = 1`.** It does not add a
scope line to Claim 9, because "not distinguishable from zero" is not a scope line — it is the
absence of one. It does not touch the falsification measured at `h = 4` and `h = 12`.

## What this settles, and what it does not

- **Item B's stage B4 may proceed on the response-policy hypothesis.** Exclusion beating
  rejection at `h = 1` is now measured at 80% power, not the 53% that produced the original
  finding, and it holds.
- **The winner's-curse caveat was not decoration.** The registered effect size (+0.0053) was an
  overestimate, as flagged before the run; the adequately-powered estimate is +0.0037. Anything
  downstream that used +0.0053 as a magnitude (rather than a direction) should be corrected to
  +0.0037.
- **Question 2 remains open**, and B4's design should not assume an answer to it. If B4 needs
  question 2 resolved, that requires its own power calculation against its own registered effect
  size — this run does not supply one, by design.
