# Overnight diagnostic — SUMMARY

Generated 2026-08-29 01:38

No commits, no source edits, no `results/` deletions, no ledger updates. Findings are
recorded for Kushal to act on, not acted on.

**One exception, found by this batch's own tree check and reported in full at §9:**
D5's gate suite overwrote six tracked files under `results/`. They were preserved to
`raw/mutated_by_D5/` and restored; the tree now matches its as-found state.

## 1. Modules

| module | what it does | outcome | notes |
|---|---|---|---|
| **D0** | preflight | **OK** | rc=0 0s |
| **D1** | arm liveness matrix | **FINDING** | rc=0 834s |
| **D2** | flag-confound audit | **FINDING** | rc=0 0s |
| **D3** | degeneracy pre-screen | **FINDING** | rc=0 2s |
| **D4** | provenance & completeness | **FINDING** | rc=0 37s |
| **D5** | gates & negative fixtures | **OK** | rc=0 1482s |
| **D6** | document reconciliation | **FINDING** | rc=0 0s |
| **D7** | clean integrity test | **SKIPPED** | rc=0 0s |
| **D8** | build hygiene | **OK** | rc=0 1s |

## 2. Degenerate / inert / broken cells (D1)

| config | arm | verdict | configuration |
|---|---|---|---|
| A | `ca_tbl_full` | **GATE INERT: 0 suspects** | boxHalf=2000 RURAL_OPEN legacy dwell=90 |
| B | `ca_tbl_no_integrity` | **DEGENERATE: 100.0% on 868** | boxHalf=2000 RURAL_OPEN propagation dwell=90 |
| B | `ca_tbl_no_integrity` | **DEGENERATE: 0 escalations** | boxHalf=2000 RURAL_OPEN propagation dwell=90 |
| C | `ca_tbl_full` | **GATE INERT: 0 suspects** | boxHalf=1500 RURAL_OPEN legacy dwell=800   [H_cost PASS cell] |
| D | `ca_tbl_full` | **GATE INERT: 0 suspects** | boxHalf=2000 RURAL_FOREST legacy dwell=800 [H_reliability cell] |
| E | `ca_tbl_no_integrity` | **DEGENERATE: 100.0% on 868** | boxHalf=2000 RURAL_OPEN propagation dwell=90 + suspectForcesEscalation=false |
| E | `ca_tbl_no_integrity` | **DEGENERATE: 0 escalations** | boxHalf=2000 RURAL_OPEN propagation dwell=90 + suspectForcesEscalation=false |
| E | `ca_tbl_full` | **DEGENERATE: 100.0% on 868** | boxHalf=2000 RURAL_OPEN propagation dwell=90 + suspectForcesEscalation=false |
| E | `ca_tbl_full` | **DEGENERATE: 0 escalations** | boxHalf=2000 RURAL_OPEN propagation dwell=90 + suspectForcesEscalation=false |

## 3. Confounded campaigns (D2)

- `results/p10_grid` (all 320 B1 + 680 B2 cells): `suspectForcesEscalation` unset, therefore **the `ca_tbl_no_integrity` column is confounded** — it measures escalation, not rejection. **The `ca_tbl_full` vs `static_2p4` comparisons in the same directories are NOT affected**, because neither arm's behaviour depends on that flag (`static_2p4` has no gate; `ca_tbl_full` has the flag at its documented default in both members of the comparison). H_cost and H_reliability stand.
- `results/p10_env_v7`: `floorOnDemand` unset — energy comparisons omit a 40-47% reduction. Superseded by `p10_env_v8`, which sets it.
- `results/p10_env_v8`, `results/p10_plane_v7`: `floorOnDemand` and `escalationRequiresEvidence` both set. `suspectForcesEscalation` unset, so their `no_integrity` columns carry the same confound; their `static_2p4` comparisons do not.

STATUS: FINDING

## 4. Provenance and digest failures (D4)

- entries checked against files on disk: **12130**
- directories with NO digest block: **704**
- digest failures (missing/unreadable/hash mismatch): **0**

**Killed mid-run:**
- `results/_v6_recheck_a5` (6 traces, no arm_summary)

## 5. Document numbers that could not be reproduced (D6)

| claim | document | cited dir | published | reproduced | verdict |
|---|---|---|---|---|---|
| trig_precision | Simulation.md/ledger | `results/p10_plane_fix` | 0.9343 | 0.9279 (n=20) | **MISMATCH** |
| trig_recall | Simulation.md/ledger | `results/p10_plane_fix` | 0.7041 | 0.7005 (n=20) | **MISMATCH** |
| ca_tbl_full totalJ | report §1.3 | `results/p10_plane_fix` | 24.2600 | 24.2597 (n=20) | MATCH |

## r80 = 909.5 m, quoted in three documents

- `Architecture.md:71` still quotes 909.5 m — measured post-D1 value is **1149.5 m**. **MISMATCH**
- `CA-TBL_Technical_Report_SelfContained.md:62` still quotes 909.5 m — measured post-D1 value is **1149.5 m**. **MISMATCH**
- `CA_TBL_SIMULATION_REPORT.md:72` still quotes 909.5 m — measured post-D1 value is **1149.5 m**. **MISMATCH**

## 6. D7 — clean integrity test

**SKIPPED**, and this is the registered reason.

D1 config E measured `ca_tbl_full` with **0 escalations** under
`--suspectForcesEscalation=false`. Without the suspect trigger the arm does not
escalate either, so both arms are degenerate and the comparison is void in the
other direction. Running it would produce two tier-1-locked arms and a
meaningless equality.

STATUS: SKIPPED

## 7. Tree state

- **start**: clean (no tracked modifications)
- **during the batch**: six tracked files were modified —

```
 M results/causal/arm_summary.csv     M results/det_a/arm_summary.csv
 M results/causal/run_id.txt          M results/det_a/run_id.txt
 M results/det_b/arm_summary.csv      M results/det_b/run_id.txt
```

- **end, after restoration**: clean (no tracked modifications)

**RESOLVED — mid-batch mutation detected and corrected.** Cause: D5's gate suite
writes into tracked result directories; full account and the reasoning in **§9**.
The regenerated files were preserved to `raw/mutated_by_D5/` and the six paths
restored with `git checkout --`. Final tree state matches as-found.

The mutation is kept on the record deliberately — it is one of this batch's
findings, not an incident to tidy away. **It does not invalidate anything below:**
the affected paths are `results/causal`, `results/det_a` and `results/det_b`, none
of which is a source for §2, §3, §5, §6 or §10. §4's digest verification ran
before D5 and covered 12,130 entries with zero failures.

## 8. What was NOT done, and why

- **No fresh-clone build (D8).** ~40 min of budget; D5 exercises the built binary instead. The clone test therefore remains unverified against the post-D1 engine.
- **`baseline_v7` was not generated.** It is a close-out item, not a diagnostic, and §12 forbids changing anything.
- **No document or ledger updates.** W1, W4, Claim 3's new entry, Claim 7's review and `Architecture.md:71` (r80 909.5 -> 1149.5) all remain outstanding.
- **Claim 9 was not moved.** The B2 `no_integrity` observation is confounded; D7 is the test that would settle it, and its outcome is in §6.
- **D1 covers 6 configurations x 6 arms at 1 seed.** It detects degeneracy, not effect size. A cell reading `ok` here is not evidence of a result.
- **D3 screens existing directories only.** It cannot flag a configuration never run.
- **D5's negative fixtures cover the digest check and the degeneracy screen.** The gate suite's individual assertions were run but not each given a failing fixture.
- **No campaign was re-run to replace a confounded one.** D2 lists what is affected; regenerating is a decision for Kushal.

---

## 9. FINDING discovered by this batch's own tree check: D5 mutates its subject

`scripts/catbl_gates.sh` **writes into tracked result directories**. Running it
modified six tracked files:

```
 M results/causal/arm_summary.csv      M results/det_a/arm_summary.csv
 M results/causal/run_id.txt           M results/det_a/run_id.txt
 M results/det_b/arm_summary.csv       M results/det_b/run_id.txt
```

The diff is a full regeneration — `source a059203 dirty 4` replaced by
`source 13a62bb dirty 0`, with new digests.

**Why this matters beyond tidiness.** These directories are *evidence*. The gate
suite is the thing that verifies them, and running it overwrites them with output
from whatever binary is current. A verification step that rewrites its own subject
cannot be used to check whether the subject was correct before it ran, and the
overwrite is silent — nothing in the suite reports it.

It also means every past invocation of `catbl_gates.sh` silently re-dated these
three directories, so their sidecars record the last gate run rather than the run
that produced the evidence.

**Action taken:** the six files were copied to
`raw/mutated_by_D5/` and then restored with `git checkout --` so the tree matches
its as-found state. Nothing was committed. The regenerated versions are preserved
in `raw/` in case the *newer* provenance (`dirty 0` against the post-D1 engine) is
the version Kushal would rather keep — that is his call, not the batch's.

**Tree state after restoration: matches as-found (clean).**

---

## 10. Synthesis — three structural findings D1 produced that were not being looked for

These are recorded, not acted on. Each would move a claim.

### 10.1 Under `legacy` NLOS, `ca_tbl_full` and `ca_tbl_no_integrity` are the same arm

Configs A, C and D — every `legacy` configuration tested — produce **0 suspects**, and
the two arms come back with identical cycle splits and identical `cep50M`:

```
A  full 2/1365/73  esc 4  sus 0  cep50 2.4203      no_integrity 2/1365/73  esc 4  cep50 2.4203
C  full 3/964/473  esc 3  sus 0  cep50 1.3834      no_integrity 3/964/473  esc 3  cep50 1.3834
D  full 18/1216/206 esc 19 sus 0 cep50 2.8844      no_integrity 18/1216/206 esc 19 cep50 2.8844
```

The gate never fires, so the ablation compares an arm against itself — in **these
configurations**.

**Scope corrected (post-batch, task 1).** An earlier draft of this section read "any
`legacy` result reported as with vs without the integrity check is measuring nothing."
That is **too broad** and is withdrawn. Measured across both injected-bias campaigns:

```
campaign_recovery    240 paired cells   suspects median 106, zero in   0/240   arms differ 240/240
campaign_20260810    280 paired cells   suspects median  28, zero in  52/280   arms differ 228/280
```

Under an injected 40 m bias the gate fires hard and the arms are **not** identical, so
**Claim 9's falsification is measured on a live comparison and stands.** The correct
scope of this finding is: *legacy configurations **without** injected bias*, where the
naturally-occurring Mahalanobis distance rarely exceeds `9.21` on an all-LOS cycle.

The `no-injection` claim survives, and it still covers **config C, the H_cost PASS
cell**. That PASS is a real cost result for the tiered policy against `static_2p4`, but
it carries no information about integrity.

Consistent with the `allLos` precondition at `ca-tbl-policy.cc:903`, which is still
hardcoded because `suspectRequireAllLos` was specified and never built.

### 10.2 Under `propagation` NLOS, PCS never escalates — the gate's side effect is the only escalation mechanism

Config E, `ca_tbl_full` with `--suspectForcesEscalation=false`:

```
361 suspects, 0 escalations, 1440/1440 cycles on 868, cep50 89.91 m
```

The gate detects 361 times and the arm still never leaves tier 1. Removing the
suspect trigger removes **all** escalation, which means PCS alone
(`pcs < pcsThresholdUp`) never fires under propagation NLOS at this geometry.

The confidence-driven escalation the architecture is built around is not what
escalates. The integrity gate's documented side effect is. That is a claim-level
finding about the policy, not about the gate.

### 10.3 `trig_tp + trig_fp = 0` in all six configurations — CORRECTED

The trigger metric has **no population in the six configurations tested here**
(configs A–F, one seed each), for every arm. `trig_precision` and `trig_recall` are
undefined-by-construction **in those runs**.

**Two claims that appeared here originally were wrong and are withdrawn:**

> ~~the ledger's published values come from the injection campaigns only~~

False. `results/p10_plane_fix` is a **non-injection** campaign and carries
`tp + fp = 1370` pooled over 20 seeds; `p10_plane_v7` carries 1459. Whether a
configuration exercises the trigger is a property of **that configuration**, not of
whether it injects faults. The six configs above simply are not that design.

> ~~the published `trig_precision` (0.9343) and `trig_recall` (0.7041) do not
> reproduce from the directory the ledger cites (measured 0.9279 and 0.7005)~~

False. They reproduce **exactly** under the estimator the ledger's own text
specifies, *"20 seeds pooled"*. 0.9279 / 0.7005 is the **mean of per-seed ratios** — a
different estimator over the same population, compared against a pooled figure.

    pooled over 20 seeds     0.9343 / 0.7041   <- the published pair
    mean of per-seed ratios  0.9279 / 0.7005   <- what was computed

Both errors have the shape of the project's standing rule — *a count is not an
observable until its population is stated* — with a different unstated quantity each
time: the **estimator** in one, **which directories the claim ranged over** in the
other. See `DEFECTS.md`, "The population rule now has an estimator clause".

### 10.4 What this does NOT establish

- It does **not** falsify the integrity gate. Under `propagation` the gate fires
  (158-361 suspects) and the combined detect-and-escalate mechanism does move
  `cep50M` from 92.15 m to 3.10 m. Whether *rejection alone* is worth anything is
  the question D7 was built to answer, and D7 is void because both arms are
  degenerate without the escalation trigger.
- It does **not** invalidate H_cost or H_reliability. Those compare `ca_tbl_full`
  against `static_2p4`, and neither arm's behaviour depends on the flags at issue.
- 1 seed per configuration. These are degeneracy verdicts, not effect sizes.
