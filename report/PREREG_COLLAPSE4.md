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

## Results (recorded 2026-11-22; hypotheses unchanged)

120 runs, all exit 0, binary `9702c84cd` (`results/collapse4/collapse4_analysis.md`, sidecars in
`results/collapse4/runs/`).

| Hypothesis | Verdict | Detail |
|---|---|---|
| H13 (battery) | **8 of 8 hold** | N = 20: 5th-percentile worker 169–193 h with the gate, against 36–46 h without (base). N = 1: median 275–291 h, against 25–59 h |
| H14 (gate costs no localisation) | **4 of 4 hold** | gate L2 − base L2: +0.01 to +0.04 |
| H15 (depth) | **holds** for 5 and 8 storeys | resolved 135/600 (23%) and 102/600 (17%); median absolute depth error 0.74 m for both |
| H16 (finder frame costs no localisation) | **3 of 4 hold** | fails at 5 storeys, low bracket (−0.13); elsewhere −0.02 to +0.00 |

**Not hypothesised, reported.** For the lone worker (N = 1, 5 seeds per cell) the gate's L2 differs from base by
−0.40 to +0.00 (s5 central 0.80 → 0.40, s8 low 0.80 → 0.40). Five seeds are few, but the direction is consistent:
a lone worker loses the UWB fixes that occasionally do get through. The crew results (H14) show no such cost.
