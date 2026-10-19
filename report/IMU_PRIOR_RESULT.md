# C3 — the inertial prior, measured

> **Local line, legacy topology.** Ported to `main` on 2026-10-16 as a record (`docs/LOCAL_LINE_INTEGRATION.md`). Every number below was produced on the local line's binary and the pre-correction topology `main` treats as a different deployment (`RULES.md`), on commits that resolve through `SHA_MAP.md` to `archive/local-line-2026-10-16`. Flags it names (`--imuPrior`, `--intraCycleHandshake`, `--imuCoastMomentMatch`) exist on that branch, not on `main`.

Registered in `report/PREREG_IMU_PRIOR.md` (`cb60c71`) before the first cell; A0 v2 in
`report/PREREG_A0_V2.md` (`a4d6bf4`). Campaign run at `cb60c71`: **120/120 invocations, every
sidecar `dirty 0` and digest-verified, scored once** by the registered scorer, unedited. Data and
scorer output committed at `89ac3b6` (`results/imu_prior/`, `report/imu_prior/`).

**One line.** A body-worn IMU doing pedestrian dead reckoning makes no-fix cycles **5–10×
more accurate when outages last up to about two minutes**, and **3× worse than constant velocity
when they last tens of minutes** — and the second is not bad luck but two identified mechanisms,
one of which is in the filter itself.

---

## 1. Registered outcomes

| | prediction | outcome |
|---|---|---|
| **P1 / R0** | static arms' radio side byte-identical cv vs imu | **HOLDS** — 0 of 180 row pairs differ. The IMU reached nothing on the radio side |
| **P2** | `imuJ` = 11.5236 J and 1440 predicts on every IMU row | **HOLDS** — 0 of 360 rows off |
| **P3** | no-fix error falls (imu/cv < 1, CI below 1) in every testable cell | **FAILS** — holds in 5 of 7 testable cells, reverses on `static_uwb` (below) |
| **P4** | deep-outage ratio ≤ 0.5, `static_2p4` at `boxHalf = 3000` | **HOLDS** — 0.092 [0.050, 0.192], n = 14 |
| **P5** | `A_est` rises, `static_2p4` and `static_uwb` at 3000 | **FAILS** — `static_2p4` +0.0248 [+0.0167, +0.0334] holds; `static_uwb` −0.0157 [−0.050, +0.023] does not |
| **P6** | escalations fall where the cv mean ≥ 10 | **FAILS in part** — `fixed_threshold` −172 (ref) and −125 (sparse), `ca_tbl_no_integrity` −4.6 (sparse) hold; `ca_tbl_full` +0.13 [−1.27, +1.53] at sparse does not |

### Error on no-fix cycles, imu / cv (geometric mean of per-seed median ratios)

| | `boxHalf = 2000` | `boxHalf = 3000` |
|---|---|---|
| `static_2p4` | **0.171** [0.137, 0.221], 4.21 → 0.65 m | **0.151** [0.131, 0.172], 7.34 → 1.13 m |
| `ca_tbl_no_integrity` | not testable (< 5 no-fix cycles) | **0.155** [0.122, 0.200] |
| `ca_tbl_full` | not testable | **0.182** [0.126, 0.262] |
| `fixed_threshold` | not testable | **0.672** [0.526, 0.862] |
| `static_868` | not testable | 0.482 [0.318, 0.711], n = 9 (below the 10-seed scoring bar) |
| **`static_uwb`** | **3.538** [2.237, 5.546], 129 → 613 m | **3.107** [1.909, 4.875], 209 → 947 m |

## 2. Not predicted, and reported because they matter

- **`fixed_threshold` becomes cheaper and more available.** Radio energy ×0.668 (ref) and ×0.721
  (sparse), escalations −94 % / −88 %, `A_fix` +0.037 at sparse. Its trigger is
  `traceSigma > 100 m²`; a dead-reckoning prior grows the covariance slower, so most of its
  escalations — largely futile — stop.
- **A motion prior rescues RSSI fixes.** `static_868` `A_fix` +0.302 (ref) and +0.174 (sparse):
  tier 1's ~50 %-of-range ranging errors are far better constrained against a dead-reckoned prior
  than against constant velocity.
- **The filter is consistent where fixes are frequent and overconfident where they are not.**
  Mean position NEES (2.0 = consistent): `static_2p4` 1.86 / 1.82, `ca_tbl_no_integrity` 1.81 /
  1.86 — consistent; `ca_tbl_full` 5.0 / 144; `static_868` 27 / 89; `static_uwb` ~9×10⁴ / 3×10⁴.

## 3. Why `static_uwb` reverses — two mechanisms, measured

`static_uwb` gets every fix inside a 10 m site ring and is otherwise out of coverage for up to
~29 minutes. Read from one trace (`ref_imu_s301`, longest outage 624 cycles): **at the moment the
outage begins**, the filter's gyro-bias estimate is wrong by ~1 400 °/h (0.39 °/s, 2.6 σ beyond
its own prior) and its stride-scale estimate by +10 % (3.3 σ). It then dead-reckons 2 km off
while reporting σ = 40 m.

A diagnostic (validator, synthetic, **ranging exactly as the filter models it**) separates two
causes:

1. **Through the site dwell the filter is consistent.** Dwell motion replicated — 0.45 m/s,
   ±1.2 rad/s heading noise, the 10 m ring with its projection and π flip, five anchors at
   σ = 0.1 m, 36 cycles: NEES 2.006; z-scores of the heading-offset, bias and stride-scale errors
   have RMS 0.99, 0.95, 1.06. **The confidently wrong states in the scenario are therefore not
   the filter's doing** — unmodelled error in the scenario's ranging (NLOS excess delay, the open
   ~3.7 dB UWB residual) is being absorbed into the sensor states.
2. **Over long outages the filter is overconfident even with a perfect model.** Outage after the
   dwell, NEES at its end and mean error:

   | outage | 25 s | 62 s | 125 s | 250 s | 500 s | 1000 s |
   |---|---|---|---|---|---|---|
   | NEES | 2.11 | 2.10 | 2.07 | 2.79 | 11.8 | 178 |
   | error | 1 m | 3 m | 7 m | 22 m | 67 m | 200 m |

   **Consistent to about two minutes; overconfident beyond about four, even here.** G17's filter
   test exercises 50 s outages and could not see it — a population gap in the gate, stated as
   such.

   **Root cause, measured.** At the end of the outage the *sensor* states are still calibrated —
   z-score RMS of heading offset, bias and stride scale 0.95–1.05 at 125 s, 500 s and 1000 s, and
   the filter's heading σ matches the realised heading error (17.7° believed vs 16.8° realised at
   500 s; 34.8° vs 32.9° at 1000 s). Only the **position** covariance is wrong. That is the
   classic EKF failure under heading uncertainty: a heading error δ rotates the dead-reckoned path;
   the cross-track part, L sin δ, is linear and captured, but the along-track shortening,
   L(1 − cos δ) ≈ Lδ²/2, is second-order and the Jacobian drops it. At 500 s (δ ≈ 17°,
   L ≈ 750 m) that is ≈ 33 m along a direction where the filter believes σ ≈ 7.5 m — a NEES
   contribution of order (33/7.5)² ≈ 19, against 12 measured.

   **Tested, not assumed: a better linearisation is not the fix.** An unscented predict (sigma
   points in θ, carrying the cos-shortening through every cycle; same test, same seeds) improves
   the end-of-outage NEES — 11.8 → 9.2 at 500 s, 178 → 54 at 1000 s — but does not restore
   consistency. The per-cycle linearisation is a part of the error, roughly a factor of three at
   1000 s, and not most of it. The rest is the Gaussian assumption itself: once heading
   uncertainty is large, the accumulated position distribution is curved (the "banana"), and any
   Gaussian filter, extended or unscented, re-approximates it as an ellipse every cycle.

   So the validity condition is **the filter's own heading σ**, not a time: a Gaussian predict is
   adequate while σθ stays below roughly 10°, which at these parameters is about two minutes of
   outage. Beyond it the honest options are to stop trusting the dead-reckoned position or to
   change the filter class.

Constant velocity "wins" on `static_uwb` for a reason that is not a virtue: after a dwell its
velocity estimate is near zero, so it coasts in place at the last site, and the walker's tour
keeps returning to sites. Its error is bounded by the geometry, not by anything it measures.

## 3b. Sensitivity to the dominant assumption — residual gyro bias (registered §8, no verdict)

The residual turn-on bias (0.15 dps nominal, from the data sheet's TCO and **our** 10 K) is the
dominant heading error. Re-run at `boxHalf = 3000`, seeds 301–310, `static_2p4` and
`ca_tbl_full`, against the primary's constant-velocity cells; 20 invocations at `e94d04d`, all
`dirty 0`, digests verified. No-fix median error, imu / cv:

| residual bias | `static_2p4` no-fix | `static_2p4` deep | `ca_tbl_full` no-fix |
|---|---|---|---|
| 0.05 dps | 0.163 [0.123, 0.210] | 0.079 [0.041, 0.153] | 0.216 [0.123, 0.395] |
| 0.15 dps (primary) | 0.160 [0.122, 0.204] | 0.083 [0.042, 0.163] | 0.243 [0.130, 0.452] |
| 0.5 dps (uncalibrated, worst case) | 0.200 [0.133, 0.319] | 0.147 [0.043, 0.618] | 0.398 [0.186, 1.119] |

The short-outage benefit is insensitive to a three-fold better calibration and **erodes, without
vanishing, at the uncalibrated worst case** — where `ca_tbl_full`'s interval reaches 1. A tag
that does not calibrate its gyro at power-on loses most of what the prior buys it on the tiered
arm. Seeds with too few deep cycles leave `ca_tbl_full`'s deep column not testable.

## 4. A0 v2

| cell | `D_T` / `D_B` (`A_fix`) | ρ radio | verdict (primary) |
|---|---|---|---|
| ref, cv | 30 / 30 | 2.141 [2.114, 2.170] | **FAIL** (a tie — predicted from published data) |
| ref, imu | 30 / 30 | 2.134 [2.106, 2.163] | **FAIL** |
| sparse, cv | 20 / 1 | 2.738 [2.688, 2.788] | **FAIL** (allowance 1.63) |
| sparse, imu | 29 / 2 | 2.735 [2.685, 2.784] | **FAIL** (allowance 1.90) |

Secondary, registered as such: **sparse, imu, system energy — ρ = 1.710 [1.700, 1.720], PASS at
κ = 1, FAIL at κ ≤ 0.5.** This is exactly the compression `PREREG_A0_V2.md` §2.2 predicted before
any cell: a shared 11.52 J pulls the ratio toward 1. It is a statement about carrying an IMU, not
about tiering. On `A_est` the sparse imu cell is a 29/29 tie, FAIL.

**The architecture still does not pay at `R = 25 m`** under the registered criterion, with or
without the inertial prior.

## 5. What this establishes, and what it does not

**Establishes.** The inertial prior, as built, is a strong improvement for outages up to about two
minutes — the gaps a tier-2 or tiered arm has between fixes — and it changes covariance-triggered
behaviour, cutting a threshold policy's futile escalations by ~90 %. It does not reach the radio
path (P1, exact).

**Caveat on the sparse configuration.** At `boxHalf = 3000` only **4.2 %** of cycles fall inside
three-anchor UWB coverage (verifier, `sparse_cv_s301`), below the verifier's own 5 % floor for the
escalation decision to be exercised. The sparse half of this result therefore says little about
tier 3; its weight is on tier-2 gaps and the tier-1 floor.

**Does not establish.** That it is safe to coast on: beyond a few minutes it is overconfident by
construction, and in the scenario its sensor states also absorb ranging errors. It is not, as
built, a substitute for coverage.

## 6. Consequences — next steps, each needing its own flag, null control and registration

1. **Bound dead reckoning by the filter's own heading σ** (§3.2): coast on the IMU only while σθ
   is below ~10°, and beyond it stop trusting the dead-reckoned position — inflate its covariance,
   or hold position as constant velocity effectively does. Keyed on σθ, which the filter knows,
   not on elapsed time. Default off. An unscented predict was tried and is not sufficient (§3.2);
   a particle filter would be, at a cost this scenario does not need if (1) holds.
   **Landed (2026-10-15), a different mitigation:** `--imuCoastMomentMatch` (`21d181b`, default
   off) adds the dropped along-track variance, |path|² Var(cos δ), during the outage — end NEES
   1.85 / 1.47 / 1.57 / 2.68 at 125 / 250 / 500 / 1000 s on the §3.2 test, mean error unchanged.
   It keeps the covariance honest; it does not make the position better, and it does nothing for
   mechanism 1. No scenario campaign with it on exists yet.
2. **Close G17's population gap.** Add the long-outage case (250–1000 s) to G17's filter test; it
   fails today, which is the point — a red case that names the defect until (1) lands.
   **Done as G20** (`4332e89`): a 1000 s outage after the dwell; moment-matched passes, the plain
   EKF fails (NEES 177.7). G20 tests the filter class, not the scenario's default, which keeps the
   plain EKF.
3. **Protect the sensor states from unmodelled ranging error** — e.g. update heading offset, bias
   and stride scale only from fixes that pass the integrity check. Default off.
4. E3 (CAL-05 with the inertial prior) runs at `boxHalf = 2000`, where `ca_tbl_full`'s outages
   are short — inside the regime where the prior is consistent. Its result should be read with
   `ca_tbl_full`'s NEES of 5.0 in mind.
