# CAL-02 — results: the trace ranks error backwards at 5 km, so the fit drops it; the calibrated preset behaves like the legacy one, and is adopted by the registered rule

**Registered in `report/PREREG_CAL02.md` (`7af04f5`, engine `995eeb5`) before any cell
ran.** The outputs, commands and SHA-256 of every summary are in
`results/cal02_2026-10-03/` (`RUN.md`). All 120 cells (80 gen, 40 eval) exit 0 and read
`source 7af04f5 dirty 0`. The analysis is `scripts/cal02_analyse.py`, committed with the
registration and applied unchanged. Its outputs are `FIT.md`, `fit.json` and `EVAL.md`.

---

## 1. Weights

- **Data:** 54 097 `ca_tbl_full` fix cycles were read. 537 had no geometry (GDOP 0, PCS 0)
  and were excluded, as registered.
- **The back-derived t3** lay inside [−0.001, 1.001] on every one of the 53 560
  geometry-bearing cycles (24 961 main, 28 599 end), so no abort and the decomposition is
  exact.
- **FIT-R on the preset's fit seeds 1-20** (5 km) picks the direction −(1, −0.75, −0.42).
  The best-ranking score weights the trace term t1 **negatively**.
- `--pcsForm=calibrated` takes only non-negative weights, so w1 is clamped to 0, as
  registered. That leaves **(0, 1.920, 1.080)**: GDOP and the NLOS term only.
- **Held-out seeds 21-40:**
  - ρ(calibrated score, error) is −0.104 (the right sign, weak);
  - ρ(−traceSigma, error) is **+0.315**: the trace control has the wrong sign;
  - A = +0.419 [+0.229, +0.591] → **BETTER**, by the registered test.

  The verdict is real but hollow. The calibrated score beats a control that ranks error
  backwards. It does not rank error well.

**Why the trace is backwards at 5 km** (descriptive; read from the gen data):
- At 5 km, 98 % of tag 0's fixes are 868 MHz floor fixes, and nearly all are SUSPECT, so
  DC-7 inflates R on almost every one.
- Within those fixes the trace ranks error the wrong way (ρ(−trace, error) = +0.22 within
  868 MHz).
- In the endgame the trace works: ρ = −0.53 overall and −0.55 within 2.4 GHz.

So the registration fitted the weights where the trace is least informative, and the fit
discarded the term that carries the information everywhere else. TECH_DEBT N.16.

## 2. Thresholds (pooled preset + endgame, fit seeds 1-20)

| threshold | positive / negative cycles | status | value | LOSO F1 | held-out F1 |
|---|---|---|---|---|---|
| `pcsThrUp868` | 39 / 1 875 | FITTED | 3.000 | 0.039 | 0.045 |
| `pcsThrUp2p4` | 4 / 23 594 | UNFITTED (legacy rate 0.969) | 2.024 | — | — |
| `pcsThrUpUwb` | 7 / 1 462 | UNFITTED (legacy rate 0.999) | 1.501 | — | — |
| `pcsThrDown2p4` | 23 598 / 0 | UNFITTED (legacy rate 0.030) | 2.361 | — | — |
| `pcsThrDownUwb` | 1 398 / 71 | FITTED | 0.587 → **1.681** (raised for hysteresis) | 0.976 | 0.998 |

- Even pooled with the endgame, UWB "would solve" (`shouldUwb`) is true on 4 + 7 cycles
  of 25 000. So both UWB-entry thresholds keep the legacy rate.
- `pcsThrUp868`'s best F1 is 0.04: from 868 MHz there is almost never a 2.4 GHz three-anchor
  solve to escalate to, and the fitted threshold escalates on nearly every cycle.
- The fit-to-held-out shift is small where a threshold was fitted (0.039 → 0.045,
  0.976 → 0.998).

## 3. Evaluation (held-out seeds 21-40)

| config | arm | availability | CEP50 / CEP95 (m) | escalations | occupancy 868 / 2.4 / UWB | subject J |
|---|---|---|---|---|---|---|
| main | calibrated | 0.337 | 19.8 / 42.3 | 103.0 | 0.166 / 0.833 / 0.001 | 14.95 |
| main | legacy | 0.333 | 19.6 / 43.4 | 104.0 | 0.167 / 0.833 / 0.000 | 14.71 |
| main | `fixed_threshold` | 0.376 | 18.1 / 33.0 | 42.5 | 0.694 / 0.304 / 0.001 | 11.53 |
| end | calibrated | 0.635 | 5.3 / 9.4 | 14.7 | 0.003 / 0.908 / 0.088 | 35.45 |
| end | legacy | 0.637 | 5.4 / 9.2 | 14.0 | 0.005 / 0.911 / 0.084 | 33.48 |
| end | `fixed_threshold` | 0.642 | 4.2 / 7.3 | 49.7 | 0.638 / 0.319 / 0.043 | 22.41 |

- The calibrated preset behaves like the legacy one: the same tier occupancy to 0.004,
  and the same availability to 0.004.
- `fixed_threshold` is better than both on availability, CEP and energy, in both
  configurations. It spends most cycles on 868 MHz, and the PCS arms spend them on
  2.4 GHz.

## 4. Predictions

- **P1 failed.** The fitted weights are not the trace term. Rank agreement with t1 is
  −0.07, because at 5 km the trace ranks error backwards (§1).
- **P2 failed.** A is BETTER, not INDISTINGUISHABLE or WORSE. The calibrated score wins
  because the control is inverted, not because it is good.
- **P3 held.** `pcsThrUp2p4` and `pcsThrUpUwb` are UNFITTED: `shouldUwb` had 4 and 7
  positive cycles.
- **P4 failed.** The calibrated arm is no closer to `fixed_threshold` than the legacy one
  (mean occupancy difference 1.058 against 1.057 at 5 km, 1.269 against 1.266 in the
  endgame). Both PCS arms escalate to 2.4 GHz and stay there.

## 5. Adoption (the registered rule, §8)

The rule: adopt unless the calibrated preset's mean availability is lower than the legacy
preset's by more than 0.01 in either configuration. The differences are +0.004 (main) and
−0.002 (end), so **the preset adopts `--pcsForm=calibrated`** with the eight values of
`fit.json`. That is in its own commit, with G24 re-run.

The adoption changes almost nothing, and that is said plainly. Neither PCS form beats
`fixed_threshold` here, and the reason is upstream of the weights: the trace term is
anti-informative where most fixes are 868 MHz and SUSPECT. That is N.16, for the owner,
together with N.14's floor-first rule, which decides most of what the tiers do.
