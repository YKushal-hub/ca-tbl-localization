# The three tiers

| | tier 1 | tier 2 | tier 3 |
|---|---|---|---|
| band | 868 MHz LoRa | 2.4 GHz LoRa | UWB, about 7.99 GHz |
| part | SX1262 | SX1280 | DW3000 |
| observable | RSSI inversion; TDoA where ≥ 3 gateways hear | two-way time of flight | double-sided two-way ranging |
| transmit power | +14 dBm | +12.5 dBm | −16.47 dBm |
| sensitivity | −123 dBm | −112 dBm (SF7, 812.5 kHz) | −99.3 dBm (850 kb/s, 1024-symbol preamble) |
| ranging floor | none (error grows with range) | 3.0 m | 0.10 m |
| measured `r80` | 2390 m | 320 m | 5.4 m |
| mean abs. range error at `r80` | 1543 m | 4.7 m | 0.08 m |

`r80` and the range errors come from the simulator's built-in range probe under the evaluated configuration, 2000 frames
per distance (`--probe=true`). Hardware parameters are read from the data sheets (SX1262, SX1280 Table 6-1, DW3000
Table 9); the ranging floors are cited from published measurements (Zhou et al. 2025; Benouakta et al. 2023).

## Tier 1: 868 MHz

Wide-area coverage floor, always reachable. RSSI inversion is poor as a ranging method (the error is the link's own
shadowing seen from the other end), so tier 1 is a **coverage floor and a coarse fix**, not a ranging tier; TDoA adds a
position where three gateways hear the same broadcast. Measured delivery: 0.95 at 1 km, 0.86 at 2 km, 0.71 at 3 km.

## Tier 2: 2.4 GHz

Time-of-flight refinement, modelled on the SX1280's own 812.5 kHz row rather than the wider 1625 kHz row, a 6 dB
difference. Measured delivery: 0.98 at 100 m, 0.82 at 300 m, 0.41 at 900 m.

## Tier 3: UWB

Short-range precision. Frames are billed, ranged and decoded in one mode. Measured delivery: 0.99 at 2 m, 0.83 at 5 m,
0.51 at 10 m, 0.14 at 20 m.

!!! warning "A design constant beyond a tier's reach"
    The evidence gate uses a 20 m prediction radius, which lies beyond the distance at which a tier-3 link delivers one
    frame in seven. The constant is the project's, not measured.

## Propagation

Each tier has its own physics engine because the differences between bands are differences of mechanism: vegetation
costs a few decibels at 868 MHz and tens at UWB; the thermal floor over 499.2 MHz sits 36 dB above that over 125 kHz;
and UWB detection integrates a preamble where LoRa relies on chirp processing gain. Each engine applies log-distance
path loss, spatially correlated shadowing (30 m decorrelation), Rician fading with I/Q state per band, a persistent NLOS
state, terrain diffraction at each tier's own carrier wavelength, and ITU-R P.833 vegetation attenuation. All tiers are
simulated in the same forest, path-loss exponent drawn from $\mathcal N(2.3,0.1)$ truncated to [2.1, 2.6], shadowing
about 7 dB, foliage 0.20 dB/m saturating at 25 dB.

The path-loss exponent and shadowing values are engineering assumptions with **no traced field source**.
