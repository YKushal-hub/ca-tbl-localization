# Gate state after the environment correction — 12/15, and why the three reds stay

**Run at `68f1c58`**, seed 7, duration 3600, clean tree.

| gate | before | after | |
|---|---|---|---|
| G1, G2, G3, G6, G7, G8, G9, G10, G12, G13, G14, G15 | PASS | **PASS** | 12 |
| G11 | FAIL | **FAIL** | deliberate, `TECH_DEBT` J.1 |
| **G4** | PASS | **FAIL** | new — and it is a true finding |
| **G5** | PASS | **FAIL** | new — and it fails *because* G4 does |

**The suite went from 14/15 to 12/15 and the change should not be reverted.** A green
gate over wrong physics is the failure mode this project exists to prevent, and the two
new reds are the gate doing its job.

---

## G4 — the default deployment no longer exercises tier 2

The single failing check:

> `trace_static_2p4_seed7.csv`: fix coverage above 50%: only **41/1440 (2.8%)** steps
> produced a fix — this arm's RMSE is dead reckoning, not localisation

That is correct and it is not a regression in the engine. With 2.4 GHz r80 corrected from
1149.5 m to **156.0 m**, the default deployment — `boxHalf=1000`, a 3×3 grid at
0.9·`boxHalf`, so 900 m gateway spacing — puts the walker within tier 2's reach almost
never. The verifier is refusing to certify an RMSE that is dead reckoning, which is
exactly what it is for.

**The defaults it trips over were derived from the split-environment probe and their own
comments now contradict the measurement:**

| default | its comment | measured now |
|---|---|---|
| `boxHalfM = 1000.0` | "868 r80 = 1000 m" | 868 r80 is **1515.3 m** |
| `t2TriadRadiusM = 150.0` | "2p4 r80 ≥ 900 m, so siting-limited not range-limited" | 2p4 r80 is **156.0 m** — range-limited, and 150 m is now *at* the limit rather than far inside it |

**What size would fix it, and why it is not applied here.** Tier 2 needs the walker
within ~156 m of a gateway for most of the run. Gateway spacing is 0.9·`boxHalf` on a 3×3
grid and the walker tours to 0.47·`boxHalf`, so spacing ≲ 2 × 156 = 312 m puts
`boxHalf` ≲ 350 m.

That is not applied in this commit for a stated reason rather than for convenience: K1
measured cost per seed rising steeply as the box shrinks — 924 s/seed at `boxHalf=400`
against 157 s at 2500 — because a smaller box puts every gateway and finder in range of
everything. A default of 350 would make the gate suite roughly five times slower on every
future run, and that is a decision about the project's tooling, not a bug fix to be made
at the end of a session. **Registered as the next item.**

## G5 — a self-test whose healthy fixture is no longer healthy

G5 feeds the verifier a known-good input and asserts it exits 0. Its "known-good" input
is the fresh default run — the same run G4 evaluates. So:

```
G5 FAIL  healthy input passes                exit=1 want=0
G5 ok    missing arm trace is caught         exit=1
G5 ok    summary newer than traces is caught exit=1
G5 ok    summary older than traces is caught exit=1
G5 ok    traces from two runs are caught     exit=1
G5 ok    missing run_id.txt is caught        exit=1
G5 ok    waiver without a stated mode is refused exit=1
G5 FAIL  waiver WITH a stated mode proceeds  exit=1 want=0
```

**Six of eight sub-checks pass, and both failures are the two that require a clean run.**
Every negative fixture still fires. G5 is not independently broken; it inherits G4, and
it goes green the moment the default deployment is resized.

## Why neither is fixed by touching the gate

The verifier **already has** the concept these failures need. For `static_uwb` it emits a
documented finding rather than a failure:

> fix coverage 0/1440 (0.0%) — expected, not a defect: single-band UWB at r80 = 13.5 m
> cannot cover this deployment. Quote `rmseFixedM` for this arm, never `rmseM`

Under the corrected propagation `static_2p4` is in exactly that position: a short-range
single-band control in a deployment sized for tier 1. So the principled change is to make
that rule conditional on the arm's measured r80 against the deployment size, instead of
special-casing UWB.

**That change is not made here**, and the reason is a rule rather than a schedule: rule 6
requires a gate to arrive with its negative fixture in the same commit, and a gate edited
under time pressure to stop it being red is the precise failure mode `RULES.md` §1.6 and
J.1 were written about. The gate is telling the truth. It stays red until the thing it is
complaining about is fixed.

## The precedent this follows

G11 has been red by design since it was written, and `TECH_DEBT` J.1 states why: the
analytic model refusing to reconcile is the only instrument that has ever caught a
receiver-sensitivity error in this project, and making it green in exchange for a green
line would destroy it. **G4 and G5 are now in the same category** — red because they are
measuring something true that has not been fixed yet.

**12/15 with three explained reds is a more honest state than 14/15 was**, because 14/15
included a suite that could not see a 7.4× error in tier 2's range.

---

## Registered next, in order

1. **Resize the default deployment** to one the corrected physics exercises, and update
   the four stale derived defaults (`boxHalfM`, `t2TriadRadiusM`, `uwbTriadRadiusM`,
   `gwGrid`) from a fresh probe rather than from the split-environment one. Clears G4 and
   G5 together. Requires deciding what the gate suite's runtime budget is.
2. **Generalise the verifier's single-band coverage rule** from "UWB is exempt" to "an arm
   whose measured r80 cannot cover this deployment is a documented finding" — with its
   negative fixture, per rule 6.
3. `--escalationRequiresEvidence` to default **true** (`report/K2_K3_RESULTS.md`, K3:
   323.75 J at 20/20 seeds, no significant accuracy change). **Not landed in this session**
   — it is a second reference regeneration on top of an already-red suite, and stacking
   two default changes before the first is settled is how a tree becomes unverifiable.
