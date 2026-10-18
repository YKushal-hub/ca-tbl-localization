# B8 — why the delivery curve is flatter than Φ(M/σ) (S6), diagnosed

Scope item B8 (prof_review/model/PENDING_SCOPE.md). Engine `be9b8f21` (the binary built at that commit, run from
outside the repo, so its `run_id.txt` reads `source unknown`). Command per seed 1–3:

```
--specProfile=true --probe=true --probeTerms=true --probeMonotoneTime=true --seed=<s>
```

The preset includes `--rangeErrorNoLinkLeak` (N.55), and the probe runs with B5's monotone time (N.56). Data:
`report/b8/m{false,true}/s*/probe_terms.csv`. Table: `scripts/b8_curve.py report/b8/mtrue`.

## Finding
**There is no undiagnosed mechanism.** Delivery follows Φ(M/σ_margin), where σ_margin is the SD of the realised
margin. That SD is 8.6–16.7 dB, not the shadowing σ (6.8–8.9 dB) that S6 compared against.

- **868 MHz:** delivered = P(margin > 0) to ±0.002 at every distance. Φ(M/σ_margin) matches to ±0.02.
- **2.4 GHz and UWB:** the same to ±0.03.
  - The small residuals are both one-sided: delivery runs slightly below P(margin > 0) near the anchor (UWB 5 m:
    0.913 vs 0.947) and slightly above it in the far tail (2.4 GHz 1.6 km: 0.034 vs 0.021).
  - Both point to the two-way exchange's per-frame margins, not to a missing term.
- **What widens σ:** NLOS excess. Its SD grows with distance, from 1–2 dB near the anchor to 9–13 dB beyond 1.5 km,
  and it is 24–47 % of the margin variance beyond 400 m (2.4 GHz) and beyond 1 km (868 MHz). Foliage (about 4.3 dB
  once saturated), fading (about 4 dB) and path-loss geometry (2–3.5 dB) make up the rest.
- **The terms are not independent:** σ_margin exceeds the root-sum-square of the independent term SDs by 0.3–2 dB.
  They covary positively, through the shared distance and height draws.
- **N.56 (monotone time)** changes the curve by at most 0.05 (UWB 5–10 m). Fading SD on 2.4 GHz rises 3.5 → 3.9–4.7 dB.
  So the frozen-time probe understated fading, but it does not explain S6.

## Consequence
- Any outage rule that uses the shadowing σ alone (S6's comparison, a Φ(M/σ_shadow) predictor) is too steep. Beyond
  600 m on 2.4 GHz it predicts 0.08 where 0.23 is delivered.
- **What already uses the right curve:**
  - the fitted constants (`--onDemandR50M/--onDemandSigmaDb`, B4);
  - C1's `negR50M`/`negSigmaDb` = 9.1 dB.
  Both are least-squares fits to the delivered curve, so they absorb σ_margin on average. A distance-dependent
  σ(d) would fit the 2.4 GHz tail better: 9 dB at 100 m, 15 dB at 1.6 km.
- **No engine change.** The engine is behaving as modelled; S6 is closed as a comparison against the wrong σ. TECH_DEBT
  O.5 records this.

## Table (seeds 1–3 pooled, monotone time)
| tier | d (m) | delivered | P(margin > 0) | Phi(M/sd_margin) | Phi(M/sd_shadowing) | sd_margin | sd_shadowing | sd_nlos | sd_foliage | sd_fading | NLOS share of variance |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 2.4GHz | 50 | 0.989 | 0.994 | 0.999 | 1.000 | 8.9 | 6.8 | 1.9 | 1.4 | 3.8 | 0.05 |
| 2.4GHz | 100 | 0.902 | 0.920 | 0.927 | 0.988 | 10.5 | 6.8 | 2.5 | 3.2 | 3.9 | 0.08 |
| 2.4GHz | 150 | 0.779 | 0.803 | 0.795 | 0.912 | 11.5 | 7.0 | 3.2 | 3.9 | 3.8 | 0.11 |
| 2.4GHz | 200 | 0.696 | 0.712 | 0.689 | 0.806 | 11.8 | 6.7 | 3.5 | 4.2 | 3.9 | 0.13 |
| 2.4GHz | 300 | 0.525 | 0.529 | 0.504 | 0.506 | 12.2 | 6.9 | 4.6 | 4.3 | 3.9 | 0.20 |
| 2.4GHz | 430 | 0.376 | 0.369 | 0.349 | 0.242 | 12.6 | 7.0 | 5.4 | 4.3 | 4.0 | 0.24 |
| 2.4GHz | 600 | 0.234 | 0.228 | 0.224 | 0.081 | 13.1 | 7.1 | 6.4 | 4.2 | 4.0 | 0.30 |
| 2.4GHz | 900 | 0.112 | 0.103 | 0.116 | 0.011 | 14.0 | 7.3 | 7.9 | 4.2 | 4.1 | 0.37 |
| 2.4GHz | 1200 | 0.068 | 0.052 | 0.072 | 0.003 | 14.6 | 7.6 | 8.5 | 4.2 | 4.1 | 0.38 |
| 2.4GHz | 1600 | 0.034 | 0.021 | 0.044 | 0.000 | 15.4 | 8.0 | 9.6 | 4.3 | 4.2 | 0.42 |
| 2.4GHz | 2000 | 0.018 | 0.010 | 0.028 | 0.000 | 15.7 | 8.1 | 10.4 | 4.3 | 4.1 | 0.45 |
| 2.4GHz | 2500 | 0.011 | 0.004 | 0.019 | 0.000 | 16.7 | 8.5 | 11.2 | 4.3 | 4.1 | 0.47 |
| 868MHz | 500 | 0.984 | 0.984 | 0.996 | 1.000 | 11.5 | 7.0 | 4.2 | 4.4 | 4.0 | 0.16 |
| 868MHz | 1000 | 0.935 | 0.934 | 0.953 | 0.999 | 12.8 | 7.1 | 5.6 | 4.4 | 4.0 | 0.25 |
| 868MHz | 1500 | 0.847 | 0.847 | 0.859 | 0.980 | 14.2 | 7.4 | 6.5 | 4.5 | 4.1 | 0.29 |
| 868MHz | 2000 | 0.755 | 0.754 | 0.751 | 0.909 | 15.0 | 7.6 | 7.2 | 4.4 | 4.2 | 0.32 |
| 868MHz | 3000 | 0.582 | 0.581 | 0.563 | 0.621 | 15.8 | 8.1 | 8.1 | 4.4 | 4.3 | 0.35 |
| 868MHz | 4000 | 0.441 | 0.439 | 0.432 | 0.372 | 16.0 | 8.4 | 8.6 | 4.4 | 4.5 | 0.36 |
| 868MHz | 6000 | 0.254 | 0.253 | 0.257 | 0.115 | 16.3 | 8.9 | 9.1 | 4.3 | 4.6 | 0.36 |
| UWB | 2 | 0.987 | 0.995 | 0.994 | 1.000 | 9.8 | 7.0 | 0.0 | 0.0 | 3.8 | 0.00 |
| UWB | 5 | 0.913 | 0.947 | 0.940 | 0.977 | 8.9 | 6.9 | 0.6 | 0.0 | 3.8 | 0.01 |
| UWB | 10 | 0.689 | 0.727 | 0.719 | 0.767 | 8.6 | 6.8 | 1.3 | 0.1 | 3.7 | 0.03 |
| UWB | 20 | 0.288 | 0.254 | 0.247 | 0.194 | 8.6 | 6.8 | 1.6 | 0.3 | 3.9 | 0.04 |
| UWB | 30 | 0.101 | 0.062 | 0.067 | 0.030 | 8.4 | 6.7 | 2.1 | 0.6 | 3.9 | 0.06 |
| UWB | 50 | 0.014 | 0.002 | 0.007 | 0.001 | 8.7 | 6.9 | 2.5 | 1.4 | 3.9 | 0.09 |
