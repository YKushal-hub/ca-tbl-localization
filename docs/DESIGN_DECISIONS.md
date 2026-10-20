# Design decisions not specified by the project brief

**2026-09-07.** The owner's instruction: *every node carries all three bands; there is a
lost node, fixed gateways, and finder nodes; only gateways are immobile; everything else
follows from that — and **any design decision not in that brief has to be documented**.*

This file is that record. Each entry states the choice, what it was chosen over, and why
— so a reader can disagree with a specific decision instead of with the whole model.

---

## 0. The error this corrects, first

The simulator modelled **three disjoint anchor populations**, one per tier, in different
places: tier-1 gateways on a `gwGrid × gwGrid` array, tier-2 anchors ringed at 150 m
around four facility sites, tier-3 anchors ringed at 10 m around the same sites.
`Architecture.md` §140–143 specified it that way too, so this was not code drifting from
a spec — **the spec had it wrong as well.**

The system has **one population of tri-band nodes**. A tier is a choice of *which radio
to use*, not of *whom to talk to*.

**Why it mattered, concretely.** Under the old model the lost node reached UWB by
wandering near a facility. Under the corrected one, UWB (tens of metres) barely reaches
from a fixed gateway at all — measured mean UWB neighbours **0.01** with no finders. The
old topology manufactured tier-3 coverage that the architecture is supposed to *earn*,
and in doing so it hid the mechanism the whole design rests on: a finder carries tier 3
to the casualty.

**Blast radius, stated plainly.** This changes the deployment, so it changes every
result. `baseline_v7` does not reproduce under the corrected topology and is not
supposed to. The legacy model is retained behind `--topology=1` **only** so results
published before this date remain reproducible for the record; it does not describe the
system and is not a configuration anyone should run for new work.

---

## 1. Finder mobility — converge on the estimate

**Decision.** `--finderMode=1` (default): finders move toward the system's **current
position estimate** of the lost node at walking speed, standing off 8 m once inside it.

**Over:** touring the facility sites like any other field node (`--finderMode=0`), which
is what the code did when finders were first added and which produced almost no tier-3
contact (mean UWB neighbours 0.15 against 0.78 converging).

**Why.** The purpose of localising a casualty is to send someone there. A finder that
wanders is not a searcher. Critically it converges on the **estimate**, not the true
position — nobody knows the true position — so a bad fix sends searchers to the wrong
place and the cost of poor localisation appears as distance walked rather than only as a
number in a table.

**Not in the brief.** The brief says finders move; it does not say how. This is the
strongest modelling assumption in the corrected system and the one most worth arguing
with.

**Known limitation.** All finders converge on the same point. Real search teams spread
to cover an uncertainty region, so this is optimistic about contact and pessimistic about
area coverage. A dispersal model keyed to the covariance is the obvious next step.

## 2. The 8 m stand-off

**Decision.** A converging finder stops 8 m from the estimate.

**Why.** Without it the finder oscillates across the estimate every cycle, which both
looks wrong and repeatedly breaks the short-range link the approach exists to establish.

**Not in the brief. The constant is ours** and is not calibrated against anything.

## 3. Finders take the 2.5 % duty cap, not 10 %

**Decision.** `isAccessPoint = false` for finders.

**Why.** The context document is explicit that under G.S.R. 853(E) *"gateways/anchors
(your network access points) get a 10 % duty cycle budget, field nodes get 2.5 %."* A
finder is a person carrying a radio, not installed infrastructure. Getting this backwards
would hand searchers four times the airtime they are entitled to.

**Derived from the brief rather than chosen**, but recorded because it is a judgement
about which category a finder falls into.

## 4. Finders are modelled as mobile gateways

**Decision.** A finder is registered on all three tiers exactly like a gateway, beacons
on all three, and is discovered and ranged through the ordinary neighbour table. The only
difference from a gateway is that its position changes.

**Why.** It requires no special case in the ranging, energy or MAC paths, so a finder
cannot accidentally get accounting a gateway would not. It also matches the brief: the
only stated difference between the node classes is mobility.

**Consequence not in the brief.** The lost node ranges *to* finders. Finder-initiated
ranging, and finders relaying to gateways, are not modelled.

## 5. Gateway placement and count

**Decision (2026-10-03).** The default deployment is unchanged: the `gwGrid × gwGrid`
array, 3×3 over a 2 km box. Every result in `results/` that does not say otherwise used it.
The **spec configuration** (`--specProfile`) uses **eight gateways on a ring** at 0.6 of the
half-width of a 10 km square (`--nGateways=8`, `--boxHalf=5000`). That is GEO-01 stage 2's
registered **fallback**: no candidate geometry met the SAR criterion.

**The brief says 3 gateways, 5 field nodes, 5 km radius.** Three gateways at 5 km cannot
carry a SAR fix on tier 1 in this model. That is a finding, below, not a placement choice.

**Anyone quoting a gateway count should read it off `deployment.json`, not off the brief.**

### How the shape was chosen: GEO-01 (`docs/SPEC_CONFORMANCE_PLAN.md` §3.4)

The owner's decision 5 (2026-09-30): the least infrastructure that meets a registered SAR
criterion, with gateways only where a vehicle can reach.
- **The shapes** exist since 2026-10-02 (MS-2, gate G38): `--gwLayout=perimeter|cluster|
  star|lshape|file`, beside the ring (`--nGateways --gwRingFrac`), the arc (`--gwArcDeg
  --gwArcCentreDeg`), the ridge (`--gwRidge`) and the grid.
- **Stage 1** (`report/GEO01_STAGE1_RESULTS.md`): a geometric screen of every shape at 3-8
  gateways. Tier 1 alone covers a mean P(≥3) of at most 0.71 of the 10 km square at N ≤ 8.
  It pruned ten candidates for simulation.
- **Stage 2** (`report/PREREG_GEO01.md`, registered before any cell;
  `report/GEO01_RESULTS.md`): ten candidates × 10 seeds × 1 800 s, the preset minus its
  geometry. The criterion was availability ≥ 0.95 and median time to a 100 m fix ≤ 600 s.
  - No candidate met it. Availability was at most 0.329, on `ca_tbl_full`.
  - The registered fallback, the highest mean availability, is N8_ring; the runner-up is
    N8_arc240.
  - Three gateways gave 0.22-0.25.
- **Stage 3** (inside CAMP-SPEC, `report/CAMP_SPEC_RESULTS.md` §2): the two re-run under
  the full preset. The order held: 0.333 against 0.322.

What closing the gap would take is an owner decision (`HANDOFF_2026_10_03.md` §3 item 6):
more infrastructure, or the subjects' own mesh carrying fixes.

### The earlier comparison, filed here (`HANDOFF_2026_09_09.md` §4, seed 7)

At `boxHalf=2000`, before the spec scale was tested:
- **The interior grid** gave CEP50 28 m. The owner rejected it on 2026-09-09 as unrealistic
  for SAR: "no SAR team carries a repeater into the middle".
- **The ridge** gave CEP50 1 932 m: a line of gateways is degenerate for multilateration.
- **The arc** was then built as the access-route deployment. NC2 uses five gateways on a
  140° arc centred west.

These are single-seed figures from the legacy runs, kept as the record of why the grid and
the ridge were ruled out, not as results.

**2026-10-04: the preset takes the spec's three gateways** (owner call delegated, §9):
`--nGateways=3 --gwLayout=star`, GEO-01 stage 2's best N = 3 layout (availability 0.245,
against N8_ring's 0.329). N8_ring was a fallback for a criterion of ours that no geometry
met; the spec's deployment is three gateways at 5 km. N8_ring stays reachable as a
sensitivity geometry.

## 6. Facilities remain, as places rather than infrastructure

**Decision.** The four facility sites still exist and the lost node still dwells at them,
but they no longer host anchors.

**Why.** They were doing two jobs: mobility targets and anchor sites. The second was part
of the error. The first is a reasonable model of a search area with huts, gates and
checkpoints, and removing it would change the mobility as well as the topology, confounding
two changes in one step.

## 7. Node id ranges

Gateways `101+`, lost node `1`, peer/relay tags `2+`, finders `501+`. Arbitrary, but
fixed, so a reader can identify a node class from its id alone in the traces and in the
packet log.

---

## Still unmodelled, and named so the gap is visible

- **Gateways as edge servers — modelled, and the default since 2026-09-29.** The brief has
  gateways performing EKF fusion and NLOS/hop computation rather than the
  battery-constrained field node. `--gatewayEdgeServer` (default true) bills the field node
  one tier-1 broadcast and one downlink per fix instead of a reply per gateway; the filter
  is the same, so at defaults only energy moves (`docs/TECH_DEBT.md` L.2.3). The uplink of
  a tier-2 or tier-3 range to the gateway is billed since 2026-10-02 under
  `--edgeUplinkRanges` (SY-1, in the preset); by default it is still not.
- **Finder-to-gateway relaying.** By default finders are ranging targets only. Under
  `--meshLinks=all` (NW-0, 2026-10-03) every radio pair can exchange frames, finders
  included, and a RESPONSE travels back hop by hop (`--reversePath --reversePathHop`, NW-8);
  both are in the preset.
- **A still lost node.** Supported via `--speed=0`, not yet exercised as a scenario.
- **5 km radius.** Tested since 2026-10-03: GEO-01 stage 2 and CAMP-SPEC ran the preset at
  `boxHalf=5000` (section 5). Earlier, K1c ran at `boxHalf=2500`, K10 at 6000, and the L.2
  coverage probe at 3000 and 5000.

---

## 8. The SHS inertial witness is ported but NOT usable, and is off by default

**2026-09-07.** The step-and-heading system from `IMU_SHS_V1.1` is now on main: 17 files,
builds, runs, and is wired as the inertial witness behind `--imuCrossCheck` (default
**false**). It replaced a stub that modelled dead reckoning as a scale error on distance
— a strapdown error model, which the SHS header explicitly says does not apply to it.

The consistency test was rebuilt on the right error model at the same time: an SHS
produces an absolute position whose drift is bounded by **time since the last trusted
fix**, not by distance walked, so the tolerance grows with unaided running time. The
engine also takes the CLEAN/SUSPECT verdict back through `OnTrustedFix`, so a fix the
check calls suspect cannot launder itself into the one estimate that was independent
of it.

### It does not track well enough to be used, and the fault is the driver

Measured, seed 7, 600 s: the engine ends **330 m** from truth.

Two bugs of mine were found and fixed on the way:

- **Cadence was fixed at 1.8 Hz**, so the distance the engine accumulated was
  proportional to elapsed time rather than to speed — and the node dwells for long
  stretches. Cadence now derives from speed through the Weinberg step length.
- **`SetInitialState` was seeded with heading zero**, asserting the node faced +x.
  That alone was the difference between 1023 m and 330 m of final error. The seed is
  now deferred to the first cycle in which the node actually moves, which is the first
  moment its heading is defined at all.

### What remains is a model mismatch, not a bug

The scenario's walker turns **instantaneously** at waypoints and reports position once
per 2.5 s cycle. A step-and-heading system estimates heading by integrating yaw rate,
and a step-function turn smeared into a 2.5 s average is not something it can follow.
The engine's own documentation notes that heading is only re-derived when two fixes
bracket a stretch walked approximately straight, which such a route rarely provides.

Fixing it properly means one of: a mobility model with finite turn rates, a higher
position-report rate into the driver, or driving the engine from a continuous
trajectory rather than from waypoint samples. None is a small change and none was
attempted.

### Consequence for the integrity check

The result in `report/INTEGRITY_CHECK_RESULTS.md` — 8/8 seeds on energy, 7/8 on
accuracy — was produced with **corroboration alone**, and re-measuring after this port
gives identical numbers. The inertial witness contributes nothing at present and is off.

**Any claim that this system currently performs IMU cross-validation would be false.**
That is written here rather than left for a reader to infer from the existence of a flag.

## 9. The owner's open calls, decided against the spec (2026-10-04, delegated)

On 2026-10-04 the owner handed every open call in `HANDOFF_2026_10_04.md` §3 to whatever
fits the spec, `finalyearprojectcontext.docx`, better. Each is decided here against the
section it answers. Line numbers are of the text extracted from the docx. A call that
moves results goes through its own commit, gate and, where a registered rule exists, its
own registered campaign. None of these decisions overrides a registered adoption rule.

| call | decision | spec basis | how it is carried out |
|---|---|---|---|
| N.14, the fuse | **Neither "adopt anyway" nor nothing: build the narrower variant, fuse only when the floor is short, and let a registered campaign (FUSE-02) decide it by FUSE-01's rule.** | Physical §2.3: one EKF corrected by a fix from any tier, with a tier-specific R, so "higher tiers tighten the estimate more per fix". A UWB range thrown away is against that. §5 Evaluation reports CEP50/95, so a 13.7 % CEP95 cost cannot be waved through. Adopting against FUSE-01b's registered rule would break rule 13. | A flag value, its gate, PREREG FUSE-02 on the finished preset, the campaign, adoption by rule. |
| Pre-N.21 preset campaigns (GEO-01 stage 2, CAL-02, CAMP-SPEC, FUSE-01, FUSE-01b) | **Not re-registered one by one. One final registered campaign on the finished preset supersedes their preset numbers; they stay as dated history.** | Phase 5 asks for one comparison table: this system against fixed-threshold switching, Poucet-style fusion and single-hop confidence, on error against hop count, PCS-error correlation, and switch-decision accuracy under induced degradation. | The final campaign, registered once the preset stops moving. |
| `--halfDuplex`, N.22, `--halfDuplexChip` | **All three into the preset: `--halfDuplex=2`, `--halfDuplexChip=true`, `--collisionBothLost=true`.** | §1.1-1.2 put 868 MHz and 2.4 GHz on one LR1121 die, and the final plan says "one board, two radio ICs". A LoRa transceiver cannot transmit and receive at once, and the capture rule (MAC §3, §6) applies to both frames of a collision. These are fidelity to the named hardware, like N.21's oracle closure, not tuning. | One P6 commit, G24 re-run. Every earlier preset number is superseded by the final campaign. |
| N.4, a finder that never hears its subject | **Neither fallback. Closed: the spec's own path already resolves it.** | Transport §2 and §4: the rescuer's handheld opens a session and issues an identity-scoped QUERY; Network §4: any node holding a fresh entry answers by proxy. The finder steers on what its own query returns. | Measured under the preset (seed 7, 900 s): finders blind 1-10 of 360 cycles. With the finders' sessions off, finders 503 and 504 are blind for all 360 cycles; with digest gossip off, 1-38. Recorded in TECH_DEBT N.4. |
| Row 56, the gateway count | **The preset takes the spec's deployment, 3 gateways, in GEO-01's best N = 3 layout (`N3_star`). `N8_ring` stays as a named sensitivity geometry.** | The final audit (lines 852-856): "3 gateways, 5 field nodes, 5 km radius", and "most fixes will fall into Case B or Case C ... the framework was built for your real deployment". The availability ≥ 0.95 criterion that made `N8_ring` the fallback was ours, and no geometry met it. | One P6 commit, G24 re-run. The PCS calibration is re-fitted on it (below). |
| Row 6, the UWB CIR NLOS score | **Stays PARTIAL in simulation. No obstruction model of our own is built to fill the matrix.** | §1.3 takes the score from the DW3000's CIR, "cited from UWB ranging literature rather than invented", and validates it on the bench (Phase 0, Phase 2). The cited 802.15.4a model does not fill the matrix (UWB_NLOS_RESULTS). An uncited model built until it does would manufacture the result, the failure the topology correction was about. | No code. The CIR score is a hardware-validation item. |
| Row 17, PCS term 1 | **Kept DIVERGENT, as decided.** | §2.5 writes 1/trace(Σ); the bounded monotone transform keeps its order and keeps the byte. | No change. |
| The PCS calibration | **Re-fitted on the finished preset by CAL-02's registered procedure (CAL-03), because the geometry it was fitted on is no longer the preset's.** | Final audit, Gap B: weights by leave-one-out on the deployment's own data. | PREREG CAL-03 after FUSE-02, then the campaign. |
| J.2, tier-1 TDoA as the binary default | **Stays off by default; stays on in the preset.** | §1.1 uses TDOA "where three or more anchors receive the same transmission", with GPS-PPS anchor sync (line 854). The preset is the spec configuration; the binary default is the null-control baseline (`RULES.md` §4). TDOA-01's overconfident tail is reported, not hidden. | No change. |
| SAR availability at 5 km | **No availability criterion is applied; availability is reported as measured.** | The spec sets none. The 0.95 was ours. | The final campaign reports it. |
| How the novelty claims are worded | **Not this repository's to write.** | The spec's own wording (line 272) is the assembly-level claim `RULES.md` §9 already holds. | Thesis-side. |
| N.16, I.1, I.6 | **Carried into the final campaign:** N.16 as a reported finding, I.6 by the campaign itself, I.1 as its integrity-check precision and recall. | Phase 5 metrics. | The final campaign. |

**2026-10-08, the owner's calls (direct, not delegated).**

| call | decision | how it is carried out |
|---|---|---|
| FIXES-N26 | **Adopted into the preset by owner override of the registered rule.** FIXES-N26's rule said DO NOT ADOPT on one term: subject energy at 5 km rose from 12.37 to 16.69 J. The owner took the fixes for what the same registered campaign measured: availability +0.111 / +0.139 / +0.133 (5 km, endgame, N8_ring), 5 km tag 0 CEP95 −151.8 m, and UWB-delivered fixes on 20 of 20 endgame seeds. | `--specProfile` gains `--loraDualBase`, `--uwbCrossTierPoll`, `--tdoaInnovationGate`, `--mahalUseR` and `--integrityChi2=1992.9`. The preset alone reproduces FIXES-N26's registered ALL cell byte for byte. G24 re-run. **This is the one place a registered adoption rule is overridden, and it is the owner's.** |
| Row 17, PCS term 1 | **The literal 1/trace(Σ) is built behind `--pcsTerm1Inverse` (G90); the preset keeps the bounded form.** | Switching the preset would need the PCS re-calibrated on the literal term, which this call did not ask for. |
| Row 6 | **Unchanged: stays PARTIAL, as decided on 2026-10-04.** | No code. |

## 10. The preset adopts the master-plan-v3 fixes (owner decision, 2026-10-17)

`--specProfile` now also sets `--rngKeyed --rangeErrorNoLinkLeak --rssiRangeFromShadowing --nlosRangeBias
--rangeReplyPosition --acquireParticles --foliageDepthClamp`. This is the set V8-REGEN measured
(report/V8_REGEN_RESULTS.md), plus the B3f clamp every V-stage run already used.

**Chosen over** keeping them opt-in. The owner took them after V8: the endgame `ca_tbl_full` CEP95 went 12.6 → 5.3 m,
the static lost node 48.4 → 20.2 m, and at 5 km the reply positions were worth 289 m. Three of the seven are
physics-correctness fixes rather than improvements: the leak (N.55), the realistic 868 MHz error (S1) and NLOS range
bias.

**Not adopted:**
- `--rangeBiasStates` (V3: accuracy cost);
- `--c2bEscalation` (V5: NOT SUPPORTED);
- `--dualBaseOnDemand` / `--onDemandCalibrate` (V6: nothing to save);
- `--nlos3gppRMa` (V2a-2: being measured; adoption is the owner's call after its read-out).

**Reproducing earlier results:** every result before this decision reproduces under `--specProfile` with the seven
flags given explicitly as false. Verified: a 600 s, 4-arm preset run is byte-identical to the B4 binary's in all 224
simulation outputs; only `spec_profile.txt` differs, because it lists the preset. Known consequence: the 8-gateway ring
reads worse under the realistic 868 MHz error (V8 R8: 72 → 215 m). That is the physics being truer, not a regression.
(2026-10-17 correction: an earlier note here and at TECH_DEBT 3.2 said `relayChain` was inert. It is active on the
preset's subject path; see TECH_DEBT O.1.)


## 11. Tier 2 is the SX1280, as two separate chips (owner decision 2026-10-16, applied 2026-10-17)

The preset's `--radioBasis` moves from `lr1121` (tiers 1 and 2 on one LR1121 die) to `sx`: an SX1262 at 868 MHz and an
SX1280 at 2.4 GHz, the documented ranging chip (POUCET p. 4, Wolf p. 2). `--halfDuplexChip` moves to false: the two
bands no longer share a die. Each transceiver's own half-duplex limit (`--halfDuplex=2`) is unchanged. Earlier
results reproduce with `--radioBasis=lr1121 --halfDuplexChip=true`; NC10 pins both.

## 12. What counts as a fix (owner decision 2026-10-17)

A cycle is a fix cycle only when the tracking filter (EKF) took a range update that cycle (`hadFix`). Cycles in which
`--acquireParticles`' particle cloud carries the estimate are not fix cycles, although they have an estimate with an
honest covariance. Availability under `--acquireParticles` therefore reads lower than its estimate coverage; V4 §2
gives the size (`static_2p4` 5 km: 0.064 → 0.034).

## 13. The preset adopts B7 `--fadingPerBand` and B1 `--relayNisInflate` (2026-10-17, UPG-SCOPE)
Evidence: `report/UPG_SCOPE_RESULTS.md` (exploratory; 20 paired seeds, configs A and B).

**Adopted:**
- **B7 `--fadingPerBand`** removes a modelling defect: 868 MHz and 2.4 GHz links between the same node ids shared one
  fading process. It does not move accuracy: B −0.4 [−0.9, +0.1] m on 5.6 m; A −4.6 m on 333 m (interval spans 0).
  Energy is 2–3 % lower.
- **B1 `--relayNisInflate`** inflates a relayed range's covariance by the relay's own NIS mean, the dominant
  overconfident class (B1, E[z²] 12–37). Coverage rises (A 0.577 → 0.587, B 0.858 → 0.873) at no accuracy cost.
  Endgame energy is 5 % lower.

**Not adopted:**
- C1 `--acquireNegInfo` and C2 `--tofRecal`: no supported gain, and lower 5 km coverage.
- C3 `--hold24AfterK`: −10 % endgame energy with post-acquisition accuracy unchanged, but its registered prediction
  (per-run CEP95 within +20 %) missed at +21 %. It is available as an energy mode.

**C5 `--tofCalibrate`** belongs with `--tofErrorModel=wolf` (calibrated hardware), not with the preset.

**Reproducing earlier preset results:** add `--fadingPerBand=false --relayNisInflate=false`. NC10, G63, G87, G89 and
G147's base run pin both.

## 14. The preset adopts `--rssiSigmaLogNormal` (2026-10-17, SIG868)
The 868 MHz range σ given to the filter becomes the exact SD of the log-normal RSSI range error, not the first-order
one. The registered adoption rule fired (S1, S3 and S4 held; `report/SIG868_RESULTS.md`):
- calibrated-Wolf endgame hybrid CEP95 53.4 → 34.7 m (−18.6 [−25.8, −11.6]);
- engine-ToF endgame unchanged (+0.4 [−0.2, +1.1] m);
- 5 km +20 m on 324 m, interval spanning 0;
- 868 MHz alone improves: 52.4 → 25.1 m in the endgame.

It is the B2 consistency fix on the 868 MHz class, and it explains most of TECH_DEBT O.7.

**Reproducing earlier preset results:** add `--rssiSigmaLogNormal=false`. NC10, G63, G87, G89, G141 and G147 pin it.

**Headline tables:** CAMP-FINAL2 was measured before this adoption. Its `ca_tbl_full` endgame and 5 km figures are
within the SIG868 intervals above. Its `static_868` figures are superseded by SIG868's SIG rows.

## 15. The preset sets `--imuQuScale=30` (2026-10-17, B2CONS)
The IMU increment covariance handed to every filter is scaled by 30. Before this, the preset left it at the code
default of 1, and every campaign since S2 passed 10. The dead-reckoner's error is coherent, so its variance per cycle
is measured 3–54× too small. The registered adoption rule fired (`report/B2CONS_RESULTS.md`):
- `ca_tbl_full` coverage 0.647 → 0.692 (5 km) and 0.901 → 0.959 (endgame);
- CEP95 within +10 % (+20.9 m on 344 m; −0.1 m on 5.0 m).

**Campaigns** stop passing `--imuQuScale` (an explicit value still wins).

**Reproducing earlier results:**
- add `--imuQuScale=1` for pre-S2 preset runs;
- add `--imuQuScale=10` for 2026-10 campaigns.

NC10, G63, G87, G89, G100 and G130 pin it.

**`--rssiCorrInflate` (B2's other candidate) is not adopted.** It is right for 868 MHz alone (NEES 36 → 8), but its
rule failed on the hybrid's endgame coverage.

## 16. The code-default deployment shrinks to `--boxHalf=340` (owner decision 2026-10-18; TECH_DEBT K.6)
**Why.** At 1000 m the default 3×3 gateway grid had a 900 m pitch. The corrected probe puts the 2.4 GHz r80 at 156 m,
so tier 2 could not cover the deployment: `static_2p4` fixed on 41 of 1440 cycles, and G4/G5 had been red since the
environment split. At 340 m the pitch is 306 m, under 2 × r80.

**Measured** (seed 7, 3600 s, G3's configuration): `static_2p4` fix share 0.80 and the G4 verifier passes. The run took
113 s, so K.6's feared 5× suite slowdown (measured at 400 m under an older engine) does not materialise.

**Scope.** This is the code default only. The preset (`--specProfile`) sets its own `--boxHalf=5000`, so no preset
campaign moves.

**Reproducing earlier default-config results:** add `--boxHalf=1000`. NC1, NC6, NC7 and NC8 pin it.

## 17. PRESET-2: the candidate set is NOT adopted by its rule; arbitration V2 stays (2026-10-20)
**Registered:** `report/PREREG_PRESET2.md` and its Amendment 1. **Result:** `report/PRESET2_RESULTS.md`, seeds
101–120, 180 cells.

**The rule failed on one interval.** The endgame's five-node A(25 m) contrast was +0.035 [−0.044, 0.103]. That interval
failed both P1 (raise) and P3 (lower bound > −0.02). P2 (5 km tag 0, +0.443) and P4 (energy ratio 0.83–0.96) held.

So `--ekf5Window=60 --t2SensitivityRowKHz=812.5 --publishAfterStep=true --nlosRSource=detector
--foliageScaleBounds=true --interferenceDistance=true --rssiRangeFromRxPower=true --pcsByteScale=4` does not enter
`kSpecProfile`. Campaigns keep passing their stack explicitly.

**Arbitration V2, requester escalation and queue gossip stay** (P5 failed). With the requester rule able to refuse
(`--pcsByteScale=4`), turning them off costs +2.06 J [0.56, 3.68] in the endgame. This reverses DEBT-A's −18 %, which
was measured while the rule could never refuse (N.7).

**Open for the owner.** Every measured effect of the set on the requirement is upward:
- 5 km tag 0 0.39 → 0.83, five-node 0.27 → 0.50;
- 8-gateway ring tag 0 0.79 → 0.96;
- energy falls 4–17 %.

Adopting it would be an owner override, as §9 was. That is recommended in the result file and not applied.

## 18. The preset adopts the two receiver corrections (2026-10-20, FIDELITY-1's registered rule)
**What.** `--uwbSensitivityDbm=-99.3` (DW3000 data sheet, against our −104 dBm) and `--iqOverloadOnly=true` join
`kSpecProfile`.

`--iqOverloadOnly` stops the engines subtracting `20 log10(1 + EVM)` from rx power before comparing with a data-sheet
sensitivity, except under ADC overload. The subtraction is outside every census and saturates at 6 dB on UWB. It
double-counts impairments a data-sheet figure already contains (TECH_DEBT R.2).

**Why by rule, not by result.** The registration decided adoption on G161 green and the null control alone, before
any cell. Physics chosen by its effect on A(25 m) would be selecting the world. Both conditions held.

**FIDELITY-1 itself fired a registered abort:** three single window-solver failures, all in `static_2p4`. Its
numbers are not citable. The size of the change is owed to a re-run (FIDELITY-1b).

**Reproducing earlier preset results:** add `--uwbSensitivityDbm=-104 --iqOverloadOnly=false`.

## 20. The preset is the realistic world: the S stack enters `--specProfile` (owner decision 2026-10-21)
**What.**
- The filter: `--imuFilter=ekf5 --imuMagSigmaDeg=3 --ekf5FinderConsider --ekf5RelayGate`.
- The world: `--unknownStart --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2`.
- The comparator's threshold: `--fixedVarThreshold=12`.

**Why.** Every campaign since REAL-BEST (2026-10-18) passed these by hand. `--specProfile` alone still ran the
constant-velocity filter with a near-perfect gyro, seeded at the truth, so a forgotten flag silently ran the
optimistic world. This also closes T14 (the near-perfect gyro) and T38 (SHS is superseded by the 5-state filter).

**Code defaults are unchanged**, and they anchor the null controls (NC1–9).

**Reproducing earlier preset results:** give `--imuFilter=cv --imuMagSigmaDeg=0 --ekf5FinderConsider=false
--ekf5RelayGate=false --unknownStart=false --imuGyroBiasSigmaDps=0 --imuStrideSigmaPct=0 --finderPosSigma=0
--fixedVarThreshold=-1`.

## 21. PRESET-2's candidate set enters the preset by owner override (owner decision O10, option A, 2026-10-21)
**What.** `--t2SensitivityRowKHz=812.5 --publishAfterStep --nlosRSource=detector --foliageScaleBounds
--interferenceDistance --rssiRangeFromRxPower --pcsByteScale=4`, and `--ekf5Window=60` in its own commit.

**The registered rule failed, and the override is recorded as such.** PRESET-2 (`report/PRESET2_RESULTS.md`) failed
P1 and P3 on one interval: the endgame five-node contrast +0.035 [−0.044, 0.103] (DESIGN_DECISIONS 17). Every
measured effect was upward:
- 5 km tag 0 A(25 m): 0.386 → 0.829;
- 5 km five-node: 0.268 → 0.503;
- 8-gateway ring tag 0: 0.790 → 0.959;
- subject energy: −4 to −17 %.

Four of the settings are corrections that remove known wrong numbers or an oracle (D-8, the NLOS truth leak, the
foliage bounds, interference distance). The owner adopted the set as DESIGN_DECISIONS 9 adopted FIXES-N26.

**Consequences:**
- G93 stays red: the code default keeps the old row (O12, option B).
- Arbitration V2 stays, as PRESET-2's P5 found.
- Every claim drawn from the preset after this commit uses the set. The size of the whole preset change is measured
  by PRESET-3.

**Reproducing earlier preset results:** give `--t2SensitivityRowKHz=0 --publishAfterStep=false
--nlosRSource=operative --foliageScaleBounds=false --interferenceDistance=false --rssiRangeFromRxPower=false
--pcsByteScale=1 --ekf5Window=0`.

## 19. The Switching-Point Integrity Check leaves the preset (owner decision O13, 2026-10-21)
**What.** `--integrityCheck=false` in `kSpecProfile`. `ca_tbl_full` then runs exactly as the `ca_tbl_no_integrity` arm
does (G163 asserts it, byte for byte). The check's code, its flags and the `ca_tbl_no_integrity` arm all stay.

**Why.** Two registered results:
- **INTEG2** (fault-injected): at the preset's χ² 1992.9 the check catches 0.03–0.10 of injected faults, and no
  re-fit helps, because the fault-free distance tail (p99 21 322 at 5 km) blinds any threshold.
- **INTEG3:** it is not the cause of any 5 km deficit, and it costs +0.036 [0.004, 0.070] five-node A(25 m) on the
  8-gateway ring with no fault present.

The integrity claim was already withdrawn in the shipped default (Technical Report CR-9).

**What could bring it back:** a measurement covariance that carries NLOS and acquisition-state error (N.15), then a
registered INTEG4.

**Consequences:**
- G24's integrity populations become structurally zero under the preset, with this decision as the reason.
- **Reproducing earlier preset results:** give `--integrityCheck=true`.

*(Numbered 19 because O13 was decided first; recorded after 20 and 21 in this file.)*
