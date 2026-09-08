# Behaviour anomalies — when the system does something it was not meant to

**Started 2026-09-09.** This is not `TECH_DEBT.md`. That register holds things known to
be **missing, unfinished or wrong by construction**. This one holds things the system
**does** that nobody intended — where the code is arguably correct, the gates are green,
and the behaviour is still not what the design meant.

**Why they need separate files.** Debt is a decision you have already made. An anomaly is
a question you have not answered yet, and the two get triaged differently: debt is
scheduled, an anomaly is *diagnosed first*, because until you know why it happens you do
not know whether it is a bug, a modelling artefact, or a real property of the system that
nobody predicted. Filing an anomaly as debt closes the question prematurely, and this
project has already paid for that twice — the 51 J artefact and the "868 beats CA-TBL"
withdrawal were both anomalies triaged as findings before they were diagnosed.

---

## How to file one

Every entry carries all six fields. An entry missing **Reproduce** or **Expected** is not
a report, it is a feeling.

```
### A.N  One-line description
**Observed:**   what happened, with the numbers and the exact invocation
**Expected:**   what the design says should have happened, and WHERE it says so
**Reproduce:**  the command, on a stated commit
**Diagnosis:**  UNDIAGNOSED / mechanism identified / artefact of the model
**Severity:**   does it invalidate a result, mislead a reader, or neither
**Disposition:** open / moved to TECH_DEBT as <id> / closed with reason
```

**Rules earned elsewhere in this project, restated because they apply hardest here:**

- **Rule 8 first.** Check per-seed consistency *before* explaining a cross-arm or
  cross-binary difference. Four attribution passes were once spent explaining noise.
- **Rule 9.** Write down what the anomaly looks like *on disk* before building a check
  for it. An anomaly whose signature is an absence cannot be found by scanning presence.
- **An exact zero is a control-flow claim, not a measurement.** Prove the path is
  reachable before reporting that it produced nothing.
- **Two mechanisms agreeing exactly is not agreement.** It is usually two different
  refusals. This has happened here — see A.2.

---

## Open

### A.1  Four independent mechanisms have empty or near-empty populations
**Observed:** Covariance Intersection 0 firings; Case A 8 attempts in 480 cycles;
the Mahalanobis integrity check 0 eligible cycles of 1431; routing has no route to
select. Each was measured separately and each looked like its own problem.

**Expected:** all four are specified as load-bearing. The docx treats sparse-anchor
resolution, entry fusion and the integrity check as normal-path mechanisms, not corner
cases.

**Reproduce:** `--sparseCaseA=true --tableFuseCI=true` at
`--boxHalf=1000 --nFinders=4 --duration=1200`, commit `26d002e`.

**Diagnosis:** **mechanism identified, and it is ONE cause not four.** Tier 1 blankets
the box: `runFloor` is true on 99.9% of cycles, `static_868` records zero no-fix cycles,
and ~96% of every arm's cycles carry an 868 transmission regardless of active tier
(`TECH_DEBT` K.3). Every mechanism built to handle scarcity therefore never encounters
any. It is not four bugs; it is one deployment property.

**Severity:** does not invalidate anything measured. **Does mislead** — a reader seeing
"implemented" for four mechanisms would reasonably assume they act.

**Disposition:** open. Diagnosed but not resolved; the resolution is a regime where tier 1
does not cover. Tracked as the headline item of `TECH_DEBT` L.2.

### A.2  Two gate forms produced byte-identical output and it read as agreement
**Observed:** `--uwbEvidenceFromNeighbours` produced `arm_summary` byte-identical to the
static-map form on all 8 seeds.

**Expected:** two different predicates, computed from different inputs, should differ
somewhere.

**Diagnosis:** **artefact, and a dangerous one.** Both were permanent refusals arriving by
different reasoning — the live form read a neighbour table belonging to a radio that is
put in deep sleep and cleared every cycle, so it could never say yes. Byte-identity was
two vetoes, not one agreement.

**Severity:** would have invalidated the attribution of a 66 J saving.

**Disposition:** closed — fixed, and the lesson is now a standing rule above.

### A.3  The IMU witness refuses 440 escalations in the full-stack configuration
**Observed:** with `--imuCrossCheck=true` alongside the rest of the stack:
`INTEGRITY: escalations refused -- IMU inconsistency 440, weak corroboration 0`.

**Expected:** in the 8-seed integrity result the IMU witness fired **once** against
corroboration's sixteen, and was described as the quiet witness.

**Reproduce:** the full-stack invocation at commit `26d002e` with `--imuCrossCheck=true`.

**Diagnosis:** **UNDIAGNOSED.** Consistent with `TECH_DEBT` K.9 — the SHS gait calibration
diverges where fixes are sparse — but the jump from 1 to 440 has not been traced, and it
is not known whether the witness is refusing correctly (the filter really is inconsistent)
or diverging and refusing everything.

**Severity:** high if used. A witness that refuses ~everything is a permanent veto on
escalation, which is the same failure shape as A.2.

**Disposition:** open. **Do not enable `--imuCrossCheck` in a demonstration or a
campaign** until diagnosed; corroboration alone is what earned the 8/8 result.

### A.4  Tier 1 out-ranged by tier 2 before the environment correction
**Observed:** at the pre-2026-09-08 default, 2.4 GHz out-ranged 868 MHz against ~27 dB of
link budget in tier 1's favour.

**Diagnosis:** **mechanism identified.** `envFacility`/`vegFacility` placed tiers 2 and 3
in `RURAL_OPEN`/`SPARSE_SHRUBS` while tier 1 sat in `RURAL_FORESTED`/`DENSE_FOREST` — a
leftover from the three-population model, false since the tri-band correction.

**Severity:** invalidated every result in the tree. This is the largest single anomaly the
project has found.

**Disposition:** moved to `TECH_DEBT` K.1/K.2. Default corrected; the regeneration it
forces is open.

### A.6  A session reaches TRACKING having never had a precise fix
**Observed:** config A (`--nGateways=3 --boxHalf=2000 --nFinders=0`) ends at
`final state TRACKING` with `renders HIGH 0` — no UWB fix ever occurred.

**Expected:** the docx FSM reads
`SEARCHING -> LOCATED_COARSE -> LOCATED_PRECISE -> TRACKING`, so TRACKING reads as
"precision achieved and being maintained".

**Reproduce:** that config at commit `ac5766f` with `--sessionLayer=true`.

**Diagnosis:** **mechanism identified, and it is the spec's own FSM.** The path taken was
`SEARCHING -> LOCATED_COARSE -> STALE -> TRACKING`: the session went stale during the 289
unlocalisable cycles and the spec says STALE **recovers to TRACKING** rather than
reopening, deliberately, so a recovered target does not report a second "first fix".
Reaching TRACKING without precision is therefore correct per the FSM and misleading per
the name.

**Severity:** does not invalidate anything. **Misleads in a demonstration** — a viewer
reads TRACKING as "we have a precise lock".

**Disposition:** open. Not a code fix: the FSM is the spec's. Either the state needs a
distinguishing sub-label, or a demo must show the *path* and not only the final state.

### A.7  The case breakdown does not partition the cycles
**Observed:** config A reports D 32 + A 83 + B 0 + C 289 = 404 against 480 cycles.
Config D reports 409 + 0 + 0 + 12 = 421 of 480.

**Expected:** four mutually exclusive cases covering every cycle.

**Diagnosis:** **mechanism identified.** Cycles where the active tier returned one or two
ranges *and the tier-1 floor solved* fall into no bucket: they are not D (the active tier
had fewer than three), not A or B (Case A/B only runs when the floor has failed), and not
C (they were localised). The ~60–76 unaccounted cycles per run are exactly those.

**Severity:** low, but it **must not be presented as a complete breakdown**. The four
numbers are a census of how the fix was obtained, not a partition of the run.

**Disposition:** open. Either add a fifth bucket ("floor solved for an escalated tier") or
label the census as partial wherever it is shown.

---

## Closed

### A.5  Finders converged on a position no packet ever carried
**Observed:** "UWB reachable, energy halved, 109 → 51 J".
**Diagnosis:** artefact. The finders were steering on `policy.EstX()` read straight out of
the simulator — a number no frame delivered. With the position actually travelling over
the air the figure is **80.1 J**, and the 29 J difference is dissemination latency the
artefact hid.
**Severity:** would have been a headline result built on a channel that did not exist.
**Disposition:** closed. Caught only because instrumentation was added asking whether any
finder actually *held* the entry — the answer was 0 of 4. **Nothing in the 15-gate suite
tests for this class of error**, and that gap is `TECH_DEBT` H.3.

### A.8  "C3 identity queries: answered 0" was a property of the counter
**Observed:** every run, in every configuration, reported `answered 0` alongside 18–21
correctly ignored. It was read across several sessions as "the identity query reaches
targets but the matching one never replies".

**Expected:** with `--identityQueryTarget=2` and `--nTags=3`, the peer holding manifest
id 2 should answer.

**Diagnosis:** **artefact.** The counter summed `nodeMac[]` — the MEASURED TAG's three
MACs — only. Manifest id 2 belongs to a PEER tag, so a query addressed to it could only
ever be counted as *ignored* by the measured tag; the answer, if it came, was invisible
by construction. Summing over every MAC gives **answered 9** and **answered 5** in the
same two configurations.

**Severity:** would have been reported as a broken identity-scoped query in a
demonstration, and the mechanism works.

**Disposition:** closed, fixed. The lesson is the recurring one: **an exact zero is a
control-flow claim, not a measurement**, and this one survived multiple readings because
it was accompanied by a plausible non-zero number next to it.
