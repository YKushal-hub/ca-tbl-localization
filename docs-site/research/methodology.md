# Methodology and baselines

## Question

Does a decision layer that escalates between radio tiers earn its complexity against simply pinning the radio to one
tier, or against fusing every radio at every cycle? The brief fixes the requirement in advance:
**A(25 m) ≥ 0.95**.

## Metrics

| metric | definition |
|---|---|
| **A(25 m)** | the share of **all** decision cycles on which a node's position estimate is within 25 m of the truth. A cycle without a fix counts by the estimate the filter holds then, which is what a searcher would be sent to; the denominator is every cycle |
| tag 0 / five-node | tag 0 is the lost node; the five-node pool is tag 0 and four peer subjects, pooled cycle by cycle |
| CEP95 | the radius containing tag 0's error on 95 % of cycles |
| subject energy | mean total radio energy of a subject over the run, joules |
| fix share | the fraction of cycles with a fix. **Not** the requirement; earlier campaigns reported it as "availability" |

Time to first fix and time to convergence are **not** measured.

## Arms

| arm | definition |
|---|---|
| `ca_tbl_full` | the system |
| `fixed_threshold` | escalate when `tr P > 12` m²: a flat variance threshold in place of the score |
| `poucet_fusion` | every radio on, each cycle's ranges from all three tiers fused, no switching, every tier billed; the posture of the closest fielded system |
| `static_2p4` | tier 2 only, no floor |
| `static_868` | tier 1 only, same filter |

All arms share the preset's world, filter and deployment. The static arms are the control for tiering;
`fixed_threshold` differs from the full system only in what triggers escalation, which makes it the control for the
[uncertainty score](../theory/uncertainty-score.md).

## Campaigns

| campaign | seeds | cells | purpose | registration |
|---|---|---|---|---|
| PRESET-5 | 341–360 | 80 | the adopted preset against the brief's deployment, five arms, 5 km and endgame | [`report/PREREG_PRESET5.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/report/PREREG_PRESET5.md) |
| DEPLOY-REQ | 301–320 | 240 | gateways {3, 8, 12} × finders {8, 12, 16, 24} at 5 km | [`report/PREREG_DEPLOY_REQ.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/report/PREREG_DEPLOY_REQ.md) |
| INTEG2 | | | the integrity check on injected faults | [`results/integ2_2026-10-18`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/results/integ2_2026-10-18) |
| INTEG3 | 181–200 | | the integrity check, fault-free | [`results/integ3_2026-10-20`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/results/integ3_2026-10-20) |

All cells run 1800 s (720 cycles of 2.5 s), on one frozen binary, from a clean tree. Campaigns are not pooled.

## Statistics

Each comparison is paired by seed across 20 seeds. The interval is a 95 % bootstrap percentile interval of the mean
paired difference with 10 000 resamples. Predictions, falsifiers and abort conditions are committed before the first
cell runs.

## What a result is

A count, rate or sum is not an observable until its denominator, observation window and rule for observations
truncated by that window are all stated. A single-seed cross-arm difference is never a result. An exact zero is a
control-flow claim, not a measurement, and the code path must be proven reachable before it is reported. Every gate
has a negative fixture that has been observed to fail.

## Scenario

A **search**, not a monitored enclosure. Twelve surveyed gateways on a 3 km ring and twelve finders in pairs; five
subjects whose filters start at the box centre. *5 km* is the full box (half-width 5 km); *endgame* is the last stretch,
a 750 m half-width box with the finders converging directly, where UWB is in reach. The brief's deployment (3 gateways,
8 finders) is kept as a pinned-back variant so that every result has a before and an after.
