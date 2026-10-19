# Pre-registration — C9-ESCALATE: detect-and-escalate under injected fault

Written 2026-09-05, **before any cell of this campaign has run**.
Source commit: `170cd20`   Build profile: optimized
Work package: A2, `docs/WORK_PACKAGES.md`. The branch point of `MASTER_PLAN.md` §8.

---

## 0. What prompted this, stated so it can be checked

The Switching-Point Integrity Check has been falsified as **rejection alone**: negative in
the mean in 8/8 cells, with the sign test running against the policy in all eight
(`CA-TBL_Technical_Report_SelfContained.md` §5.1, from `results/c9_clean` and
`results/abl_clean`).

Every one of those cells set `suspectForcesEscalation=false`. Census over the tracked
sidecars, re-run at commit `170cd20` rather than quoted from a working document:

    run_id.txt citing injectRangeBias            162
    ... with suspectForcesEscalation=true           0
    ... with suspectForcesEscalation=false        160
    distinct bias magnitudes                        1   (40 m)
    distinct window fractions                       1   (0.2)

The combined **detect-and-escalate** mechanism — the one recorded as moving `cep50M` from
92.15 m to 3.10 m under propagation-derived NLOS — has therefore never been run under
injection at all. This campaign runs it.

### 0.1 A correction to the work package, applied here

`docs/WORK_PACKAGES.md` A2 and `docs/KICKOFF.md` both give a loop that writes
`--injectFaultAnchors=$fa` into a directory named `F$((fa+1))`. That mapping is **wrong in
both directions** against the convention every existing directory uses:

| flag | fault model | directory name in existing evidence |
|---|---|---|
| `--injectFaultAnchors=1` | F1, single-anchor — the canonical RAIM model | `*_F1_seed*` |
| `--injectFaultAnchors=0` | F3, common-mode across every tier-2 link | `*_F3_seed*` |

Verified against `results/abl_clean/bounded_F1_seed1/run_id.txt` (`injectFaultAnchors=1`)
and `results/abl_clean/bounded_F3_seed1/run_id.txt` (`injectFaultAnchors=0`).

The work package's loop would label common-mode as `F1` and emit a nonexistent `F2`,
silently setting F3 beside F1 when the result is read against the §5.1 table. **This
campaign uses the existing convention.** The naming is corrected here, before the runs,
because afterwards it would be indistinguishable from relabelling to taste.

---

## 1. Question

With escalation enabled, does `ca_tbl_full` beat `ca_tbl_no_integrity` on availability
under the fault class the integrity check was designed for?

---

## 2. Population and denominator

**Stage 1.** 4 recovery policies × 2 fault models × 20 seeds = **160 runs**.

    --boxHalf=2000 --gwGrid=5 --duration=3600
    --escalationRequiresEvidence=true --nlosSource=propagation
    --pcsForm=v2 --mahalUseR=true
    --injectRangeBias=40 --injectBiasFraction=0.2
    --suspectForcesEscalation=true
    --suspectRecovery={none,inflate,bounded,reinit}
    --injectFaultAnchors={1,0}  ->  {F1,F3}
    --arms=ca_tbl_full,ca_tbl_no_integrity

Geometry, environment, seeds, arms and every flag other than `suspectForcesEscalation`
match `results/abl_clean` and `results/c9_clean`, so the resulting table is comparable to
§5.1 cell for cell. `none` is the policy held in `c9_clean`; the other three are in
`abl_clean`.

**Denominator.** Availability is over **all 1440 decision cycles of a run**. A cycle with
no fix counts against. This is the denominator §4.1 of the technical report uses, and it
is not renegotiated here.

**Observation window.** The full 3600 s run. The injected bias is active over one
contiguous window of `injectBiasFraction` of the run, with a **seed-derived start that is
identical across arms** — the ablation measures the gate, not the fault placement.

**Truncation rule.** A cycle whose window is cut by run end still settles any threshold it
has already crossed; it is not recounted symmetrically. (`RULES.md` rules 1 and 2.)

---

## 3. Metric and estimator

**Primary.** Availability difference `ca_tbl_full − ca_tbl_no_integrity`, **paired per
seed**, reported as the **mean of per-seed differences over 20 seeds with a sign test
beside it**. This is the estimator §5.1 used, chosen so the two tables can be read against
each other without re-derivation.

**F1 and F3 are reported separately, never pooled.** They are different fault models, and
the gate's effect is already recorded as form-dependent.

**Secondary, and the reason this campaign is worth its compute.** Detection precision and
recall against injected ground truth, per fault model: a firing inside the bias window is
a true positive, a firing outside it a false positive. This is **CAL-05** — the
calibration the thesis register defers to Phase 2.1 and the project has never produced.

**Recorded but not adjudicated:** total energy, escalation count, `cep50M`, `noFixCycles`.
Energy is expected to rise, because escalation costs. This campaign registers no energy
criterion, so **no energy verdict will be stated from it**; A0 is where that belongs.

No single-seed cross-arm difference will be reported. (`RULES.md` rule 8.)

---

## 4. Prediction

Stated numerically, with direction, before any cell runs.

1. The mean paired difference is **≥ 0 in at least 5 of the 8 cells**, against 0 of 8 in
   the rejection-only result.
2. At least one policy shows a mean difference **> +0.01 in both fault models**.
3. The improvement is **larger for F3 than for F1** at the same policy. A common-mode bias
   on every tier-2 link degrades the entire tier-2 fix, so escalating away from it should
   supply more than escalating away from one displaced anchor.
4. Detection recall inside the bias window is **> 0.5** for F3.

The mechanism these follow from: rejection alone discards a fix and supplies nothing in its
place, so availability falls. Detect-and-escalate discards **and** requests a better tier,
so a replacement fix is available in the same cycle.

---

## 5. Falsifier

The prediction is wrong if the mean paired difference is **negative in 6 or more of the 8
cells**, reproducing the rejection-only pattern with escalation enabled. That is reachable
by this campaign, and it would mean escalation does not rescue the rejected cycle.

A positive mean whose sign test does **not** run with `ca_tbl_full` also falsifies it: that
is a few large seeds, not an effect.

---

## 6. Decision rule — three outcomes, fixed now

**PASS.** Mean difference > 0 with the sign test running with `ca_tbl_full` in **≥ 6 of 8
cells**, and the direction holding at **at least two bias magnitudes** in stage 2. Claim 2
is rehabilitated as *detect-and-escalate*, never as *rejection*.

**FAIL.** Mean difference negative in **≥ 6 of 8 cells** at **more than one bias
magnitude**. Claim 2 closes honestly and the thesis takes the Track B framing.
`Simulation.md` §13.2 registered in advance that a null here is a real answer.

**INCONCLUSIVE.** Anything else — including a sign that differs across fault models, and
including the case where the gate fires too rarely for a difference to be resolvable.
Explicitly: **if neither arm meets `(A ≥ 0.95, R ≤ 25 m)` anywhere in the grid, that is
INCONCLUSIVE, not PASS.** A criterion no competitor can meet decides nothing.

---

## 7. Abort conditions

- If stage 1 returns FAIL as defined above, **stage 2 runs at two further magnitudes only**
  ({10, 160} m), to satisfy the "more than one magnitude" clause, and then stops. The rest
  of the sweep is not run: it would be measuring the shape of a null.
- If any cell fails to produce 20 seeds of `arm_summary.csv` rows, the campaign stops and
  the missing cells are named. **Row counts are checked, not non-emptiness** — with
  per-seed appending, a summary is non-empty from seed 1. (`RULES.md` rule 12.)
- If `run_id.txt` records `dirty` other than 0 for any cited directory, that directory is
  not citable and the campaign is re-run from a clean tree.

---

## 8. What this campaign cannot establish

**The prior is not the specified prior.** The integrity check tests a fix against a
constant-velocity prior propagated from previous radio fixes. Chapter 10 §10.7.2 specifies
an **independent** prior, and a prior built from earlier radio fixes is not independent of
the radio in the sense that requires. A positive result here is evidence about **the
implemented check**, not about the specified one. This qualification travels with any
positive result and is not to be dropped in summary.

Also outside its reach:

- One geometry (`boxHalf=2000`, `gwGrid=5`) and one environment. Nothing here transfers to
  the density sweep without being run there.
- The bias is injected on tier-2 links. It says nothing about tier-1 or tier-3 faults.
- The ordering among recovery policies is a **measured value with no mechanism claimed**,
  as in §5.1. It will not be reported as a mechanism here either.
- Detection precision and recall are measured against an *injected* fault whose window is
  known. They do not establish the gate's false-positive rate against natural channel
  variation, which is a different population and is not sampled here.
