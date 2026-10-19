# CA-TBL — work packages

Executable task specs. Strategy and justification live in `MASTER_PLAN.md`; this file is what to
do, where, and how to know it worked.

**Read `RULES.md` first.** Every package below assumes its rules — default-off flag, null
control, pre-registration, negative fixture, provenance.

**Line numbers verified against `01c852d` (2026-09-02).** Re-grep before trusting any of them
after a commit touches the file. The symbol names are stable; the line numbers are not.

**Status legend:** `READY` all prerequisites met · `GATED` blocked on a named item ·
`DECISION` blocked on a human answer.

> **⚠ TOPOLOGY, corrected 2026-09-07 (`99fdf1a`) — after these packages were written.**
> Every package below was specified against the legacy model of three **disjoint** anchor
> populations. The system has **one population of tri-band nodes**: every node carries all
> three radios, gateways are the only immobile class, **finders** are mobile tri-band nodes,
> and facility sites are mobility targets hosting **no anchors**. See `RULES.md` and
> `Architecture.md` §3.
>
> All 15 packages are complete (14 done, A8 closed by result — see
> `report/NOT_DONE_2026_09_05.md`), **but every one was measured under `--topology=1`.**
> Treat the numbers in the "how to know it worked" clauses as legacy references, not as
> current expectations. A5 in particular no longer poses a meaningful question — see its
> own note.

---

## The 48–72 hour cut

**What fits, and what does not.** Costs below assume an agent session working the code while
engine runs proceed in parallel on your machine.

### In scope

| Slot | WP | Why it fits |
|---|---|---|
| **Hour 0** | Kick off **A2 stage 1** (160 runs) | Compute only, no code. Start it before anything else so it runs while code work proceeds |
| Day 1 AM | **D-3** CRLB gate off tier 1 | Two edits plus a string fix. It is a live error being reported |
| Day 1 AM | **A6** expose `pcsThresholdDown`, `nDeescalate` | Two `AddValue` calls + null control |
| Day 1 PM → Day 2 | **C1** `ESC_REQUEST`/`ESC_GRANT` on the wire | The single highest-value build. Tests claims 4 and 5, which have never been tested |
| Day 2 (parallel) | **A1** `--dwell` sweep | Compute only, no code |
| Day 3 AM | **C1** null control + re-baseline | `--sendHandshakeFrames=false` must reproduce `baseline_v6` byte-for-byte |
| Day 3 PM | **A2** analysis + **A0** criterion applied | Produces CAL-05 and the branch-point verdict |

### Explicitly out of scope at this budget — do not start these

| WP | Why not |
|---|---|
| **C2** two tags | The refactor is a day; **proving `--nTags=1` byte-identity is the risk**, and it is unbounded. Starting it and not finishing leaves the tree in a state where no result is comparable |
| **C3, C4** | Gated on C2 |
| **C5** hop-provenance | Needs a relayed-ranging error model written and reviewed first. Weeks, not days |
| **A8** weighted PCS | Invalidates the entire results tree. Never a time-boxed task |
| **A5** siting | Gated on A4, and `BuildDeployment()` is hardcoded |
| **A7** sensitivity gate | Small, but the negative fixture is what makes it worth having and that is the slow half. Take it only if C1 lands early |

### The honest trade

You cannot both **build claim 3** and **close the claim-2 question** in 2–3 days. This cut does
**C1 only** from Track C — which is the right single choice, because it is the only Track C item
that tests a claim that already exists rather than adding new capability, and it is the only one
that isolates cleanly behind a flag.

If the goal is instead to **finish the measurement story and write up**, drop C1 and run
A2 + A1 + A0 + D-3 + A6 + A7. That is comfortably 2 days, needs almost no new code, and closes
CAL-03, CAL-04, CAL-05 plus the branch point. Pick one lane — attempting both produces two
half-done things.

---

## Priority order

| # | WP | Type | Cost | Status |
|---|---|---|---|---|
| 1 | **A0** register the joint criterion | Analysis | Hours, no compute | READY |
| 2 | **A2** detect-and-escalate under injection | Compute only | ~160 runs | READY |
| 3 | **D-3** CRLB gate off tier 1 | Code, small | Hours | READY |
| 4 | **A1** `--dwell` sweep at b3000 | Compute only | ~120 runs | READY |
| 5 | **A7** sensitivity-table fidelity check | Code + gate | Days | READY |
| 6 | **A6** expose hysteresis params | Code, trivial | Hours + compute | READY |
| 7 | **C1** `ESC_REQUEST`/`ESC_GRANT` on the wire | Code, moderate | ~1 week | READY |
| 8 | **A3** intra-cycle handshake deferral | Code, small | Days | READY |
| 9 | **A4** tier-1 calibration | Analysis + code | Days | READY |
| 10 | **C2** two tags | Refactor | ~1 week | GATED on C1 landing + re-baseline |
| 11 | **C3** `QUERY`/`RESPONSE` | Code, moderate | ~1 week | GATED on C2 |
| 12 | **C4** multi-target arbitration | Code, moderate | ~1 week | GATED on C2 |
| 13 | **A5** complementary siting | Code + compute | Days | GATED on A4 |
| 14 | **C5** hop-provenance | Protocol + error model | Weeks | GATED on C2 + model review |
| 15 | **A8** weighted/gated PCS | Campaign | Invalidates results tree | DECISION — only if A2 succeeds |

---

## A0 — Register a joint availability/energy criterion

**Why.** The project registered a **cost** criterion for an architecture whose value is
**reliability**, and the two select for opposite conditions: a cost win needs a qualifying
competitor, a reliability win needs the competitor to fail. The screening rule and the verdict
rule carried disjoint cell sets, and all ten adjudicable cells sat in `RURAL_OPEN` — the easiest
environment, where tiering is least needed. At `spacing/r80` = 1.02 the tiered arm meets the
requirement in **82 % more deployments for 27 % more energy**, and M1 scores that FAIL.

**Do.**
1. Write the criterion as a pre-registration in `report/`. Suggested form: a maximum acceptable
   energy multiplier per additional deployment meeting `(A ≥ 0.95, R ≤ 25 m)`. State the
   trade-off numerically **in advance**.
2. Commit it.
3. **Then** apply it to the existing 320-cell grid in `results/p10_grid`.

**Verification.** The commit timestamp must precede the analysis. If the criterion is written
after seeing which cells pass, it is worthless — that is exactly what pre-registration prevents.

**Output.** A verdict for the §4.1 reliability result (`ca_tbl_full` 19/20 vs `static_2p4` 0/20
at 1.56× energy), which currently has none because no reliability criterion was ever registered.

**No code change.**

---

## A2 — Detect-and-escalate under injected fault  ← THE BRANCH POINT

**The harness already exists.** Flags at `scratch/ca_tbl_causal_sim.cc:3345-3364`; bias applied
at `:2496-2510`; fault window computed at `:2338-2339`; parameters threaded through `RunArm` at
`:1859-1861` and the call site at `:3819-3821`.

| flag | meaning |
|---|---|
| `--injectRangeBias` | metres of range bias, deliberately **not** setting the NLOS flag — `suspect` requires all-LOS, so a fault that announces itself is invisible to the gate by construction |
| `--injectFaultAnchors` | `0` = F3 common-mode on every tier-2 link; `1` = F1 single-anchor, the canonical RAIM fault model. Faulted anchor is seed-derived, identical across arms |
| `--injectBiasFraction` | fraction of the run the bias is active, one contiguous window, seed-derived start, identical across arms |

**The gap.** A census of every `run_id.txt` citing `injectRangeBias`:

```
injection x suspectForcesEscalation=false   160 runs   (4 policies x F1/F3 x 20 seeds)
injection x suspectForcesEscalation=true      0 runs
distinct bias magnitudes tested               1        (40 m)
distinct window fractions tested              1        (0.2)
```

So the check is falsified **as rejection alone, at one operating point.** The combined
detect-and-escalate mechanism — the one that moves `cep50M` 92.15 m → 3.10 m under
propagation-derived NLOS — has never been run under injection.

**Do.** No engine change.

```bash
# Stage 1 — the untested cell: 4 policies x 2 fault modes x 20 seeds = 160 runs
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

# Stage 2 — magnitude and window axes at the best policy from stage 1
#   --injectRangeBias={10,20,40,80,160}  with --injectBiasFraction=0.2
#   --injectRangeBias=40                 with --injectBiasFraction={0.05,0.1,0.2,0.4}
```

**Pre-register before stage 1.** Prediction, falsifier, abort condition.

**Verification.**
- Estimator: mean of per-seed differences over 20 seeds, sign test beside it — the same
  estimator `results/abl_clean` used, so the two are comparable.
- Report **F1 and F3 separately.** They are different fault models and the gate's effect is
  already recorded as form-dependent.
- Compute detection **precision/recall against injected ground truth** — this is CAL-05, the
  calibration the thesis specifies and the project has never produced.

**Abort criterion, pre-registered.** If `ca_tbl_full` does not beat `ca_tbl_no_integrity` with
escalation enabled, under the fault the check was designed for, at **more than one bias
magnitude**, claim 2 closes honestly and the thesis takes the Track B framing. A null result here
is a real answer — `Simulation.md` §13.2 registered exactly that in advance.

**Qualification that must travel with any positive result.** The prior the check tests against is
a constant-velocity prior propagated from previous radio fixes, **not** the independent prior the
specification requires. A positive result is evidence about the implemented check, not the
specified one.

---

## D-3 — Take the CRLB gate off tier 1

**Why.** The gate runs on tier 1, whose estimator is RSSI inversion, not time-of-arrival.
`src/lora-physics-engine/model/submodules/lora-ranging-submodule.cc:151` states in its own
comment that applying a ToA Cramér–Rao bound to it is a category error. The tier-1 sigma is
`estD * ln10/(10n) * shadowingStdEff` — proportional to range. The measured tier-1 ratio of 3.319
therefore compares unrelated quantities and has been reported for that tier throughout.

**Do.**
1. `scratch/ca_tbl_causal_sim.cc` CRLB gate (grep `worstCrlbRatio`): skip tier 1 entirely. Tiers
   2 and 3 are ToA estimators and the gate is meaningful there.
2. Fix the reporting defect in the same commit: `worstCrlbRatio` is tracked as a **minimum** and
   tested only `< 0.333`, which is correct by design — only `ratio < 1` is physically impossible
   — but the pass string reads *"CRLB OK (within 3x of the single-shot bound)"*, asserting a
   two-sided property from a one-sided test. Change it to *"no point beats the bound by more than
   3x"*. Add a separate `bestCrlbRatio` tracked as a maximum if the upper side is wanted.
3. Add a tier-1-appropriate self-consistency check in its place, or state in the probe output
   that tier 1 has no bound check and why.

**Verification.** Gate suite green except the two known reds (G3 by design, G11b permanent).

---

## A1 — `--dwell` sweep at b3000

**Why.** Gateway density is **not** the variable that explains the conjunction result.
`b1000/gwGrid=3` and `b3000/gwGrid=9` have identical density (2.25 gw/km²) and differ **8×** in
median error (1.53 m vs 12.05 m). Precision-tier **coverage fraction** tracks it: 11.49 % vs
3.39 %, and a dwell/transit model predicts both to within 1–2 percentage points.

Facility sites sit at `±0.47 × boxHalf` (`scratch/ca_tbl_causal_sim.cc:270-273`, hardcoded), so
inter-site separation scales with the box while anchor rings keep a fixed absolute radius.
Coverage fraction ≈ `dwell / (dwell + transit)`.

**Do.** No code change. `--dwell` exists (`:3274`, default 90 s). Restoring b1000's coverage
fraction at b3000 needs `dwell / (dwell + 1880) ≈ 0.115`, i.e. **dwell ≈ 245 s**.

```bash
for d in 90 150 245 400 800; do
  for s in $(seq 1 20); do
    ./ns3 run "ca_tbl_causal_sim --seed=$s --duration=3600 --boxHalf=3000 --gwGrid=3 \
      --dwell=$d --arms=ca_tbl_full,fixed_threshold,static_2p4,static_868 \
      --trace=true --outdir=results/dwell_b3000/d${d}_seed$s"
  done
done
```

**Verification.** Measured coverage fraction must track `dwell/(dwell+transit)` as it did at both
existing geometries. If it does not, the mechanism is wrong and that is the result.

**Caveat, registered in advance.** Raising dwell and adding facility sites both raise coverage
fraction but are **not the same intervention** — more sites shortens transit, longer dwell
increases stationary time. If the conjunction returns under one and not the other, coverage
fraction is not the whole story. The facility-count sweep is the cleaner test and needs a
`BuildDeployment()` change; this is the cheap one that runs first.

**Output.** Reframes the conjunction claim from *fails at scale* (fatal) to *requires a
precision-tier coverage fraction above a threshold* (a deployment requirement, and a better
result).

---

## A7 — A tier-2 fidelity check that reads the sensitivity tables

**Why.** The six-decibel indexing error (`sf - 7` against tables whose index 0 is SF5) sat on the
tier carrying ~95 % of active cycles and passed every gate, verifier check and validation in the
project. Asked directly: **no check in the project would catch that defect today.** What found it
was an independent closed-form model refusing to reconcile.

The existing fidelity checks — CRLB ratio 1.105, UWB floor 0.154 m, energy to 10 s.f. — verify
**ranging precision and energy accounting**, both downstream of whether a packet was received.

**Do.**
1. Add a gate that reads the sensitivity tables directly and asserts the SF→index mapping against
   the table headers, per tier.
2. Assert monotonicity: sensitivity must improve monotonically with SF.
3. Cross-check the tabulated value at the configured SF against an independent computation from
   `kTB + NF + required SNR`.

**Negative fixture, required.** Re-introduce the `sf - 7` error in a `/tmp` fixture and confirm
the gate fires. A gate that has only ever passed proves nothing.

**Register as G12** in `scripts/catbl_gates.sh`.

**Output.** Lets claim 7 move back from *verified for ranging precision and energy accounting,
unverified for receiver sensitivity* toward an unqualified statement.

---

## A6 — Expose hysteresis parameters

**Why.** `pcsThresholdDown` and `nDeescalate` exist in `CaTblPolicyConfig` but are not registered
with `AddValue`, so the hysteresis band has **never been swept** — despite being CAL-03/CAL-04 in
the thesis register and an explicit design element.

**Do.** Two `cmd.AddValue` calls in the block at `scratch/ca_tbl_causal_sim.cc:3178-3450`,
threaded through `RunArm` alongside the existing policy config. Defaults must reproduce the
current baseline byte-for-byte.

**Verification.** Null control at defaults. Then sweep.

**Output.** CAL-03, CAL-04. A flat response is a reportable result.

---

## C1 — `ESC_REQUEST` / `ESC_GRANT` as real frames

**Why.** The escalation handshake is **billed but never sent**.
`energy.AccountEscalationHandshake(...)` is called at `scratch/ca_tbl_causal_sim.cc:2610` and
`:2720`; no frame exists, so it never contends, never passes CAD, never meets the duty governor
as a frame. The duty-cycle findings are about correctly-sized *accounting*, not contention.

**Current state, verified.** `CA_TBL_MSG_ESC_REQUEST`, `CA_TBL_MSG_ESC_GRANT`,
`CA_TBL_MSG_ENTRY_PUSH`, `CA_TBL_MSG_QUERY`, `CA_TBL_MSG_RESPONSE` appear in
`ca-tbl-header.cc` (name, ACK policy, access category, payload size) and in
`scratch/ca_tbl_packet_validator.cc` (unit tests construct them). They are **never constructed on
the live scenario path.** What is actually transmitted: `CaTblMac::StartHandshake()`
(`ca-tbl-mac.cc:977`) sends `DISCOVERY` then `DIGEST`, or a single `TRIGGERED_UPDATE` on the
receding path at `:988`.

**Second blocker.** There is **no receive-side dispatch on message type.** The MAC counts and
charges per type (`Counters(f.hdr.GetMsgType())`) but implements no request → response semantics.
`ENTRY_PUSH` is described in the `StartHandshake` comment as arriving "as a reply"; no code
generates that reply.

**Do.**
1. `HandleReceive` dispatch keyed on `GetMsgType()` in `ca-tbl-mac.cc`, with per-type reply logic.
2. Anchor-side grant/deny for `ESC_REQUEST` → `ESC_GRANT`.
3. Replace the accounting calls at `ca_tbl_causal_sim.cc:2610` and `:2720` with real frames when
   the flag is on.
4. **`ca-tbl-header.cc` needs no change** — sizes, access categories and ACK policies are already
   defined for all nine types, and the validator covers them 38/38.

**Isolation.** `--sendHandshakeFrames=false` (default) keeps the accounting path and must
reproduce `results/baseline_v6` byte-for-byte.

**Expect every escalating arm to change when the flag is on.** Real frames contend where a charge
did not. Re-baseline before comparing anything downstream.

**Why first in Track C.** It is the only item that tests an **existing** claim — claim 4's custom
handshake and claim 5's packet taxonomy — rather than adding capability, and it is the
cheapest. Best ratio of claim-coverage to work.

---

## A3 — Intra-cycle handshake deferral

**Why.** A tier-1 handoff costs a ranging slot: ToA 56.58 ms arms 2.263 s of off-time at the
2.5 % cap against a 2.5 s cycle, and a handoff needs two transmissions. The earlier proposed fix
(`dt ≥ 3.91 s`) was an **average-rate calculation, not the binding constraint** — pre-emptions
persist at every `dt` because the off-time lockout blocks a handshake attempted at the same cycle
instant.

**Related machinery already exists.** `--deferEscalationOnDutyLimit` (default true, `:3403`) and
`--arbitrateDutyBudget` (default true, `:3415`) — the latter pre-empts the *next* cycle's tier-1
ranging and gives the budget to the handshake. Handled at `:2568-2582` and `:2760-2762`.

**Do.** Add deferral **within** the cycle: schedule the handshake ≥ 2.263 s after that cycle's
ranging rather than pre-empting the next cycle. Derived to eliminate pre-emption at any
`dt ≥ 2.32 s` at no accuracy cost.

**Isolation.** New flag, default off, reproducing current behaviour.

**Verification.** Pre-emption count → 0 at `dt ≥ 2.32 s`; accuracy unchanged within the
paired-seed CI.

**Abort.** If pre-emptions persist after deferral, the lockout model is wrong — restate the
regulatory finding. That is itself a result.

---

## A4 — Tier-1 measurement calibration

**Why.** Above `spacing/r80` ≈ 1.55 no arm meets the requirement, and **96–99.6 % of inadequate
fixes occur on cycles with fewer than three tier-2 anchors in range.** Open question: is tier-1
range uncertainty over-stated relative to realised error? If so, the filter is discounting
exactly the measurements that would carry those cycles, and the upper edge could move **with no
siting change at all.**

**Do.** Compare tier-1 reported sigma against realised error across the density sweep from the
existing traces. Recalibrate if over-stated.

**Verification.** NEES on tier-1-only cycles against the 2-DOF expectation. Baseline for
comparison: in the one passing cell, NEES is 2.95 at 20 seeds against an expectation of 2.0,
median 0.58, 3.3 % of cycles above the 95 % bound, concentrated in UWB.

**Interacts with D-3 and C2 of `MASTER_PLAN.md`** — the RSSI-inversion sigma is proportional to
range by construction, which is both why the CRLB gate does not apply and a candidate reason the
sigma is mis-scaled.

**Ordering, non-negotiable.** A4 runs **before** A5, so that A5 does not answer a question a
calibration fix already answered.

---

## C2 — Two tags

**What is already multi-node.** Every anchor has its own `CaTblMac` on the shared channel — at
`gwGrid=3` that is 9+ MAC instances per tier already contending (anchor MAC setup around
`ca_tbl_causal_sim.cc:2113`). Engines are keyed by node id and expose `RegisterNode(uint32_t,
Vector)` and `UpdateNodePosition(uint32_t, Vector)` (`lora-physics-engine.h:361,364`).
`CaTblMacConfig` already carries `identityClass`.

**What is singular** — five objects inside `RunArm()`:

| object | line |
|---|---|
| `const uint32_t nodeId = 1` | `:1901` (also `:1105` in the probe path) |
| `CaTblEnergyModel energy` | `:1934` |
| `CaTblPolicy policy` | `:2007` |
| `CaTblMac* nodeMac[CA_TBL_TIER_COUNT]` | `:2065` |
| `NodeTrajectory` | grep `NodeTrajectory` |

The cycle loop reads them as free variables throughout.

**Do.** Promote the five into a per-tag struct; loop the cycle body over tags. Engines, channel,
anchors and provider are shared and need no change. **`scratch/ca_tbl_causal_sim.cc` only** —
nothing in `CaTblPolicy`, `CaTblMac` or `CaTblEnergyModel` assumes a single instance.

**The hard part is not the refactor.** `macRng` is a single shared stream created at `:2041` and
seeded at `:2046` (`SetStream(seed * 7919 + 104729)`); every MAC draws from it (`:2083`, `:2113`,
`:2122`, `:2129`). A second tag issues additional draws and shifts the stream for everything
after it.

**Gate — do not proceed past this.** `--nTags=1` must leave the draw order **byte-identical**.
Diff `arm_summary.csv` and all six traces against `results/baseline_v6`. If the loop is structured
so a single tag executes exactly the current call sequence this holds by construction; if the diff
is not clean, the refactor changed something and the diff says so. Fix it before building on top.

---

## C3 — `QUERY` / `RESPONSE`, identity-scoped search

**GATED on C1 (dispatch) and C2 (a second target to scope between).**

**Do.** Identity-scoped flood plus proxy-response state. `identityClass` filtering already exists
in `CaTblMacConfig`.

**Fix in the same package:** nothing sets mobility state, so `m_mobility` is never
`CA_TBL_MOB_RECEDING` and the receding branch at `ca-tbl-mac.cc:988` is dead code. Claim 4 is
currently exercised on its stationary path only.

---

## C4 — Multi-target UWB arbitration

**GATED on C2.**

`scratch/python_handoff/arbitration_harness.py` already scores
`U(r) = λ₁T_deg + λ₂T_batt + λ₃T_stale + λ₄T_query` offline with a calibrated `k`. Nothing
instantiates a second target for it to arbitrate between.

**Do.** Port the scorer anchor-side into `CaTblMac`. Report the **time-to-precision delta between
highest- and lowest-priority targets** under simultaneous contention, plus a fairness delta.

**Output.** CAL-09.

---

## A5 — Complementary siting

**DONE, and SUPERSEDED by the 2026-09-07 topology correction.** A5 asked where a *dedicated
tier-3 anchor population* should sit relative to a *dedicated tier-2 population*. **Neither
population exists.** Tier 2 and tier 3 now live on the same tri-band nodes, so they cannot be
sited apart and the question is void — `--sitingMode` is read only under `--topology=1`.

Its measured finding stands as a legacy result and its *reasoning* survives the correction:
complementary siting forfeited tier 3 outright (coverage 50.2 → 0, all five seeds) because the
nearest rotated site was 1526 m away against a UWB range of tens of metres. **The same
arithmetic is why a fixed gateway cannot serve tier 3 at all, and therefore why finders exist.**

*Original specification, retained for the record:*

**GATED on A4.**

**Do.** Distribute tier-2 anchors rather than ringing four facilities. Requires a
`BuildDeployment()` change — sites are hardcoded at `:270-273`. Run as a **second treatment arm
alongside** the co-located control so siting is not confounded.

**Registered prediction — honour it.** If `static_2p4` gains as much as the tiered arms, this is
a **deployment improvement, not an argument for tiering**, and must be reported as such.

---

## C5 — Hop-provenance

**GATED on C2 *and* on an error-model review that happens before any code.**

**Why the literal `0` is correct and not a stub.** `CaTblMac::NoteNeighbour()`
(`ca-tbl-mac.cc:337`) rejects any frame that has traversed a hop at `:353`, and the guard's
comment calls it *"the whole correctness of the table"*. The reason is an energy defect: a relayed
frame carries the **originator** in `origin_id` — the relay does not rewrite it, because flood
dedup keys on the originator — so admitting it inserts a peer the node may be unable to hear into
the poll set, and every subsequent poll burns a full `rxTimeoutS` against it: **0.93 mJ on UWB,
about three times a successful exchange.** The poll set is one-hop by construction; every
measurement genuinely has `hopsTraversed == 0`. Passing `0` reports the truth.

**Plumbing that already exists, and is misleading because of it.** `CaTblPolicy::Step(...,
uint32_t hopCount, ...)` at `ca-tbl-policy.cc:702`; `ComputePcs` applying the penalty on both
paths at `:659-661` (v2) and `:680-682` (legacy); `CaTblHeader::GetHopsTraversed()`; the MAC
already flooding with dedup, decrementing hop limit and forwarding the count into the PHY.
Seeing only this, "wire it through and set `hopWeight > 0`" looks right. **It is wrong.**

**Two routes, both real work.**
1. **Relayed ranging** — a two-way exchange through an intermediate node, with range and
   uncertainty **composed across hops**. A new protocol and a new error model.
2. **Admit multi-hop neighbours** *and* add per-peer reachability tracking so unreachable peers
   are not polled. The reachability half is the hard part; done wrong it reinstates exactly the
   defect the neighbour table was added to remove.

**Files.** `ca-tbl-mac.cc/.h` (admission, reachability), `ca-tbl-policy.cc/.h` (composing
multi-hop range uncertainty), `ca_tbl_causal_sim.cc` (relays).

**The gate.** Write the relayed-ranging error model — how range uncertainty composes across a hop
— and have it reviewed **before any code**. If that model cannot be written down, C5 is not
buildable and hop-provenance descopes; C1–C4 still land and claim 3 is still substantially built.

**Hard rule.** `hopWeight = 0.0` at `ca-tbl-policy.cc:83` stays 0 until relayed measurements
actually exist. See `RULES.md` §2.

---

## A8 — Weighted or gated PCS

**DECISION — run only if A2 succeeds. Do not start otherwise.**

**Why it is last despite being the most obvious fix.**
1. **The evidence against the concept does not depend on the form.** The legacy score carries no
   information beyond the EKF trace it is computed from (ρ = −0.199 against a monotone control at
   −0.200). PCS v2 — a different functional form, pre-registered before measurement — **fails the
   same test.** Two forms have now failed the same information test.
2. **A fix invalidates the entire results tree.** That is why the saturation defect is recorded
   rather than corrected.

**Three candidate forms, none run:** a weighted sum (this is CAL-01, the LOOCV calibration the
thesis specifies and the project never attempted); a `term1` gate that vetoes when the covariance
term saturates; `min` instead of sum.

**If run:** the control must be a **monotone-of-filter-statistic control**, to catch the case
where a new score merely reproduces the baseline. Score it against the **A0 criterion**, not the
cost criterion.

**Abort.** If a third form fails the monotone control, the confidence-score contribution is
closed. **Do not run a fourth.**

---

## Pre-registration template

Copy to `report/PREREG_<name>.md` and **commit before the first run**.

```markdown
# Pre-registration — <campaign name>
Written <date>, before any cell of this campaign has run.
Source commit: <sha>   Build profile: optimized

## 1. Question
One sentence. What is being decided.

## 2. Population and denominator
Arms, seeds, geometries, environments. The observation window. The rule for
observations truncated by that window.

## 3. Metric and estimator
Exact definition. Paired or unpaired. Bootstrap resamples if any, and at what level
(seed-level, not row-level). State whether it is a ratio of means or a mean of ratios.

## 4. Prediction
What is expected, numerically, with a direction.

## 5. Falsifier
What observation would show the prediction wrong. Must be reachable by this campaign.

## 6. Decision rule
Three outcomes fixed in advance. "No competitor meets the requirement" is
INCONCLUSIVE, not PASS.

## 7. Abort conditions
What stops the campaign early, and what is reported if it does.

## 8. What this campaign cannot establish
Stated now, so it is not claimed later.
```

---

## Definition of done, per package

A package is finished when **all** of:

- [ ] Flag defaults reproduce the prior baseline byte-for-byte (null control run and diffed)
- [ ] Pre-registration committed **before** the first measurement run
- [ ] `run_id.txt` records `dirty 0` for every cited directory
- [ ] Any new gate has a negative fixture and has been **observed to fail** on it
- [ ] `bash scripts/catbl_gates.sh` green except the two known reds (G3 by design, G11b permanent)
- [ ] `python3 scratch/python_handoff/verify_causal_sim.py <outdir>` passes
- [ ] Row counts checked, not just non-empty files
- [ ] Every number in the write-up traces to a file in `results/`
- [ ] A corrections register entry if anything previously stated is now wrong
