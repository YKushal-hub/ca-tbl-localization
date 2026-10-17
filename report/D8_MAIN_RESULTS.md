# D8-MAIN — tier 2 on its own data-sheet row, measured on the preset

Registered in `report/PREREG_D8_MAIN.md` (`52b8ebac`) before any cell; 120 of 120 cells, every
`run_id.txt` `source 52b8ebac dirty 0`; scored once by the registered scorer, unedited, "Problems: 0"
(`results/d8_main_2026-10-17/D8_MAIN.md`). Preset at `93695ac` (102 settings, the N.26 fixes in),
six arms per cell, seeds 1–20, `--t2SensitivityRowKHz=812.5` (**on**) against the committed table
(**off**), paired by seed. The defect is TECH_DEBT N.27.

## Registered outcomes

| | prediction | outcome |
|---|---|---|
| R1 | off cells reproduce FIXES-N26's ALL cells, `ca_tbl_full` row | **HOLDS** — 60 of 60 identical |
| P1 | `static_2p4` availability rises in A, B, C | **HOLDS** — +0.043, +0.059, +0.055, every interval above 0 |
| P2 | `lora_fusion` availability rises in A and B | **FAILS** — B +0.031 [0.016, 0.045] holds; A +0.012 [−0.005, 0.028] does not |
| P3 | `ca_tbl_full` availability rises in A | **HOLDS** — +0.041 [0.033, 0.048] |

## What the correction moves

**CA-TBL** (`ca_tbl_full`), paired on − off:

| config | availability | Δ CEP95 subjects | Δ subject J |
|---|---|---|---|
| A 5 km | 0.285 → 0.326, +0.041 [0.033, 0.048] | −15.4 m [−22.8, −9.6] | −3.21 J [−3.99, −2.43] |
| B endgame | 0.562 → 0.584, +0.022 [0.013, 0.030] | +2.5 m [−4.0, 12.1] | +1.08 J [−1.36, 3.48] |
| C N8 ring | 0.414 → 0.458, +0.044 [0.030, 0.057] | −11.6 m [−15.0, −8.4] | −3.64 J [−4.44, −2.79] |

A more sensitive tier 2 raises availability for every arm that uses 2.4 GHz, and at 5 km and on the
ring it **lowers** subject energy by 3–4.5 J for all of them (`static_868`, which does not range on 2.4 GHz, moves by
under 0.05 J) — plausibly fewer futile 2.4 GHz attempts; that mechanism is not measured here. UWB-
delivered fixes at 5 km rise from 3 (2 seeds) to 14 (6 seeds).

## The comparison table, before and after

CA-TBL − comparator, paired by seed:

| config | contrast | availability off | availability on | CEP95 tag 0 off | CEP95 tag 0 on | subject J off | subject J on |
|---|---|---|---|---|---|---|---|
| A | − LoRa fusion | +0.060 [0.037, 0.081] | **+0.088** [0.060, 0.116] | −36.5 [−63.2, −13.6] | −39.3 [−78.4, −10.3] | −0.12 [−0.72, 0.55] | **+0.57** [0.13, 1.03] |
| A | − Poucet | +0.046 [0.022, 0.070] | **+0.081** [0.056, 0.105] | −38.3 [−59.4, −20.0] | −52.8 [−116.6, −7.0] | −445.0 | −444.9 |
| B | − LoRa fusion | +0.066 [0.055, 0.077] | +0.057 [0.045, 0.070] | **−2.5** [−4.9, −0.2] | −0.9 [−2.3, 0.6] | +13.5 [12.0, 15.3] | +14.7 [12.4, 17.4] |
| B | − Poucet | +0.051 [0.041, 0.062] | +0.059 [0.044, 0.074] | **−3.0** [−4.8, −1.2] | −0.9 [−2.0, 0.2] | −392.5 | −389.8 |
| C | − LoRa fusion | +0.021 [0.005, 0.038] | **+0.037** [0.016, 0.058] | −3.7 [−11.8, 4.0] | +0.8 [−7.3, 6.6] | +0.14 [−0.46, 0.80] | +0.36 [−0.06, 0.79] |
| C | − Poucet | +0.018 [0.002, 0.035] | **+0.040** [0.016, 0.064] | −6.1 [−12.9, −0.2] | −4.1 [−13.7, 3.2] | −444.5 | −444.4 |

Read plainly:
- **The availability claim survives and strengthens.** CA-TBL's lead over both comparators stays
  significant everywhere and widens in A and C (roughly doubling over Poucet).
- **The endgame accuracy claim does not survive.** At B, CA-TBL's CEP95 advantage over both
  comparators was significant with the defect and is not without it (−0.9 m, intervals spanning 0).
- **At 5 km CA-TBL now costs more than LoRa fusion** (+0.57 J per subject, interval above 0), where
  with the defect there was no difference.
- The Poucet energy gap (~445 J, UWB always on) is unchanged.

## What this cannot establish

Whether the corrected row should become the default (the owner's decision: it moves every tier-2
number in the tree, and this campaign says which claims move with it); tier 1 or tier 3 sensitivity;
bandwidths other than 800 kHz. Every number above is also conditional on DC-1's near-perfect gyro
(TECH_DEBT N.28), which DC1-BIAS measures separately.
