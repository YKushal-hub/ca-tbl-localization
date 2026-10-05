# FIXES-N26 — results: with all four fixes CA-TBL beats Poucet and dual-band LoRa fusion on availability and accuracy; the registered rule does not adopt them, on 5 km energy alone

**Registered in `report/PREREG_FIXES_N26.md` (`afb8862`, engine `999f676`) before any cell
ran.** Phase 1 (40 calibration cells) and phase 2 (263 cells) all exit 0 and read
`source afb8862b dirty 0`. The reproduction cells match CAMP-FINAL's and LORA-CMP-2's rows,
and the registered analysis reports "Problems: 0".

Tables: `results/fixes_n26_2026-10-05/eval/FIXES_N26.md`. Numbers as CSV:
`results/fixes_n26_2026-10-05/eval/fixes_n26_table.csv`. Provenance:
`results/fixes_n26_2026-10-05/RUN.md`.

Throughout, **ALL** = `--loraDualBase --uwbCrossTierPoll --tdoaInnovationGate --mahalUseR
--integrityChi2=1992.9` on `ca_tbl_full`. Differences are paired by seed, mean [95 %], over
seeds 1-20.

---

## 1. Headline

| configuration | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|
| **5 km**, ALL − CA-TBL preset | **+0.111** [0.088, 0.135] | **−151.8** [−302.3, −31.8] | +4.32 [3.34, 5.28] |
| 5 km, ALL − Poucet | **+0.047** [0.024, 0.072] | **−161.6** [−384.7, −30.9] | −445.8 |
| 5 km, ALL − dual-band LoRa fusion | **+0.059** [0.033, 0.086] | **−353.1** [−700.2, −83.0] | −0.06 [−0.71, 0.65] |
| **endgame**, ALL − CA-TBL preset | **+0.139** [0.121, 0.159] | **−6.4** [−10.0, −3.5] | −1.93 [−4.30, 0.22] |
| endgame, ALL − Poucet | **+0.057** [0.047, 0.066] | −1.5 [−2.9, 0.0] | −393.3 |
| endgame, ALL − dual-band LoRa fusion | **+0.061** [0.049, 0.073] | **−3.6** [−6.6, −0.9] | +13.52 [11.97, 15.28] |
| **N8_ring**, ALL − CA-TBL preset | **+0.133** [0.116, 0.152] | +1.0 [−5.4, 7.9] | +2.41 [1.63, 3.16] |
| N8_ring, ALL − dual-band LoRa fusion | **+0.023** [0.005, 0.041] | +6.6 [−1.1, 15.0] | +0.09 |

The absolute values:

| | availability | tag 0 CEP50 / CEP95 (m) | subject J | UWB-delivered fixes (seeds) |
|---|---|---|---|---|
| 5 km: preset → ALL | 0.174 → **0.285** | 70.9 / 209.6 → **24.4 / 57.8** | 12.37 → 16.69 | 0 (0) → 3 (2) |
| endgame: preset → ALL | 0.423 → **0.562** | 8.4 / 18.4 → **5.7 / 12.0** | 24.91 → **22.97** | 0 (0) → **371 (20)** |
| N8_ring: preset → ALL | 0.281 → **0.414** | 19.5 / 43.7 → 18.5 / 44.7 | 12.78 → 15.18 | 0 → 2 (1) |
| Poucet, 5 km / endgame | 0.238 / 0.506 | 161.8 / 219.4, 6.4 / 13.5 | 462.46 / 416.24 | — |
| dual-band LoRa fusion, 5 km / endgame | 0.226 / 0.501 | 193.1 / 410.9, 6.6 / 15.6 | 16.75 / 9.45 | — |

**Against Poucet:**
- ALL fixes more often in both configurations.
- Its tag 0 CEP95 is lower at 5 km. In the endgame it is 1.5 m lower, with the interval's
  upper end at 0.0, so the endgame accuracy gap is at the edge of detection.
- It uses 1/28 and 1/18 of Poucet's subject energy.

**Against dual-band LoRa fusion:**
- ALL fixes more often everywhere.
- It is more accurate at 5 km and in the endgame, and indistinguishable on N8_ring.
- It uses the same energy at 5 km and on N8_ring, and +13.5 J in the endgame. That 13.5 J buys
  the UWB fixes: 371, on all 20 seeds.

## 2. What each fix carries (single-fix cells, A and B)

| fix alone | 5 km: Δ av / Δ CEP95 / Δ J | endgame: Δ av / Δ CEP95 / Δ J |
|---|---|---|
| D, dual LoRa base | **+0.118** / **−106.4** / +5.35 | **+0.170** / **−1.7** / +13.83 |
| U, UWB cross-tier poll | −0.003 / −0.1 / −0.24 | −0.000 / −1.1 / +0.52 |
| T, TDoA innovation gate | −0.003 / **−139.4** / −0.06 | +0.003 / −0.4 / −0.25 |
| M, integrity (mahalUseR + 1992.9) | +0.003 / +7.7 / +0.24 | **+0.040** / **−2.6** / **−7.51** |

How the fixes combine:
- **D carries the availability.** Alone, it also raises endgame energy: UWB is used much more
  (412 UWB fixes, 38.74 J).
- **M takes most of that back.** The preset's integrity check flagged nearly every fix, so
  it no longer drives escalations. Together, the endgame comes in below the preset's energy.
- **T carries 5 km accuracy.** N.25's wrong TDoA roots are refused.
- **U does nothing alone.** Its UWB ranges, fewer than three, are only usable once D fuses
  them.

## 3. The integrity check at the fitted threshold

**Fault-free.** SUSPECT on 1.6 % (5 km) and 0.5 % (endgame) of tag 0's fix cycles, against
the preset's 77.5 % and 96.9 %. The false alarms are gone.

**Under the injected faults (E, seeds 1-10)** it now catches almost nothing:

| fault | precision | recall |
|---|---|---|
| antenna | 0.143 | 0.015 |
| rb_common | 0.375 | 0.031 |
| rb_single | 0.265 | 0.023 |
| txdrop | 0.129 | 0.009 |

CAMP-FINAL's precision was 0.149 / 0.307 / 0.305 / 0.247, with recall 0.56-0.91 because it
flagged nearly everything. At a threshold of 1 992.9 the check stays silent in the fault
windows too.

Tag 0's CEP95 inside the window is 25-61 m. CAMP-FINAL's `ca_tbl_full` had 359-538 m and its
`ca_tbl_no_integrity` 62-72 m. So accuracy under the faults is better than either preset arm,
but not because the check detects the fault.

**Conclusion.** The distance's tail (§ FIT.md: p99 582-6 521 even with R in S) is the
remaining defect. The covariance the check uses still understates a minority of fixes'
error. *Hypothesis, not established:* NLOS ranges and finder pseudo-anchor positions whose
error is not in R. Until that is modelled, no threshold gives both a low false-alarm rate and
detection. TECH_DEBT N.15.

## 4. Predictions

| | registered | measured | verdict |
|---|---|---|---|
| Q1 | fitted integrityChi2 in [9.21, 60] | 1992.9 | **failed** |
| Q2 | A and B: availability gain interval above 0 | +0.111 [0.088, 0.135]; +0.139 [0.121, 0.159] | **held** |
| Q3 | B: ALL availability ≥ 0.491 | 0.562 | **held** |
| Q4 | B: ALL subject J < 24.91 | 22.97 | **held** |
| Q5 | B: UWB-delivered fixes on ≥ 10 of 20 seeds | 20 of 20 (371 fixes) | **held** |
| Q6 | A: tag 0 CEP95 mean per-seed change ≤ +0.10 | −0.235 | **held** |
| Q7 | fault-free SUSPECT share ≤ 0.05 in A and B | 0.016; 0.005 | **held** |
| Q8 | E: precision above CAMP-FINAL's under every fault | above only for rb_common (0.375 against 0.307) | **failed** |
| Q9 | the registered rule ADOPTs | DO NOT ADOPT | **failed** |

## 5. The adoption rule

| term | A (5 km) | B (endgame) |
|---|---|---|
| availability gain, interval lower bound > 0 | **pass** (0.089) | **pass** (0.121) |
| tag 0 CEP95 mean per-seed change ≤ +0.10 | **pass** (−0.235) | **pass** (−0.293) |
| subject J not above the preset's | **fail** (16.69 against 12.37) | **pass** (22.97 against 24.91) |

**DO NOT ADOPT.** The preset keeps its 97 settings. The one failing term is 5 km energy:
+4.32 J, about +35 %. It is carried by the dual base keeping the 2.4 GHz radio awake and
ranging every cycle, where 2.4 GHz rarely reaches (D alone: +5.35 J). Dual-band LoRa fusion
pays the same there (16.75 J).

Any change to the rule or to the flags after these results needs a new registration. A
candidate is ranging 2.4 GHz only when the floor is short or a 2.4 GHz neighbour was heard;
it is not established.
