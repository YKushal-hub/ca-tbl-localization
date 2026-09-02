# Terrain sensitivity of §5.1 and §5.2 — registration, written before computing

The verifiable timestamp is this file's commit, which precedes any result.

## Why this replaces reconciliation as the blocking condition

The sweeps have been blocked on §7 reconciliation for four passes. Reconciliation is now
known to be **unreachable** (`DEFECTS.md`, "THE TERMINUS"), so blocking on it blocks
forever. The condition that actually matters is different and is answerable:

> **Do §5.1's and §5.2's conclusions depend on terrain at all?**

Terrain is a scenario-fixed loss, non-negative by construction and saturating at 30 dB
(`lora-physics-engine.cc:400`). Its *value* is unvalidatable, but its *range* is known
exactly: **[0, 30] dB**. A conclusion invariant across that whole range does not depend on
the unvalidated term, and can be reported with the unvalidated term named.

This is a claim about **robustness**, not calibration — and it is a stronger claim than a
fully reconciled model would support, because it holds for every value terrain could take
rather than for one fitted value.

## How terrain is applied

As a uniform extra loss added to the link budget at both limits, **0 dB and 30 dB**. It is
never implemented, never fitted, and never taken from the residual. Both limits are
reported, overlaid, and every number carries the label
**unvalidated-by-construction** wherever terrain enters it.

## §5.1 — the range-precision frontier

Swept jointly over frequency and bandwidth. `x` = achievable range at a fixed link budget;
`y` = achievable ranging sigma from the CRLB.

**Registered prediction:** the frontier's *position* moves with terrain — range falls at
30 dB, since 30 dB of extra loss must cost range. Its **shape is invariant**: the
`range × precision` trade-off exponent is set by the physics of path loss versus CRLB
bandwidth scaling, and a uniform offset in dB cannot change an exponent.

**Registered falsifier, unchanged from the original registration:** a single `(f, BW)`
that beats all three CA-TBL tiers simultaneously on *both* axes falsifies the
architecture's premise. **It is checked separately at 0 dB and at 30 dB.** If it fires at
one limit and not the other, the premise depends on an unvalidated term and the §5.1
conclusion is **undeterminable by this module** — which is itself a reportable result.

## §5.2 — NLOS as bias versus as noise

Model A treats NLOS excess delay as inflated sigma; model B as a positive bias. Position
error is swept against anchor count under both.

**Registered prediction: INVARIANT to terrain, exactly.** The reasoning, stated before
measuring so it can be wrong: terrain is a *loss*, and the bias/noise distinction is a
property of the *error distribution's shape* — whether the mean of the ranging error is
zero. A uniform loss changes SNR, hence the CRLB floor, hence the magnitude of both
curves; it cannot make a zero-mean error non-zero-mean or the reverse. So model A should
still fall without bound and model B should still flatten at a floor, at both limits.

**Falsifier:** if the A-versus-B distinction changes character between 0 and 30 dB — one
of them changing whether it flattens — the prediction is wrong and §5.2's conclusion
depends on terrain.

## What a pass licenses, and what it does not

If both are invariant: the sweeps run, and every result carries "invariant to terrain
across its full physical range, tested at both limits". This does **not** mean the model
is validated — G11b is still permanently red and G11a fails on two of three tiers. It
means these two conclusions do not rest on the part that cannot be validated.
