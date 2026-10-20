# EKF5-WINDOW results — out of sample, the window smoother does not carry its 5 km gain; it sharpens the endgame

Registration: `report/PREREG_EKF5_WINDOW.md` (`61326646`, pushed before any cell). 80 cells on **seeds 61–80, never
run by this line before**. All `rc 0`, `source 61326646 dirty 0`, one frozen binary. Launched 2026-10-20T08:38:27Z,
done 08:57:43Z. Scored once, unedited, "Problems: 0". Tracked: `results/ekf5_window_2026-10-20/`.

Every cell runs REAL-BEST's S stack in the realistic world: ekf5, a magnetometer, finder consider and the relay
gate. **W** adds `--ekf5Window=60`. Every arm runs under the same variant.

## 1. Outcome

| # | prediction | outcome |
|---|---|---|
| P1 | W lowers CA-TBL's tag 0 CEP95 against S at 5 km | **not held**: −28.1 [−243.0, +138.1] |
| P2 | under W, CA-TBL's 5 km coverage ≥ 0.90 | **not held**: 0.391 → 0.596 |
| P3 | W raises CA-TBL's endgame availability | **not held**: +0.007 [−0.004, +0.020] |
| P4 | no significant 5 km availability cost | **held**: −0.005 [−0.021, +0.009] |
| P5 | under W, CA-TBL not significantly less accurate than Poucet, A and B | **held**: A +292 [−15, +662]; B +0.1 [−0.2, +0.5] |

**Decision rule (P1 + P3 + P4): not met.** `--ekf5Window` is not put to the owner as part of the preset. It stays a
default-off flag.

## 2. What it did

**5 km: the pilot's gain did not replicate.**
- These seeds are much harder than the pilot's. Under S, CA-TBL's mean CEP95 is 960.5 m (median 443.8), against
  216 m on seeds 1–6.
- Per seed, S → W:
  - helped: 66 3503 → 1793, 69 259 → 185, 70 36 → 16, 71 276 → 151, 73 169 → 12, 75 210 → 36;
  - hurt: 61 2211 → 2498, 62 3122 → 3264, 64 2915 → 3095, 76 505 → 950, 80 452 → 1179;
  - the rest within ±5 %.
- Five seeds hold over 1.5 km under both. A window over the filter's own measurements cannot correct a run that
  never had the geometry to fix a position.
- Consistency improves without accuracy following: median NEES 23.8 → 3.2, coverage 0.39 → 0.60.
- Under W, Poucet gained most at 5 km: 952 → 640 m, −311.8 [−658.8, −64.4]. All three tiers every cycle give the
  window more to resolve.

**Endgame: a significant accuracy gain that was not predicted.**
- CA-TBL 4.6 → 3.1 m, −1.5 [−3.3, −0.2], with coverage 0.888 → 0.961.
- `fixed_threshold` 4.5 → 3.0 m, −1.5 [−3.1, −0.3].
- Poucet 3.6 → 3.0 m, −0.6 [−1.0, −0.1].
- `static_868` 67.5 → 42.9 m, −24.6 [−52.6, −4.1].
- Under W, CA-TBL ties Poucet's endgame accuracy (+0.1 [−0.2, +0.5]) with higher availability (+0.028
  [0.014, 0.042]) at about 1/18 of the energy.
- These are not registered predictions. P3 asked for availability, which did not move. They are reported as
  observed, for a registration of their own.

**Energy.** The window changes no transmission. Subject J moves within noise for CA-TBL in both configurations
(A −0.13, B +1.26 J, intervals spanning 0).

## 3. What this establishes, and what it does not

- **It establishes** that N.62's mechanism, range innovations corrupting heading, bias and scale under one precise
  anchor, is not the dominant 5 km failure on seeds 61–80. Most of their 5 km error is runs that never acquire a
  usable geometry, which no estimator over the same measurements can repair.
- **It does not establish** the endgame gain: that needs its own registration.
- **It does not establish** the window length (60, OURS), compute cost on a field device, real sensors, the ring,
  or Wolf ToF.
- **The exploratory pilot (seeds 1–6) overstated the 5 km effect** by about an order of magnitude. Its seeds
  happened to be ones where the N.62 mechanism dominated. This is the second time in this line that a six-seed
  pilot has not carried out of sample (EKF5-CONSIDER's was the first).
