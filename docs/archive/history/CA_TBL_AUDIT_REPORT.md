# CA-TBL — Independent Verification Audit Report

Run 2026-08-01 at HEAD `c9ab733b5`. Auditor had a shell; all sim runs via
`runuser -u kushal`. Audit outputs in `audit/` (not committed).

## Summary

```
REPRODUCED: 47    TRACED: 0    UNTRACED: 1    CONTRADICTED: 3
Checks run: 43    Checks blocked: 3
Defects found: 4  (B3 category error, V4.4-D1 missing-arm, V4.4-D2 staleness,
                   tier-1 estimator gate absent — now added and passing)
```

Every headline statistic reproduced. The three remaining contradictions are two
probe range figures (0.4% and 5% off, both hedged with "≈" in the source
document) and one under-specified statistic. **This audit's first pass recorded
two further contradictions that were themselves wrong** — see Contradictions.

The most consequential finding is B3: the CRLB gate is applied to tier 1, whose
estimator is RSSI inversion, not time-of-arrival — a comparison the ranging
submodule's own comment calls a category error. Found only because the probe
output was captured to a file for the first time.

## Blocking findings

**None that invalidate a headline claim.** B1 and B2 are both CLOSED. One NEW
finding (B3) was opened by the probe capture and is not closed.

**B3. The CRLB gate is applied to a tier whose estimator it does not describe.**
Two layers, and the second is the real one.

*Reporting defect (minor).* `ca_tbl_causal_sim.cc:1471` tracks `worstCrlbRatio` as
a **minimum** and tests only `< 0.333`. That is correct by design — only
`ratio < 1` is physically impossible, and real estimators routinely sit 1.5-3x
above CRLB. But every other value falls to an `else` printing *"CRLB OK (within
3x of the single-shot bound)"*, which asserts a two-sided property from a
one-sided test. Minimal fix is the string: *"no point beats the bound by more
than 3x"*, plus a separate `bestCrlbRatio` tracked as a maximum if the upper side
is wanted. v3 section 3.8 quotes "within 3x" as though a two-sided check passed.

*Category error (the real finding).* The gate runs on **tier 1**, and
`lora-ranging-submodule.cc:151` states in its own comment that this is invalid:
*"RSSI-inversion ranging. bandwidthHz, snrDbInBand and spreadingFactor are unused
here by design: this is not a time-of-arrival estimator, so the ToA Cramer-Rao
bound does not apply to it and applying one would be a category error."*
The tier-1 estimator inverts `propagationLossDb` and its sigma is
`estD * ln10/(10n) * shadowingStdEff` — proportional to range, which is the
~50%-of-distance signature v3 section 5.4 records. **The measured tier-1 ratio of
3.319 therefore compares unrelated quantities**, and the probe has been reporting
it for that tier throughout. The gate should not run on tier 1 at all.
Tiers 2 and UWB are ToA estimators and the gate is meaningful there.
Evidence: `results/probe_audit/probe_stdout.txt`,
`ca_tbl_causal_sim.cc:1462-1490`, `lora-ranging-submodule.cc:151-188`.

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
| **tier-1 r80** | **1522 m** | **1515.3 m** | **CONTRADICTED — stale carry-forward** |
| **UWB r80** | **13.5 m** | **12.8 m** | **CONTRADICTED — stale carry-forward** |

## Contradictions

| documented | measured | file | which is right |
|---|---|---|---|
| tier-1 r80 1522 m | 1515.3 m | `results/probe_audit/probe_stdout.txt` | **1515.3** |
| UWB r80 13.5 m | 12.8 m | same | **12.8** |
| 2820 outages | 2812 completed + 8 censored | `results/tracegw9/.../trace_static_2p4_*.csv` | **both defensible** — see below |

**On the two probe figures.** These are not "approximately" tolerances. The probe
is deterministic at a fixed seed, so identical code gives identical values, and
**tier-2 r80 reproduces to 909.5 m exactly** while tier-1 and UWB drift. A changed
channel realisation would have moved all three. The likelier cause is that v3
section 3.1 measured tier-2 r80 freshly that session (after the censoring fix)
while section 5.4's tier-1 and UWB figures were carried forward from an earlier
build. Remedy: update v3 to the captured values and name the build they came from.
Neither verdict moves at 12.8 m, but `uwbTriad = 10 m` was sized against 13.5 and
the `uwbTriad <= 15` guard derived from it, so the sizing rationale should quote
the current number.

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

## B3's consequence — and the gate that was missing

The probe runs three gates: monotonicity and shadowing decorrelation (channel),
and CRLB (estimator). Since the CRLB gate is a category error on tier 1, **tier 1
had no estimator check at all** — the tier supplying 77% of fixes at boxHalf 3000
and acting as the coverage floor for the whole architecture had never been
validated against anything.

Added here as a self-consistency check, from columns already in
`probe_range_sweep.csv`. Tier-1 sigma is closed-form:
`sigma = estD * ln(10)/(10n) * shadowingStdEff`, with `n = 2.3` for
`ENV_RURAL_FORESTED` (`lora-physics-parameters.cc:27`, `Normal(2.3, 0.1, 2.1, 2.6)`).
Predicted fractional error is `0.1001 * shadow_sd_eff`.

| region | n | mean measured/predicted | CV |
|---|---|---|---|
| PDR >= 0.90 (unbiased) | 4 | **0.696** | **2.6%** |
| PDR < 0.90 (survivorship-biased) | 5 | 0.578 | 6.4% |

**PASS.** The ratio is flat to 2.6% wherever `mean_abs_range_err_m` is unbiased.
The monotone decline beyond (0.728 -> 0.524 across the full sweep) is confined to
the low-PDR tail where v3 section 3.9 documents survivorship conditioning, and its
direction — measured error too LOW — is what that bias predicts, since a link that
decodes at 6 km had a favourable shadowing draw that also makes it range better.
The constant 0.696 offset is the mean-abs-vs-sigma factor, below the ~0.80
Gaussian expectation as it should be for a log-normal inverted through the
path-loss model.

**Stated limit: this is self-consistency, not physical validation.** It confirms
the code implements its own formula. It says nothing about whether RSSI-inversion
ranging with a 7-9 dB shadowing sigma is the right model for forest at 868 MHz;
that needs field data.

### V4.4 Verifier self-audit — RUN, two holes found

| check | verdict | evidence |
|---|---|---|
| gate-count arithmetic matches call sites | REPRODUCED | 19 `check()` + 5 `warn()` sites, multiplied by arm/seed loops, give 72/15 |
| early exits are loud, not silent | REPRODUCED | every `return`/`continue`/`break` is preceded by a `FAIL.append` |
| **missing-arm detection** | **DEFECT** | see below |
| **trace-vs-summary staleness** | **DEFECT** | no `getmtime`/`st_mtime`/staleness logic anywhere in the file |

**V4.4-D1. The verifier passes while auditing an arbitrary subset of arms.**
Deleting 2 of 6 trace files from a directory and re-running:

| | checks | gates | verdict | exit |
|---|---|---|---|---|
| all 6 traces | 72 | 90 | "All invariants hold" | 0 |
| 2 traces deleted | **54** | **68** | **"All invariants hold"** | **0** |

Nothing asserts which arms should be present. A batch that silently produced four
arms would pass cleanly. **This directly undermines "72 checks passed" as a
load-bearing statement** — 54 checks passed prints the same conclusion, and the
gate count is the only signal, compared against nothing. Same class as the
`warn()` hole found earlier in the session: a gate that stops protecting when its
input changes shape.

**V4.4-D2. No staleness check.** v3 section 3.10 (verifier read a previous
session's traces alongside a fresh summary) was caught by an internal
contradiction, not by a gate, and there is still no gate. The only protection is
run ordering inside `catbl_gates.sh`, which protects that script's runs and not a
direct invocation.

### V5.1 Denominator audit — RUN, two under-stated rates

| rate | issue |
|---|---|
| firings "61% -> 3.2% of eligible" | denominators differ: 146 (trace3000) vs 157 (mahalR3000). Defensible — each run has its own eligible population — but never stated. |
| `blind/esc` "4.15 -> 0.96" | both terms moved: blindCycles 16188 -> 8142 *and* escalations 3900 -> 8488 (aggregate over 20 seeds). The ratio is sound; it was reported as though only the numerator moved. |

Neither changes a verdict. Both are instances of the rule-4 family and support the
widened form. The escalations-per-fix-cycle catch earlier in the session was the
same defect found by accident, which is why this phase was worth running.

## Note on the remaining two gaps

**V5.1 (denominator audit)** has the stronger yield argument: the censoring defect
above is literally a V5.1-class finding, surfaced by accident in V8.3. A phase
that would have found it deliberately is worth running.

**V4.4 (verifier self-audit)** is supported by analogy only, and the report should
not overstate it. B3 was found in the *probe's* gate in `ca_tbl_causal_sim.cc`,
not in `verify_causal_sim.py`. Different code, different author-moment. It raises
the prior — a project that ships one gate whose message overstates its test
plausibly ships two — but `verify_causal_sim.py` has **not** been shown defective
by this audit. The one hole previously found in it (`warn()` recording nothing on
success) was found and fixed earlier in the session, not here.
