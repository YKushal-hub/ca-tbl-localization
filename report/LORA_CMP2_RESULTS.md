# LORA-CMP-2 — results: LoRa on both bands against CA-TBL, and why Poucet beats CA-TBL

**Registered in `report/PREREG_LORA_CMP2.md` (`73b5bd3`, engine `3d637f4`) before any cell
ran.** All 143 cells exit 0 and read `source 73b5bd31 dirty 0`.

**The registered reproduction check fired on the first analysis.** X_main's and X_end's
`poucet_fusion` rows differed from CAMP-FINAL's in two snapshot-diagnostic columns
(`rmse_mlat`, `cep95_mlat`) and in nothing else. The cause:
- 1e-13 codegen differences, amplified by near-singular snapshot solves;
- engine-evaluation counts are identical on all 5 760 trace rows (rule 8).

The cause and the amendment are in the prereg's §7, committed (`3e40aff`) before this
write-up. With those two columns left out, every compared row reproduces, and the analysis
reports "Problems: 0".

Tables: `results/lora_cmp2_2026-10-05/LORA_CMP2.md`. Numbers as CSV:
`results/lora_cmp2_2026-10-05/lora_cmp2_table.csv`. Provenance:
`results/lora_cmp2_2026-10-05/RUN.md`.

How to read the tables: Δ is the comparator minus CA-TBL, paired by seed, mean [95 % seed
bootstrap]. Availability is the mean over the five subjects; CEP95 is tag 0's.

---

## 1. LoRa on both bands, against CA-TBL

**5 km, three gateways**

| comparator | availability | CEP95 tag 0 (m) | subject J | Δ availability | Δ CEP95 (m) | Δ J |
|---|---|---|---|---|---|---|
| **CA-TBL** | 0.174 | 209.6 | 12.37 | — | — | — |
| LoRa 868 RSSI | 0.159 | 120.4 | 6.07 | −0.015 [−0.035, 0.004] | −89.2 [−250.3, 48.8] | −6.3 |
| LoRa 868 RSSI + TDoA | 0.165 | 559.2 | 6.10 | −0.009 [−0.031, 0.011] | +349.7 [−0.3, 763.2] | −6.3 |
| LoRa 2.4 GHz ToF | 0.031 | 54.9 | 14.21 | **−0.143** [−0.164, −0.125] | **−154.6** [−295.2, −40.7] | +1.8 |
| LoRa dual-band, TDoA | 0.226 | 410.9 | 16.75 | **+0.052** [0.033, 0.071] | +201.3 [−44.1, 485.6] | +4.4 |
| LoRa dual-band, RSSI | 0.228 | 106.7 | 16.75 | **+0.054** [0.029, 0.079] | −102.9 [−254.2, 22.8] | +4.4 |
| Poucet (LoRa + UWB) | 0.238 | 219.4 | 462.46 | **+0.064** [0.038, 0.090] | +9.8 [−223.1, 291.1] | +450.1 |

**Endgame, finders converged**

| comparator | availability | CEP95 tag 0 (m) | subject J | Δ availability | Δ CEP95 (m) | Δ J |
|---|---|---|---|---|---|---|
| **CA-TBL** | 0.423 | 18.4 | 24.91 | — | — | — |
| LoRa 868 RSSI | 0.371 | 106.0 | 6.35 | **−0.052** [−0.064, −0.039] | **+87.6** [56.3, 124.2] | −18.6 |
| LoRa 868 RSSI + TDoA | 0.371 | 20.6 | 6.43 | **−0.052** [−0.064, −0.039] | **+2.2** [0.3, 4.2] | −18.5 |
| LoRa 2.4 GHz ToF | 0.271 | 10.8 | 5.81 | **−0.151** [−0.173, −0.130] | **−7.6** [−12.0, −3.7] | −19.1 |
| LoRa dual-band, TDoA | 0.501 | 15.6 | 9.45 | **+0.079** [0.060, 0.099] | −2.8 [−6.2, 0.1] | **−15.5** |
| LoRa dual-band, RSSI | 0.500 | 19.5 | 9.32 | **+0.077** [0.056, 0.100] | +1.1 [−5.6, 11.1] | **−15.6** |
| Poucet (LoRa + UWB) | 0.506 | 13.5 | 416.24 | **+0.083** [0.065, 0.102] | **−4.9** [−7.6, −2.4] | +391.3 |

**Eight gateways on a ring**

| comparator | availability | CEP95 tag 0 (m) | subject J | Δ availability | Δ CEP95 (m) | Δ J |
|---|---|---|---|---|---|---|
| **CA-TBL** | 0.281 | 43.7 | 12.78 | — | — | — |
| LoRa 868 RSSI | 0.311 | 106.8 | 6.04 | **+0.030** [0.012, 0.048] | **+63.0** [46.4, 81.3] | −6.7 |
| LoRa 868 RSSI + TDoA | 0.319 | 33.3 | 6.05 | **+0.038** [0.014, 0.061] | **−10.4** [−17.7, −2.9] | −6.7 |
| LoRa 2.4 GHz ToF | 0.036 | 31.5 | 12.43 | **−0.246** [−0.269, −0.224] | **−12.2** [−21.3, −4.1] | −0.3 |
| LoRa dual-band, TDoA | 0.391 | 38.1 | 15.10 | **+0.110** [0.087, 0.132] | −5.7 [−13.6, 2.1] | +2.3 |
| LoRa dual-band, RSSI | 0.386 | 68.2 | 15.08 | **+0.104** [0.083, 0.124] | **+24.5** [13.8, 35.9] | +2.3 |

Fixed-threshold switching and single-hop are as in `report/LORA_CMP_RESULTS.md`.

- **Single LoRa bands lose to CA-TBL on availability.** 2.4 GHz ToF is the most accurate
  where it fixes, but it fixes rarely: at most 0.271 availability.
- **Dual-band LoRa fusion beats CA-TBL on availability in every configuration:** +0.052,
  +0.079 and +0.110.
  - Its tag 0 CEP95 cannot be distinguished from CA-TBL's in any configuration, with TDoA.
  - In the endgame it does this on **38 % of CA-TBL's subject energy** (9.45 against 24.91 J).
- **Gateway TDoA at three gateways (TECH_DEBT N.25) carries over to fusion.** At 5 km, mean
  tag 0 CEP95 is 410.9 m with TDoA against 106.7 m without. At eight gateways TDoA helps:
  38.1 against 68.2 m.

## 2. Predictions

| | registered | measured | verdict |
|---|---|---|---|
| **M1** | endgame: `lora_fusion` (TDoA) availability within 0.03 of Poucet's 0.506 | 0.501; Poucet − `lora_fusion` paired +0.004 [−0.009, 0.017] | **held** |
| **M2** | `lora_fusion` subject J under 10 % of Poucet's in A and B | 16.75 / 462.46 = 3.6 %; 9.45 / 416.24 = 2.3 % | **held** |
| **M3** | endgame: Δ availability (`lora_fusion` − CA-TBL) interval above 0 | +0.079 [0.060, 0.099] | **held** |
| **M4** | 5 km: `lora_fusion` with TDoA has the higher mean CEP95 | 410.9 against 106.7 m | **held** |
| **M5** | N8_ring: `static_2p4` availability below 0.10 | 0.036 | **held** |

All five held.

## 3. Why Poucet beats CA-TBL

### 3.1 It is not the UWB (registered)

UWB's contribution to Poucet is `poucet_fusion` − `lora_fusion` with TDoA, paired by seed:

| | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|
| 5 km | +0.012 [−0.009, 0.035] | −191.5 [−596.9, 171.0] | +445.7 |
| endgame | +0.004 [−0.009, 0.017] | −2.1 [−4.6, 0.1] | +406.8 |

**UWB adds nothing measurable** to Poucet's availability or accuracy in these deployments, at
about 40 times the energy. Poucet's lead over CA-TBL is the lead of fusing the two LoRa bands.

### 3.2 What CA-TBL does instead (measured from CAMP-FINAL's traces after its registration; diagnosis, not a registered result)

1. **It uses one tier's fix per cycle; the comparators fuse.** This is by design
   (`ca_tbl_causal_sim.cc`, at the fix selection: "Preference rather than fusion is
   deliberate… Cross-tier fusion would produce a better number and a worse experiment").
   - At 5 km, tag 0 is on 2.4 GHz for 11 736 of 14 400 cycles.
   - Only 2 951 of those have the three 2.4 GHz neighbours a 2.4 GHz-only solve needs.
   - On 6 771 the 1-2 ranges it has are discarded, and 3 729 of its 5 062 2.4 GHz-cycle
     fixes come from the 868 MHz floor.
2. **Its UWB never delivers.** In the endgame CA-TBL is on UWB for 900 of 14 400 tag 0 cycles:
   - it never has three UWB neighbours: 0 on 704 cycles, 1-2 on 196;
   - a finder is physically within UWB range on 823 of them, but the UWB radio sleeps until
     the escalation and has not discovered it;
   - over 20 seeds it makes **0 UWB-delivered fixes** (CAMP-FINAL);
   - yet UWB is **64.6 % of its endgame subject energy** (16.08 of 24.91 J).
   - Four finders tasked across five subjects rarely put three in UWB range of one subject.
3. **The headline metrics hide a trade-off.** Per subject, endgame, CA-TBL − Poucet, paired:
   - tag 0: availability **+0.036** [0.023, 0.050] for CA-TBL; CEP95 +4.9 m [2.5, 7.7] for
     Poucet;
   - subjects 2-4: availability −0.14 to −0.16 for Poucet;
   - subjects 3 and 4: CEP95 **−14.8** [−22.3, −8.6] and **−17.4** [−30.7, −6.6] m for
     CA-TBL. Subject 2's −5.2 [−15.0, 3.1] cannot be distinguished.

   Poucet fixes more of the subjects' cycles by accepting worse fixes. The table's
   availability averages the subjects, and its CEP is tag 0's alone, so this does not show.

### 3.3 What would and would not change it

- **CA-TBL cannot beat an always-on fusion of every radio on accuracy and availability on the
  same deployment.** Each cycle, that fusion uses a superset of CA-TBL's measurements.
  CA-TBL's ground is energy at comparable accuracy, plus what Poucet lacks: the integrity
  check, the mesh and identity-scoped search.
- **On the evidence here, two engine changes would close most of the gap.** Both are
  candidates, not findings, and each needs its own registered campaign (TECH_DEBT N.26):
  - fusing a short active set with the floor (FUSE-02 did this; it failed at 5 km, partly
    through N.25);
  - not escalating into UWB without a discovered UWB neighbour, or listening for finders
    first.
