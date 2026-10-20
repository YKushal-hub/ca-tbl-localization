# PREREG DEBT-A — eight open TECH_DEBT items measured on the final preset (exploratory)

**Registered before any cell runs** (rule 13). Written 2026-10-18 on `integrate/2026-10-18-kushaal-kn`. This commit adds
this file, `scripts/debt_a_run.sh` and `scripts/debt_a_analyse.py`. The engine is the parent's: the preset of DD 10–16
plus the provenance stamps, and the new default-off flags of T12, T29 and T46 (G153–G155).

**Nothing is adopted by this campaign.** The variants that change physics or a preset decision are the owner's to
adopt (TECH_DEBT_STATUS, O-list). This campaign measures them.

**Disclosed:**
- T46 dev, 3 seeds, 5 km, `static_868` gateway ranges: E[z²] 2.23 → 2.73 with the flag. The likely cause is that
  received links carry less foliage than the all-link expectation the flag subtracts.
- T12 probe: UWB foliage at 10 m 3.3 → 8.1 dB, UWB PDR 0.64 → 0.46.
- T29 unit gate only.

## Cells
Common:
```
--specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true
--unknownStart=true --fixedVarThreshold=12 --arms=static_868,static_2p4,ca_tbl_full,fixed_threshold
```
Configs: A (5 km) and B (`--boxHalf=750 --finderMode=1`).

| variant | flags | debt item |
|---|---|---|
| BASE | the preset | |
| NLOSDET | `--nlosRSource=detector` | T27 |
| FOLB | `--foliageScaleBounds=true` | T12 |
| INTERF | `--interferenceDistance=true` | T29 |
| RXPOW | `--rssiRangeFromRxPower=true` | T46 |
| SUBNOTDOA | `--subjectTier1Tdoa=false` | T26: H2 in its TDoA form |
| EDGEOFF | `--gatewayEdgeServer=false` | T33: C6 under the fix-tier bill |
| ARBOFF | `--arbitrationV2=false` | T42: identity-scoped arbitration |

Seeds 1–20: 320 cells.

## Predictions (paired over seeds, mean [95 % bootstrap]; tag 0 unless stated)
| # | prediction |
|---|---|
| D1 | NLOSDET: `ca_tbl_full` CEP95 \|Δ\| ≤ 10 % of BASE in A and B (N.52: removing NLOS truth "changes nothing measurable") |
| D2 | FOLB: B, `ca_tbl_full` UWB tier occupancy falls (less UWB reach) |
| D3 | INTERF: A, `ca_tbl_full` fix share rises (fewer phantom long-range collisions) |
| D4 | RXPOW: A, `static_868` CEP95 rises (a truer, noisier RSSI range) |
| D5 | SUBNOTDOA: A, `ca_tbl_full` subjects' share of cycles within 25 m rises (H2, TDoA form) |
| D6 | EDGEOFF: `static_868` tag-0 energy higher than BASE by ≥ 25 % in A (C6 sign and size under the current bill) |
| D7 | ARBOFF: B, `ca_tbl_full` CEP95 \|Δ\| ≤ 10 % (arbitration changes who gets UWB, not tag 0's accuracy) |

## Abort conditions
- A run without `rc 0`.
- A `run_id.txt` without `dirty 0`.
- A manifest mismatch.
