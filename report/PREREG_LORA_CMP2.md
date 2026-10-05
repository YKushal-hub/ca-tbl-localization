# PREREG LORA-CMP-2 — the LoRa comparators completed on both LoRa bands

**Registered before any cell runs** (rule 13). Written 2026-10-05. Engine: the build of
`3d637f4`, which adds `--arms=lora_fusion` (G88). This commit adds this file,
`scripts/lora_cmp2_run.sh` and `scripts/lora_cmp2_analyse.py`, and nothing else. Outputs go to
`results/lora_cmp2_2026-10-05/`, which does not exist at registration.

**Why.** The owner asked for the LoRa comparison to cover LoRa on all its frequencies, not
only RSSI and TDoA. LORA-CMP covered 868 MHz LoRa (RSSI, RSSI + gateway TDoA). CAMP-FINAL
covered 2.4 GHz LoRa time-of-flight (`static_2p4`, SX1280-style round-trip ranging) at 5 km
and in the endgame, but not on N8_ring. Nothing covered **both LoRa bands fused**, which is
what Poucet does without its UWB. The owner also asked why Poucet beats CA-TBL. A dual-band
LoRa arm separates what Poucet gains from fusing the two LoRa bands from what it gains from
UWB.

## 1. Comparators, per configuration (5 km, endgame, N8_ring)

| comparator | sim | cells |
|---|---|---|
| CA-TBL | `ca_tbl_full`, the preset | CAMP-FINAL A_main, B_end, C_n8 |
| LoRa 868 RSSI | `static_868 --tier1Tdoa=false` | LORA-CMP R_main, R_end, R_n8 |
| LoRa 868 RSSI + gateway TDoA | `static_868` | CAMP-FINAL A_main, B_end, C_n8 |
| LoRa 2.4 GHz ToF | `static_2p4` | CAMP-FINAL A_main, B_end; **new: S2_n8** |
| LoRa dual-band fusion, 868 RSSI + TDoA and 2.4 ToF | `lora_fusion` | **new: F_main, F_end, F_n8** |
| LoRa dual-band fusion, 868 RSSI and 2.4 ToF | `lora_fusion --tier1Tdoa=false` | **new: FR_main, FR_end, FR_n8** |
| fixed-threshold switching, Poucet fusion, single-hop | as LORA-CMP | CAMP-FINAL (A and B only) |

Not represented, as in LORA-CMP:
- RSSI fingerprinting;
- TDoA without RSSI;
- **LoRa bands other than 868 MHz and 2.4 GHz** (433 and 915 MHz). The engine models only the
  two bands the architecture and Poucet use, and terrain hardcodes 868 MHz (RULES.md §5).

## 2. Cells (`lora_cmp2_run.sh`)

All cells use the preset at 1 800 s, traces on, with the partner cell's flags.

| cell | arm | seeds | partner |
|---|---|---|---|
| F_main / F_end / F_n8 | `lora_fusion` | 1-20 | A_main / B_end / C_n8 |
| FR_main / FR_end / FR_n8 | `lora_fusion --tier1Tdoa=false` | 1-20 | the same |
| S2_n8 | `static_2p4` | 1-20 | C_n8 |
| X_main, X_end | `ca_tbl_full,static_868,static_2p4,poucet_fusion` | 1 | A_main / B_end seed 1 |
| X_rssi | `static_868 --tier1Tdoa=false` | 1 | LORA-CMP R_main seed 1 |

143 cells.

**The pairing crosses a binary change (rule 8).** CAMP-FINAL and LORA-CMP ran the frozen
`d49f99c` build; this campaign runs `3d637f4`'s. Its null control shows `arm_summary.csv`
byte-identical, with traces differing only in the last printed digit (TECH_DEBT 4.1). The X
cells test the claim the pairing rests on: with this binary, every compared arm's
`arm_summary.csv` and `subject_summary.csv` rows reproduce the earlier campaigns'
byte-for-byte. The analysis reads only those two files.

**Isolation.** As before: a git worktree at this commit, holding a byte copy of `3d637f4`'s
build. Its SHA-256 is listed in RUN.md.

## 3. Metrics (`lora_cmp2_analyse.py`)

The metrics are LORA-CMP's, imported unchanged: per seed, CAMP-SPEC's matrix definitions plus
J per subject fix, and paired differences from CA-TBL.

Added: **poucet_fusion − lora_fusion (both with TDoA), paired**, in A and B. This is what UWB
adds to Poucet over fusing the two LoRa bands.

## 4. Predictions, made before any cell

**Disclosed:**
- The analysis was exercised on four 60 s runs outside `results/` (seed 1).
- G88's endgame run (seed 7, 900 s) was read for its gate checks only.
- The only `lora_fusion` quantity seen is 27 fix cycles fusing both bands in G88's run.

| # | prediction |
|---|---|
| **M1** | Endgame: `lora_fusion` (TDoA) availability is within 0.03 of `poucet_fusion`'s (0.506). Poucet's lead comes from fusing the LoRa bands, not from UWB: its fixes labelled UWB are 201 of 13 583 over 20 seeds |
| **M2** | `lora_fusion`'s subject J is under 10 % of `poucet_fusion`'s in A and in B |
| **M3** | Endgame: the interval of availability (`lora_fusion` − CA-TBL) lies above 0 |
| **M4** | 5 km: `lora_fusion` with TDoA has a higher mean tag 0 CEP95 than without (N.25 carries over) |
| **M5** | N8_ring: `static_2p4` availability is below 0.10 |

Each is written up as held or failed in `report/LORA_CMP2_RESULTS.md`.

## 5. Abort conditions (the analysis's "Problems")

Any of the following stops the write-up. The cause is fixed, and no cell is dropped:
- not 143 cells, or a cell exits non-zero;
- a `run_id.txt` does not read one source with `dirty 0`;
- an X cell's compared rows differ from the earlier campaign's;
- an F cell lacks its `tdoa_lora_fusion` file, or an FR cell has a `tdoa_` file;
- a `lora_fusion` row transmits on UWB or bills nothing on 2.4 GHz;
- a comparator has fewer than 20 seeds.

## 6. What follows

Nothing is adopted. The table answers the owner's coverage request, and the Poucet split
answers part of "why Poucet wins". The diagnosis of CA-TBL's own UWB path (0–2 UWB neighbours
on its UWB cycles, measured from CAMP-FINAL's B_end traces) is reported separately and is not
a prediction here.
