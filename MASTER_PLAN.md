# CA-TBL / CA-DBL — Master Implementation and Completion Plan

**Written** 2026-09-04. **Repository state** `01c852d`, 2026-09-02, clean working tree, 89 results directories (1 746 tracked evidence files).
**Thesis state** all 15 chapters `SCAFFOLD` (headings only, no prose), 43 open register items, last register update 2026-07-30.

**Scope.** One document covering both halves of the project: what the ns-3 implementation has built against what the Phase 1 blueprint specifies, where the two contradict, what remains unbuilt, what is measurably broken, and two branching routes forward with an explicit branch point and abort criteria.

> **⚠ TOPOLOGY, corrected 2026-09-07 (`99fdf1a`) — after this plan was written.** This document was drafted against the legacy deployment model of three **disjoint** anchor populations (tier-1 gateways on a grid; tier-2 anchors ringed at 150 m and tier-3 at 10 m around four facility sites). **That model was wrong.** The system has **one population of tri-band nodes** — every node carries all three radios, and a tier is a choice of *which radio to use*, not of *whom to talk to*. Gateways are the only immobile class; **finders** are mobile tri-band nodes; facility sites are mobility targets that **host no anchors**. Authoritative: `Architecture.md` §3, `RULES.md`, and `BuildDeployment()` at `scratch/ca_tbl_causal_sim.cc:177`.
>
> **Consequences for this plan.** Any row, gap or track whose reasoning rests on *anchors clustered at four facility sites* — notably the M-4 / §13.3 "coverage limit" diagnosis and the complementary-siting track — is reasoning about a geometry the system does not have, and needs re-deriving rather than re-reading. Every results figure quoted here predates the correction and is **provisional pending re-run**.

---

## 0. Source basis

Every statement in this plan traces to one of the following. Where a number appears, its source file is named. Nothing here is estimated except where the word *estimate* appears.

| Ref | Source | What it establishes |
|---|---|---|
| S1 | `Architecture.md` (repo, §1–13) | System design as built: tiers, engines, MAC, energy model, handoff loop, verification pyramid |
| S2 | `CA-TBL_Technical_Report_SelfContained.md` (repo) | Claim-by-claim record — authoritative for results |
| S3 | `Simulation.md` §13–14 (repo) | Work programme and project standing |
| S4 | `report/PCS_REDESIGN_RESULTS.md` Parts 0–8 (repo) | Evidence chain for the decision-layer question, corrections registers |
| S5 | `docs/archive/CA_TBL_PHASE8_SCOPE.md` (repo) | Line-level scoping of the three unbuilt novelty components |
| S6 | `CA_TBL_SIM_AUDIT.md`, `CA_TBL_AUDIT_REPORT.md` (repo) | Build-order history; independent adversarial audit |
| S7 | `EXCLUDED.md`, `SHA_MAP.md` (repo) | Evidence-tracking discipline, history-rewrite lookup |
| S8 | `CA-DBL_Phase1_Blueprint.md` (project) | Phase 1 drafting plan, source hierarchy, chapter map, §6 engine-coverage gap table |
| S9 | `terminology_lock.md`, `pending_items.md`, `drafting_status.md` (project) | Locked terminology, 43 open items, per-chapter status |
| S10 | `ch09`–`ch15` scaffolds (project) | Specified section structure and claim assignment |

**Two internal conflicts in the sources, flagged not resolved.**

- **Tier-3 `r80`.** S1 §2.1 records **12.8 m**; S2 §2 records **≈ 13.5 m**. S6's untraceable-numbers table lists the 13.5 m figure as unbackable from disk at the time of that audit, so 12.8 m is the later measurement. The two documents disagree and one is stale.
- **Tracked sidecar count.** S7 (`EXCLUDED.md`) states **1 065 sidecars are tracked**; S7 (`SHA_MAP.md`) states **1 283 sidecars** cite the six rewritten SHAs. Both may be true under different counting rules (`arm_summary.csv` + `run_id.txt` pairs versus `run_id.txt` alone), but neither document says which rule it uses.

Both are recorded in §13 as actions, not resolved here.

---

## 1. The situation in one page

The Phase 1 blueprint (S8) was written against an engine that was 868 MHz only, star-topology, stationary-node, with no MAC, no estimator, no energy model, and no decision layer. Its §6 table concludes: *"every CA-DBL novelty claim is currently unsimulated."* That table is now obsolete. The repository has since built three physics engines, a MAC with duty-cycle governance and emergent capture, a datasheet-traceable energy model, a four-state EKF, a confidence score, an integrity gate, and a six-arm comparative campaign at 20 seeds across three geometries, four environments and a density sweep.

It has also **measured the central claims and falsified most of them.**

| Blueprint claim | Where specified | Measured outcome |
|---|---|---|
| Confidence-driven escalation beats static provisioning on cost | S10 ch10 §10.6, claim 1 | **Falsified.** Loses to `static_2p4` on accuracy-per-joule at 0/20 seeds, three geometries, four environments. Static tier-2 provisioning dominates all 13 operating points of both decision rules' Pareto fronts (S2 §1, S4 §5.3, §6.10) |
| PCS beats a flat variance threshold | S10 ch10 §10.6 | **Falsified thinly.** 19/20 vs 18/20 deployments at 24.26 J vs 22.77 J (S2 §5.2) |
| PCS carries information | S10 ch10 §10.6 | **Falsified.** ρ = −0.199 against a monotone-of-filter-statistic control at −0.200. A redesigned score (v2), pre-registered before measurement, fails the same test (S2 §1, S4 §6.1) |
| Switching-Point Integrity Check is the central contribution | S10 ch10 §10.7 | **Falsified as rejection-only** — negative in the mean in 8/8 cells, sign test against the policy in all eight (S2 §5.1). **Not falsified as detect-and-escalate** — moves `cep50M` 92.15 m → 3.10 m under propagation-derived NLOS |
| Hop-aware confidence degradation | S10 ch12, claim 1 | **No substrate.** `hopWeight = 0`, `hopCount` passed as literal `0`, and the MAC rejects any frame that has traversed a hop by design (S5 §2) |
| Identity-scoped prioritisation and arbitration | S10 ch12, claim 3 | **No substrate.** `QUERY`/`RESPONSE` are defined in the header but never constructed or sent; one tag in the scenario; `U(r)` scored offline only (S5 §0, §3) |

Two things follow, and they pull in opposite directions.

**First:** the thesis cannot be written as specified. Chapter 10 §10.7 is headed *"The central contribution of this thesis"* and Chapter 12 carries claim 3. One has been measured negative in the configuration tested; the other has never had an implementation to test. A Phase 1 document that specifies these as contributions, submitted alongside a repository that has measured them, is internally inconsistent even though Phase 1 is permitted to contain no results.

**Second:** the falsifications are narrower than they read, and three of them have named, unrun experiments attached that could reverse them. The integrity gate was never shown a fault it was designed to catch — every firing measured is on natural channel variation (S4 §7.10 item 5). The cost metric was registered for an architecture whose value proposition is reliability, and the two select for opposite conditions (S2 §7). The one surviving reliability result — 19/20 vs 0/20 deployments in forest — carries **no verdict** because no reliability criterion was ever registered (S2 §4.1).

That is the branch point this plan is built around.

---

## 2. Built versus specified — the conformance matrix

### 2.1 Layer by layer

Status codes: **BUILT** implemented and exercised · **PARTIAL** implemented but not exercised on the path the spec describes · **ABSENT** no implementation · **DIVERGENT** implemented differently from the specification.

| Blueprint element | Spec § (S10) | Repository state | Evidence | Status |
|---|---|---|---|---|
| Tier 1, 868 MHz propagation + ranging | ch09 §9.3 | `LoRaPhysicsEngine`, ~35 submodules, `r80` 1515.3 m | S1 §2.1, §4.2 | BUILT |
| Tier 1 observable = **TDoA multilateration (Chan, Foy)** | ch09 §9.3 | **RSSI inversion only.** No timing layer, no time-sync, no TDoA solver | S1 §6.1 | **DIVERGENT** |
| Tier 2, 2.4 GHz symmetric two-way ToF | ch09 §9.4 | `LoRa2p4PhysicsEngine`, floor 3.0 m cited from Zhou et al. 2025 | S1 §6.1–6.2 | BUILT |
| Tier 3, UWB DS-TWR | ch09 §9.5 | `LoRaUWBPhysicsEngine`, three-message TWR, floor 0.10 m cited from Benouakta et al. 2023 | S1 §6.3 | BUILT |
| CIR leading-edge NLOS detection | ch09 §9.5 | NLOS modelled as a probabilistic state; **no CIR, no classifier** | S6 §1.1 | ABSENT |
| Anchor bootstrap, GPS-PPS synchronisation, GDOP | ch09 §9.6 | GDOP present (PCS term 2). **No synchronisation layer** — not needed, because there is no TDoA | S1 §2 | ABSENT (consequent) |
| Sparse-anchor geometry resolution (Case A/B/C) | ch09 §9.7 | Fusion requires ≥ 3 anchors; otherwise coasts on prediction | S6 §3.1 | ABSENT |
| EKF predict/correct | ch10 §10.3–10.4 | Four-state (position, velocity), sequential scalar updates, process noise 1.0 m/cycle | S2 §2 | PARTIAL |
| **Inertial dead-reckoning prior (IMU)** | ch10 §10.3.1 | **Engine ported, driver does not track, prior still not wired.** 17 SHS (step-and-heading) files lifted from `origin/IMU_SHS_V1.1` (`0b563fb`), off by default. Its own header refuses the EKF prediction step: the integrity check was derived against a strapdown prior whose error model does not apply to an SHS one. The gait driver synthesised here does not track — 330 m against truth, because the mobility model turns instantaneously at waypoints and a step-and-heading system cannot follow a step-function turn. The EKF prior remains constant-velocity. Separately, an IMU *consistency witness* is built and exercised (`--imuCrossCheck`), and fires 1 refusal against corroboration's 16 | `docs/DESIGN_DECISIONS.md` §8; `0b563fb` | **PARTIAL** (2026-09-07) |
| Sequence-number supersession | ch10 §10.5.1 | Absent | S6 §3.1 | ABSENT |
| Covariance Intersection fusion | ch10 §10.5.2 | Absent — requires multi-path fusion, which requires mesh | S6 §3.1 | ABSENT |
| **PCS composition with weights w₁,w₂,w₃** | ch10 §10.6.4–10.6.5 | **Unweighted sum** of three [0,1] terms against threshold 1.55/1.70/2.30. The absent weighting is the direct cause of the saturation defect | S2 §2, §6.2 | **DIVERGENT** |
| PCS weight determination by LOOCV | ch10 §10.6.5 (CAL-01) | **Never implemented.** Thresholds were hardcoded magic numbers; v2 replaced the form rather than fitting the weights | S6 §3, S4 §0.2 | ABSENT |
| Switching-Point Integrity Check | ch10 §10.7 | **Two forms, and they must not be conflated.** (a) The Mahalanobis test `D² = dᵀ(P_prior + R_fix)⁻¹d` vs χ²₀.₉₉ = 9.21 — built, measured negative, and under the corrected topology it has an **empty eligible population**: it runs on all-LOS cycles and there are 0 of 1431. It validates a fix against the filter's own prior, so the filter that produced the confidence also supplies the yardstick. (b) The form the brief actually specifies — cross-validation against **independent** evidence before a band switch — built 2026-09-07 (`68c6996`) as two suppress-only witnesses, IMU dead-reckoning consistency and neighbour corroboration. Measured 8 paired seeds: energy −15.360 J, 95% CI [−21.433, −9.233], 8/8; accuracy −0.743 m, CI [−1.052, −0.416], 7/8. **Both axes at once, both intervals excluding zero.** The effect is corroboration alone; the inertial witness barely fires | `report/INTEGRITY_CHECK_RESULTS.md`; `docs/TECH_DEBT.md` §I.1 | **BUILT** — (a) negative, (b) positive |
| Integrity check against **environmental fault classes** | ch10 §10.7.5 (TABLE 10.8) | **Built and run.** `--injectRangeBias` (common-mode metres), `--injectFaultAnchors` (0 = F3 common-mode, 1 = F1 single-anchor RAIM model), `--injectBiasFraction` (contiguous window, seed-derived start, identical across arms). 162 results directories | `ca_tbl_causal_sim.cc:3345-3364`, `:2496-2510`; `results/abl_clean`, `results/c9_clean` | BUILT |
| Escalation / de-escalation with hysteresis | ch10 §10.8 | Built and **now exposed**: `--pcsHysteresisBand` and `--nDeescalate`, `f578f16`. The band is the swept quantity rather than the two absolute thresholds, so hysteresis moves without also moving the operating point. Defaults reproduce `baseline_v7` byte-for-byte | S1 §9.2; `f578f16` | **BUILT** (2026-09-05) |
| Estimation runs on the **Anchor/Gateway Node** | ch10 §10.1 | **On the gateways by default since 2026-09-29** (`--gatewayEdgeServer`, L.2.3). The node broadcasts, the gateways measure and report over the backhaul, and the fix comes back in one downlink per fix — which is what the node is billed, with no per-gateway reply. The filter's inputs and arithmetic are unchanged; what moved is where it is modelled to run, and so what the node pays. At defaults only energy moves: B3 C6 found all seven accuracy columns byte-identical on 20/20 seeds and totalJ lower by 42.5 % (`static_868`) and 32.9 % (`fixed_threshold`, `ca_tbl_full`), while `static_2p4` costs 0.2 % more. **Why PARTIAL and not BUILT:** getting a tier-2 or tier-3 range from the exchange that measured it to the gateway's estimator is not billed. Node-side estimation, the CONTRADICTION this row recorded until then, stays reachable as `--gatewayEdgeServer=false` and reproduces every earlier result | `report/B3_REGEN_RESULTS.md` C6; `docs/TECH_DEBT.md` L.2.3 | **PARTIAL** (2026-09-29; was CONTRADICTION) |
| Duty cycle + APC, G.S.R. 853(E) Table-II | ch11 §11.1–11.2 | Built and enforced. `duty_868` = 0.0226 against the 0.025 cap | S1 §7.5 | BUILT |
| Time-on-air per SF/BW/payload | ch11 §11.3 | Computed from the Semtech formula; measured tier-1 airtime matches computed ToA to 5 s.f. | S1 §7.6, §8.4 | BUILT |
| CAD-based CSMA/CA, priority contention windows | ch11 §11.4–11.5 | Built. 200 k TX / 40 k collisions / **14 564 capture events** — capture is emergent, not drawn | S1 §7.4 | BUILT |
| 9-byte MAC header, 9 message types | ch11 §11.9 | `CaTblHeader`, validator 38/38 | S1 §7.2–7.3 | BUILT |
| **`ESC_REQUEST` / `ESC_GRANT` on the wire** | ch11 §11.6 | **Now sent**, behind `--sendHandshakeFrames` (default off), `ac41f6c`. `DeliverRx` gained the receive-side dispatch it never had; `ESC_REQUEST`→`ESC_GRANT` and `DIGEST`→`ENTRY_PUSH` are constructed on the live path for the first time. The charge is KEPT and the frame ADDED — replacing it would transmit uncharged frames, since the MAC has no TX callback | S5 §3; `ac41f6c`, `4c0b175` | **BUILT** (2026-09-05) |
| `QUERY` / `RESPONSE` / `ENTRY_PUSH` on the wire | ch12 §12.4, ch11 | **All three now constructed and answered on the live path.** `ENTRY_PUSH` via the `DIGEST` exchange (C1); `QUERY`/`RESPONSE` via `SendQuery()` and the responder in `DeliverRx` (C3), behind `--identityQueryPeriodS` (default 0). The header field the query needs is carried and not serialised, so airtime is unchanged | S5 §3 | **BUILT** (2026-09-06) |
| Mobility-state-conditioned message selection | ch11 §11.11 | `m_mobility` is never set to `CA_TBL_MOB_RECEDING`; the receding branch is dead code | S5 §3 | PARTIAL |
| Multi-node PHY substrate (a second tag that can be heard) | ch12 §12.1 | **Built.** `EnginePhyProvider` carried one node id and one position and returned early below the MAC, so a peer's frames counted as sent, lost nothing to duty, CAD or collision, and were never delivered. It now carries a position per node: +112k channel deliveries and link evaluations 518k->955k at `--nTags=2`. Null control reproduces all seven `baseline_v7` digests | `report/C4_PROVIDER_BLOCKER.md` | **BUILT** (2026-09-05) |
| Mesh routing, hop count, confidence-weighted path selection | ch12 §12.2–12.3 | **Hop-provenance BUILT; multi-hop routing absent.** A relayed range is admitted under A1–A5 and weighted by `σ_eff² = σ_range² + uᵀP_B u`, behind `--relayedRanging` (default off): 78/31/11/11 admitted at seed 7, σ 3.0 m → 39–52 m inflated, and A1 rejects 54/40/11/11 when forced onto tier 1. **Routing beyond one hop is not built and is bounded by the model, not by effort** — A2 caps depth at 1, because at depth 2 the relay's own position is itself relayed and `P_B` understates its error by exactly the term this model adds. **2026-09-08: the confidence half is now live.** The provenance term reaches the score (`57ebb41`) and has a non-zero input for the first time (`edf73a5`, which fixed an accumulator cleared between its two contributors): relayed variance share 0.20 over 180 cycles, and `--hopProvenanceForm` 0 vs 1 now change the run (37 vs 40 escalations). Whether it carries information the covariance does not is a separate campaign | `docs/RELAYED_RANGING_ERROR_MODEL.md`; G15; `docs/TECH_DEBT.md` §G | **PARTIAL** (2026-09-08) |
| Identity-scoped query and proxy response | ch12 §12.4 | **Scoping built and measured; proxy response not built.** The scoping is at the RESPONDER: a query floods, every target hears it, only the one whose `targetManifestId` matches answers. 36 cells (3 seeds x 6 arms x 2 directions), discrimination perfect in every one — naming the tag gives 23-34 answered / 0 ignored, naming the peer gives 0 answered / 23-34 correctly ignored. A **proxy** answering on another node's behalf is a different mechanism and is not implemented | S5 §3; `report/C3_C4_ON_PROVIDER.md` | **PARTIAL** (2026-09-06) |
| Identity-scoped UWB arbitration | ch12 §12.6 | **Built, and both contended paths execute.** `U(r)` ported into `CaTblMac::ArbitrateRequest()`, behind `--uwbArbitration` (default off). Anchors see requests from both manifest ids; **deny 148-248 and pre-empt 75-201** on the arms that reach UWB, against 0/0 before. The three static arms are the internal control and correctly show one requester and zero of each | S5 §0, §1; `report/C3_C4_ON_PROVIDER.md` | **BUILT** (2026-09-06) |
| **Position dissemination — the anti-entropy exchange** | ch12 §12.4, ch13 | **BUILT 2026-09-08.** The table entry `{pos, Σ, timestamp, seq_no, hop_count, source_type, velocity}` was billed for since S4 and carried nothing; it now carries contents, merges under modular supersession, and travels **both ways**. Before `cf25341` only searchers asked, so 4/4 finders knew where the casualty was and the casualty knew nothing about them. The casualty now digests on its **active tier** (which keeps the §6 one-tier-per-cycle invariant) and a node reads the **union of its own three per-tier tables**, since knowledge is not tier-scoped. Casualty holds 3.3 of 4 finder entries at 79 s mean age, from 0 | `cf25341`; `report/EVIDENCE_GATE_AND_EXCHANGE.md` | **BUILT** (2026-09-08) |
| **Finder position knowledge — GNSS error modelled** | ch09 §9.6, ch10 | **BUILT 2026-09-08.** Finders were handed to the filter as *perfectly known* points while their published entry claimed a 4.0 m² variance — the claim and the value disagreed, and the value was the flattering one. `--finderPosSigma` (default 0.0, self-consistent at 2.0) gives a searcher a real receiver error as a per-finder constant offset drawn from its own RNG stream; it steers by what it believes and walks in the real world. A finder is then the pseudo-anchor C5 already defines, with its variance inflated by position uncertainty **and by entry age** (`f88e224` — omitting age drove rmse 7.44 → 86.71 m). Also fixed a plain bug: a finder published its spawn position once and then walked for the whole run | `5262c59`, `f88e224`, `937a55a` | **BUILT** (2026-09-08) |
| **UWB coverage evidence for escalation** | ch10 §10.8 | **BUILT 2026-09-08, after a false start.** The gate predicted tier-3 coverage from a **static anchor map** snapshotted at scenario start — right when tier 3 sat in fixed rings, wrong once UWB comes from finders that move and are never in it. The first replacement (`40e3258`) read the UWB neighbour table, which **can never be non-empty before escalating**, because an unused radio sleeps and its table is cleared each cycle: a permanent veto on tier 3, byte-identical to the static map on all 8 seeds. Corrected (`c236834`) to predict from **measured range on the radio actually awake**. Tier-3 cycles 0.0 → 5.4. The −66 J belongs to `--escalationRequiresEvidence`, **not** to this flag | `c236834`; `report/EVIDENCE_GATE_AND_EXCHANGE.md` §2 | **BUILT** (2026-09-08) |
| Transport / application layer, session lifecycle | ch13 | Absent | S6 §3.1 | ABSENT |
| Per-action energy accounting | ch14 §14.10 | Built. Reconciles to datasheet currents to 10 significant figures | S1 §8.5 | BUILT |
| Fault injection | ch14 §14.15 | Built; see the row above. One bias magnitude (40 m) and one window fraction (0.2) tested | `results/*/run_id.txt` | BUILT |

**Count, recomputed 2026-09-29 from the table itself** (parsed from the status column). Of 34 rows: **19 BUILT**, **6 PARTIAL**, **2 DIVERGENT**, **0 CONTRADICTION**, **7 ABSENT**. The one change since the 2026-09-08 count (19 / 5 / 2 / 1 / 7) is the estimation-location row, CONTRADICTION → PARTIAL, under L.2.3. **No other row was re-audited for this count.** Code has landed since 2026-09-08 behind rows that still read as they did then — the tier-1 observable (`--tier1Tdoa`), the CIR classifier (`--nlosClassifier`), Case A/B/C, PCS weights by LOOCV (CAL-01) — so treat those rows as dated, not current.

> The line this replaces read *"Of 30 rows: 11 BUILT, 4 PARTIAL, 2 DIVERGENT, 1 CONTRADICTION, 12 ABSENT"* and
> disagreed with the table above it on the row count and on four of five tallies. It was written by hand and never
> recomputed as rows moved. **This is the staleness failure §8 of `RULES.md` warns about, inside the document that
> tracks it** — and it is the second instance recorded in this section. The count is now derived by parsing the
> table's own status column; regenerate it that way rather than editing the number.

The 7 ABSENT rows distribute as: 3 in Ch. 9 (CIR classifier, GPS-PPS synchronisation, sparse-anchor geometry), 3 in Ch. 10 (sequence supersession, Covariance Intersection fusion, PCS weight calibration by LOOCV), 1 in Ch. 13 (transport/session layer). **Chapter 12 still has no ABSENT row.** The IMU prior left this list on 2026-09-07 and is now PARTIAL rather than BUILT: the SHS engine is ported and its driver does not track, which is a different and smaller gap than having no inertial model at all.

**What is still ABSENT is not evenly weighted.** Sparse-anchor geometry and Covariance Intersection matter only once multi-hop fusion exists, which A2's depth cap forecloses. The two that bite now are the **transport/session layer** (the brief's SEARCH_AND_LOCALIZE orchestration, without which multi-target search is a shell script) and **PCS weight calibration by LOOCV** — CAL-01, the mechanism the blueprint actually specified, which was never implemented and whose absence is the direct cause of the saturation defect that A8 closed by result.

**Chapter 12 previously had zero substrate. It now has four rows built and one partial**, and the reason the whole chapter moved at once is that a single defect was holding all of it: the PHY provider modelled one field node, so no second target could be heard no matter what was built above it. **Its one remaining ABSENT row is mesh routing / hop-provenance (C5)**, which is gated on the relayed-ranging error model — now written (`docs/RELAYED_RANGING_ERROR_MODEL.md`) — and on enabling tag-to-tag links, which is deliberately still off: the link budget would evaluate, but a relayed range is not admissible until the model that weights it is wired in.

> **Correction, 2026-09-04.** Two rows above were first written as ABSENT on the strength of S4 §7.10 item 5 (*"S7 environmental fault injection. Not implemented, no flag."*), which is dated 2026-08-05. That note is **superseded**: the harness was built and the campaigns ran on 2026-08-27. Verified against the flag registrations at `ca_tbl_causal_sim.cc:3345-3364`, the bias application at `:2496-2510`, and 162 `run_id.txt` sidecars. The F1/F3 columns in S2 §5.1 are those results. **This is a live instance of the project's own staleness problem** — a superseded status note in a working document propagated into a plan. Any agent reading S4 must check the flag table and the results tree before treating a *not implemented* note as current.

### 2.2 Naming and terminology divergence

`terminology_lock.md` (S9) is marked **LOCKED** and its left column is mandatory. The repository violates it throughout. This is not cosmetic: the lock exists so a reader can follow one system across fifteen chapters, and the repository is the source most of Chapters 9–15 must be written from.

| Locked term (S9) | Repository uses | Occurrences |
|---|---|---|
| **CA-DBL** | **CA-TBL** ("Confidence-Aware Tier-Based Localization") | Every file, every commit message, the repository name itself |
| Field Node | "field tag", "tag", "node" — all three are on the forbidden list | `Architecture.md` §2.3, throughout |
| Anchor/Gateway Node | "gateways", "anchors", "facility sites" — note that since 2026-09-07 "anchor" means *any node reachable on a tier*, and facility sites host none | `Architecture.md` §3 |
| Tier 1 (LoRa 868 MHz) | "868 MHz", "tier 1", "static_868" | Throughout; arm names are baked into 89 results directories |
| progressive handoff | "handoff", "escalation" used interchangeably for the whole mechanism | `Architecture.md` §9 |
| Position Confidence Score (PCS) | "confidence score", "PCS" | Consistent enough |
| Switching-Point Integrity Check | "integrity gate", "integrity check", "evidence gate" (a *different* mechanism) | `Architecture.md` §9.3 vs §2.3 |

**The `CA-DBL` / `CA-TBL` question is a real decision, not a find-and-replace.** *Dual-Band* is factually wrong for a three-tier system — the acronym predates the third tier. *Tier-Based* is accurate. But `CA-DBL` is what the locked terminology, the blueprint and every chapter scaffold use, and the change propagates into `notation_table.md`, `glossary.md`, `acronym_list.md` and every drafted heading. Decide once, log it as `TERM-CHANGE-01` per S9 §10, and propagate. See DEC-A.

**The `evidence gate` collision is the more dangerous one.** `Architecture.md` §9.3 defines an *evidence gate* — a suppress-only quality-and-coverage precondition on escalation to tier 3 — which is a **different mechanism** from the Switching-Point Integrity Check of ch10 §10.7. Both are called "the gate" in places. Any thesis text drawn from the repository must keep them distinct or it will attribute one mechanism's measured behaviour to the other.

### 2.3 Architectural contradictions

These cannot be documented away. One of the two sides must change.

**C1 — Where estimation runs.** ch10 §10.1 states, normatively: *"Estimation runs on the Anchor/Gateway Node, not the Field Node. State the reason."* The implementation runs `CaTblPolicy` — EKF, PCS, integrity gate, escalation FSM — **on the field tag** (S1 §2.3 block diagram). These imply different energy models, different message flows, and a different privacy and identity story. The energy model as built charges the tag for its own ranging and its own handshake; a tag that does not estimate would not need the same telemetry uplink, and an anchor that does estimate needs a downlink to tell the tag which tier to use next. **Resolve before Chapter 9 or 10 is drafted.**

> **Resolved in the code's default, 2026-09-29 (`docs/TECH_DEBT.md` L.2.3).** `--gatewayEdgeServer` now defaults true: the specification's side was kept and the implementation moved. The tag is billed one tier-1 broadcast and one downlink per fix, and no per-gateway reply. What remains is recorded in the §2.1 row as PARTIAL: a tier-2 or tier-3 range still has to reach the gateway's estimator, and nothing bills that.

**C2 — Tier 1's observable.** ch09 §9.3 specifies TDoA multilateration with Chan and Foy solvers. The implementation is RSSI inversion. This was flagged as blueprint decision D5 and never closed. It has consequences beyond Chapter 9: an RSSI-inversion sigma is proportional to range (`estD · ln10/(10n) · shadowingStdEff`), which is what produced the ~50 %-of-distance error signature and the tier-1 calibration question in S3 §13.3. It also means the CRLB gate was applied to tier 1 for the whole project, which finding B3 of S6 calls a category error in the ranging submodule's own comment.

**C3 — The prior in the predict step.** ch10 §10.3.1 specifies an inertial dead-reckoning prior and draws its derivation from the IMU source material (blueprint rank 3, "essentially complete"). The implementation has a constant-velocity prior and no IMU. The Switching-Point Integrity Check of ch10 §10.7.2 requires an **independent prior construction** — the whole point being that the prior is independent of the radio fix it is testing. A constant-velocity prior propagated from previous radio fixes is *not* independent in the sense the specification requires. **This weakens the integrity check's measured falsification as evidence about the specified mechanism**, and it cuts both ways: the thing that was measured is not quite the thing that was specified.

**C4 — Radio part basis.** ch09 §9.2 and the lock specify **LR1121** (868 + 2.4 GHz, one die) plus DW3000-class. The energy model uses **SX1262** and **SX1280** datasheet currents (S1 §8.2), with LR1121 cited only for the dual-band feasibility claim. This partially closes register item PROV-03 (which recorded an SX1276 basis) and opens a smaller one in its place. Not a contradiction in physics — the parts are close — but the conformance delta must be stated, not silently inherited.

---

## 3. How the parts fit together

### 3.1 The spine: the causal per-cycle loop

Everything in the system hangs off one loop, and its invariant is what makes the experiment valid.

```
cycle k
  1. node moves; all engines' node positions updated
  2. range on the ACTIVE TIER ONLY — the tier selected at cycle k−1
  3. charge energy for that tier only
  4. if active tier delivered ≥ 3 ranges → use them
     else → floor on demand: range tier 1 as fallback (same cycle, zero added latency)
  5. EKF update from multilateration over the active tier's anchors
  6. compute PCS
  7. escalate?  → evidence gate (quality AND coverage) → escalate, charge handshake on the tier being left
     no?        → N consecutive healthy cycles? → de-escalate
  8. inactive radios sleep, charged at sleep current
cycle k+1
```

**The invariant:** exactly one tier transmits per cycle, and only that tier is charged. An arm that never escalates accrues exactly zero energy on the tiers it never used (S1 §9.1).

This is load-bearing and it was not always true. An earlier pipeline generated all three tiers' data unconditionally and selected afterwards, under which *the energy saved by not escalating was structurally unmeasurable* (S6 §2.1). Fixing that was step S1 of the build order, and it is why claim 1 became testable at all. **Any new work package that moves a transmission outside this loop, or charges a tier that did not transmit, breaks the only thing that makes the energy axis mean anything.** This is the first thing to check in any change review.

### 3.2 Module dependency graph

```
                    ┌─────────────────────────────────────────┐
                    │  scratch/ca_tbl_causal_sim.cc           │
                    │  geometry · mobility · arms · sweeps    │
                    │  CaTblChannel (shared medium, capture)  │
                    └───────────────┬─────────────────────────┘
                                    │ owns one of each, per tag
        ┌───────────────────────────┼───────────────────────────┐
        │                           │                           │
   CaTblPolicy                 CaTblMac                 CaTblEnergyModel
   EKF · PCS ·            CSMA/CA · duty governor ·     per-action joules
   evidence gate ·        neighbour table · 9 msg       from datasheet
   integrity gate ·       types · APC                   currents
   escalation FSM              │                              │
        │                      │ CaTblHeader                  │
        │                      │ (wire sizes → ToA → duty)    │
        │                      │                              │
        └──────────────┬───────┴──────────────────────────────┘
                       │  "active tier for next cycle"
        ┌──────────────┴───────────────┬──────────────────────┐
   LoRaPhysicsEngine       LoRa2p4PhysicsEngine      LoRaUWBPhysicsEngine
   868 MHz                 2.4 GHz                   UWB 7.99 GHz
        └──────────────────────────────┴──────────────────────┘
                    ~35 submodules each, PhysicsResult ≈ 150 fields
                                    │
                    ┌───────────────┴─────────────────┐
              arm_summary.csv                  trace_<arm>_seed<N>.csv
              run_id.txt (commit, dirty count, digests)
                                    │
                    ┌───────────────┴─────────────────┐
        verify_causal_sim.py                  scripts/catbl_gates.sh
        141 checks / 161 gates                G1–G11
```

**Three couplings are easy to miss and each has already cost the project a wrong result.**

1. **Wire size → time-on-air → duty cycle → handoff latency → metres of error.** The 9-byte header is not cosmetic. Tier-1 ranging ToA is 56.58 ms at SF7/125 kHz/CR4/5/20 B; at the 2.5 % cap that arms 2.263 s of off-time against a 2.5 s cycle; a handoff needs two transmissions; therefore **a handoff costs a ranging slot** (S1 §7.6). Change a payload size and you change the accuracy result.

2. **Sensitivity tables sit outside the loss sum.** The six-decibel indexing error (`sf - 7` against tables whose index 0 is SF5) lived in the sensitivity tables on the tier carrying ~95 % of active cycles, and **passed every gate, verifier check and validation in the project** (S2 §5.4b, §6.1). The fidelity checks verify ranging precision and energy accounting, both of which are downstream of whether a packet was received at all. `effectiveSensitivity` is additionally reduced by `linkHysteresisMarginDb` (1.5 dB) whenever the link was previously receiving, so the comparison threshold moves with the outcome (S1 §5.1). **An audit that enumerates the loss sum will not find either.**

3. **Arms share the RNG stream but not its consumption rate.** Each arm consumes the stream at its own rate because which tier is polled depends on the arm's own behaviour — two arms with identical filter state reached 334 and 336 engine evaluations by t = 2.5 s (S1 §11.5). **Enforced rule: a single-seed cross-arm difference is never reported as a result.** Any new work package that adds transmissions perturbs this for every arm, which is why every new mechanism must sit behind a default-off flag with a byte-identity test.

### 3.3 The verification apparatus, and why it is the asset

Six layers: physics validation (CRLB, probe sweeps, shadowing decorrelation, 246 empirical artefacts) → protocol validation (header validator 38/38, capture emergence, governor enforcement) → energy validation (10 s.f. datasheet reconciliation) → results verifier (141 checks / 161 gates) → gate suite G1–G11 → verifier self-test (11 cases confirming the verifier fails on every defect it should catch).

Every gate has a negative fixture built in `/tmp`, and **every one has been observed to fail** (S2 §3). Provenance is recorded per run: commit, count of modified tracked files, build profile, and a SHA-256 digest of each output written after the last stream closes. `dirty 0` means the binary is rebuildable from a named commit. 1 065 sidecars are tracked; ~13 GB of per-cycle traces are excluded and regenerate from the seed (S7).

**This apparatus is the strongest part of the work and it is technology-independent.** It survives every falsification in §1 intact. Track B is built on that observation.

---

## 4. Gaps

### 4.1 Unbuilt subsystems, with the reason each is unbuilt

| Gap | Blocked by | Reachability | Source |
|---|---|---|---|
| Second tag in the scenario | The field-node stack is one of everything inside `RunArm()`: `nodeId = 1` as a compile-time constant, one `CaTblEnergyModel`, one `CaTblPolicy`, three `CaTblMac`. The MAC and channel layers are **already multi-node** — every anchor has its own `CaTblMac` on the shared channel | **Reachable.** Mechanical refactor of `ca_tbl_causal_sim.cc` only; no `src/` change found necessary. The risk is proving `--nTags=1` byte-identity, not the refactor | S5 §1 |
| `ESC_REQUEST` / `ESC_GRANT` as real frames | The handshake is an accounting call, not a transmission. Needs a receive-side dispatch on `GetMsgType()` and grant/deny logic at anchors | **Reachable and cheapest.** Isolates cleanly behind `--sendHandshakeFrames=false`, which reproduces the existing baseline | S5 §3 |
| `QUERY` / `RESPONSE`, identity-scoped search | Never constructed. Needs the message dispatch above **and** more than one target to scope between | Reachable, gated behind the two-tag work | S5 §3 |
| Multi-target arbitration | `U(r) = λ₁T_deg + λ₂T_batt + λ₃T_stale + λ₄T_query` exists in Python; no scenario instantiates a second target | Reachable, gated behind the two-tag work | S5 §0 |
| Hop-provenance confidence | `NoteNeighbour()` rejects hopped frames **deliberately**: a relayed frame carries the *originator* in `origin_id`, so admitting it inserts a peer the node may not hear into the poll set, and every subsequent poll burns a full `rxTimeoutS` — 0.93 mJ on UWB, ~3× a successful exchange. Requires either relayed ranging with a composed cross-hop error model, or multi-hop neighbour admission **plus** per-peer reachability tracking | **Assessed out of reach for this project** by the scope document itself. The reachability half is the hard part and it is gated behind the two-tag work | S5 §2 |
| Detect-and-escalate under injected fault | The harness exists and ran, but **all 160 measured cells set `suspectForcesEscalation=false`**, isolating rejection alone. Zero runs combine injection with `=true` | **A flag flip, not a build.** See A2 | `run_id.txt` census, §6 A2 |
| Injected-fault magnitude and window sweep | Only one bias magnitude (40 m) and one window fraction (0.2) have been run | Compute only | `run_id.txt` census |
| Hysteresis / `nDeescalate` sweep | Parameters exist but are not exposed by `AddValue` | Reachable, trivial | S4 §7.10 item 4 |
| IMU model and inertial prior | Nothing exists | Reachable but substantial; see C3 | §2.3 |
| TDoA timing layer and solver | Nothing exists | Substantial; see C2 | §2.3 |

### 4.2 Live defects — carried knowingly

| ID | Defect | Status | Consequence |
|---|---|---|---|
| D-1 | **PCS unweighted-sum term saturation.** Three terms each bounded [0,1], summed without weights against 1.55. The uncertainty term tends to 0 as covariance grows, so geometry and LOS fraction alone hold PCS above threshold regardless of how uncertain the filter is. It can fail to contribute; it can never veto | **REPAIRED 2026-09-05**, behind `--pcsForm={weighted,gated}`, `5f1ff78`. Default `legacy` is unchanged and reproduces `baseline_v7` byte-for-byte, so the results tree is NOT invalidated — the repair is opt-in. Measured over 20 paired seeds: `gated` improves `cep50M` by 3.799 m on `ca_tbl_no_integrity` on 20/20 seeds for 28 J; `weighted` 3.848 m for 142 J. **But every form still loses to the monotone-of-filter-statistic control**, so the defect is fixed and the confidence-score claim is closed. `report/PCS_FORMS_RESULTS.md` | Measured: `term1` 0.0006, `term2+term3` = 1.9173 against threshold 1.55, **0 escalations**, 1440/1440 cycles on tier 1, `cep50` 89.91 m — with NEES 1.95, a correctly calibrated filter whose confidence metric cannot express its own uncertainty (S2 §5.5) |
| D-2 | **`LoRaDistributionSampler` ignores its seed.** 5 random variables created, 0 `SetStream` calls | **Decision not to fix**, recorded | Documentation is accurate; determinism holds at defaults and at six non-default flag settings, so no result is known to be affected. But it is an unseeded draw in a project whose whole evidence model rests on determinism |
| D-3 | **CRLB gate applied to tier 1** (audit finding B3). The ranging submodule's own comment states this is a category error — tier 1 is RSSI inversion, not a time-of-arrival estimator | **ALREADY FIXED — this row was stale.** `ca_tbl_causal_sim.cc:1632` skips tier 1 and prints "CRLB: NOT APPLICABLE … that was a category error", and the one-sided pass string is corrected too. Verified 2026-09-05; no change was needed. `docs/WORK_PACKAGES.md` still lists D-3 as READY priority 3 and is also stale | The measured tier-1 ratio of 3.319 compares unrelated quantities and has been reported for that tier throughout. Also a reporting defect: `worstCrlbRatio` is tracked as a minimum and tested one-sided, but the pass string asserts a two-sided property |
| D-4 | **Terrain hardcodes 868 MHz** (`lora-terrain-submodule.cc:69-70`) and applies that wavelength to every tier, UWB at 7.99 GHz included | **FIXED 2026-09-05**, behind `--terrainPerTierFreq` (default off), `ea900d2`. `TerrainParams` carries `frequencyHz`, defaulting to 868e6 so every tier keeps the wavelength it always had; the flag sets 2.4 GHz and 7.9872 GHz on tiers 2 and 3. Makes terrain LESS WRONG, not validated — it is still transcription-only and G11b is still permanently red | The engine's terrain does not scale with frequency. Both headline conclusions were shown invariant to it, and G11b is **permanently red** — terrain is transcription-only and was not rescoped |
| D-5 | **Receiver sensitivity is unverified.** No check in the project would catch the six-decibel defect today | Open; the closing check is specified in S2 §8.4 | Packet-level physics is verified for ranging precision and energy accounting, and **unverified for receiver sensitivity**. Claim 7 is re-scoped from PROVEN on this basis |
| D-6 | **G3 fails by design.** The integrity check fires once at seed 7, so byte-identity between the ablation arms is no longer available | Known, documented in the README | The divergence begins at the firing cycle and not before, which is the treatment effect rather than RNG contamination — but a reader must be told, or a red gate reads as a broken build |
| D-7 | Two unattributed propagation residuals: **UWB ~3.7 dB** (eight candidates ruled out, saturating by 20 m) and **868 MHz ~3.5 dB near-field** (six candidates ruled out, shrinking monotonically as `nlos_frac` rises) | **Deliberately open** | Conclusions were drawn by recomputing under ±4 dB of margin offset chosen to bracket exactly these residuals |

### 4.3 Measurement gaps — experiments named but not run

Ordered by value, highest first. Each has a written plan or a derivation already on disk.

| ID | Experiment | Why it matters | Source |
|---|---|---|---|
| M-1 | **Detect-and-escalate under injected fault.** Re-run the `abl_clean` / `c9_clean` matrix with `--suspectForcesEscalation=true`, and sweep `--injectRangeBias` and `--injectBiasFraction` beyond the single tested point (40 m, 0.2) | The harness exists and the rejection-only arm is falsified in 8/8 cells. **The combined mechanism has never been run under injection** — and it is the one that moves `cep50M` 92.15 m → 3.10 m under propagation NLOS. This is the last untested cell of the thesis's stated central contribution, and it costs compute, not code | `run_id.txt` census; S2 §5.1 |
| M-2 | **Intra-cycle handshake scheduling.** Deferring the handshake ≥ 2.263 s after the ranging within a cycle should eliminate the pre-emption at any `dt ≥ 2.32 s` at no accuracy cost | Derived from the lockout arithmetic, not run. Named as the strongest Phase 8 candidate. Would convert a regulatory finding from a constraint into a mitigation | S4 §7.10 item 1 |
| M-3 | **`--dwell` sweep at b3000.** Coverage fraction, not gateway density, is the variable that tracks the conjunction result: b1000/gw3 and b3000/gw9 have identical density (2.25 gw/km²) and differ **8×** in median error. Restoring b1000's coverage fraction at b3000 needs `dwell ≈ 245 s` | `--dwell` already exists (default 90 s). **Testable over the existing binary with no code change**, and Part 7 did not notice this | S4 §8.2 |
| M-4 | **Tier-1 measurement calibration.** Open question whether tier-1 range uncertainty is over-stated relative to realised error. If it is, the filter is discounting exactly the measurements that would carry the upper-edge cycles | 96–99.6 % of inadequate fixes occur on cycles with fewer than three tier-2 anchors in range. A calibration fix could move the upper edge **with no siting change at all** — and is scheduled first precisely so the siting experiment does not answer a question calibration already answered | S3 §13.3 |
| M-5 | **Complementary siting** as a second treatment arm alongside the co-located control | Registered prediction: if `static_2p4` gains as much as the tiered arms, this is a deployment improvement **and must be reported as such**, not as an argument for tiering | S3 §13.3 |
| M-6 | **Hysteresis / `nDeescalate` sweep** | Requires exposing two parameters via `AddValue`. Never measured | S4 §7.10 item 4 |
| M-7 | **Sparse-end energy as a bracket.** At the sparsest densities the out-of-coverage search-fraction assumption accounts for up to **46.5 %** of single-band energy; below `spacing/r80` ≈ 1.55 it is ≤ 5 % | Removes the most obvious objection to half the energy table | S3 §13.4 |
| M-8 | **Constant-density Pareto at b3000** with `gwGrid = 9` | The existing b3000 Pareto carries a density confound | S4 §7.10 item 2 |
| M-9 | **A tier-2 fidelity check that reads the sensitivity tables** | Closes D-5. No check in the project would catch that defect today | S2 §8.4 |

### 4.4 Provenance and evidence gaps

| Gap | State |
|---|---|
| `[M-24]` — integrity detection separation ratios, F0 suspect rate, detection latency | Rests on `results/campaign_20260810/stage1`, which records **`dirty 4`**. Cannot be repointed: no injection campaign anywhere records `dirty 0`, and the two that now do (`c9_clean`, `abl_clean`) do not re-measure those statistics. Closing it needs a fifth campaign of roughly 3.8 hours. **Every other published number traces to a `dirty 0` source** |
| Nine probe-derived numbers | Untraceable at the time of the S6 audit — no probe stdout on disk newer than the build. Includes the tier-2 `r80` that sizes the deployment geometry. Load-bearing for interpretation, not for any verdict. Closing: re-run `--probe=true` and capture stdout |
| Six commit SHAs cited by 1 283 sidecars | Changed in the 2026-09-01 attribution rewrite. Sidecars were **not** edited — rewriting evidence would be falsification — and resolve through the `SHA_MAP.md` lookup table instead. The verifier's four-outcome resolution has a *does not resolve (history rewritten)* branch that lands them here |
| Tier-3 `r80` disagreement | `Architecture.md` §2.1 says 12.8 m; the technical report §2 says ≈ 13.5 m. One document is stale |

### 4.5 Thesis-document gaps

All 15 chapters are `SCAFFOLD` — headings, source bindings and pending markers, **no prose**. 43 open register items: 7 provenance defects, 7 unresolved decisions, 10 calibration parameters, 12 research tasks, 5 writing tasks, 4 novelty-exposure findings.

**Register items the repository has now answered, and which must be closed or rewritten:**

| Item | Original state (2026-07-30) | Answered by |
|---|---|---|
| Blueprint D1 (does Phase 1 contain real results?) | Open — 14 or 15 chapters | Superseded. The engine now has far more than a validated channel layer; the question is no longer whether Ch. 15 reports it but how Chapters 9–14 are written against a system that has been measured |
| Blueprint D2 (Ch. 14 scope) | Open — extend the engine, or narrow to Tier 1 | **Answered by construction.** All three tiers are built and exercised |
| Blueprint D5 (TDoA in the sweep matrix?) | Open | **Answered by omission** — the implementation is RSSI-only. Needs to be recorded as a decision, not left as a gap. See C2 |
| Blueprint D7 / DEC-06 (role of the ML dataset) | Open — three options | Still open. The repository's `ml_pipeline/` and the 5M-row dataset are not wired into the CA-TBL decision path |
| DEC-05 (Tier 2/3 simulation depth) | Open | **Answered.** Full protocol simulation for all three |
| PROV-03 (SX1276 basis vs LR1121/DW3000 spec) | Open | **Partially closed.** Basis is now SX1262 + SX1280 + DW3000; a smaller delta against LR1121 replaces it. See C4 |
| CAL-01 (PCS weights by LOOCV) | Deferred to Phase 2.1 | **Never done, and the reason matters.** The implementation used an unweighted sum, which is the direct cause of D-1. Then v2 replaced the functional form rather than fitting weights. The specified calibration has still not been attempted |
| CAL-03, CAL-04 (thresholds, hysteresis dwell) | Deferred to Phase 2.1 | Thresholds set by hand (1.55/1.70/2.30), then derived (v2, `0.0`). Hysteresis **never swept** — no CLI flag |
| CAL-05 (integrity threshold by precision/recall against fault injection) | Deferred to Phase 2.1 | Threshold fixed at χ²₀.₉₉ = 9.21. **Fault-injection matrix never built**, so no precision/recall exists |
| CAL-08, CAL-09, CAL-10 (path metric, arbitration weights, hop slack) | Deferred to Phase 2.1 | No substrate — Chapter 12's mechanisms are unbuilt |
| NOV-01 (hop-count degradation is well characterised in prior work) | Raised, unresolved | Now compounded: the contribution was to be *composing* that degradation into a transported confidence scalar, and there is no hop count to compose |
| NOV-03 (GNSS-free time sync, 30 ps TDEV, exists) | Raised, unresolved | Moot unless C2 is resolved toward TDoA |

**Still genuinely blocking and untouched by the repository:** RES-01 (Table 3.1, ≈ 84 cited cells), RES-02 (Table 4.1, ≈ 128 cited cells), RES-05 (13 unlocated LoRa-localization references), RES-06 (Ch. 1 statistics — nothing in the pool supports them), DEC-02 (institutional formatting template).

---

## 5. The decision that governs everything

Before any work package is chosen, one question must be answered, because the two answers imply different work and the wrong sequencing wastes weeks.

> **Is the thesis's contribution the confidence-driven escalation architecture, or the method by which that architecture was evaluated?**

The evidence for each:

**For the architecture (Track A).** Three of the falsifications are narrower than they read. The integrity check has never been shown its own fault class (M-1). The cost metric was registered for an architecture whose value is reliability, and S2 §7 demonstrates — not argues — that the screening rule and the verdict rule carried disjoint cell sets, with all ten adjudicable cells in the easiest environment. The one reliability result that stands (19/20 vs 0/20 deployments, 1.56× energy) has **no verdict attached only because no reliability criterion was registered**. Three named, cheap experiments (M-1, M-2, M-3) could each move a headline.

**For the method (Track B).** The apparatus is complete, self-tested, and independent of whether any claim passes: pre-registration with committed `PREDICTION.md` files timestamped before the first cell ran; a gate suite where every gate has a negative fixture and every one has been observed to fail; per-run provenance with digests; mechanism-aware degeneracy screening that refuses a verdict without a named mechanism and reports its own false-positive rate; and a dimensionless axis that survived a 26 % change in its own denominator. Four defects were found by the method itself — two by checks failing, two by results being arithmetically impossible — including a six-decibel physics error on the tier carrying 95 % of active cycles that **every fidelity check in the project passed**.

**The answer chosen here is: run Track A as a bounded, pre-registered attempt with hard abort criteria, and write Track B's framing in parallel so it is ready if Track A aborts.** Track B is not a consolation prize — S2 §4.3 already reports the dimensionless-axis transfer as *"a result about method, and it does not depend on any claim passing."* Track A's experiments are cheap enough (M-3 needs no code change at all) that not running them would be the more expensive choice.

**What must not happen:** drafting Chapters 9–13 as if the falsifications do not exist, then discovering at Chapter 14 that the specification and the measured system describe different things. Chapter 8, which states the novelty claims, is scheduled *after* Chapters 9–13 in the blueprint's own drafting order precisely so objectives match what was specified. That ordering now has to absorb what was **measured** as well.

---

## 6. Track A — bounded redesign

Every work package below carries: the problem, the change, the files, the isolation mechanism, the verification, the abort criterion, and what it unblocks. **Pre-register each before running it**, per the project's own standing discipline: predictions, falsifiers and abort conditions committed to a `PREDICTION.md` before the first cell runs.

### A0 — Register a joint criterion over availability and energy *(prerequisite for everything else)*

**Problem.** S2 §7: the project registered a **cost** criterion for an architecture whose value proposition is **reliability**, and the two select for opposite conditions. A cost win needs a qualifying competitor; a reliability win needs the competitor to fail. Demonstrated, not argued: the screening rule and the verdict rule carried disjoint cell sets, and the single PASS in 320 cells sat in a combination the screen had rejected. At `spacing/r80` = 1.02 the tiered arm meets the requirement in **82 % more deployments for 27 % more energy**, and M1 scores that FAIL.

**Change.** A criterion stating the trade-off in advance — for example a maximum acceptable energy multiplier per additional deployment meeting `(A ≥ 0.95, R ≤ 25 m)` — rather than one axis binarised into a qualifier. Specified in S2 §8.1 and **not yet written**.

**Files.** Pre-registration document; `scripts/` analysis path; no engine change.

**Verification.** The criterion must be applied to the existing 320-cell grid **before** any new campaign runs, so that it is scored against data collected under the old rule. If it is written after seeing which cells would pass, it is worthless — this is exactly what pre-registration exists to prevent.

**Abort criterion.** None; this is a precondition, not an experiment.

**Effort.** Small. **Unblocks:** every verdict in Track A, and the §4.1 reliability result's missing verdict.

---

### A1 — `--dwell` sweep at b3000 *(no code change)*

**Problem.** The conjunction claim — only a hybrid delivers continuous availability *and* sub-10 m median accuracy — holds at one geometry and fails at every density tested at a larger one. Part 7 framed this as *"does not transfer to scale"*, which is fatal. S4 §8.2 corrects the framing: b1000/gw3 and b3000/gw9 have **identical gateway density** (2.25 gw/km²) and differ **8× in median error**, so density is not the explanatory variable. Precision-tier **coverage fraction** is — 11.49 % vs 3.39 %, matching a dwell/transit prediction to within 1–2 percentage points at both geometries.

**Change.** None. `--dwell` exists (`ca_tbl_causal_sim.cc:2937`, default 90 s). Sweep it at b3000. Restoring b1000's coverage fraction needs `dwell ≈ 245 s`, from `dwell / (dwell + 1880) ≈ 0.115`.

**Verification.** Measured coverage fraction must track the `dwell/(dwell+transit)` prediction, as it did at both existing geometries.

**Abort / caveat, stated in advance (S4 §8.2).** Raising dwell and adding facility sites both raise coverage fraction but **are not the same intervention** — more sites shortens transit, longer dwell increases stationary time. If the conjunction returns under one and not the other, coverage fraction is not the whole story and the claim must be stated as dwell-dependent rather than coverage-dependent. The facility-count sweep is the cleaner test and needs a `BuildDeployment()` change; this is the cheap one that runs first.

**Effort.** Compute time only. **Unblocks:** reframes the conjunction from *fails at scale* to a **deployment requirement** — a more useful result and a defensible thesis claim.

---

### A2 — Detect-and-escalate under injected fault *(the branch point — and it needs no new code)*

**What already exists, corrected from the first draft of this plan.** The fault-injection harness is **built and has been run**. `--injectRangeBias` applies a common-mode range bias in metres that deliberately does **not** set the NLOS flag — the flag help text states the reason: `suspect` requires all-LOS, so a fault that announces itself is invisible to the gate by construction and would test nothing. `--injectFaultAnchors` selects `0` = F3 common-mode across every tier-2 link, or `1` = F1 single-anchor, described in the source as *the canonical RAIM fault model and the physically realistic one — one anchor displaced, drifting, detuned or blocked*. The faulted anchor and the window start are **seed-derived, so they are reproducible and identical across arms** — the ablation measures the gate, not the fault placement. 162 results directories carry it.

**The gap, stated precisely.** A census of every `run_id.txt` citing `injectRangeBias`:

| configuration | runs |
|---|---|
| injection × `suspectForcesEscalation=false` | **160** (4 recovery policies × F1/F3 × 20 seeds) |
| injection × `suspectForcesEscalation=true` | **0** |
| injection with the flag absent (defaults true) | 2 |
| distinct bias magnitudes tested | **1** (40 m) |
| distinct window fractions tested | **1** (0.2) |

So the check has been falsified **as rejection alone, under its own fault model, at one operating point.** That is a stronger negative than the first draft of this plan credited. But the combined **detect-and-escalate** mechanism — the one that moves `cep50M` from 92.15 m to 3.10 m under propagation-derived NLOS — has **never been run under injection at all**.

**Change.** No engine change. Re-run the `abl_clean` matrix with `--suspectForcesEscalation=true`, then sweep the two untested axes.

```
# the untested cell, 4 policies x 2 fault modes x 20 seeds
--injectRangeBias=40 --injectBiasFraction=0.2 --injectFaultAnchors={0,1} \
--suspectForcesEscalation=true --suspectRecovery={none,inflate,bounded,reinit} \
--pcsForm=v2 --nlosSource=propagation --mahalUseR=true \
--arms=ca_tbl_full,ca_tbl_no_integrity

# then the magnitude / window axes at the best policy
--injectRangeBias={10,20,40,80,160}   --injectBiasFraction=0.2
--injectRangeBias=40                  --injectBiasFraction={0.05,0.1,0.2,0.4}
```

**Verification.** Detection precision/recall against injected ground truth — which is CAL-05, the calibration the thesis specifies and the project has still never produced. Report F1 and F3 separately; they are different fault models and S2 §6.9 already records that the gate's effect is form-dependent.

**A qualification that must travel with any positive result.** The prior the check tests against is a constant-velocity prior propagated from previous radio fixes, **not** the independent prior ch10 §10.7.2 specifies. A positive result here is evidence about the implemented check, not about the specified one. See C3 and DEC-D.

**Abort criterion — pre-registered, and a null result is a real answer.** If `ca_tbl_full` does not beat `ca_tbl_no_integrity` with escalation enabled, under the fault the check was designed for, at more than one bias magnitude, then claim 2 closes honestly and Track B takes the framing. S3 §13.2 registered exactly this in advance: *"a null result there is a real answer."* Honour it.

**Effort.** **Compute only.** This was the largest item in the first draft and is now among the cheapest. **Unblocks:** the last untested cell of the thesis's stated central contribution, and CAL-05.

---

### A3 — Intra-cycle handshake scheduling

**Problem.** A tier-1 handoff costs a ranging slot: ToA 56.58 ms arms 2.263 s of off-time at the 2.5 % cap against a 2.5 s cycle, and a handoff needs two transmissions. S4 correction 9 records that the earlier proposed fix (`dt ≥ 3.91 s`) was an **average-rate calculation, not the binding constraint** — pre-emptions persist at every `dt` because the off-time lockout blocks a handshake attempted at the same cycle instant.

**Change.** Defer the handshake ≥ 2.263 s after the ranging **within** the cycle. Derived from the lockout arithmetic to eliminate the pre-emption at any `dt ≥ 2.32 s` at no accuracy cost. **Derived, not run** — named the strongest Phase 8 candidate.

**Isolation.** Flag-gated; off reproduces the baseline.

**Verification.** Pre-emption count → 0 at `dt ≥ 2.32 s`; accuracy unchanged within the paired-seed CI.

**Abort criterion.** If pre-emptions persist after deferral, the lockout model is wrong and the regulatory finding must be restated — which is itself a result worth having.

**Effort.** Small–moderate. **Unblocks:** converts the duty-cycle mechanism finding from a constraint into a mitigation, strengthening one of the two results that already survive independently of the decision layer.

---

### A4 — Tier-1 measurement calibration *(scheduled before A5)*

> **DONE 2026-09-06, and its framing is superseded.** Tier-1 sigma is over-stated by ~1.7×
> consistently across a 2.2× density range, but the baseline NEES of 2.95 does not reconcile
> against 1.342, so **no calibration was applied** and the 1.7× is not attributable to tier-1
> `R` alone (NEES normalises by the posterior covariance, which carries `Q` too). See
> `report/A4_CALIBRATION_RESULTS.md`. Separately, the "coverage limit" framing below counts
> *tier-2 anchors ringed at facility sites*, a geometry the 2026-09-07 topology correction
> removed — re-derive it before acting on it.

**Problem.** Above `spacing/r80` ≈ 1.55 no arm meets the requirement, and **96–99.6 % of inadequate fixes occur on cycles where fewer than three tier-2 anchors are in range**. Open question: is tier-1 range uncertainty over-stated relative to realised error? If so, the filter is discounting exactly the measurements that would carry those cycles.

**Change.** Compare tier-1 reported sigma against realised error across the density sweep; recalibrate if over-stated. Interacts with C2 and D-3: the RSSI-inversion sigma is proportional to range by construction, which is both the reason the CRLB gate does not apply and a candidate reason the sigma is mis-scaled.

**Verification.** NEES on tier-1-only cycles against the 2-DOF expectation. Note the existing calibration state in the one passing cell: NEES 2.95 at 20 seeds against an expectation of 2.0, median 0.58, 3.3 % of cycles above the 95 % bound, concentrated in UWB (S2 §4.2 caveat 5).

**Abort criterion.** If tier-1 sigma is correctly calibrated, the upper edge is a genuine coverage limit and A5 becomes the only route.

**Effort.** Small–moderate. **Ordering rationale, from S3 §13.3:** scheduled first *precisely so that A5 does not answer a question a calibration fix already answered.*

---

### A5 — Complementary siting as a second treatment arm

> **DONE 2026-09-06, and the question is now VOID.** Complementary siting forfeited tier 3
> outright — coverage 50.2 → 0 across all five seeds — because the nearest rotated site was
> 1526 m away against a UWB range of tens of metres (`report/A5_SITING_RESULTS.md`). The
> 2026-09-07 topology correction then removed the premise entirely: **tier 2 and tier 3 live
> on the same tri-band nodes and cannot be sited apart.** `--sitingMode` is read only under
> `--topology=1`. The surviving lesson is the arithmetic: it is *why a fixed gateway cannot
> serve tier 3 at all*, and therefore why finders exist.

**Change.** Distribute tier-2 anchors rather than ringing four facilities; run **alongside** the co-located control so siting is not confounded with everything else. Requires a `BuildDeployment()` change — the four sites are hardcoded at `±0.47 × boxHalf`.

**Verification / registered prediction (S3 §13.3).** If `static_2p4` gains as much as the tiered arms, **this is a deployment improvement rather than an argument for tiering, and must be reported as such.**

**Abort criterion.** The prediction above *is* the abort criterion. Honour it.

**Effort.** Moderate. **Unblocks:** the upper edge of the operating envelope, and the cleaner test of A1's coverage-fraction hypothesis.

---

### A6 — Expose hysteresis and `nDeescalate`; sweep

**Problem.** `pcsThresholdDown` and `nDeescalate` are not exposed by `AddValue`, so the hysteresis band has **never been swept** despite being CAL-03/CAL-04 in the thesis register and an explicit design element in ch10 §10.8.2.

**Change.** Two `AddValue` calls, then a sweep.

**Abort criterion.** None — this is a gap-closing measurement, and a flat response is a reportable result.

**Effort.** Trivial code, moderate compute. **Unblocks:** CAL-03, CAL-04.

---

### A7 — A tier-2 fidelity check that reads the sensitivity tables

**Problem.** D-5. The six-decibel indexing error passed every gate, verifier check and validation in the project. Asked directly, S2 §5.4b answers: **no check in the project would catch that defect today.** What found it was a closed-form propagation model built to reproduce the engine and refusing to be tuned when it could not — not a gate.

**Change.** The check is specified in S2 §8.4. Build it.

**Verification.** Negative fixture required, per the project's own rule that no gate is trusted without forced-failure evidence. Re-introduce the `sf - 7` error in a fixture and confirm the check fires.

**Effort.** Small. **Unblocks:** re-scoping claim 7 from *verified for ranging precision and energy accounting, unverified for receiver sensitivity* back to an unqualified fidelity statement.

---

### A8 — A weighted or gated PCS *(lowest priority in Track A, and the reason is important)*

**Problem.** D-1. Three candidate forms are named in S2 §8.2 — a weighted sum, a `term1` gate that vetoes when the covariance term saturates, or `min` instead of sum — and **none has been run**. The thesis specifies weights determined by LOOCV (CAL-01), which was never attempted; v2 replaced the functional form instead of fitting the weights.

**Why it is lowest priority despite being the most obvious fix.** Two independent reasons:

1. **The evidence against the concept does not depend on the form.** The legacy score carries no information beyond the EKF trace it is computed from (ρ = −0.199 against a monotone control at −0.200), and v2 — a different functional form, pre-registered before measurement — **fails the same test**. Two forms have now failed the same information test. A third is not obviously different in kind.
2. **A fix changes the engine and invalidates the entire results tree**, which is why D-1 was recorded rather than corrected in the first place.

**If it is run anyway**, the pre-registered control must be the same one: a **monotone-of-filter-statistic control**, to catch the case where a new score merely reproduces the baseline. And it must be run against the A0 criterion, not the cost criterion.

**Abort criterion.** If a third form fails the monotone-control test, the confidence-score contribution is closed and Track B is the framing. **Do not run a fourth.**

**Effort.** Large, because of the invalidation cost. **Unblocks:** CAL-01, and claim 1 — if it works, which two prior attempts suggest it will not.

---

---

## 6b. Track C — building claim 3

**Decision taken (DEC-E): build, do not descope.** S5 assessed hop-provenance as out of reach and recommended descoping. That recommendation is overridden; what follows is the honest path to building as much of claim 3 as is reachable, with the one genuinely hard item isolated and costed rather than hidden.

Order is fixed by dependency, and it is the order S5 §4 recommends on independent grounds.

### C1 — `ESC_REQUEST` / `ESC_GRANT` as real frames *(do this first)*

**Problem.** The escalation handshake is **billed but never sent**. `CaTblEnergyModel::AccountEscalationHandshake()` charges airtime and energy; no frame exists, so it never contends for the channel, never passes through CAD, and never meets the duty governor as a frame. Part 5's duty-cycle findings are therefore about a correctly-sized *accounting* of a handshake, not one that contended. The five message types `ENTRY_PUSH`, `ESC_REQUEST`, `ESC_GRANT`, `QUERY`, `RESPONSE` appear **only** in `ca-tbl-header.cc` — in the switch statements giving each its name (`:27-37`), ACK policy (`:52`), access category (`:105-114`) and payload size (`:121`, `:162`). They are never constructed anywhere in `src/` or `scratch/`.

**Second blocker, easy to miss.** There is **no receive-side dispatch on message type.** The MAC counts and charges per type (`Counters(f.hdr.GetMsgType())`, e.g. `ca-tbl-mac.cc:465`, `:498`, `:528`) but implements no request → response semantics. `ENTRY_PUSH` is described in the `StartHandshake` comment as arriving "as a reply"; no code generates that reply.

**Change.** A `HandleReceive` dispatch keyed on `GetMsgType()` with per-type reply logic; replace the accounting call at `ca_tbl_causal_sim.cc:2560-2563` with real frames; give anchors grant/deny logic. `ca-tbl-header.cc` needs **no change** — sizes, access categories and ACK policies are already defined for all nine types.

**Isolation.** `--sendHandshakeFrames=false` keeps the accounting path and must reproduce `baseline_v6` byte-for-byte. S5 calls this the best-isolated of the three items.

**Why first.** It is the only item that tests an **existing claim** (claim 4's custom handshake, claim 5's packet taxonomy) rather than adding capability, and it is the cheapest. Best ratio of claim-coverage to work.

**Expect every escalating arm to change** — real frames contend where a charge did not. Re-baseline before comparing anything.

### C2 — Two tags in the scenario

**What is already multi-node, and it is substantial.** The MAC and channel layers are **already multi-node**. Every anchor has its own `CaTblMac` registered on the shared channel (`ca_tbl_causal_sim.cc:1973-2000`) — at `gwGrid=3` that is 9+ MAC instances per tier already contending. The engines are keyed by node id and expose `RegisterNode(uint32_t, Vector)` and `UpdateNodePosition(uint32_t, Vector)` (`lora-physics-engine.h:361,364`). `CaTblMacConfig` already carries `identityClass`, set to `CA_TBL_ID_TARGET` for the field node (`:1959-1960`) and `CA_TBL_ID_ANCHOR` for anchors (`:1985`).

**What is singular.** Five objects inside `RunArm()`: `const uint32_t nodeId = 1` (`:1799`), `CaTblEnergyModel energy` (`:1832`), `CaTblPolicy policy` (`:1893`), `CaTblMac* nodeMac[CA_TBL_TIER_COUNT]` (`:1951`), `NodeTrajectory` (`:744`). The cycle loop reads them as free variables throughout (`:2325`, `:2436`, `:2560`).

**Change.** Promote the five into a per-tag struct; loop the cycle body over tags. Engines, channel, anchors and provider are shared and need no change. **`scratch/ca_tbl_causal_sim.cc` only** — S5 found nothing in `CaTblPolicy`, `CaTblMac` or `CaTblEnergyModel` that assumes a single instance.

**The hard part is not the refactor.** `macRng` is a single shared stream seeded at `:1932` (`SetStream(seed * 7919 + 104729)`) and every MAC draws from it (`:1969`, `:1999`, `:2008`). A second tag shifts the stream for everything after it. **`--nTags=1` must leave the draw order byte-identical**, tested by diffing `arm_summary.csv` and all six traces against `results/baseline_v6`. If the loop is structured so a single tag executes exactly the current call sequence this holds by construction; if the diff is not clean, the refactor changed something and the diff says so.

**Unblocks:** C3 and C4.

### C3 — `QUERY` / `RESPONSE` and identity-scoped search

Needs C1's dispatch and C2's second target to scope *between*. `identityClass` filtering already exists in `CaTblMacConfig`. Adds an identity-scoped flood plus proxy-response state.

Also fix here: **nothing sets mobility state**, so `m_mobility` is never `CA_TBL_MOB_RECEDING` and the receding branch at `ca-tbl-mac.cc:988` is dead code. Claim 4 is currently exercised on its stationary path only.

### C4 — Multi-target UWB arbitration

`arbitration_harness.py` already scores `U(r) = λ₁T_deg + λ₂T_batt + λ₃T_stale + λ₄T_query` offline with a calibrated `k`. Nothing instantiates a second target for it to arbitrate between. With C2 landed, port the scorer into `CaTblMac` anchor-side and report the **time-to-precision delta between highest- and lowest-priority targets** under simultaneous contention. Produces CAL-09.

### C5 — Hop-provenance: the honest cost

**Why the literal `0` at `ca_tbl_causal_sim.cc:2438` is correct and not a stub.** `CaTblMac::NoteNeighbour()` rejects any frame that has traversed a hop (`ca-tbl-mac.cc:337-356`), and the guard's own comment says it is *"the whole correctness of the table"*. The reason is an energy defect, not a simplification: a relayed frame carries the **originator** in `origin_id` — the relay does not rewrite it, because flood dedup keys on the originator — so admitting it inserts a peer the node may be unable to hear into the poll set, and every subsequent poll burns a full `rxTimeoutS` against it: **0.93 mJ on UWB, about three times a successful exchange.** The poll set is one-hop by construction, so every measurement genuinely has `hopsTraversed == 0`. Passing `0` reports the truth.

The plumbing exists and is misleading for that reason: `CaTblPolicy::Step(..., uint32_t hopCount, ...)` (`ca-tbl-policy.cc:693`), `ComputePcs(..., hopCount, ...)` applying the penalty on both paths (`:648-650` v2, `:669-671` legacy), `CaTblHeader::GetHopsTraversed()` (`ca-tbl-header.cc:413-419`), and the MAC already flooding with dedup, decrementing hop limit and forwarding the count into the PHY (`ca-tbl-mac.cc:842`, `:909`, `:929`).

**Two routes, both real work:**

1. **Relayed ranging** — a two-way exchange through an intermediate node, with range and uncertainty **composed across hops**. A new protocol and a new error model.
2. **Admit multi-hop neighbours** *and* add per-peer reachability tracking so unreachable peers are not polled. The reachability half is the hard part and, done wrong, reinstates the exact defect the neighbour table was added to remove.

**Files:** `ca-tbl-mac.cc/.h` (admission, reachability), `ca-tbl-policy.cc/.h` (composing multi-hop range uncertainty), `ca_tbl_causal_sim.cc` (relays — **depends on C2**).

**Hard rule, and it survives the decision to build.** **Do not set `hopWeight > 0` until relayed measurements actually exist.** `hopWeight = 0` at `ca-tbl-policy.cc:76` is currently correct. A non-zero weight on a hop count that is structurally always zero changes nothing; applied to an admitted-but-unreachable peer it degrades the score for an **energy** defect rather than a **provenance** one, which is a wrong result that will look like a working mechanism.

**Gate before starting C5.** Write the relayed-ranging error model — how range uncertainty composes across a hop — and have it reviewed *before* any code. If that model cannot be written down, C5 is not buildable and the hop-provenance half of claim 1 must be descoped after all. This is the one place where S5's original recommendation may reassert itself on technical grounds rather than effort.

### Track A abort conditions, consolidated

Track A aborts to Track B when **any two** of the following hold:

1. A2 returns null under the fault class the check was designed for.
2. A8 (if run) fails the monotone-of-filter-statistic control — the third form to do so.
3. A0's joint criterion, applied to the existing 320-cell grid, still returns no PASS region for the tiered arm.

Any **one** of these on its own narrows the claim but does not close it. Two together mean the decision layer has been given a fair test under a fair metric against its own fault model and has not paid for itself, and continuing is sunk-cost reasoning.

---

## 7. Track B — pivot to the measured contribution

Track B is not a fallback narrative bolted onto a failed project. It is the framing the evidence already supports, and S2 §3 states it plainly: *"This section leads because it is the strongest part of the work."*

### 7.1 What survives, and what each is worth

| Surviving result | Strength | Independent of the decision layer? |
|---|---|---|
| **Tiering buys reliability in a degraded environment.** `ca_tbl_full` 19/20 vs `static_2p4` 0/20 deployments at `RURAL_FOREST`, `dwell = 800`, `boxHalf = 2000`, 1.56× energy; paired per-seed difference peaks at +0.95, near-perfect separation | Strong measurement, **no verdict** — no reliability criterion was registered. A0 closes this | No — it is about tiering, but not about the confidence score |
| **The dimensionless axis transfers.** Correcting the sensitivity defect moved `r80` 909.5 → 1149.5 m (26 %); the window moved ~29 % in physical units and stayed at `spacing/r80` 1.02–1.23 | A result **about method**, explicitly stated as not depending on any claim passing | **Yes** |
| **A duty-cycle limit makes a tier-1 handoff cost a ranging slot**, regardless of cycle period | Mechanism, generalises past this implementation. Chain: legal constraint → protocol parameter → handoff latency → metres of error | **Yes** |
| **Absolute-target escalation converts infrastructure improvement into accuracy regression.** As gateways go 4 → 9, tier-1 median error falls 168 → 63 m and tier-1 outage falls 5.66 % → 0.01 %, while the proposed system's median error **more than doubles**, 11.86 → 26.12 m | Mechanism, and **invisible at any single density** — a single-point evaluation would not have found it | **Yes** — it is a property of absolute-target switching rules generally |
| **Coverage fraction, not density, governs accuracy; density governs continuity.** Identical density, 8× error difference; dwell/transit predicts both geometries to 1–2 points | Deployment requirement, testable further by A1 | **Yes** |
| **Only a hybrid delivers continuous availability with sub-10 m median accuracy** — 1.54 % outage vs single-band 2.4 GHz's 46.25 % at the sparse geometry | Operational result; geometry-scoped, A1 tests the scoping | Partly |
| **One configuration in 320 is cheaper and equally reliable.** M1 = 0.9531, CI [0.9296, 0.9775], seed-level bootstrap over 10 000 resamples | The only PASS under the registered rule. **Five caveats, none optional** — a single isolated cell, a 4.5 % margin, neighbours on the wrong side, all ten adjudicable cells in the easiest environment, and imperfect filter calibration in that cell | No |
| **The measurement apparatus itself** | Complete, self-tested, with every gate observed to fail on a negative fixture | **Yes** |

### 7.2 The framing

The thesis contribution becomes: **a method for evaluating tiered radio localization architectures honestly, and four mechanism findings that method produced** — three of which generalise past the implementation and none of which requires the confidence score to work.

The structural finding of S2 §7 is the intellectual core: *a project can register a metric that cannot express the claim its architecture makes, and only discover it by running the campaign.* The screening rule and the verdict rule carried disjoint cell sets; all ten adjudicable cells sat in the easiest environment — the one where tiering is least needed — because only there does a single-band arm also qualify. That limitation was **named in the pre-registration before the campaign ran**, which is what makes it an observation about the metric rather than an excuse constructed after an unwelcome result.

Supporting this: four defects the method found, two by checks failing and two by results being arithmetically impossible, including a physics error on the tier carrying 95 % of active cycles that every fidelity check passed. And a methodological contribution in its own right — mechanism-aware degeneracy screening, which refuses a single verdict without a named mechanism, and which **reports its own false-positive rate** (an energy rule that flagged 66.5 % of the corpus, corrected to 0.1 % once a material-share threshold was required).

### 7.3 What Track B still requires

Track B does **not** mean stopping work. It requires:

- **A0** — still needed, to attach a verdict to the reliability result.
- **A1** — still needed, to convert the conjunction claim from *fails at scale* into a deployment requirement.
- **A3** — still valuable, since the duty-cycle finding is one of the two surviving mechanisms and a mitigation strengthens it.
- **A7** — still needed; a methodology thesis whose own fidelity checks have a known blind spot is weaker for it.
- **D-3** fixed — a category error in a reported gate is a direct hit on a methodology contribution.
- The **`[M-24]` provenance gap** either closed (≈ 3.8 h campaign) or prominently stated.
- Chapter 8's novelty claims **rewritten** to match. Chapter 12's claim 3 is **not** withdrawn — DEC-E commits to building it (Track C), and Track C is independent of whether Track A succeeds. If C1–C4 land, claim 3 becomes testable for the first time and Track B's framing gains a component the confidence-score result cannot take away.

A8 becomes optional. A5 and A6 become optional. **Track C is unaffected** — it tests claims 3, 4 and 5, none of which depend on the confidence model working.

---

## 8. Sequencing

```
NOW ─── A0  register the joint criterion            [no compute, blocks every verdict]
        D-3 fix the CRLB gate on tier 1             [small, and it is a live error]
        DEC-B, DEC-C, DEC-D  (§10)                  [decisions, no code]
         │
         ├── A2  detect-and-escalate under injection ══ THE BRANCH POINT ══
         │       [COMPUTE ONLY — the harness exists. Run this first: it is
         │        cheap and it decides the framing of the whole thesis]
         │            │                                   │
         │     beats no-gate                       null at >1 magnitude
         │            │                                   │
         │            ▼                                   ▼
         │     claim 2 rehabilitated              claim 2 closed honestly
         │     A8 becomes worth running           A8 NOT run · TRACK B framing
         │            └──────────────┬────────────────────┘
         │                           │
         ├── A1  --dwell sweep at b3000             [no code change — run in parallel]
         ├── A7  sensitivity fidelity check         [small, closes D-5]
         └── A6  expose hysteresis params + sweep   [trivial code]
                                     │
         ┌── A4  tier-1 calibration ──┤             [must precede A5]
         │            ▼               │
         │        A5  complementary siting          [only if A4 says sigma is fine]
         └── A3  intra-cycle handshake deferral
                                     │
TRACK C (claim 3, DEC-E = build) ────┤
        C1  ESC_REQUEST/GRANT on the wire           [first: tests an existing claim]
             └─► re-baseline; every escalating arm changes
        C2  two tags  ── gate: --nTags=1 byte-identical vs baseline_v6
             ├─► C3  QUERY/RESPONSE, identity-scoped search
             └─► C4  multi-target arbitration (CAL-09)
        C5  hop-provenance ── gate: write the relayed-ranging error
             model FIRST. No model ⇒ descope C5, keep C1–C4.
             NEVER set hopWeight > 0 before relayed measurements exist
                                     │
                                     ▼
                    Thesis Ch. 8 written LAST,
                    against what was measured
```

**A2 moved to the front on the strength of the census in §6b.** It was costed as the largest engine change in the first draft of this plan and is now compute-only. It is also the only item whose outcome changes what the thesis argues, so running it before A1/A6/A7 costs nothing and de-risks everything downstream.

**Two rules govern the whole sequence, both from the project's own discipline:**

1. **Pre-register before running.** Predictions, falsifiers and abort conditions to a `PREDICTION.md`, committed before the first cell. Directory timestamps must be checkable against the commit.
2. **Every new mechanism sits behind a default-off flag, and the off state must reproduce the existing baseline byte-for-byte.** Arms consume the shared RNG stream at their own rate, so any added transmission perturbs every arm. This is testable the same way `--arms` was, and it is not optional.

---

## 9. Thesis chapter mapping

Chapter-by-chapter, what changes given the repository state. The blueprint's drafting order (Ch. 9 → 10 → 11–13 → 15 → 14 → 3, 4 → 5, 2 → 6, 7 → 8 → 1) still holds; the content bindings do not.

| Ch. | Status | What must change |
|---|---|---|
| 1 | SCAFFOLD, blocked on RES-06 | Unchanged. Ch. 1 statistics gap is real and untouched by any of this |
| 2 | SCAFFOLD, draftable | Unchanged |
| 3 | SCAFFOLD, blocked on RES-01 | §3.13–3.16 (IMU, dead reckoning, ZUPT, sensor fusion) are draftable from the rank-3 source — **but see C3.** If the implementation has no IMU and Phase 2.1 will not add one, Ch. 3 may survey inertial sensing while Ch. 10 must not specify an inertial prior it will never have. Decide together |
| 4 | SCAFFOLD, blocked on RES-02 | Unchanged |
| 5 | SCAFFOLD | Unchanged. Rank-5 material still needs star → mesh reframing — **but if Ch. 12's mesh is descoped, so is that reframing.** Cheaper, not harder |
| 6 | SCAFFOLD, blocked on RES-05 | Add NOV-02 (`albinsaid2025design`, SX1280 coordination) to §6.5 + Table 6.13; NOV-04 (`muppala2021feasibility`, GNSS-free TDoA with regulatory analysis) to §6.4. NOV-03 to §9.9 or dropped with C2 |
| 7 | SCAFFOLD, sequenced after 6 | **NOV-01 forces a narrowing.** Hop-count degradation is well characterised; the contribution was to be composing it into a transported scalar. With no hop count, Gap 1 must be rewritten or withdrawn |
| **8** | SCAFFOLD | **Rewrite the novelty claims entirely.** Written last, against what was measured. §8.6 (exclusions) gains TDoA and mesh if C2 and Ch. 12 are descoped. §8.10 (*what Phase 1 does and does not establish*) becomes the most load-bearing section in the thesis |
| **9** | SCAFFOLD, "start here" | **Resolve C1, C2, C4 first.** §9.3's TDoA multilateration is not what was built. §9.6's GPS-PPS synchronisation is consequent on it. §9.2's LR1121 basis needs the C4 delta stated. Otherwise draftable from `Architecture.md` §1–6 with terminology translation |
| **10** | SCAFFOLD, "the contribution core" | **The most affected chapter.** §10.3.1 inertial prior — C3. §10.5 CI fusion and sequence supersession — no substrate. §10.6.5 PCS weights — never done, and D-1 is the consequence. §10.7 *"central contribution"* — measured negative in one configuration, untested in its own fault model, resolved by A2. Draft **after** A2 branches |
| 11 | SCAFFOLD, draftable | **The best-supported chapter in the thesis.** `Architecture.md` §7–8 supplies measured contention behaviour, the G.S.R. 853(E) duty-cycle chain, computed ToA matching measurement to 5 s.f., and the datasheet energy reconciliation. Table 11.1 is computable now. Note §11.6/§11.11: `ESC_REQUEST`/`ESC_GRANT` are billed not sent, and the receding branch is dead code — specify honestly or fix via the Phase 8 Q3 item |
| **12** | SCAFFOLD, "draftable" | **Not draftable as specified.** Every mechanism is absent and hop-provenance is assessed out of reach. Either descope to a specification-only chapter with an explicit *not implemented in Phase 2.1* statement, or withdraw claim 3. **This is the largest single thesis decision** |
| 13 | SCAFFOLD | Same as 12 — no substrate. §13.7's single fault-model statement is still required and still valuable |
| **14** | SCAFFOLD, blocked on 9–13 | **Its own premise has inverted.** The scaffold states: *"This chapter specifies what Phase 2.1 must build. The existing prototype is subordinate to this specification, not the reverse."* Phase 2.1 is substantially built and has produced findings the specification does not anticipate. Either Ch. 14 documents what was built (making Phase 1 partly retrospective) or it specifies a Phase 2.2 that differs from what exists. **Answer explicitly** |
| 15 | SCAFFOLD, blocked on DEC-04, PROV-01/02 | PROV-01 and PROV-02 are still open and still correct as stated. The chapter's scope has grown well past a channel model — the engine now includes MAC, energy and estimation. Rename or rescope |

---

## 10. Decision register — required now

| ID | Decision | Blocks | Recommendation |
|---|---|---|---|
| **DEC-A** | `CA-DBL` or `CA-TBL` | Every chapter, `terminology_lock.md`, `glossary.md`, `acronym_list.md`, `notation_table.md` | **ANSWERED 2026-09-04: CA-TBL.** *Dual-Band* is factually wrong for three tiers. Log as `TERM-CHANGE-01` and propagate through the thesis registers. The repository, the results tree and 89 directories of arm names already use it, so nothing in the repo changes |
| **DEC-B** | C1 — estimation on the Anchor/Gateway Node or the Field Node | Ch. 9, Ch. 10, the energy model narrative | **Field Node**, matching the implementation. Changing the implementation invalidates the energy model, which is the strongest validated component |
| **DEC-C** | C2 — Tier 1 observable: TDoA or RSSI inversion | Ch. 9 §9.3, §9.6, Ch. 6 §6.4, NOV-03, NOV-04, D-3 | **RSSI inversion**, matching the implementation. Move TDoA to Ch. 8 §8.6 exclusions with a stated justification, exactly as AoA was handled |
| **DEC-D** | C3 — inertial prior in the predict step | Ch. 3 §3.13–3.16, Ch. 10 §10.3, Ch. 10 §10.7.2 | Depends on A2. If A2 runs, the independent-prior requirement matters and an IMU model may be needed for the check to mean what the spec says. If Track B, drop it and state the limitation |
| **DEC-E** | Chapter 12 and claim 3 | Ch. 7, Ch. 8, Ch. 12, Ch. 13, Ch. 14 | **ANSWERED 2026-09-04: build it.** Track C, §6b. C1 → C2 → C3 → C4 are reachable. C5 (hop-provenance) is gated on writing a relayed-ranging error model first; if that model cannot be written, C5 descopes and the rest of claim 3 still lands |
| **DEC-F** | Ch. 14's premise — specification or documentation | Ch. 14, Ch. 15, the Phase 1/2.1 boundary | Split: §14.1–14.9 specify; a new section documents what Phase 2.1 built and what it found, clearly marked as out of Phase 1 scope. This is honest and preserves the phase discipline |
| **DEC-G** | Blueprint D7 / DEC-06 — role of the 5M-row ML dataset | Ch. 14, Ch. 15 | Still open. It is not wired into the CA-TBL decision path. Simplest coherent answer: **channel-model validation artefact only**, ML naming dropped |
| **DEC-H** | Whether to run A8 at all | Track A scope | **Only if A2 succeeds.** Two functional forms have failed the same information test; a third costs the whole results tree |
| DEC-01/02/03 | Front matter, institutional formatting, deadline | Every chapter's final layout | Unchanged and still ADMIN-blocked |

---

## 11. Standing rules to carry forward

These were earned by defects, and each is cheaper to keep than to relearn.

1. **A count, rate or sum is not an observable until its denominator, its observation window, and its rule for observations truncated by that window are all stated.** The third clause was added after the outage count produced a 2820/2812 discrepancy.
2. **Censoring is asymmetric** — a truncated observation still settles any threshold it has already crossed. Added after a symmetric recount produced two false contradictions in an audit's first pass.
3. **Measure an unused quantity after a back-derivation.** The one real contradiction in the independent audit was found only because a check recounted from raw values rather than reusing the summariser.
4. **An exact zero is a control-flow claim, not a measurement.** A `+0.000` result was withdrawn after a reachability check.
5. **Check every arm before claiming uniqueness.**
6. **No gate is trusted without forced-failure evidence.** Every gate has a negative fixture; every one has been observed to fail.
7. **Verification must not mutate its subject.** The gate suite once regenerated three tracked results directories in place and re-dated their sidecars.
8. **A single-seed cross-arm difference is never a result.**
9. **Design against the failure mode.** A check for a defect whose mode is *absence* cannot be a scan of present data.
10. **An enumeration of a loss sum will not find a term that lives in the threshold.** Audit what a quantity is compared against, not only what is subtracted from it.
11. **Run every gate and report a summary; never exit on first failure.** While one gate was deliberately red, three others could not be observed to run at all.
12. **Completeness gates must count rows, not test for non-empty.** A per-seed appended summary is non-empty from seed 1; this caught 8 of 9 Pareto points at 14/20 rows before analysis.

---

## 12. What not to do

- **Do not draft Chapters 9–13 before DEC-B, DEC-C and DEC-E are answered.** Three specified mechanisms contradict the implementation and four have no substrate. Drafting first means rewriting.
- **Do not fix D-1 casually.** It invalidates the entire results tree. It is recorded rather than corrected for that reason, and A8 is last in Track A because of it.
- **Do not set `hopWeight > 0`.** Applied to a hop count that is structurally always zero it changes nothing; applied to an admitted-but-unreachable peer it degrades the score for an energy defect rather than a provenance one.
- **Do not repoint `report/REPORT_DATA_PACK.md`.** It documents superseded campaigns **on purpose**; repointing would attach its descriptions to numbers they did not generate. It is the one document with a registered dirty-citation exemption.
- **Do not edit `run_id.txt` sidecars.** They are evidence. The 1 283 that cite rewritten SHAs resolve through `SHA_MAP.md`; rewriting them would be falsification.
- **Do not invent a reliability criterion after seeing which cells pass it.** A0 must be written against the existing grid before any new campaign, or it is worthless.
- **Do not run a fourth PCS functional form** if a third fails the monotone-of-filter-statistic control.
- **Do not report the `inflate < bounded ≈ reinit < none` ordering as a mechanism.** It is a measured value with no mechanism established, and it is stated that way deliberately.
- **Do not quote `pareto_sweep.py --analyse`'s verdict string in support of the domination claim.** It compares the two decision rules to each other and does not test domination.
- **Do not let the *evidence gate* and the *Switching-Point Integrity Check* blur.** They are different mechanisms with different measured behaviour, and the repository calls both "the gate" in places.

---

## 13. Immediate next actions

Ordered. The first three need no compute and no code.

1. **DEC-A and DEC-E are answered** (CA-TBL; build claim 3). Log DEC-A as `TERM-CHANGE-01` with the propagation pass recorded in `consistency_checklist.md`. **DEC-B, DEC-C, DEC-D still open** and still block Chapters 9–10.
2. Write and commit **A0's joint criterion** as a pre-registration, then apply it to the existing 320-cell grid. Then run **A2** — it is compute-only and it is the branch point.
3. Update `CA-DBL_Phase1_Blueprint.md` §6 — its engine-coverage table is the primary planning input for Chapter 14, and **13 of its 20 rows are now wrong** (7 still hold: Tier 1 propagation, TDoA, CIR classifier, CI fusion, mesh routing, identity/arbitration, transport). Replace it with §2.1 of this document.
4. Start **A1** (`--dwell` sweep at b3000) — no code change, runs while the above proceeds.
5. Fix **D-3** (CRLB gate on tier 1) and build **A7** (sensitivity fidelity check with a negative fixture).
6. Start **Track C** at **C1** (`ESC_REQUEST`/`ESC_GRANT` on the wire), behind `--sendHandshakeFrames=false`, then re-baseline.
7. Reconcile the two source conflicts named in §0: the tier-3 `r80` disagreement (`Architecture.md` §2.1 vs technical report §2), and the tracked-sidecar count (`EXCLUDED.md` vs `SHA_MAP.md`). Both are one-line fixes and both are the kind of drift the project's own consistency discipline exists to catch.

---

*Every claim in this document traces to a file in the repository or a doc in the thesis project, named at the point of use. No parameter, result, citation or measurement has been invented. Where two sources disagree, both are named and neither is silently preferred.*
