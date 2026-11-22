# COLLAPSE-7 — pre-registration (committed before the first cell)

**Why.** DD 52 left one known read of World truth: DD 48's finder frame solve took the other finders' heights from
the truth. Under `--collapseNominalHeights` it now uses their GNSS heights (true + the fixed GPS-SPS-AVG draw).
COLLAPSE-4's H16 ("the frame solve costs no localisation") was measured with both leaks.

**Design.** COLLAPSE-4's N = 20 cells exactly (5 and 8 storeys × low and central brackets, seeds 811–815), with
configs `gate` and `frame` only, plus `--collapseNominalHeights=true` everywhere. 40 runs. Runner
`scripts/collapse7_run.sh` (derived from `collapse4_run.sh`). Analyser: `scripts/collapse4_analyse.py` unchanged.
Its H14 lines compare `gate` with `base`, which this campaign does not run, and are ignored.

**H32:** `frame` L2 ≥ `gate` L2 − 0.05 (mean over seeds) in every cell. This is H16's rule, re-tested without
the leaks.

## Results (recorded 2026-11-22)

40 runs, all exit 0, binary `11427fd71` (`results/collapse5/collapse7_analysis.md`).

- **H32 holds, 4 of 4:** frame L2 − gate L2 is +0.01, −0.00, −0.01 and +0.03. COLLAPSE-4 had 3 of 4 with the
  leaks.
- **Not hypothesised here, but with no leak:**
  - battery: the 5th-percentile worker lasts 169–187 h (COLLAPSE-4 H13's rule holds);
  - depth: resolved for 31% / 20% of workers at 5 / 8 storeys, median absolute error 0.89 / 0.81 m (H15's rule
    holds).
