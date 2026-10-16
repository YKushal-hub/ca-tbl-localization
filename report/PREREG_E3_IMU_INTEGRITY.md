# Pre-registration — E3: CAL-05 with the inertial prior

Written 2026-10-14, while the primary IMU campaign (`report/PREREG_IMU_PRIOR.md`) was
running and **before any of its results were read**. Code under test: `9f5085a`
(`--imuPrior`). Committed before E3's first cell. Runner
`scripts/sweeps/run_e3_imu_integrity.sh`, scorer `scripts/e3_imu_integrity_analysis.py`,
committed with this file and not edited after the first cell.

## 1. Question

`report/CAL05_DETECTION_CURVE.md` measured the Switching-Point Integrity Check as a
detector and named its own limit: *"The prior is not the specified one. The check tests
against a constant-velocity prior propagated from previous radio fixes, not the independent
prior ch10 §10.7.2 requires."* DEC-D was left open on exactly that. This asks: **with the
inertial prior, does the check detect injected range bias differently?**

This is the integrity check (`ca-tbl-policy.cc`, Mahalanobis against χ²₀.₉₉ = 9.21), not the
evidence gate. Different mechanisms; kept distinct throughout.

## 2. Design

CAL-05's grid exactly, twice:

| | |
|---|---|
| flags | `--duration=3600 --boxHalf=2000 --gwGrid=5 --escalationRequiresEvidence=true --nlosSource=propagation --pcsForm=v2 --mahalUseR=true --injectBiasFraction=0.2 --suspectRecovery=bounded --suspectForcesEscalation=true --arms=ca_tbl_full,ca_tbl_no_integrity --trace=true` |
| bias | `--injectRangeBias` ∈ {10, 20, 40, 80, 160} m |
| fault | F1 = `--injectFaultAnchors=1`, F3 = `--injectFaultAnchors=0` — reported separately, never pooled |
| seeds | 1–20, **the seeds CAL-05 used**, so each cell is paired with its CAL-05 cell |
| conditions | `cv` (no IMU flag) and `imu` (`--imuPrior=true`, defaults) |

400 invocations. The `cv` half is a **regeneration of CAL-05 under current code**, which is
the second purpose of this campaign (R1 below).

## 3. Denominators, verbatim from CAL-05

The fault window is contiguous and seed-derived:
`biasStartS = 3600 × (0.25 + 0.5 × (seed mod 7)/7)`, injected iff
`biasStartS ≤ t < biasStartS + 720`. 288 injected cycles per seed, never truncated by the
end of the run (checked per cell, not assumed).

Per seed, on `ca_tbl_full`: **TP** suspect ∧ injected; **FP** suspect ∧ ¬injected; **FN**
¬suspect ∧ injected ∧ hadFix. **Recall** = TP/(TP+FN), over injected cycles where the check
could fire (a cycle without a fix has nothing to test). **False-alarm rate** = FP over
non-injected cycles with a fix. Precision = TP/(TP+FP) is reported as CAL-05 did.

## 4. Predictions

**R1 (reproduction, exact).** Every `cv` cell reproduces its CAL-05 sidecar digest —
`arm_summary.csv` and both traces — byte-for-byte: 200 cells, 600 files.
*Basis:* CAL-05's cells ran at three commits — `results/c9_stage2` at `9723dfc` (149 cells)
and `8fddef9` (11, the mixed-SHA incident recorded on 2026-09-05), the 40 m cells in
`results/c9_escalate` at `6d44895` — all `dirty 0`. Every engine change since is flag-gated, and
the IMU commit's null control showed the default path unchanged at four flag sets — but
**not at this one** (`pcsForm=v2`, `bounded`). *Falsifier:* any differing file. A failure
does **not** abort E3 — both of its conditions come from the same current code, so the
paired comparison stands — but it is a finding about what the merge or a later commit did to
this path, and it is reported first.

**E1 (direction).** Recall rises with the inertial prior. Per (bias, fault), the mean
per-seed recall difference `imu − cv` is **> 0 at 20, 40 and 80 m for both F1 and F3**
(6 cells), with the bootstrap CI excluding 0 in **at least 4 of the 6**.
*Mechanism:* the test statistic is `d^T S^-1 d` with `S = P_prior + R_fix`. Constant
velocity adds 1 m² per axis per second of process noise — 2.5 m² per cycle — to `P_prior`;
the inertial prior adds metres-squared-per-hour. A tighter `S` puts the same bias further out
in the χ² tail. Not predicted at 10 m (below CAL-05's floor, where no prior separates a bias
from ranging noise) or 160 m (recall 0.83–0.91 already).
*Falsifier:* any of the six point estimates ≤ 0, or fewer than four CIs excluding 0.

**Not predicted, reported two-sided: the false-alarm rate.** A tighter prior is also a
prior more easily contradicted by an *unmodelled* error — NLOS excess delay is not in the
range model's mean — so the false-alarm rate may rise. If recall and false alarms rise
together, the inertial prior has moved the operating point along the curve rather than
improved the detector, and that is what will be reported.

**Descriptive:** the 50 % recall crossing per fault and condition, interpolated as CAL-05
did (≈ 58 m F1, ≈ 33 m F3 under `cv`).

## 5. Abort conditions

- Any cell failing completeness (digest, `dirty 0`, two summary rows counted, two traces) is
  named and excluded; more than 10 % excluded aborts.
- No seed extension, no re-run to replace a cell, no scorer edit after the first cell.

## 6. Disclosure

The CAL-05 `cv` numbers are published (`report/CAL05_DETECTION_CURVE.md`) and were read; the
`cv` half is not blind and is not meant to be. **No E3 `imu` cell exists before this commit,
and no result of the primary IMU campaign had been read when this was written.**

**Timing, stated exactly.** This file — predictions included — was written while the primary
IMU campaign ran. It is **committed after** that campaign was scored (`89ac3b6`), because no
commit could be made while its cells were running without giving them mixed source SHAs. The
only text added after reading is this paragraph. One primary result bears on E3 and is named
here rather than discovered later: the IMU filter is **overconfident where fixes are sparse**
(position NEES 5.0 at `boxHalf = 2000` and 144 at 3000 on `ca_tbl_full`, against 2.0). That
strengthens the registered two-sided concern about the false-alarm rate; no prediction was
changed in response.

## 7. What this cannot establish

- That the check is now independent. With the inertial prior its *increment* is independent
  of the radio; its *origin* is still the last accepted fix, so a fault that has already been
  accepted contaminates the prior exactly as before.
- Anything about faults other than a constant range bias in a contiguous window.
- Anything about policies other than `bounded`, or window fractions other than 0.2.
