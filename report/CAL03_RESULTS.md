# CAL-03 — results: at three gateways the fit keeps the trace term, which now ranks error the right way; the calibrated preset behaves like the legacy one and is adopted by the registered rule; fixed_threshold still has the higher availability

**Registered in `report/PREREG_CAL03.md` (`a7d8e90`, engine `e4ae4a1`) before any cell
ran.** All 120 cells (80 gen, 40 eval) exit 0 and read `source a7d8e90e dirty 0`. They ran
from a worktree at the registration commit with a frozen copy of the binary (`RUN.md`). The
analysis is `scripts/cal03_analyse.py`, CAL-02's computation unchanged, applied unchanged.
Its outputs are `FIT.md`, `fit.json` and `EVAL.md` in `results/cal03_2026-10-04/`.

---

## 1. Weights

- **Data:** 40 833 `ca_tbl_full` fix cycles; 2 678 with no geometry (GDOP 0, PCS 0)
  excluded, as registered. The back-derived t3 passed the abort check.
- **FIT-R on the fit seeds 1-20 (5 km)** picks the direction +(1, −0.1, 0.0018). Clamped to
  non-negative and normalised to sum 3: **(2.995, 0, 0.005)**, the trace term almost alone
  (rank agreement with t1 0.993).
- **Held-out seeds 21-40:** ρ(calibrated score, error) −0.280, ρ(−traceSigma, error)
  −0.281. A = −0.0008 [−0.0027, +0.0010] → **INDISTINGUISHABLE**.

CAL-02, at eight gateways on a ring, found the trace ranking error backwards at 5 km (ρ
+0.315 held-out; TECH_DEBT N.16) and dropped it. At the spec's three gateways it ranks error
the right way (ρ −0.281), and the fit keeps it. N.16 does not recur in this configuration.

## 2. Thresholds (pooled 5 km + endgame, fit seeds 1-20)

| threshold | positive / negative cycles | status | value | LOSO F1 | held-out F1 |
|---|---|---|---|---|---|
| `pcsThrUp868` | 163 / 1 722 | FITTED | 1.642 | 0.188 | 0.153 |
| `pcsThrUp2p4` | 0 / 17 044 | UNFITTED (legacy rate 0.980) | 1.377 | — | — |
| `pcsThrUpUwb` | 0 / 724 | UNFITTED (legacy rate 1.000) | 1.679 | — | — |
| `pcsThrDown2p4` | 12 510 / 4 534 | FITTED | 0.921 → **1.542** (raised for hysteresis) | 0.954 | 0.966 |
| `pcsThrDownUwb` | 85 / 639 | FITTED | 0.546 → **1.880** (raised for hysteresis) | 0.156 | 0.239 |

UWB "would solve" is never true on a fit cycle: the two UWB-entry thresholds keep the legacy
firing rate, as at eight gateways.

## 3. Evaluation (held-out seeds 21-40, `ca_tbl_full`)

| config | arm | availability | CEP50 / CEP95 (m) | escalations | occupancy 868 / 2.4 / UWB | subject J |
|---|---|---|---|---|---|---|
| 5 km | calibrated | 0.141 | 22.9 / 64.3 | 61.5 | 0.194 / 0.805 / 0.001 | 13.24 |
| 5 km | legacy | 0.142 | 20.7 / 64.1 | 58.1 | 0.175 / 0.824 / 0.001 | 13.47 |
| 5 km | `fixed_threshold` | 0.151 | 39.3 / 113.1 | 46.0 | 0.465 / 0.535 / 0.000 | 11.66 |
| endgame | calibrated | 0.417 | 7.6 / 18.4 | 53.0 | 0.085 / 0.870 / 0.046 | 21.84 |
| endgame | legacy | 0.410 | 7.5 / 17.8 | 54.1 | 0.080 / 0.876 / 0.044 | 23.26 |
| endgame | `fixed_threshold` | 0.438 | 7.8 / 19.1 | 36.3 | 0.728 / 0.250 / 0.023 | 14.08 |

## 4. Predictions

- **P1 failed.** The fit did not drop the trace term: w1 = 2.995. The prediction rested on
  CAL-02's inverted trace (N.16), which three gateways do not reproduce.
- **P2 failed.** A is INDISTINGUISHABLE, not BETTER: the control no longer ranks error
  backwards, so the calibrated score (essentially the trace) cannot beat it.
- **P3 held.** `pcsThrUp2p4` and `pcsThrUpUwb` are UNFITTED (0 positive cycles).
- **P4 held.** Calibrated against legacy availability: −0.001 at 5 km, +0.007 in the endgame.

## 5. Adoption (CAL-02's §8 rule)

The calibrated preset's mean availability is not lower than the legacy preset's by more than
0.01 in either configuration, so **the preset takes the re-fitted values**, in their own
commit. `fixed_threshold` has the higher availability in both configurations (+0.010 at
5 km, +0.021 in the endgame), as CAL-02 and CAMP-SPEC found at eight gateways; that is a
finding for the final campaign, not a criterion here.
