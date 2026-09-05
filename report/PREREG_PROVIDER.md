# Pre-registration — the multi-node PHY provider

**Committed before the first measurement run.** Rule 13. The null control below is a
verification and would be run regardless; P2 and P3 are the measurement, and they are
written down here before the binary that produces them has finished linking.

## What changed

`EnginePhyProvider` held a single `m_nodeId` and a single `m_nodePos` and returned early
from `Evaluate()` for any frame with neither endpoint equal to that node. It now carries a
position per field node.

**The engines were already multi-node.** `m_nodeStates` is a map, peer tags were already
`RegisterNode()`d and `UpdateNodePosition()`d on all three engines. The provider was the
only single-node component in the path, and it sits *below* the MAC, so nothing above it
could observe the loss: the peer's frames were counted as sent, lost nothing to the duty
governor, to CAD or to collision, and were never delivered.

This corrects the scope recorded in `report/C4_PROVIDER_BLOCKER.md`, which inferred from
the same measurement that a full per-tag refactor (C2-full) was a precondition. It is not.
The engine-side multi-node support that refactor was assumed to require already exists.

## The flag this sits behind

`--nTags`, which already exists and already defaults to 1. With `nTags=1` the peer map is
empty and `Evaluate()` takes the same branches, in the same order, that it took before.
No new flag is added; per CLAUDE.md §4 the requirement is a default-off flag whose off
state reproduces the baseline byte-for-byte, and `--nTags=1` is that state. P1 tests it.

## Predictions

**P1 — null control.** With `baseline_v7`'s own recorded invocation
(`--seed=7 --duration=3600 --nlosSource=propagation --arbitrateDutyBudget=true
--pcsForm=legacy --trace=true`), all **seven** SHA-256 digests in
`results/baseline_v7/run_id.txt` reproduce exactly: `arm_summary.csv` and all six traces.

*Falsifier:* any one of the seven differs.

**P2 — the provider now evaluates peer links.** At `--nTags=2`, the new binary reports
strictly more provider evaluations (an existing `arm_summary` column) than the pre-change
binary `b8e1b3f` on the identical invocation.

*Falsifier:* evaluations equal or lower.

**P3 — the peer's frames are now delivered.** At `--nTags=2`, channel deliveries
(`macDeliveries`, also an existing column) are strictly higher than the pre-change binary's.

*Falsifier:* no increase. That would mean something ABOVE the provider is also discarding
peer frames, and the provider was not the only blocker — in which case P3 is reported as
a failure and the remaining blocker is found before anything is claimed fixed.

## Abort conditions

- **If P1 fails, P2 and P3 are not reported as results at all.** A change that perturbs the
  baseline makes the before/after comparison meaningless, and reporting the effect anyway
  would be reporting a difference that has two causes. The branch is parked instead.
- If the pre-change binary cannot be run on the identical invocation, there is no before
  and the comparison is abandoned rather than substituted with a different reference.

## Scope limits, stated in advance

- **This is one seed.** Rule 8 forbids treating a single-seed cross-arm difference as a
  result. P2 and P3 are *within-arm, across-binary* at a fixed seed, which is a different
  comparison, and the claim they support is a control-flow claim (rule 4): a path that was
  structurally unreachable is now reachable. **No magnitude is claimed from one seed.**
  Any quantitative statement about peer contention needs a seed sweep and does not appear
  in this pre-registration.
- **Tag-to-tag links remain unevaluable, deliberately.** They are reachable — the engine
  auto-registers an unknown receiver as a gateway with `fineTimestamp=false`, the correct
  hardware class for a tag — but a tag-to-tag range enters the solution as a *relayed*
  measurement, and the relayed-ranging error model that would say how to weight it is the
  written, reviewed precondition of C5 and does not exist. Enabling the link without it
  would produce ranges that look usable and are not attributable.
- **The peer remains unmeasured.** It has no `CaTblPolicy` and no `CaTblEnergyModel`, by
  the existing design: it is an addressing and contention peer, not a second measured
  subject. Making it a measured subject is C2-full and is not in this change.
