# PREREG FIDELITY-1 — two receiver corrections, measured on the design requirement

**Registered before any cell runs** (rule 13). Written 2026-10-20. This commit adds this file,
`scripts/fidelity1_run.sh` and `scripts/fidelity1_analyse.py`, and nothing else. Outputs go outside the repository, to
`~/ns/catbl-work/fidelity1_2026-10-20/`, which does not exist at registration.

## 1. What is corrected, and why each is a correction rather than a choice

| flag | what it corrects | source |
|---|---|---|
| `--uwbSensitivityDbm=-99.3` | Tier 3's −104 dBm sensitivity is OURS (TECH_DEBT 2.4). | the DW3000 data sheet the project cites (TECH_DEBT N.41) |
| `--iqOverloadOnly=true` | Every engine subtracts an IQ "SNR degradation", `20 log10(1 + EVM)`, from rx power after the loss sum and before comparing with a **data-sheet** sensitivity. That counts the receiver's own noise and IQ/ADC impairments twice: a data-sheet figure is the input power at which the real receiver meets its PER with those impairments inside it. The EVM's thermal-noise term saturates the subtraction at 6.02 dB on every UWB link. Only ADC overload, which a sensitivity figure does not cover, keeps its penalty. | SX1276 Table 3-10, SX1280 Table 6-1 and DW3000 sensitivities are conducted measurements; `experiments/IQ_TERM_REGISTRATION.md` measures the term |

**Their effect on UWB nearly cancels.** −99.3 dBm costs 4.7 dB, and the double count was worth about 6 dB. They
are run together so that the preset never carries one correction without the other: either alone would move tier 3
by 5–6 dB in one direction, and neither alone describes the receiver.

**On 868 MHz and 2.4 GHz the IQ correction alone acts.** IQ-TERM measured it at 0.6 dB at high SNR, rising to 5.3 dB
(868 MHz) and 6.0 dB (2.4 GHz) at the edge of range, so both tiers' reach grows. On UWB it was a saturated
5.9–6.0 dB, so the net UWB change is about +1.3 dB.

## 2. Adoption is decided before the numbers, and not by them

**Rule.** F1's two settings join `--specProfile` when two conditions hold:
- G161 is green: `--iqOverloadOnly` raises the margin at every probe point, by the IQ term on average, and
  `--uwbSensitivityDbm` moves only UWB, by 4.7 dB;
- the null control is byte-identical with both flags off (13 of 13 at `635c0ab9`).

Neither condition reads this campaign.

IQ-TERM's Q4, the per-point form of the first check, **failed** on stream divergence. Its result section says why,
and G161's docstring records the redesign. The rule uses the mean form, because the per-point form tests the noise
of two diverged streams, not the flag.

**Reason.** These are fidelity corrections. A preset that is more pessimistic than the receiver it models is not
safer, it is wrong. Choosing physics by its effect on A(25 m) would be selecting the world to flatter the
requirement. The campaign measures the size of the change, so every later claim states it, and the predictions below
test our understanding of the mechanism.

## 3. Cells (`fidelity1_run.sh`)

**Every cell** runs REAL-BEST's S stack in the realistic world:

    --specProfile=true --duration=1800 --trace=true --traceCovariance=true --unknownStart=true --fixedVarThreshold=12
    --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2 --imuFilter=ekf5 --imuMagSigmaDeg=3
    --ekf5FinderConsider=true --ekf5RelayGate=true --reportAirtime=true

with all five arms.

**The base is chosen by rule and recorded in `RUN.md`'s launch line:**
- **N:** PRESET-2's N set, if `PRESET2.md` reads "N becomes the preset: MET".
- **W:** `--ekf5Window=60` alone otherwise (the window is adopted by owner decision; TECH_DEBT Q).

**Configurations:**
- A: 5 km.
- B: endgame, `--boxHalf=750 --finderMode=1`.
- C: the 8-gateway ring, `--nGateways=8 --gwLayout=legacy`.

**Variants:**
- F0: the base.
- F1: the base + `--uwbSensitivityDbm=-99.3 --iqOverloadOnly=true`.

**Seeds:** 141–160, not run by any campaign. **Total:** 3 × 2 × 20 = **120 cells**, one frozen binary checked
before every cell.

## 4. Predictions (`fidelity1_analyse.py`)

| # | prediction | basis |
|---|---|---|
| **R1** | F1 lowers CA-TBL's five-node A(25 m) in no configuration: every paired lower bound above −0.03 | both corrections add margin on tiers 1–2 and net +1.3 dB on tier 3 |
| **R2** | F1 moves CA-TBL's tag-0 A(25 m) in B by under 0.02 either way: the paired interval lies inside [−0.02, 0.02] | B is UWB-driven, and on UWB the two corrections nearly cancel |
| **R3** | F1 lowers CA-TBL's subject energy in A: the paired interval is below 0 | more margin on tier 1 means fewer failed exchanges, retries and escalations |

**Against them, stated now:**
- R2 is a narrow interval, and rule 8 noise may be wider than 0.02 at 20 seeds. A failure of R2 by width is reported
  as such.
- More tier-1 reach can also mean more contention, which could raise energy (R3).

## 5. Abort conditions

- not 120 cells, or a cell's `rc` not 0;
- a `run_id.txt` not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 141–160;
- a cell whose flags do not match its variant;
- any window solver failure;
- an arm with fewer than 20 seeds anywhere.

## 6. What this cannot establish

- **Real hardware.** Neither correction is validated against a measured link here.
- **Whether −99.3 dBm is the right DW3000 row** for the configured data rate and preamble. It is the figure the
  project cites, used as cited.
- **The relay round's event slice (M.1(b)) and anchor-pair CAD (3.5).** Both are cut from this campaign for time;
  their exclusion is recorded in TECH_DEBT with that reason.
