# S2-C NLOS audit — what the NLOS component does, what it is fed, and what it can know

**Code-reading plus post-hoc measurement from ACC-A's link logs** (`ca_tbl_full`, tag 0,
M_A_k10 and M_B_k10, seeds 1-20). Exploratory, not registered. Script:
`scripts/s2c_nlos_audit.py <ACC-A outdir>`. The ablation that tests the consequences is
registered separately: `PREREG_S2C_NLOS.md`.

## 1. The chain, as built (verified in code on this branch)

**PCS.** `ComputePcs` (`ca-tbl-policy.cc`) under the preset's CAL-03 fit:
- PCS = 2.994675 · traceRef/(traceRef + trace P) + 0 · (1/GDOP) + 0.005325 · term3;
- term 3 is the classifier's mean (1 − score) under `--pcsNlosClassifier`.

The NLOS term can move PCS by at most 0.0053. That is below every threshold gap, so in this preset
it does not change a decision (`ACC_A_DECISION_DIAGNOSIS` §1: PCS equals the trace formula on
every decision row).

**R.**
- `Step()` triples an NLOS-tagged range's engine sigma (`nlosSigmaInflation` 3, `:133`).
- The tag is `--nlosSource`. Under the preset that is `propagation`: the engine's own
  `PhysicsResult::isNLOS`, i.e. **the truth**.
- The detector's verdict feeds only:
  - PCS term 3;
  - the integrity check's all-LOS condition (`--integrityNlosFromScore`).
  It has never fed R.

**What the engine does to an NLOS range** (`lora-ranging-submodule.cc`):

| tier | NLOS range effect |
|---|---|
| 868 MHz (RSSI inversion) | **none**: the range is truth + N(0, σ); NLOS enters only through σ and through link loss |
| 2.4 GHz (ToF) | bias N(1.5, 0.8) m truncated to [0, 6]; floor 3.6 m against 3.0 m (both "NOT CITED") |
| UWB | bias lognormal, median 0.3 m, truncated to [0, 3]; floor 0.25 m against 0.10 m |

## 2. Measured from the logs

| cell | tier | detector | precision | recall | FPR | AUC (scored) | NLOS links scored / lost before a score |
|---|---|---|---|---|---|---|---|
| A | 868 | RSSI/SNR variability | 0.079 | 0.073 | 0.066 | 0.623 | 2 968 / 8 258 |
| A | 2.4 | ToF vs RSSI | 0.031 | 0.375 | 0.031 | 0.738 | 24 / 1 027 |
| A | UWB | CIR leading edge | — | — | — | — | 0 / 0 |
| B | 868 | RSSI/SNR variability | 0.047 | 0.085 | 0.067 | 0.670 | 2 943 / 2 721 |
| B | 2.4 | ToF vs RSSI | 0.019 | 0.214 | 0.032 | 0.777 | 98 / 2 142 |
| B | UWB | CIR leading edge | — | — | 0.000 | — | 0 / 1 (68 LOS scored) |

**2.4 GHz range residual (range − true distance):**

| cell | LOS | NLOS |
|---|---|---|
| A | −0.03 ± 5.42 m (n 9 457) | +0.42 ± 11.72 m (n 24) |
| B | +0.04 ± 4.66 m (n 35 421) | +0.64 ± 12.64 m (n 99) |

## 3. Reading

1. **Simulated NLOS shows up as link loss, not as biased ranges.**
   - 96-98 % of 2.4 GHz NLOS links are lost before a score.
   - The ones that survive are biased by under 1 m, against a 3-5 m LOS spread.
   - 868 MHz NLOS has no range bias at all.

   So there is little NLOS *range error* for any NLOS handling to correct.
2. **The oracle inflation is mostly a down-weighting of unbiased ranges.** At 868 MHz it divides the
   weight of ~3 000 truth-NLOS ranges per 20 runs by 9, though they carry no NLOS bias. Whether that
   costs accuracy is Q1 of the ablation.
3. **The fielded detectors cannot stand in for the oracle.** At precision 0.02-0.08, a
   detector-keyed inflation would inflate 12-50 LOS ranges for every NLOS one. Q2/Q3 of the ablation
   test the consequence.
4. **The UWB detector is unexercised.** No NLOS UWB link was scored in 40 runs (G24-style
   population gap).
5. **Not tested here: an 868 MHz RSSI-vs-path-loss residual detector.** With no NLOS range
   bias at 868 MHz in this engine, it has no measurement error to detect, so it is not a
   testable improvement until a configurable NLOS-bias model exists.
6. **A configurable NLOS-bias physics regime is not built here.** Calibration evidence would be
   needed, and it would be a change of scenario.
