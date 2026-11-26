# COLLAPSE-6 — pre-registration (committed before the first cell)

**Why.** An audit for the plan's no-leak rule found a World-to-system leak in every earlier COLLAPSE pipeline
campaign (COLLAPSE-1, -2 stations, -3, -4 and -5 part (a)). Under `--rangeHeightCorrect`, the three sites that
project a range onto the ground passed the nodes' true heights (DD 52):
- a buried worker's height, which encodes its depth;
- a finder's height on the pile.

So the filter and the DD 47 depth solver were given part of the World's geometry. The link-field estimators
(COLLAPSE-2 (c), COLLAPSE-5 (b)) read only `obs_*.csv` and are not affected.

`--collapseNominalHeights=true` replaces those heights with what the system can know:
- surveyed gateway masts;
- the nominal 1.5 m carry height for every carried device.

Exploratory, one seed (921, 8 storeys, central, N = 20): L2 0.60 → 0.55, median CEP50 1.12 → 1.53 m.

**Design.** COLLAPSE-5 part (a) exactly (`scripts/collapse5_run.sh`'s pipe cells, seeds 821–825), plus
`--collapseNominalHeights=true` everywhere. Added are N = 1 cells (`$PIN_ONE_WORKER`, arms
`ca_tbl_full,static_868,static_2p4`). 70 runs. Runner `scripts/collapse6_run.sh`, analyser
`scripts/collapse6_analyse.py`.

## Hypotheses

- **H29:** with the fix, COLLAPSE-5's H17, H18, H19 and H22 keep their verdicts (all held), by the same rules.
- **H30 (descriptive):** L2 with the fix minus L2 with the leak, per configuration, arm and storey count.
- **H31:** the lone worker's L2 with `ca_tbl_full` ≥ the best single radio's − 0.10, at both storey counts.

**Consequence.** The paper's pipeline headline numbers are taken from COLLAPSE-6. COLLAPSE-1 to -5 (a) are kept,
marked as run with the leak.

## Results (recorded 2026-11-22; hypotheses unchanged)

70 runs, all exit 0, binary `1772009c5` (`results/collapse5/collapse6_analysis.md`).

**Analysis fix after the run (disclosed).** `collapse5_analyse.arm_stats` read only `subject_summary.csv`, which
N = 1 runs do not write. It now falls back to `arm_summary.csv`, as `collapse4_analyse.py` does. The measure is
unchanged, and COLLAPSE-5's analysis reproduces exactly with the fix.

- **H30 (effect of the leak, descriptive):** L2 changes by −0.04 to +0.04 in every configuration and arm. The leak
  was real but small. The single exploratory seed's −0.05 was within this spread.
- **H29 fails: 2 of the 4 earlier verdicts change.**
  - H17 (ToF rubble correction): +0.04 at both storey counts (0.81 vs 0.77; 0.67 vs 0.63), short of the
    registered +0.05.
  - H18 (always-on): fails at 5 storeys, where always-on is 0.08 better (0.85 vs 0.77) at 13× the energy. It holds
    at 8 storeys (0.63 vs 0.64).
  - H19 holds at both: 868 MHz alone 0.81 / 0.62 vs ours 0.77 / 0.63.
  - H22 holds: 0.55 vs 0.10.
- **H31 fails at both storey counts:** the lone worker's L2 is 0.60 with ours vs 0.80 with 868 MHz alone (5
  seeds). **Correction (2026-11-23):** the cause was not tested. An earlier version attributed it to
  COLLAPSE-4's unregistered note that the UWB futility gate costs a lone worker its
  occasional UWB fixes.
- **Other values with the fix:**
  - crew of 10: 0.80 / 0.48 (vs 0.77 / 0.63 for 20);
  - two stations: 0.17 / 0.12;
  - Mueller error: ours 0.66 / 0.55.
