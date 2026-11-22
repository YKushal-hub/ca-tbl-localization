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
