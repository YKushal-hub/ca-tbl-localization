# CA-TBL — Independent Verification Audit Report

Run 2026-08-01 at HEAD `c9ab733b5`. Auditor had a shell; all sim runs via
`runuser -u kushal`. Audit outputs in `audit/` (not committed).

## Summary

```
REPRODUCED: 42    TRACED: 0    UNTRACED: 1    CONTRADICTED: 3
Checks run: 36    Checks blocked: 3
```

Every headline statistic reproduced. The three remaining contradictions are two
probe range figures (0.4% and 5% off, both hedged with "≈" in the source
document) and one under-specified statistic. **This audit's first pass recorded
two further contradictions that were themselves wrong** — see Contradictions.

The most consequential finding is B3, a gate whose pass message asserts a bound
it never tests, found only because the probe output was captured to a file for
the first time.

## Blocking findings

**None that invalidate a headline claim.** B1 and B2 are both CLOSED. One NEW
finding (B3) was opened by the probe capture and is not closed.

**B3. The CRLB gate is one-sided and its pass message asserts a bound it never
tests.** `ca_tbl_causal_sim.cc:1471` tests only `worstCrlbRatio < 0.333` — the
physically-impossible case where measured precision beats the Cramer-Rao floor by
3x. Every other value falls through to an `else` printing *"CRLB OK (within 3x of
the single-shot bound)"*. **The measured tier-1 ratio is 3.319**, so the gate is
currently printing a statement that is arithmetically false, and it would print
the same at a ratio of 1000. There is no upper bound in the gate at all.
Being worse than the CRLB is expected of a real system, so this is not
necessarily a physics defect — but v3 section 3.8 quotes "within 3x" as though a
two-sided check had passed, and no such check exists. Evidence:
`results/probe_audit/probe_stdout.txt`, `ca_tbl_causal_sim.cc:1462-1490`.

**B1. The gw3-vs-gw9 comparison spans two binaries.** The binary is dated
`2026-08-01 02:20:43`. Every results directory through `ms_c3_fair` (`01:46`)
predates it; `gw9_b3000` onward postdates it. The headline finding — that claim
2's parity at boxHalf 3000 was a tier-1 redundancy artefact — compares gw3 data
built at `3bbb30df6` against gw9 data built at `c9ab733b5`, with `--mahalUseR`,
`--fastDemote`, `--multiStepDemote` and `--gwGrid` added in between.
**CLOSED ANALYTICALLY.** Regenerating seeds samples the question; reading the diff
settles it. `git diff 3bbb30df6..HEAD` over `ca-tbl-policy.{h,cc}` and
`ca_tbl_causal_sim.cc` contains only three kinds of change:
1. **Flag-gated, default-off** — `mahalUseR`, `fastDemoteMode`, `multiStepMode`
   leave `pcfg` at its constructor defaults when unset.
2. **Computed but consumed only under a flag** — `lastNonEmptyAtS` and
   `tierEvidenceMask` are written unconditionally, read only inside
   `if (m_config.multiStepDemote)`. No RNG draw, no control-flow effect.
3. **An algebraic identity** — the grid rewrite. At `gw=3`,
   `fx = -g + 2g*gx/(gw-1)` yields `-g, 0, +g` for `gx = 0,1,2`, matching the old
   `g*gx` for `gx = -1,0,1` in the same iteration order, so `id++` assigns
   identical anchor IDs.
No plumbing running regardless of flag state, no changed initialisation order, no
new RNG object, no moved call site. **The pre-build results are therefore valid by
construction, for all seeds and all geometries** — strictly stronger than the
20-seed sample. Corroborated by two independent byte-identity checks
(`accept_gw` vs `baseline_v3`; `repro_b3000_s7` vs `sweep20_b3000/seed_7`).

**B2. ~~G2–G4 have never been run at HEAD.~~ CLOSED.** At the time of the first
report, `results/det_a`/`det_b` were dated `01:05`/`01:11` against a `02:20`
build, so only G1 was confirmed at HEAD. The full gate script has since been run
at `c9ab733b5`: **G1 PASS, G2 PASS, G3 PASS, G4 PASS**, 72 checks passed, 14/15
warning gates clear, 90 gates evaluated. The totals are identical to the
pre-`gwGrid` run, which independently confirms the three-commit restructure
changed no gate outcome. Evidence: `audit/gates_at_head.log`.

## Findings by phase

### V1 Provenance

| check | verdict | evidence | note |
|---|---|---|---|
| V1.1 results vs binary mtime | CONTRADICTED | `stat`, `git log` | 17 of 22 dirs predate the binary — see B1 |
| V1.2 regenerate boxHalf 3000 s7 | REPRODUCED | `diff` vs `audit/repro_b3000_s7` | byte-identical at HEAD |
| V1.3 command lines recoverable | REPRODUCED | `scripts/sweeps/*.sh` | every batch dir has a script; ad-hoc dirs (`accept_*`, `cachetest`) recorded in commit messages only |

### V2 Determinism and RNG hygiene

| check | verdict | evidence | note |
|---|---|---|---|
| V2.1 determinism at defaults | REPRODUCED | G2 in `scripts/catbl_gates.sh` | covered by the HEAD gate run |
| V2.2 determinism at 6 non-default flags | REPRODUCED | `audit/detf_*` pairwise diff | `--mahalUseR=true`, `--fastDemote=all`, `--multiStepDemote=all`, `--gwGrid=9`, `--boxHalf=3000`, `--searchDuty=0.0` all byte-identical across repeat runs. **No flag introduces an unseeded draw.** |
| V2.3 wall-clock reaching a branch | REPRODUCED | `grep` | only comments reference `chrono`; no live use |
| V2.4 `ResetNextStreamIndex` in both entry points | REPRODUCED | `ca_tbl_causal_sim.cc:981, 1673` | present in `RunProbe` and `RunArm` |
| V2.4b `LoRaDistributionSampler` ignores its seed | REPRODUCED | `lora-distribution-sampler.cc:8-17` | **defect still live**: 5 RVs created, 0 `SetStream`. Documentation accurate. |
| V2.5 ablation clean at seed 7 / divergence at 3,14,17 | BLOCKED | — | not run |

### V3 Harness integrity — audited hardest

| check | verdict | evidence | note |
|---|---|---|---|
| V3.2 batch completeness, 13 dirs | REPRODUCED | `find` + `awk` | all 13 have exactly 20 files and 20 distinct seeds |
| V3.2b seed-set integrity | REPRODUCED | `awk` on seed column | no `{1,3,5,...}` pattern — the `xargs` failure mode did **not** recur |
| V3.3 arm completeness | REPRODUCED | `find` + row count | every `arm_summary.csv` has 6 arms (or 12 for the 2-seed cachetest) |
| V3.4 silent failures in logs | REPRODUCED | `grep -ril` | zero matches for error/abort/assert/terminate/segfault |
| V3.1 script grouping review | REPRODUCED | read all `scripts/sweeps/*.sh` | all post-incident scripts use uniform 4-field job lines with `xargs -n 4` |

### V7/V8 Statistical integrity and adversarial re-derivation

All recomputed with an independent implementation that does not import
`paired_stats.py` or `summarise.py`.

| number | documented | recomputed | verdict |
|---|---|---|---|
| claim 2, gw3 off | 1.0530 | 1.0530 [0.845, 1.313] | REPRODUCED |
| claim 2, gw3 fd=prop | 1.4826 | 1.4826 [1.082, 2.032] | REPRODUCED |
| claim 2, gw3 fd=all | 0.7034 | 0.7034 [0.585, 0.846] | REPRODUCED |
| claim 2, gw9 off | 0.7690 | 0.7690 [0.706, 0.837] | REPRODUCED |
| claim 2, gw9 fd=prop | 1.6487 | 1.6487 [1.477, 1.840] | REPRODUCED |
| claim 2, gw9 fd=all | 0.6963 | 0.6963 [0.618, 0.784] | REPRODUCED |
| claim 1, box 1000 | 1/62.6× | 1/62.6× | REPRODUCED |
| claim 1, box 3000 | 1/136× | 1/136.0× | REPRODUCED |
| claim 1, gw9 | 1/95× | 1/95.3× | REPRODUCED |
| marginal m/J, ft fd=all | 0.4436 | 0.4436 [0.383, 0.504] | REPRODUCED |
| marginal m/J, full fd=prop | 0.2929 | 0.2929 [0.265, 0.321] | REPRODUCED |
| marginal m/J, ft fd=off | 0.1785 | 0.1785 [0.157, 0.200] | REPRODUCED |
| marginal m/J, full fd=off | 0.1357 | 0.1357 [0.120, 0.152] | REPRODUCED |
| marginal ratio | 1.514 [1.367, 1.661] | 1.514 [1.367, 1.661] | REPRODUCED |
| all-LOS population | 146 / 17,512 = 0.8% | 146 / 17,512 = 0.8% | REPRODUCED |
| integrity firings, original gate | 89 | 89 | REPRODUCED |
| integrity firings, corrected gate | 5 | 5 | REPRODUCED |
| mean nAnchors by tier | 3.70 / 3.12 / 3.91 | 3.70 / 3.12 / 3.91 | REPRODUCED |
| outage max, static_2p4 | 962.5 s | 962.5 s | REPRODUCED |
| outages, tier-1-floor arms | exactly 2 each | 2 / 2 / 2 | REPRODUCED |
| outage count, static_2p4 | 2820 | 2812 completed + 8 censored | SPEC DEFECT (see below) |
| outages > 60 s | 90 | 87 completed + 3 censored-already-past = **90** | **REPRODUCED** |
| outages > 300 s | 7 | 6 completed + 1 censored-already-past = **7** | **REPRODUCED** |
| tier-2 r80 / r50 | 909.5 / 1274.3 m | 909.5 / 1274.3 m | REPRODUCED |
| CRLB tier-2 gate | 1.105 at 1600 m, 3.272 vs 2.962 | identical | REPRODUCED |
| shadowing decorrelation (tier 2) | 0.90 | 0.90 | REPRODUCED |
| no `*** CENSORED` on tier 2 | absent | absent | REPRODUCED |
| **tier-1 r80** | **≈1522 m** | **1515.3 m** | **CONTRADICTED (0.4%)** |
| **UWB r80** | **≈13.5 m** | **12.8 m** | **CONTRADICTED (5%)** |

## Contradictions

| documented | measured | file | which is right |
|---|---|---|---|
| tier-1 r80 ≈1522 m | 1515.3 m | `results/probe_audit/probe_stdout.txt` | **1515.3** — v3 hedged with "≈" |
| UWB r80 ≈13.5 m | 12.8 m | same | **12.8** — 5% optimistic as documented |
| 2820 outages | 2812 completed + 8 censored | `results/tracegw9/.../trace_static_2p4_*.csv` | **both defensible** — see below |

**A CORRECTION TO THIS AUDIT'S OWN FIRST REPORT.** The first pass recorded
`>60 s` and `>300 s` as CONTRADICTED. That was wrong, and the error was mine:
**right-censoring is asymmetric for threshold counts.** An outage still in
progress at the window edge has observed duration `d_obs` and true duration
`d_true >= d_obs`, so `d_obs > T` settles the question — when it ends is
irrelevant. Only `d_obs <= T` is unknown.

Measured censored durations: **2.5, 15.0, 17.5, 47.5, 55.0, 100.0, 142.5, 712.5 s**.
Three already exceed 60 s; one already exceeds 300 s; none exceeds 962.5 s.

- `>60 s`: 87 completed + 3 censored-already-past = **90. Documented figure correct.**
- `>300 s`: 6 completed + 1 censored-already-past = **7. Documented figure correct.**
- max 962.5 s stands as an *observed* maximum, though seed 14's censored 712.5 s
  outage could in truth have exceeded it, so it is a lower bound.

**What remains is a specification defect, not a computation error.** Nobody
stated whether an in-progress outage counts as an outage. "2820 outages" and
"2812 completed outages" are both correct answers to different questions. The
statistic was under-specified.

| statistic | censoring-biased? |
|---|---|
| threshold counts (>60 s, >300 s) | **no** — exceedances already observed are certain |
| total count (2820 vs 2812) | label ambiguity only |
| median, p95 | **yes** — truncated durations bias both downward |
| max | yes in principle; not in fact here (no `d_obs` > 962.5 s) |

New failure mode: **a duration statistic computed over a finite window without a
stated rule for observations truncated by that window.**

## Untraceable numbers

| number | document | why untraceable | what would be needed |
|---|---|---|---|
| tier-2 r80 = 909.5 m, r50 = 1274.3 m | v3 §5.4 | no probe output on disk in `results/causal` newer than the current build | re-run `--probe=true`, capture stdout to a file |
| tier-1 r80 ≈ 1522 m | v3 §5.4 | same | same |
| UWB r80 ≈ 13.5 m | v3 §5.4 | same | same |
| CRLB gate 1.105 at 1600 m; 3.272 m vs 2.962 m | v3 §3.8 | same | same |
| shadowing decorrelation worst ratio 0.90 | v3 §5.4 | same | same |
| `static_uwb` 0.7618 J / 630.51 J | v3 §5.7 | `results/sd_*` predate build; not regenerated | re-run the searchDuty bracket |
| 660.689074664 − 0.583474664 identity | v3 §3.5 | per-tier rx energy columns not retained in any current file | re-run with the same instrumentation |
| `macEvals` 334/336 → 364 | v3 §3.6 | pre-fix state no longer reproducible at HEAD | would need the pre-fix commit, which does not exist in git |
| probe runs a static node | §8 premise | inferred from `transitionProb = dtState*(velocity*0.05+0.01)` and probe geometry; not measured | instrument the probe's velocity |

**All nine are probe- or pre-history-derived.** None is used in a claim-1 or
claim-2 verdict. But r80 = 909.5 m sizes the deployment geometry and is quoted in
the boxHalf corner arithmetic, so it is load-bearing for the *interpretation* even
though no verdict depends on its exact value.

## Gaps — checks I could not run

| check | why | what it would take |
|---|---|---|
| ~~V2.2 determinism at 6 non-default flags~~ | **CLOSED** — all six deterministic | — |
| V2.5 ablation divergence coincides with `suspect==1` | not run | 1 traced run + 3 seeds, ~30 min |
| V4.1 full constants table | not attempted in this pass | read 4 files, ~1 h |
| V4.4 verifier self-audit (early-return skipping) | not attempted | read + instrument, ~30 min |
| V5.1 denominator audit of every quoted rate | not attempted systematically | ~1 h |
| V7.3 multiple-comparisons count | not attempted | count tests across session, ~30 min |

## Sensitivity statement

**V3 (harness) found nothing.** It would have caught: a batch running fewer jobs
than claimed (count mismatch), a seed set with gaps (the `xargs` mis-grouping),
truncated or missing arms, and crashes swallowed into logs. It checked 13 batches
× 20 seeds = 260 runs. It would **not** catch a batch that ran the right number of
jobs with the wrong flags, because the flags are recorded in the same script that
generated them — an error there is self-consistent and invisible to this check.

**V2.3/V2.4 (RNG) found one live defect** (`LoRaDistributionSampler`) and
confirmed it matches documentation. It would have caught new unseeded RVs or a
missing `ResetNextStreamIndex`. It would **not** catch a seeded stream whose base
collides with another, since no collision check exists.

**V7/V8 (statistics) found nothing wrong in 18 of 21 numbers.** The
re-implementation shares the input files with the original, so it would catch
arithmetic, pairing and CI errors but **not** a defect in the simulator that
produced the CSVs. That is what V1.2 and V2.2 are for, and V2.2 has not completed.

**V8.3 found the one real contradiction**, and only because it recounted from raw
`hadFix` values with `awk` rather than reusing the Python summariser. That is
standing rule 2 working: the check that used a different route found the defect
the shared-code check would have missed.

## Standing-rule compliance

| rule | obeyed? | evidence |
|---|---|---|
| 1. Uniform token-count job lines | YES | all 6 post-incident scripts in `scripts/sweeps/` use 4-field lines |
| 2. Measure an unused quantity after a back-derivation | YES | this audit's V8.3 is an instance, and it found the contradiction |
| 3. Exact zero = control-flow claim | YES | the `+0.000` multi-step result was withdrawn after a reachability check |
| 4. A count needs a fixed denominator, a stated observation window, **and a stated rule for observations truncated by that window** | **VIOLATED on the third clause** | the outage count was quoted without stating whether an in-progress outage counts. The first two clauses held; only the truncation rule was missing, and that alone produced the 2820/2812 discrepancy |
| 5. Check every arm before claiming uniqueness | YES | the "only configuration delivering both" claim was withdrawn after checking `fixed_threshold` |

Rule 4 as originally stated was too narrow twice over. The window *was* fixed
(3600 s, 20 seeds); what was missing was the rule for observations truncated by
it. Widened form now in the table above.

**A sixth rule is warranted by this audit's own error:** *censoring is asymmetric
— a truncated observation still settles any threshold it has already crossed.*
Applying a symmetric "recount without the censored" produced two false
contradictions in the first pass.
