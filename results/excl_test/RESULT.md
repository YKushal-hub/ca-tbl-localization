# excl_test — result

Registered in `PREDICTION.md`, committed `215e2d8`, before the first cell. Cells re-run at
`42c7e30` after the branch census was instrumented; availability figures are identical
across both sources, so the census does not perturb the exclude path.

Provenance: 40 cells, `source 42c7e30 dirty 0 profile optimized`, no exceptions.

## Availability

Estimator: **fraction of the 1440 cycles producing a fix with `errM ≤ 25 m`.** Validated
before use by reproducing `c9_clean`'s ledger figures exactly, including per-seed sign
counts. Two simpler definitions (`fixCycles/steps`, `1 − noFixCycles/steps`) give ≈0.0001
and do **not** reproduce the ledger; they were rejected rather than used.

| fault | policy | full | no_integrity | gap | negative seeds |
|---|---|---|---|---|---|
| F1 single-anchor | none | 0.9009 | 0.9314 | −0.0305 | 18/20 |
| F1 | **exclude** | 0.9073 | 0.9314 | **−0.0241** | 16/20 |
| F3 common-mode | none | 0.8006 | 0.8547 | −0.0540 | 19/20 |
| F3 | **exclude** | 0.7720 | 0.8547 | **−0.0827** | **20/20** |

## Registered prediction: MISSED, in both cells, and in opposite directions

Registered: *`exclude` loses less availability than `none` in every cell*, with the gap
shrinking to at least half — −0.016 or better (F1), −0.027 or better (F3).

- **F1: direction HELD, magnitude MISSED.** −0.0305 → −0.0241, a 21% closure against a
  registered 50%.
- **F3: MISSED outright — `exclude` is worse.** −0.0540 → −0.0827, a 53% *widening*, with
  the sign now holding on 20/20 seeds rather than 19/20.

The registered falsifier — *if `exclude` is no better than `none`, the "rejection discards
information" hypothesis is wrong* — **fires on F3.**

## Branch census — what the mechanism actually did

Population: `ca_tbl_full` only; `no_integrity` never trips the gate and has no branches.

| fault | gate trips | none cleared | ambiguous | too few | **cleared** |
|---|---|---|---|---|---|
| F1 | 2949 | 1300 (44.1%) | 303 (10.3%) | 125 (4.2%) | **1221 (41.4%)** |
| F3 | 6006 | 4498 (74.9%) | 294 (4.9%) | 336 (5.6%) | **878 (14.6%)** |

## The mechanism, evidenced rather than asserted

**F3's harm is quantitatively the spuriously-cleared cycles.**

    spurious clear rate = 878 / 20 seeds / 1440 cycles = 0.0305
    availability harm   = 0.0827 - 0.0540             = 0.0287

Within 6% of each other. Under a common-mode fault **every** link carries the same offset,
so no single exclusion can restore consistency — 74.9% of trips clear nothing, as the
single-fault assumption predicts. The 14.6% that *do* clear are the damage: a subset that
passes the gate is still biased and is now computed from **less** redundancy, so the filter
accepts a confidently wrong fix. Rejection at least preserved a good prior to coast on.
Exclusion converts "no fix this cycle" into "a wrong fix that also corrupts the next one".

**F1's benefit is far smaller than its clear rate.**

    clear rate       = 1221 / 20 / 1440 = 0.0424
    availability gain = 0.0305 - 0.0241 = 0.0064

Only about 15% of cleared cycles convert into a fix inside 25 m.

## The finding that matters for what comes next

**Neither F1 nor F3 is a single-fault case, so the canonical condition for exclusion is
never tested here.** `ca_tbl_causal_sim.cc:2506` selects faulted anchors as
`(anchorId % 97) % 3 == seed % 3` — roughly **one third** of anchors — while its own
comment describes this as "a SINGLE-ANCHOR fault (h = 1), which is the canonical RAIM
model". F3 biases all of them. Exclusion's benefit tracks how close the fault is to one
faulty measurement, and this campaign brackets that condition without ever meeting it.

Not changed: the injector is the model the ledger was measured under, and altering it would
invalidate the corpus. Recorded as the scope limit on this result.

## What this does NOT say

**Claim 9's ledger entry is unmodified.** This is not a revival. `exclude` does not beat
`no_integrity` in any cell, and that was explicitly not predicted and is not scored here.

The correct reading is a **better-characterised falsification**: the response policy is
part of the story on F1 — worth 21% of the gap — and is actively the wrong response under
common-mode faults. Fixing the response is not enough at `R = 25 m`.
