# hsweep — fault-multiplicity sweep, pre-registration

Written before any cell. Committed so the timestamp precedes the first result.

## Why

`excl_test` found that exclusion helps at one fault model and harms at the other, making
**fault multiplicity `h`** the governing variable. Reading the injector then showed the
canonical single-fault condition is never tested: `ca_tbl_causal_sim.cc:2506` faults
**4 of 12** tier-2 anchors while its comment claims `h = 1`, and F3 faults all 12.

This sweeps `h` across its full range. **It is a characterization, not a rescue.**

## Design

    h        1, 2, 3, 4, 6, 8, 12          (12 = every tier-2 anchor)
    policies none, exclude
    arms     ca_tbl_full, ca_tbl_no_integrity
    seeds    20                            -> 280 cells

All other flags identical to `c9_clean` / `excl_test`. `--faultMultiplicity=h` faults
exactly `h` anchors, chosen by a seed-mixed hash so the choice is deterministic and
**identical across arms** — the ablation must see the same fault or it measures fault
placement rather than the detector.

**The existing F1 and F3 results are not replaced.** They become points on this curve at
`h = 4` and `h = 12`, and the curve should explain them.

## Registered predictions

| # | prediction | falsifier |
|---|---|---|
| 1 | At **`h = 1`**, `exclude` beats `none` on availability by a clear margin | no better at `h = 1` ⇒ the response policy is not the mechanism's problem at all, and item B must not be built on this hypothesis |
| 2 | The advantage **degrades monotonically with `h`** and crosses zero somewhere in **2 ≤ h ≤ 4** | a non-monotonic curve ⇒ something other than multiplicity governs, and it must be found before B |
| 3 | Above the crossing `exclude` is worse, and the harm **tracks the spurious-clear rate** — as at F3, where 0.0305 spurious clears matched 0.0287 availability harm within 6% | harm not tracking spurious clears ⇒ the F3 mechanism in `excl_test/RESULT.md` was coincidence and needs withdrawing |
| 4 | The **branch census** shifts with `h`: "cleared" high at `h = 1` and falling, "none cleared" low at `h = 1` and rising toward F3's 74.9% | — |

**Not predicted, not scored:** whether `exclude` beats `no_integrity` at any `h`. That is
the ledger's question; predicting it would be predicting the conclusion.

## Reported at every h, including where exclusion loses

**Misleading-information rate** — a fix that passed its own integrity check while being
wrong:

    MI rate = (cycles where exclusion cleared the gate AND true error > 25 m) / 1440

Counted in the simulator, which holds ground truth; the policy cannot. This is the failure
mode a binary accept/reject cannot express, and it is the measurement that will motivate a
protection level. **Item D's criterion is NOT registered here** — this pass measures the
quantity only.

## Abort conditions

- degeneracy pre-flight, mechanism `integrity`, at `h = 1`: must be **LIVE**. Measured
  before registration: 16 suspects, 6 fields differing — LIVE.
- any sidecar not recording `dirty 0`
- the byte-identity null control failing

## What this does not license

Claim 9's verdict is untouched. `exclude` beating `none` at `h = 1` says nothing about
whether either beats `no_integrity`. A losing region is part of the finding and is reported.
