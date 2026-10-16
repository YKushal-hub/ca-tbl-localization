# Pre-registration — A1: precision-tier coverage fraction against dwell, scored by A0 v2

Written 2026-10-14, before any cell. Committed before the first cell; runner
`scripts/sweeps/run_a1_dwell.sh`, scorer `scripts/a1_dwell_analysis.py`, both committed with
this file and not edited after the first cell.

## 0. What changed since A1 was specified

`docs/WORK_PACKAGES.md` §A1 designed this sweep to reframe the conjunction claim (Claim 3)
from "fails at scale" to "needs a coverage fraction above a threshold". Claim 3 has since been
withdrawn twice (W4, W11), the second time because its only PASS was an artefact of the legacy
NLOS source. Running A1 under the flags it was written for would measure the withdrawn
artefact. It is therefore re-specified on the configuration the current evidence uses, and
asks two questions that are open now:

1. **Mechanism.** Does precision-tier coverage fraction follow `dwell / (dwell + T)`?
2. **Verdict.** Does A0 v2 return PASS for `ca_tbl_full` against `static_2p4` at any dwell —
   i.e. does raising the time spent where tier 3 can serve make the tiered architecture pay at
   `R = 25 m`? `JOINT_CRITERION_RESULT.md` ends by naming the tighter operating point as the
   obvious next experiment; dwell is the cheapest axis that moves it, needing no code.

## 1. Design

| | |
|---|---|
| flags | `--duration=3600 --boxHalf=3000 --gwGrid=5 --nlosSource=propagation --escalationRequiresEvidence=true --pcsForm=min --trace=true --arms=ca_tbl_full,static_2p4,fixed_threshold,static_868` |
| axis | `--dwell` ∈ {90, 150, 245, 400, 800} s. 90 is the default; 245 is the work package's derived value; 800 is the dwell §4.1's original reliability claim was stated at |
| seeds | **501–520**, N = 20. Disjoint from every registered campaign and from the IMU work (301–330, 401–402) |
| IMU | off — A1 measures the radio architecture; the inertial prior has its own campaign |

## 2. Quantities

**Coverage fraction** `f(seed, dwell) = cyclesInUwbCoverage / steps` from `arm_summary.csv`:
the geometric label (≥ 3 tier-3 anchors in nominal range), a property of the trajectory, so
identical across arms of a seed by construction (the Tourist's stream is exclusive). Its
denominator is all 1440 cycles; no cycle is truncated out of it.

**Model.** `f = d / (d + T)`, with `d` the mean dwell (`--dwell`; the walker draws
`dwell × U(0.6, 1.4)`, mean `dwell`) and `T` a single transit-time parameter. **T is fitted on
the 90 s cell alone** — `T = 90 × (1 − f̄₉₀) / f̄₉₀` — and the model then **predicts** the other
four cells.

**A0 v2** exactly as `report/PREREG_A0_V2.md`: primary `A_fix` and radio energy, the fourth
branch, κ = 1 with the {0.25, 0.5, 1, 2} band.

## 3. Predictions

**P1 (mechanism).** At each of 150, 245, 400 and 800 s, the predicted `f` lies inside the
seed-bootstrap 95 % CI of the measured mean `f`, or within 1 percentage point of it.
*Falsifier:* any of the four outside both. Then coverage fraction is not a dwell/transit duty
cycle, and the work package's mechanism is wrong — which is the result.

**P2 (monotone).** Measured mean `f` increases strictly with dwell. *Falsifier:* any inversion.

**Verdict — not predicted.** Whether A0 v2 returns PASS anywhere is the question, and no
direction is registered. What *is* registered is how to read it: a PASS at long dwell and FAIL
at short dwell says the architecture pays only where the node spends a large fraction of its
time inside precision-tier coverage — a deployment requirement, and the size of it is the
crossing dwell.

## 4. Abort conditions

Completeness (digest, `dirty 0`, four arm rows counted, four traces) per cell; > 10 % excluded
aborts. No seed extension; no re-run to replace a cell; no scorer edit after the first cell.

## 5. Disclosure

Nothing at `boxHalf = 3000` with these flags and these seeds has been run. The only
`boxHalf = 3000` data read under the current flags is the IMU null-control reference at seed 3
(dwell 90): `cyclesInUwbCoverage` was not looked at.

## 6. What this cannot establish

Dwell and facility count both raise coverage fraction and are not the same intervention
(`WORK_PACKAGES.md` §A1 caveat): if the verdict moves with dwell, that says nothing yet about
adding sites. Anything at another `R`.
