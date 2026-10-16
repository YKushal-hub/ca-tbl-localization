# Integrating the local line into `main` — comparison, research, and what was ported

Written 2026-10-16 on `integrate/local-line-2026-10-16`, branched from `main` at `4ca0dff`.

## 0. The two lines

Both share history up to `56d47fb` (2026-09-07, PR #1). After it they never met.

| | `main` (this branch's base) | the local line |
|---|---|---|
| commits since the split | 393 | 81 |
| span | 2026-09-04 → 2026-10-09 | 2026-09-02 → 2026-10-15 |
| topology | **corrected** (one population of tri-band nodes, finders; `--topology=0`) | legacy disjoint rings (the pre-`99fdf1a` model) |
| scenario | 19 694 lines, 97–102-setting `--specProfile`, mesh, transport, TDoA, 89 gates | 4 500 lines, ~20 gates |
| commit messages | no assistant attribution (owner rule, 2026-09-05) | 25 carry a `Co-Authored-By` trailer |

**`main` is the base.** It is the line the owner's decisions were taken on, it carries the
topology correction, and every result on the local line was produced on the topology `main`
records as "a different deployment" (`RULES.md`). The local line is integrated by **porting**
what it has that `main` lacks, each piece behind its own default-off flag with a null control
against `main`'s binary — not by a git merge, which conflicts in 13 files (the engine core
among them) and would import the trailers `main` forbids.

The local line's history is kept, attribution stripped, on `archive/local-line-2026-10-16`;
`SHA_MAP.md` maps its old SHAs to the rewritten ones so every ported sidecar resolves (G13).

## 1. What each line has that the other does not

### 1.1 Only on `main` — kept as is

The corrected topology and finders; the mesh, transport and application layers; tier-1 TDoA;
the NLOS classifier; the calibrated PCS (CAL-01..03); the spec preset and its conformance
programme; LR1121 basis; half-duplex; the comparison arms (Poucet, LoRa fusion); FIXES-N26;
the gate suite G1–G90. None of this is touched.

### 1.2 Only on the local line — and whether it applies to `main`

| item | applies to `main`? | evidence | ported as |
|---|---|---|---|
| **D-8 — tier 2's sensitivity table is the SX1280 1625 kHz row** | **yes, and worse there**: `main`'s SY-3 shifts the same table by 10·log10(BW / 800 kHz) as though it were an 800 kHz row, so tier 2 is ~6 dB pessimistic at 812.5 kHz and ~3 dB at 1625 kHz | `lora-2p4-physics-parameters.cc` header "BW=800kHz baseline", SF7 −106 dBm; data sheet rev 3.2 Table 6-1 | `--t2SensitivityRowKHz` (§2.1) and gate **G93** |
| **A data-sheet sensitivity gate** | yes — `main`'s G14 checks the SF *index*, which D1 broke; nothing checks the *values*, which is how D-8 passed every gate | G19 on the local line, red at default | **G93** + fixture |
| **IMU sensor model and 5-state filter** | yes, as an alternative to DC-1 (§1.3) | closed-form check, NEES | §2.3 |
| M-01 is the validation harness's self-collision | **yes** — `main`'s `Simulation.md` §2.2 uses M-01 to rule out SF12 | §3 | a finding, recorded |
| the A3 handshake defect (off-time not armed) | **no** — `main`'s handshake is a frame and `AccountFrameTx` arms the off-time | `ca-tbl-energy-model.cc` | record only |
| the claims audit of 2026-09-02..16 (W11, W12, Claim 2 6.53×, Claim 4 24.5 %, joint criterion) | superseded — every claim was re-measured on the corrected topology by B3 (2026-09-28) | `report/B3*` | record only (archive branch) |
| `--pcsForm=min`, `--uwbSiting`, the exclusion response | no — `main` has the calibrated PCS and `--sitingMode` | — | not ported |

### 1.3 The two IMUs — which is better

| | `main`: `CaTblImuDeadReckoner` (`--imuPredict`, DC-1) | local: `CaTblImuModel` + `CaTblImuEkf` (`--imuPrior`) |
|---|---|---|
| what it is | displacement-level error model; drives the CV filter's prediction as a control input with an accumulated-covariance increment | gyro and stride sensor model; drives a 5-state EKF [x, y, θ, b, s] that **estimates** gyro bias and stride scale |
| gyro bias | **`N(0, σ²)` with σ = the MPU-6050's bias instability, 8.816 °/h** | BMI270 data sheet: ZRO ±0.5 °/s, TCO 0.015 °/s/K; residual after power-on calibration 0.15 °/s (10 K drift — **ours**) |
| covariance | increments of Σ_acc; "correlation with earlier states is still ignored" (its own header) | propagated through the Jacobian; consistency measured (NEES) |
| validation | G40: the input reaches the filter | closed forms by independent code (Allan deviation, E[sin θ sin θ]); NEES 2.0 on short outages; moment-matched coasting holds NEES 1.5–2.7 to 1000 s |
| integration | wired into Case B (DC-2), the witness (DC-9), subjects | tag 0 only, legacy topology |

**The bias parameter is a category error.** Bias *instability* is the floor of a running
gyro's Allan deviation — how far the bias wanders over ~100 s. The bias a filter faces is the
**turn-on and temperature residual**, which the MPU-6050 data sheet gives as ±20 °/s
uncalibrated and which even a calibrated MEMS gyro holds only to ~0.05–0.5 °/s. Drawn at
8.816 °/h, `main`'s dead reckoner accumulates ≈ 2.4° of heading error in 1000 s; at a calibrated
0.15 °/s it is ≈ 150°. And DC-1's filter has no bias state, so it cannot learn the error from
fixes. Everything downstream of DC-1 — Case B's heading (DC-2), the IMU witness (DC-9) — is
measured with a near-perfect gyro.

**Verdict.** The local model is the better *model*; `main`'s is the better *integrated*. The
port therefore (a) adds the local sensor model and its closed-form gate as a pure addition,
and (b) adds a default-off flag that lets DC-1 draw its bias at a turn-on figure, so the
optimism can be measured on `main`'s own configuration. Replacing DC-1's filter with the
5-state EKF touches Case B, Case D, the sparse cases and the witness; it is planned (§4), not
done here.

## 2. Ported

Each piece is default-off or a pure addition; the null control is `main`'s binary at `4ca0dff`
against this branch's, on `tools/nullcontrol/args/NC1`–`NC9` plus the cases the new flags can
reach (§2.1).

| piece | files | flag / gate | effect at defaults |
|---|---|---|---|
| D-8 correction | `lora-2p4-physics-parameters.{h,cc}` (`Sx1280LowPowerSensitivityRow`), scenario | `--t2SensitivityRowKHz` | none (null control) |
| data-sheet sensitivity gate | `scratch/ca_tbl_sensitivity_probe.cc` (`--bw2p4`), `scripts/sensitivity_fidelity_check.py` (`--bw2p4`), `scripts/catbl_sensitivity_fixture.sh` (7 directions) | **G93**, FAIL BY FINDING | — |
| IMU sensor model | `ca-tbl-imu-sensor-model.{h,cc}` (renamed from the local `ca-tbl-imu-model.*`; `main` uses that name for DC-1) | **G91** | not wired: none |
| 5-state filter + moment-matched coasting | `ca-tbl-imu-ekf.{h,cc}` (standalone; the local `CaTblFilter` interface is not ported) | **G91**, **G94** | not wired: none |
| validator and closed forms | `scratch/ca_tbl_imu_validator.cc`, `scripts/imu_closed_form_check.py`, `scripts/catbl_imu_fixture.sh`, `scripts/imu_long_outage_check.py`, `scripts/catbl_imu_long_outage_fixture.sh` | G91, G94 | — |
| DC-1 turn-on bias | scenario (`Dc1ImuParams()`) | `--imuGyroBiasSigmaDps` (refused without `--imuPredict`) | none (null control) |
| G10's negative fixture | `scripts/catbl_citation_fixture.sh` (6 directions) | G10 | — |
| cited evidence held | `scripts/cited_evidence_gate.py`, `scripts/catbl_cited_evidence_fixture.sh` | **G92**, FAIL BY FINDING (15 paths) | — |
| findings | `docs/TECH_DEBT.md` N.27–N.30; `Simulation.md` M-01 annotated | — | — |
| the local line's evidence, as a record | `results/{imu_prior, imu_prior_sens, a3_handshake, a1_dwell, d8_sensitivity, e3_imu_integrity}` (summaries and sidecars only, as `.gitignore` keeps them); `report/{IMU_PRIOR,A3,A1,D8_STAGE1}_RESULT.md` with a legacy-topology banner; their registrations unedited; their runners and scorers (`run_a1_dwell.sh` as `run_a1_dwell_local.sh`, `main` having its own) | G13 resolves the sidecars through `SHA_MAP.md` | — |

Not ported: the local `CaTblFilter` interface and the filter's wiring into the policy (§4.3);
`--imuPrior`, `--imuCoastMomentMatch` and `--intraCycleHandshake` (local-line flags; the
handshake defect is already fixed on `main`); the claims audit of 2026-09-02..16 and its
documents (superseded by B3); `report/AUDIT_2026_10_15.md` (an audit of the local line's own
documents); the unrun `PREREG_D8_STAGE2.md` (written for the local line's claims, which `main`
does not make).

### 2.1 Null control

`main`'s binary at `4ca0dff` (built in this tree with the changes stashed) against this
branch's, every output file except `run_id.txt` compared byte for byte:

| case | arguments | files | result |
|---|---|---|---|
| NC1–NC9 | `tools/nullcontrol/args/NC*.args` | 41 | 9 of 9 IDENTICAL |
| X1 | NC1's + `--imuPredict=true` (DC-1, which no NC case reaches) | 13 | IDENTICAL |
| X2 | NC1's + `--bw2p4=1625000` (SY-3's shift, which D-8 interacts with) | 7 | IDENTICAL |
| X3 | `--specProfile=true --duration=600` (the preset) | 329 | IDENTICAL |

Run twice: on the D-8-only intermediate binary (commit 2) and on the final one (commits 2–5),
**12 of 12 IDENTICAL both times, 390 files.** The final binary rebuilt from the committed tree is
byte-identical to the one tested. Reachability: with `--t2SensitivityRowKHz=812.5` all 7 of NC1's
files change (seed 7: `static_2p4` fix cycles 11 → 95 — one seed, not a result); with
`--imuGyroBiasSigmaDps=0.15` all 13 of X1's do.

### 2.2 The full suite on this branch

Built from clean (every target); the twelve unit suites all pass, 0 failed (packet validator 76,
integrity 40, the rest as `SETUP_NEW_MACHINE.md` records). Gate suite: **82 PASS, 10 FAIL,
0 ERROR**, tree unchanged by the run.
- G91, G94 and G10's new fixture **pass**. G92 and G93 are FAIL BY FINDING, as intended.
- G4, G5, G6, G11 and G24 are `main`'s recorded reds.
- **G63, G87 and G89 are red on `main` itself**: run against `4ca0dff`'s binary they fail with
  identical numbers. They went red with `e308c14f`, after `main`'s last full suite
  (TECH_DEBT N.31). Not caused here, and not changed here.

## 3. Findings carried over

- **D-8** — as §1.2. Not corrected by default: a correction moves every tier-2 number and is a
  planned campaign (owner decision 2026-10-15, local line).
- **M-01** — `scratch/lora_empirical_validation.cc` metric 2 sends a packet every 0.1 s; an SF12
  frame lasts over a second, so SF12 frames collide with the harness's own previous frame
  (199/200 at 500 m, local line, seed 7). At 3 s spacing SF12 PDR at 500 m is 0.985–1.000. The
  "collision-dominated" reading in `Simulation.md` §2.2 is right about collisions and wrong
  about whose: they are the test's, not the channel's.
- **DC-1's gyro bias** — as §1.3.
- **E3** (run after the archive was cut, legacy topology): the inertial prior raises the
  integrity check's recall in every registered cell and its false-alarm rate in every cell —
  the operating point moved, the detector was not shown better (`report/E3_RESULT.md`).

## 4. Left, in order

1. D-8 stage 1 on `main`'s configuration (the preset at 5 km and in the endgame) — registered
   before it runs; the local line's stage 1 measured the legacy topology only.
2. DC-1 with a turn-on bias, on the preset — registered before it runs.
3. The 5-state filter as `--imuFilter`, wired through Case B and the witness, with its own null
   control and gate.
