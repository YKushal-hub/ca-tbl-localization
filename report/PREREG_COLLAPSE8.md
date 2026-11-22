# COLLAPSE-8 — pre-registration (committed before the first cell)

**Why.** COLLAPSE-6 H31 failed: a lone worker does worse with CA-TBL than with 868 MHz alone (L2 0.60 vs 0.80).

Exploratory pilot (seeds 931–935, 5 and 8 storeys, central; not evidence):
- capping the UWB veto backoff for a lone device (DD 53, `--uwbFutilityLoneMaxBackoffS`) and the ToF rubble
  correction do not close the gap;
- the cause is the 2.4 GHz time-of-flight ranges fused every cycle by `--loraDualBase`, and the escalations;
- **DD 53b (`--loneStay868`)** fixes the pilot. A device that hears no other worker on 868 MHz (its own neighbour
  table; radio only) does not fuse 2.4 GHz while on 868 MHz, and with ≥ 3 868 MHz ranges does not escalate. L2
  went 0.60 → 0.80 (5 storeys) and 0.60 → 1.00 (8 storeys), and energy halved.

**Design.** COLLAPSE-4's geometry. Common flags:
- `--uwbFutilityCycles=4 --collapseNominalHeights=true --collapseDepthSolve=true`, and DD 41 on (the default);
- 5 and 8 storeys, low and central brackets;
- N = 1 (arms `ca_tbl_full,static_868`) and N = 20 (`ca_tbl_full`);
- configs `base` and `lone` (+ `--loneStay868=true`);
- seeds 841–850 (new), 1800 s. 160 runs.

Runner `scripts/collapse8_run.sh`, analyser `scripts/collapse8_analyse.py`.

## Hypotheses (L2 is the mean over seeds; battery is the median hours on one CR123A)

- **H33:** N = 1, `lone` L2 ≥ 868 MHz alone − 0.10 in every cell.
- **H34:** N = 1, `lone` L2 ≥ `base` L2 in every cell.
- **H35:** N = 20, |`lone` − `base`| ≤ 0.05 in every cell. A crew hears its members, so the rule should not fire.
- **H36:** N = 1, `lone` median battery ≥ 72 h in every cell.

## Results (recorded 2026-11-22; hypotheses unchanged)

160 runs, all exit 0, binary `2665cdcdf` (`results/collapse5/collapse8_analysis.md`).

| Cell | N = 1 lone | N = 1 base | 868 MHz alone | N = 20 lone / base | lone battery |
|---|---|---|---|---|---|
| 5 storeys, low | 0.70 | 0.80 | 0.60 | 0.72 / 0.72 | 433 h |
| 5 storeys, central | 0.90 | 0.90 | 0.90 | 0.81 / 0.81 | 375 h |
| 8 storeys, low | 0.90 | 0.70 | 0.80 | 0.83 / 0.83 | 510 h |
| 8 storeys, central | 0.70 | 0.60 | 0.70 | 0.74 / 0.73 | 261 h |

- **H33 holds, 4 of 4:** the lone worker is at or above 868 MHz alone in every cell. COLLAPSE-6's H31 gap is closed.
- **H34 holds in 3 of 4:** it fails at 5 storeys, low bracket (0.70 vs 0.80).
- **H35 holds, 4 of 4:** the crew is unchanged (within 0.01). The rule does not fire for a crew, as designed.
- **H36 holds, 4 of 4:** 261–510 h, against 225–280 h for base.
