# CA-TBL Field Node IMU Simulation — Design Documentation (Backpack / SHS)

## 1. Scope and mounting assumption

**The IMU (MPU-6050) is assumed to be mounted inside/on the Field Node's backpack** — i.e. trunk-mounted, not foot-mounted. This is a deliberate architectural decision, not a default:

- ZUPT (Zero Velocity Update, thesis Eq. 3.18) requires a true zero-velocity instant, which only exists at the foot during mid-stance. At the backpack, the wearer's center of mass never stops moving during walking, so **ZUPT is not applicable at this mounting location.**
- Because of this, the design does **not** use strapdown double-integration + ZUPT (thesis Sec 3.14–3.15) for position. Instead it uses a **Step-and-Heading System (SHS)**: detect discrete steps, estimate step length empirically (no integration), integrate heading separately, and update position once per step.

## 2. SHS architecture — the four pieces

| # | Piece | What it does |
|---|---|---|
| 1 | Step detector | Detects each footfall / walking bout from the backpack's acceleration signal, gated on combined vertical + AP signal energy so noise cannot register as gait |
| 2 | Step-length model | Converts per-step acceleration features into a distance, replacing double-integration. k_w calibrated online against radio fixes, summing bracketed steps as vectors |
| 3 | Heading estimator | Integrates yaw-axis gyro to get direction of travel, seeded from the known deployment orientation, aided by a ZARU bias observer, and corrected against drift only when the stretch qualifies |
| 4 | Position update | `p_new = p_old + s_w * [cos(theta), sin(theta)]`, once per detected step |

Two supporting pieces sit alongside these:

| Piece | What it does |
|---|---|
| Walking-energy gate | Classifies each window as walking or not, from the combined vertical + anterior-posterior band-passed energy. Prevents noise being read as gait, and supplies the stillness signal the bias observer needs |
| ZARU gyro bias observer | When the wearer is stationary the true yaw rate is zero, so the gyro reading is its own bias. Averaged over rest periods and subtracted during heading integration |
| Turn detector | Flags turns from the integrated heading (>90 degrees over 0.5-10 s, per Bertoli et al.). **Diagnostics and a hook only** -- it does not yet alter detection, because the source paper adapts by switching wavelet kernel and that has no equivalent in this project's matched-filter design |

**Deployment assumptions** (these materially shape the design, and were not recorded in the original version):

- The node starts from an **exactly known position and a known orientation**. Both are seeded via `ShsEngine::SetInitialState` rather than bootstrapped from radio fixes.
- A **straight calibration walk at deployment is acceptable**, and is what calibrates k_w before the route begins. It must be long relative to the radio fix noise — **200 m recommended for 3 m fixes** (see Section 6b; at 50 m the worst case is 2.5x its own median, and that tail only collapses at 200 m).
- The heading is **re-seeded to the known deployment value at the end of the calibration walk**, not to the direction the fix pair implies. Two 3 m fixes 200 m apart imply a direction uncertain by atan(3*sqrt(2)/200) ~= 0.021 rad, which would otherwise persist for the whole route — measured at up to 0.031 rad, worth 154 m over a 5 km traverse.

**On the Iluz two-channel split.** The primary paper is paywalled and this algorithm was reconstructed from secondary sources, so the division of labour between the two channels is an explicit interpretation: both channels decide **whether** a window is walking, the vertical channel alone decides **where** the steps are. Step instants must come from the vertical channel regardless, because the Weinberg handoff needs the vertical valley-peak-valley pair (Ho et al. 2016), and driving peak-finding from a combined signal risks double-counting when the two channels run out of phase.

## 3. Submodules and their governing research papers

| Submodule / Decision | Method | Governing paper(s) |
|---|---|---|
| Accelerometer sensor model | Eq. (3.11): a~ = a + b_a + n_a, noise density 400 ug/sqrt(Hz) | TDK/InvenSense, "MPU-6000/MPU-6050 Product Specification," Doc PS-MPU-6000A-00, Rev 3.4, 2013 |
| Gyroscope sensor model | Eq. (3.12): w~ = w + b_g + n_g, measured Allan-variance noise/bias | Rudyk, A.V. et al., "Strapdown Inertial Navigation Systems for Positioning Mobile Robots — MEMS Gyroscopes Random Errors Analysis Using Allan Variance Method," Sensors, 20(17), 4841, 2020 |
| Mounting placement rationale | Peak acceleration by body location: back 0.9-5.0g vs ankle 3.0-12.0g | Wu, W.H., Cheng, C.C., peak-acceleration-by-placement comparison study |
| Mounting placement rationale (2) | Waist/thigh acceleration envelope across gait speeds (~within +/-6g for ~99% of walking) | Rowlands, A.V. et al., "Accelerations of the Waist and Lower Extremities over a Range of Gait Velocities," PMC4180224 |
| AFS_SEL sensor range | **+/-6g** (was +/-8g) | Rowlands et al. (PMC4180224): waist within +/-6g for 99% of gait up to 4.8 m/s. Now evidence-based, and implemented as real saturation |
| FS_SEL sensor range | +/-500 deg/s | **Engineering decision — still no citation.** See Flag 2 |
| Backpack-specific acceleration (context only, not used for range value) | Directly measured backpack vertical acceleration during walking, 3.7-6 km/h | Ren, L., Jones, R.K., Howard, D., "Dynamic analysis of load carriage biomechanics during level walking," J. Biomech., 38(4), pp. 853-863, 2005 |
| Step detector algorithm | Band-pass 0.5-3 Hz, 5s window, convolution with 2 Hz sinusoidal template, local maxima = steps, window valid if 2-15 steps | Iluz, T. et al., "Automated Detection of Missteps during Community Ambulation in Patients with Parkinson's Disease," J. NeuroEngineering and Rehabilitation, 11, 48, 2014 |
| Peak threshold form, and turn detection | Keep peaks of "magnitude >40% of the mean of all peaks detected"; a turn is a yaw change ">90 degrees ... duration between 0.5 and 10 s" | **Bertoli, M. et al., "Validation of a Step Detection Algorithm during Straight Walking and Turning in Patients with Parkinson's Disease and Older Adults Using an Inertial Measurement Unit at the Lower Back," PMC5591331.** Same sensor location class as this project, and the only source found that validates step detection explicitly across BOTH straight and turning walking |
| Centripetal acceleration is real and measurable at a trunk mount during turns | Lumbar-mounted measurement; straight walking tightly distributed about 0, turning not | Fino, P.C., Horak, F.B., Curtze, C., IEEE T-NSRE 28(3):629-636, 2020 (PMC7228504). **Evidence the mechanism is real, NOT the source of the injection formula** |
| Trunk leans into the turn to counteract centrifugal effect | Trunk-fixed accelerometer measuring inclination during curved walking | PMC4325939. **Qualitative confirmation only -- contributes no formula and no magnitude** |
| Step detector algorithm selection (why Iluz, not Paraschiv-Ionescu) | Head-to-head comparison of 3 gait-sequence-detection algorithms at lower-back placement; Iluz's method (GSDA) won for the healthy-adult cohort | Micó-Amigo, M.E., Bonci, T., Paraschiv-Ionescu, A. et al. (Mobilise-D consortium), "Assessing real-world gait with digital technology?...", J. NeuroEngineering and Rehabilitation, 20, 78, 2023 |
| Step detector -> step-length handoff | Defines one step as a valley->peak->valley cycle in vertical acceleration; a_max/a_min extracted from that cycle | Ho, N.-H., Truong, P.H., Jeong, G.-M., "Step-Detection and Adaptive Step-Length Estimation for Pedestrian Dead-Reckoning at Various Walking Speeds Using a Smartphone," Sensors, 16(9), 1423, 2016 |
| Step detector -> step-length handoff (corroborating) | Same handoff pattern (peak detection on vertical + AP axes, step length from step frequency), independently arrived at for waist-worn placement | Alvarez, J.C., Alvarez, D., López, A., González, R.C., "Pedestrian Navigation Based on a Waist-Worn Inertial Sensor," Sensors, 12(8), pp. 10536-10549, 2012 |
| Step-length formula | s_w = k_w * (a_max - a_min)^(1/4) | Weinberg, H., "Using the ADXL202 in Pedestrian Dead-Reckoning Applications," Analog Devices AN-602, 2002 |
| Step-length constant k_w | **Online, calibrated continuously against radio fixes** (not a fixed literature value) | Pattern confirmed viable by: "Context-assisted personalized pedestrian dead reckoning localization with a smartphone," GIScience & Remote Sensing, 2024, DOI 10.1080/10095020.2024.2338225 |
| Walk-detection algorithm ruled out | HDE (Heuristic Drift Elimination) and variants — depends on indoor architectural regularity (corridors, right angles) not present in SAR/outdoor terrain | Ruled out after review; not used |
| Heading estimator | Gyro Z-axis integration for yaw, no magnetometer | Standard strapdown reduction of Eq. (3.13) to a single heading scalar |
| Heading drift-bounding pattern | RF-fix-aided correction during straight-line stretches (EKF combining IMU heading with RF position estimates) | Pattern from: "An INS and UWB Fusion-Based Gyroscope Drift Correction Approach for Indoor Pedestrian Tracking," PMC7472062; "Tightly Coupling Fusion of UWB Ranging and IMU Pedestrian Dead Reckoning for Indoor Localization," IEEE Access, 2021 |
| Heading reference theta_0 | Derived from the first two trusted radio fixes (same mechanism as ongoing drift correction, applied once at start-up) | Same pattern as above, applied to initialization |
| Heading sign convention | Positive gyro Z = counterclockwise rotation = increasing theta (standard math convention, matches MPU-6050's native right-hand-rule output, zero conversion needed) | MPU-6050 datasheet coordinate-frame convention (confirmed via multiple secondary sources referencing the official InvenSense axis diagram) |
| Position update formula | p_new = p_old + s_w * [cos(theta), sin(theta)] | Standard SHS formula; consistent with the theta convention above |
| Rough-terrain / sway correction (identified, not yet integrated) | Inverted-pendulum model correcting trunk sway in trunk-mounted PDR — best available proxy for uneven SAR ground | "IPM-INS: A Pedestrian Inertial Navigation System Using a Trunk-Mounted IMU Based on Inverted Pendulum Model," IEEE Trans. Instrumentation and Measurement, 74, 2025 |

## 4. Core formulas, with citations directly beneath each

### Accelerometer measurement model
```
a~ = a + b_a + n_a
```
*Source: CA-TBL Phase 1 Thesis, Eq. (3.11); noise/bias numeric values from TDK/InvenSense MPU-6000/MPU-6050 Product Specification (PS-MPU-6000A-00, Rev 3.4, 2013) and Rudyk et al., Sensors 20(17):4841, 2020.*

### Gyroscope measurement model
```
w~ = w + b_g + n_g
```
*Source: CA-TBL Phase 1 Thesis, Eq. (3.12); numeric values from Rudyk et al., Sensors 20(17):4841, 2020.*

### Gyro bias instability, deg/h to rad/s
```
b_g[rad/s] = b_g[deg/h] * (pi / 180) / 3600
```
*Source: standard unit conversion, applied to the measured value in Rudyk et al., 2020.*

### First-order Gauss-Markov bias evolution (used for both accel and gyro bias)
```
b_k = b_(k-1) * exp(-dt / tau) + w,   w ~ N(0, sigma^2 * (1 - exp(-2*dt/tau)))
```
*Source: standard Allan-variance-consistent bias synthesis method; tau for the gyro (80s) is drawn from the reported Allan-deviation-minimum region (20-150s) in Rudyk et al., 2020. tau for the accelerometer (300s) is an engineering estimate, no part-specific figure found.*

### Walk / step detection (Iluz method)
```
1. Band-pass filter vertical + anterior-posterior acceleration, 0.5-3 Hz
2. 5-second sliding window
3. Convolve windowed signal with a 2 Hz sinusoidal template
4. Local maxima of convolution = individual gait cycles (steps)
5. Window classified as gait if: 2 <= (steps in window) <= 15
```
*Source: Iluz, T. et al., J. NeuroEngineering and Rehabilitation, 11, 48, 2014 (algorithm description corroborated by two independent secondary sources, since the primary paper is paywalled).*

### Step window definition (handoff to step-length model)
```
one step = one valley -> peak -> valley cycle of vertical acceleration
a_max = the peak value within that cycle
a_min = the valley value(s) bounding that cycle
```
*Source: Ho, Truong, Jeong, Sensors 16(9):1423, 2016; corroborated by Alvarez et al., Sensors 12(8):10536, 2012.*

### Step-length formula (Weinberg)
```
s_w = k_w * (a_max - a_min)^(1/4)
```
*Source: Weinberg, H., Analog Devices AN-602, 2002.*
**k_w is not a fixed constant** — calibrated online by comparing SHS-predicted distance against trusted radio fixes. See Section 5, Flag 3.

### Heading integration
```
theta(t) = theta_0 + integral[0,t] ( w_z(t') - b_g_hat ) dt'
```
*Source: reduction of thesis Eq. (3.13) to a single horizontal heading scalar (2D state scope, consistent with thesis Sec 10.2). theta_0 per Section 5, Flag 4.*

### Position update
```
p_new = p_old + s_w * [cos(theta), sin(theta)]
```
*Source: standard SHS position-update formula. Correct specifically under the standard math angle convention (theta=0 along +x, increasing counterclockwise) — confirmed consistent with the MPU-6050's native gyro Z-axis sign (right-hand rule datasheet convention) and with the existing EKF state definition, so no conversion is required at either interface.*

## 5. Constants table

| Constant | Value | Source |
|---|---|---|
| Accelerometer noise density | 3.9227e-3 m/s^2/sqrt(Hz) (400 ug/sqrt(Hz)) | TDK/InvenSense PS-MPU-6000A-00 Rev 3.4; independently confirmed by Cai et al., 2019 (measured 3.924e-3) |
| Accelerometer bias instability | 1.471e-3 m/s^2 (~150 ug) | **Estimate — SUPERSEDED, see Flag 1. Cai et al. measured 1.88-1.94e-4 m/s^2 for the MPU-6000. Code not yet updated.** |
| Accelerometer bias GM time constant | 300 s | Cai et al., 2019 (MPU-6000 correlation time, up to 300 s) — **estimate now confirmed** |
| Gyroscope noise density | 1.6682e-4 rad/s/sqrt(Hz) (0.009558 deg/s/sqrt(Hz), mean of 3 axes) | Rudyk et al., 2020 |
| Gyroscope bias instability | 8.816 deg/h (worst axis, Z) | Rudyk et al., 2020 |
| Gyroscope bias GM time constant | 80 s | Rudyk et al., 2020 (within reported 20-150s AV-minimum region) |
| Sample rate | 100 Hz | Engineering choice, within MPU-6050's 4-1000 Hz configurable ODR |
| AFS_SEL | **+/-6g** | Rowlands et al. (PMC4180224): waist within +/-6g for 99% of gait up to 4.8 m/s. **Changed from +/-8g; now evidence-based, and actually implemented as saturation** |
| FS_SEL | +/-500 deg/s | **Engineering decision — still no direct citation (Flag 2). Now implemented as saturation** |
| Iluz band-pass range | 0.5-3 Hz | Iluz et al., 2014 |
| Iluz window length | 5 s | Iluz et al., 2014 |
| Iluz template frequency | 2 Hz | Iluz et al., 2014 |
| Iluz step-count validity bound | 2-15 steps per window | Iluz et al., 2014 |
| Peak threshold (`peakRelativeThreshold`) | 0.40 (fraction of mean candidate peak) | **CITED — Bertoli et al., PMC5591331**, which keeps peaks of "magnitude >40% of the mean of all peaks detected", at a lower-back mount validated across straight AND turning walking. **Replaces `peakProminenceThreshold = 0.05`**, an absolute compare against a correlation sum running 1-100 that never rejected anything |
| Walking-energy gate (`walkingRmsThresholdMps2`) | 0.21 m/s^2 | **Uncited implementation constant.** Measured against this project's own sensor model (`scratch/shs-noise-floor.cc`): combined vertical+AP band-passed RMS is 0.0147 standing, 0.758 light walking, 2.272 normal walking |
| Turn magnitude (`turnMagnitudeRad`) | pi/2 (90 deg) | **CITED — Bertoli et al., PMC5591331**: a turn is "a change of the yaw angle ... with a magnitude >90 degrees and a duration between 0.5 and 10 s" |
| Turn duration bounds (`minTurnDurationS`, `maxTurnDurationS`) | 0.5 s, 10.0 s | **CITED — Bertoli et al., PMC5591331**, same definition |
| Turn hysteresis (`turnExitFraction`) | 0.75 | **Uncited implementation detail.** Required because a bare threshold chatters when the real turn magnitude sits near it -- the square-loop route's 90 degree turns against a 90 degree threshold reported 132 turns for 24 real ones without it |
| Turn history decimation (`historySamplePeriodS`) | 0.1 s | **Uncited implementation detail.** Bounds the per-tick scan; any value well below minTurnDurationS leaves the decision unchanged |
| Trunk lean during turns (`leanMaxDeg`, `leanRiseS`) | 10 deg, 0.75 s | **UNSOURCED ESTIMATES.** PMC4325939 confirms the mechanism and its direction (leaning into the turn); neither magnitude nor timing is from any paper |
| Minimum calibration straightness (`minCalibrationStraightness`) | 0.2 | **Uncited implementation constant.** Below this a stretch carries too little scale information to calibrate k_w against |
| Minimum correction baseline (`minCorrectionBaselineM`) | 20 m | **Uncited implementation constant.** Sizing rule: several times the 1-sigma fix noise, since a fix pair implies a direction with uncertainty ~atan(sigma_fix / baseline) |
| ZARU stillness rate gate | 0.05 rad/s | **Uncited.** ~30 sigma of the modelled per-sample gyro noise at 100 Hz |
| ZARU dwell / EMA weight | 5 s / 0.2 | **Uncited.** Sized so the EMA across dwells brings the estimate below the bias instability |
| Weinberg constant k_w | **Not fixed — online-calibrated** | Pattern from GIScience & Remote Sensing, 2024 (10.1080/10095020.2024.2338225) |
| Heading reference theta_0 | **Seeded from the known deployment orientation** (fix-pair derivation retained as fallback) | See Flag 5 |

## 6. Flagged limitations (open items, not blocking, documented explicitly)

1. **RESOLVED — Accelerometer bias instability and time constant.** Cai et al., "Performance Assessment of an Ultra Low-Cost Inertial Measurement Unit for Ground Vehicle Navigation," Sensors, 2019 (PMC6767180) runs Allan variance on the **MPU-6000** — the same silicon as the MPU-6050, sharing the joint PS-MPU-6000A-00 datasheet already cited here. It gives accelerometer bias instability **1.880e-4 to 1.939e-4 m/s^2** and a correlation time **up to 300 s**, confirming the 300 s estimate and superseding the 150 ug one, which is **~7.6x too pessimistic**. The same paper independently confirms the accelerometer noise density (measured 3.924e-3 vs the 3.9227e-3 in use). **Code not yet updated** — `accelBiasInstability` in `lora-shs-parameters.h` still reads 1.471e-3. Error is in the safe direction, so nothing is broken; the error budget is merely inflated.
   Note the two sources disagree on the gyroscope, in opposite directions: Rudyk gives noise density 1.668e-4 rad/s/sqrt(Hz) and bias instability 4.27e-5 rad/s; Cai gives 8.726e-5 and 1.575-1.805e-4. Part-to-part spread across two samples. Treating these as a range, and bounding results at both ends, is more defensible than picking one.
2. **STILL OPEN — FS_SEL (+/-500 deg/s) has no supporting citation.** A literature search found no study reporting peak trunk yaw rate in a form usable for choosing a full-scale range. Circumstantially, chest-mounted gyroscopes during walking are reported around 40-60 deg/s, making +/-500 roughly 10x margin — likely more than needed. Reducing to +/-250 deg/s would double resolution while retaining 4x headroom. Now at least implemented as real saturation rather than being a documentation-only value.
3. **CLOSED — AFS_SEL.** Changed from +/-8g to **+/-6g** on the evidence of Rowlands et al. (PMC4180224), which measured waist acceleration within +/-6g for 99% of gait velocities up to 4.8 m/s; a trunk/backpack mount sees the same envelope. This doubles resolution against the +/-16g the part also offers. The trade is explicit: the ~1% of samples outside +/-6g correspond to running and impact rather than walking, and now saturate. Revisit if the wearer is expected to run. Also note this was previously a **documentation-only value** — no clipping existed in the sensor model at all. It is now implemented.
4. **STILL OPEN — RF-aided heading drift correction is unvalidated at LoRa's profile.** The established literature (PMC7472062; IEEE Access 2021) uses UWB or WiFi/BLE, both far higher update-rate and accuracy. Two things have changed since this was first written:
   - Seeding theta_0 from the known deployment orientation (Flag 5) removes the dependency at startup, so this now affects only ongoing correction.
   - Correction remains a **hard snap** (`m_theta = thetaImplied`), not a filtered update, so every correction injects the full fix noise into heading. A weighted blend would degrade gracefully where a snap does not. This is part of the Section 7 redesign.
   Three guards now bound the damage: rotation since the last fix, agreement with the current heading, and a **minimum baseline** (see Flag 8).
5. **LARGELY RESOLVED — Cold start.** Resolved by two facts about the deployment that were not recorded when this was written: **the node starts from an exactly known position AND a known orientation**, and **a straight calibration walk at deployment is acceptable**.
   - `ShsEngine::SetInitialState(x, y, theta)` seeds position and heading directly. Without it, on a route whose every fix interval contains a turn, theta_0 never initializes at all and the heading stays on its fallback for the entire run — measured at 0.35 rad and 15 m of error on a square loop at 60 s fix intervals.
   - A straight calibration walk gives the k_w calibration a fix pair bracketing a known distance with no turning. Unaided over a 300 m route: **168 m error without a calibration walk, 6.3 m with a 20 m one.**
   - **The calibration walk must be sized against the radio fix noise**, which is the new operational finding. A fix pair estimates distance with relative error ~sigma_fix/baseline, so with 3 m LoRa fixes the k_w spread is +/-26% at 20 m, +/-11% at 50 m, +/-5.5% at 100 m and +/-2.7% at 200 m. **100 m minimum is recommended for 3 m fixes.**
   - Residual: `kwFallback = 0.7` is still an unsourced placeholder governing the walk itself. It should become a deliberate population-average value.
6. **PARTIALLY CLOSED — false step detection, and it was WIDER than described here.** The concern was written as strap bounce; the actual defect was that the detector fired on **the sensor noise floor itself**. `peakProminenceThreshold` is an absolute threshold on an unnormalized correlation sum, so it never rejected anything: a stationary wearer produced **~215 "steps" per 120 s and ~95 m of phantom displacement** — more steps standing still than walking.
   Closed by a **walking-energy gate** (`walkingRmsThresholdMps2`) that rejects quiet windows on band-passed signal energy before any peak-finding runs. A stationary node now reports exactly 0 steps and 0 m.
   **Still open:** the gate tests energy only. Genuine rhythmic pack bounce at gait frequency has both the energy and the rhythm to pass it. The periodicity/continuity constraint that would catch that is designed but not built.
7. **STILL OPEN — Rough-terrain / sway correction (IPM-INS) identified but not integrated.** Open question unchanged: does it modify the step detector, the step-length model, or sit as a separate correction layer.
8. **NEW, FIXED — the chord-versus-path assumption, which was systemic.** Three separate places treated the straight-line chord between two fixes as if it described the path walked between them, which is only true if the wearer did not turn:
   - **k_w calibration** divided the chord by the *scalar* sum of step features. Biased k_w low in proportion to turning: -2.7% at 10 s fix intervals, -10.4% at 30 s, **-29.4% at 60 s**, the last costing 36 m of position error. Fixed by summing the bracketed steps as **vectors**: `k_w = chord / |sum q_i u_i|`, which is exact for any path shape and reduces to the old formula on a straight line.
   - **Heading drift correction** could accept a chord that coincidentally fell near the post-turn heading. Fixed with a gyro-integrated rotation guard.
   - **theta_0 derivation** assumed the first fix pair's stretch was straight without checking. Fixed with the same guard, and the candidate pair now slides forward so a later straight stretch can still initialize.
   **Design principle to preserve: no consumer of a fix pair may assume the stretch between them was straight.** Without this stated, a fourth instance will appear.
9. **NEW, FIXED — short-baseline corrections were actively harmful.** Heading correction previously accepted any baseline above 1 mm. A fix pair implies a direction with uncertainty ~atan(sigma_fix / baseline), so over a short baseline the implied direction is mostly noise. Measured with 3 m fixes and a 20 m baseline: **0.36 rad of heading error injected on a dead-straight route whose unaided drift over the same period was 0.006 rad** — a correction 60x worse than the drift it was meant to remove. Fixed by `minCorrectionBaselineM` (default 20 m). The sizing rule matters more than the number: several times the 1-sigma fix noise.
10. **NEW, OPEN — k_w calibration has no minimum-baseline guard**, unlike heading. It is partly protected by the EMA across fixes, but the same short-baseline noise amplification applies. Symmetric with Flag 9 and worth closing.
11. **TURNING MODELED, PARTIALLY VALIDATED — measured penalty 1.00x with correct axis wiring, 3.40x if miswired. Consistent with the TRUNK-mounted benchmark (3 pp), not the foot-mounted one (1.66x).**

    **What was built.** A lateral (mediolateral) accelerometer axis added to `ImuSensorState`, gravity added to the vertical channel, and two turn-synchronous mechanisms injected during turns:

    ```
      centripetal : a_c = v * omega_z
      trunk lean  : a_lean = g * sin(theta_lean),
                    vertical correspondingly reading g * cos(theta_lean)
    ```

    Both are ordinary physics, not taken from any paper. The literature contributes evidence the mechanisms are real at a trunk mount, not their magnitudes — Fino et al. 2020 (PMC7228504) for centripetal acceleration, PMC4325939 for leaning into the turn. Fino et al.'s Eq. (1)-(11) margin-of-stability framework is NOT implemented; their one transferable guideline (average, do not integrate, a lateral-axis feature over a step) is recorded for future use and does not affect Step 3, which already works on `a_max - a_min` within a window rather than an integral.

    **Estimates, flagged:** `leanMaxDeg = 10.0`, `leanRiseS = 0.75`. Neither from a paper. **Stride shortening during turns is deliberately NOT implemented** — no source gives a magnitude, and an invented number would be worse than the gap.

    **Also adopted from Bertoli et al. (PMC5591331):** yaw-based turn detection (>90 degrees over 0.5-10 s), giving the engine turn awareness it previously lacked entirely. Measured **89% recall (21.4 of 24 real turns), 0 false positives** on a straight route. Turn state is **diagnostics and a hook only** — Bertoli et al. adapt by switching wavelet kernel (gaus2 during turns, db2 otherwise), which has no equivalent in this project's matched-filter design, and inventing an adaptation would be worse than leaving the hook unused.

    **Measured, 1 hour, 24 turns, 8 runs per configuration:**

    | Configuration | Final error | Worst instant | Steps |
    |---|---|---|---|
    | Turn injection OFF | 7.88 m | 12.55 m | 7238 |
    | Turn injection ON | **7.88 m** | **12.55 m** | 7238 |
    | ON, axis miswired | 26.78 m | 39.84 m | 5948 (17.8% missed) |

    **Penalty with correct wiring: 1.00x.** Not a modelling failure — a structural property the turn model has now *demonstrated* rather than assumed: the turn signals live on the lateral axis, and the pipeline does not read it. The only vertical perturbation is `g(1 - cos theta_lean)` = 0.15 m/s^2 at 10 degrees, against a 6.0 m/s^2 gait signal.

    **Against the RIGHT benchmark.** The 1.66x foot-mounted figure (7.40% -> 12.27%) was the wrong comparison: that is strapdown, where turning couples into a 3D attitude solution and disrupts ZUPT. The trunk-mounted benchmark is **Bertoli et al.: step-detection accuracy 91% straight vs 88% turning — a 3 percentage-point degradation**, not a 66% error increase. Our ~0 pp is close to that, so the result is consistent with trunk-mounted literature rather than anomalous.

    **Why this is only PARTIALLY validated.** Bertoli et al. needed a turn-adaptive wavelet to reach their 3 pp. We see 0 pp with no adaptation at all. Nobody re-engineers a detector around turns unless the turn signal breaks the original — so **our injection is still missing something**, most plausibly the unimplemented stride shortening. Their population (Parkinson's patients and older adults) also has harder gait than a healthy field operator, so their 3 pp is likely an upper bound.

    **Penalty if the axis assignment is wrong: 3.40x, plus 17.8% of steps missed.** Now the dominant turn-related risk, and a *mounting* question rather than an algorithm one. See Flag 12 — the two are not independent; turning is the condition under which a wrong axis assignment does its damage.

    **To close fully:** implement stride shortening as an explicitly flagged tunable, model trunk sway beyond simple roll, and resolve the axis assignment against the physical build.

12. **NEW, OPEN — the mounting orientation is not specified anywhere in this design, and its cost is now measured.** `lora-shs-engine.cc` wires measAy as vertical, measAx as anterior-posterior, and deliberately does not feed measAz (lateral) to the detector. Which PHYSICAL MPU-6050 axis is which remains unresolved. **Measured cost of getting it wrong: 3.46x position error and 13.1% of steps missed over a 1-hour turning route** (see Flag 11). This is an unmade design decision, not merely an unverified constant, it is now the dominant turn-related risk, and it needs answering against the physical build. Guarded by LateralAxisTurnResponseTest.
13. **NEW, OPEN — scope gaps.** The design is 2D only (per thesis Sec 10.2), but SAR terrain has vertical extent and a slope produces vertical acceleration the step detector reads as gait. Only walking is modelled: no running, crawling, stairs, or being carried. The walking gate now correctly reports "not walking" when stationary, but nothing decides what the position estimate should do during such periods.

## 6b. Measured performance

All figures from `shs-imu-example`, five RNG runs each. **Position is no longer snapped to radio fixes** (`ShsEngineParams::snapPositionToTrustedFix` defaults to false), so these are honest accumulated errors rather than one fix interval of dead reckoning. Under the previous snapping behaviour a 60-minute run reported "0.0% error" while carrying the same ~1 m error as a 2-minute one — the estimate was reset at every fix and the percentage merely diluted by a growing denominator.

**Sensor model validation.** Uncorrected heading drift over 1 hour measures **1.79 deg (1 sigma)** against **1.86 deg** predicted by `sigma_b * sqrt(2 * tau * t)` from the Rudyk bias instability — 4% agreement. The 8.816 deg/h figure is the *size of the bias*, not accumulated angle: because the bias mean-reverts on tau = 80 s, the angle grows as sqrt(t), not t.

**Unaided dead reckoning, 300 m route, no fixes during the route:**

| Radio fix noise | Calibration walk | Position error |
|---|---|---|
| 0 m (perfect) | 50 m | 1.78 - 1.84% |
| 0 m (perfect) | 100 m | **0.45 - 0.91%** |
| 3 m (LoRa) | 50 m | 4.92 - 16.27% |
| 3 m (LoRa) | 100 m | 2.21 - 8.28% |

For comparison, published waist-worn PDR reports ~2% (Alvarez et al., Sensors 12(8):10536) and 1.25% return position error (Sensors 15(11):28129). With good calibration this pipeline is at or inside that band. **The dominant error source is now k_w calibration quality, which is limited by radio fix noise — not by IMU physics.**

**FINAL 1-hour test.** 2.0 Hz cadence, 1.4 m/s, 0.7 m/step. Known deployment position and heading, 200 m straight calibration walk with 3 m LoRa fixes, then **no fixes at all** for the whole hour. 8 runs per configuration. Run with the final pipeline: relative peak threshold, yaw turn detection, three-axis sensor model with gravity and turn injection.

| Configuration | Final error | Worst instant | % of path | Steps |
|---|---|---|---|---|
| Closed loop, 24 turns (5067 m) | 7.88 m | 12.55 m | 0.16% | 7238 / 7239 |
| Straight traverse (5267 m) | 140.00 m | 140.35 m | 2.66% | 7522 / 7522 |
| Closed loop, axis MISWIRED | 26.78 m | 39.84 m | 0.53% | 5948 (17.8% missed) |

Turn detection: **21.4 of 24 real turns (89% recall), 0 false positives** on the straight route. k_w mean error 1.73%. Heading error after 2160 degrees of rotation: RMS 1.94 degrees. Stationary node: 0 steps, 0 m.

**The 18x gap between the two routes is geometric, not a turning effect.** Error is proportional to displacement from the last known position, not to distance walked. Fitting the loop-size sweep gives

```
  error ~= 3.8 m + 3.6% * (displacement from last known position)
```

which matches measurement across loop sides of 100, 200, 400 and 800 m. A closed loop caps displacement at its diagonal; a traverse does not. The 3.8 m floor is the radio-fix noise at the calibration re-seed. **Operationally: accuracy depends on how far the wearer gets from their last known position, not how far they walk.**

**Which figure to quote: the straight-traverse 2.66%.** It is the conservative one and the right number for a traverse away from base. The loop figure of 0.16% is real but flattering -- a closed route returns to its start, so the dominant k_w scale error contracts the whole path and barely shows at the endpoint. Published RPE figures share that blindness, so the comparison is like-for-like, but the worst-instant column (0.25%) is the honest number for a loop.

### Final benchmark conclusion

Sorted by how much weight each comparison can actually bear.

**Tier 1 -- genuine independent agreement.** Nothing was tuned toward these.

| Metric | Ours | Benchmark | Source | Delta |
|---|---|---|---|---|
| **Weinberg, uncalibrated** | 11.1% | 10.01% | Sensors 21(10):3527 (10 subjects, >10 h) | **+11%** |
| Accel noise density | 3.9227e-3 | 3.924e-3 measured | Cai et al., PMC6767180 | **+0.03%** |

The Weinberg row is the strongest result obtained. `kwFallback = 0.7` is an arbitrary placeholder, true k_w is ~0.447, and 11.1% falls out of that ratio unprompted.

**Tier 2 -- in-band, comparable conditions.**

| Metric | Ours | Benchmark | Source | Verdict |
|---|---|---|---|---|
| Open route, ~5 km unaided | **2.66%** | ~2% over 4 km | PDR: A basis for personal positioning | Match, slightly worse |
| Waist-worn PDR | 2.66% | ~2% | Alvarez et al., Sensors 12(8):10536 | Match, slightly worse |
| Step-length PDR band | 2.66% | 2.5-5% | general literature | Inside band |

**Tier 3 -- we look better; treat with suspicion.**

| Metric | Ours | Benchmark | Why the gap |
|---|---|---|---|
| Closed-loop RPE | 0.16% | 1.25% (Sensors 15(11):28129) | Metric blindness to scale error. Worst-instant 0.25% is the honest figure |
| Step detection, straight | ~100% | 91% (PMC5591331) | Synthetic gait vs real PD patients and older adults. Not comparable |
| Step detection, turning | ~100% | 88% (PMC5591331) | Same |
| Turning degradation | **0 pp** | 3 pp (PMC5591331) | They needed a turn-adaptive wavelet to reach 3pp; we needed nothing. Evidence our turn injection is still incomplete |
| Turning penalty | 1.00x | 1.66x (foot-mounted) | Architectural: foot strapdown couples turns into 3D attitude, SHS integrates one yaw scalar |

**Tier 4 -- internal consistency, not independent.**

| Metric | Ours | Theory |
|---|---|---|
| Gyro drift, 1 hour | 1.79 deg | 1.86 deg from sigma_b * sqrt(2*tau*t) |

4% agreement, but the constant was fed in and the accumulation checked against it. Validates the Gauss-Markov implementation, not the physics against nature.

**Overall.** Where the comparison is genuinely like-for-like, this pipeline matches the literature. Where it looks dramatically better, the reason is identifiable every time -- metric blindness, synthetic versus real gait, or a genuine architectural difference. **Nothing comes out worse than published work, which is itself a mild warning sign:** gait here is perfectly periodic, identical every step, on flat ground with a rigid pack. Real gait is not. These figures are optimistic by an unknown margin, and closing that needs hardware rather than more simulation.

**Step detection:** 99%+ on synthetic gait across cadences 1.0-2.0 Hz. A stationary node reports **0 steps and 0 m** — previously 215 steps and 95 m of phantom displacement.

**ZARU gyro bias observer.** Benefit depends on resting more often than the gyro bias time constant (tau = 80 s). RMS heading error over 1 hour, 8 runs:

| Rest cadence | Observer off | Observer on |
|---|---|---|
| every 150 s (~2 tau) | 1.58 deg | 1.49 deg (-5%, within noise) |
| every 60 s (< tau) | 1.58 deg | **1.20 deg (-24%)** |

Rest less often than tau and the bias has largely decorrelated before it is next observed, so the estimate is stale and subtracting it adds noise rather than removing bias. This is a real limit of ZARU at a trunk mount, where — unlike a foot mount — there is no per-stride zero-velocity instant to exploit.

## 7. EKF / Switching-Point Integrity Check — requires redesign, not yet done

The gateway-side EKF prediction step (thesis Eq. 10.2-10.3) and the Switching-Point Integrity Check (Eq. 10.19-10.20) were originally built assuming a **strapdown-INS + ZUPT prior**, with process noise Q derived from clean IMU bias-instability/noise-density statistics (Eq. 3.17's closed-form growth laws).

**The SHS-based prior this document describes has fundamentally different error sources**: step-length model error, missed/false step detection rate, and heading drift between radio-fix corrections — none of which have a closed-form growth law equivalent to Eq. (3.17). The existing Q matrix and covariance-propagation logic do not apply as-is.

**This redesign has not been started.** It sits downstream of the IMU simulation described in this document (the simulation produces a position estimate; the EKF/integrity check consumes it) — the IMU simulation itself is complete and usable on its own, but is not yet safely pluggable into the rest of the CA-TBL Job 1 / Job 2 framework until this redesign is done.
