# Pre-registration — A0: a joint criterion over availability and energy

Written 2026-09-05. Source commit: `3c3c5f4`.
Work package: A0, `docs/WORK_PACKAGES.md`. Specified but never written at
`CA-TBL_Technical_Report_SelfContained.md` §8.1.

**Status of the data this will be applied to: `results/p10_grid` does not exist in this
repository.** It is cited as the source of two headline results and is absent from the
tree. It is being regenerated. This criterion is therefore written **before the data it
will score exists on this machine** — which is a stronger position than §8.1 asks for,
and the reason to write it now rather than after the regeneration finishes.

---

## 0. What is being fixed

The project registered a **cost** criterion (M1) for an architecture whose value
proposition is **reliability**. §7 demonstrates rather than argues the consequence: the
screening rule selects cells where the single-band arm **fails**, while the verdict rule
requires the single-band arm to **qualify**, so the two carry disjoint cell sets. All ten
adjudicable cells sat in `RURAL_OPEN`, the easiest environment — the one where tiering is
least needed — because only there does a single-band arm also qualify.

M1's structural defect, stated exactly: **when the competitor meets the requirement in
zero deployments, M1 returns INCONCLUSIVE by its own text.** An architecture that rescues
deployments nothing else can serve scores no better than one that does nothing. That is
the case this criterion must be able to decide.

---

## 1. Definitions — taken from the existing analysis, not invented

Operationalised exactly as `scratch/python_handoff/analyse_grid_b1.py` already does, so
that a verdict under this criterion and a verdict under M1 are computed from identical
quantities:

    A, R, CYC = 0.95, 25.0, 1440

**Availability of an arm in one deployment.**

    A(arm, seed) = |{ cycles : hadFix AND errM <= 25.0 }| / 1440

Read from `trace_<arm>_seed<N>.csv`. The denominator is **all 1440 decision cycles**; a
cycle with no fix counts against. A cycle truncated by run end still settles the
threshold it has already crossed and is not recounted symmetrically. (`CLAUDE.md` rules
1 and 2.)

**A deployment is met** iff `A(arm, seed) >= 0.95`. One deployment = one seed at one
configuration `c = (envFacility, nlosSource, dwell, boxHalf)`.

**Energy** is `totalJ` from `arm_summary.csv`, paired within a seed.

---

## 2. The criterion (M2)

For configuration `c` over `N` paired seeds, tiered arm `T` against baseline `B`:

    dD(c)  = D_T(c) - D_B(c)            additional deployments met, D = count over N seeds
    rho(c) = geometric mean over seeds of  totalJ_T / totalJ_B

`rho` is the geometric mean of per-seed paired ratios with a seed-level bootstrap 95 % CI
at 10 000 resamples — the estimator already used by `gci()` in `analyse_grid_b1.py` and by
`scripts/sweeps/paired_stats.py`. A ratio's sampling distribution is skewed; an arithmetic
mean of raw ratios is not the estimator to quote.

**The allowance.**

    allow(c) = min( 2.0 , 1 + kappa * dD(c)/N )        kappa = 1.0

**Verdict.**

| outcome | condition |
|---|---|
| **PASS** | `dD > 0` **and** the bootstrap CI for `rho` lies **entirely below** `allow(c)` |
| **FAIL** | `dD <= 0`, **or** the CI for `rho` lies entirely **above** `allow(c)` |
| **INCONCLUSIVE** | the CI straddles `allow(c)` |

Requiring the whole CI below the allowance mirrors M1's requirement that its CI lie
entirely below 1.0. The burden stays on the tiered arm.

**What this fixes, and it is the entire point.** `rho` and `dD` are both defined when
`D_B = 0`. A configuration where the baseline meets nothing and the tiered arm meets most
deployments returns a **verdict**, where M1 returns INCONCLUSIVE.

---

## 3. Why kappa = 1.0 and why the cap is 2.0

Both are fixed from the deployment argument, not from the data.

**kappa = 1.0 is the linear trade:** energy may rise by at most the fraction of the
deployment pool converted from failing to meeting. Rescue 40 % of deployments, spend at
most 40 % more energy. It needs no calibration constant and no appeal to a measured value.

**The 2.0 cap is an endurance boundary.** Doubling energy halves endurance on a fixed
battery. That is where a mission-length tag needs a second battery or a mid-mission
recharge, which changes the deployment's logistics rather than its energy budget. Beyond
that point an energy multiplier is not a trade the operator can make, whatever it buys.
The cap binds only when `dD/N > 1.0`, which cannot occur — so it is a stated ceiling, not
an active constraint at `kappa = 1.0`. It is registered because `kappa` may be varied in
§4 and the ceiling must not move with it.

---

## 4. Sensitivity, reported always — not on request

The verdict at `kappa = 1.0` is **the** registered verdict. Alongside it, every table
reports the verdict at `kappa in {0.25, 0.5, 1.0, 2.0}`, so that any dependence of a
conclusion on the choice of `kappa` is visible in the same table rather than discoverable
later. A cell whose verdict is identical across the band is robust to it; a cell that
flips must be reported as flipping, with the crossing value named.

This is the same discipline as recomputing every conclusion under ±4 dB of margin offset
to bracket the two open propagation residuals: a conclusion that survives its own
sensitivity band is worth more than one asserted at a point.

---

## 5. Disclosure — what was already known when this was written

Pre-registration is worthless if written after seeing which cells pass, so the honest
statement of what was already visible:

**One cell's numbers were public in the technical report before this file existed.**
§4.1 records `ca_tbl_full` **19/20** against `static_2p4` **0/20** deployments at
`RURAL_FOREST`, `dwell=800`, `boxHalf=2000`, at **1.56×** the energy.

Applying the criterion above to those published numbers by hand: `dD/N = 0.95`, so
`allow = 1.95`, and `rho = 1.56` sits below it. **That cell passes at `kappa = 1.0`, and
it fails at `kappa = 0.5`, where the allowance is 1.475.** The crossing lies between
those two values.

Two consequences, both binding:

1. **For that configuration the criterion is not blind, and its verdict must be reported
   as such** — "criterion known to post-date the observation" — not as a clean
   pre-registered PASS. Nothing recovers blindness once the numbers have been read.
2. For the other configurations of the grid the criterion **is** blind, because
   `results/p10_grid` is not in this repository and had not been regenerated when this was
   written. That is checkable: the regeneration commit necessarily post-dates this file's
   commit.

The `kappa` band in §4 exists because of this disclosure. The headline cell's verdict is
`kappa`-sensitive in the range a reasonable person might choose, and hiding that behind a
single registered value would be the exact failure this document is meant to prevent.

---

## 6. Abort conditions

- If the regenerated grid does not reproduce the census `analyse_grid_b1.py` expects —
  6 traces, 6 summary rows and 7 verified digests per cell — the affected cells are
  **named and excluded**, not silently dropped, and the exclusion count is reported beside
  every verdict.
- If regeneration produces a grid whose §4.1 configuration does **not** show 19/20 against
  0/20, the regeneration has not reproduced the cited campaign. The criterion is not
  applied to it, and the discrepancy is reported as a provenance finding about the
  original result.

---

## 7. What this criterion cannot establish

- It scores a **trade**, not a mechanism. A PASS says the energy premium bought enough
  additional deployments to be worth paying at the registered rate. It says nothing about
  *why*, and nothing about whether the confidence score contributed — `ca_tbl_full` and
  `fixed_threshold` can both pass it.
- `kappa` is an engineering choice, labelled as one. It is not measured, not derived from
  any datasheet, and not cited. §4's band is the mitigation, not a defence.
- It inherits every limitation of the availability definition it borrows: one accuracy
  threshold (25 m), one availability threshold (0.95), and a per-cycle predicate that
  treats a 26 m fix and a missing fix identically.
- The regenerated grid will carry **different `run_id.txt` provenance** from the campaign
  the technical report cites. It answers A0; it does not restore the original evidence.
