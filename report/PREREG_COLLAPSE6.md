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
