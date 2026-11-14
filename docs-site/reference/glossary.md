# Glossary

| term | meaning |
|---|---|
| **A(25 m)** | the share of all decision cycles whose position estimate is within 25 m of the truth; the design requirement is A ≥ 0.95 |
| **arm** | one configuration compared in a run: `ca_tbl_full`, `fixed_threshold`, `poucet_fusion`, `static_2p4`, `static_868` |
| **CEP95** | the radius containing the position error on 95 % of cycles |
| **endgame** | the last stretch of a search: a 750 m half-width box, finders converging directly |
| **evidence gate** | a suppress-only check on escalating from 2.4 GHz to UWB: estimate quality and predicted UWB coverage. Not the integrity check |
| **finder** | a mobile searcher carrying a tri-band node, with GNSS |
| **floor** | tier-1 ranging in the same cycle when the active tier returned fewer than three ranges |
| **gate (regression)** | a check in `scripts/catbl_gates.sh`, G1–G166, each with a negative fixture |
| **gateway** | fixed, surveyed infrastructure on a 15 m mast; hosts the edge estimator |
| **integrity check** | the Switching-Point Integrity Check: a Mahalanobis test of a fix against the prior. Evaluated; off in the preset. Not the evidence gate |
| **lost node / tag 0** | the subject the search is for (node 1) |
| **NIS** | normalised innovation squared |
| **PCS** | position-confidence score, the code's name for the [uncertainty score](../theory/uncertainty-score.md) |
| **preset** | `--specProfile=true`: the configuration all claims are drawn from |
| **registered campaign** | a campaign whose predictions, falsifiers and abort conditions were committed before its first cell ran |
| **`r80`** | the distance at which 80 % of frames are received |
| **subject** | a field node being located; tag 0 and four peers |
| **switching-point test** | witness agreement on residuals against other nodes' reports; suppress-only; on |
| **tier** | a choice of radio: 1 = 868 MHz LoRa, 2 = 2.4 GHz LoRa, 3 = UWB |
| **TDoA** | time difference of arrival across gateways |
| **TWR** | two-way ranging |
