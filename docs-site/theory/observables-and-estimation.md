# Observables and estimation

Each tier returns a range, or a position, and a per-measurement standard deviation. The filter consumes both.
Implementation: [`ca-tbl-policy.cc`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/src/lora-physics-engine/model/ca-tbl-policy.cc),
[`ca-tbl-tdoa.cc`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/src/lora-physics-engine/model/ca-tbl-tdoa.cc) and the three `lora-*-physics-engine.*`.

## Tier 1: RSSI and TDoA

Received power is inverted through a log-distance model,

$$PL(d) = PL(d_0) + 10\,n\log_{10}(d/d_0) + X_\sigma .$$

The shadowing term is the link's own spatially correlated draw, so the range error is the shadowing seen from the other
end, not independent noise. To first order $\sigma_r = \frac{d\ln 10}{10 n}\sigma_{dB}$; the filter is given the exact
standard deviation of the log-normal distribution this implies. Tier-1 uncertainty grows in proportion to range, so no
fixed accuracy describes the tier, and a time-of-arrival Cramér–Rao bound does not apply to the RSSI observable.

Where three or more gateways decode the same broadcast, a **TDoA position** is added. Each gateway stamps the arrival on
a GPS-disciplined clock; the differences are solved by Chan's closed form for four or more gateways, refined by Foy's
Taylor iteration, and exactly for three. The stamp error is the clock residual plus the one-way time-of-arrival bound at
the link's SNR plus an NLOS excess delay; the clock and NLOS terms are **models, not measurements**. A three-gateway
solve has no redundancy, so a TDoA position is fused only if its normalised innovation against the state is at most
9.21, the 99th percentile of $\chi^2$ with two degrees of freedom (`--tdoaInnovationGate`).

## Tier 2: two-way time of flight

The SX1280 round-trip engine is read directly: $\hat d = c(t_{RTT}-t_{silence})/2$, subtracting the responder's fixed
turnaround. The receiver is modelled on its own 812.5 kHz data-sheet row. The precision floor is 3.0 m, from outdoor
SX1280 time-of-flight measurements (Zhou et al., 2025).

## Tier 3: double-sided two-way ranging

A three-message exchange cancels first-order clock offset:

$$\mathrm{ToF}=\frac{T_{round,1}T_{round,2}-T_{reply,1}T_{reply,2}}{T_{round,1}+T_{round,2}+T_{reply,1}+T_{reply,2}} .$$

The radio is modelled in **one** data-sheet mode for everything it takes from it: 850 kb/s with a 1024-symbol preamble,
at −99.3 dBm. Frames are billed, ranged and decoded in that mode. The floor is 0.10 m (Benouakta et al., 2023).

## Non-line-of-sight

A non-line-of-sight link biases its range through the propagation layer's own state. **No truth of that state reaches
the filter**: the variance it uses is set by a detector scoring the link from what a node can observe. The detector's
measured precision on one audit was 0.02–0.08, so it flags mostly line-of-sight links, and it is circular by
construction on a simulated channel.

## The filter

Each subject runs a **five-state extended Kalman filter** over planar position, heading offset, gyro bias and stride
scale. Inertial increments drive the prediction, with covariance inflated by a factor of 30 because the dead-reckoner's
error is coherent. Ranges are fused by sequential scalar updates; the magnetometer reading is fused as a direct
measurement of the heading offset. In the preset:

| element | value |
|---|---|
| gyro turn-on bias | 0.15 °/s |
| stride-length noise | 5 % |
| magnetometer | 3° white noise plus 3° Gauss–Markov disturbance, 120 s time constant |
| finder GNSS error | 2 m per axis, carried as a **consider state**, not range noise |
| window smoother | last 60 cycles re-solved jointly by Levenberg–Marquardt for path, heading offset, gyro bias, stride scale and each finder's offset |
| start | every filter starts at the box centre, σ = half-width/√3; the truth reaches only the evaluation |
| acquisition | 2000-particle filter over the prior box, handing over to the EKF when the cloud is one mode |

These are the project's settings for a body-worn sensor, not measurements of a specific part.

## Relayed ranges

A range to a node whose own position is uncertain is not a range to a surveyed anchor. With $P_B$ that node's
reported covariance and $u$ the unit vector along the measurement,

$$\sigma_{\mathrm{eff}}^2=\sigma_{\mathrm{range}}^2+u^{\mathsf T}P_B u .$$

Projecting onto $u$ admits only the part of the neighbour's uncertainty that bears on this range. The equation applies
to finders (pseudo-anchors) and to peer subjects, relaying to depth two; a relayed range is further inflated by the
relay's mean normalised innovation squared, skipped if its innovation exceeds $\chi^2_1(0.999)$, and bounded by a
variance budget of $(156\,\mathrm{m})^2$ (which is the project's choice). Covariance intersection merges table
entries of unknown cross-correlation.
