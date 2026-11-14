# Components

Paths are relative to the repository root. Line numbers drift; search for the symbol.

| component | path | responsibility |
|---|---|---|
| 868 MHz engine | `src/lora-physics-engine/model/lora-physics-engine.*` | propagation, reception, RSSI ranging |
| 2.4 GHz engine | `…/lora-2p4-physics-engine.*` | propagation, two-way ToF |
| UWB engine | `…/lora-uwb-physics-engine.*` | propagation, double-sided TWR |
| submodules | `…/submodules/` | about 35 per engine: shadowing, fading, foliage, terrain, interference, receiver decision, ranging |
| `CaTblHeader` | `…/ca-tbl-header.*` | message taxonomy, wire sizes, access categories, ACK policy |
| `CaTblMac` | `…/ca-tbl-mac.*` | CSMA/CA, duty-cycle governor, adaptive power control, neighbour table, 9 message types |
| `CaTblEnergyModel` | `…/ca-tbl-energy-model.*` | per-action joule accounting from datasheet currents |
| `CaTblPolicy` | `…/ca-tbl-policy.*` | EKF, score, evidence gate, integrity check, escalation state machine |
| IMU filter and sensor model | `…/ca-tbl-imu-*.{h,cc}` | the 5-state filter and its synthetic IMU |
| TDoA | `…/ca-tbl-tdoa.*` | tier-1 TDoA solve |
| NLOS classifier | `…/ca-tbl-nlos-classifier.*` | per-tier NLOS scores |
| session, routing, relay admission, grant coordination, node lifecycle, sparse anchor, LOOCV | `…/ca-tbl-session.*`, `ca-tbl-routing.*`, `ca-tbl-relay-admission.*`, `ca-tbl-grant-coord.*`, `ca-tbl-node-lifecycle.*`, `ca-tbl-sparse-anchor.*`, `ca-tbl-loocv.*` | the network layer and the calibration support |
| scenario harness | `scratch/ca_tbl_causal_sim.cc` | geometry, mobility, arms, the preset (`kSpecProfile[]`), about 315 flags, the shared medium and capture arbitration |
| unit suites | `scratch/ca_tbl_*_test.cc`, `ca_tbl_packet_validator.cc` | twelve programs, 401 checks |
| gates | `scripts/catbl_gates.sh` | regression gates G1–G166 |
| analysis | `scripts/*_analyse.py`, `scripts/goal_report.py` | scoring of registered campaigns |

## The MAC

Nine message types fall into four access categories adapted from IEEE 802.11e EDCA:

| class | messages | ACK |
|---|---|---|
| urgent | triggered update, last gasp | none; 2–3× jittered rebroadcast |
| high | **escalation request / grant** | **acknowledged**, the only class that is |
| normal | entry push, digest response | implicit |
| low | discovery beacon | none |

The 9-byte MAC header (packet id, originator, hop limit, message type, tier) is common to all types, so flood control
and deduplication are type-agnostic. Collisions are **emergent**: two overlapping transmissions collide and a 6 dB
capture rule judges both frames; an interferer is attenuated by its own distance to the receiver. Before transmitting a
node performs LoRa channel-activity detection and backs off for a random number of slots scaled by its access category.

## The receive chain

A frame passes CRC, a duplicate cache keyed on the originator's packet id, a hop-limit check, the header parser and a
dispatcher before it can influence a position. Because a relayed frame carries the *originator* in its identity field,
admitting it to the poll set would insert a peer the receiver may be unable to hear; the poll set is therefore
**one-hop by construction**.
