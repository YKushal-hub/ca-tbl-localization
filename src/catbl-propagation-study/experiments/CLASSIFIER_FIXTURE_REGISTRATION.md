# Negative fixture for the robustness classifier — registration, before running

Verifiable timestamp: this file's commit, which precedes any fixture result.

## Why

Fifteen ROBUST rows rest on a classifier that has never been observed to fail. This
project's oldest rule applies to it as much as to any gate: **a check that cannot be
observed to fail is not a check.** G5, G8, G9, G10, G11, G12 and G13 all carry negative
fixtures. The robustness classifier, which is carrying fifteen published conclusions, has
none.

## What is being tested

    def classify(by_terr, by_off):
        if len(set(by_terr)) != 1: return "UNDETERMINABLE"
        if len(set(by_off))  != 1: return "SENSITIVE"
        return "ROBUST"

`by_terr` and `by_off` are lists of **monotonic-shape strings** computed at each terrain
limit and each margin offset.

## A structural finding, recorded before the fixtures run

Auditing which experiments actually feed the robustness axes into their own function:

| rows | behaviour |
|---|---|
| P1–P9, P14 (**ten**) | pass `t` and `o` into `r80`; genuinely recomputed on both axes |
| **P10, P11, P12, P13, P15, P16 (six)** | **ignore `t` and `o` entirely** |

P15 is the starkest: it builds `bt = [tuple(...)]*2` and `bo = [tuple(...)]*3` — the same
tuple repeated — so its verdict lists cannot differ under any input. P13's function is
`lambda t,o,x: 1.0/math.sqrt(x)`. P10's is `p*delay`. None of these can move under terrain
or offset because none of them reads terrain or offset.

**So six of the sixteen ROBUST classifications are invariant by construction**, exactly the
warrant already recorded for P12 and P16 — the note was right and its scope was three times
too narrow. That is a larger and more precise explanation for "0 SENSITIVE of 16" than
"the test is too coarse".

## Registered prediction

**All three fixtures classify correctly.** The classifier is capable of emitting all three
verdicts; it was never exercised on inputs that demand them.

Specifically:

| fixture | construction | must classify |
|---|---|---|
| **F1** terrain-sensitive | a threshold crossing placed between the 0 dB and 30 dB values, so the monotonic shape differs across terrain but not across offsets | **UNDETERMINABLE** |
| **F2** offset-sensitive | a quantity whose shape differs across the three offsets while being identical at both terrain limits | **SENSITIVE** |
| **F3** robust | an analytic identity independent of both axes | **ROBUST** |

**F2 is the one at risk**, and the reason is stated in advance: `by_terr` is evaluated at
`offset = 0` and `by_off` at `terrain = 0`, so a construction that moves under offset will
usually also move under terrain and be caught by the earlier branch, returning
UNDETERMINABLE instead. If F2 misclassifies, the classifier cannot distinguish SENSITIVE
from UNDETERMINABLE and **0 SENSITIVE of 16 carries no information.**

F3 is included because the G12 incident showed the legitimate direction is the one that
gets forgotten: the fixture must confirm a genuinely robust conclusion still passes, not
only that a bad one fails.

## Consequence, registered before the result

| result | consequence |
|---|---|
| all three correct | 0 SENSITIVE of 16 is **a real result about those ten genuinely-tested parameters**; the six construction-invariant rows are relabelled regardless |
| any misclassifies | every ROBUST row becomes **NOT OBSERVED TO FAIL**, with the reason recorded. The table is not deleted and the conclusions are not weakened — only the column's warrant changes |

Fixtures live in `/tmp` only. A fixture that fails by crashing is not a fixture.

---

# RESULT — all three classify correctly. Registered prediction HELD.

Run after the registration was committed (`eef62db`), with `classify` and `shape` copied
verbatim from `ofat_run.py`. Fixtures in `/tmp` only. None failed by crashing.

| fixture | construction | must classify | did classify | |
|---|---|---|---|---|
| **F1** | `x` at terrain 0, `−x` at terrain 30 — shape flips across terrain only | UNDETERMINABLE | **UNDETERMINABLE** | ok |
| **F2** | `x·sign(o)` — shape flips across offset, identical at both terrain limits | SENSITIVE | **SENSITIVE** | ok |
| **F3** | `log(x)` — analytic identity in `x` alone | ROBUST | **ROBUST** | ok |

    F1  by_terr=['increasing','decreasing']   by_off=['increasing','increasing','increasing']
    F2  by_terr=['increasing','increasing']   by_off=['decreasing','increasing','increasing']
    F3  by_terr=['increasing','increasing']   by_off=['increasing','increasing','increasing']

**The classifier can be observed to fail, in both directions.** F2 — named in advance as
the one at risk, because `by_terr` is evaluated at `offset = 0` and a construction moving
under offset usually also moves under terrain — classified correctly, so SENSITIVE and
UNDETERMINABLE are genuinely distinguishable.

## Consequence: outcome 1, applied

**For the ten genuinely-tested rows (P1–P9, P14), "0 SENSITIVE" is a real result.** Nine
ROBUST and one UNDETERMINABLE (P8) keep their classification, and the ±4 dB offset really
did fail to move any of their conclusions.

**The six construction-invariant rows are relabelled regardless**, because the fixture
result says nothing about them — they were never put to the classifier in a form that could
fail:

| row | why it cannot move | new label |
|---|---|---|
| P10 | function is `p·delay` | INVARIANT BY CONSTRUCTION |
| P11 | function is `mean_fading_loss_db(K)` | INVARIANT BY CONSTRUCTION |
| P12 | function is `delay_spread · BW` | INVARIANT BY CONSTRUCTION |
| P13 | function is `1/√n` | INVARIANT BY CONSTRUCTION |
| P15 | verdict lists built as one tuple repeated | INVARIANT BY CONSTRUCTION |
| P16 | function is `0.423/f_D` | INVARIANT BY CONSTRUCTION |

**The corrected table is 9 ROBUST, 0 SENSITIVE, 1 UNDETERMINABLE, 6 INVARIANT BY
CONSTRUCTION** — not 15/0/1.

Per the registered consequence, **no conclusion is weakened to match the relabelled
column.** The six rows' findings stand on their own arguments and several never needed the
robustness column at all: `nlosSigmaInflation`'s answer (P10 vs P13) is an asymptotic
argument about how a bias and a zero-mean noise behave in anchor count, and is true
independently of any calibration this model has. P15's `GDOP = ∞` for collinear anchors is
rank deficiency. P16's coherence-time crossing is Doppler arithmetic. **These survive
either way, which is precisely why their robustness column was uninformative.**

---

# The magnitude dimension — computed at reduced resolution, and valid for only two rows

Population stated: **250 of the 2000 corpus draws, 1-in-8 systematic**, 18 bisection steps.
A subsample, because the full-resolution pass did not complete in two attempts.

| row | effect-size span at baseline | stability across the six robustness levels | valid? |
|---|---|---|---|
| P5 noise figure | 2.198× | **2.24×** | **yes** — 0 of 18 levels at the floor |
| P8 shadowing sigma | 1.409× | **2.11×** | **yes** — 0 of 18 levels at the floor |
| P1 carrier frequency | 17.34× | 48212× | **no** — 2 of 18 levels at the dead-link floor |
| P3, P4, P7, P9 | — | 15079×, 16090×, 4714×, 6.45× | **not individually checked**; the four-to-five-figure values have the signature of floor contamination |

**The metric is invalid wherever a level hits the 1 mm dead-link floor**, because the span
then measures the distance to an arbitrary constant rather than a change in effect size.
Derived before the numbers were trusted: a level at the floor inflates the span by roughly
`r_max/10⁻³`, which is the ~10⁴–10⁵ seen. Verified directly for P1.

**Where it is valid, it says the effect size roughly doubles.** P5 at 2.24× and P8 at
2.11× across terrain and offset. So the *shape* conclusions are stable while their
*magnitudes* are not, to about a factor of two — which supports the original concern rather
than dismissing it. The registered rule tests shape and shape alone, and that is now
demonstrated rather than suspected.

This does not change any row's classification. It is reported alongside, as registered.
