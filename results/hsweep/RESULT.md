# hsweep — result

Registered `a3b468e`, before the first cell. 280 cells, `source a3b468e dirty 0`, zero
failures. Estimator: fraction of 1440 cycles with a fix whose `errM ≤ 25 m`.

## Availability, paired by seed

Same seed, same fault set, `none` vs `exclude`. Positive advantage = exclusion loses less.

| h | none gap | exclude gap | advantage | 95% CI | seeds + | verdict |
|---|---|---|---|---|---|---|
| **1** | −0.0028 | **+0.0025** | **+0.0053** | ±0.0051 | 13/20 | **exclusion better** |
| 2 | −0.0081 | −0.0022 | +0.0059 | ±0.0070 | 13/20 | not distinguishable |
| 3 | −0.0136 | −0.0104 | +0.0032 | ±0.0072 | 10/20 | not distinguishable |
| 4 | −0.0153 | −0.0333 | −0.0180 | ±0.0207 | 7/20 | not distinguishable |
| 6 | −0.0255 | −0.0383 | −0.0127 | ±0.0242 | 10/20 | not distinguishable |
| 8 | −0.0301 | −0.0347 | −0.0046 | ±0.0156 | 7/20 | not distinguishable |
| 12 | −0.0540 | −0.0827 | −0.0286 | ±0.0453 | 8/20 | not distinguishable |

**Only `h = 1` resolves.** Every other point's interval contains zero. The per-seed standard
deviation grows from 0.0117 at `h = 1` to 0.1034 at `h = 12` — a 9× spread — so at 20 seeds
the sweep cannot distinguish exclusion from rejection anywhere except the single-fault case.
That is the dominant result and it governs everything below.

## Branch census — clean, monotone, and decisive

| h | trips | cleared | none cleared | ambiguous | too few | MI cycles | MI rate |
|---|---|---|---|---|---|---|---|
| 1 | 942 | **67.8%** | 17.7% | 8.8% | 5.6% | 114 | 0.00396 |
| 2 | 1562 | 60.2% | 26.3% | 9.2% | 4.2% | 238 | 0.00826 |
| 3 | 2362 | 49.6% | 34.0% | 10.7% | 5.7% | 461 | 0.01601 |
| 4 | 3316 | 37.4% | 50.1% | 8.0% | 4.5% | 582 | 0.02021 |
| 6 | 3764 | 27.8% | 57.1% | 9.5% | 5.6% | 695 | 0.02413 |
| 8 | 4169 | 25.4% | 58.8% | 9.9% | 5.9% | 855 | 0.02969 |
| 12 | 6006 | **14.6%** | **74.9%** | 4.9% | 5.6% | 832 | 0.02889 |

`cleared` falls strictly and monotonically, 67.8% → 14.6%. `none cleared` rises strictly,
17.7% → 74.9%. Gate trips rise 942 → 6006. **The mechanism's behaviour is unambiguously
governed by fault multiplicity** — exactly as leave-one-out predicts, since a single
exclusion can only restore consistency when a single measurement is faulty.

## Predictions, scored

**1 — HELD, marginally.** At `h = 1` exclusion beats rejection, +0.0053 with the interval
excluding zero. But the lower bound is **+0.0002** and only 13/20 seeds are positive. The
registration said "by a clear margin"; this is a real effect at the edge of resolution, not
a clear margin. Reported as held in direction, marginal in magnitude.

**2 — SPLIT.** The zero crossing falls between `h = 3` and `h = 4`, inside the registered
`2 ≤ h ≤ 4` band. **HELD.** Monotonicity **fails** as a point estimate — the advantage rises
at 1→2, 4→6 and 6→8 — but all three rises have overlapping confidence intervals, so
non-monotonicity is **not established**. Registered falsifier ("a non-monotonic curve ⇒
something other than multiplicity governs") **does not fire**: the data cannot resolve
monotonicity either way.

**3 — MISSED, and it withdraws a previous claim.** Across the losing region the correlation
between harm and MI rate is **r = −0.083** — no relationship. At `h = 8` the MI rate is
highest (0.02969) while the harm is lowest (0.0046).

> **WITHDRAWN:** `excl_test/RESULT.md` claimed F3's harm *is* the spuriously-cleared
> cycles, on the strength of 0.0305 spurious clears matching 0.0287 availability harm
> within 6%. That agreement was **one point**, and it does not generalise. The registered
> falsifier for prediction 3 required withdrawing it if harm did not track spurious clears.
> It does not. The mechanism remains *plausible* — a cleared-but-biased subset is real, and
> the MI rate measures it — but the quantitative claim that it accounts for the harm is
> withdrawn.

**4 — HELD cleanly.** The census shifts exactly as registered: cleared high at `h = 1` and
falling monotonically, none-cleared low at `h = 1` and rising to F3's 74.9%.

## Misleading information — measured, not yet acted on

MI rate rises 0.00396 → 0.02969, roughly 7.5× from `h = 1` to `h = 8`. These are fixes that
**passed their own integrity check and were outside the 25 m requirement**. A binary
accept/reject cannot express this: the system reports them as good.

Measured here deliberately, as the registration said. **Item D's criterion is not
registered in this pass.**

## The ledger's question, reported but not scored

Registered as not predicted and not scored: whether `exclude` beats `no_integrity`. At
`h = 1` the exclude gap is **+0.0025 ± 0.0055** — positive in point estimate, **not
distinguishable from zero**. So even in the single-fault case, and even with the response
policy corrected, **`full` does not demonstrably beat `no_integrity`.** Claim 9's
falsification is untouched.
