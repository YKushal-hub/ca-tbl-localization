# CA-TBL Simulation — Session Handoff v3

**Paste this entire file as your first message in the new chat.** Attach
`finalyearprojectcontext.md` and `LoRa Localization System.md` alongside it.

Supersedes `CA_TBL_HANDOFF_v2.md`, which is now wrong in several load-bearing
places — see §10, because those errors are instructive rather than merely stale.
v2 in turn superseded `CA_TBL_HANDOFF.md`; keep both only for the novelty-analysis
thread, which remains correct.

Written 2026-07-31. Everything below traces to a run in the session that produced
it, to a citation, to arithmetic shown inline, or is explicitly marked unverified.

---

## 0. Your role and the working rules

You are continuing an ns-3 simulation for a final-year Cyber-Physical Systems
project. These rules were each adopted after a specific failure.

1. **Correctness before progress.** Do not build a layer on a layer that fails its
   own checks.
2. **Report negative results, do not tune them away.** Two of the six claims fail,
   by 58x and 26x. Write them up and diagnose them.
3. **Every number traces to a run, a datasheet, a citation, or arithmetic shown in
   the response.** Mark anything unverified as unverified.
4. **Lead with what is wrong.** State defects first when reviewing.
5. **Distinguish spec from invention.** The design document marks each mechanism as
   borrowed or contributed, and that distinction IS the novelty claim.
6. **Build a detector before trusting a result.** Every defect in §3 was found by a
   check added on purpose. §3.6 in particular was found only because a per-cycle
   counter was added specifically to distinguish three competing explanations.
7. **A passing verifier is not a sound result.** This session: a gate passed while
   reading a *stale file from the previous session*, and a "clean" ablation turned
   out to be clean because the treatment never fires.
8. **One change at a time, with its own before/after.** Two of this session's
   diagnoses were only attributable because the change that caused them was
   isolated.
9. **User preferences:** concise, direct, no filler, no praise openers, bullets and
   tables over prose, no visuals unless asked, code without comments unless the
   file's existing style is heavily commented (the CA-TBL files are — match them).

---

## 1. Environment — read before running anything

- Repo: `\\wsl.localhost\ubuntu-22.04\home\kushal\ns\ns-3-dev` (ns-3.47, WSL Ubuntu 22.04).
- **The agent sandbox cannot mount that UNC path.** `mcp__workspace__bash` fails
  with `UNC paths are not supported`. You have **no shell and no Python**.
  Read/Write/Edit/Glob work normally on the repo.
- **`Grep` over the repo root is unreliable** — it silently returns incomplete
  results on this tree (a search for `CaTblPhyProvider` at repo root returned only
  the handoff .md, missing `src/lora-physics-engine/model/ca-tbl-mac.h`, which
  defines it). Scope every grep to a specific directory or file.
- **The user runs all builds and simulations and pastes back output.** Give exact
  copy-pasteable commands in one block. The user's shell is PowerShell driving WSL
  — avoid inline `python3 -c` and nested `awk` quoting; use a script file.
- ns-3 builds *every* file in `scratch/`. One broken scratch file blocks the tree.
- Compiler: `-Wall -Wpedantic -std=c++23`, not `-Werror`.

---

## 2. What the project is

Unchanged from v2 §2. A GPS-denied localization system for alpine/forest
search-and-rescue, three radio tiers, one decision layer.

| Tier | Radio | Role |
|---|---|---|
| 1 | LoRa 868 MHz (SX1262) | wide-area mesh, coarse TDoA/RSSI. Always on. Coverage floor. |
| 2 | LoRa 2.4 GHz (SX1280) | mid-range two-way ToF refinement |
| 3 | UWB (DW3000, Ch9 @ 7.99 GHz) | short-range precision, accuracy ceiling |

### The locked novelty claim — four parts in combination

1. Self-deployable mesh topology
2. Hop-aware confidence degradation feeding the switch decision
3. **A trust/integrity check at the LoRa→UWB switching seam** — the central claim,
   framed as *environmental fault tolerance*, not an adversary
4. Identity-scoped, priority-based multi-target arbitration

**Closest prior art, to cite rather than hide: Poucet** (French National Defense;
LoRa 868 + LoRa 2.4 + UWB + GPS + altimeter, field-validated). Honest framing:
"Poucet proved dual-band LoRa+UWB works in the field; we extend it to a
self-deployable mesh with hop-aware confidence and switching-point integrity."

Explicitly **not** claimed: LoRa+UWB fusion, mesh LoRa localization, threshold
switching, multi-hop uncertainty propagation, Byzantine-robust localization,
identity-based access control.

### The six things the simulation must prove

1. Architectural advantage — hybrid beats single-band
2. Procedure advantage — the handoff is worth it
3. Handoff triggers correctly
4. Custom handshake
5. Specialized packets
6. Real-life physics applied to packets

---

## 3. Defects found and fixed this session

### 3.1 Tier-2 r80 was censored, and censoring printed identically to a measurement

`dists2p4` ended at 900 m with PDR 0.804 and no falloff. `interpRange()` returns
the last swept distance when the threshold is never crossed — and printed it in
exactly the same format as a real interpolated crossing. "Tier-2 r80 = 900 m" was
read and quoted as a measurement for a full session.

**Fixed.** Sweep extended to 6000 m; `interpRange()` now returns a `censored` flag
and a `*** CENSORED` block prints when it is set.

**Measured: tier-2 r80 = 909.5 m, r50 = 1274.3 m.** Not censored.

> Note for the next session: `*** CENSORED` **not** printing on tier 2 is the pass
> condition, not a failure. It fires only when PDR never crosses the threshold
> anywhere in the sweep. This was misread three times.

### 3.2 v2 §6's energy diagnosis was wrong for tier 1

v2 claimed "the energy axis measures anchor count, not radio cost" and tabulated
J-per-anchor across the three tiers. `AccountRangingExchange()` charges the 868
tier **one broadcast, independent of anchor count**
(ca-tbl-energy-model.cc:361–373). Dividing tier-1 energy by 9 anchors is a
meaningless operation, and the near-equal J/anchor across tiers was arithmetic
coincidence. The tiers reach it by unrelated routes:

| tier | mechanism | anchor-count dependence |
|---|---|---|
| 868 | 1 TX, 51.5 ms @ 45 mA = 7.64 mJ/cycle | none |
| 2p4 | ~0.87 mJ per anchor, miss ≈ hit | linear, ~91% of tier total |
| UWB | ~0.99 mJ per miss (94% of it rxTimeout @ 94 mA) | linear, ~100% of tier total |

The real defect was narrower: **tiers 2 and 3 blind-polled unreachable anchors.**

### 3.3 `CaTblPhyProvider` had no real implementation, and the sim never ran the scheduler

`ca_tbl_causal_sim.cc` contained no `Simulator::Run()` at all — it faked time in a
sequential loop and called `ProcessTransmission()` directly. The event-driven MAC
could not have run. The only `CaTblPhyProvider` was `StubPhy` in the packet
validator, which returns a fixed power for every link — which is why the S4
validator measured 1129 collisions and **0 captures**: capture arbitration needs a
power spread to arbitrate over.

**Fixed.** `EnginePhyProvider` in the scenario, three `CaTblChannel`s (one per
tier, because `CaTblMacConfig` carries a single sf/bandwidth pair), one MAC per
anchor, three MACs for the field node, self-rescheduling `BeaconDriver` per anchor,
and per-cycle `Simulator::Stop(dt); Simulator::Run()` slicing.

Captures are now nonzero (~15,000 per arm). **But see §3.11 before quoting any MAC
statistic.**

Three stated limitations: reciprocal links; no anchor↔anchor links (every anchor is
a hidden terminal to every other, so beacon loss is an *upper* bound); hops now
forwarded to the engines.

### 3.4 Ranging poll gated on a one-hop neighbour cache

`CaTblMac` gained a neighbour table. The poll set is now what the node has actually
heard, not the deployment map.

**The `GetHopsTraversed() != 0` guard is load-bearing.** `DISCOVERY` is a flooded
type, so a relayed beacon carries the *originator's* `origin_id`. Admitting it
would readmit exactly the unreachable anchors the table exists to exclude, and the
energy number would look fixed while being wrong in the same direction as before.

Sleeping tiers get `ClearNeighbours()` — a radio in deep sleep hears nothing, so
re-escalation costs a discovery window rather than being instantaneous.

### 3.5 MY DEFECT: search energy charged zero, re-inverting the energy axis

With the poll gated, a UWB radio that is awake and out of coverage issued no poll,
and `AccountCad()` is a no-op on UWB (no CAD primitive on DW3000). It was charged
**exactly zero** on the 1064 of 1440 cycles it spent finding nothing.

`static_uwb` fell from 28.35 J — the most expensive arm — to **0.7618 J**, the
cheapest. That is the same inversion the S2.5 rewrite was written to kill, reached
by a new route. The file's own reading guide says: *"If it is still the cheapest
arm, it is still failing to range."* Verifier check 12 caught it and named the
mechanism.

**Fixed.** `CaTblEnergyModel::AccountListen()` charges at RX current (distinct from
`AccountIdle()` — 94 mA vs 12 mA on DW3000, a factor of 7.8). A tier that is awake
with an empty neighbour table is charged `min(dt, beaconPeriod) * searchDutyFrac`.

**Verification, exact:** `rx_uwb` 660.689074664 − 0.583474664 = **660.105600 J**;
1064 blind cycles × 2.0 s × 94 mA × 3.3 V = **660.1056 J**. Ten significant figures.

### 3.6 THE BIG ONE: no two arms have ever shared a channel realisation

`LoRaDistributionSampler(uint32_t seed)` takes a seed and **never uses it**
(lora-distribution-sampler.cc:8). It creates five `CreateObject<...RandomVariable>()`
objects (lines 9–13) with **no `SetStream()` call anywhere in the file**. Each takes
the next automatic stream index.

`RngSeedManager::SetSeed()` does **not** reset that counter — `ResetNextStreamIndex()`
is a separate call nothing was making (rng-seed-manager.cc:71 vs 104). So arm 6's
engines began where arm 5's stopped, and every arm drew from different substreams.

**Measured:** `ca_tbl_full` and `ca_tbl_no_integrity`, identical EKF state to 15
significant figures and identical poll counts, reached **334 vs 336** engine
evaluations by t=2.5 — the first cycle, before either arm's policy had made a single
decision. At t=7.5 the same six anchors returned 6 decodes on one arm and 2 on the
other, and the arms never re-converged.

This invalidated **every cross-arm comparison in the project's history**, including
the integrity ablation, whose two arms differed by 8.1% RMSE while the check itself
fired zero times.

**Fixed** with `RngSeedManager::ResetNextStreamIndex()` at the top of `RunArm()` and
`RunProbe()`. Post-fix, all six arms consume identically at t=2.5 (`macEvals = 364`),
and the null ablation is byte-identical.

**What this does NOT fix:** the index reset equalises the *starting point*, not the
*consumption rate*. Two arms that genuinely behave differently still read different
draws for the same link at the same time. Nothing short of a position-keyed
pre-generated channel field fixes that. **The 20-seed sweep remains mandatory for
any comparison between behaviourally different arms.**

### 3.7 MY WRONG INTERMEDIATE DIAGNOSIS

Mid-session I told the user "the engines seed a private sampler from
`config.randomSeed`, so all arms read the same stream at different rates." That was
wrong, and comfortable. They read **different substreams entirely**. The correct
reasoning that exposed it: two arms reading one stream from one starting point
diverge only if one consumes a draw the other does not; identical traces through
t=5.0 proved identical consumption; therefore the divergence had to predate any
policy decision — which pointed at construction order, not consumption rate.

**"The streams got out of phase" is a restatement of the symptom, not a cause.**

### 3.8 The CRLB gate followed the extended sweep into the survivorship tail

Extending `dists2p4` to 6000 m moved the gate's reported worst case from 900 m at
PDR 0.804 to **4000 m at PDR ≈ 0**, where mean range error is conditioned on the
handful of trials that decoded — and a link that decodes at the edge of coverage is
one whose shadowing draw was favourable, which also makes it range better.

**Fixed** with a PDR ≥ 0.20 floor on the worst-ratio search. The gate now reports
1.105 at 1600 m (measured 3.272 m vs a 2.962 m bound) — sitting at ~1.0 against the
Cramér-Rao floor, on a sample that supports the claim.

### 3.9 Diagnostic columns documented wrongly

`two_ray_frac` and `nlos_frac` were documented as "fraction of RECEIVED trials".
They are fractions of **all** trials — visible once the 6000 m row reported
`pdr 0.000` alongside `two_ray_frac 0.889`. Comment corrected; code unchanged, so
prior CSVs remain comparable. Note `mean_abs_range_err_m` **is** conditioned on
reception and is therefore survivorship-biased wherever PDR is low.

### 3.10 The verifier read stale traces for a full run

The determinism passes use `--trace=false`, so a command block that omits a
separate traced run leaves the previous session's `trace_*.csv` in place. The
verifier happily reported `fix coverage 125/1440` and `0.164 m over 101 cycles`
from the old build while reading the new `arm_summary.csv` — an internal
contradiction (the arm table implied 135 fixes) that was the only thing that
exposed it. **Always run traced into the verifier's directory immediately before
running the verifier.**

### 3.11 Beacon flooding — the MAC statistics do not mean what they appear to

`mac_tx ≈ 200,000` against an expected beacon count of 9×360 + 12×1800 + 20×1800 =
**60,840**. `CaTblIsFlooded(CA_TBL_MSG_DISCOVERY)` returns true, so `Send()` gives
beacons `hop_limit = 3`. Anchors cannot hear each other, but the **node** hears them
and rebroadcasts, anchors hear the node and rebroadcast again. Every beacon becomes
a small flood through the node.

Consequences, all unresolved:
- The ~40,000 collisions are dominated by flood traffic, not discovery contention.
- The node transmits ~140,000 frames it is charged nothing for (no MAC is given an
  energy model — see §3.12).
- Whether a one-hop "I am here" announcement should be a flooded type at all is a
  design question in the taxonomy. `hop_limit = 1` for DISCOVERY would make these
  numbers measure what claim 6 wants them to measure.

### 3.12 Energy is deliberately NOT routed through the MAC

`CaTblMac` only ever calls `AccountCad()` and `NoteDeferred()`; it charges no TX or
RX joules. `SetEnergyModel()` is left unset on every MAC in the scenario, so all
energy stays in the explicit `AccountRangingExchange` / `AccountEscalationHandshake`
/ `AccountListen` path. This keeps the energy delta attributable to the gating
change. It also means §3.11's flood traffic is free. Both are deliberate; both are
scope limits to state, not bugs.

---

## 4. Current state of the claims

| # | Claim | Status | Blocker |
|---|---|---|---|
| 1 | Hybrid > single-band | **FAILS 58.3x** | `static_2p4` 2.501e-02 vs `ca_tbl_full` 4.288e-04. Mechanism understood — see §6.1. |
| 2 | Handoff worth its complexity | **FAILS 26.1x** | `fixed_threshold` 1.119e-02. Mechanism understood — see §6.2. |
| 3 | Trigger correctness | unknown | `trig_f1` not captured in the final paste. Thresholds still hand-set, no LOOCV. |
| 4 | Custom handshake | wired, half-tested | `StartHandshake()` runs on escalation, but nothing sets mobility state, so the RECEDING branch never executes. |
| 5 | Specialized packets | wired | Payload sizing and access categories now drive real airtime and backoff. See §3.11. |
| 6 | Real physics on packets | wired | Captures nonzero for the first time. Numbers confounded by §3.11. |
| — | **Central integrity claim** | **zero measured effect** | `suspect = 0` on all 1440 cycles. The ablation is now provably byte-identical. |

**The clean ablation is not evidence for the integrity claim.** It is evidence that
the experiment finally runs and that the mechanism it tests currently does nothing.

---

## 5. Verified results — safe to quote, with their caveats

### 5.1 Determinism

Two runs of `--seed=7 --duration=3600 --trace=false` produce byte-identical
`arm_summary.csv`, re-established after the RNG initialisation changed.

### 5.2 Null ablation is exact

`ca_tbl_full` and `ca_tbl_no_integrity` are byte-identical across all 1440 cycles
except the `mahalanobis` trace column: same 4.5627 m RMSE, same 511.166069545 J,
same 193 escalations. This is the correct output of an ablation whose treatment
never fires, and it is the proof that §3.6 is fixed.

### 5.3 Arm table, seed 7, dt = 2.5 s, searchDuty = 1.0, 1440 steps

```
arm                   rmseM     cep50M   cep95M    rmseFixedM  fixCycles  totalJ         acc/J
static_868            157.1536  129.3704 284.7075  157.4041    1433       13.588749504   4.683e-04
static_2p4              2.3576    1.8936   4.2406    2.3576     1440       16.955067096  2.501e-02
static_uwb            116.7271   35.3906 306.0785    2.8090      158      630.509946407  1.359e-05
fixed_threshold         3.3653    2.3605   6.3910    3.3636     1437       26.551725726  1.119e-02
ca_tbl_no_integrity     4.5627    2.8175   9.3735    4.5536     1435      511.166069545  4.288e-04
ca_tbl_full             4.5627    2.8175   9.3735    4.5536     1435      511.166069545  4.288e-04
```

`acc/J = 1/(rmseM * totalJ)`, computed by hand from the two columns. **This is one
seed.** Nothing here is a result until the 20-seed sweep with confidence intervals.

### 5.4 Probe, 2000 trials/point, 64 decorrelation sites

| tier | r80 | r50 | notes |
|---|---|---|---|
| 868 MHz | ~1522 m | ~3217 m | range err ~50% of true distance throughout — tier-1 fixes come from multilateration across 9 gateways, not link ranging |
| 2.4 GHz | **909.5 m** | **1274.3 m** | measured, not censored. New this session. |
| UWB | ~13.5 m | ~28.0 m | near-pure LOS yet a tenth of the 100 m LOS reference spec |

All three tiers pass monotonicity (868 unchanged), shadowing decorrelation (worst
realised/effective sigma 0.90), and the CRLB gate under its new PDR floor.

### 5.5 UWB ranging precision

Sustained-coverage median **0.154 m over 142 cycles** (fix on this cycle and the
previous one), against a probe ranging sigma of ~0.10–0.14 m. Across three runs
under three different channel realisations this figure moved 0.164 → 0.164 → 0.154 m
— stable, and the strongest single measurement in the project.

### 5.6 Channel-redraw sensitivity — a new and important number

`ca_tbl_full`, same seed, same code path, only the stream indices changed:

| | rmseM | totalJ | escalations |
|---|---|---|---|
| pre-reset channel | 4.6996 | 500.400 | 195 |
| post-reset channel | 4.5627 | 511.166 | 193 |
| change | −2.9% | +2.2% | −1.0% |

So single-seed channel sensitivity is on the order of **3% on RMSE** for a
well-covered arm. **But it is far larger for small-n statistics:** `static_uwb`'s
`rmseFixedM` moved 6.37 → 2.81 m (−56%) under the same re-draw, because it is
computed over only ~150 fix cycles and is dominated by a handful of post-outage
reconvergence events.

**Consequence: the verifier's "coverage beats precision" DOCUMENTED FINDING is not
yet quotable.** It compares `static_2p4` 2.36 m against `static_uwb` 2.81 m — a 19%
gap, on a statistic that just moved 56% from a channel re-draw alone. It was a 2.8x
gap last run. Do not put it in the report until the seed sweep bounds it.

### 5.7 The searchDuty bracket

| `searchDuty` | `static_uwb` totalJ |
|---|---|
| 0.0 (free search) | 0.761795195 |
| 1.0 (consistent) | 630.509946407 |

A factor of 828. **99.9% of `static_uwb`'s energy at searchDuty=1.0 is that one
uncited parameter.** Report the bracket, not a point value, until §7.2 is resolved.

### 5.8 S4 MAC validator: 38/38

Unchanged. Caveats from v2 §5.6 still apply, except that "0 captures means the
capture path is untested" is now retired — see §3.3 and §3.11.

---

## 6. Why claims 1 and 2 fail — mechanisms, not suspicions

### 6.1 Claim 1: tier 2 covers the entire deployment box

From the measured r80 of 909.5 m:

- facilities sit at ±0.47 × `boxHalf` = **±470 m**
- worst-case node position is the box corner (1000, 1000)
- distance to nearest facility = √(530² + 530²) = **749.5 m**
- tier-2 anchors ring that facility at 150 m, so worst-case anchor range ≈ 600–900 m

Every point in the box is inside tier-2 r80. `static_2p4` records `fixCycles =
1440/1440`. There is nothing for escalation to add, so a hybrid cannot beat a
single band. **Claim 1 is not failing on the merits — the deployment does not pose
the question.**

This also retires v2's standing hypothesis that the tier-2 triad was undersized.
It is not; `t2Triad = 150 m` sits well inside 909.5 m.

### 6.2 Claim 2: the policy escalates blind and parks on UWB

From the previous run's tier-cycle breakdown (the post-reset run's breakdown was not
captured — re-extract it):

- `fixed_threshold` **never touches UWB**: `cycles_uwb = 0`, `j_uwb = 0.0083 J` (pure sleep)
- `ca_tbl_full` sits on UWB for **1122 of 1440 cycles**, 778 of them blind, at
  0.6204 J each = 482.7 J of the 483.4 J charged to that tier

The trace shows the loop directly: escalate at t=10, arrive with an empty neighbour
table, `cumJ` steps +0.630 J per cycle while `fixTier` falls back to 868 MHz.
195 escalations in 1440 cycles is one every 7.4 cycles: escalate on low PCS → blind
→ `activeDelivered = false` → count toward demotion → come down → PCS still low →
escalate again.

**Root cause: `CaTblPolicy::Step()` reads confidence, geometry and NLOS count and
never reads a joule.** The thesis is "escalate only when position confidence
justifies the energy". The energy half is not implemented, and once search is
charged it is the dominant term.

---

## 7. Blockers, in order

### 7.1 `--boxHalf` is unswept and the default makes claim 1 unaskable

See §6.1. At `boxHalf = 3000` the corner-to-facility distance is 2249 m, well
outside 909.5 m, so tier-2 coverage becomes genuinely spotty. **This is the single
highest-value experiment available and it needs no code change.**

### 7.2 `searchDuty` has no citation

99.9% of `static_uwb`'s energy. Needs a field-measured or datasheet-anchored
receive-duty figure for a DW3000-class device doing discovery, or the claim stays
bracketed. This is the same class of problem `tofLosFloorM` had before v2 §12.

### 7.3 Escalation is not gated on coverage evidence

`BuildDeployment()` **co-locates tier-2 and tier-3 anchors at the same facility
sites, deliberately**. So a populated tier-2 neighbour table is free evidence that
UWB anchors are within a few hundred metres. Gating escalation on
`nodeMac[2P4]->GetNeighbourCount() > 0` converts blind UWB search into targeted UWB
wake — which is the architectural advantage the project is trying to demonstrate,
and it is currently unclaimed. **Own diff, own before/after.**

### 7.4 The integrity check never fires

`suspect = 0` in 1440 cycles. At 3% channel sensitivity and 20 seeds (SE ≈ 0.7%),
an effect needs to exceed roughly 1.5% to be detectable — and a mechanism that fires
zero times produces exactly 0%. **S7 fault injection is not polish; it is the only
route to the central novelty claim.** No number of seeds substitutes for it.

---

## 8. Everything else outstanding

1. **`LoRaDistributionSampler`'s dead `seed` parameter.** It accepts a seed it
   ignores, and `CA_TBL_MAKE_CONFIGS(..., seed, ...)` threads it through the whole
   module on the assumption that it establishes reproducibility. Either wire it
   (`SetStream(base + k)` with per-tier bases spaced ≥5 apart) or delete it.
   Wiring is strictly more robust than `ResetNextStreamIndex()` because it removes
   the dependence on construction order entirely. **Do this after the seed sweep,
   not before — it changes every channel realisation again.**
2. **Two file-scope caches survive across arms**: `g_fadingComponents`
   (lora-fading-submodule.cc:6) and `g_foliageDynamicCache`
   (lora-foliage-submodule.cc:8). The empty null-ablation diff is evidence they are
   **not contaminating this configuration** — arms 5 and 6 inherit different cache
   contents and still produce identical output. But `g_foliageDynamicCache` stores
   a `Time` while `Simulator::Destroy()` resets the clock to zero underneath it, a
   latent hazard whenever foliage dynamics are actually exercised.
   Status: latent and unexercised, not required.
3. **The UWB engine runs the LoRa CSS chain.** `GetSensitivityThreshold(sf, CR)`
   and `ComputeDemodulation()` applied to HRP UWB, which has no chirp to dechirp.
   Pre-existing and structural. It sets the r80 = 13.5 m that drives the entire
   coverage argument.
4. **DISCOVERY is a flooded type** — see §3.11.
5. **`nlosSigmaInflation = 3.0`**, **PCS thresholds `1.55/1.70/2.30`**,
   **`neighbourTtlS`**, **the three beacon periods**, **`searchDuty`**,
   **HIGH/NORMAL/LOW contention windows**, **`slotTimeS`**, **the 6 dB capture
   threshold** — all uncited. Sweep in S7.
6. **`hopWeight = 0`.** Do not claim hop-provenance confidence degradation until S5.
7. **Trigger ground truth is geometric, not error-based.** `shouldUwb := visUwb >= 3`
   asks "could UWB have solved here", which is necessary but not sufficient.
8. **Case A/B/C resolution** is not implemented; below 3 ranges the filter coasts.
9. **MCU energy is not modelled**, only radio.
10. **SF12 is unusable in this simulator** — PDR 0.255–0.355 at 500 m against SF7's
    1.00, almost certainly collision-dominated.
11. **EKF process noise under-specified for long coasting gaps** (v2 §7.2). `Q` does
    not grow with coasting time.
12. **`--vegFacility` / `--envFacility` unswept**, both chosen in the direction that
    helps UWB.

---

## 9. What the verifier can and cannot see

Current state: **72 checks passed, 3 DOCUMENTED FINDINGS, 1 WARNING, 0 FAILURES.**

- **The WARNING is claim 2 failing.** A clean run is not a good result.
- **It reads whichever `trace_*.csv` happens to be on disk.** See §3.10.
- **One DOCUMENTED FINDING is not yet a finding** — the "coverage beats precision"
  comparison rests on a statistic that moved 56% under a channel re-draw. See §5.6.
- **The gate count moved 77 → 76 across this session and nobody established which
  check stopped being counted.** Outstanding.
- The shadowing gate audits one spatial field out of four (v2 §3.6). Necessary, not
  sufficient.

---

## 10. Where v2 is now wrong

Keep v2 for the novelty-verification thread and the Poucet analysis. Do not trust
it on:

- **§3.2's justification of the ranging fix** — the CRLB arithmetic stands, but the
  claim that arms share a channel realisation underlies several of its comparisons
  and is false (§3.6).
- **§5.2's arm table** — void. Predates the MAC wiring, the search charge, and the
  stream-index fix. Every number changed.
- **§5.3's "0.164 m over 101 cycles"** — superseded by 0.154 m over 142 cycles, and
  the 101-cycle figure was quoted from a *stale trace file* (§3.10).
- **§5.4's "most interesting finding"** — the `static_2p4` vs `static_uwb`
  fix-conditional comparison. Still directionally true, but the gap collapsed from
  2.8x to 1.19x under a channel re-draw. Not quotable (§5.6).
- **§5.5's tier-2 row** — "r80 >= 900 m CENSORED" is now measured at 909.5 m.
- **§6 "THE BLOCKING DEFECT"** — wrong for tier 1 (§3.2). The real defect was
  blind-polling on tiers 2 and 3.
- **§7.1** — done. Tier-2 r80 measured.
- **§8.11 "verifier check 12 passes for a partly wrong reason"** — it later failed
  for the right reason (§3.5) and now passes for the right reason.
- **Any claim that the simulation's arms share a channel realisation.** They never
  did (§3.6).
- **The claim that `RunArm()`'s per-arm engine reconstruction guarantees an
  identical channel** — the comment saying so has been rewritten in place.

---

## 11. Citations still needed

Carried from v2 §12: `tofNlosFloorM = 3.6`, `uwbNlosFloorM = 0.25`,
`nlosSigmaInflation = 3.0`, the PCS thresholds, HIGH/NORMAL/LOW contention windows,
`slotTimeS`, the 6 dB capture threshold.

New this session: **`searchDuty`** (§7.2, most load-bearing), `neighbourTtlS`,
`beacon868S` / `beacon2p4S` / `beaconUwbS`, the CRLB gate's PDR floor of 0.20.

Cited and settled (v2 §12): `tofLosFloorM = 3.0` (Zhou et al., arXiv:2509.23125),
`uwbLosFloorM = 0.10` (Benouakta et al., Electronics 2023),
`uwbPreambleSymbols = 1024` (DW3000 Family User Manual).

---

## 12. Command reference

```bash
cd ~/ns/ns-3-dev
./ns3 build 2>&1 | tail -40

# S4 MAC unit tests -- deterministic, ~1 s.
mkdir -p results/s4
./ns3 run "ca_tbl_packet_validator --outdir=results/s4" 2>&1 | tail -80

# Geometry + physics pre-flight. Require monotonicity, shadowing decorrelation and
# CRLB OK on all three tiers, and NO "*** CENSORED" block on any tier.
mkdir -p results/causal
./ns3 run "ca_tbl_causal_sim --probe=true --outdir=results/causal" 2>&1 | tail -120

# Determinism gate. IDENTICAL flags on both runs.
mkdir -p results/det_a results/det_b
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/det_a --trace=false" 2>&1 | tail -3
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/det_b --trace=false" 2>&1 | tail -3
diff results/det_a/arm_summary.csv results/det_b/arm_summary.csv && echo DETERMINISTIC

# Traced run IMMEDIATELY before the verifier -- see section 3.10.
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal" 2>&1 | tail -30
python3 scratch/python_handoff/verify_causal_sim.py results/causal

# Null-ablation control. Must be empty while suspect==0.
diff <(cut -d, -f1,3-15,17-37 results/causal/trace_ca_tbl_full_seed7.csv) \
     <(cut -d, -f1,3-15,17-37 results/causal/trace_ca_tbl_no_integrity_seed7.csv) \
  && echo "ABLATION CLEAN"

# The two experiments that matter next.
./ns3 run "ca_tbl_causal_sim --seedStart=1 --seedCount=20 --duration=3600 --outdir=results/sweep20 --trace=false"
./ns3 run "ca_tbl_causal_sim --seed=7 --boxHalf=3000 --duration=3600 --outdir=results/box3000"

# searchDuty bracket endpoints.
./ns3 run "ca_tbl_causal_sim --seed=7 --searchDuty=0.0 --duration=3600 --outdir=results/causal_nosearch --trace=false"
```

Knobs: `--env --envFacility --vegWide --vegFacility --boxHalf --uwbTriad --t2Triad
--uwbAnchors --t2Anchors --speed --dwell --dt --duration --seed --seedStart
--seedCount --trace --probe --outdir --beacon868 --beacon2p4 --beaconUwb
--nbrTtlMult --searchDuty`

Defaults: `boxHalf=1000`, `uwbTriad=10`, `t2Triad=150`, `uwbAnchors=5`,
`t2Anchors=3`, `env=1`, `envFacility=0`, `vegWide=3`, `vegFacility=1`,
`duration=3600`, `dt=2.5`, `beacon868=10.0`, `beacon2p4=2.0`, `beaconUwb=2.0`,
`nbrTtlMult=3.0`, `searchDuty=1.0`.

---

## 13. Trace columns

`t, arm, seed, activeTier, nextTier, fixTier, usedFloor, trueX, trueY, estX, estY,
errM, pcs, gdop, traceSigma, mahalanobis, nAnchors, nNlos, hadFix, escalated,
deescalated, suspect, rejected, vis868, vis2p4, visUwb, shouldUwb, cumJ, nbr868,
nbr2p4, nbrUwb, polled, blind, directCalls, macEvals, estXhp, estYhp`

Fields 34–37 are the divergence instrument from §3.6. `directCalls` is cumulative
`ProcessTransmission()` calls from the ranging poll; `macEvals` is cumulative calls
from the MAC/channel layer. **There is no t=0 row** — the loop advances the
scheduler before reading the clock, so line 2 of the file is t=dt.

`arm_summary.csv` additionally carries per-tier `tx_/rx_/cad_/idle_/sleep_` energy
buckets, `meanNbr_*`, `blindCycles`, `nbrInsertions`, and `mac_tx/collisions/
captures/deliveries`.

---

## 14. First message to send

> Read `CA_TBL_HANDOFF_v3.md`, then `CA_TBL_SIM_AUDIT.md` and `simstatus.md` Part 6.
> Note that `CA_TBL_S1_S2_README.md` is current only through S2.5 and knows nothing
> about S4 or anything in v3 §3.
>
> Two experiments, in order:
> (a) `--boxHalf=3000`. Tier-2 r80 is 909.5 m and the facilities sit at ±470 m, so
>     at the default box tier 2 covers everything and claim 1 has no question to
>     answer. See §6.1.
> (b) The 20-seed sweep with confidence intervals. Nothing in §5.3 is a result
>     until it exists, and §5.6 shows small-n statistics moving 56% on a channel
>     re-draw.
>
> I have no shell — give me exact commands and I will run them and paste the
> output. My shell is PowerShell driving WSL, so avoid inline `python3 -c` and
> nested `awk` quoting.
>
> Report negative results rather than tuning them away. Claims 1 and 2 currently
> fail by 58x and 26x; that is expected and I want them diagnosed, not fixed by
> parameter choice. The integrity check fires zero times, so do not read the clean
> ablation as support for it.
