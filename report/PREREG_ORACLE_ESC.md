# PREREG ORACLE-ESC — the upper bound on a learned confidence input to the escalation trigger

Registered 2026-10-21, before any campaign cell. The runner (`scripts/oracle_esc_run.sh`) and the analysis
(`scripts/oracle_esc_analyse.py`) are committed with this file and are not edited after the first cell.

## 1. Question

The escalation rule reads one number per fix:
- fixed_threshold reads the position variance trace(P);
- CA-TBL reads PCS, a monotone function of trace(P) on the S2-C preset (S2C_RESULTS; not re-checked on the current
  preset).

Adding ML to the decision path would replace that number with a learned estimate of the position error. A learned
estimate cannot be better than the truth. So the experiment gives the trigger the **true** squared position error,
the quantity trace(P) estimates, and measures how far fixed_threshold's energy–accuracy curve moves.

- If it does not move, no learned confidence input can move it.
- If it moves, that is an upper bound, not a result for any learned model.

## 2. Mechanism

`--oracleTrigger` (default false; byte-identical when false, section 6):
- **What it reads.** fixed_threshold's escalation test `trace(P) > T` and its de-escalation test
  `trace(P) < T/2` read `(x̂ − x)² + (ŷ − y)²` in place of trace(P). This is the post-update estimate against the
  node's true position on that cycle.
- **What stays the same.** The threshold `T` (`--fixedVarThreshold`), the hysteresis, `nDeescalate` and every
  other path (no-fix demotion, query hook, UWB grant) are unchanged.
- **Which nodes.** Tag 0 and the measured subjects.
- **What is not affected.** Other arms. Relay peers that are not measured subjects.

## 3. Design

| | |
|---|---|
| binary | built from the commit that adds this file; copied with a SHA-256 manifest and verified before every cell |
| common flags | `--specProfile=true --duration=1800 --trace=true --traceCovariance=true --arms=fixed_threshold` |
| geometries | **A**: 5 km (the preset's default box); **B**: endgame, `--boxHalf=750 --finderMode=1` |
| triggers | **K**: trace(P) (flag absent); **O**: `--oracleTrigger=true` |
| thresholds T (m²) | 3, 12, 50, 200 (`--fixedVarThreshold=T`; 12 is the preset's) |
| seeds | 321–340 (fresh; the highest registered before this is 320) |
| cells | 2 geometries × 2 triggers × 4 T × 20 seeds = 320, plus 2 preset-check cells (section 5) |

Why these thresholds: the radial σ values are 1.7, 3.5, 7.1 and 14.1 m. A probe run (seed 999, preset, outside
the seed range) put tag 0's fix-cycle quantiles as follows:

| geometry | quantity | 5 % (m²) | 50 % (m²) | 95 % (m²) |
|---|---|---|---|---|
| A | trace(P) | 11 | 39 | 867 |
| A | true squared error | 22 | 204 | 951 |
| B | trace(P) | 0.6 | 1.2 | 4.2 |
| B | true squared error | 0.2 | 6.8 | 39 |

The four values therefore bind on both quantities in both geometries.

## 4. Metrics and statistics

The metrics come from `scripts/goal_report.py` `cell()`:
- **primary:** five-node A(25 m), the share of all decision cycles with the estimate within 25 m, pooled over
  tag 0 and the subjects;
- **energy:** five-node mean J (`subject_summary.csv` `totalJ`, all five nodes);
- **secondary:** tag-0 A(25 m) and tag-0 CEP95.

Each comparison is paired over seeds:
- bootstrap with 10 000 resamples and one fixed resample matrix per geometry (NumPy seed 20261021);
- **99 %** percentile intervals, because each geometry has 16 oracle-versus-trace pairs;
- every verdict is a plain bool read from the interval its row prints (RULES.md rule 18 on the rules branch).

For every pair (O at T_o, K at T_k), dA = five-node A(25 m) of O minus that of K, and dJ = five-node J of O minus
that of K.

**Accuracy headroom (per geometry):**
- **Qualifying pair:** mean dJ ≤ 0 (the oracle uses no more energy) and the dA interval's lower bound > 0.
- **H:** the largest such lower bound over the qualifying pairs.
- **Verdict:** **MATERIAL** if H ≥ 0.02; **SMALL** if 0 < H < 0.02; **NONE** if no pair qualifies.

**Energy headroom (per geometry):**
- **Qualifying pair:** the dA interval's lower bound ≥ −0.02 (no worse than 2 points) and the dJ interval's upper
  bound < 0.
- **E:** the largest such saving, measured as −(upper bound of dJ) / mean J of K, over the qualifying pairs.
- **Verdict:** **MATERIAL** if E ≥ 0.10; **SMALL** if 0 < E < 0.10; **NONE** if no pair qualifies.

**Registered reading:**
- NONE on both headrooms in both geometries means that a learned confidence input cannot improve this trigger in
  this simulator.
- Any MATERIAL means that a learned input has room. The next step would then be a learned error predictor on
  non-truth features, trained and evaluated on disjoint seeds against the K curve.
- SMALL is reported as measured and decides nothing.

## 5. Checks and abort conditions

The analysis exits 1, and no verdict is read, if any of these holds:
1. a cell's `rc` is not 0;
2. a cell is missing, or a configuration has other than 20 seeds;
3. `run_id.txt` is not `dirty 0`, or more than one source appears;
4. an O cell lacks the `ORACLE TRIGGER` line, or a K cell has it;
5. an O cell reports `tag 0 0` or `subjects 0` oracle decisions, so the path was not exercised (RULES.md rule 4);
6. **preset check:** `P_A` and `P_B` (seed 321, the same flags without `--fixedVarThreshold`) are not
   byte-identical to `K_T12` seed 321 in every fixed_threshold file except `run_id.txt`. An explicit 12 must be the
   preset's 12.

**Precondition, before the campaign:** the null control in section 6 passes.

## 6. Null control (precondition)

With the flag absent, the new binary must be byte-identical to the binary of the parent commit `a2e738cd` in every
output file but `run_id.txt`:
- seed 7, 600 s, all arms;
- in three settings: the preset at 5 km, the preset in the endgame, and code defaults.

With `--oracleTrigger=true`, every non-fixed_threshold file must also be identical. The result is recorded in the
results file.

## 7. Predictions (written before any cell)

- **P1.** At every T, O escalates more often than K in both geometries. The filter is overconfident (section 3:
  median trace(P) is below the median squared error by a factor of 5 in A and 6 in B), so the true error exceeds T
  more often.
- **P2.** In B, accuracy headroom is NONE or SMALL: five-node A(25 m) is already near its ceiling there.
- **P3.** In A, accuracy headroom is NONE or SMALL. Escalating cannot help a node whose 2.4 GHz anchors are out
  of reach (about 156 m), however well its error is known.
- **P4.** No directional prediction for energy headroom.

## 8. Limitations

- **This bounds a learned input, not every learned policy.** It bounds a learned replacement for the number the
  threshold rule reads. It does not bound:
  - a learned policy that predicts the benefit of escalating (for example, whether the next tier is reachable);
  - a learned estimator that changes the position itself, such as an NLOS bias correction.
- **An upper bound is all a positive result gives.** The oracle uses the truth at decision time, so a learned model
  can only do worse. NONE is conclusive for this input; MATERIAL is only an upper bound.
- **Only the simulator is tested.** Its 2.4 GHz error floor is 3 m and its NLOS removes links rather than biasing
  ranges. Field behaviour may differ.
- **Only fixed_threshold is tested.** CA-TBL's rule is covered only through the S2-C finding that its PCS is a
  monotone function of trace(P) on the S2-C preset.

# RESULT — after the registration above (`1136c314`)

## Run

| time (UTC) | event | cells |
|---|---|---|
| — | first launch, from `1136c314` | aborted after 23 |
| 20:01:00 | relaunch, from `e8195863`, dirty 0 | — |
| — | disk full (`ENOSPC`) | stopped at 64: 60 finished, 4 partial |
| 20:12:56 | resume, same source, same verified binary | — |
| 20:53:04 | done | **322 of 322**, every `rc` 0 |

- **First launch aborted.** Commits made during it moved the repository HEAD to `e8195863`, so its cells carried two
  source lines (20 and 3). Section 5 check 3 would have failed. It was not analysed, and its cells were deleted (its
  `RUN.md` is kept outside the repository).
- **Final run.** One source, every section 5 check passed, scored once. Output:
  `results/oracle_esc_2026-10-21/ORACLE_ESC.md`.

**Housekeeping, disclosed.** The disk allowance could not hold 322 full cells (17–24 MB each).
- After each cell finished, `results/oracle_esc_2026-10-21/prune_cells.py` did two things:
  1. verified every file against the SHA-256 digest the simulator writes into the cell's `run_id.txt`;
  2. deleted the files the analysis never reads (event, entry and NLOS link logs).
- Every cell's `run_id.txt` still records the digest of every file the run wrote.
- The four preset-check cells were kept whole.
- All 322 cells verified (`prune.log`).

**Binary provenance, corrected.** Section 3 says the binary was built from the commit that adds this file. It was
compiled from the working tree before `bd15470a` was committed:
- its stamp reads `build source a2e738cd dirty 3`;
- the three dirty files are exactly `bd15470a`'s diff;
- `src/` and `scratch/` are unchanged from `bd15470a` to `e8195863`;
- the copied binary's SHA-256 is in `build.sha256`.

## Verdicts (as registered)

| geometry | accuracy headroom | energy headroom | pair that decides it |
|---|---|---|---|
| A (5 km) | **NONE** | **NONE** | no pair qualifies |
| B (endgame) | **SMALL** (H = +0.000) | **MATERIAL** (E = 0.131) | accuracy: O T200 − K T200, +0.022 [+0.000, +0.059]; energy: O T200 − K T3, dJ −4.05 J [−5.51, −2.67], dA +0.015 [−0.008, +0.072] |

The registered reading is "a learned confidence input has room": **True**.

## The rule was too loose (found after scoring; disclosed, the script is not edited)

**What went wrong.** Section 4 lets an oracle point qualify against **any** trace setting, including ones another
trace setting beats on both axes. Both endgame qualifiers use such dominated settings:
- **K T3** (0.962, 20.30 J) is beaten by K T12 (0.977, 16.81 J);
- **K T200** (0.955, 16.70 J) is beaten by K T50 (0.975, 16.49 J).

**Post hoc.** The non-dominated trace settings are T12, T50 and T200 at 5 km, and T12 and T50 in the endgame. Against
them **no oracle pair qualifies, on accuracy or on energy**. These use the same resample matrices; endgame examples:

| pair | dA | dJ (J) |
|---|---|---|
| O12 − K12 | −0.002 [−0.006, +0.002] | +0.91 [−0.18, +2.13] |
| O50 − K50 | −0.004 [−0.011, +0.002] | +0.25 [−0.75, +1.26] |

The registered MATERIAL is what retuning T alone already gives: K T3 → K T50 saves 3.81 J.

**Reading.** Perfect knowledge of the position error does not move fixed_threshold's energy–accuracy frontier:
- at 5 km, every same-T accuracy interval includes 0;
- in the endgame, tag 0 is at 0.997 under every configuration.

So a learned confidence input to this trigger has nothing to gain in this simulator.

## Predictions

| prediction | outcome | evidence |
|---|---|---|
| P1 | **Fails** | holds only at 5 km, T3: +21.1 [+11.5, +31.3] escalations; the other seven intervals include 0 or fall below it |
| P2 | Holds | endgame accuracy headroom SMALL |
| P3 | Holds | 5 km accuracy headroom NONE |

## Not established

- **This bound does not cover every learned policy.** It says nothing about learned policies that predict whether
  escalating would pay off, or about learned estimators that change the position.
- **Field behaviour is untested** (section 8).
