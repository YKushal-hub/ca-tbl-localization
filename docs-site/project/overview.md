# Overview and scope

## Why a hierarchy of radios

Satellite navigation fails in enclosed, obstructed, underground or collapsed settings, and radio reaches where
satellites do not. The radio combination used here is not new: a LoRa-plus-UWB design was fielded with firefighters
and soldiers (Poucet et al., IPIN 2021), and mesh LoRa localisation, threshold-based band switching, multi-hop
uncertainty propagation and fault-tolerant localisation all have prior work, cited in
[`Architecture.md` §13.2](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/Architecture.md).

What remains open is **when combining radios pays for itself**. The obstacle is physical:

- range wants low frequency and narrow bandwidth, which leaves ranging coarse;
- precision wants wide bandwidth, short reach, and a receive current above the transmit current, so a precision
  exchange spends most of its energy listening.

No single radio satisfies all three demands (reach, precision, battery). The project asks whether a decision layer
that escalates between radios earns its complexity against simply pinning the radio to one tier.

## Objectives

1. Specify a three-tier stack and implement it in ns-3 with physics engines faithful enough that conclusions are about
   the architecture and not the simulator.
2. Fix a design requirement in advance: **A ≥ 0.95 at R ≤ 25 m**, the share of all decision cycles with a position
   within 25 m.
3. Compare the decision layer with pinned and fused alternatives on paired seeds, under pre-registered rules.
4. Account for energy per radio action, so that the energy saved by *not* escalating is measurable.

## Contributions, stated narrowly

The contribution is **the assembly and its quantification**, plus the measurement apparatus (registered campaigns,
provenance sidecars, regression gates with negative fixtures). It is not the radio combination, mesh LoRa
localisation, threshold band switching, multi-hop uncertainty propagation or fault-tolerant localisation.

The fault model is **environmental, not adversarial**: cold-sagged battery skewing RSSI, antenna detuning from ice,
canopy multipath, sensor drift.

## Supported and unsupported claims

The claim set is recorded in the
[Technical Report](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/CA-TBL_Technical_Report_SelfContained.md)
(CR-24), which is authoritative for results.

| claimed | not claimed |
|---|---|
| the assembly and its quantification | that the escalation rule beats a flat variance threshold |
| availability above single-band arms | that the score carries information beyond the covariance trace |
| availability level with POUCET-style fusion at about 1/34 of its energy (5 km) | that the integrity check pays |
| tag 0 meets the requirement; so does the five-node pool, on 12 gateways and 12 finders | that the 5 km margin is wide |

!!! warning "Simulation only"
    No hardware was used. Receiver sensitivities are read from data-sheet rows; no check in the project would catch
    a new error of the six-decibel kind that once passed every gate (see [limitations](../research/limitations.md)).
