# Results

Every number below comes from a registered campaign in the repository and is **simulation only**. Intervals are 95 %
bootstrap percentile intervals over 20 paired seeds. [The Technical Report](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/CA-TBL_Technical_Report_SelfContained.md)
is authoritative; [`report/GOAL_STATUS.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/report/GOAL_STATUS.md) tracks the requirement campaign by campaign.

## The design requirement

CA-TBL under the preset, PRESET-5 (seeds 341–360):

| configuration | tag 0 A(25 m) | five-node A(25 m) | runs ≥ 0.95 | CEP95, tag 0 | subject energy |
|---|---|---|---|---|---|
| 5 km, 12 gateways + 12 finders | **0.986** [0.982, 0.989] | **0.952** [0.911, 0.975] | 18 of 20 | 4.7 m [3.9, 5.8] | 13.3 J |
| endgame, 12 + 12 | **0.999** | **0.993** | 20 of 20 | 1.4 m | 25.7 J |
| 5 km, brief's 3 gateways + 8 finders | 0.969 [0.955, 0.980] | **0.875** [0.836, 0.912] | 4 of 20 | 75.9 m [7.5, 210.3] | 13.3 J |
| endgame, brief's | 0.997 | 0.982 | | 2.8 m | 17.9 J |

!!! warning "The 5 km margin is modest"
    0.952 on these seeds against 0.975 on DEPLOY-REQ's. The requirement is met on both 20-seed sets and not by a wide
    margin.

## Against the comparators

PRESET-5, CA-TBL minus comparator, adopted deployment. **A difference whose interval spans zero is not a result.**

| comparator | Δ five-node A(25 m), 5 km | Δ five-node A(25 m), endgame | Δ subject J, 5 km | Δ subject J, endgame |
|---|---|---|---|---|
| `fixed_threshold` | −0.016 [−0.054, 0.008] | 0.000 [−0.001, 0.000] | 0.10 [−0.18, 0.48] | 0.64 [−0.06, 1.32] |
| `poucet_fusion` | 0.000 [−0.044, 0.031] | +0.011 [0.003, 0.025] | −443.8 [−444.4, −443.3] | −363.9 [−374.7, −350.7] |
| `static_2p4` | +0.013 [−0.025, 0.044] | +0.001 [−0.000, 0.002] | +7.3 [7.0, 7.6] | +15.5 [14.6, 16.4] |
| `static_868` | **+0.617** [0.556, 0.679] | **+0.240** [0.185, 0.304] | +4.7 [4.5, 5.0] | +15.9 [15.0, 16.7] |

What this says:

- **Level with a flat variance threshold.** The escalation rule is not shown to beat it. The score has no detectable
  effect over a threshold on the trace.
- **Level with POUCET-style fusion on availability, at about 1/34 of its subject energy at 5 km** (13.3 J against
  457.1 J) and about 1/15 in the endgame. That is an advantage in energy, not in accuracy.
- **Level with a static 2.4 GHz arm on availability** on this dense deployment, at higher energy. Static 2.4 GHz comes
  within 0.013 of the requirement at 5 km (0.939 [0.916, 0.957]) with a tag-0 fix share of 0.80 against CA-TBL's 0.998,
  so it holds position by dead reckoning through cycles with no fix. With the brief's 3 gateways it reaches 0.777
  [0.722, 0.827] against CA-TBL's 0.875 [0.836, 0.912].
- **Far above a static 868 MHz arm**, which is below the requirement everywhere (0.335 at 5 km).

## The deployment requirement

DEPLOY-REQ: CA-TBL five-node A(25 m) at 5 km, 20 seeds per cell.

| gateways \ finders | 8 | 12 | 16 | 24 |
|---|---|---|---|---|
| 3 | 0.794 | 0.827 | 0.900 | 0.936 |
| 8 | 0.916 | 0.955 | 0.941 | 0.982 |
| 12 | 0.912 | **0.975** | 0.970 | 0.972 |

Tag 0 alone meets the requirement in all twelve cells (0.956 to 0.992). The limit is therefore **anchor supply** for
the pool, not the filter: with three gateways no finder count up to 24 reaches 0.95. Eight gateways with twelve finders
sat on the line (0.955 here, 0.947 on fresh seeds), so the preset's ring was grown to twelve. The flat-threshold arm
shows the same dependence (0.974 at 12 + 12; 0.819 to 0.950 with three gateways), so this is a finding about the
deployment and not about the escalation rule.

## The integrity check

The Switching-Point Integrity Check (a Mahalanobis test of a fix against the prior; **not** the evidence gate) was
evaluated and **removed from the preset** (DD 19).

| | at the preset's fitted threshold, 1992.9 | re-fitted to the fault-free tail, 9303.8 |
|---|---|---|
| precision, four injected faults | 0.04–0.29 | 0.04–0.29 |
| recall | 0.010–0.074 | 0.003–0.023 |
| mean recall | 0.046 | 0.012 |

The fault-free statistic's 99th percentile is 21 322 at 5 km (nominal $\chi^2_{0.99}(2)=9.21$), so a threshold has to be
fitted to the healthy tail, and then it blinds the test. With no fault present, switching it off changes five-node
A(25 m) by −0.024 [−0.056, 0.006] at 5 km, −0.002 [−0.007, 0.003] in the endgame, and **+0.036 [0.003, 0.069]** on
INTEG3's eight-gateway ring, the one cell whose interval excludes zero. INTEG2's registered rule did not recommend keeping
the test. The switching-point test, which uses other nodes' reports, stays on.

## What is not claimed

- That the escalation rule beats a flat variance threshold.
- That the score carries information beyond the covariance trace.
- That the integrity check pays.
- Any result on hardware.
