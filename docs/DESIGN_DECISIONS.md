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

## 22. Eight finders, tasked in pairs (owner decision O11, 2026-10-21)
**What.** `--nFinders=8` (was 4, decision 6) and `--finderTasking=pairs` (was roundrobin) in `kSpecProfile`.

**Why** (ANCHOR-SUPPLY, `report/PREREG_ANCHOR_SUPPLY.md`, seeds 121–140):
- **Eight finders** take the endgame five-node A(25 m) from 0.873 (four) to **0.970** (R8), and to 0.962 under
  pairs (P8). That is the first configuration in the project to meet the requirement for all five nodes.
- **Why four fall short:** with four finders, 6–12 % of endgame subject-runs lock onto a position 130–300 m wrong
  (TECH_DEBT R.13), and negative information cannot find them (NEGINFO). With eight there were none.
- **Pairs** cuts endgame subject energy by 24 % at eight finders (P8 against R8: 17.8 against 23.5 J), with
  five-node availability level (−0.008 [−0.048, 0.018]).

**The brief's deployment changes:** four finders become eight. The requirement-level statement is "eight finders in
the endgame" (open-decisions §10.1, option a).

**At 5 km no finder count meets the requirement** (best five-node 0.58). The 8-gateway ring is the infrastructure
that meets it for tag 0.

**Reproducing earlier preset results:** give `--nFinders=4 --finderTasking=roundrobin`.

## 23. Tier 3 runs one DW3000 mode: 850 kb/s, 1024-symbol preamble (UWB-MODE, registered rule, 2026-10-21)
**What.** `--uwbMode=850k` replaces `--uwbSensitivityDbm=-99.3` in `kSpecProfile`.

**Why a change was owed.** Tier 3 mixed three DW3000 modes (TECH_DEBT 2.4). Frames were billed at 6.81 Mb/s with a
128-symbol preamble. Ranging integrated 1024 symbols. The sensitivity, from DD 18, was the 850 kb/s row (−99.3 dBm).
The model billed a frame the receiver it described could not decode at that sensitivity. `--uwbMode` (G164) puts all
three on one row of DW3000 Data Sheet v1.1 Table 9, and G93 now checks that row against the data sheet.

**Why 850k** (`report/PREREG_UWB_MODE.md`, seeds 261–280, 180 cells). The owner asked to run both consistent modes
and adopt the better one. The rule was registered before any cell. Steps 1–3 (availability) did not separate the
modes. Step 4 did: **850k's subject energy is lower than 6m8's by 0.65 J [0.30, 1.03] in the endgame.** The extra
6.1 dB of sensitivity and 12 dB of ranging gain save more than 850k's 10.4× frame airtime costs, because tier-3
frames are a negligible share of the bill.

**The size of the correction against the old preset (L):** none measurable. CA-TBL endgame five-node
+0.014 [−0.002, 0.038], 5 km and ring within ±0.001, subject J −0.19 J [−0.69, 0.32].

**Choosing by performance is legitimate here, unlike DD 18.** The DW3000's mode is configured by the engineer
deploying it, not given by the world. Legacy was not a candidate: it was inconsistent.

**Reproducing earlier preset results:** give `--uwbMode=legacy --uwbSensitivityDbm=-99.3`
(`scripts/preset_pins.sh` carries it).

**An explicit sensitivity wins over the preset's mode.** Under `--specProfile`, a hand-set `--uwbSensitivityDbm` makes
the preset's `--uwbMode` yield to legacy, with a NOTE. A hand-set sensitivity has no Table 9 row, so half-applying the
mode would rebuild the mixed model. Typing `--uwbMode=6m8|850k` together with `--uwbSensitivityDbm` stays refused
(G164 YIELD and REFUSE). The full suite found this: G89's pin sets −104 dBm by hand.

## 24. Two fault-path corrections enter the preset (plan item 10, 2026-10-21)
**What.** `--silenceStopsRelay=true` (TECH_DEBT N.1) and `--deadBatteryNoDraw=true` (N.20) in `kSpecProfile`.

**Why.** Both are corrections to what a dead node does, not choices:
- **N.1:** a radio silenced by a spent battery or by `--injectSilentId` stopped its own frames but went on relaying
  other nodes' floods, so every fault run overstated how much of the mesh the fault removes.
- **N.20:** a node whose battery ran out went on being billed for listening, the neighbour-search window and sleep.

**Where they act.** Only on a node that is silenced or out of battery. Neither condition occurs in a fault-free
preset run, so such runs are byte-identical with or without them (the null control in the commit). Fault runs move,
by the size G70 and G71 measure.

**Not G157's defect class.** G157 asks whether an adopted mechanism fires into a zero in the fault-free endgame.
These two have no population there by construction, so they are not listed in G157. Their reachability is G70's and
G71's, on fault runs.

**Unused, not refused.** Each flag used to refuse to run without a fault. G24 runs the preset's settings as plain
flags, so a preset entry cannot be refused when typed by hand. Without a fault, each flag now prints a NOTE that it is
unused and the run proceeds; G70 and G71 check for the NOTE, with a fixture where it is missing.
`--deadBatteryNoDraw` without `--silenceStopsRelay` is a contradiction and stays refused.

**Reproducing earlier fault-run preset results:** give `--silenceStopsRelay=false --deadBatteryNoDraw=false`.

## 25. Eight gateways and twelve finders: the deployment that meets the requirement at 5 km (owner decision, 2026-10-22)
**What.** In `kSpecProfile`:
- `--nGateways=8 --gwLayout=legacy`, an 8-gateway ring (was 3, star layout; DD 9);
- `--nFinders=12` (was 8; DD 22). Pairs tasking stays.

**Why** (DEPLOY-REQ, `report/PREREG_DEPLOY_REQ.md`, registered, 240 cells, seeds 301–320):
- **The brief's 3 gateways do not meet A ≥ 0.95 for the five-node pool at 5 km at any finder count up to 24** (best
  0.936). No node is within 1.5 km of a gateway (DISSEM's addendum), and finders alone cannot replace that
  infrastructure.
- **8 gateways and 12 finders is the smallest deployment that meets it:** tag 0 0.987, five-node 0.955, 95 % of runs.
- 12 gateways meet it with more margin (0.970–0.975). The owner chose the smaller deployment.

**Known margin.** With 8 gateways, 16 finders gave 0.941: a single 20-seed mean is not monotone in finders, and the
8-gateway result meets the requirement on the mean, not with room. PRESET-4 (`report/PREREG_PRESET4.md`) measures the
adopted preset against the old on fresh seeds, at 5 km and in the endgame, which DEPLOY-REQ did not run.

**This departs from the brief's deployment** (3 gateways, DD 9) by owner decision. The brief's deployment stays
measurable by pinning back.

**Reproducing earlier preset results:** give `--nGateways=3 --gwLayout=star --nFinders=8`. `scripts/preset_pins.sh`
carries it for gates written against the earlier preset.

## 26. Twelve gateways: the requirement at 5 km with a margin (owner decision, 2026-10-22, after PRESET-4)
**What.** `--nGateways=12` in `kSpecProfile` (was 8, DD 25). The ring layout and 12 finders stay.

**Why.** PRESET-4 (`report/PREREG_PRESET4.md`) measured DD 25 on fresh seeds. The 5 km five-node pool came to 0.947,
against 0.955 on DEPLOY-REQ's seeds. At 8 gateways the requirement sits on the line. DEPLOY-REQ's 12-gateway ring
met it at every finder count from 12 (0.970–0.975; 100 % of runs at 12 finders), so the margin comes from gateways,
not from doubling finders (8 + 24: 0.982, at a large energy cost). PRESET-5 (`report/PREREG_PRESET5.md`) measures
the adopted preset against the brief's deployment on fresh seeds.

**Reproducing earlier preset results:** give `--nGateways=8` for DD 25's preset, and `--nGateways=3 --gwLayout=star
--nFinders=8` for the brief's. `scripts/preset_pins.sh` pins the latter.

## 27. A realistic deployment: three gateways on a line, searchers dispatched from one camp (owner decision, 2026-11-20)
**What.** In `kSpecProfile`: `--nGateways=3 --gwRidge=true --baseCampFrac=0.9 --nFinders=2 --searchLaunchOnLoss=300`
(`--gwLayout` stays `legacy`, the builder that places ridge lines). Replaces DD 25–26 (12 gateways on a ring, 12 finders
in the field from t = 0).

**Why.** An industrial site does not ring its work area with twelve powered, GPS-timed masts, and its responders do not
stand beside every worker from the first second. Gateways follow a line (an access road, a pipeline, a ridge) or a few
convenient spots, and a small team sets out from one place after an alarm. The ring and the finders in the field
manufactured the coverage: PARETO-2 found the 2.4 GHz-only arm meeting A(25 m) there only because a searcher was always
in reach. On one exploratory seed (7, 1800 s, not citable) the line deployment gave the lost node A(25 m) 0.989 for
ca_tbl_full against 0.519 for static_2p4 and 0.010 for static_868, and every arm's five-node pool fell below 0.2, which
is why the realistic study (`~/catbl_work/tii_paper/PLAN_REALISTIC.md`) measures time to find rather than A(25 m) alone.

**Reproducing earlier preset results:** `PIN_PRE_DD27` in `scripts/preset_pins.sh` restores DD 26's deployment (verified
2026-11-20: preset + pin against the pre-change build, seed 7, 600 s, six arms, 5 km and endgame: every output file
byte-identical except `spec_profile.txt`, the preset's own description). `PIN_PRE_DD19` gains `--gwRidge=false
--baseCampFrac=0 --searchLaunchOnLoss=0` and reproduces exactly as before (same check).

## 28. Estimation on the worker's device, and mast-height correction of ranges (owner decision, 2026-11-20)

**Where the filter runs.** For the realistic study (REALISTIC-1) and the paper, every worker's device runs its own filter
(`--gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false`); stations measure the 868 MHz broadcast
and reply, and each reply is billed to the device (`--tier1ChargeReplies`, default true). Reason: with station-side
estimation a worker no station hears gets no update, yet the simulator runs every node's filter each cycle, so
station-side results described a system that would not behave that way out of station contact. On the device the
filter keeps going on the motion sensor and on searcher/peer ranges. Exploratory check (seed 7, one station): positions
byte-identical to station-side estimation, battery +7 % (ca_tbl_full) and +20 % (static_868). Processor energy is not
modelled (stated limitation). The preset is unchanged (station-side), so earlier results and gates replay as they are;
the study passes the three flags on every run.

**Height (`--rangeHeightCorrect`, default off).** Ground is flat; gateways are on 15 m masts and devices are carried at
1.5 m. The engines measure slant ranges, while the filter is 2-D and read every range as horizontal: about 0.9 m long at
100 m and 4 m at 20 m. With the flag the device projects each mast range onto the ground, sqrt(max(r^2 - dh^2, 0)),
using the surveyed mast height and its own carried height; peer and searcher ranges (dh = 0) are untouched. Terrain
elevation still enters only as signal loss; slopes, multi-level sites and estimating height are out of scope.
Off: byte-identical to the build before the flag (seed 7, 7200 s, one station, three arms: every output except
`run_id.txt`). Gate G172 (+ selftest, five negative fixtures).

## 29. The work area's side of a gateway line (`--acquireSideOfRoad`, default off; 2026-11-20)

Found in REALISTIC-1 (ROAD3 with no searchers, before its result): with every gateway on one line, a range-only position
and its mirror image across the line are equally likely, and filters locked onto the mirror (estimates at y ~ 3150 m for
a worker at 2350 m, the ridge at 2750 m; ~800 m off for every design). A real deployment knows which side of its access
road the work area lies on. With the flag (needs `--gwRidge` and `--acquireParticles`) the acquisition particles are
drawn only on the work sites' side (y <= gwRidgeY x boxHalf) and particles that drift across carry no weight; after
hand-over, a Kalman estimate that crosses the line is reflected back (ranges to anchors on the line are unchanged by the
reflection; position rows of P mapped by the reflection). Pilot (not citable; seeds 501 and 504, 3600 s, no searchers):
lost-worker error at 10-50 min from 300-6600 m to 1-75 m for ca_tbl_full and static_2p4; no reflections were needed
(the acquisition constraint was enough). Off: byte-identical to the build before (seed 503, 900 s, two arms, against
REALISTIC-1's frozen binary; every output except run_id.txt and paths in run.log). Gate G173 (+ selftest, five negative
fixtures). Tested in REALISTIC-3; REALISTIC-1 and -2 stand as registered (flag off).

## 30-32. The search party under a 2.4 GHz-only design (three defects; flags default off; 2026-11-21)

Found while checking REALISTIC-1's H2 (2.4 GHz-only searchers reached the casualty in 0 of 60 runs, even at the
cluster site where the casualty is 0.8 km from the camp with five anchors in reach). Three defects, each confined to
the searchers, each found on traces:
- **DD 30 `--finderDigestOnArmTier`.** The searchers' entry poll (DIGEST) and the table they steer on were hard-wired
  to their 868 MHz radio for every arm. Under static_2p4 the casualty's 868 MHz radio is off, so its entries reached
  the searchers' 2.4 GHz table, which nothing read. With the flag, under static_2p4 the party polls and steers on 2.4 GHz.
- **DD 31 `--finderSweepFromDispatch` (bug).** The no-entry sweep's radius was 0.9 x speed x t from the start of the
  run, so a party held at the camp until an alarm at 50 min appeared on a 3.2 km spiral at dispatch (every
  static_2p4 run of REALISTIC-1/3: a 3.24 km one-step jump; every other arm: 3 m). With the flag the clock starts at
  the dispatch.
- **DD 32 `--finderLkpFirst`.** The party swept round the camp whenever the entry it held was coarser than the sweep
  so far, ignoring a good last known position. With the flag it sweeps only while the entry is uninformative (sigma
  above half the box half-width, like the start-up prior) and otherwise heads for the last known position; with
  `--finderStaleSearch` it then orbits a stale one. Standard search doctrine.
Pilot (not citable; seeds 501-502, cluster and ROAD3): with all four on, 2.4 GHz-only searchers reached the casualty
at the cluster site as fast as ca_tbl_full (692 vs 720-748 s) and on ROAD3 later or not at all (2543 s vs 2075 s;
never vs 2208 s). Off: byte-identical (seed 501, CLUS3, 7200 s and seed 503, ROAD3, 3300 s, against REALISTIC-3's
frozen binary). Gate G174 (+ selftest, five negative fixtures). REALISTIC-1/2/3's searcher results were measured
without these fixes and are reported as such; REALISTIC-4 repeats the searcher cells with them.

## 33-34. Crew filters that are lost and confident (flags default off; 2026-11-21)

Found while asking why REALISTIC-2's 8 h quiet shift at the cluster site (Q1 failed: ours 0.22 against 2.4 GHz only's
0.55 rough-position availability) went wrong. On traces (seed 541, subject 2) the crew's filters, lost after start-up
(sqrt(tr P) of kilometres), were pulled onto a wrong root by the first long 2.4 GHz range (6.8 km in open terrain) or a
two-gateway intersection, then claimed +/-20-30 m while 0.4-5 km off; one learned a stride scale of -50 and a gyro bias
of -0.37 rad/s and circled in place. Over 7200 s, 8124 of 14400 tracked-node cycles were "confidently wrong" (error >
500 m at sqrt(tr P) < 50 m).
- **DD 33 `--ekf5CoarsePosOnly`.** An 868 MHz range corrects position only (heading, bias, scale are Schmidt consider
  states for it; kept out of the window smoother). Pilot (8 h, seeds 541-543, not citable): ours' A250 0.09/0.29/0.42
  against 0.09/0.39/0.24 off. **Not adopted** (no consistent gain); kept, documented, default off.
- **DD 34 `--reacquireSigmaM`.** A filter whose sqrt(tr P) exceeds the threshold (after a hand-over has armed it)
  re-enters the particle acquisition round its estimate, the cloud as wide as its uncertainty and never wider than the
  start-up prior, and hands over again when the cloud is one mode. Pilot (8 h, seeds 541-543, threshold 500 m, not
  citable): ours' A250 0.54/0.61/0.69 (from 0.09/0.39/0.24) at 32-40 J/h (from ~45); 2.4 GHz only 0.56/0.53/0.37 (from
  0.63/0.65/0.67). Confidently wrong cycles 8124 -> 13 (G175). `--acquireHandoverCurvature` was tried alongside and
  lowered both designs (ours 0.09/0.09/0.24, 2.4 GHz only 0.37/0.23/0.37); not used.
Off: byte-identical (seed 505, ROAD3 with every REALISTIC-4 flag, 3300 s, two arms, against REALISTIC-4's frozen binary).
Gate G175 (+ selftest, five negative fixtures). Tested in REALISTIC-5 on new seeds.

## 35. A different incident on every seed (`--incidentRandom`, default off; 2026-11-21)

Found in a full-system audit after REALISTIC-5: the casualty stood at the identical point at the alarm on every seed of
every realistic campaign (RMS spread 0 m; ROAD at (-1591, 1673), CLUS and RAND at (5000, 3823)), so each cell's 20 seeds
were one incident under 20 noise draws. From the corner camp of the CLUS and RAND sites the fixed due-east trail also
pinned the casualty to the box edge. With the flag the incident is drawn per seed from its own random stream: a trail
from the start toward the site centre +/- 45 deg, leaving it at 15-35 min, a 45-135 deg turn either way, a stop 5-15 min
later (still by the 50 min alarm); the box edge reflects. The same seed gives the same incident in every arm.
Off: byte-identical (seed 566, CLUS3 with every REALISTIC-5 flag, 3300 s, two arms, against REALISTIC-5's frozen
binary). Gate G176 (+ selftest, five negative fixtures). Tested in REALISTIC-6.

Also found in the audit, not changed in code: (i) the relay schedule (`--relaySlots=auto`) needs ~N^2 50 ms slots and
refuses crews above ~7 in a 2.5 s cycle; larger crews run with `--relaySlots=false` (REALISTIC-6 crew cells). (ii) The
gateways' 868 MHz replies under device-side estimation are billed to the node but not simulated as gateway frames
against the 10 % access-point duty cycle; one batched broadcast per gateway per cycle (~3 % duty for 7 nodes) is the
protocol the bill corresponds to.

## 36. Collapse extension, World side: the rubble model (no flag yet; nothing sets it; 2026-11-21)

First module of the collapse extension (plan: ~/catbl_work/tii_paper/PLAN_COLLAPSE.md; numbers: docs/SOURCES_COLLAPSE.md).
`CaTblRubbleWorld` (ca-tbl-rubble-world.{h,cc}) holds the ground truth of a collapsed structure: non-overlapping rubble
boxes and survivable voids inside them, plus one bracket (low / central / high). For a link it gives the metres of pile
on the straight path (slab method, voids subtracted), the loss in dB and the time-of-flight bias in m.
- **Loss.** Below 2.5 GHz: A(f) = A2·(f/2 GHz)^k, with A2 = 20 / 31 / 80 dB/m and k = log2(20/8) / log2(31/14) /
  log2(31/14) (Oestges 2013 layered and measured rubble). 868 MHz gives 6.63 / 11.90 / 30.71 dB/m, 2.44 GHz gives
  26.01 / 38.94 / 100.49. Above 3 GHz: η·A_solid(f). η = 0.3522 / 0.5459 / 0.7706 is derived as A_rubble(2 GHz) /
  A_solid(2 GHz) (no pile density measurement was found). A_solid is ITU-R P.2040-4 concrete (low, central) or
  NISTIR 6055 plain concrete (high). 7.99 GHz gives 59.09 / 91.59 / 219.45 dB/m.
- **Bias.** η·(√ε′ − 1) = 0.454 / 0.704 / 1.541 m per metre of pile, added to 2.4 GHz and UWB ranges after the
  engines' own error draw. 868 MHz RSSI-inversion ranging sees the loss as distance: d·10^(L/10n). Both are applied
  after the engines' link-leak handling, so the loss enters the budget once (tier 3).
- **Not modelled yet:** loss of the direct path under deep rubble (Alsindi 2009 tables pending). This is a stated
  limitation until sourced.
- **Engines.** Each engine has `SetRubbleWorld(const CaTblRubbleWorld*)`; nullptr is the default and every hook is
  inside `if (m_rubble)`. Nothing in the simulator calls it yet; `--scenario=collapse` (DD 37) will.
- **World/system split.** System-side files may not include the header; G177 fails if one does.
Off: byte-identical. The three reference runs were static_868 device-side at seed 7 for 600 s, preset ca_tbl_full at
seed 7 for 300 s, and ca_tbl_full at seed 11 for 900 s. They were run on the build before the hooks (ec656a27b
sources) and after the hooks, giving 178 files. All are identical except run_id.txt, which holds the build stamp.
Gate G177 (`scripts/rubble_gate.sh`): 50 checks covering code values against the log's rows, the A5 targets (the
868 MHz 2–5 m totals span NIST's 30 and 80 dB bounds; Oestges' detection range is 10.92 m against ~10 m), geometry,
refusals and the no-leak rule. Selftest: 12 doctored inputs, all caught.

## 37. Collapse extension, World: lowest-loss rubble path and the collapse generator (2026-11-22)

**Physics fix to DD 36.** DD 36 charged a link the rubble on its straight line. For a device 2 m deep and a rescuer
20 m away on the surface, that line grazes through about 19 m of rubble (230 dB at 868 MHz). A real wave leaves the
pile by a short route and travels the rest in air. `CaTblRubbleWorld::Path(f, a, b)` now takes the lowest-loss of
these routes:
- the straight line;
- for a buried end, rubble to an exit point on the top or a side face, then air (two-pass line search per face);
- for two buried ends, out of the top above one, through the air, back in above the other.

Cost = A(f)·(rubble metres) + 20 log10(path length / straight distance) + the Fresnel power loss of each surface
crossing. That loss is taken at the actual angle, TE/TM averaged (ITU-R P.2040-4 eqs. (30)–(35)). Rays beyond the
critical angle are totally reflected, and that route is not available. The pile's refractive index is the slowing
model's n = 1 + η(√ε′ − 1): 1.454 / 1.704 / 2.541, with critical angles 43.45° / 35.94° / 23.17°. So loss and delay
use one effective medium. Bias = (path length − straight distance) + (n − 1)·(rubble metres). The engines call
Path(); the straight-line LossDb/BiasM stay for the gate fixtures.

Fixtures (G177, now 58 checks):
- straight up 2 m: 2·A + 0.3047 dB;
- 20 m away: one exit, 2.0009 m of rubble, against 11.6 m on the straight line;
- two buried ends: out-and-in, 4.000 m of rubble;
- symmetric to 0.05 dB;
- a device near a side exits sideways (0.5 m);
- a clear path gets nothing.

**Collapse generator.** `scratch/ca_tbl_collapse_sim.cc`. The main simulator estimates one subject; workers beyond
it are protocol peers without estimation. Making it estimate 20–30 subjects would rework the code every earlier
result runs on. So the collapse study has its own program, reusing the same engines, PHY values (SF7/125 kHz/14 dBm,
SF7/800 kHz/12.5 dBm, UWB 499.2 MHz/−16.47 dBm) and rubble World. The main simulator is untouched.
- **Layout.** A box pile; N workers placed uniformly in the footprint at depths uniform in [depthMin, depthMax];
  optional voids. Candidate rescuer points on a surface grid at given hold heights; ground stations on a ring.
- **No defaults.** Every geometry, time and GPS value is a required argument and is refused if missing. The
  pre-registration supplies values from rows or sweeps.
- **Draws.** Each slot draws every (worker, point), (worker, station) and (worker, worker) link on each band through
  the engines in keyed-RNG mode. G178 shows a link's outcome does not depend on which other links were drawn. A
  rescuer policy choosing points from this field therefore sees what it would have measured there.
- **Interference.** The engines' interference registry and external-interferer field (one switch) are off. A link
  outcome means "scheduled alone"; contention is the MAC stage (B5).
- **Environment.** ENV_RURAL_OPEN, so all obstruction comes from the rubble model; ENV_INDUSTRIAL is a sensitivity
  case.
- **Outputs.** obs_links.csv (received, rx power, SNR, range if received); obs_anchors.csv (GPS fixes of points and
  stations, σ from rows GPS-SPS-*). Ground truth goes separately to truth_victims.csv, truth_anchors.csv and
  truth_links.csv. System-side scripts in scripts/collapse/ may not read truth_* (G178 leak check; only score_*.py
  may).

First look (seed 3; 30 × 30 m pile, 3.66 m high; 20 workers at 0.5–3.5 m; central bracket; not citable):
- 868 MHz: every worker-to-surface link received, but the RSSI range error has a median of 120 m (the extra loss
  reads as distance).
- 2.4 GHz: 38% received; range error median 1.1 m, 90th percentile 5.2 m.
- UWB at 7.99 GHz: nothing through rubble.
This is the cross-band picture B1 is built on.

Off: byte-identical (same three reference runs as DD 36; 178 files, only run_id.txt differs). Gates G177 and G178
(+ selftests: 14 and 5 fixtures).

## 38. Collapse extension, System: first estimators (tof / rss / joint) and the scorer (2026-11-22)

`scripts/collapse/estimate_collapse.py` is system side. It reads obs_*.csv and the operational plan (hold heights)
only; G178 fails if a system-side script mentions truth_ files. `score_collapse.py` is the only reader of truth.
- **Sweep.** Rescuers walk the surface spots in serpentine order, split among --rescuers, one spot per slot,
  measuring at every hold height.
- **Air law.** Fitted per band from the system's own point-to-station links (no rubble): loss = L0 + 10 n log10 d.
- **Estimators.** Per worker after each slot, 3-D:
  - **tof:** 2.4 GHz / UWB multilateration, σ 3.0 m / 0.10 m (the earlier work's cited floors);
  - **rss:** received power at every band with one unknown rubble excess C_b per band (calibration-free);
  - **joint (B1):** both, with the ToF bias tied to that excess, r = |x − a| + C_b/ρ_b. ρ_b is the rubble's
    loss-to-delay ratio, A_b/(n − 1). It varies little across the published brackets (2.44 GHz: 55.33–65.21 dB/m;
    7.99 GHz: 130.2–142.4; 868 MHz: 14.6–19.9), so the system uses the midpoint with the range as uncertainty and
    never knows the World's bracket.
- **Depth prior.** A buried device lies below the surface (rescuer points minus hold height). It is generic, not
  site knowledge.
- **Frames.** Positions are in the rescuers' local frame (σ 0.10 m per axis, the UWB line-of-sight floor) or in
  GPS (rows GPS-SPS-*).

First look (seed 3; the DD 37 layout; 2 rescuers; 18 slots; local frame; not citable). Located / horizontal median /
p90 / ≤ 2 m:
- joint: 1.00 / 1.43 / 5.55 / 0.60;
- rss: 1.00 / 1.50 / 5.55 / 0.55;
- tof: 0.45 / 1.33 / 1.83 / 0.40.
Vertical error medians are 1.4–3.1 m: depth is the weak axis, as planned (B2, Cramér–Rao bound next).

## 39. Collapse World in the main simulator: 20 buried workers through the full pipeline (`--collapse`, default off; 2026-11-22)

The main simulator already localises several workers (`--measuredSubjects=N`, MS-1: each subject runs ranging, the
tier-1 floor, fix selection, `Step()` with its own escalation, the handshake, battery and IMU). So the collapse study
runs there, with the preset's engine physics, MAC, relays, finders and energy unchanged, instead of in a separate
program. `scratch/ca_tbl_collapse_sim.cc` stays a link-level tool for the rubble physics.

**Mechanism.**
- `--collapse` builds one `CaTblRubbleWorld` per arm: a box pile centred on the origin, `--collapseFootX/Y`,
  `--collapsePileH`, `--collapseBracket`. It sets it on the three engines.
- Every tag (the measured one and every peer) is buried and still from t = 0.
  - (x, y) is uniform in the footprint (0.5 m margin); depth is uniform in [`--collapseDepthMin`, `--collapseDepthMax`].
  - Each position comes from its own stream keyed by seed and worker index. So worker k lies at the same place
    whatever `--nTags` is: the 1-worker run is worker 1 of the 20-worker run.
- A walker or finder inside the footprint stands on the pile: its device is 1.5 m above the top. Elsewhere it stays
  1.5 m above the ground.
- No geometry value has a default: `--collapse` without all five is refused, as are out-of-range values and a bad
  bracket.
- `PIN_ONE_WORKER` in `scripts/preset_pins.sh` is the preset with one worker. The preset's relay, fix-routing,
  case-C and finder-tasking features need ≥ 3 tags or > 1 subject, and refuse otherwise.

**First runs** (seed 5, 600 s; 30 × 30 m pile 3.66 m high; depths 0.5–3.5 m; central bracket; placeholders, not
citable).
- **Station geometry decides everything.**
  - The preset's line of 3 gateways 55 m from the pile gave mirror-ambiguous tier-1 TDoA and median errors of
    250–320 m.
  - 4 gateways on a ring around the camp (90, 90) left the pile outside the ring, and no worker got a fix.
  - With `--gwRidge=false --baseCampFrac=0 --boxHalf=50 --nGateways=4` (a 30 m ring centred on the pile), tier-1
    TDoA solved in 211 of 236 cycles.
- **Worker 1, median error (CEP50):**

| Arm | 1 worker | 20 workers |
|---|---|---|
| ca_tbl_full | 5.4 m | 6.1 m |
| static_868 | 654 m | 7.3 m |
| static_2p4 | never fixes | never fixes |

- Open: the 1-worker static_868 result (the pin removes the relays it uses with 20). The 2.4 GHz-only arm never fixes
  because 2.4 GHz through rubble to the 30 m ring is rarely heard.
- `subject_summary.csv`'s `fixCycles` counts only precise-tier fixes. Use the traces for 868 MHz fixes.

**Off: byte-identical.**
- Three reference runs (178 files) match.
- `scripts/merge_identity_check.sh` against `main`'s binary still gives 614 identical files.

**Gate G179** (`scripts/collapse_main_gate.sh`):
- 20 subjects in every arm;
- every worker still and inside the footprint;
- worker 1 at the same spot alone and with 20;
- the 1-worker ca_tbl_full arm fixes;
- the stations form the ring;
- three refusals.
Selftest: 4 doctored outputs, all caught.

## 40. The collapse link field with the preset's engines (`--collapseField`, default off; 2026-11-22)

`--collapse --collapseField` draws the link field (obs_* / truth_* files) inside the main simulator
(`RunCollapseField`) instead of running arms.
- **Engines.** Built by `CA_TBL_MAKE_CONFIGS`, exactly as in an arm, so the preset's ~20 engine switches apply. The
  standalone `scratch/ca_tbl_collapse_sim.cc` did not apply them. That is why its VAL-1 failed: the 868 MHz
  range-error leak and the receiver's clipping stage were on there.
- **Positions.** Buried devices are where the full runs put them (`CollapseBuriedPos`).
- **Isolation.** Interference is off ("link scheduled alone").
- **Extra output.** `truth_draws.csv` carries each draw's engine total loss for VAL-1.

**Validation on it** (rows VAL-* in `docs/SOURCES_COLLAPSE.md`; sourced 8-storey IND2 geometry, 4 stations at
56.45 m; seed 21): 11 of 12 checks pass.
- VAL-1 is exact (≤ 0.0001 dB over 30,078 draws).
- VAL-2 spans NIST's bounds (1.7–103.6 dB).
- VAL-6 reproduces SLOW-BR exactly (0.454, 0.704).
- UWB line-of-sight spread is 0.107 m, inside Benouakta's 0.10–0.33 m.
- **Fails:** 2.4 GHz line-of-sight spread at Zhou's 28–57 m is 5.11 m against the measured 1.27–3.2 m. At SNR ≥ 30 dB
  it is 2.9–3.1 m; lower-SNR links widen it through the CRLB term. The model is conservative for 2.4 GHz. Reported,
  not tuned.

Off: byte-identical (178 reference files; 614 files against `main`'s binary).

## 41. Copies of one broadcast collided with each other (`--interferenceFrameKey`, default off; 2026-11-22)

**Defect found (earlier simulator, every run with more than one field node on the mesh).** The channel delivers a
frame to each receiver through `EnginePhyProvider::EvaluateFrame()`, one engine call per receiver. Each call enters
the engines' interference registry as a transmission of the field node in the engine's node slot. For a gateway
beacon that is the receiving subject, not the gateway. So the second and later receivers of one broadcast found the
earlier receivers' entries overlapping in time and lost the frame.

**Measured.**
- Buried subjects 2..20, 30 m from four stations, heard no anchor in 84% of cycles; subject 1 (evaluated first)
  heard 4–6 in every cycle.
- At link level (DD 40, interference off) every one of them is heard about 100% of the time.
- On the open-site preset with 5 subjects (`main`'s ENV-SWEEP E0 configuration, seed 361, 1800 s), the subjects'
  median CEP50 is **2126 m without the fix and 3.6 m with it** (ca_tbl_full).
- Tag 0 changes too: static_868 CEP50 291 → 382 m on that seed. Its receptions also shared the registry with other
  receivers' copies.

**Fix.** Each registry entry carries a frame key; 0, the default, means none. Under `--interferenceFrameKey` the
provider sets one key per (sender, tier, time) before evaluating the receivers and clears it afterwards. Copies of
one frame then never interfere with each other, while different frames still do.

**Status.** Default off: every earlier result is byte-identical (178 reference files; 614 files against `main`'s
binary). It is **on** in the collapse study and will be pre-registered there.

**Owner decision needed.** Adopt it into the preset and re-run the multi-subject campaigns? Every
`subject_summary.csv` row for subjects 2..N on `main` and on this branch is affected, and tag 0's rows are affected
where several field nodes share the mesh.

**Gate G180:**
- off: subjects 2..5 starve (≥ 30% of cycles with no anchor; 49.5% measured);
- on: ≤ 5% (1.0%);
- tag 0 keeps its anchors in both.
Selftest: 3 doctored outputs, all caught.

## 42. Worker-to-worker cooperation (B4) and the depth bound (B2 theory) (2026-11-22)

**B4 (`coop` in `scripts/collapse/estimate_collapse.py`).** All workers are solved together: joint's residuals plus
the 868 MHz worker-to-worker links.
- **Coupling.** A worker-to-worker path leaves the pile above one worker and re-enters above the other (DD 37), so
  its excess is the two workers' own: P_ij = Ptx − L(|x_i − x_j|) − C_i − C_j, with no new unknowns.
- **Checked against the World.** C_ij − (C_i + C_j) has median 0.78 dB, 10th–90th percentile 0.28–2.35 dB (190
  pairs; 8-storey IND2; central).
- **One residual per pair** (median power over the slots heard). Repeated slots of a link whose shadowing is fixed
  only overweight it. With every slot used, coop was worse than joint (median 3.36 m against 1.63 m).

**Result** (preset-physics field, seed 31, 20 workers; not citable):

| Rescuer coverage | Median error, coop | Median error, joint | Within 2 m, coop | Within 2 m, joint | p90, coop | p90, joint |
|---|---|---|---|---|---|---|
| Full | 1.45 m | 1.63 m | 0.40 | 0.35 | — | — |
| Half | 2.30 m | 2.65 m | 0.45 | 0.40 | 16.9 m | 12.2 m |
| Quarter | 8.09 m | 5.32 m | — | — | — | — |

So cooperation helps the typical worker when rescuers cover at least half the surface, but makes the worst cases
worse. A rigidity check (only lean on neighbours that are themselves anchored; Eren 2004 / Aspnes 2006) is the
next step.

**B2 bound (`scripts/collapse/crb_depth.py`).** Cramér–Rao bound for a worker 2 m deep, ranging from a grid
around it:
- 2.4 GHz (σ 3 m): 0.6–1.8 m.
- UWB (σ 0.10 m): 2–6 cm.
- Holding the device low and high (0.3 and 1.5 m) gives with 26 contacts within 5 m (0.88 m) about what one height
  gives with 49 contacts within 10 m (0.82 m).
- The estimators' depth errors (median 1.3–2.6 m) are 1.5–3× the bound.

## 43. Phase 0: the workers compute on their own nRF52840 (`--phase0Timing`, output only; 2026-11-22)

`--phase0Timing` times every worker's `CaTblPolicy::Step()` (filter, 60-step smoother, particle acquisition,
escalation) on the host clock and prints one PHASE0 line. The run's files are unchanged.
- **Measured** on core 0 (taskset; 20 workers; 8-storey collapse; 600 s): 1077 µs per step for ca_tbl_full, 1019 µs
  for static_868.
- **CoreMark** on the same core under the same load: 25,134 (CM-HOST).
- **Scaled** to the nRF52840 (212 CoreMark, NRF-CM): 128 / 121 ms per 2.5 s step, i.e. 5.1 / 4.8% of the cycle.
- **Energy** at 3.3 mA · 3.0 V (NRF-ICPU0): 1.26 mJ per step = 1.82 J/h. That is 2.0% of the full system's radio
  energy (89.4 J/h) and 6.1% of static_868's (28.2 J/h).

**Decision** (plan rule: compute ≤ 10% of radio energy and ≤ 20% of the cycle). On-worker estimation passes as
long as the double-precision penalty on the single-precision M4F (NRF-FPU) is ≤ 3.9× for ca_tbl_full (≤ 1.6× for
static_868). No sourced penalty was found, so the result is stated as that bound (`scripts/collapse/phase0.py`). A
single-precision port of the filter makes it k = 1. Device-side estimation (the REALISTIC studies'
`--gatewayEdgeServer=false`) therefore stays.

**COLLAPSE-3** (`report/PREREG_COLLAPSE3.md`, registered before COLLAPSE-1's energy is looked at): hours on one
CR123A (16,740 J, BATT-CR123A) per worker, from COLLAPSE-1's 1800 s energy plus this compute.

## 44. B3, the rescuers' frame from their own ranges (`--frame uwb`, field estimator; 2026-11-22)

`estimate_collapse.py --frame uwb` places each rescuer point from its time-of-flight ranges to the stations:
- UWB where heard (σ 0.10 m), 2.4 GHz otherwise (σ 3.0 m), median over slots;
- the stations placed by their GPS fixes;
- a point with fewer than 3 station ranges keeps its GPS fix.

The workers are then located against these points.

**Result** (seed 31 field: 8 storeys, central, 20 workers; not citable). Median horizontal error and fraction
within 2 m:

| Frame | Median | Within 2 m |
|---|---|---|
| Assumed local frame, `--collapseLocalSigma` 0.10 m | 1.63 m | 0.60 |
| GPS fixes | 2.82 m | 0.45 |
| B3 | 3.66 m | 0.30 |

At the sourced station standoff (NIOSH-CZ, 56.45 m), UWB reaches a station on only 75 of ~16,000 point-station
links, so the points are placed by 2.4 GHz ranges, which are worse than GPS here.

**Conclusion, stated in the paper.** The localisation results that use the local frame assume a rescuer frame of
UWB quality. Reaching that frame needs rescuer-to-rescuer UWB over a few metres (not modelled), or stations closer
than the collapse zone. In the main simulator the finders' own position error is the earlier work's GNSS model
(`--finderPosSigma=2` in the preset, DD 20).

**DD 44 update: rescuers ranging each other (2026-11-22).** The field gets `--collapsePointLinksM=10` (rescuer
points draw links with every point within 10 m), and `--frame uwb` now solves all 128 rescuer points jointly:
point-to-point ToF, point-to-station ToF, and each point's GPS fix as a weak prior. On the seed 31 field it used
1,800 range pairs, 1,273 of them point-to-point UWB. Median horizontal error and fraction within 2 m:

| Frame | Median | Within 2 m |
|---|---|---|
| B3 (joint frame solve) | 2.50 m | 0.40 |
| GPS | 2.82 m | 0.45 |
| Assumed local frame (σ 0.10 m) | 1.63 m | 0.60 |

The rescuers' own UWB therefore improves on raw GPS, but does not reach the assumed UWB-quality frame. The likely
reason is that the frame's absolute placement still rests on GPS and 2.4 GHz station ranges; the remaining error is
mostly a common offset. Results that use the local frame are stated as conditional on that frame quality. Scoring
relative to the rescuers' own frame (where they dig) rather than in absolute coordinates is the next refinement.
Exploratory, not citable.

**DD 44 correction (2026-11-22).** The explanation above ("mostly a common offset") is **refuted**. The scorer's
`--relative` mode removes the rescuer frame's mean horizontal offset, as rescuers digging in their own frame would.
Results:

| Frame | Offset removed | Median, absolute | Median, relative |
|---|---|---|---|
| B3 | (+0.49, −0.20) m | 2.50 m | 2.71 m |
| GPS | — | 2.82 m | 2.48 m |
| Local | — | 1.63 m | 1.62 m |

The B3 frame's error is distortion between points, not a shift. The 10 m UWB mesh is tied to the stations by
2.4 GHz ranges (σ 3 m) and to GPS (σ 3.27 m), and these bend it. A denser or longer-range rescuer UWB mesh, or
stations within UWB range, is what the frame needs. Exploratory, not citable.
