# CA-TBL — Independent Verification Audit Report

Run 2026-08-01 at HEAD `c9ab733b5`. Auditor had a shell; all sim runs via
`runuser -u kushal`. Audit outputs in `audit/` (not committed).

## Summary

```
REPRODUCED: 37    TRACED: 0    UNTRACED: 9    CONTRADICTED: 3
Checks run: 32    Checks blocked: 5
```

Every headline statistic reproduced. The contradictions are confined to one
metric (outage counts) and are a methodological defect, not a fabrication. The
larger problem is coverage: five checks could not be run, and the probe figures
that size the entire deployment geometry are among them.

## Blocking findings

**None that invalidate a headline claim.** One remains open (B1, mitigated); B2
has been closed since the first report.

**B1. The gw3-vs-gw9 comparison spans two binaries.** The binary is dated
`2026-08-01 02:20:43`. Every results directory through `ms_c3_fair` (`01:46`)
predates it; `gw9_b3000` onward postdates it. The headline finding — that claim
2's parity at boxHalf 3000 was a tier-1 redundancy artefact — compares gw3 data
built at `3bbb30df6` against gw9 data built at `c9ab733b5`, with `--mahalUseR`,
`--fastDemote`, `--multiStepDemote` and `--gwGrid` added in between.
**Mitigated but not eliminated:** V1.2 regenerated `sweep20_b3000/seed_7` at HEAD
and it is byte-identical. That is one seed at one geometry, not the whole set.
**Strengthened after V2.2 completed:** all six non-default flag configurations are
deterministic and their defaults are byte-identical to the pre-flag baseline, so
the flags added between the two builds provably do not perturb behaviour when off.
The residual exposure is a change that alters behaviour identically on repeat runs
— which byte-identity at seed 7 plus flag-default acceptance makes unlikely, but
does not exclude. Regenerating all 20 seeds of `sweep20_b3000` at HEAD would close
it (~30 min).

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
| **outage count, static_2p4** | **2820** | **2812** | **CONTRADICTED** |
| **outages > 60 s** | **90** | **87** | **CONTRADICTED** |
| **outages > 300 s** | **7** | **6** | **CONTRADICTED** |

## Contradictions

| documented | measured | file | which is right |
|---|---|---|---|
| 2820 outages | 2812 | `results/tracegw9/seed_*/trace_static_2p4_*.csv` | **2812**, with 8 censored |
| 90 outages > 60 s | 87 | same | **87** completed, 3 censored |
| 7 outages > 300 s | 6 | same | **6** completed, 1 censored |

**Cause, confirmed:** exactly **8** of the 20 `static_2p4` traces end with
`hadFix = 0`, and 2820 − 2812 = 8. The summariser's trailing `if cur:
runs.append(cur)` counted each right-censored outage as a completed one of its
observed length. This both inflates the count and *understates* the durations of
those 8, because a censored outage is at least as long as observed.

**Impact:** the claim "one outage exceeding five minutes roughly every three
hours" rests on 7, of which 6 are complete. The qualitative finding (the tail is
severe; 16-minute worst case) survives — the max is a completed outage. The
counts should be restated as "87 completed outages over 60 s, plus 3 censored".

This is failure mode #12-adjacent and a new one: **a duration distribution
computed over an observation window treats truncation as an event.**

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
| 4. A count needs a fixed denominator | **VIOLATED** | the outage *count* 2820 was quoted against a 3600 s window without stating that 8 observations were censored by the window edge |
| 5. Check every arm before claiming uniqueness | YES | the "only configuration delivering both" claim was withdrawn after checking `fixed_threshold` |

Rule 4's violation is the same defect as the contradiction above, which suggests
the rule as stated is too narrow: it should read **a count needs a fixed
denominator *and* a stated observation window.**
