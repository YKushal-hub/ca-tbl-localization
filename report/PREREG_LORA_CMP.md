# PREREG LORA-CMP — CA-TBL against outdoor LoRa-based localisation, on our simulator

**Registered before any cell runs** (rule 13). Written 2026-10-05. Engine: CAMP-FINAL's frozen
build of `d49f99c` (`src/` and `scratch/` are unchanged since). This commit adds this file,
`scripts/lora_cmp_run.sh` and `scripts/lora_cmp_analyse.py`, and nothing else. Outputs go to
`results/lora_cmp_2026-10-05/`, which does not exist at registration.

**Why.** The owner asked for numbers comparing CA-TBL with other outdoor LoRa-based
localisation, on our simulator. CAMP-FINAL already holds most comparators. It lacks the most
common one: **plain RSSI path-loss multilateration on 868 MHz.** Under `--specProfile` tier 1
is RSSI combined with gateway TDoA (`--tier1Tdoa`, Physical §1.1), so CAMP-FINAL's
`static_868` is "LoRa RSSI + gateway TDoA", not plain RSSI.

## 1. Comparators

| comparator | what it stands for | sim | cells |
|---|---|---|---|
| CA-TBL | the system | `ca_tbl_full`, the preset | CAMP-FINAL A_main, B_end, C_n8 |
| LoRa RSSI multilateration | path-loss ranging to the gateways, 868 MHz only | `static_868 --tier1Tdoa=false` | **new: R_main, R_end, R_n8** |
| LoRa RSSI + gateway TDoA | the same with PPS-stamped gateway TDoA | `static_868`, the preset | CAMP-FINAL A_main, B_end, C_n8 |
| fixed-threshold band switching | threshold-based band switching | `fixed_threshold` | CAMP-FINAL A_main, B_end |
| Poucet-style LoRa+UWB fusion | Vey et al., IPIN 2021: every radio on, fused | `poucet_fusion` (TECH_DEBT N.24) | CAMP-FINAL A_main, B_end |
| CA-TBL single-hop | LoRaWAPS-style one-hop confidence | `ca_tbl_full` with multi-hop off | CAMP-FINAL G_single, G_single_end |

Every comparator runs inside this simulator, under the same channel, MAC and energy model. None
is a reimplementation of a published system's numbers, and none is compared with a published
figure. The table measures **the methods on our deployment**.

**Not represented:** RSSI fingerprinting (no site survey exists in the simulator) and TDoA
without RSSI (tier 1 always fuses its RSSI ranges).

## 2. Cells (`lora_cmp_run.sh`)

Each cell uses the preset at 1 800 s with traces on, seeds 1-20. Every cell carries its
CAMP-FINAL partner's flags plus `--tier1Tdoa=false`, and only the `static_868` arm.

| cell | partner | extra flags |
|---|---|---|
| R_main | A_main | `--reportAirtime=true` |
| R_end | B_end | `--boxHalf=750 --finderMode=1 --reportAirtime=true` |
| R_n8 | C_n8 | `--nGateways=8 --gwLayout=legacy` |
| X_repro | A_main seed 1 | `--arms=static_868` without `--tier1Tdoa=false`: the reproduction check |

61 cells in total. A `--specProfile` dependent of `--tier1Tdoa` (`--subjectTier1Tdoa`) is
unused, not refused, when the prerequisite is turned off explicitly.

**Isolation.** As CAMP-FINAL: a git worktree at this commit holding a byte copy of
CAMP-FINAL's frozen build. Its SHA-256 is checked against CAMP-FINAL's and listed in RUN.md.

**Pairing.** Each arm resets its own random stream. So a single-arm cell's `static_868` is
the same run as CAMP-FINAL's seven-arm cell's. This was checked before registration: A_main
seed 1 `static_868` run alone reproduced CAMP-FINAL's `arm_summary.csv` row and its
`subject_summary.csv` rows byte-for-byte. X_repro repeats that check inside the campaign.
Every comparator therefore shares seeds 1-20 with CA-TBL and is paired by seed.

## 3. Metrics (`lora_cmp_analyse.py`)

For each comparator and configuration, computed per seed with CAMP-SPEC's matrix definitions:
- availability, with a 95 % seed bootstrap;
- tag 0 CEP50 and CEP95;
- subject CEP95;
- TTFF;
- subject J.

Added:
- **J per subject fix:** summed subject energy divided by summed subject fix cycles.
- **Paired differences from CA-TBL**, per seed, mean with a 95 % bootstrap: comparator −
  CA-TBL on availability, tag 0 CEP95 and subject J.

The numbers are also written to `lora_cmp_table.csv`.

## 4. Predictions, made before any R cell

**Disclosed before registration:**
- Seed 1 at 5 km was run both ways, outside `results/`. With RSSI only, tag 0 CEP50 / CEP95
  was 22.4 / 26.8 m. With RSSI + TDoA it was 728.6 / 2 455 m, with 7 TDoA solves OK out of
  334 tier-1 fix cycles. That is a single seed, not a result (rule 8).
- The analysis was exercised on that run, and so printed CAMP-FINAL's comparator rows. Those
  are already-collected data. The predictions below concern only the R cells.

| # | prediction |
|---|---|
| **L1** | At 5 km, RSSI-only `static_868`'s mean tag 0 CEP95 is below RSSI + TDoA's (559.2 m) |
| **L2** | At 5 km, CA-TBL is not more accurate than RSSI-only: the interval of Δ CEP95 (RSSI-only − CA-TBL) does not lie entirely above 0 |
| **L3** | In the endgame, CA-TBL fixes more often than RSSI-only: the interval of Δ availability (RSSI-only − CA-TBL) lies below 0 |
| **L4** | In every configuration, RSSI-only's mean subject J is within 5 % of RSSI + TDoA's. TDoA adds no frame and no charge (the flag's own claim) |
| **L5** | On N8_ring, RSSI-only's availability is within 0.03 of RSSI + TDoA's (0.319) |

Each is written up as held or failed in `report/LORA_CMP_RESULTS.md`.

## 5. Abort conditions (the analysis's "Problems")

Any of the following stops the write-up. The cause is fixed, and no cell is dropped:
- a cell exits non-zero, or there are not 61 cells;
- a `run_id.txt` does not read one source with `dirty 0`;
- X_repro's row differs from CAMP-FINAL's;
- an R cell writes a `tdoa_` file (TDoA was not off);
- a CAMP-FINAL `static_868` cell lacks one (TDoA was not on);
- a comparator has fewer than 20 seeds.

## 6. What follows

Nothing is adopted: this measures comparators. If L1 holds, it means gateway TDoA at the
spec's three gateways worsens LoRa-only accuracy at 5 km. That is a finding about the preset's
tier 1, and it goes to TECH_DEBT for its own investigation. It is not fixed here.
