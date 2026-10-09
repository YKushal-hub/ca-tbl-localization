# ACC-B3-US results — the unknown start (N.39)

2026-10-09. Exploratory grade. Registered in `report/PREREG_ACC_B3_UNKNOWN_START.md` (`ead864fb`) before the first
cell.
- **Runs:** `results/acc_b3_us_cit_2026-10-09`, a citable re-run (`source 53766acb dirty 0`), byte-identical to the exploratory runs these numbers were first computed from.
- **The s = 0 reference:** the citable `results/acc_b3_cit_2026-10-09`.

## Setup

- **Settings:**
  - **US:** `--unknownStartSigmaM=100`. Every filter starts off its seed point by N(0, 100²) per axis, under the
    100 m prior it already claimed.
  - **USB3:** US plus `--dualBaseRelayTier --caseDInfoRatio=3`.
- **Runs:** seeds 1–20, 1800 s, all five arms. A = 5 km; B = endgame.
- **Verdicts:** paired seed bootstrap. "fix" is five-node availability, "w25" five-node within 25 m, and CEP is
  tag 0's.

## 1. What an honest start does to every arm

Five-node within 25 m, s = 0 → s = 100:

| | A s = 0 | A s = 100 | B s = 0 | B s = 100 |
| --- | --- | --- | --- | --- |
| `ca_tbl_full` | 0.613 | 0.427 | 0.968 | 0.871 |
| `fixed_threshold` | 0.625 | 0.437 | 0.964 | 0.870 |
| `static_2p4` | 0.683 | 0.461 | 0.968 | 0.825 |
| `poucet_fusion` | 0.498 | 0.312 | 0.884 | 0.788 |
| `lora_fusion` | 0.517 | 0.347 | 0.887 | 0.758 |

Every arm loses about a third of its 5 km within-25 m share. **The seeded-at-truth start had been carrying all of
them, not only `static_2p4`.**

## 2. Predictions

| | prediction | outcome |
| --- | --- | --- |
| U1 | `static_2p4`'s 5 km within 25 m falls by ≥ 0.10 | **holds**: −0.222 [−0.283, −0.166] |
| U2 | 5 km, US: `ca_tbl_full` − `static_2p4` in within 25 m is a WIN | **fails**: −0.034 [−0.114, +0.044], tie |
| U3 | US: availability WIN over `static_2p4` in A and B | **holds**: +0.251 and +0.283 |
| U4 | endgame, US: within 25 m against `static_2p4` is not a LOSS | **holds**: +0.046 [−0.018, +0.106], tie; CEP95 −8.6 m WIN |
| U5 | 5 km: `ca_tbl_full`'s drop is smaller than `static_2p4`'s | **fails**: +0.036 [−0.045, +0.124], tie |
| U6 | USB3 − US, `ca_tbl_full`: no accuracy LOSS | **holds**: all ties; B availability +0.024 WIN; A energy −0.51 J WIN |

The registered falsifier for U2 fired. Even with an honest start, `ca_tbl_full` is not shown to be more accurate
than `static_2p4` at 5 km: the two tie.

## 3. `ca_tbl_full` against every arm under USB3 (same binary, same setting)

| | A (5 km) | B (endgame) |
| --- | --- | --- |
| `fixed_threshold` | all accuracy ties | all accuracy ties (within 25 m +0.046, tie) |
| `static_2p4` | availability WIN; accuracy ties | availability WIN; **within 25 m +0.053 [+0.004, +0.106] WIN**; CEP ties |
| `poucet_fusion` | WIN on availability, within 25 m, tag 0 within 25 m, CEP50, CEP95 | WIN on availability and within 25 m; CEP ties |
| `lora_fusion` | WIN on availability, within 25 m, tag 0 within 25 m, CEP50; CEP95 tie | WIN on availability and within 25 m; CEP ties |

**Under an honest start with the B3 flags, `ca_tbl_full` has no accuracy LOSS against any arm in either
configuration.** It is significantly better than `static_2p4` on endgame within 25 m, and it has the highest
availability everywhere.

Energy is unchanged in kind: `ca_tbl_full` costs more than `static_2p4` (+3.0 J at 5 km, +18.0 J in the endgame)
and more than `lora_fusion` in the endgame.

## 4. What this cannot establish

- **One start-error level.** s = 100 matches the prior the code already claims; it is not a measured search
  start.
- **The start model:** a single Gaussian draw per node, with no correlation between nodes.
- **Exploratory grade:** a single campaign, registered but not blind to the earlier diagnosis.
