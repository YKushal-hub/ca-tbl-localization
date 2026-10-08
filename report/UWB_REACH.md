# UWB reach — why tier 3 reaches tens of metres, decomposed one term at a time

**2026-10-09, S3 (HANDOFF_2026_10_08 item 7; TECH_DEBT N.41). Analysis only: no engine change,
no simulator run.**

To reproduce, run `python3 scripts/uwb_reach_ablation.py`. It reads nothing from the engine. It
runs the independent link-budget model (`src/catbl-propagation-study/linkbudget.py`, at
`5018c7aa` or later) over that study's frozen corpus. That is n = 2000, sha256 `ea400769…`,
regenerated with `corpus.py` and matching its tracked anchor. The engine's own r80 (12.8 m)
is from `results/probe_v7`, through `validate.py`.

**Why.** RULES.md's topology block rests on "UWB reaches tens of metres". Published UWB ranging
reaches further: Benouakta et al. 2023 (Architecture R-02) report 124 m. This note says which
terms make the difference, and which of them are ours.

## 1. The radio as simulated, against the datasheet

| quantity | engine | DW3000 datasheet v1.1 (Decawave 2020) |
|---|---|---|
| TX power | −16.47 dBm | −14.3 dBm (−41.3 dBm/MHz × 499.2 MHz) |
| sensitivity | **−104 dBm, uncited** (`lora-uwb-physics-parameters.cc:52-59`) | −99.3 dBm (ch 9, 850 kb/s, PSR 1024) |
| link budget | 87.5 dB | 85 dB (ch 9), 86 dB (ch 5), 0 dBi |
| antenna gain | 0 dB (`antennaGainDb` zeroed, RULES.md §5) | 0 dBi reference |

The engine's sensitivity is 4.7 dB better than the datasheet's, with no source. It is OURS until
cited. The datasheet figures are HANDOFF_2026_10_08's reading, not re-read in this session.

## 2. Free-space reach of the datasheet budget (no foliage, shadowing or fading)

| channel | 0 dBi | +3 dBi at each end |
|---|---|---|
| ch 9 (7.99 GHz), 85 dB | 53.1 m | 106.0 m |
| ch 5 (6.49 GHz), 86 dB | 73.3 m | 146.4 m |

A reach of 124 m needs antenna gain on ch 5, or more than +3 dBi each end on ch 9. The engine
zeroes antenna gain, so it cannot produce such a reach whatever else it models.

## 3. r80 in the independent model, one term removed at a time

| configuration | r80 (m) | r50 (m) |
|---|---|---|
| as simulated (TX −16.47 dBm, sens −104 dBm, 0 dBi, sparse shrubs with the engine's foliage scaling, shadowing 5 dB, Rician) | 20.7 | 35.3 |
| datasheet radio (TX −14.3 dBm, sens −99.3 dBm) | 15.4 | 26.5 |
| … and no foliage | 29.3 | 50.1 |
| … and no foliage, no shadowing | 40.6 | 51.6 |
| … and no foliage, no shadowing, no fading | 50.2 | 54.1 |
| simulated radio, no foliage | 39.2 | 67.1 |
| simulated radio, no foliage, no shadowing, no fading | 64.7 | 71.7 |
| *engine (probe_v7)* | *12.8* | — |

Reading down the datasheet rows:
- foliage costs 13.9 m of r80;
- shadowing costs 11.3 m;
- fading costs 9.6 m.

With all three removed, the model's 50.2 m is close to the free-space 53.1 m. That is the
consistency check on the model itself.

## 4. The engine's UWB foliage scaling, and what it does not explain

**What the handoff thought.** The engine multiplies foliage specific attenuation by 5.28 for UWB
(`lora-uwb-physics-parameters.cc:116`) and 2.17 for 2.4 GHz (`lora-2p4-physics-parameters.cc:77`).
`linkbudget.py` omitted both. Taken at face value, ×5.28 adds 4.15 dB at the median depth, close
to the open UWB ~3.7 dB residual.

**What the engine does.**
- Only the distribution's *mean* is scaled. Its bounds stay at 0.2γ and 1.8γ of the unscaled
  0.04 dB/m (`lora-physics-parameters.cc:318`).
- `SampleNormal` clamps (`lora-distribution-sampler.cc:76`), and `ComputeFoliageLossDb` draws
  γ through it (`lora-foliage-submodule.cc:21`).
- So the UWB draw, Normal(0.211, 0.01) clamped to [0.008, 0.072], sits at 0.072 with
  probability 1.000. The 2.4 GHz draw does so with probability 0.931.

| tier | written scaling | effective E[γ] | foliage at median depth | vs unscaled |
|---|---|---|---|---|
| 2.4 GHz | ×2.17 | **×1.79** | 4.69 dB | +1.60 dB |
| UWB | ×5.28 | **×1.80** | 4.70 dB | +1.61 dB |
| (×5.28 unclamped, not the engine) | — | ×5.28 | 7.24 dB | +4.15 dB |

**That is an engine defect, recorded and not fixed** (N.41). The written ITU-R P.833
extrapolation is silently capped at ×1.8, the same on both tiers. Fixing it changes the engine
and every result, so it is a planned change, not a bug fix.

**`linkbudget.py` now models it faithfully (`5018c7aa`).** With it, `validate.py` gives:

| tier | gap before → after | r80 model (engine) |
|---|---|---|
| UWB | 2 m −0.09 → −1.80; 20 m +4.20 → +2.49; 180 m +5.35 → +3.64 dB | 25.0 → 20.7 m (12.8) |
| 2.4 GHz | 10-430 m: −0.5..−1.3 → **−2.2..−3.0 dB** | 1593.8 → 1420.5 m (1149.5) |

- **The UWB residual does not close.** The change is a uniform −1.70 dB, and G11A's residual is a
  *rise* of ~3.7 dB across 2-20 m. A distance-independent term cannot produce a rise. The rise is
  unchanged at ~4.3 dB. G11A's ruling-out of foliage stands.
- **2.4 GHz now disagrees more.** In the near field, where terrain is provably inactive, the
  model is 2-3 dB too lossy. Something on the engine's 2.4 GHz side is not what the model
  assumes. It is open (N.41).
- Rule 3 is what found both: the 2.4 GHz cross-check is the quantity the UWB question did not
  use.

## 5. What is OURS in tier 3's reach

- The −104 dBm sensitivity (§1).
- 0 dB antenna gain, applied by zeroing the computed term (RULES.md §5).
- The f^0.75 extrapolation of foliage to 8 GHz. The source comment calls it uncertain, since
  the wavelength (3.75 cm) approaches leaf size. It is also capped at ×1.8 by the clamp (§4).

None of these changes the topology rule. Under every row of §3, a fixed gateway offers tier 3
only to something within tens of metres, and the finder is what carries it to the casualty.
