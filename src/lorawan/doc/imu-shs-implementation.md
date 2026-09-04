# Implementation.md — Backpack-Mounted IMU / SHS Simulation Build Guide

Companion to `IMU_SHS_Design.md`. That document explains *why* each decision was made and cites the papers; this document is the *build order* — what to implement, in what sequence, with the exact formulas and constants needed at each step.

---

## Step 0 — Sensor model (MPU-6050 measurement simulation)

Simulates raw accelerometer + gyroscope samples, before any SHS processing.

**Formulas:**
```
a~ = a_true + b_a + n_a          (accelerometer, Eq. 3.11)
w~ = w_true + b_g + n_g          (gyroscope, Eq. 3.12)

Bias evolution (Gauss-Markov), each tick dt:
  b_k = b_(k-1) * exp(-dt/tau) + w_noise
  w_noise ~ N(0, sigma^2 * (1 - exp(-2*dt/tau)))

White noise terms:
  n_a ~ N(0, accelNoiseDensity / sqrt(dt))
  n_g ~ N(0, gyroNoiseDensity / sqrt(dt))
```

**Constants needed:**
| Constant | Value |
|---|---|
| accelNoiseDensity | 3.9227e-3 m/s^2/sqrt(Hz) |
| accelBiasInstability | 1.471e-3 m/s^2 (estimate) |
| accelBiasTimeConstant (tau) | 300 s (estimate) |
| gyroNoiseDensity | 1.6682e-4 rad/s/sqrt(Hz) |
| gyroBiasInstability | 8.816 deg/h -> convert: * (pi/180) / 3600 = rad/s |
| gyroBiasTimeConstant (tau) | 80 s |
| Sample rate | 100 Hz (dt = 0.01 s) |
| AFS_SEL | +/-8g |
| FS_SEL | +/-500 deg/s |

**References:** TDK/InvenSense PS-MPU-6000A-00 Rev 3.4, 2013 (accel noise density); Rudyk et al., Sensors 20(17):4841, 2020 (gyro noise density + bias instability + GM time constant).

---

## Step 1 — Step detector (Iluz method)

Input: the raw accelerometer stream from Step 0 (vertical + anterior-posterior axes only — this method does not use the full 3-axis resultant magnitude).

**Algorithm, in order:**
```
1. Band-pass filter the vertical + AP acceleration signal: 0.5-3 Hz
2. Slide a 5-second window across the filtered signal
3. Convolve the windowed signal with a 2 Hz sinusoidal template
4. Find local maxima of the convolution output -> each is a candidate step timestamp
5. Classification rule: window is valid "gait" IFF 2 <= (step count in window) <= 15
6. Only step timestamps inside a window classified as gait are kept; others discarded
```

**Constants needed:**
| Constant | Value |
|---|---|
| Band-pass range | 0.5-3 Hz |
| Window length | 5 s |
| Template frequency | 2 Hz |
| Valid step count range | 2-15 steps per window |

**Reference:** Iluz, T. et al., J. NeuroEngineering and Rehabilitation, 11, 48, 2014. (Selected for this placement/population per Micó-Amigo et al., Mobilise-D consortium, J. NeuroEngineering and Rehabilitation, 20, 78, 2023.)

**Output of this step:** a stream of accepted step timestamps.

---

## Step 2 — Per-step window extraction (handoff to step-length model)

For each accepted step timestamp from Step 1, extract the acceleration features Weinberg's formula needs.

**Definition:**
```
one step = one valley -> peak -> valley cycle of vertical acceleration
a_max = the peak (local maximum) within that cycle
a_min = the valley value(s) bounding that cycle
```

Practically: search a small window around each Step-1 timestamp for the nearest local minima on either side (the valleys) and the local maximum between them (the peak). This defines the exact `a_max`/`a_min` pair for that single step.

**Reference:** Ho, Truong, Jeong, Sensors 16(9):1423, 2016; corroborated by Alvarez et al., Sensors 12(8):10536, 2012.

**Output of this step:** for each step, a pair `(a_max, a_min)`.

---

## Step 3 — Step-length model (Weinberg)

**Formula:**
```
s_w = k_w * (a_max - a_min)^(1/4)
```

**k_w is NOT a fixed constant.** Implement it as an online-updated value:
```
Whenever a trusted radio fix pair (both CLEAN-flagged, not stale) brackets N detected
steps with known true distance D between them:
    k_w_estimate = D / sum_over_N_steps( (a_max_i - a_min_i)^(1/4) )
    k_w = running update (e.g. exponential moving average) of k_w_estimate
```
Until the first such calibration event occurs, use a population-average fallback value (not yet chosen — flagged in the design doc as an open cold-start item; a reasonable placeholder should be selected during implementation, e.g. a literature-typical adult step-length constant, and clearly marked as provisional until real calibration data exists).

**Reference:** Weinberg, H., Analog Devices AN-602, 2002 (formula). Online-calibration-against-external-fix pattern: "Context-assisted personalized pedestrian dead reckoning localization with a smartphone," GIScience & Remote Sensing, 2024, DOI 10.1080/10095020.2024.2338225.

**Output of this step:** step length `s_w` for each detected step.

---

## Step 4 — Heading estimator

**Formula:**
```
theta(t) = theta_0 + integral[0,t] ( w_z(t') - b_g_hat ) dt'
```
i.e. each tick: `theta += (w_z_measured - b_g_hat) * dt`

**Sign convention:** positive gyro Z = counterclockwise = increasing theta. This is the MPU-6050's native right-hand-rule output — **no sign flip needed**, provided the chip is physically mounted with its Z-axis pointing straight up out of the backpack. (If mounting orientation differs, a fixed rotation offset must be applied here — not yet needed for the current design assumption.)

**theta_0 (initial heading):** derived from the first two trusted radio fixes — the direction implied by those two fix positions becomes the calibration point for theta_0, using the same RF-aiding mechanism as ongoing drift correction below. Until the first two trusted fixes arrive, theta_0 is undefined (same cold-start caveat as k_w — a provisional default, e.g. theta_0 = 0, should be used and clearly marked as such).

**Drift correction (ongoing):** periodically, when radio fixes show the wearer moved in an approximately straight line over some interval, use that implied direction to correct accumulated gyro heading drift — same architecture as UWB/WiFi-aided PDR fusion (EKF combining IMU heading with RF position estimates), substituting LoRa fixes as the aiding source.
**Flagged limitation:** no published paper validates this specific substitution (LoRa in place of UWB/WiFi) — implement as designed, but treat as unvalidated pending real hardware/field data.

**References:** Reduction of thesis Eq. (3.13) to a 2D heading scalar. Drift-correction pattern: "An INS and UWB Fusion-Based Gyroscope Drift Correction Approach for Indoor Pedestrian Tracking," PMC7472062; "Tightly Coupling Fusion of UWB Ranging and IMU Pedestrian Dead Reckoning for Indoor Localization," IEEE Access, 2021.

**Output of this step:** `theta` at the time of each detected step.

---

## Step 5 — Position update

**Formula:**
```
p_new = p_old + s_w * [cos(theta), sin(theta)]
```
Applied once per detected step, using that step's `s_w` (Step 3) and `theta` (Step 4).

**Convention check (must hold for this formula to be correct as written):** standard math angle convention, theta=0 along +x, increasing counterclockwise. Confirmed consistent with Step 4's gyro sign convention and with the existing EKF state `[x, y, vx, vy]` — no conversion required at either interface, provided Step 4 is implemented as specified above.

**Reference:** Standard SHS position-update formula, consistent with the theta convention established in Step 4.

**Output of this step:** running position estimate `p = (x, y)`, updated once per step.

---

## What this pipeline does NOT yet include (do not assume these are handled)

1. **No integration into the EKF prediction step or Switching-Point Integrity Check.** This pipeline produces a position estimate; nothing here yet defines how that position's uncertainty should be represented for consumption by the existing Eq. (10.2)-(10.3) / Eq. (10.19)-(10.20) machinery. That is a separate, not-yet-started redesign (see `IMU_SHS_Design.md` Section 7).
2. **No rough-terrain / sway correction (IPM-INS).** Steps 0-5 above assume level or moderately uneven ground; the trunk-sway correction identified as the best available proxy for SAR rough terrain has not been incorporated into any step.
3. **No defined cold-start defaults** for k_w or theta_0 before the first trusted radio fix arrives — flagged at Steps 3 and 4 above, needs a decision before this is deployment-ready.

---

## Suggested build/test order

1. Implement Step 0 in isolation; validate against the closed-form bias-growth checks (Eq. 3.17-style sanity tests) the way the earlier strapdown engine was validated, before trusting it as an input to anything else.
2. Implement Steps 1-2 together; test against a synthetic walking signal with known step timestamps before connecting to Step 0's simulated output.
3. Implement Step 3 with a hardcoded placeholder `k_w` first (defer the online-calibration logic) to validate the rest of the pipeline end-to-end.
4. Implement Step 4 with `theta_0 = 0` and no drift correction first, to validate the integration math alone.
5. Implement Step 5, then connect the full chain (Steps 0->5) end-to-end.
6. Only after the full chain is validated end-to-end, layer in the online k_w calibration and RF-aided theta_0/drift-correction logic (Steps 3-4's "not yet defined" pieces), since both depend on a radio-fix interface that should be mocked/stubbed until the rest of the pipeline is proven correct on its own.
