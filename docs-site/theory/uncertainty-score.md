# The uncertainty score

Called the *position-confidence score* (**PCS**) in the code (`pcsForm`, `pcsW1..3`, `pcsThr*`) and in older
documents. This page uses *covariance-based uncertainty score* because that is what the measurements show it to be.

## Definition

Three terms, each in [0, 1]:

| term | definition |
|---|---|
| $t_1$ filter uncertainty | $\tau/(\tau+\mathrm{tr}\,P)$, $\tau = 10$ m² |
| $t_2$ geometry | $1/\max(\mathrm{GDOP},1)$ |
| $t_3$ line-of-sight fraction | from the NLOS classifier |

In the evaluated form the score is a weighted sum,

$$\Phi = w_1 t_1 + w_2 t_2 + w_3 t_3,\qquad (w_1,w_2,w_3) = (2.995,\,0.000,\,0.005).$$

A fourth, hop-provenance term exists with zero weight: the poll set is one-hop by construction, so the hop count it
would penalise is always zero, and a non-zero weight would change nothing except where it degraded the score for a
reason unrelated to provenance. It must stay zero until relayed measurements exist.

## What the evidence says

The weights are the output of the project's calibration campaigns (CAL-01, CAL-03). Leave-one-seed-out calibration of
the three terms ranks position error **no better than the covariance trace**, and the optimiser puts essentially all of
the weight on $t_1$, a monotone function of $\mathrm{tr}\,P$. The score therefore carries no information beyond the
trace, and escalating on it is escalating on the trace. In the paired comparison, CA-TBL is level with
`fixed_threshold`, which switches on `tr P > 12 m²` alone ([results](../research/results.md)).

The accurate name is therefore *uncertainty-aware*: the system acts on the filter's own covariance, and relay ranges
inflate it. It is **not** a calibrated confidence in the statistical sense; the score is an index, and its thresholds
were fitted to decisions, not to coverage.

!!! warning "Not claimed"
    That the score carries information beyond the covariance trace. The novelty claim was withdrawn (Technical Report
    CR-24).

## Where it travels

The score is carried in a one-byte field over [0, 4] (`--pcsByteScale=4`) so a requester can refuse a request as
well as grant one. It also weights fix routing (`--fixRoutingConfidence`) and is returned in query responses.

Source: [`ca-tbl-policy.cc`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/src/lora-physics-engine/model/ca-tbl-policy.cc); preset values in
[`kSpecProfile[]`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/scratch/ca_tbl_causal_sim.cc).
