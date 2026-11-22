# COLLAPSE-4 — pre-registration (committed before the first cell)

**Question.** Do the three mechanisms built after COLLAPSE-1 do what they were built for, in the full pipeline,
without costing localisation?
- DD 46: the UWB futility gate;
- DD 47: depth (B2);
- DD 48: the finder frame (B3).

**Design.** COLLAPSE-1's flags and geometry (report/PREREG_COLLAPSE1.md) at 5 and 8 storeys, low and central
brackets, N = 1 and 20, arm `ca_tbl_full`, 1800 s, seeds 811–815 (new). `--collapseDepthSolve=true` in every run
(output only). Three configurations:
- `base`: COLLAPSE-1's flags;
- `gate`: + `--uwbFutilityCycles=4`;
- `frame`: + `--uwbFutilityCycles=4 --finderFrameSolve=true`.

120 runs. Runner `scripts/collapse4_run.sh`, analyser `scripts/collapse4_analyse.py` (both committed with this
file). Battery: one CR123A, 16,740 J, plus Phase 0 compute 1.82 J/h (as COLLAPSE-3).

## Hypotheses

- **H13 (battery):** with `gate`, the 5th-percentile worker lasts ≥ 72 h in every N = 20 cell, and the lone worker's
  median lasts ≥ 72 h in every N = 1 cell.
- **H14 (no localisation cost):** `gate` L2 ≥ `base` L2 − 0.05 (mean over seeds) in every N = 20 cell.
- **H15 (depth, B2):** pooled over each storey count's N = 20 runs, the median absolute depth error of the resolved
  workers is ≤ 1.0 m, and ≥ 10% of workers are resolved.
- **H16 (finder frame, B3):** `frame` L2 ≥ `gate` L2 − 0.05 (mean over seeds) in every N = 20 cell.

Failures are reported as failures.
