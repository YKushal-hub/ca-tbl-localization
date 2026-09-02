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

---

# RESULTS — both conclusions are INVARIANT to terrain across its full range

Computed after the registration above was committed (`e3c5339`).

## §5.1 — the range-precision frontier

The original registration specified `y` as "achievable ranging sigma from the CRLB" but
never fixed the **operating point** at which sigma is evaluated. That ambiguity changes the
answer, so both readings are reported rather than one being chosen after the fact.

| | terrain 0 dB | terrain 30 dB | invariant? |
|---|---|---|---|
| **Metric A** — sigma at half each design's own achievable range | slope **+2.2000**, frontier 8 pts, falsifier **0** | slope **+2.2000**, frontier 8 pts, falsifier **0** | **YES, exactly** |
| **Metric B** — sigma at a fixed 100 m | slope **+1.1000**, frontier 8 pts, falsifier **3** | slope **+1.1000**, frontier 8 pts, falsifier **3** | **YES, exactly** |

Tier positions move exactly as predicted — every achievable range falls by the same factor
`10^(30/(10n)) = 23.1013`, verified identical for all 56 grid points — while the frontier's
**shape is unchanged to four decimals under both metrics**. The registered prediction
holds: a uniform offset in dB shifts `log10(range)` by a constant and cannot change a
log-log slope.

**The falsifier verdict is the same at both limits under both metrics**, which is the
question that was asked. §5.1's conclusion, whichever metric is adopted, does not depend on
terrain.

### But the falsifier's verdict depends on the metric, and that is a separate finding

Under **Metric A** no `(f, BW)` beats all three tiers on both axes, at either limit. The
architecture's premise survives.

Under **Metric B** three points do, at both limits: **169 MHz at 250 / 500 / 800 kHz**.
These are physically realisable — `BW/f ≈ 0.005`, nothing like a bandwidth exceeding its
carrier — so there is no realisability escape. At 100 m a 169 MHz link has so much surplus
budget that its CRLB, which scales as `1/(BW·sqrt(SNR))`, beats UWB's despite 600× less
bandwidth.

The premise "no point is simultaneously long-range and precise" therefore holds when each
design is judged **at its own operating point**, and fails when all designs are judged **at
a common short distance**. That is a real limitation of the premise and it is reported as
one. It is *not* a terrain finding — it fires identically at 0 and 30 dB. It is also
subject to the CRLB's own limits, which ignore multipath and clock stability, the two
things that actually floor low-bandwidth ranging in practice.

## §5.2 — NLOS as bias versus as noise

| | terrain 0 dB | terrain 30 dB |
|---|---|---|
| model A, error ratio n=1 → n=1000 | **31.6×** (= sqrt(1000)) | **31.6×** |
| model B, error ratio n=1 → n=1000 | 1.82–1.83× | 1.85–3.03× |
| model B floor | **7.5000 m** | **7.5000 m** |

**Exactly invariant, as registered.** Model A falls without bound as `1/sqrt(n)` at both
limits on all three tiers; model B flattens at a floor of `P_nlos × bias = 0.30 × 25 =
7.5000 m`, identical to four decimals at both limits and independent of tier.

The registered reasoning was correct and is worth keeping because it is what made the
prediction falsifiable: terrain changes SNR and therefore the CRLB magnitude — 868's model
A at n=1 goes from 0.958 m to 30.3 m — but it cannot make a zero-mean error non-zero-mean.
The floor is `P × bias`, which contains no sigma at all.

## Verdict

**Both conclusions are invariant to terrain across its full physical range, tested at both
limits.** The sweeps are unblocked with respect to terrain, and every result they produce
may carry that sentence.

This does **not** mean the model is validated. G11b is permanently red and G11a fails on
two of three tiers. It means these two conclusions do not rest on the part that cannot be
validated — a claim about robustness, not calibration.

---

# §5.1's falsifier: allocation check on the winning points

The falsifier fired under metric B. Before the verdict lands, the check it had not had:
`BW/f ≈ 0.005` establishes that the **fractional bandwidth is achievable**. It does not
establish that **the spectrum exists**, and only the second claim matters for a falsifier
about a deployable architecture.

## The winning points, checked against real allocations

All three winners sit at **169 MHz**, needing 250, 500 or 800 kHz.

| region | instrument | channel cap near 169 MHz |
|---|---|---|
| CEPT / EU | ECC/DEC/(05)02, harmonised plan for 169.4–169.8125 MHz; 100 kHz spacing subdividable into 50 or 25 kHz | **50 kHz** |
| US | FCC narrowbanding, Part 90, 150–174 MHz, mandatory since 1 Jan 2013 | **12.5 kHz** |

**The whole band is 412.5 kHz wide** (169.8125 − 169.4). So 500 kHz and 800 kHz are wider
than the entire allocation, before any channel rule is applied.

| winning point | vs CEPT 50 kHz cap | vs FCC 12.5 kHz cap | vs 412.5 kHz band |
|---|---|---|---|
| 169 MHz, 250 kHz | **5× over** | 20× over | fits the band, not a channel |
| 169 MHz, 500 kHz | **10× over** | 40× over | **wider than the band** |
| 169 MHz, 800 kHz | **16× over** | 64× over | **~2× the whole band** |

**Allocatable winners: 0**, at terrain 0 dB and at 30 dB.

Power is not the binding constraint — the modelled 12.5 dBm (18 mW) sits well under the
band's SRD limits — and neither is antenna size on its own, though it is not free: λ/4 at
169 MHz is **0.443 m**, against 0.086 m at 868 MHz, 0.031 m at 2.44 GHz and **0.009 m** at
7.99 GHz. A 48× longer element on a body-worn or airborne node. **Bandwidth is what makes
these points undeployable, and it is not close.**

## Registered consequence applied: outcome 1

**The premise survives, with its constraint named:**

> **No *deployable* radio is simultaneously long-range and precise.**

And, reported alongside it as the registered consequence requires:

> **The unconstrained premise is false.** A `(f, BW)` pair that beats all three tiers on
> both axes does exist in physics — 169 MHz at 250–800 kHz. What forbids it is spectrum
> policy, not the range-precision trade-off.

That is a stronger and more honest claim than the unqualified version, because it names the
reason. It also transfers: in a jurisdiction that allocated 800 kHz at 169 MHz, the
architecture's premise would not hold, and the tiering would need a different
justification.

## The gap in the original registration, recorded rather than resolved after the fact

The original §5.1 registration specified `y` as "achievable ranging sigma from the CRLB"
and **never fixed the operating point at which sigma is evaluated**. That silence decides
the verdict: metric A (each design at its own operating point) finds no winner; metric B (a
common 100 m) finds three. Both are reported. **The metric that saves the premise was not
selected** — the premise is saved by the allocation check, which is independent of the
metric and holds under both.

Two further limits on metric B's winners, stated so they are not mistaken for the reason
they were rejected: the CRLB ignores multipath and clock stability, which are what actually
floor low-bandwidth ranging in practice, so the 0.04 m sigma at 169 MHz is optimistic
independently of allocation.
