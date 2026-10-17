# CAMP-FINAL2 results: the headline tables on the final preset (scope D1)

Registration: `report/PREREG_CAMP_FINAL2.md` (commit `06ea4386`, before any cell). 221 cells, all `rc 0` and
`source 06ea4386 dirty 0`. The binary was frozen with a SHA-256 manifest, checked before every cell. Launched
2026-10-17T02:41Z, done 02:56Z. Tracked: `results/camp_final2_2026-10-17/` (arm and subject summaries, sidecars,
ToA table, the generated tables). Analyser: `scripts/camp_final2_analyse.py`. No abort condition fired.

**Not the same as the unregistered draft** on `draft/camp-final2` (`c432134e`, 2026-10-08), which added
`lora_fusion` as an eighth arm. That draft was never registered. This campaign keeps CAMP-FINAL's seven arms.

**These are the current numbers.** RULES.md, the Technical Report and the thesis cite them from here.
`report/CAMP_FINAL_RESULTS.md` (2026-10-04) keeps its dated text.

## 1. Predictions
| # | prediction | outcome | figure |
|---|---|---|---|
| Q1 | A: no UWB-delivered fix | **not held** | 7 fixes on 3 of 20 seeds (CAMP-FINAL: 0) |
| Q2 | PCS against trace: INDISTINGUISHABLE in A, BETTER in B | **not held** (B) | A −0.0037 [−0.0299, 0.0237]; B −0.0007 [−0.0066, 0.0046], both INDISTINGUISHABLE |
| Q3 | integrity precision < 0.5 under every fault | **held** | max 0.324 (`rb_common`, downweight) |
| Q4 | B: `poucet_fusion` subject energy ≥ 5× `ca_tbl_full` | **held** | 373.35 / 21.67 J = 17.2× |
| Q5 | single-hop depth-0 median within 10 % | **not held** | 12.1 vs 15.6 m (−22 %), in the single-hop baseline's favour (CAMP-FINAL: −12 %) |
| Q6 | A 868 digest saving below 0 | **held** | −33.14 [−37.35, −28.86] s |
| Q7 | A − H RESPONSE per answer above 0 | **not held** | +0.091 [−0.173, +0.365] s: interval spans 0 |
| Q8 | A availability `ca_tbl_full` > `static_868` + 0.02 | **held** | 0.352 vs 0.299 |
| Q9 | A post-acquisition CEP95 `ca_tbl_full` − `static_868` below 0 | **held** | −208.1 [−334.7, −79.5] m (257 vs 465 m) |
| Q10 | B `static_2p4` CEP95 ≤ 2× hybrid at ≤ 0.5× energy | **held** | 8.7 vs 4.6 m; 8.96 vs 25.98 J |

Six held, four not.

## 2. The headline, in one table (tag 0, mean over 20 seeds)
| arm | 5 km CEP95 | 5 km CEP95 after 300 s | 5 km J | endgame CEP95 | endgame after 300 s | endgame J |
|---|---|---|---|---|---|---|
| `static_868` | 493.5 | 464.7 | 8.4 | 52.4 | 42.1 | 9.5 |
| `static_2p4` | 396.8 | **96.6** | 13.4 | 8.7 | 7.7 | 9.0 |
| `static_uwb` | 773.0 | — | 445.8 | 88.7 | 2.5 | 402.1 |
| `fixed_threshold` | 339.2 | 227.2 | 17.7 | 5.7 | 5.4 | 28.1 |
| `ca_tbl_full` | 324.3 | 256.6 | 16.6 | **4.6** | **4.3** | 26.0 |
| `poucet_fusion` | **276.3** | 187.7 | 459.8 | 4.7 | 4.7 | 382.7 |

## 3. Reading
- **What changed against CAMP-FINAL (2026-10-04):**
  - The 5 km hybrid CEP95 went from about 1150 to 324 m, mostly through V7's reply positions.
  - The endgame hybrid CEP95 went from tens of metres to 4.6 m.
  - The 868-alone arm is worse in the endgame (52 m), because its error model is now correlated shadowing.
- **The hybrid against its comparators:**
  - **5 km:** best on availability, but not on CEP95. `poucet_fusion` is lower (276 m) at 28× the energy. After
    acquisition, 2.4 GHz alone is far lower (97 m), over a fix share of 1.7 %.
  - **Endgame:** best on CEP95 (4.6 m). `poucet_fusion` ties it (4.7 m) at 15× the energy. 2.4 GHz alone is 8.7 m
    at a third of the energy.
- **PCS earns nothing against the trace** in any cell. The `fixed_threshold` arm is within 1–15 m at 5 km and 1 m in
  the endgame, consistent with B4.
- **Calibrated Wolf (W_end)** repeats TECH_DEBT O.7 at 20 seeds. The hybrid is 53.4 m at 89 J; 2.4 GHz alone is
  28.3 m at 8.8 J. Under Wolf errors the hybrid's endgame is worse than 2.4 alone, and costs ten times as much.
- **The integrity check** fires with precision 0.05–0.32 and recall 0.02–0.13. Its in-window CEP95 is lower than
  without it in every fault, but its out-of-window CEP95 is higher (414–426 against 202–243 m), so overall it costs
  accuracy. This is consistent with its withdrawal as a claim (DESIGN_DECISIONS, thesis §8.8).
- **Relayed depth:** at 5 km, depth-1 fused sets have a median error of 37.6 m against 15.6 m at depth 0. Relays
  remain the weaker class even with `--relayNisInflate`.

## 4. Generated tables
## A_main

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | tier occupancy 868/2.4/UWB | subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.352 [0.316, 0.388] | 133.3 / 324.3 | 721.6 | 8 | 0.173 / 0.826 / 0.001 | 18.72 |
| ca_tbl_no_integrity | 0.348 [0.314, 0.383] | 140.1 / 337.2 | 734.3 | 8 | 0.246 / 0.750 / 0.005 | 19.78 |
| fixed_threshold | 0.343 [0.308, 0.379] | 128.2 / 339.2 | 768.8 | 8 | 0.235 / 0.761 / 0.004 | 19.19 |
| static_868 | 0.299 [0.275, 0.325] | 375.7 / 493.5 | 747.5 | 8 | 1.000 / 0.000 / 0.000 | 8.13 |
| static_2p4 | 0.017 [0.011, 0.025] | 111.1 / 396.8 | 559.0 | 242 | 0.000 / 1.000 / 0.000 | 13.20 |
| static_uwb | 0.000 [0.000, 0.000] | 713.6 / 773.0 | 756.2 | 1800 | 0.000 / 0.000 / 1.000 | 445.63 |
| poucet_fusion | 0.344 [0.315, 0.377] | 103.9 / 276.3 | 638.4 | 8 | 1.000 / 0.000 / 0.000 | 462.93 |

PCS-error correlation against the trace control (per seed Spearman on tag 0's fix cycles; A = rho_control - rho_PCS, so > 0 means PCS ranks error better):

- ca_tbl_full: A = -0.0037 [-0.0299, 0.0237] over 20 seeds -> INDISTINGUISHABLE
- ca_tbl_no_integrity: A = 0.0034 [-0.0001, 0.0104] over 20 seeds -> INDISTINGUISHABLE

Base-station batch session: time to first coarse median 127 s, to precise median 1800 s (never = 1800); precision spread UNDEFINED on 20 of 20 seeds.

## B_end

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | tier occupancy 868/2.4/UWB | subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.606 [0.582, 0.629] | 1.9 / 4.6 | 43.9 | 2 | 0.555 / 0.404 / 0.041 | 21.67 |
| ca_tbl_no_integrity | 0.604 [0.580, 0.628] | 1.9 / 4.9 | 45.5 | 2 | 0.574 / 0.387 / 0.040 | 20.85 |
| fixed_threshold | 0.609 [0.584, 0.632] | 1.9 / 5.7 | 51.6 | 2 | 0.547 / 0.403 / 0.050 | 21.86 |
| static_868 | 0.422 [0.406, 0.439] | 15.6 / 52.4 | 157.7 | 5 | 1.000 / 0.000 / 0.000 | 8.34 |
| static_2p4 | 0.278 [0.263, 0.291] | 2.1 / 8.7 | 51.2 | 2 | 0.000 / 1.000 / 0.000 | 5.35 |
| static_uwb | 0.000 [0.000, 0.000] | 61.3 / 88.7 | 105.0 | 1800 | 0.000 / 0.000 / 1.000 | 390.11 |
| poucet_fusion | 0.573 [0.554, 0.592] | 1.9 / 4.7 | 84.4 | 2 | 1.000 / 0.000 / 0.000 | 373.35 |

PCS-error correlation against the trace control (per seed Spearman on tag 0's fix cycles; A = rho_control - rho_PCS, so > 0 means PCS ranks error better):

- ca_tbl_full: A = -0.0007 [-0.0066, 0.0046] over 20 seeds -> INDISTINGUISHABLE
- ca_tbl_no_integrity: A = 0.0000 [0.0000, 0.0001] over 20 seeds -> BETTER

Base-station batch session: time to first coarse median 30 s, to precise median 1800 s (never = 1800); precision spread UNDEFINED on 5 of 20 seeds.

## C_n8

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | tier occupancy 868/2.4/UWB | subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.464 [0.433, 0.495] | 44.5 / 169.9 | 574.2 | 5 | 0.220 / 0.776 / 0.004 | 16.99 |
| static_868 | 0.404 [0.377, 0.435] | 75.8 / 246.0 | 617.6 | 5 | 1.000 / 0.000 / 0.000 | 7.99 |

PCS-error correlation against the trace control (per seed Spearman on tag 0's fix cycles; A = rho_control - rho_PCS, so > 0 means PCS ranks error better):

- ca_tbl_full: A = -0.0152 [-0.0417, 0.0126] over 20 seeds -> INDISTINGUISHABLE

Base-station batch session: time to first coarse median 154 s, to precise median 1800 s (never = 1800); precision spread UNDEFINED on 20 of 20 seeds.

## W_end

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | tier occupancy 868/2.4/UWB | subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.589 [0.563, 0.614] | 22.5 / 53.4 | 87.1 | 2 | 0.052 / 0.732 / 0.216 | 53.15 |
| static_2p4 | 0.241 [0.229, 0.253] | 13.1 / 28.3 | 68.8 | 2 | 0.000 / 1.000 / 0.000 | 5.26 |
| static_868 | 0.422 [0.407, 0.439] | 15.6 / 52.4 | 157.7 | 5 | 1.000 / 0.000 / 0.000 | 8.34 |

PCS-error correlation against the trace control (per seed Spearman on tag 0's fix cycles; A = rho_control - rho_PCS, so > 0 means PCS ranks error better):

- ca_tbl_full: A = 0.0000 [-0.0000, 0.0001] over 20 seeds -> INDISTINGUISHABLE

Base-station batch session: time to first coarse median 25 s, to precise median 756 s (never = 1800); precision spread UNDEFINED on 0 of 20 seeds.

## CEP95 after acquisition (B14: tag 0 fixes after 300 s; mean over seeds, NaN-aware)

| cell | arm | CEP95 (m) | CEP95 after 300 s (m) | tag-0 J |
|---|---|---|---|---|
| A_main | ca_tbl_full | 324.3 | 256.6 | 16.62 |
| A_main | ca_tbl_no_integrity | 337.2 | 239.9 | 18.13 |
| A_main | fixed_threshold | 339.2 | 227.2 | 17.73 |
| A_main | static_868 | 493.5 | 464.7 | 8.38 |
| A_main | static_2p4 | 396.8 | 96.6 | 13.36 |
| A_main | static_uwb | 773.0 | nan | 445.84 |
| A_main | poucet_fusion | 276.3 | 187.7 | 459.76 |
| B_end | ca_tbl_full | 4.6 | 4.3 | 25.98 |
| B_end | ca_tbl_no_integrity | 4.9 | 4.7 | 25.31 |
| B_end | fixed_threshold | 5.7 | 5.4 | 28.07 |
| B_end | static_868 | 52.4 | 42.1 | 9.52 |
| B_end | static_2p4 | 8.7 | 7.7 | 8.96 |
| B_end | static_uwb | 88.7 | 2.5 | 402.10 |
| B_end | poucet_fusion | 4.7 | 4.7 | 382.70 |
| C_n8 | ca_tbl_full | 169.9 | 95.5 | 15.40 |
| C_n8 | static_868 | 246.0 | 126.2 | 8.51 |
| W_end | ca_tbl_full | 53.4 | 52.6 | 89.43 |
| W_end | static_2p4 | 28.3 | 24.9 | 8.80 |
| W_end | static_868 | 52.4 | 42.1 | 9.52 |

A_main: batch precision spread defined on 0 of 20 seeds.

B_end: batch precision spread defined on 15 of 20 seeds, median 974 s.

A_main: ca_tbl_full UWB-delivered fixes (tag 0 and subjects) 7, on 3 of 20 seeds.

B_end: ca_tbl_full UWB-delivered fixes (tag 0 and subjects) 1068, on 20 of 20 seeds.

## E: the SY-5 fault matrix (tag 0; label = the fault window)

| fault | recovery | arm | integrity precision | integrity recall | CEP95 in window (m) | CEP95 outside (m) | seeds |
|---|---|---|---|---|---|---|---|
| antenna | downweight | ca_tbl_full | 0.054 | 0.023 | 74.9 | 414.6 | 10 |
| antenna | downweight | ca_tbl_no_integrity | nan | 0.000 | 102.4 | 202.3 | 10 |
| antenna | none | ca_tbl_full | 0.059 | 0.025 | 91.2 | 426.2 | 10 |
| antenna | none | ca_tbl_no_integrity | nan | 0.000 | 102.4 | 202.3 | 10 |
| rb_common | downweight | ca_tbl_full | 0.324 | 0.131 | 88.3 | 414.1 | 10 |
| rb_common | downweight | ca_tbl_no_integrity | nan | 0.000 | 160.2 | 242.6 | 10 |
| rb_common | none | ca_tbl_full | 0.282 | 0.107 | 102.9 | 426.2 | 10 |
| rb_common | none | ca_tbl_no_integrity | nan | 0.000 | 160.2 | 242.6 | 10 |
| rb_single | downweight | ca_tbl_full | 0.304 | 0.122 | 113.3 | 414.1 | 10 |
| rb_single | downweight | ca_tbl_no_integrity | nan | 0.000 | 121.6 | 225.4 | 10 |
| rb_single | none | ca_tbl_full | 0.280 | 0.113 | 128.0 | 426.2 | 10 |
| rb_single | none | ca_tbl_no_integrity | nan | 0.000 | 121.6 | 225.4 | 10 |
| txdrop | downweight | ca_tbl_full | 0.131 | 0.046 | 33.8 | 414.0 | 10 |
| txdrop | downweight | ca_tbl_no_integrity | nan | 0.000 | 59.8 | 233.9 | 10 |
| txdrop | none | ca_tbl_full | 0.114 | 0.037 | 50.7 | 426.2 | 10 |
| txdrop | none | ca_tbl_no_integrity | nan | 0.000 | 59.8 | 233.9 | 10 |

## G: the single-hop confidence baseline (LoRaWAPS-style) against the preset (ca_tbl_full, tag 0)

| configuration | hop depth of the fused set | fix cycles | median error (m) | rho(PCS, error) |
|---|---|---|---|---|
| preset 5 km | 0 | 11748 | 15.6 | -0.517 |
| preset 5 km | 1 | 417 | 37.6 | -0.552 |
| preset 5 km | 2 | 112 | 22.1 | -0.509 |
| preset endgame | 0 | 12181 | 1.8 | -0.330 |
| preset endgame | 1 | 1104 | 1.6 | -0.391 |
| preset endgame | 2 | 1028 | 1.6 | -0.087 |
| single-hop 5 km | 0 | 12249 | 12.1 | -0.470 |
| single-hop endgame | 0 | 14308 | 1.7 | -0.347 |

## Airtime (ca_tbl_full; --reportAirtime)

The digest against full pushes, per tier: saving = FULL_PUSH_CF - (DIGEST + ENTRY_PUSH), per seed, so > 0 means the digest exchange took less airtime than pushing the whole table at every digest would have.

| cell | tier | DIGEST + ENTRY_PUSH (s) | full-push counterfactual (s) | saving (s), mean [95 %] | seeds |
|---|---|---|---|---|---|
| A_main | 0 | 78.73 | 45.59 | -33.14 [-37.35, -28.86] | 20 |
| A_main | 1 | 6.46 | 12.77 | 6.31 [5.48, 7.16] | 20 |
| B_end | 0 | 123.37 | 65.95 | -57.42 [-61.54, -53.44] | 20 |
| B_end | 1 | 13.20 | 15.60 | 2.40 [1.56, 3.30] | 20 |

The reverse path against flooding (5 km): A_main's ca_tbl_full against H_flood (the same preset with --reversePath=false), paired by seed. RESPONSE airtime summed over the tiers; per answer is that over the base station's answers delivered.

| quantity | reverse path (A) | flooding (H) | A - H, mean [95 %] | seeds |
|---|---|---|---|---|
| RESPONSE airtime (s) | 66.04 | 53.00 | 13.04 [10.32, 15.81] | 20 |
| RESPONSE airtime per answer delivered (s) | 2.810 | 2.719 | 0.091 [-0.173, 0.365] | 20 |

Time-on-air table: /home/nethi/catbl_work/campaigns/camp_final2/T_toa/s1/toa_table.csv

## Predictions evaluated by this script

- **Q1**: not held -- A UWB-delivered fixes 7
- **Q6**: held -- A 868 digest saving -33.14 [-37.35, -28.86] s
- **Q7**: not held -- A - H RESPONSE per answer 0.091 [-0.173, 0.365] s
- **Q8**: held -- A availability ca_tbl_full 0.352 vs static_868 0.299
- **Q9**: held -- A post-acq CEP95 ca_tbl_full - static_868 -208.1 [-334.7, -79.5] m
- **Q10**: held -- B static_2p4 CEP95 8.7 vs ca_tbl_full 4.6 m; J 8.96 vs 25.98

Problems: 0 (none)
