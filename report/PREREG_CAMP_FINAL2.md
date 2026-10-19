# PREREG CAMP-FINAL2 — the headline tables regenerated on the final preset (scope D1)

**Registered before any cell runs** (rule 13). Written 2026-10-17. This commit adds this file,
`scripts/camp_final2_run.sh` and `scripts/camp_final2_analyse.py`, and nothing else; the engine is the parent's.

**What the parent's preset is:**
- DESIGN_DECISIONS 10: the v3 fixes;
- DD 11: SX1262 + SX1280;
- DD 13: `--fadingPerBand`, `--relayNisInflate`.

The outputs go outside the repo (`~/catbl_work/campaigns/camp_final2`). Tracked summaries go to
`results/camp_final2_2026-10-17/`, which does not exist at registration.

**Why.** Every tracked headline number predates the current preset. CAMP-FINAL (2026-10-04, `report/CAMP_FINAL_RESULTS.md`)
is the comparison table the spec's Phase 5 asks for. This re-runs its cells unchanged on the final engine, so the thesis
and the reports can cite current numbers.

**What differs from CAMP-FINAL's cells:**
1. Every cell adds `--unknownStart=true --imuQuScale=10 --fixedVarThreshold=12`, the common flags of every campaign
   since 2026-10-09 (V-stages, UPG-SCOPE).
2. A new cell, **W_end**: the endgame with calibrated Wolf ToF (`--tofErrorModel=wolf --tofCalibrate=true`) on
   `ca_tbl_full`, `static_2p4` and `static_868`, seeds 1–20.
3. The analysis adds the post-acquisition CEP95 (B14).

The binary is frozen into the outdir with a SHA-256 manifest, checked before every cell.

**Disclosed before registration:**
- UPG-SCOPE's BASE cells: the same preset minus DD 13, with A and B on four arms.
- The V8 and V7 results.
- A smoke test of the analyser on a throwaway 60 s grid outside results/. It is not data for this registration.

## 1. Cells
- `T_toa`.
- `A_main` and `B_end` (seven arms, `--reportAirtime`).
- `C_n8`.
- `G_single` and `G_single_end`.
- `H_flood`.
- `W_end`.
- The fault matrix `E_<fault>_<recovery>`: 4 faults × 2 recoveries × seeds 1–10.

All 1 800 s, seeds 1–20 unless stated: 1 + 7 × 20 + 80 = 221 cells.

## 2. Predictions
Q1–Q7 are CAMP-FINAL's P1, P2, P3, P4, P5, P6 and P7, worded as there. Two carry over the sign CAMP-FINAL
observed: Q2 and Q7.

| # | prediction |
|---|---|
| Q1 | A: `ca_tbl_full` has no UWB-delivered fix on any seed |
| Q2 | PCS against the trace control: INDISTINGUISHABLE in A, BETTER in B |
| Q3 | integrity precision < 0.5 for `ca_tbl_full` under every fault |
| Q4 | B: `poucet_fusion` subject energy ≥ 5× `ca_tbl_full`'s |
| Q5 | single-hop depth-0 median within 10 % of the preset's at 5 km |
| Q6 | A, 868 MHz: the digest saving's interval lies below 0 |
| Q7 | A − H RESPONSE airtime per answer: interval above 0 (the reverse path costs more per answer; CAMP-FINAL P7's observed sign) |
| Q8 | A: `ca_tbl_full` availability exceeds `static_868`'s by more than 0.02 (UPG-SCOPE BASE fix share: 0.856 vs 0.780) |
| Q9 | A: `ca_tbl_full` post-acquisition CEP95 below `static_868`'s; the paired interval lies below 0 (UPG BASE: 243 vs 445 m) |
| Q10 | B: `static_2p4` CEP95 within 2× `ca_tbl_full`'s, at ≤ 0.5× its tag-0 energy (UPG BASE: 8.9 vs 5.6 m; 9.0 vs 28.5 J) |

Q1 and Q6–Q10 are evaluated by the analyser; Q2–Q5 are read from its tables.

## 3. Abort conditions
- A cell exits non-zero.
- A manifest mismatch.
- An A, B or H cell writes no `airtime_*` file.
- Any `run_id.txt` without `dirty 0`.

## 4. What follows
Nothing is adopted. The tables become the current numbers for RULES.md, the Technical Report and the thesis (D2, D3).
Older reports keep their dated text.
