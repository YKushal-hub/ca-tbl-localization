# CA-TBL — Backpack IMU Step-and-Heading System (SHS)

Branch `IMU_SHS_V1.1`.

This branch contains **only** the IMU / SHS simulation code. It is a standalone
drop-in for the ns-3 `lorawan` module and carries no other project history: no
ns-3 tree, no rest-of-`lorawan` sources, no content from any other branch.

The IMU (MPU-6050) is assumed **trunk-mounted, in or on the field node's
backpack** — not foot-mounted. ZUPT needs a true zero-velocity instant that only
exists at the foot during mid-stance, and the trunk never stops during walking.
So this is a Step-and-Heading System (detect steps, estimate step length
empirically, integrate heading separately, update position once per step) rather
than strapdown double-integration plus ZUPT.

## What is here

```
src/lorawan/
├── model/
│   ├── lora-shs-parameters.h              all constants/params -- edit values HERE
│   ├── lora-imu-sensor-submodule.{h,cc}   Step 0: MPU-6050 3-axis sensor model
│   ├── lora-step-detector-submodule.{h,cc} Steps 1-2: Iluz detection + Ho et al. extraction
│   ├── lora-step-length-submodule.{h,cc}  Step 3: Weinberg + online k_w calibration
│   ├── lora-heading-submodule.{h,cc}      Step 4: gyro integration + RF-aided correction
│   ├── lora-position-update-submodule.{h,cc} Step 5: position update
│   ├── lora-gyro-bias-observer.{h,cc}     ZARU gyro bias observer (aids Step 4)
│   ├── lora-turn-detector.{h,cc}          yaw-based turn detection (diagnostics)
│   └── lora-shs-engine.{h,cc}             orchestrator, wires Steps 0-5 together
├── test/lorawan-shs-test-suite.cc         12 test cases, all passing
├── examples/shs-imu-example.cc            end-to-end runnable scenario
└── doc/
    ├── imu-shs-design.md                  WHY each decision, with citations
    └── imu-shs-implementation.md          build order, formulas, constants
```

All classes are in `namespace ns3::lorawan`.

## Pipeline

| Step | Piece | Method | Governing source |
|---|---|---|---|
| 0 | Sensor model | `a~ = a + b_a + n_a`, `w~ = w + b_g + n_g`, Gauss-Markov bias | InvenSense PS-MPU-6000A-00 Rev 3.4; Rudyk et al., Sensors 20(17):4841, 2020 |
| 1 | Step detector | Band-pass 0.5-3 Hz, 5 s window, 2 Hz matched filter, 2-15 steps/window = gait | Iluz et al., JNER 11:48, 2014 |
| 2 | Per-step window | one step = valley -> peak -> valley of vertical accel | Ho, Truong, Jeong, Sensors 16(9):1423, 2016 |
| 3 | Step length | `s_w = k_w * (a_max - a_min)^(1/4)`, k_w calibrated online | Weinberg, Analog Devices AN-602, 2002 |
| 4 | Heading | `theta += (w_z - b_g_hat) * dt`, theta_0 and drift from radio fixes | thesis Eq. (3.13) reduced to 2D |
| 5 | Position | `p_new = p_old + s_w * [cos(theta), sin(theta)]` | standard SHS update |

Angle convention throughout is standard mathematical: theta = 0 along +x,
increasing counterclockwise. This matches the MPU-6050's native right-hand-rule
gyro Z output, so no sign flip is applied anywhere.

## Integrating into an ns-3 tree

Built and tested against **ns-3.48** with the `lorawan` module present.

1. Copy `src/lorawan/{model,test,examples,doc}` over the same paths in your
   ns-3 tree's `lorawan` module.

2. Add to `src/lorawan/CMakeLists.txt`, in `set(source_files ...)`:

   ```cmake
   model/lora-imu-sensor-submodule.cc
   model/lora-step-detector-submodule.cc
   model/lora-step-length-submodule.cc
   model/lora-heading-submodule.cc
   model/lora-position-update-submodule.cc
   model/lora-gyro-bias-observer.cc
   model/lora-shs-engine.cc
   model/lora-turn-detector.cc
   ```

   in `set(header_files ...)`:

   ```cmake
   model/lora-shs-parameters.h
   model/lora-imu-sensor-submodule.h
   model/lora-step-detector-submodule.h
   model/lora-step-length-submodule.h
   model/lora-heading-submodule.h
   model/lora-position-update-submodule.h
   model/lora-gyro-bias-observer.h
   model/lora-shs-engine.h
   model/lora-turn-detector.h
   ```

   and in the `build_lib(... TEST_SOURCES ...)` list:

   ```cmake
   test/lorawan-shs-test-suite.cc
   ```

3. Add to `src/lorawan/examples/CMakeLists.txt`, in `set(base_examples ...)`:

   ```cmake
   shs-imu-example
   ```

4. Build and run:

   ```bash
   ./ns3 configure --enable-examples --enable-tests
   ./ns3 build lorawan
   ./test.py -s lorawan-shs
   ./ns3 run shs-imu-example
   ```

The only ns-3 dependency is `ns3/random-variable-stream.h` — the sensor model
draws its noise and Gauss-Markov bias from a `NormalRandomVariable` so runs are
reproducible from the ns-3 RNG. Everything downstream of Step 0 is pure C++.

## Example scenarios

```bash
# straight traverse, default 120 s
./ns3 run shs-imu-example

# closed square loop with real turns, known deployment state,
# 200 m straight calibration walk, then no fixes for an hour
./ns3 run "shs-imu-example --duration=3600 --loopSideM=200 --calibrationWalkM=200 --fixIntervalS=0"

# quantify a wrong axis choice
./ns3 run "shs-imu-example --wrongAxisWiring=true"
```

Run `./ns3 run "shs-imu-example --PrintHelp"` for the full option list.

## Status

Twelve test cases, all passing, covering each submodule in isolation plus
end-to-end runs on a straight route and a closed loop with real turns.

Measured (see `doc/imu-shs-design.md` Section 6b for full tables and method):

- Uncorrected heading drift over 1 hour: **1.79 deg (1 sigma)** measured vs
  **1.86 deg** predicted from the Rudyk bias instability — 4% agreement.
- 1-hour closed loop, 5067 m walked, no fixes at all after calibration:
  **7.88 m final error (0.16% of path)**, 7238 of 7239 steps detected.
- 1-hour straight traverse, 5267 m: **140 m (2.66%)**. The 18x gap against the
  loop is **geometric, not a turning effect** — error scales with displacement
  from the last known position, not with distance walked.
- Turn detection: 89% recall, 0 false positives on the straight route.
- Stationary node: 0 steps, 0 m.

For comparison, published waist-worn PDR reports ~2% error (Alvarez et al.,
Sensors 12(8):10536). With good calibration this pipeline sits at or inside that
band. **The dominant error source is k_w calibration quality, which is limited
by radio fix noise — not by IMU physics.**

## Known limitations — read before relying on this

These are carried in full, with reasoning, in `doc/imu-shs-design.md` Section 6.
The load-bearing ones:

1. **Physical axis assignment is still a placeholder.** Which MPU-6050 axis
   physically points up inside the backpack is not yet known. `lora-shs-engine.cc`
   wires `measAy` as vertical and `measAx` as anterior-posterior. If the axis
   facing laterally is the one wired as vertical, every turn dumps non-gait
   signal into the detector's primary input — measured at 17.8% missed steps.
   The `--wrongAxisWiring` flag and `LateralAxisTurnResponseTest` quantify it.
2. **No published work validates RF-aided heading correction using LoRa.** The
   established literature (PMC7472062; IEEE Access 2021) uses UWB or WiFi/BLE,
   both far higher update-rate and accuracy than LoRa. This is a principled
   adaptation, not a proven design. Revisit against real field data.
3. **Accelerometer bias instability (150 ug) and its time constant (300 s) are
   estimates**, not MPU-6050 measurements. The Rudyk Allan-variance study covers
   the gyroscope only. Do not treat them as equal-confidence to the gyro figures.
4. **FS_SEL (+/-500 deg/s) has no supporting citation**; AFS_SEL (+/-8g) is an
   engineering decision informed by, not stated in, the cited placement studies.
5. **Turn-injection magnitudes (trunk lean 10 deg, 0.75 s onset) are unsourced
   estimates.** The literature supports that these mechanisms are real and
   measurable at a trunk mount, not their magnitudes.
6. **Not wired into the EKF or Switching-Point Integrity Check.** Those were
   derived against a strapdown+ZUPT prior whose error model does not apply to an
   SHS prior — step-length model error, missed/false detection rate and heading
   drift have no closed-form growth law equivalent to Eq. (3.17). That redesign
   has not been started (design doc Section 7). This engine produces a position
   estimate and deliberately stops there.
7. **No rough-terrain / trunk-sway correction.** IPM-INS was identified as the
   best available proxy for uneven SAR ground but is not integrated.
