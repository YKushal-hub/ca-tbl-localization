# PREREG ML-PATHLOSS-POS — learned 868 MHz ranging, used for localisation, on real measurements

Registered 2026-10-21, before any position is solved. Script: `scripts/pathloss_ml_bonn_pos.py`, committed with this
file and not edited after the first run.

## 1. Question

ML-PATHLOSS (`report/PREREG_ML_PATHLOSS.md`, result at `61b0111d`) compared ranges, one packet and one gateway at a
time:
- ML was no better than per-gateway linear regression on RSSI and SNR (C4);
- both were far better than log-distance inversion (×/÷1.61 against ×/÷2.68–4.41).

An 868 MHz-only localiser, the paper's "LoRa-868 RSSI" comparator, turns those ranges into a **position** by
multilateration. Does the range gain survive into position, and does ML add anything there?

## 2. Data, models, folds

All three are ML-PATHLOSS's, imported unchanged from `scripts/pathloss_ml_bonn.py`:
- `load()`: the Bonn data, haversine truth and the 10 m filter;
- C1–C4 and ML;
- the spatially blocked 5-fold cross-validation, with the same seed, blocks and folds.

So each packet's ranges come from models that never saw its 1 km block.

**A packet** is the rows sharing date, packet number and GPS fix. One is kept if it was heard by at least 3
distinct gateways after the 10 m filter.

## 3. Estimator

For each packet and each range model, the position minimises Σ_k (log10 max(‖p − g_k‖, 1 m) − ℓ̂_k)²:
- ℓ̂_k is the model's log10 range to gateway k;
- the residual is in log space because the range error is multiplicative;
- the solver is `scipy.optimize.least_squares` with default settings;
- it starts from the gateways' centroid weighted by 10^(−ℓ̂_k).

**Positions** are on the same local equirectangular projection the blocks use. The error is the distance to the
packet's GPS fix.

**Range-free reference (G0):** the unweighted centroid of the receiving gateways.

## 4. Evaluation

- **Test statistic:** the mean log10 position error (a 1 m floor).
- **Also reported:** the median, 68 % and 95 % errors.
- **Intervals:** 95 % cluster bootstrap over blocks (10 000 resamples, NumPy seed 20261021) of per-packet
  differences.
- **Selections**, both made on the test packets (conservative against ML):
  - "best classical" is the C1–C4 model with the lowest mean log10 position error;
  - "best inversion" is the better of C1 and C2.

**Verdicts:**

| contrast | verdict rule |
|---|---|
| ML − best classical | **HELPS** if the interval lies below 0 and ML's median error is ≤ 0.90 × the other; **SMALL** if below 0 with a ratio > 0.90; **NO GAIN** otherwise |
| ML − best inversion | **GAIN** if the interval lies below 0, else **NO GAIN** |
| C4 − best inversion | **GAIN** if the interval lies below 0, else **NO GAIN** (whether the gain needs ML) |

## 5. Predictions (before solving)

These are made knowing ML-PATHLOSS's range result.

- **Q1.** ML − best inversion: GAIN.
- **Q2.** C4 − best inversion: GAIN.
- **Q3.** ML − best classical: NO GAIN or SMALL.
- **Q4.** ML and C4 have a lower median position error than G0.

## 6. Limitations

- **Urban, vehicle-mounted.** Bonn, trucks and 16 m gateways, as in ML-PATHLOSS. Not CA-TBL's rural search case.
- **A minority of packets.** Only packets heard by ≥ 3 gateways are localisable: 21 292 of 56 540 in a count made
  before the 10 m filter and before any solve.
- **No tracking or fusion.** One packet, one fix.
- **Prior from training routes.** The inverse-regression and ML ranges carry a prior from the training routes'
  distance distribution (ML-PATHLOSS, Reading 2). Their gain is measured where the test routes resemble the
  training routes.
