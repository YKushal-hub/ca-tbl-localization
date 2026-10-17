# PREREG D8-MAIN — tier 2 on its own data-sheet row, measured on the preset

**Registered before any cell runs** (rule 13). Written 2026-10-17. Engine: `main` at `93695ac`
(`--t2SensitivityRowKHz`, landed `b78e4821` with its null control; gate G93). This commit adds this
file, `scripts/d8_main_run.sh` and `scripts/d8_main_analyse.py`, and nothing else. Outputs go to
`results/d8_main_2026-10-17/`, which does not exist at registration.

**Why.** TECH_DEBT N.27: tier 2's sensitivity table is the SX1280's 1625 kHz row (SF7 −106 dBm)
applied at 800 kHz, where the data sheet gives −112 dBm — ~6 dB pessimistic, at every bandwidth
SY-3 offers. G93 is red on it. Every published tier-2 number on `main` was produced with it,
including the comparison table (CA-TBL against Poucet and dual-band LoRa fusion), in which
2.4 GHz is carried by CA-TBL's dual base and by both comparators. This campaign measures what the
correction moves. **It adopts nothing:** making the row the default changes every result in the
tree and is the owner's decision, taken separately with this campaign's numbers in hand.

## 1. Cells (`d8_main_run.sh`)

`--specProfile=true --duration=1800 --trace=true`, the preset as it stands at `93695ac`
(102 settings, the N.26 fixes included), every comparator arm in one invocation:
`--arms=ca_tbl_full,static_868,static_2p4,fixed_threshold,poucet_fusion,lora_fusion`.

| config | flags beyond the preset | as |
|---|---|---|
| **A** 5 km | `--reportAirtime=true` | CAMP-FINAL A_main / FIXES-N26 ALL_main |
| **B** endgame | `--boxHalf=750 --finderMode=1 --reportAirtime=true` | B_end / ALL_end |
| **C** N8 ring | `--nGateways=8 --gwLayout=legacy` | C_n8 / ALL_n8 |

× condition **off** (no flag) and **on** (`--t2SensitivityRowKHz=812.5`, the row for the
preset's 800 kHz) × seeds **1–20**, paired by seed. **120 cells.** Arms are independent of the
arms beside them (G87's INDEP check), so one invocation per cell is the same as one per arm.

## 2. Quantities (`d8_main_analyse.py`), `main`'s own estimators

Per (config, condition, arm), `lora_cmp_analyse.per_seed`: availability (subjects' mean fix-cycle
fraction), tag 0 CEP50 / CEP95, subjects' CEP95, time to first fix, subject J; for `ca_tbl_full`,
UWB-delivered fixes (`camp_final_analyse.uwb_fixes`). Paired on − off per arm, mean with the
fixed-seed bootstrap of `camp_spec_analyse.ci`. And the comparison table's two contrasts —
**CA-TBL − dual-band LoRa fusion** and **CA-TBL − Poucet** — paired by seed, computed under off
and under on.

## 3. Predictions, made before any cell

| # | prediction | basis |
|---|---|---|
| **R1** | the **off** cells' `ca_tbl_full` rows of `arm_summary.csv` reproduce FIXES-N26's ALL_main / ALL_end / ALL_n8 rows byte for byte, 60 of 60 | `e308c14f`: "the preset alone reproduces FIXES-N26's registered ALL cell byte for byte"; every engine change since is default-off with a null control |
| **P1** | `static_2p4` availability rises in A, B and C — paired interval above 0 in all three | the flag adds 6 dB of tier-2 margin at every distance (local line probe); NC1 seed 7: 11 → 95 fix cycles |
| **P2** | `lora_fusion` availability rises in A and B — interval above 0 | it ranges 2.4 GHz every cycle |
| **P3** | `ca_tbl_full` availability rises in A — interval above 0 | under the preset's dual base, 2.4 GHz ranges join every fused set |

**Reported, not predicted:** every arm's paired change on every quantity; whether either
comparison-table contrast changes sign or significance; energy; UWB-delivered fixes; `static_868`
(it does not range on 2.4 GHz, but relays, the mesh and the shared MAC stream do carry 2.4 GHz
traffic — rule 8 — so no identity is asserted).

**R1 failing does not abort**: both conditions come from the same binary, so the paired
comparison stands. It is a finding about what changed since FIXES-N26, reported first.

## 4. Abort conditions (the analysis's "Problems")

Any of these stops the write-up; the cause is fixed and no cell is dropped:
- not 120 cells, or a cell's `rc` is not 0;
- a `run_id.txt` that does not read one source with `dirty 0` across the campaign;
- an **on** cell whose log lacks the `D-8:` line, or an **off** cell whose log has one;
- a (config, condition, arm) with fewer than 20 seeds.

## 5. Disclosure

Before this commit: G93's probe runs; NC1 at seed 7 (900 s, not the preset) with the flag on and
off, during the integration; and **one smoke cell per config A and B at seed 99** with the flag on,
to time a cell (≈ 5–6 min for six arms). Seed 99 is outside 1–20 and is not data. No other run of
the preset with the flag on exists. The local line's stage 1 (legacy topology) was read.

## 6. What this cannot establish

Anything about tier 1 or tier 3 sensitivity (tier 3 is unchecked, audit D5); whether the corrected
table should become the default (an owner decision, §0); bandwidths other than 800 kHz.
