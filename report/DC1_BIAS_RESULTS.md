# DC1-BIAS — the preset with a gyro that has a turn-on bias

Registered in `report/PREREG_DC1_BIAS.md` (`4739849b`) before any cell; 180 of 180 cells, every
`run_id.txt` `source 4739849b dirty 0`; scored once by the registered scorer, unedited,
"Problems: 0" (`results/dc1_bias_2026-10-17/DC1_BIAS.md`). The preset at `93695ac`, six arms per
cell, seeds 1–20, DC-1's gyro bias sigma at **L0** 8.816 °/h (the preset), **L2** 0.15 °/s and
**L3** 0.5 °/s, paired by seed. The defect is TECH_DEBT N.28.

## Registered outcomes

| | prediction | outcome |
|---|---|---|
| R1 | L0 reproduces D8-MAIN's off cells, all six rows | **HOLDS** — 60 of 60 identical |
| P1 | `ca_tbl_full` tag 0 CEP95 rises at L3 against L0 in A and B | **HOLDS** — +1918.6 m [1482.3, 2395.1] in A, +271.0 m [209.9, 349.4] in B |
| P2 | in B, `ca_tbl_full`'s mean tag 0 CEP95 does not decrease L0 → L2 → L3 | **FAILS** — 12.0, 303.5, 283.0 m: L3 is on the same plateau as L2 |

## What a realistic gyro does to the preset

Tag 0 CEP95, `ca_tbl_full`, mean over 20 seeds:

| config | L0 8.816 °/h | L2 0.15 °/s | L3 0.5 °/s | Δ availability at L3 |
|---|---|---|---|---|
| A 5 km | 57.8 m | 1661.9 m | 1976.4 m | −0.066 [−0.089, −0.043] |
| B endgame | 12.0 m | 303.5 m | 283.0 m | −0.078 [−0.106, −0.050] |
| C N8 ring | 44.7 m | 971.8 m | 881.6 m | −0.052 [−0.070, −0.036] |

**Every arm degrades alike**, the comparators included: the preset gives every arm DC-1. At 5 km
and L3, tag 0 CEP95 rises by 1.8–2.2 km for all six arms; in the endgame by 0.27–0.50 km.

**It is a plateau, not a slope.** Per seed in A, the CEP95 change at L3 is uncorrelated with the
seed's drawn |bias| (Pearson r = −0.08, 20 seeds). The seed that drew only 0.005 °/s still goes
from 8.8 m to 186.6 m. DC-1's heading error grows as bias × elapsed time and is never corrected
(its header: "the heading error does not reset at a fix"), and its filter has no heading or bias
state to learn it from fixes. So 0.005 °/s is ≈ 9° of heading error by the end of a 1800 s run and
0.03 °/s is ≈ 50°; past that the dead-reckoned direction is effectively arbitrary, and more bias
cannot make it worse. That is why P2 failed. A candidate amplifier, **not measured**: the
IMU-model integrity witness, fed the same drifting dead reckoning, may veto fixes the filter needed.

## The comparison table, by level

CA-TBL − comparator, paired by seed:

| config | contrast | Δ availability L0 / L2 / L3 | Δ CEP95 tag 0 L0 / L2 / L3 |
|---|---|---|---|
| A | − LoRa fusion | +0.060 / +0.042 / +0.035 (all above 0) | −36.5 / −330.9 (n.s.) / −140.9 (n.s.) |
| A | − Poucet | +0.046 / +0.035 / +0.035 (all above 0) | −38.3 / −332.4 (n.s.) / −117.0 (n.s.) |
| B | − LoRa fusion | +0.066 / +0.066 / +0.066 (all above 0) | −2.5 / −54.6 (n.s.) / **−188.1** [−331.8, −62.2] |
| B | − Poucet | +0.051 / +0.064 / +0.068 (all above 0) | −3.0 / −59.0 (n.s.) / **−149.4** [−253.4, −44.3] |
| C | − LoRa fusion | +0.021 / +0.033 / **+0.009 (n.s.)** | −3.7 / −148.4 (n.s.) / +67.1 (n.s.) |
| C | − Poucet | +0.018 / +0.021 / **+0.013 (n.s.)** | −6.1 / −196.8 (n.s.) / +32.6 (n.s.) |

CA-TBL's availability lead survives a realistic gyro in A and B and is lost on the ring at L3. The
accuracy contrasts are dominated by the gyro: hundreds of metres, mostly not significant.

## What this says

**Every accuracy figure produced under the preset is conditional on a near-perfect gyro**, and the
conditioning is not mild: with any realistic MEMS bias, CEP95 is in the hundreds of metres to
kilometres for every arm. That is a property of DC-1's design (no heading or bias correction), not
of tiering. The remedy is a filter that estimates heading and bias from fixes — the local line's
5-state filter, ported as `ca-tbl-imu-ekf.{h,cc}` (G91, G94) and not yet wired into the policy.
A rule fixed in the session's work plan before scoring (not a committed registration) said P1
holding means that wiring proceeds; it does, as its own registered mechanism.

## What this cannot establish

What the right default is (the owner's decision); whether the witness amplifies the loss; DC-1's
scale error or noise density; anything about a filter that estimates the bias, which is next.
