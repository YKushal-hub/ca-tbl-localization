# LORA-CMP — results: CA-TBL against outdoor LoRa-based localisation, on our simulator

**Registered in `report/PREREG_LORA_CMP.md` (`0a542ec`) before any R cell ran.** All 61
cells exit 0 and read `source 0a542ecc dirty 0`. They ran on CAMP-FINAL's frozen build, and
X_repro reproduced CAMP-FINAL's row byte-for-byte. The registered analysis was applied
unchanged ("Problems: 0"). Tables: `results/lora_cmp_2026-10-05/LORA_CMP.md`. Numbers as CSV:
`results/lora_cmp_2026-10-05/lora_cmp_table.csv`. Provenance:
`results/lora_cmp_2026-10-05/RUN.md`.

**Scope.** Every comparator runs in this simulator, on the same deployment, channel, MAC and
energy model (PREREG §1). These are **methods measured on our deployment**, not published
systems' numbers. RSSI fingerprinting and TDoA without RSSI are not represented.

**How to read the numbers.**
- Tag 0 CEP is the mean over seeds of each seed's CEP. Availability and energy are means over
  the five subjects.
- Δ is the comparator minus CA-TBL, paired by seed, mean [95 % seed bootstrap].
- **At 5 km, tag 0 CEP95 varies from seed to seed by an order of magnitude**, so its paired
  intervals are wide.
- Availability intervals in a few CAMP-FINAL rows differ from `CAMP_FINAL.md` by 0.001 in one
  bound. The bootstrap draws in a different order; the means are identical.

---

## 1. 5 km, the spec's three gateways (CAMP-FINAL A_main, G_single; R_main)

| comparator | availability | CEP50 / CEP95 tag 0 (m) | subject J | Δ availability | Δ CEP95 (m) | Δ subject J |
|---|---|---|---|---|---|---|
| **CA-TBL** | 0.174 | 70.9 / 209.6 | 12.37 | — | — | — |
| LoRa RSSI multilateration | 0.159 | 51.7 / 120.4 | 6.07 | −0.015 [−0.035, 0.004] | −89.2 [−247.7, 48.7] | −6.3 [−7.5, −5.1] |
| LoRa RSSI + gateway TDoA | 0.165 | 289.2 / 559.2 | 6.10 | −0.009 [−0.032, 0.012] | +349.7 [−8.7, 752.8] | −6.3 [−7.4, −5.1] |
| fixed-threshold switching | 0.186 | 69.3 / 179.4 | 11.04 | +0.012 [−0.000, 0.024] | −30.2 [−140.7, 67.1] | −1.3 [−1.7, −0.9] |
| Poucet-style LoRa+UWB fusion | 0.238 | 161.8 / 219.4 | 462.46 | **+0.064 [0.037, 0.090]** | +9.8 [−219.7, 293.6] | +450.1 |
| CA-TBL single-hop (LoRaWAPS-style) | 0.152 | 53.5 / 111.1 | 12.27 | **−0.022 [−0.036, −0.008]** | −98.5 [−240.9, 13.8] | −0.1 [−1.2, 1.6] |

- **No comparator differs from CA-TBL on tag 0 CEP95:** every paired interval includes 0.
- **On availability:** Poucet fusion is higher (at 37× the subject energy) and single-hop is
  lower. Plain RSSI LoRa cannot be distinguished from CA-TBL, at half its energy.

## 2. Endgame, finders converged (B_end, G_single_end; R_end)

| comparator | availability | CEP50 / CEP95 tag 0 (m) | subject J | Δ availability | Δ CEP95 (m) | Δ subject J |
|---|---|---|---|---|---|---|
| **CA-TBL** | 0.423 | 8.4 / 18.4 | 24.91 | — | — | — |
| LoRa RSSI multilateration | 0.371 | 64.6 / 106.0 | 6.35 | **−0.052 [−0.063, −0.039]** | **+87.6 [55.9, 125.0]** | −18.6 |
| LoRa RSSI + gateway TDoA | 0.371 | 9.1 / 20.6 | 6.43 | **−0.052 [−0.064, −0.039]** | **+2.2 [0.3, 4.2]** | −18.5 |
| fixed-threshold switching | 0.439 | 7.7 / 16.6 | 15.44 | **+0.017 [0.006, 0.027]** | −1.9 [−4.2, 0.1] | −9.5 |
| Poucet-style LoRa+UWB fusion | 0.506 | 6.4 / 13.5 | 416.24 | **+0.083 [0.065, 0.102]** | **−4.9 [−7.7, −2.4]** | +391.3 |
| CA-TBL single-hop (LoRaWAPS-style) | 0.415 | 8.9 / 19.8 | 21.16 | −0.008 [−0.018, 0.001] | +1.3 [−0.3, 2.9] | −3.7 [−6.2, −1.2] |

**CA-TBL beats both LoRa-only methods.** It fixes 5.2 points more often than either. Its tag 0
CEP95 is 87.6 m lower than RSSI-only's and 2.2 m lower than RSSI + TDoA's. It spends 3.9× the
subject energy to do so (24.91 against 6.35 J).

It does not beat the two other comparators:
- **Fixed-threshold switching** fixes 1.7 points more often, at 62 % of CA-TBL's energy. CEP95
  cannot be distinguished.
- **Poucet fusion** is better on both availability and CEP95, at 16.7× CA-TBL's energy.

## 3. Eight gateways on a ring (C_n8; R_n8)

| comparator | availability | CEP50 / CEP95 tag 0 (m) | subject J | Δ availability | Δ CEP95 (m) | Δ subject J |
|---|---|---|---|---|---|---|
| **CA-TBL** | 0.281 | 19.5 / 43.7 | 12.78 | — | — | — |
| LoRa RSSI multilateration | 0.311 | 51.7 / 106.8 | 6.04 | **+0.030 [0.012, 0.049]** | **+63.0 [46.2, 81.6]** | −6.7 |
| LoRa RSSI + gateway TDoA | 0.319 | 16.3 / 33.3 | 6.05 | **+0.038 [0.014, 0.062]** | **−10.4 [−17.7, −2.9]** | −6.7 |

With eight gateways, LoRa RSSI + TDoA alone **beats CA-TBL on availability and on CEP95**, at
47 % of its energy.

## 4. Predictions

| | registered | measured | verdict |
|---|---|---|---|
| **L1** | 5 km: RSSI-only mean tag 0 CEP95 below RSSI + TDoA's 559.2 m | 120.4 m | **held** |
| **L2** | 5 km: interval of Δ CEP95 (RSSI-only − CA-TBL) not entirely above 0 | −89.2 [−247.7, 48.7] | **held** |
| **L3** | endgame: interval of Δ availability (RSSI-only − CA-TBL) below 0 | −0.052 [−0.063, −0.039] | **held** |
| **L4** | RSSI-only subject J within 5 % of RSSI + TDoA's in every configuration | 5 km 6.07 vs 6.10 (−0.6 %); endgame 6.35 vs 6.43 (−1.2 %); N8 6.04 vs 6.05 (−0.1 %) | **held** |
| **L5** | N8_ring: RSSI-only availability within 0.03 of RSSI + TDoA's | 0.3114 vs 0.3193 (−0.0079) | **held** |

All five held.

## 5. Observation outside the registration: what gateway TDoA does to LoRa-only accuracy

This was computed after the registered analysis, with its own functions. It is RSSI-only
minus RSSI + TDoA, `static_868`, paired by seed:

| configuration | Δ CEP95 tag 0 (m) | seeds where RSSI-only is better | Δ availability |
|---|---|---|---|
| 5 km, 3 gateways | −438.9 [−826.2, −119.4] | 13 of 20 | −0.006 [−0.016, 0.004] |
| endgame, 3 gateways | +85.4 [54.2, 121.7] | 0 of 20 | −0.000 [−0.010, 0.010] |
| N8_ring, 8 gateways | +73.4 [56.6, 91.4] | 0 of 20 | −0.008 [−0.021, 0.006] |

- **In the endgame and at eight gateways, RSSI-only is better on none of the 20 seeds.**
  TDoA lowers CEP95 by 73-85 m on average.
- **At 5 km with three gateways it makes accuracy worse** on average, on 13 of 20 seeds. On
  seed 1 it raised tag 0 CEP95 from 26.8 to 2 455 m, with 7 solves accepted out of 334 tier-1
  cycles.
- *Hypothesis, not tested:* with three gateways TDoA has no redundancy. A wrong root choice is
  fused with its full covariance, so the filter trusts it, and nothing downstream rejects it.

The preset uses TDoA at three gateways, so this affects CA-TBL's tier 1 at 5 km too.
Recorded as TECH_DEBT N.25; not changed here.
