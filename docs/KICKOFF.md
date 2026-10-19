# Feeding this to an agent session — 48–72 hour run

Literal prompts. Paste them one at a time, in order. Do not paste them all at once.

**Before session 1, once:**

```bash
cd /path/to/ca-tbl-localization
git add RULES.md MASTER_PLAN.md docs/WORK_PACKAGES.md docs/CONTEXT_PACK.md
git commit -m "docs: agent working rules, master plan, and work packages"
```

`RULES.md` at the repo root loads automatically every session. That is the point of it — it
carries the rules that stop an agent destroying the evidence tree, and you should not have to
restate them.

**Pick a lane before you start.** See `docs/WORK_PACKAGES.md` § "The 48–72 hour cut". Lane A
builds `ESC_REQUEST`/`ESC_GRANT` on the wire; Lane B closes the measurement story. Both fit
2–3 days. Doing both does not.

---

## Hour 0 — start the compute yourself, before any agent session

This is the one thing that must not wait, because it runs while everything else happens. It needs
no code change.

```bash
mkdir -p results/c9_escalate
for pol in none inflate bounded reinit; do
  for fa in 0 1; do
    for s in $(seq 1 20); do
      ./ns3 run "ca_tbl_causal_sim --seed=$s --duration=3600 --boxHalf=2000 --gwGrid=5 \
        --escalationRequiresEvidence=true --nlosSource=propagation --pcsForm=v2 \
        --mahalUseR=true --injectRangeBias=40 --injectBiasFraction=0.2 \
        --injectFaultAnchors=$fa --suspectRecovery=$pol \
        --suspectForcesEscalation=true \
        --arms=ca_tbl_full,ca_tbl_no_integrity --trace=true \
        --outdir=results/c9_escalate/${pol}_F$((fa+1))_seed$s"
    done
  done
done
```

**Write the pre-registration before launching this**, not after. Prompt 1 below does that.

---

## Prompt 1 — orientation and pre-registration *(both lanes, ~30 min)*

```
Read RULES.md, then docs/WORK_PACKAGES.md, then MASTER_PLAN.md §1 and §5.

Do not change any code this session.

Task: write the pre-registration for work package A2, using the template at the end of
docs/WORK_PACKAGES.md. Save it as report/PREREG_C9_ESCALATE.md and commit it.

Context you need:
- The integrity check has been falsified as REJECTION ALONE: negative in the mean in 8/8
  cells, sign test against the policy in all eight. Source: the F1/F3 table in
  CA-TBL_Technical_Report_SelfContained.md §5.1, from results/abl_clean and results/c9_clean.
- All 160 of those runs set --suspectForcesEscalation=false. Zero runs combine fault
  injection with =true. Verify this yourself:
    grep -rh "injectRangeBias" results/ | grep -c "suspectForcesEscalation=true"
- The combined detect-and-escalate mechanism moves cep50M from 92.15 m to 3.10 m under
  propagation-derived NLOS and has never been run under injection.

The pre-registration must state, before any cell runs: the estimator (mean of per-seed
differences over 20 seeds with a sign test beside it, matching abl_clean so the two are
comparable), that F1 and F3 are reported separately, the abort condition (null at more
than one bias magnitude closes claim 2), and one thing this campaign cannot establish.

Report back what you wrote and what you verified. Do not start the runs.
```

---

# LANE A — build `ESC_REQUEST` / `ESC_GRANT` on the wire

## Prompt 2A — the small fixes first *(~2 hours)*

```
Two independent small changes. Separate commits, one change per commit.

(1) Work package D-3 in docs/WORK_PACKAGES.md. The CRLB gate runs on tier 1, whose
    estimator is RSSI inversion, not time-of-arrival. Read the comment at
    src/lora-physics-engine/model/submodules/lora-ranging-submodule.cc:151 first — it
    states that applying a ToA Cramer-Rao bound to it is a category error.
    Skip tier 1 in the gate. Also fix the pass string: worstCrlbRatio is tracked as a
    minimum and tested one-sided, but the string asserts a two-sided property.

(2) Work package A6. pcsThresholdDown and nDeescalate exist in the policy config but are
    not registered with cmd.AddValue, so the hysteresis band has never been swept. Add
    them to the AddValue block in scratch/ca_tbl_causal_sim.cc and thread them through
    RunArm. Match the house style for help text: explain WHY the default is what it is,
    the way the surrounding flags do.

For (2) the defaults must reproduce the current baseline byte-for-byte. Build optimized,
run the null control, diff arm_summary.csv against the existing baseline, and show me the
diff output before you commit.
```

## Prompt 3A — C1, the main build *(~1.5 days)*

```
Work package C1 in docs/WORK_PACKAGES.md. Read it fully before starting, and read
docs/archive/CA_TBL_PHASE8_SCOPE.md §3 for the original scoping — its findings hold but
its line numbers are stale by about 100 lines, so re-grep every symbol.

The problem: the escalation handshake is billed but never sent.
energy.AccountEscalationHandshake(...) is called at scratch/ca_tbl_causal_sim.cc:2610 and
:2720. No frame exists, so it never contends, never passes CAD, and never meets the duty
governor as a frame.

Build, in this order, committing each step separately:

1. A HandleReceive dispatch in ca-tbl-mac.cc keyed on GetMsgType(), with per-type reply
   logic. There is currently no receive-side dispatch at all — the MAC counts and charges
   per type but implements no request/response semantics.
2. Anchor-side grant/deny for ESC_REQUEST -> ESC_GRANT.
3. A --sendHandshakeFrames flag, DEFAULT FALSE, that switches the two accounting call
   sites to real frames when true.

Do NOT change ca-tbl-header.cc. Sizes, access categories and ACK policies are already
defined for all nine message types, and scratch/ca_tbl_packet_validator.cc covers them
38/38. If you think the header needs changing, stop and tell me why instead.

Constraints from RULES.md that apply here:
- The per-cycle invariant: exactly one tier transmits per cycle and only that tier is
  charged. Your frames must not break this. Check it explicitly and tell me how you did.
- Default-off flag with a byte-identical null control. This is not optional: arms share
  the macRng stream and consume it at their own rate, so any added transmission perturbs
  every arm.

Stop after step 1 and show me the dispatch design before implementing 2 and 3.
```

## Prompt 4A — null control and re-baseline *(~half a day)*

```
Verify C1.

1. Build optimized.
2. Null control: --sendHandshakeFrames=false must reproduce results/baseline_v6
   byte-for-byte. Diff arm_summary.csv and all six traces. Show me the result.
   If it is not byte-identical, STOP and find out why. Do not proceed.
3. Then run a small comparison with the flag on, and report what changed and by how much.
   Expect every escalating arm to change — real frames contend where a charge did not.
   That is the treatment effect, not a bug.
4. Run bash scripts/catbl_gates.sh. G3 fails by design and G11b is permanently red;
   everything else must be green. Report the summary.
5. Write a short entry in report/ recording what was measured, with every number traced
   to a file in results/.

Do not re-baseline any tracked results directory in place. Regeneration goes to a temp
directory. See RULES.md rule 7.
```

---

# LANE B — close the measurement story

Use this instead of Lane A if the goal is a write-up rather than new capability. Prompt 2A
applies to both lanes; run it either way.

## Prompt 3B — the dwell sweep *(compute; ~1 hour of agent time)*

```
Work package A1 in docs/WORK_PACKAGES.md. No code change.

Background: gateway density is NOT the variable that explains the conjunction result.
b1000/gwGrid=3 and b3000/gwGrid=9 have identical density (2.25 gw/km^2) and differ 8x in
median error. Precision-tier coverage fraction tracks it instead, and a dwell/transit
model predicts both geometries to within 1-2 percentage points. See
report/PCS_REDESIGN_RESULTS.md §8.2.

Write the sweep script for --dwell at boxHalf=3000 over {90, 150, 245, 400, 800} x 20
seeds. 245 s is the value predicted to restore b1000's coverage fraction, from
dwell/(dwell+1880) ~= 0.115.

Use the uniform 4-field job-line format the other scripts in scripts/sweeps/ use — that
convention exists because a previous xargs mis-grouping produced a seed set with gaps.
Add a row-counting completeness check, not a non-empty check: with --seedCount the summary
is appended per seed so it is non-empty from seed 1.

Write the script and the pre-registration. Do not launch the runs; I will.
```

## Prompt 4B — analysis *(~half a day, after compute finishes)*

```
Analyse results/c9_escalate against the pre-registration in report/PREREG_C9_ESCALATE.md.

Apply the registered decision rule verbatim, including where it returns no verdict.

Required outputs:
- Availability difference ca_tbl_full - ca_tbl_no_integrity, mean of per-seed differences
  over 20 seeds, with a sign test. F1 and F3 reported SEPARATELY.
- Detection precision and recall against the injected ground truth. This is CAL-05, which
  the project has never produced.
- A direct comparison against the rejection-only result in
  CA-TBL_Technical_Report_SelfContained.md §5.1.

Rules that apply:
- Never report a single-seed cross-arm difference.
- State the denominator, the observation window, and the rule for observations truncated
  by that window, for every count or rate.
- If any previously stated claim is now wrong, write a corrections register entry.
- Do not modify anything in results/. Analysis reads; it does not write there.

Then apply the A0 joint criterion to results/p10_grid if report/PREREG_A0*.md exists.
Tell me if it does not — that criterion must be written before it is applied, not after.
```

---

## Prompt 5 — close out *(both lanes, ~1 hour)*

```
Close-out pass.

1. Run the full gate suite and the verifier; report the summary, not just pass/fail.
2. Confirm every results directory cited by anything you wrote records dirty 0.
3. Check the "Definition of done" checklist at the end of docs/WORK_PACKAGES.md against
   what you actually did, and tell me honestly which boxes are not ticked.
4. Update MASTER_PLAN.md §2.1 for any row whose status your work changed, and add a
   dated correction note if you found any document stating something no longer true.
5. List what you did NOT do that was in scope, so it is on the record rather than
   implied.
```

---

## Three things to watch for

**It will want to fix the PCS.** The saturation defect is the most legible bug in the repo and
the fix looks like a two-line change. It invalidates every result in the tree. `RULES.md` §2
says so; if a session proposes it anyway, stop it.

**It will trust stale documents.** `report/PCS_REDESIGN_RESULTS.md` §7.10 says fault injection is
"not implemented, no flag" — that was true on 2026-08-05 and false since 2026-08-27. 162 results
directories use those flags. The rule in `RULES.md` §8 is to check the `AddValue` block and the
sidecars before believing any "not implemented" note. Watch that it does.

**It will report a single-seed difference as a result.** Arms share the RNG stream but consume it
at their own rate. This is the easiest rule to violate accidentally because the number looks fine.
