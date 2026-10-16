# Pre-registration — D-8: tier 2 on its own bandwidth's sensitivity row, stage 1

Written 2026-10-15. Code under test: `--t2SensitivityRowKHz`, introduced at `395cd64`
(its flag-off null control is in that commit); the cells run at the queue's single commit,
recorded in every sidecar. Committed
before the first cell; runner `scripts/sweeps/run_d8_sensitivity.sh`, scorer
`scripts/d8_analysis.py`, committed with this file and not edited after the first cell. The
owner chose a planned fix campaign over a patch (2026-10-15): nothing existing is invalidated
until the corrected numbers exist.

## 0. The defect

Gate G19 (A7) calls the 2.4 GHz engine's own threshold accessor: at SF7 tier 2 applies
**−106.0 dBm**, the SX1280's **1625 kHz** row (Data Sheet rev 3.2, Table 6-1, low power mode),
while the scenario runs tier 2 at **800 kHz**, whose row gives **−112.0 dBm**. Tier 2's
receiver is ~6 dB pessimistic at its own bandwidth. `--t2SensitivityRowKHz=812.5` applies the
812.5 kHz row; the scenario refuses a row that does not match the configured bandwidth. With the
fix, G19 passes; the flag-off null control is in the D-8 commit.

## 1. Design, stage 1

| | |
|---|---|
| probe | `--probe=true`, default flags, fix off and on — tier r80 from `probe_range_sweep.csv` (PDR falling through 0.80, interpolated, as `scripts/make_figures.py` computes it) |
| campaign | `C_ref` `--boxHalf=2000` and `C_sparse` `--boxHalf=3000`, both with `--duration=3600 --gwGrid=5 --nlosSource=propagation --escalationRequiresEvidence=true --pcsForm=min --trace=true`, all six arms |
| treatment | `--t2SensitivityRowKHz=812.5` against default, paired by seed |
| seeds | **701–720**, N = 20, disjoint from every registered campaign |

## 2. Predictions

**P1 (magnitude).** Tier-2 r80 under the fix is **1.4× to 2.1×** its default value
(1149.5 m at `probe_v7`). *Basis:* 6 dB of link budget scales range by 10^(6/10n); for an
effective path-loss exponent between 2 (free space) and 3.5 that is 1.48× to 2.0×, widened
slightly for interpolation between sweep points. *Falsifier:* outside the band.

**P2 (control).** Tier-1 and tier-3 r80 move by less than 1 % — the fix touches tier 2's table
only. *Falsifier:* either moves by 1 % or more.

**P3 (direction).** At `C_sparse`, `static_2p4`'s `A_fix` rises (paired difference CI entirely
above 0) — a more sensitive tier-2 receiver covers more of the gaps between sites.
*Falsifier:* CI reaches or falls below 0.

**Reported, not predicted:**
- **A0 v2 at both configurations, fix on** — the question this stage exists for. Under the
  default all four primary verdicts were FAIL (`report/IMU_PRIOR_RESULT.md`, `C_ref` and
  `C_sparse`, IMU off and on).
- Every arm's `A_fix`, `cep50M`, radio energy and escalations, paired.
- **Static tier-1 and tier-3 arms: whether their rows are byte-identical.** Not asserted:
  those arms never range on tier 2, but tier-2 beacon reception at the node draws from the
  shared MAC stream (`macRng`), so a change in tier-2 sensitivity can shift every arm's MAC
  draws (working rule 8). Identity, if it holds, is reported; a difference is reported with that
  mechanism named and is not an abort.

## 3. Stage 2 — registered now, run only after stage 1 is scored

Re-measure the surviving headline results under the fix, on their own registrations' flags and
fresh seeds: Claim 2's energy ratio (6.53×), Claim 4's coverage-floor saving (24.5 %), and the
joint criterion. Their registrations and seeds are written after stage 1, because stage 1 says
which of them can move materially; that ordering is fixed here so it cannot be chosen after.

## 4. Abort conditions

Completeness per cell (digest, `dirty 0`, six rows, six traces); > 10 % excluded aborts. The
probe must reproduce `probe_v7`'s three r80 values (1515.3 / 1149.5 / 12.8 m) to 0.1 m with the
fix **off**, or the comparison has no baseline and stage 1 stops. No seed extension, no re-run
to replace a cell, no scorer edit after the first cell.

## 5. Disclosure

Before this commit: the probe's threshold under the fix (−112.0 dBm, A7 passes) and the
scenario's refusal of mismatched rows were checked; no scenario run with the fix on exists.

## 6. What stage 1 cannot establish

Whether the published claims survive — that is stage 2. Anything about tier 3's sensitivity,
which remains unchecked against a data sheet.
