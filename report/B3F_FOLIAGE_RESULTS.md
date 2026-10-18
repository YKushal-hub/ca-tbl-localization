# B3F-FOLIAGE results — clamping the foliage depth to the link length (registered, exploratory)

Registration: `report/PREREG_B3F_FOLIAGE.md` (committed in `8396f3b2` before any cell). 10 probe runs (seeds 1-5,
clamp off/on), all `rc 0`, run on the binary built from the code at `e1c90578` (other new flags off; NC1-NC9
byte-identical). Tracked: each run's `probe_range_sweep.csv`, `run_id.txt`, `rc` (`results/b3f_2026-10-16/`).

| tier | r80 unclamped (m) | r80 clamped (m) | change | prediction |
|---|---|---|---|---|
| 868 MHz | 1622.5 | 1538.3 | −5.2 % | F1 (±1 %): **not held** |
| 2.4 GHz | 147.6 | 149.0 | +1.0 % | F3 (0-25 %): **held** |
| UWB | 0.0 | 7.4 | from none to 7.4 m | F2 (≥ 10 m): **not held** |

Reading:
- **UWB reach exists once the defect is removed** (K1's r80 of 0.0 m was the defect). It is 7.4 m, not the ≥ 10 m
  predicted, so foliage was not UWB's only limit.
- 2.4 GHz moves by 1 %, as expected for links mostly longer than the depth draw.
- 868 MHz: the clamp cannot change links longer than the draw's 500 m cap, so the −5.2 % is attributed to sampling.
  Changed receptions on short links displace later draws of the global stream (TECH_DEBT 1.1; G134 measured
  |ΔPDR| ≤ 0.017 at seed 7), and the r80 interpolation between 500 m-spaced probe rows magnifies it. Not established
  by a separate test; F1 is reported as not held.
- **S6 (master plan v2):** on the clamped probe the engine's delivery curve still departs from the pure-shadowing
  outage model Φ(M/σ_eff) by up to 0.16 (2.4 GHz) and 0.19 (868 MHz). For B4's `--dualBaseOnDemand`,
  `scripts/b4_fit_ondemand.py` fits the clamped 2.4 GHz curve (5 seeds pooled): **r50 = 328 m, σ = 9.1 dB**
  (rmse 0.017). These are the values B4 uses (PREREG_B4_SWITCHING §2).
