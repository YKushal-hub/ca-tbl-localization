# TOF-FIELD — results: at measured outdoor 2.4 GHz ranging error every arm loses accuracy; static 2.4 GHz loses most, and in the endgame the preset becomes the more accurate of the two

**Registered in `report/PREREG_TOF_FIELD.md` (`17472fe7`) before any cell ran.** All 160 cells
exit 0 and read `source 17472fe7 dirty 0`. The registered analysis, applied unchanged, reports
"Problems: 0". Tables: `results/tof_field_2026-10-09/TOF_FIELD.md`. CSV: `tof_field_table.csv`.
Provenance: `RUN.md` there (one resume after the session's 2 h limit).

**Settings:**
- D0 = the 3.0 m floor;
- R1 = Mueller rural, σ 83.6 m, bias +46.4 m;
- R0 = rural spread with the bias removed;
- S = suburban, σ 45 m, bias +39 m.

**Conventions:**
- Every comparison is within this binary, paired over seeds 1-20, as mean [95 % bootstrap].
- CEP is tag 0's per-seed CEP, averaged.
- "The preset" is `ca_tbl_full` on the 102-setting preset, which includes FIXES-N26's fixes.

---

## 1. Headline

| | 5 km (A) D0 → R1 | endgame (B) D0 → R1 |
|---|---|---|
| preset tag 0 CEP95 (m) | 57.8 → 93.0 | **12.0 → 39.3** |
| static 2.4 GHz tag 0 CEP95 (m) | 54.9 → 104.3 | **10.8 → 70.0** |
| static 2.4 GHz − preset, CEP95 | −2.8 [−22.8, 19.6] → +11.3 [−8.7, 35.0] | −1.2 [−3.0, 0.7] → **+30.7 [22.6, 39.2]** |
| preset − `fixed_threshold`, availability | **+0.011** [0.001, 0.020] → +0.002 [−0.003, 0.006] | **+0.017** [0.013, 0.021] → +0.007 [−0.004, 0.017] |
| preset − `fixed_threshold`, CEP95 | −0.1 [−9.2, 10.4] → +2.0 [−1.1, 5.7] | −0.1 [−1.6, 1.6] → +1.1 [−0.5, 3.0] |
| preset − `fixed_threshold`, subject J | −0.25 [−0.94, 0.29] → +0.63 [−0.47, 1.88] | +0.95 [−0.43, 2.34] → **+11.79 [8.17, 15.52]** |

1. **Every tier-2-dependent accuracy figure in the project is conditional on the 3 m floor.**
   - At Mueller's measured levels the preset's endgame CEP95 roughly triples: 12.0 m becomes
     39.3 (R1), 35.1 (R0) and 39.1 (S).
   - At 5 km it rises from 57.8 m to 79.7-96.2 m.
   - No arm escapes: the fusion comparators rise by 21.5-34.0 m in the endgame.
2. **Static 2.4 GHz's accuracy lead is not significant even at D0 here (Q12 failed), and under
   field error it reverses.**
   - In the endgame static 2.4 GHz is less accurate than the preset by 30.7 m (R1) and 16.6 m
     (S, [9.2, 23.5]).
   - With the bias removed (R0) the difference is +6.4 [−0.7, 13.3], not significant. So it is
     the unmodelled bias that static 2.4 GHz cannot dilute. The preset fuses 868 MHz, 2.4 GHz,
     relays and UWB.
   - At 5 km no setting separates the two on accuracy. The preset fixes 9-11× more often in
     every setting.
3. **The decision rule against `fixed_threshold` on the same measurements:**
   - small availability gains at D0 (+0.011 A, +0.017 B), so Q9 failed;
   - no accuracy difference in any setting except A R0 (−6.2 m [−12.9, −0.7]);
   - an energy cost under field error. In the endgame the preset spends +9 to +12 J per node more
     than `fixed_threshold` in R1, R0 and S. With a larger tier-2 σ, P grows, PCS falls and the
     preset escalates more. `fixed_threshold` also escalates on P, but less.
4. **Against the fusion comparators the availability lead holds under every setting**:
   - against dual-band LoRa fusion, +0.048 to +0.072;
   - against Poucet, +0.046 to +0.087.

   The accuracy lead is error-dependent:
   - against LoRa fusion it is significant only at D0 (−36.5 m A, −2.5 m B);
   - against Poucet it holds at 5 km in D0, R1 and R0 (−38.3, −36.8, −41.7 m), but in the
     endgame only at D0.
5. **Energy.**
   - The preset's subject energy rises with field error, most of all at R0, where σ is large and
     there is no bias: +11.95 J (A) and +29.98 J (B), endgame 22.97 → 52.96 J per node.
   - `fixed_threshold` follows (+11.4 / +20.1 J).
   - Static 2.4 GHz and LoRa fusion change by under 1 J. Poucet is ~420 J throughout.

## 2. Registered predictions

| # | prediction | verdict |
|---|---|---|
| Q1 | static 2.4 GHz R1 − D0 CEP95 > 0, A and B | **held** (+49.3 [19.0, 76.5]; +59.2 [49.3, 69.1]) |
| Q2 | static 2.4 GHz R0 − D0 CEP95 > 0, A and B | **failed**: A +18.1 [−11.0, 43.8]; B held (+30.7) |
| Q3 | \|Δ availability\| ≤ 0.02 at R1 for static 2.4 GHz, LoRa fusion and Poucet | **failed**: static 2.4 GHz B −0.031 [−0.037, −0.025]. Ranging error removes fixes; the mechanism is not identified here |
| Q4 | preset B R1 − D0 CEP95 > 0 | **held** (+27.3 [22.1, 33.4]) |
| Q5 | preset A R1 − D0 CEP95 > 0 | **held** (+35.2 [18.0, 52.7]) |
| Q6 | preset − LoRa fusion availability > 0, every setting, A and B | **held** |
| Q7 | preset − Poucet availability > 0, every setting, A and B | **held** |
| Q8 | B R1 preset − LoRa fusion CEP95 < 0 | **failed** (−0.3 [−7.8, 7.0]) |
| Q9 | D0 preset − `fixed_threshold` availability contains 0, A and B | **failed**: +0.011 and +0.017, both above 0 |
| Q10 | D0 preset − `fixed_threshold` CEP95 contains 0, A and B | **held** |
| Q11 | R1: Q9's and Q10's intervals contain 0, A and B | **held** |
| Q12 | B D0 static 2.4 GHz − preset CEP95 < 0 | **failed** (−1.2 [−3.0, 0.7]) |
| Q13 | R1 static 2.4 GHz − preset CEP95 not < 0, A and B | **held** |
| Q14 | B preset CEP95 at R1 > 2 × D0 | **held** (39.3 against 12.0) |

## 3. What changes where it is cited

- Any claim that static 2.4 GHz "beats" CA-TBL on accuracy rests on the 3 m floor. Here it is
  not significant even at that floor, and under measured field error it reverses in the endgame.
- Every CEP quoted for the endgame (12-18 m) is a 3 m-floor number. At field error it is
  35-40 m.
- The preset's advantage over `fixed_threshold` is availability, +0.01 to +0.02, at the floor.
  It costs energy under field error. Neither rule is more accurate, which matches
  `PCS_FORMS_RESULTS` (PCS adds nothing beyond P).
- The preset's availability advantage over both fusion comparators is robust to ranging error.

## 4. What this cannot establish

- **One machine.** No comparison is made with any other campaign. The D0 preset row (0.285;
  24.4 / 57.8 m) matches FIXES-N26's ALL_main row to the printed precision, but that is an
  observation, not a reproduction claim.
- **Mueller's SD applied as a floor at every distance** is OURS. The real error grows with
  distance, so near range is pessimistic here and far range optimistic.
- **The bias is constant and the EKF is not told.** A calibrated system (Mueller's 0.95 d + 1) is
  closer to R0 than to R1.
- **Q3's availability loss for static 2.4 GHz is unexplained.** It is recorded, not attributed.
