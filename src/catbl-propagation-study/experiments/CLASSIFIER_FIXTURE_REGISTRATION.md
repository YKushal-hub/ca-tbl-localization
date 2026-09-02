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
