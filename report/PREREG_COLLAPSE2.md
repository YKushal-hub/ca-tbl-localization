# COLLAPSE-2 — pre-registration (committed before any of its data)

Sensitivity of COLLAPSE-1's conclusions, in three parts. Sources and flags as in report/PREREG_COLLAPSE1.md.

## (a) Digging (post-processing of COLLAPSE-1; no new runs)

`scripts/collapse2_analyse.py dig <collapse1 dir>`, for N = 20, ca_tbl_full:
- teams 1 (INSARAG-MED) and 2 (INSARAG-HEAVY);
- t_dig 0.5 / 2 / 8 h (DEBRUY-TIMES span; a sweep, since no per-victim source exists);
- dig radius 1 / 2 m;
- policies `spt` and `id`.

**H8.** `spt` ≥ `id` in expected survivors (mean over seeds) in at least 80% of the
(teams × t_dig × radius × storeys × bracket) combinations.

## (b) Number of stations (new runs)

8 storeys, central, N = 20; `--nGateways` 3 and 6 (COLLAPSE-1 has 4); the same ring radius 56.45 m; seeds
801–810, paired with COLLAPSE-1's `s8_central_n20`. Runner: `scripts/collapse2_run.sh <outdir>`.

**H9.** ca_tbl_full's L2 with 3 stations is within 0.15 of its L2 with 4 (mean over seeds). With 6 it is ≥ the
4-station value − 0.05.

## (c) Estimators on the link field (B1 joint, B4 coop, rss, tof)

`--collapseField` (DD 40, preset engines):
- 8 storeys; 20 workers; brackets low / central / high;
- seeds 901–910; 2 rescuers;
- coverage 1.0 and 0.5 (`--spots`);
- local frame.

Generated and estimated by `scripts/collapse2_run.sh <outdir> field`. Scored by
`scripts/collapse2_analyse.py field <outdir>`.

**H10.** joint's within-2 m fraction ≥ rss's and ≥ tof's (mean over seeds) in the low and central brackets at
both coverages.

**H11.** coop's median horizontal error ≤ joint's (mean over seeds of the per-seed median) at coverage 0.5 in
the low and central brackets.

**H12.** The joint estimator's median depth error (central bracket, coverage 1.0) is within 3× the Cramér–Rao
depth bound for a 2 m deep worker with σ = 3 m and the 5 m, two-height grid (0.878 m; DD 42).

Failures are reported as failures.

## Results (recorded 2026-11-22; hypotheses unchanged)

- **(a) H8 holds:** `spt` ≥ `id` in 108 of 108 combinations (`results/collapse1/collapse2_dig_analysis.txt`).
- **(c) Field estimators** (`results/collapse2_field/collapse2_field_analysis.txt`, 10 seeds per bracket):
  - **H10:** 7 of 8 hold. joint ≥ tof everywhere. joint ≥ rss except at low rubble with full coverage
    (0.65 vs 0.66).
  - **H11 holds:** coop's median beats joint's at half coverage (low 2.07 vs 2.41 m; central 2.55 vs 2.69 m).
  - **H12 fails:** joint's median depth error (central, full coverage) is 3.00 m, against 3 × CRB = 2.63 m. Depth
    remains the weak axis; tof has the best depth (1.98 m) but locates only 30%.
- **(b) Stations:** running at the time of recording; result to follow.
