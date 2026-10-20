# IQ-TERM — registration, written before any probe it reads was run

Registered prospectively. The verifiable timestamp is this file's commit, which precedes the probe
directories it names. The scorer, `iq_term.py`, is committed with this file and is not edited
afterwards.

## Why this exists

G11 has failed since the six-decibel sensitivity defect was fixed. G11a then falsified the census's
account of its residual (`G11A_REGISTRATION.md`):
- **UWB:** a term rising 1.3 → 5.0 dB over 2–20 m, where terrain is provably inactive, then flat at
  4.7–5.3 dB to 180 m. It is recorded OPEN, and eight candidates are ruled out.
- **868 MHz:** −3.9 dB at 100 m, with the opposite sign, decaying with distance.

`RULES.md` §5 carries the UWB one as "~3.7 dB unattributed".

**A term outside the loss sum that no census lists.** All three engines run
`result.rxPowerDbm -= iqRes.snrDegradationDb` after the loss sum (`lora-physics-engine.cc:645`,
`lora-2p4-physics-engine.cc:641`, `lora-uwb-physics-engine.cc:667`):
- IQ impairment is enabled on every tier (`lora-physics-parameters.cc:611`).
- The degradation is `20 log10(1 + EVM) + 15 dB·clip`
  (`lora-iq-impairment-submodule.cc:48`).
- EVM includes a thermal-noise term, `10^(−SNR/20)`, so the degradation grows as SNR falls and
  saturates at 6.02 dB once SNR, before processing gain, is below 0 dB.
- **On UWB that is every link:** the probe's `mean_snr_db` is +0.8 dB at 2 m and negative beyond.
- `LOSS_TERM_CENSUS.md` and `linkbudget.py` do not contain it.
- G11a's eight ruled-out candidates do not include it.

This is RULES.md rule 10's failure mode a second time: a quantity outside the sum the census
enumerated.

It is also a candidate **defect**. A data-sheet sensitivity is the input power at which the real
receiver, with its own noise and IQ/ADC impairments, meets its PER. Subtracting those impairments
from rx power before comparing with that figure counts them twice. `--iqOverloadOnly` (default off)
keeps the subtraction only under ADC clipping, which a sensitivity figure does not cover.

## What is run

Both runs use the current engine at default flags, with the probe's own invocation (as `probe_v7`),
plus:

| run | flags | outdir |
|---|---|---|
| D | `--probe=true --probeNominalMargin=true` | `~/ns/catbl-work/probe_iq_2026-10-20/default` |
| O | `--probe=true --probeNominalMargin=true --iqOverloadOnly=true` | `~/ns/catbl-work/probe_iq_2026-10-20/overload_only` |

Neither directory exists at registration. Scored once with
`python3 src/catbl-propagation-study/iq_term.py D/probe_range_sweep.csv O/probe_range_sweep.csv`.

**Quantities:**
- `mean_iq_degradation_db`: the term itself, rebuilt exactly from `evmPercent` and
  `adcClippingOccurred` over all trials.
- `mean_nominal_margin_db`: rx power minus the table sensitivity. This removes link hysteresis,
  which G11a corrected only analytically as `1.5·pdr`; rule 3 asks for the measured quantity.
- **The model:** `linkbudget.py`'s corpus-mean margin, as `validate.py` computes it. The probe
  is never passed to it.

## Predictions

**Basis, read before registering, from data that exists.** `validate.py` against `probe_v7` today. The model has
moved since G11a (`5018c7aa`), and the UWB plateau is now 2.35–3.64 dB, not 4.7–5.3. Adding back G11a's
hysteresis correction (`+1.5·pdr`) gives the gap the nominal margin should show:
- 868 MHz at 100 m: −3.45 dB;
- 2.4 GHz at 10–50 m: −0.71 to −1.20 dB;
- UWB at 2 m: −0.31 dB;
- UWB at 30–180 m: +3.05 to +3.66 dB.

Subtracting the IQ term estimated below gives the Q5/Q6 figures.

**The IQ estimate.** IQ degradation was estimated from `probe_v7`'s `mean_snr_db`, using the default IQ
parameters at their means. That is a saturating function evaluated at the mean, so it is biased high
where SNR is spread around 0 dB. The direction is stated, and the thresholds below leave room for it.

| # | prediction | falsifier |
|---|---|---|
| **Q1** | the IQ term is > 0 at every probe point of every tier — reached, not a control-flow zero (rule 4) | any point at 0 |
| **Q2** | UWB: IQ ≥ 5.0 dB at every point from 5 m, and ≥ 4.0 dB at 2 m (saturation) | any UWB point below |
| **Q3** | 868 MHz at 100 m and 2.4 GHz at 10–50 m: IQ in [0.4, 0.8] dB (the IQ-imbalance floor; estimate 0.55) | any outside |
| **Q4** | O's nominal margin exceeds D's by D's IQ term, within 0.5 dB, at every point: the flag removes exactly this term, with no clipping at probe distances | any point outside |
| **Q5a** | 2.4 GHz G11a points: model − (nominal + IQ) within ±3.0 dB | any outside |
| **Q5b** | 868 MHz 100 m: model − (nominal + IQ) **< −3.0 dB**. The IQ term does not explain 868's anomaly; it has the wrong sign | ≥ −3.0 |
| **Q5c** | UWB 2 m: model − (nominal + IQ) **< −3.0 dB** (estimate −5.3 to −5.9) | ≥ −3.0 |
| **Q6** | UWB 30–180 m: model − (nominal + IQ) in **[−4.0, −1.5] dB** (estimate −2.4 to −3.0). The saturated 6 dB term **over-explains** the ~3 dB plateau: once it is accounted for, the engine is ~2.5 dB *stronger* than the model, flat with distance | any outside |

**Registered consequence.** G11a on the decode-independent margin with the IQ term accounted is
predicted to **FAIL**: at 868 MHz 100 m, and on UWB at every point (Q5b, Q5c, Q6).
- **What Q1–Q4 holding establishes.** The IQ term is real, it is reached on every tier, and it is outside every
  census. It is worth about 6 dB on UWB and 0.5–3.8 dB on the narrow-band tiers.
- **What Q6 holding establishes.** It does **not** close UWB's ~3.7 dB unattributed residual (RULES.md §5). That
  residual is the sum of this term and an opposite-signed ~2.5 dB that is still uncensused. The entry is rewritten
  as two terms, one attributed and one open, rather than closed.
- **Q6 failing outside the band** means the estimate above is wrong, and the term's size is what the probe
  says.

**Tolerance reason (Q4).** Arms of one probe can consume the shared stream at different rates once a
decode outcome differs (rule 8). Each point's mean over 2000 trials with shadowing sd ≈ 7 dB has a
standard error of ≈ 0.16 dB. 0.5 dB is about three standard errors of the difference of two such
means. Stated before running, not fitted.

## What this does not do

- **It does not change G11 or validate.py.** G11's registered criterion stays as it is and G11
  stays red.
- **It does not add the IQ term to `linkbudget.py`.** That would be adding a term to make a
  reconciliation pass; the G11a corollary forbids it.
- **It does not adopt `--iqOverloadOnly`.** Whether the double count is a defect to remove is a
  physics question, argued in the flag's help text. Its effect on the goal (A ≥ 0.95 at R ≤ 25 m) is
  measured only by a registered campaign.

---

# RESULT — computed after the registration above was committed (`409c9ab7`)

Both probes ran from `409c9ab7`, dirty 0, launched 2026-10-20T12:13:02Z; copied to `results/probe_iq_2026-10-20/`
with `iq_term.py`'s output (`IQ_TERM.txt`). Scored once.

| # | verdict | measured |
|---|---|---|
| Q1 | **HOLDS** | the IQ term is > 0 at all 36 points (min 0.61 dB): reached on every tier |
| Q2 | **HOLDS** | UWB 5.88 dB at 2 m, 6.01–6.02 dB from 5 m: saturated |
| Q3 | **FAILS** | 868 MHz 100 m 0.65 dB, 2.4 GHz 10/25 m 0.61/0.73 dB; 2.4 GHz 50 m **1.04 dB**, above 0.8 |
| Q4 | **FAILS** | the shift tracks the term on average (mean of shift − IQ −0.14 dB over 36 points) but not per point (−1.49..+0.96): the two probes' streams diverge once a decode outcome differs (`shadow_sd_db` differs at the same point). The tolerance's stated reason, one stream's sampling, was wrong |
| Q5a | **FAILS** | 2.4 GHz 10–50 m: model − (nominal + IQ) **+14.1 to +16.4 dB** |
| Q5b | **HOLDS** | 868 MHz 100 m −4.29 dB |
| Q5c | **FAILS** | UWB 2 m **+11.6 dB** (predicted < −3) |
| Q6 | **FAILS** | UWB 30–180 m **+15.7 to +17.9 dB** (predicted −1.5..−4.0) |

**G11a with the term accounted: FAIL**, as registered, but not for the registered reason.

## What the failures are: the basis was a stale engine

Q3, Q5a, Q5c and Q6 were set from `validate.py` against `probe_v7` (Aug 27, `fbf2153`). Today's default probe
differs from `probe_v7` on tiers 2 and 3 by far more than any IQ term:

| point | `probe_v7` margin | today | `probe_v7` PDR | today |
|---|---|---|---|---|
| 2.4 GHz 10 m | 53.66 | 38.49 | 1.000 | 0.9995 |
| 2.4 GHz 100 m | 35.25 | 14.51 | 0.9995 | 0.881 |
| UWB 2 m | 25.77 | 8.03 | 0.996 | 0.781 |
| UWB 20 m | 2.44 | −15.09 | 0.620 | 0.075 |

868 MHz is identical to `probe_v7` at every column checked.

**`--probeTerms` attributes it.** Foliage averages 15.6 dB on 2.4 GHz at 10 m and 15.8 dB on UWB at 2 m.

**Cause.** `50fc8620` (2026-09-08), "one node cannot be in two environments at once", put the facility tiers in the
wide-area vegetation (`--vegFacility=1` reproduces earlier runs). That was a deliberate engine change, and it is not
a defect here.

**The defect is G11's.** `validate.py` and G11 still reconcile the model with `probe_v7`, an engine state that no
run has used since 2026-09-08. `linkbudget.py`'s `TIER_ENV` still puts tiers 2/3 in `RURAL_OPEN`. Every G11
number since then, including G11a's, describes the pre-09-08 engine.

This is RULES.md rule 9's failure mode: a check that scans present data cannot see that the data is stale.
Recorded in TECH_DEBT (R.1) with the fix: a fresh probe at today's defaults in a new directory, and a re-registered
reconciliation. G11 is not re-pointed here.

## What stands

- **The IQ term is real.** It is reached on every tier, is outside every census, and is worth 0.6–5.3 dB on
  868 MHz, 0.6–6.0 dB on 2.4 GHz and a saturated 5.9–6.0 dB on UWB (Q1, Q2).
- **`--iqOverloadOnly` removes it on average, and nothing else.** G161 IQOFF checks this; Q4's per-point form failed
  on stream divergence.
- **Whether it closes UWB's "~3.7 dB unattributed" residual cannot be answered from these runs.** That residual was
  measured against the pre-09-08 engine. The question moves to the re-registered reconciliation.
