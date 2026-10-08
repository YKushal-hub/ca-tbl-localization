# PART A — filter consistency (system 1)

Self-contained. Do not read Part B to do this. Time budget: **4.5–5 h of work**; compute runs beside the code.
Written 2026-10-08. Evidence references `context.md` (C§n) and the scoreboard in §2.

## 1. Goal, and what this part owns

**Goal.** Make the position filter's covariance honest, for **every arm**, so that (a) accuracy rises
across the board and (b) `ca_tbl_full` ends with no significant loss to any other arm on accuracy and the
best fix availability.

**Why this and not something else (measured).** The EKF treats persistent IMU errors (a gyro bias and a
scale error that are constant for the run) as independent increments. Its covariance is 3–54× too small
in variance (C§6.5). Consequences: innovation gates that should protect it reject good data (replay:
gates alone make the endgame worse), and every consumer of `P` (PCS, integrity check) reads a wrong
number. Already measured in the closed-loop simulator: multiplying the IMU increment covariance by 10
takes the endgame from 4.8 / 16.1 m to 2.7 / 8.0 m (CEP50 / CEP95) and 5 km from 16.2 / 85.0 to
11.7 / 61.7 m. Replay says an error-state filter reaches 1.5 / 4.7 m and 7.2 / 60.4 m.

| owns | files and symbols |
|---|---|
| Part A only | `src/lora-physics-engine/model/ca-tbl-policy.{h,cc}` (class `CaTblEkf`, `CaTblPolicyConfig`, `Step()` only where it calls the EKF), `ca-tbl-imu-model.{h,cc}`, the two `SetImuControl` call sites and the IMU `Configure` calls in `scratch/ca_tbl_causal_sim.cc` |
| shared, append-only | the `cmd.AddValue` block and `SpecOptions` in `ca_tbl_causal_sim.cc` (your block: `// --- S2-A accuracy flags ---`); `scripts/catbl_gates.sh` (your gates: **G100–G109**) |
| numbers reserved | TECH_DEBT **N.30–N.34**; `report/PREREG_ACC_A_*.md`; `report/ACC_A_RESULTS.md`; `results/acc_a_*` |
| never touch | `kSpecProfile` (S1 only); `docs/SCENARIO_FLAGS.md` by hand (regenerate with `tools/regen_flags.py`); Part B's files (the ranging, dual-base and relay-chain code) |

**Independence rule.** You are judged against the unmodified preset on your own binary. A flag you add
is read by all arms; with it off, every output is byte-identical to the binary without it.

## 2. Baseline you are measured against

The 102-setting preset (`e308c14f`: FIXES-N26's four fixes are in `--specProfile`). Closed loop, 20 seeds,
A = 5 km, B = endgame (`--boxHalf=750 --finderMode=1`), `--duration=1800`. My cells (exploratory, not
citable) are identical to the tracked FIXES-N26 `ALL_main` / `ALL_end` cells, 320 of 320 values.
Columns: five-node fix availability / five-node within 25 m / tag-0 CEP50 / CEP95 (m) / J per node.

| arm | A | B |
|---|---|---|
| `ca_tbl_full` | 0.285 / 0.613 / 16.2 / 85.0 / 16.7 | 0.562 / 0.968 / 4.8 / 16.1 / 23.0 |
| `fixed_threshold` (same four fixes) | 0.274 / 0.625 / 16.7 / 72.9 / 16.9 | 0.545 / 0.964 / 4.6 / 17.6 / 22.0 |
| `static_2p4` | 0.031 / 0.683 / 12.5 / 79.4 / 14.2 | 0.271 / 0.968 / 4.3 / 13.4 / 5.8 |
| `static_868` | 0.165 / 0.319 / 52.3 / 1 807 / 6.1 | 0.371 / 0.613 / 7.8 / 29.5 / 6.4 |
| `poucet_fusion` | 0.238 / 0.499 / 29.7 / 290.0 / 462.5 | 0.506 / 0.862 / 5.4 / 18.9 / 416.2 |
| `lora_fusion` | 0.226 / 0.513 / 36.1 / 2 308.6 / 16.8 | 0.501 / 0.885 / 5.9 / 18.5 / 9.5 |

With a throwaway patch that multiplies the IMU increment covariance by 10 for all arms (this part
turns it into a flag), `ca_tbl_full` becomes 0.278 / 0.638 / 11.7 / 61.7 / 17.3 (A) and
0.575 / 0.977 / 2.7 / 8.0 / 26.0 (B). Paired against the other arms at that setting: significant WIN over
`poucet_fusion` and `lora_fusion` on every accuracy metric; tie with `fixed_threshold` and `static_2p4`
on accuracy; fix availability WIN over every arm; one LOSS: A five-node within 25 m against `static_2p4`
(−0.067 [−0.129, −0.016]), which comes from the four other subjects (Part B's territory).

## 3. Rules for the fast lane

Kept: default-off flag; null control (NC1–NC9) before reading any measurement; a one-page
pre-registration committed **before** the first measured cell; a gate with a negative fixture per
mechanism; 20-seed paired evaluation; every run with `--outdir=<temp dir>` (the default writes into
tracked `results/causal`). Tune on seeds 21–40, evaluate on seeds 1–20. Deferred: the frozen-worktree
`dirty 0` campaign and adoption into the preset (S1's call). Say "exploratory grade" in your results note.
Commits: **YKushal <kushalyalamanchi@gmail.com>**, one change per commit, no attribution trailer anywhere.

## 4. Step 0 — set up (50 min; the reference rebuild runs in the background)

```bash
git fetch origin
git checkout -b acc/a-filter origin/work/2026-09-27-spec-b1        # the main line, includes e308c14f
git checkout origin/work/2026-10-09-acc-reruns -- analysis/accuracy_replay context.md plan.md \
    plan_part_a_filter.md plan_part_b_hybrid.md                    # research files and scoreboard tools
git add -A && git commit -m "import S2 accuracy research files"
CC=gcc-11 CXX=g++-11 ./ns3 configure --enable-examples --build-profile=optimized   # gcc 13 also works
./ns3 build scratch/ca_tbl_causal_sim                              # ~15 min on 4 cores
(nohup bash tools/nullcontrol/rebuild_reference.sh $HOME/catbl_ref_spec0 > ~/ref.log 2>&1 &)   # 20–40 min
```
Then run the baseline into a temp directory (cells use the scripts in `analysis/accuracy_replay/`):
```bash
cd analysis/accuracy_replay
# the preset already contains the four fixes; do not repeat them
for s in $(seq 1 20); do
  ./runcell.sh BASE_A $s --arms=ca_tbl_full,fixed_threshold,static_868,static_2p4
  ./runcell.sh BASE_B $s --arms=ca_tbl_full,fixed_threshold,static_868,static_2p4 --boxHalf=750 --finderMode=1
  ./runcell.sh BASEC_A $s --arms=poucet_fusion,lora_fusion
  ./runcell.sh BASEC_B $s --arms=poucet_fusion,lora_fusion --boxHalf=750 --finderMode=1
done        # point BIN and LIBS at your build; use xargs -P<cores> to parallelise
python3 score.py   # edit the item lists at the bottom to your cell names
```
(`runcell.sh` also sets `CATBL_RANGELOG=1`; with an unpatched binary that variable does nothing.
`runq.sh` is only for the throwaway Qu patch; you will not need it once `--imuQuScale` exists.)
Expect your baseline to reproduce the table in §2 to within one machine's `-march=native` noise; check
`ca_tbl_full` first. Write down any difference.

## 5. The work

### A1 — `--imuQuScale` (40 min)  ·  gate G100

**What.** A default-off flag (`1.0`) that multiplies the IMU increment covariance handed to the filter.

**Where.** `scratch/ca_tbl_causal_sim.cc`: the two calls `policy.SetImuControl(imuPred->ux, imuPred->uy,
imuPred->qu[0], imuPred->qu[1], imuPred->qu[2])` (tag 0, ~line 12081) and `sb.policy.SetImuControl(ux, uy,
qu[0], qu[1], qu[2])` (subjects, ~line 10269). Multiply `qu[0..2]` by the flag. Register
`cmd.AddValue("imuQuScale", "<help text written as an argument: why 1, why this is not 10>", ...)` in your
flag block. The help text must say what it is: a stand-in for the correlation the filter ignores.

**Null control.** Flag at 1.0 and flag absent give byte-identical `arm_summary.csv` and traces, NC1–NC9.

**Gate G100** (`scripts/acc_a_gate.{sh,py}` and `_selftest.sh`):
- check 1: `--imuQuScale=1` and no flag give identical outputs (cmp every file);
- check 2: with `--imuQuScale=10`, on cycles with `imuPredicted=1` and no fix, the growth of `traceSigma`
  from one cycle to the next is at least 5× that at 1.0 (median over cycles);
- negative fixture: feed the gate two runs both made at 1.0 and call one of them "10": check 2 must fail.

**Sweep** (tuning, seeds 21–30, `ca_tbl_full` and `static_2p4`, both configurations): k ∈ {3, 10, 30}.
Pick k\* by the smaller tag-0 CEP95 in B without worsening A. Expected near 10.

### A2 — `--imuErrorStates` (2–2.5 h + 40 min verification)  ·  gate G101

**What.** A second filter, selected by a default-off flag, that carries the IMU's heading bias and scale
error as states, so their effect on position is correlated across cycles, as it is in the generator.

**The model** (reference implementation: `analysis/accuracy_replay/replay.py`, class `AugEkf`):
- state `z = [x, y, ψ, b, s]` — position, accumulated heading error, gyro bias, scale error.
- control input is the IMU's reported displacement `u = (ux, uy)` for the cycle.
- prediction: `Δ = (1 − s) · R(−ψ) · u`, i.e.
  `x' = x + (1−s)(cosψ·ux + sinψ·uy)`, `y' = y + (1−s)(−sinψ·ux + cosψ·uy)`, `ψ' = ψ + b·dt`, `b' = b`, `s' = s`.
  Jacobian `F = ∂z'/∂z`: identity plus
  `∂x'/∂ψ = (1−s)(−sinψ·ux + cosψ·uy)`, `∂y'/∂ψ = (1−s)(−cosψ·ux − sinψ·uy)`,
  `∂x'/∂s = −(cosψ·ux + sinψ·uy)`, `∂y'/∂s = −(−sinψ·ux + cosψ·uy)`, `∂ψ'/∂b = dt`.
- process noise `Q`: `σ_w²` on x and y (`σ_w = 0.05 m`, the generator's dwell sigma), `arw²·dt` on ψ
  (`arw = gyroNoiseDensity × imuArwScale`), and **a bias random walk `q_b²·dt` on b (new; OURS)**. The
  generator's bias is constant, so the replay flatters the filter; a real bias drifts. Sweep `q_b` ∈ {0,
  3e-7, 1e-6} rad/s/√s. Scale: no process noise.
- initial covariance: position block as `Initialize()` gives it; `ψ`: 1e-8; `b`: `σ_b²`; `s`: `σ_s²`, with
  `σ_b = gyroBiasSigmaRadS`, `σ_s = scaleSigma` from `CaTblImuParams`, times the multipliers of A3.
- range update: sequential scalar EKF update, `H = [dx/r, dy/r, 0, 0, 0]` (same as today). Position fix
  update (`FusePosition`, `FusePositionCov`): `H` selects x and y; gain `K = P Hᵀ S⁻¹` is 5×2.
- everything that reads the prior or the posterior reads only the 2×2 position block: `PositionNis`,
  `MahalanobisFromPrior`, `MahalanobisFromPriorWithR`, `InflatePos` (scale `P[0][0]`, `P[1][1]` as today).
  `RevertToPrior` restores the five states and their covariance from the snapshot taken in the predict.

**Where.** Do not rewrite `CaTblEkf`. Add a member struct (`m_ie`) and, at the top of each public method, a
branch `if (m_ie.on) return IeMethod(...)` so that **flag-off arithmetic is untouched and byte-identical**.
The interface the policy uses (about 20 calls; `grep -o "m_ekf\.[A-Za-z]*" ca-tbl-policy.cc | sort -u`):
`Init, SetProcessNoise, Predict, PredictWithControl, Update, FusePosition, FusePositionCov, PositionNis,
MahalanobisFromPrior, MahalanobisFromPriorWithR, RevertToPrior, InflatePos, X, Y, Vx, Vy, Pxx, Pxy, Pyy,
TracePos, PriorX, PriorY`. For `Vx, Vy` return `u/dt` of the last control (consumers: `mobilityFromRange`,
`entryVelocity`, the witnesses). `PredictWithControl` ignores its `quxx, quxy, quyy` arguments in this mode.
Add `CaTblPolicyConfig` fields for the priors and `q_b`; the scenario fills them from `CaTblImuParams`.
When no IMU control is pending in a cycle (`m_imuPending` false) fall back to the constant-velocity
position prediction exactly as today, leaving ψ, b, s unchanged.

**Verification before any simulator run (A2v).**
1. `analysis/accuracy_replay/make_ie_vectors.py` (write it, 20 lines): run `AugEkf` on a fixed synthetic
   stream (200 cycles, two anchors, known truth, `u` from the generator in `synth_imu.py`) and print the
   final state and covariance.
2. A scratch unit test in the style of `scratch/ca_tbl_integrity_test.cc` feeds the same stream to the C++
   filter. Pass: agreement to 1e-6 relative on the state and covariance.
3. On a logged stream (`rangelog_patch.txt`, one cell): the C++ filter's estimate and the Python reference
   agree to < 0.5 m median.
4. NC1–NC9 with the flag off.

**Gate G101:** flag off ⇒ outputs identical to no flag; flag on with `q_b = 0` and the generator's priors ⇒
the unit test passes (run it from the gate); negative fixture: perturb the Jacobian sign in a `/tmp` copy
and the unit test must fail.

**Stop rule.** If A2 and A2v are not working at the 3 h mark, stop, ship A1, and write what is missing in
TECH_DEBT N.31.

### A3 — IMU-quality flags and stress cases (30 min code, 30 min compute)  ·  gate G102

Three default-off multipliers on `CaTblImuParams`: `--imuBiasScale`, `--imuScaleScale`, `--imuArwScale`
(all 1.0). Apply where the dead-reckoner is configured (`imuPred->rk.Configure(CaTblImuParams(), ...)`, ~line
7707, and the subjects' `ex.rk`). Add `--imuFilterBiasScale` (1.0) that scales only the filter's *declared*
`σ_b`, to test a mis-declared prior. G102: off ⇒ identical; `--imuBiasScale=10` ⇒ the logged `psi` column
in `imu_*.csv` drifts ≥ 5× faster in the median; negative fixture as in G100.

Stress cases (tuning seeds 21–30, `ca_tbl_full` and `static_2p4`, both configurations):
gyro bias ×1, ×10, ×30; mis-declared prior (`--imuFilterBiasScale` 0.1 and 10) at ×10; bias random walk
via `q_b` set above the generator's (the generator has none, so this tests filter tolerance). **You do not
need a realistic IMU model: these show the estimator does not depend on the generator being right to
within an order of magnitude.** They do not show it works on hardware; say so in the note.

### A4 — Evaluation, pre-registration, note (60 min plus compute)

1. **Pre-register first** (`report/PREREG_ACC_A_FILTER.md`, one page, committed before the first cell):
   populations (seeds 1–20, 720 cycles; tag 0 for CEP; five nodes for availability), estimator (paired seed
   bootstrap, 4 000 resamples), the predictions and falsifiers below, the acceptance scoreboard of §7,
   and "what this cannot establish".
2. Cells (tuned values from the 21–30 sweeps): `{off, --imuQuScale=k*, --imuErrorStates (q_b*)}` × `{A, B}` ×
   seeds 1–20 × **all six arms** (the change is filter-wide). Names `results/acc_a_off`, `acc_a_q`, `acc_a_ie`.
3. `scoreboard_pairs.py` for each setting (edit the cell names): `ca_tbl_full` minus every other arm.
4. Write `report/ACC_A_RESULTS.md` (exploratory grade) and TECH_DEBT N.30 (the covariance finding), N.31
   (A2 outcome), N.32 (stress results). Commit and push the branch.

## 6. Registered predictions and falsifiers (closed loop, tag 0, paired against the unmodified preset)

| id | prediction | falsifier |
|---|---|---|
| P-A1 | `--imuQuScale=k*` (≈10): A CEP50 ≤ 13 m, CEP95 ≤ 70 m; B CEP50 ≤ 3.2 m, CEP95 ≤ 10 m (measured at k = 10: 11.7 / 61.7 and 2.7 / 8.0, so this is a replication) | B CEP95 improves < 20 % at every k |
| P-A2 | `--imuErrorStates`: A CEP50 ≤ 9 m, CEP95 ≤ 62 m; B CEP50 ≤ 2.0 m, CEP95 ≤ 6 m; median `error² / trace(P)` ≤ 10 (37.5 today, 6.3 in replay) | gain over A1 < 20 % in B; or C++ disagrees with the reference by > 0.5 m |
| P-A3 | at gyro bias ×10 and ×30, B CEP50 within 25 % of its ×1 value (replay 2.1 / 2.3 / 2.4 m) | any stress case worse than the unmodified preset by > 10 % |
| P-A4 | no arm's fix availability moves by more than 0.01; escalation counts are reported, not predicted | any arm moves by more |
| P-A5 | the TDoA and range innovation gates stop hurting once `P` is consistent: B CEP95 with gates ≤ without | gates still worsen B by > 5 % |

If a falsifier fires: stop at the previous step and record it. A null result is a result.

## 7. Acceptance scoreboard (fixed now)

For A and B, `ca_tbl_full` against each of `fixed_threshold`, `static_2p4`, `static_868`, `poucet_fusion`,
`lora_fusion`, paired over seeds 1–20, 95 % seed bootstrap, **same binary and the same filter flags on every
arm**: five-node fix availability — WIN against every arm; five-node within 25 m and tag-0 CEP50, CEP95 — no
LOSS against any arm and WIN against at least three; energy reported, not scored. Expected (opinion): fix
availability and both fusion arms — yes; `static_2p4` and `fixed_threshold` — ties; the endgame is
saturated near 2 m. After this part the fusion comparators improve too (replay endgame: `poucet_fusion`
1.5 / 4.1 m against `ca_tbl_full` 1.4 / 4.1 m), so the endgame win over `poucet_fusion` may become a tie.

## 8. Timetable

| clock | work |
|---|---|
| 0:00–0:50 | Step 0 (reference rebuild runs beside) |
| 0:50–1:30 | A1, NC, G100 |
| 1:30–4:00 | A2 + A2v; the k sweep and baseline cells run beside it |
| 4:00–4:50 | A3 flags, stress sweep, A4 pre-registration and cells |
| 4:50–5:20 | results note, TECH_DEBT, commit, push |

Compute assumption: 8+ cores. A full 6-arm 20-seed setting is ~150 core-minutes (about 40 min on 4 cores,
15 min on 12). Three settings plus sweeps is ~2 h on 4 cores. On 4 cores, run the sweeps on 2 arms and 10
seeds only, as written.

## 9. Risks

| risk | effect | mitigation |
|---|---|---|
| the C++ port disagrees with the reference | 2–3 h lost | A2v precedes any simulator run |
| replay does not predict closed loop for A2 | smaller gain than predicted | P-A2 falsifier; A1 is already measured |
| `-march=native` | byte-identity is per machine | rebuild the reference on this machine; never compare outputs across machines |
| PCS thresholds were fitted on the old covariance (CAL-03) | PCS decisions shift | do not touch the PCS form (RULES.md §2); report escalation counts; refit CAL-03 later as its own step |
| the integrity check's distance reads `P` | its fault-free SUSPECT share changes | report it before and after; do not retune here |

## Appendix — after both parts (not a dependency)

Whoever finishes second merges `acc/a-filter` and `acc/b-hybrid` (two append-only conflicts expected: the
flag block and `catbl_gates.sh`; keep both sides), builds, runs NC1–NC9 flags off, runs the 2 × 2
{A on/off} × {B on/off} scoreboard on all seven arms, then tests whether a consistent covariance makes the
integrity check usable (re-fit `--integrityChi2` on seeds 21–40 with the `fixes_n26_run.sh` fit procedure,
then the fault matrix against `fixed_threshold`). That last test is the only route to beating
`fixed_threshold` in anything; a tie is a valid result. Then the full gate suite once (~25 min).
