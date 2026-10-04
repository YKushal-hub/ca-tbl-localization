# PREREG TDOA-01 — is tier-1 TDoA's reported covariance calibrated, and how often does the exactly-3 closed form refuse?

**Registered before any cell runs** (rule 13). Written 2026-10-04. Engine source at
registration: `58b12eb`, with the preset as it stands there (86 settings).
This commit adds this file, `scripts/tdoa01_run.sh` (the runner) and
`scripts/tdoa01_analyse.py` (the analysis), and nothing else. The outputs go to
results/tdoa01_2026-10-04/, which does not exist at registration.

**What asks for it.** `docs/SPEC_CONFORMANCE_PLAN.md` PH-1 step 2: "register a TDoA σ
calibration: reported covariance against realised error on TDoA-fused cycles, the A4
method. Report the AMBIGUOUS/NO_ROOT refusal rate at 3 gateways." It was meant to run
inside CAMP-SPEC and did not. TECH_DEBT J.2 item 4 is the open question this measures: the
TDoA σ comes from a Cramér-Rao bound with no threshold effect, and nothing has checked it
against error.

**Disclosed before registration.** One unregistered run was inspected while scoping this:
G24's preset run (5 km, seed 7, 900 s), tag 0's 124 OK solves. Over them, the normalised
estimation error squared (NEES) had median 1.30, mean 1.87, and 0.968 of solves fell inside
their own 95 % ellipse. The predictions below were written after seeing that.

The analysis was then exercised, after the predictions were drafted and without changing
them, on a throwaway grid outside results/: seeds 1-2, 300 s, all three configurations.
- In it, p8's C95 was 0.928.
- p3 had only 12 OK solves, all with exactly three stamps, and no AMBIGUOUS or NO_ROOT
  among its 12 three-stamp attempts. So P3 may well fail.
- That grid is not data for this registration.

---

## 1. Cells (`tdoa01_run.sh`)

- The preset (`--specProfile=true`, which sets `--tier1Tdoa`), arms `ca_tbl_full` and
  `static_868`, 1 800 s, seeds 1-20. Three configurations, 60 runs:
  - `p8`: the preset as it is (eight gateways on a ring, 5 km);
  - `p3`: the preset with `--nGateways=3`, the spec's three gateways at 5 km (a ring at
    0.6 of the half-width);
  - `end`: the preset's endgame (`--boxHalf=750 --finderMode=1`).
- TDoA runs for tag 0 only (TECH_DEBT N.3), so each run gives two `tdoa_*.csv`, one per arm.

## 2. Metrics (from `tdoa_*.csv`)

- **Attempts by verdict:** OK, TOO_FEW, COLLINEAR, SINGULAR, NO_CONVERGE, BAD_INPUT, NO_ROOT,
  AMBIGUOUS.
- **Calibration, over OK solves:** e = (tdoaX − trueX, tdoaY − trueY) and R = the reported
  (rxx, rxy, ryy). NEES = eᵀR⁻¹e, which is χ²₂ if R is right.
  - Reported: the median and mean NEES, and the coverage C95 = the fraction with
    NEES ≤ 5.991 (χ²₂ at 0.95).
  - Per configuration, pooled over arms and seeds. C95 gets a 95 % seed bootstrap
    (10 000 resamples, generator seed 20261006; the seed is the resampled unit).
  - Split by stamps (exactly 3: the closed form; 4 or more: Chan-Ho/Foy).
- **The exactly-3 refusal rate:** (AMBIGUOUS + NO_ROOT) / attempts with exactly three stamps,
  per configuration.

## 3. Predictions, made before any cell

- **P1.** In `p8`, the reported covariance is calibrated: the C95 interval lies inside
  [0.90, 0.99].
- **P2.** In `p3`, at least 80 % of OK solves have exactly three stamps: the closed form
  carries the spec's geometry.
- **P3.** In `p3`, AMBIGUOUS + NO_ROOT is at least 10 % of the exactly-3 attempts.
- **P4.** Pooled over all configurations, C95 of exactly-3 solves is lower than C95 of 4+
  solves. The closed form's two roots, decided by RSSI ranges, should be the less reliable.

Each is written up as held or failed in report/TDOA01_RESULTS.md.

## 4. Abort conditions

- A run exits non-zero: stop, fix the cause, never drop the cell.
- A configuration has fewer than 100 OK solves in all: its calibration is reported as
  under-populated, not as a result.
- results/tdoa01_2026-10-04/ exists before the runner starts: it refuses.

If the grid is interrupted, the cells without an `rc` file are re-run with the same command
line (`tdoa01_run.sh --resume`); that is recorded in the results' `RUN.md`.

## 5. What follows

Nothing is adopted. This is a measurement.
- If P1 holds, J.2 item 4's open question is answered for the preset's geometry. The CRLB σ
  is calibrated there, and that is recorded in TECH_DEBT J.2.
- If C95 falls below 0.90 in any configuration, the TDoA σ is overconfident there, and
  inflating it becomes a flagged item of its own, not done here.
- The default (`--tier1Tdoa` off) is the owner's J.2 decision, and is not touched.
