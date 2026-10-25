# Tier escalation

## Two rules that must be kept apart

The **design** asks whether a more precise radio is worth activating *before* activating it. From the tier-1
posterior $P_{868}$ and a predicted geometry $H$ and noise $R$ for the candidate radio,

$$P^+=P_{868}-P_{868}H^{\mathsf T}(HP_{868}H^{\mathsf T}+R)^{-1}HP_{868},\qquad \Delta J=\mathrm{tr}P_{868}-\mathrm{tr}P^+ ,$$

and the radio is activated when $\Delta J/C>\lambda$ (the sensor-selection formulation of Joshi and Boyd). **This rule
has not been evaluated.**

The **implementation** evaluated here is a threshold rule on the [uncertainty score](uncertainty-score.md).

## The implemented rule

| step | behaviour |
|---|---|
| escalate | one tier when $\Phi$ falls below a per-tier threshold: **1.642** from tier 1, **1.377** from tier 2 |
| de-escalate | one tier after $N_d$ consecutive cycles with $\Phi$ above **1.542** (tier 2) or **1.880** (tier 3) |
| not delivered | if the active tier returned fewer than three ranges, count the cycle; demote after $N_d$. **Demotion is never deferred** |

The gap between the two thresholds and the dwell bias the node toward staying at higher precision, on the argument that
in a search a missed escalation costs more than a wasted one. The thresholds come from the calibration campaign; the
dwell is the project's choice.

## Suppress-only checks

An escalation the score requests passes checks that can only **suppress**, never create, an escalation:

1. **Switching-point test.** The other nodes ranged this cycle are witnesses. Witness $j$ agrees if
   $|r_j-\lVert\hat x-p_j\rVert|\le k\sqrt{\sigma_{\hat x}^2+\sigma_{p_j}^2+\sigma_\rho^2}$ with $k=3$,
   $\sigma_\rho=30$ m. With at least two witnesses, an escalation is refused if fewer than half agree.
2. **Evidence gate** (2.4 GHz to UWB). Suppress unless $\sqrt{\mathrm{tr}P}\le0.25\times20$ m and at least three
   UWB-capable nodes (gateways or finders) are predicted within 20 m.

!!! danger "Two different mechanisms"
    The **evidence gate** (above) and the **integrity check** (a Mahalanobis test of a fix against the prior) are
    different mechanisms with different measured behaviour. The repository sometimes calls both "the gate". This site
    does not. The integrity check is [off in the preset](../research/results.md#the-integrity-check).

## The handshake

An escalation is a request and a grant, sent at the highest access category. The request crosses the mesh to an
arbiter, and **UWB is entered only on a grant**. Under the 2.5 % duty cap a tier-1 ranging frame of 56.58 ms arms
2.263 s of off-time against a 2.5 s cycle, so a handshake departing tier 1 competes with ranging for the same airtime
([regulatory considerations](../energy-and-compliance/regulatory.md)).

Tier 3 is scarce, so the arbiter scores competing requests,

$$U(r)=\lambda_1 r_{\deg}+\lambda_2(1/E_b)+\lambda_3\zeta+\lambda_4\mathbb 1[\mathrm{query}],$$

over uncertainty-degradation rate, inverse remaining battery energy, staleness and an operator-query indicator,
normalised before weighting. The $\lambda$ encode policy, not measurement. The arbiter keeps a queue and anchors
gossip their grants every 5 s.
