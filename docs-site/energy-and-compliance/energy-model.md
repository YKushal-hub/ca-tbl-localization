# Energy model

Energy is a primary output: both of the project's energy claims need it. It is charged **per radio action** from
datasheet currents, not estimated from a duty-cycle average, because the decision layer acts on the number of
activations. Source: [`ca-tbl-energy-model.cc`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/src/lora-physics-engine/model/ca-tbl-energy-model.cc);
specification in [`Architecture.md` §8](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/Architecture.md).

## Datasheet basis

| | SX1262 (868 MHz) | SX1280 (2.4 GHz) | DW3000 (UWB) |
|---|---|---|---|
| TX current | 45 mA @ +14 dBm | 24 mA @ +12.5 dBm | 82 mA |
| **RX current** | 5.3 mA | 7.0 mA | **94 mA** |
| idle | 1.5 mA | 1.4 mA | 12 mA (PLL) |
| sleep | 1.2 µA | 1.2 µA | 0.7 µA |
| supply | 3.3 V | 3.3 V | 3.3 V |

!!! note "The single most consequential number"
    On the DW3000 the receive current exceeds the transmit current, so a UWB exchange is dominated by **listening**. The
    lever that saves UWB energy is the number of activations, which is exactly what the decision layer controls. In the
    850 kb/s mode each frame also costs several times the airtime of the faster mode.

## What is charged

| event | charge |
|---|---|
| channel-activity detection | wake-up at idle plus the CAD duration at CAD current |
| tier-1 ranging | one broadcast; the node then wakes for **one downlink per fix** (edge estimator), on the tier the fix came from |
| tier-2 ranging | per anchor: TX, turnaround, guarded RX |
| tier-3 ranging | per anchor: TX (poll), RX (response), TX (final) |
| **failed exchange** | TX plus the **full receive timeout**, so it costs more than a success |
| escalation handshake | request TX plus guarded RX for the grant, on the tier being left |
| inactive radios | sleep current for the whole cycle |
| neighbour discovery | beacon listening at RX current |
| edge uplink | the uplink of ranges to the edge estimator, billed for every subject |

The failed-exchange rule is deliberate: it makes escalating into a dead zone **expensive**, not merely useless.

## Time on air

LoRa airtime uses the Semtech relation, computed rather than assumed:

$$T_{sym}=2^{SF}/BW,\quad T_{pre}=(n_{pre}+4.25)T_{sym},\quad ToA=T_{pre}+n_{payload}T_{sym},$$

with the low-data-rate optimisation enabled automatically when $T_{sym}>16$ ms. A tier-1 ranging frame (SF7, 125 kHz,
CR 4/5, 20 B) is 56.58 ms; the escalation handshake frame is 41.22 ms. UWB airtime follows IEEE 802.15.4z HRP.

## Verification

Transmit energy is recomputed from the data-sheet current, the supply voltage and the logged transmit airtime, and
sleep energy from the sleep current and the run length. On 18 arm-runs (three seeds, both configurations; CA-TBL and
the two static arms) transmit energy matches to a relative error of at most $2.2\times10^{-16}$. Receive, CAD and idle
energy depend on listening windows that a run does not log separately, so they are **not** independently recomputed.

!!! warning "Not modelled"
    Processing (CPU) energy is not in the radio model. The numbers are radio energy per subject over a 1800 s run.

## Measured subject energy (joules per subject, 1800 s)

| arm | 5 km, adopted | endgame, adopted |
|---|---|---|
| `ca_tbl_full` | 13.3 | 25.7 |
| `fixed_threshold` | 13.2 | 25.0 |
| `poucet_fusion` | 457.1 | 389.6 |
| `static_2p4` | 6.0 | 10.2 |
| `static_868` | 8.6 | 9.8 |

Source: PRESET-5 ([results](../research/results.md)).
