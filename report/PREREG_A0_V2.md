# Pre-registration — A0 v2: the joint criterion, with the degenerate branch it was missing

Written 2026-10-14, **before any cell it will score exists.** Supersedes nothing: the A0
verdict on `results/p10_grid` (`report/A0_APPLIED.md`, FAIL 64/64) **stands as registered**
and is not re-scored under this document. A0 v2 applies only to cells whose sidecars
post-date the commit that adds this file, which `git log` and each `run_id.txt` make
checkable.

## 0. Why a v2 and not an amendment

`report/A0_APPLIED.md` found a defect in the registered rule by applying it: `dD = 0` arose
from `D_T = 0` **and** `D_B = 0` — a requirement no arm met — and the rule mapped that to
FAIL for the tiered arm. It proposed a fourth branch and **deliberately did not apply it**,
because amending a rule after seeing the data it scores is what pre-registration forbids.
That was right. The way forward is a new registration, on new cells, which this is.

## 1. What is unchanged from A0

Everything not listed in §2 is A0 verbatim (`report/PREREG_A0_JOINT_CRITERION.md` §1–§4):

    A, R, CYC = 0.95, 25.0, 1440
    A_fix(arm, seed) = |{ cycles : hadFix AND errM <= 25.0 }| / 1440      (trace_<arm>_seed<N>.csv)
    a deployment is met iff A_fix >= 0.95
    dD(c)  = D_T(c) - D_B(c)
    rho(c) = geometric mean over seeds of E_T / E_B, seed-level bootstrap 95 % CI, 10 000 resamples
    allow(c) = min(2.0, 1 + kappa * dD(c) / N),  kappa = 1.0,  band {0.25, 0.5, 1.0, 2.0} always reported
    T = ca_tbl_full,  B = static_2p4

`R = 25 m` is the design requirement fixed in `Architecture.md` §1.3 and is not adjustable
here.

## 2. What changes

### 2.1 The fourth branch

| outcome | condition |
|---|---|
| **INCONCLUSIVE — requirement unmet by any arm** | `D_T = 0` **and** `D_B = 0` |
| **PASS** | `dD > 0` **and** the CI for `rho` lies entirely below `allow(c)` |
| **FAIL** | `dD <= 0` (outside the row above), **or** the CI for `rho` lies entirely above `allow(c)` |
| **INCONCLUSIVE** | the CI straddles `allow(c)` |

The first row is evaluated first. It is deliberately **asymmetric** with the case
`D_T = D_B = N`: when both arms meet the requirement in every deployment the criterion *can*
decide, and what it decides is that the energy premium bought nothing — FAIL, as in A0. When
neither arm meets it anywhere, the criterion cannot decide anything and says so.

### 2.2 Energy, when an IMU is fitted

With `--imuPrior=true` every arm also pays `imuJ`, a constant reported in the run's
IMU summary file (imu_summary.csv, written only under that flag) and **not** in `totalJ` (the IMU is not a tier; see
`scratch/ca_tbl_causal_sim.cc`, `ArmResult::imuJ`).

- **Primary:** `E = totalJ` (radio energy), exactly A0's definition. A0 v2 verdicts are
  comparable across IMU-on and IMU-off cells only on this axis.
- **Secondary, reported beside it:** `E = totalJ + imuJ` (system energy).

**The secondary ratio is compressed by construction, and that is stated now rather than
discovered.** Adding the same constant to numerator and denominator pulls any ratio toward
1. At the nominal 11.52 J per arm-hour against radio energies of ~8–27 J, the compression is
large enough to move `rho` across `allow(c)` on its own. A secondary PASS that is a primary
FAIL is a statement about carrying an IMU, not about tiering, and must be reported as such.

### 2.3 Availability, when an IMU is fitted

- **Primary:** `A_fix`, unchanged. An IMU cannot create a radio fix, so under this
  definition its contribution is only indirect — through the accuracy of fix cycles and
  through any change in escalation behaviour.
- **Secondary, new, labelled as new:** `A_est = |{ cycles : errM <= 25.0 }| / 1440`, every
  cycle scored whether or not it had a fix. This is where a dead-reckoning prior's
  contribution lives. It is reported beside `A_fix`, with its own `D_T`, `D_B` and verdict,
  and it never replaces it.

Both are truth-scored: neither says the system *knew* its error was under 25 m. Neither is a
protection level.

## 3. Cells this will be applied to

The four cells of `report/PREREG_IMU_PRIOR.md` (two configurations × IMU off/on, seeds
301–330, `N = 30`). That registration is committed after this one and before its first
cell.

## 4. Disclosure — what is already known

**`C_ref` (`boxHalf = 2000`, propagation, evidence gate, PCS `min`) is not blind.**
`src/catbl-propagation-study/experiments/JOINT_CRITERION_RESULT.md` measured this
configuration at 52 seeds: both arms met the requirement on **52/52**, at **2.17×** radio
energy. That is `dD = 0` with `D_T = D_B = N` — a genuine tie, not the degenerate branch — so
the IMU-off `C_ref` cell is **predicted FAIL** from published data, and the IMU-on `C_ref`
cell is predicted FAIL on the primary axis unless the IMU changes `A_fix` or escalation
materially. Those two verdicts confirm a known result; they are not tests.

**`C_sparse` (`boxHalf = 3000`, otherwise identical) is partly known, from one control seed.**
An IMU-off run at seed 3 (outside 301–330, produced for the null control of the IMU commit)
gave `ca_tbl_full` `A_fix = 0.974` (meets), `static_2p4` `A_fix = 0.857` (does not), radio
energy 21.69 J against 8.02 J (2.7×). If seed 3 is typical, `dD/N` is near 1, `allow` near
2.0, and the primary verdict is FAIL on energy, while the secondary system-energy ratio would
be ≈ (21.7 + 11.5)/(8.0 + 11.5) ≈ 1.70 — **under** the allowance. That is exactly the §2.2
compression, visible before any campaign cell, which is why the primary axis is radio
energy.

## 5. What this cannot establish

Everything A0 §7 lists, plus: `A_est` is new in this document and has no history in the
project to calibrate against, and both availability definitions remain truth-scored rather
than self-assessed.
