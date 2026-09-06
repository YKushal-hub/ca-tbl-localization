# Pre-registration — A5, complementary tier-3 siting

**Committed before the first cell runs.** Rule 13.

**Ordering satisfied.** The work package makes A4-before-A5 non-negotiable, so that A5 does
not answer a question a calibration fix has already answered. A4 is complete
(`report/A4_CALIBRATION_RESULTS.md`) and **no calibration was applied**, so A5 measures
siting against the same filter every other result in the tree used.

## The question

Tier 2 and tier 3 are **co-located** at the four facility sites. That is deliberate: if the
two refinement tiers covered different ground, an escalation failure could be a siting
artefact rather than a decision error. A5 asks whether that deliberate choice **costs
coverage** — whether putting tier 3 in the gaps between tier-2 clusters, at the same radius
from the centre, buys availability without giving up accuracy.

`--sitingMode 1` rotates the facility square 45° for tier 3 only. Tier 2 does not move.
`dep.facilities` does not move, because the walker dwells there and moving it would change
the mobility as well as the siting.

## Design

- `--sitingMode ∈ {0, 1}`, `--seed ∈ {1..5}`, `--gwGrid=9`, `--boxHalf=3000`,
  `--duration=3600`, `--trace=false`. 10 cells.
- **Sparse end deliberately.** A4 established that 96–99.6 % of inadequate fixes occur on
  cycles with fewer than three tier-2 anchors in range. If complementary siting helps
  anywhere it helps there, and a null at the sparse end is a stronger null than a null in
  the middle.
- **Paired by seed.** Mode 0 and mode 1 share the seed, so the walker's trajectory is
  identical and the comparison is within-trajectory.

## Predictions

**P1 — availability improves.** `fixCycles` (and `cyclesInUwbCoverage`) is higher under
mode 1 than mode 0, paired by seed, on the escalating arms.

*Falsifier:* no increase, or a decrease. Reported as a negative result — co-location is
then not costing coverage, and the deliberate choice is vindicated rather than merely
untested.

**P2 — accuracy is not paid for it.** `rmseM` under mode 1 is not worse than mode 0 by more
than the seed-to-seed spread of mode 0.

*Falsifier:* accuracy degrades beyond that spread. Then complementary siting is a trade,
not an improvement, and it must be reported as one — availability bought with error is
exactly the conjunction A0 was written to stop being claimed as a win.

**P3 — the effect is not a single-seed artefact.** The sign of the mode 1 − mode 0
difference in `fixCycles` is the same in at least 4 of 5 seeds.

*Falsifier:* the sign is split 3–2 or worse. Rule 8: a single-seed difference is never a
result, and a 5-seed sign split is not much better.

## Abort conditions

- Any cell failing to produce `arm_summary.csv` → excluded **by name**, not silently
  dropped (rule 12).
- If mode 0 and mode 1 produce byte-identical `arm_summary.csv`, the flag is not wired and
  **nothing is reported** until that is fixed.

## Stated in advance

- **Five seeds is thin.** No magnitude is claimed from it; the claim is a sign and a
  direction, which is what P3 tests. A magnitude needs the 20-seed treatment the baseline
  cells got.
- **This measures siting, not the policy.** The static arms are included because their
  response to siting is the control: `static_uwb` should move most, and if it does not,
  the flag is not doing what the code says it does.
- **One density.** No claim is made about how the effect varies with anchor density.
