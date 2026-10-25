# Contributing and roadmap

Read [`RULES.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/RULES.md) before changing anything: every rule in it is load-bearing, and violating one does not
produce a compile error, it produces a wrong result that looks right. [`CONTRIBUTING.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/CONTRIBUTING.md) has the
process.

## Rules that bite

1. **Atomic commits**: one change per commit, revertible in isolation, evidence in the message.
2. **Default-off flag** for every new mechanism, byte-identical when off.
3. **Pre-register** predictions, falsifiers and abort conditions in a `PREREG_*.md` committed before the first cell.
4. **A gate and its negative fixture** in the same commit.
5. **Do not regenerate a tracked results directory in place**, edit `run_id.txt` sidecars, or track `trace_*.csv`.
6. **Do not set `hopWeight > 0`** until relayed measurements exist: the poll set is one-hop, so the hop count is
   always 0, and a non-zero weight would degrade the score for an energy defect rather than a provenance one.
7. **Do not "fix" the score's saturation casually.** It is recorded, not corrected, because a fix changes the engine and
   invalidates every result in the tree. It is a planned campaign, not a bug fix.
8. **Keep the fault model environmental**; adversarial framing belongs only in a citation to that literature.
9. **Where a constant is ours rather than cited, the source comment must say so.**

## Roadmap

Everything in the work-package list is complete. What is open (`docs/WORK_PACKAGES.md`):

| item | what |
|---|---|
| **T45 field test** | a same-site hardware test of the receiver models |
| **LIMIT rows** T22, T25, T35, T37 | measured limits or uncited constants with no known fix |
| **SCOPE T43** | a self-deploying mesh, outside the brief's deployment |
| thin 5 km margin | 0.952; improving it needs a registered campaign |
| **G11, G93** | expected reds, each by owner decision |

!!! note
    This list is a snapshot from the repository's own status documents; the authoritative current list is
    [`docs/TECH_DEBT_STATUS.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/docs/TECH_DEBT_STATUS.md).
