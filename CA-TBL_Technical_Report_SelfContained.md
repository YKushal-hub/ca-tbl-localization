# CA-TBL — Confidence-Aware Tier-Based Localization

**A simulation study of when a tiered radio architecture pays, and when it does not.**

> Here is precisely where tiering pays, where it does not, and why — established under
> pre-registration, on reproducible provenance, with the design defects the method
> caught along the way.

Every figure below cites a results directory whose `run_id.txt` records
`dirty 0` — a clean tree at a named commit — with digests verified against the files
on disk. **One exception exists and is named in §9.**

---

> ## ⚠ SCOPE: every number in this report predates the topology correction
>
> **On 2026-09-07 (`99fdf1a`) the deployment model was corrected.** Until then the
> simulator modelled three **disjoint** anchor populations — tier-1 gateways on a grid,
> tier-2 anchors ringed at 150 m and tier-3 at 10 m around four facility sites.
> `Architecture.md` specified it that way too, so the code was not drifting from a spec:
> **the spec was wrong.** The system has one population of **tri-band** nodes, and a tier
> is a choice of *which radio to use*, not of *whom to talk to*. See `Architecture.md` §3.
>
> **Why it changes results rather than only wording.** The old model let the field node
> reach UWB by walking near a facility. It therefore *manufactured* tier-3 coverage that
> the architecture is supposed to earn, and hid the mechanism the design rests on — a
> **finder** carrying tier 3 to the casualty. With no finders, measured mean UWB
> neighbours is **0.01**.
>
> **Status of this document.** Every campaign cited here ran under `--topology=1`. The
> provenance is intact and each number still reproduces from its own sidecar — but it
> reproduces a **deployment the system does not have**. Every result below is therefore
> **provisional pending re-run**, including the falsifications in §5. `baseline_v7` does
> not reproduce under the corrected topology and is not supposed to.
>
> This report has **not** been rewritten to match the new model. Editing a dated evidence
> chain to agree with a later understanding would falsify it. It is scoped instead.
>
> **2026-09-29.** Seven claim-bearing campaigns have now been re-run under the corrected
> topology and environment, and two follow-up campaigns have been run after them. §0
> states every claim against that evidence and **supersedes the statements it names**.
> The text from §1 on is kept as written. Each statement §0 supersedes carries a dated
> pointer to the corrections register, §10.

---

## 0a. Claim record, restated 2026-10-17 (supersedes §0 where they differ)

The current measured tables are in `report/CAMP_FINAL2_RESULTS.md`. Conditions: the final preset (DESIGN_DECISIONS 10,
11, 13), an unknown start, 20 paired seeds, 221 cells, `source 06ea4386 dirty 0`.

| claim | status 2026-10-17 | evidence |
|---|---|---|
| Tiering (ranging both LoRa bands opportunistically) | **supported in the robustness sense.** Best endgame CEP95 (4.6 m) and best 5 km availability (0.352), at 1.2–2.9× the tag-0 energy of a single LoRa band | CAMP-FINAL2 §2; B4 |
| Confidence-aware switching (PCS) | **not supported.** Indistinguishable from a covariance-trace threshold in every cell | CAMP-FINAL2 Q2; B4-4 |
| Integrity-gated escalation | **not supported as a claim.** Precision 0.05–0.32, recall 0.02–0.13; overall it costs accuracy outside the fault window | CAMP-FINAL2 fault matrix |
| 2.4 GHz ToF over 868 MHz RSSI (same geometry) | **supported for calibrated ToF on tracking accuracy:** 89 vs 412 m after acquisition at 5 km; 12.3 vs 27.5 m CEP95 in the endgame (the final preset, DD 14 + 15; CORRW BASE rows). **Not** for uncalibrated ToF (Wolf: 130 vs 52 m) | CAMP-FINAL2; UPG-SCOPE U6; V8 |
| Reply positions (anchor staleness) | **supported, the largest single gain:** 5 km hybrid CEP95 842 → 271 m | V7 |
| Under calibrated Wolf errors the hybrid works | **in accuracy, with `--rssiCorrInflate`:** 23.8 m against 2.4 alone 28.5 m (34.4 m without it, on the DD 14 + 15 preset). The flag is not in the preset (CORRW W3 missed by 0.3 m). The energy is about 10× | CAMP-FINAL2 W_end; SIG868; CORRW; TECH_DEBT O.13 |

**Engine corrections behind these numbers:**
- correlated-shadowing RSSI error;
- no range-error leak into reception;
- NLOS as a range bias;
- keyed RNG;
- particle-filter acquisition;
- foliage depth clamped to the link;
- per-band fading;
- relayed-range NIS inflation;
- the exact log-normal 868 MHz σ (DD 14, adopted after CAMP-FINAL2);
- the IMU increment covariance ×30 (DD 15, B2CONS: endgame consistency 0.96).

Each is default-off in code, gated, and null-controlled; the preset turns them on. The text from §0 on is kept as written.

**Lost evidence (2026-10-18, G92).** `results/p10_env_v7`, `p10_env_v8`, `p10_plane_v7` and `campaign_20260810`, cited
below, are no longer in the repository: they were deleted with the untracked results tree, and the current engine
cannot regenerate them (legacy topology). `RESULTS_NONREGENERABLE.tsv` records each, with what supersedes it. Every
figure drawn from them was already provisional (the scope box above) and none is cited in §0a.

## 0. Claim record, restated 2026-09-29

**Why this section exists.** The scope banner says every number in this report is
provisional pending re-run. Between 2026-09-27 and 2026-09-29 the re-run happened for the
claim-bearing campaigns. This section is the claim record against that evidence. It adds no
analysis: every number here is quoted from the committed read-out named beside it.

**What was run.** Each campaign was registered before its first cell, and every sidecar
reads `dirty 0` with its digests verified.

| campaign | registration | cells | results | read-out |
|---|---|---|---|---|
| B3, seven claim-bearing campaigns regenerated | `report/PREREG_B3_REGEN.md` at `7b17bd6` | 125 | `results/regen27_integrity`, `results/regen27_pcs_forms`, `results/regen27_k10`, `results/regen27_k1`, `results/regen27_edge`, `results/regen27_imu` | `report/B3_REGEN_RESULTS.md` |
| C1 with the evidence gate off | `report/PREREG_C1_NOGATE.md` at `ddae10f` | 40 | `results/regen27_integrity_nogate` | `report/C1_NOGATE_RESULTS.md` |
| CAL-01, PCS weights by leave-one-seed-out | `report/PREREG_CAL01_LOOCV.md` at `c019c0a` | 60 | `results/cal01_loocv_20260928` | `report/CAL01_LOOCV_RESULTS.md` |

**What the evidence describes, and what it does not.**

- **Which engine.** B3 and C1_NOGATE ran a binary built from `084a7ff`; `src/` and
  `scratch/` are byte-identical at `7b17bd6` and `ddae10f`. CAL-01 ran at `c019c0a`, with
  the NLOS classifier and its PCS feed switched on. Everything else added since `084a7ff`
  (tier-1 TDoA, relay chaining and relay slots, fix routing, grant gossip) is default off
  and null-controlled, and no run cited here switches it on.
- **Where the estimator ran.** In every run cited here it ran **on the node**, except the
  edge-server half of C6, which is the treatment that campaign tested. It moved to
  the gateways by default at `db2ac2e`, after these runs, and at defaults that move changes
  energy columns only (`docs/TECH_DEBT.md` L.2.3). So the **accuracy** verdicts carry over
  to the current default, and every **energy** figure below describes
  `--gatewayEdgeServer=false`, except C6's comparison of the two settings.
- **No cross-binary magnitudes.** `LoRaDistributionSampler` stays unseeded
  (`report/PREREG_B3_REGEN.md` §1), so every number below is a paired comparison inside one
  binary. No regenerated number is differenced against a pre-correction original.
- **One scenario family, in simulation.** The limitations of §9 stand.

### 0.1 The record

| claim | status, 2026-09-29 | evidence | source |
|---|---|---|---|
| The architecture beats published snapshot estimators | **SUPPORTED** | cep95 below MLAT in every moving cell. Medians 6 665 vs 13 762 m (19+/1−, p = 4.0×10⁻⁵), 6 759 vs 38 599 m (18+/2−, p = 0.0004), and **2 016 vs 20 831 m (20+/0−, p = 1.9×10⁻⁶)** | B3 C3; `results/regen27_k10`; CR-11 |
| The mesh carries availability | **SUPPORTED** | `fixCycles / steps`: WAN only 0.041 < sparse 0.319 < WAN + mesh 0.647. WAN + mesh is above WAN only on 20/20 seeds, p = 1.9×10⁻⁶ | B3 C5; `results/regen27_k10`; CR-12 |
| Estimation at the gateway saves node energy at no accuracy cost | **SUPPORTED**, and the default since `db2ac2e`. The two hybrid figures carry an open billing decision. **2026-09-30: decided**; they describe the old bill, `--edgeDownlinkTier=active` (§0.2) | totalJ lower by **42.5 %** (`static_868`), **32.9 %** (`fixed_threshold`) and **32.9 %** (`ca_tbl_full`), 20/20 seeds each, p = 1.9×10⁻⁶. The seven accuracy columns are byte-identical on 20/20 seeds | B3 C6; `results/regen27_edge`; `docs/TECH_DEBT.md` M.10; CR-13 |
| An ideal dead reckoner collapses the error tail | **SUPPORTED AS A CEILING, never as a system result** | cep95 median: `static_uwb` 8 413 → 17 m and `static_2p4` 27 151 → 537 m, both 20+/0−, p = 1.9×10⁻⁶. `ca_tbl_full` 11+/9−, p = 0.82 | B3 C7; `results/regen27_imu`; CR-14 |
| Confidence-awareness (PCS) earns its place | **NOT SUPPORTED**; CAL-01: **CALIBRATION DOES NOT RESCUE PCS** | Lost the monotone control in 6 of 8 form × arm cells; undefined in the other 2. The `gated` repair is *worse* than `legacy` on cep50 (+12.96 m, 15+/5−, p = 0.041). CAL-01: no calibration of the three terms ranks error better than the covariance trace, and the calibrated weights are the trace term alone | B3 C2; CAL-01; `results/regen27_pcs_forms`, `results/cal01_loocv_20260928`; CR-1 to CR-7 |
| The confidence model beats a flat threshold (Claim 8) | **RESTATED**: under the repaired forms it *is* the flat threshold | `ca_tbl_no_integrity` is byte-identical to `fixed_threshold` on 19 of 20 seeds under each of v2, weighted and gated. Not re-registered, so reported, not a verdict | B3 C2, C4; `results/regen27_pcs_forms`; CR-8 |
| Tiering beats single-band | **NOT SUPPORTED** | `static_2p4` dominates `ca_tbl_full` in 11 of 11 K1 cells. `ca_tbl_full` is indistinguishable from `static_868` in every K10 cell. The three K1 cells where it dominates `static_868` are a billing effect | B3 C4, C3; `results/regen27_k1`; CR-10 |
| The witness cross-check (IMU and corroboration witnesses) earns its place | **WITHDRAWN in the shipped default: no population** | Off and on rows are byte-identical on 8/8 seeds, although the IMU witness refused 203–232 escalations per seed | B3 C1; `results/regen27_integrity`; CR-9 |
| — the same, with the evidence gate off | energy **REPRODUCES**; accuracy **NOT SHOWN WORSE**; **no detection claim** | ΔtotalJ −104.86 J per 600 s, CI [−107.97, −101.12], 20/20 seeds. ΔrmseM −0.66 m, CI [−9.62, +11.24]. The IMU witness refused on 85–98 % of cycles | C1_NOGATE; `results/regen27_integrity_nogate`; CR-9 |
| The Mahalanobis Switching-Point Integrity Check: rejection alone does not pay (Claim 9) | **NOT RE-TESTED**; stays provisional under the scope banner | — | §5.1 |
| Hop-aware provenance | **NO POPULATION**, unchanged | The measured tag's `Step()` receives the hop count as a literal 0, and `hopWeight` stays 0 | `report/CAL01_LOOCV_RESULTS.md` §5; `docs/TECH_DEBT.md` L.2.4 |

### 0.2 What each verdict does and does not say

**Published estimators.** What beats a memoryless solver is the filter, not the tiering.
`ca_tbl_full` is indistinguishable from `static_868` in all three cells, and in
`wanonly_move` it escalates 0 times on 20/20 seeds. A rival is scored only from its first
estimate onward, an asymmetry that can only favour the rivals. K10's own qualifier stays.

**The mesh.** The availability belongs to the mesh, not to tiering. `static_868` reaches
0.628 in WAN + mesh and 0.451 in the sparse cell, which is above `ca_tbl_full`'s 0.319.

**The gateway estimator.** This is a tier-1 mechanism: the node skips the per-gateway
tier-1 replies and pays one downlink per fix, so the saving is the size of the reply bill.
- On tier 2 it is a pure addition: `static_2p4` costs 0.2 % more on 20/20 seeds.
- On `static_uwb` it never runs, because that arm has 0 fix cycles.
- Getting a tier-2 or tier-3 range to the gateway is not billed (`docs/TECH_DEBT.md`
  L.2.3). That flatters the setting on those two tiers only.
- The downlink is billed on the active tier, including on cycles whose fix came from the
  868 MHz floor. There it is charged as a 0.2005 mJ 2.4 GHz frame, where a bill on the
  fix's tier would charge a 0.9042 mJ 868 MHz one (`docs/TECH_DEBT.md` M.10). C6 ran under
  the active-tier bill, so the two hybrid figures (−32.9 %) would be smaller savings under
  the other. `static_868`'s −42.5 % is unaffected. Which bill is right is an open owner
  decision, and the size of the change has not been measured.
  **2026-09-30: decided.** The downlink is billed on the fix's tier by default
  (`--edgeDownlinkTier=fix`). `--edgeDownlinkTier=active` restores the bill C6 ran under
  and reproduces its seed-7 rows byte-for-byte.
  - At defaults, only energy moves, and only on floor-solved fix cycles.
  - At seed 7 × 3600 s the two hybrid savings are −30.1 % and −29.5 %, against −32.3 %
    and −31.6 % under the old bill. This is one seed and an illustration only.
  - The 20-seed figures have not been re-measured (`docs/TECH_DEBT.md` M.10).

**The IMU ceiling.** It is an upper bound on what a flawless inertial unit could contribute
here, and it is not dead reckoning alone: the treatment changes `mac_tx` on 20/20 seeds on
every arm. The step-and-heading engine actually built diverges where fixes are sparse
(`docs/TECH_DEBT.md` K.9).

**PCS.**
- Under every repaired form, PCS sits below every escalation threshold on ≥ 99.98 % of fix
  cycles. It decides nothing, and the downstream gates decide. Under each repaired form,
  `ca_tbl_no_integrity` is byte-identical to the flat variance threshold `fixed_threshold`
  on 19 of 20 seeds.
- CAL-01 then asked whether any weighting of the three terms, fitted leave-one-seed-out,
  ranks position error better than the covariance trace they are computed from, with the
  NLOS term live. The answer, with A = ρ_control − ρ_fit and 98.33 % intervals:

  | regime | A | verdict |
  |---|---|---|
  | default | −0.0016 [−0.0066, +0.0030] | indistinguishable |
  | arc (44 % of cycles without a fix) | +0.0045 [−0.0129, +0.0217] | indistinguishable |
  | searcher | −0.0011 [−0.0044, +0.0001] | indistinguishable |

- The fitted weights on the sum-3 scale are (3.001, −0.0013, −0.000004),
  (3.000, +0.0002, −0.00004) and (3.003, −0.0007, −0.0023). The hand-set (2, 0.5, 0.5)
  loses by −0.47, −0.30 and −0.21.
- **So the contribution — that PCS carries information about error beyond the filter
  statistic it is computed from — is not supported by any linear calibration of its
  terms.**
- Escalating on a monotone function of the covariance trace is still a working mechanism.
  `fixed_threshold` is that mechanism, and it needs no PCS.
- Withdrawing the novelty claim is the owner's decision (`report/CAL01_LOOCV_RESULTS.md`
  §5). This record states the evidence for it.

**Tiering.** K1's "never dominates `static_868`" is falsified in `k1a_box750`,
`k1a_box1000` and `k1a_box1500` (energy 20+/0− in each), and is reported as an envelope.
**Those three cells are a billing effect, not evidence that escalating to a precision tier
pays.**
- With tier-1 replies billed, a tier-1 cycle costs 18.5–24.0 mJ and a tier-2 cycle
  5.5–8.6 mJ.
- With floor-on-demand, the 78–88 % of cycles the arm spends on 2.4 GHz replace tier-1
  time. Escalating therefore *leaves* the dearer tier: the opposite of the premise the energy
  axis was built to test.
- `static_2p4` dominates `ca_tbl_full` in the same cells.
- The reply bill that produces them is the one the gateway-estimator default removes. The
  cells have not been re-run under it.

**The integrity check: three mechanisms, kept apart.**

- *The witness cross-check* (`--imuCrossCheck`, `--corroborationCheck`). This record allows
  only the two-part statement registered by C1_NOGATE (`report/C1_NOGATE_RESULTS.md` §7):

  > *With the evidence gate on (the shipped default), the witness cross-check has no
  > effect: 8/8 seeds byte-identical (B3). With the evidence gate off, it lowers energy on
  > 20/20 seeds (mean −104.9 J over 600 s, CI [−108.0, −101.1] J) by suppressing
  > escalation into UWB. In that configuration the UWB neighbour table held under 0.1
  > entries per cycle on average, and the 145–189 extra UWB cycles per run went with no
  > extra fix cycle on 19/20 seeds and one on the twentieth. Accuracy is not shown worse
  > (CI [−9.6, +11.2] m). On 4/20 seeds it is worse by 10–89 m, and on exactly those seeds
  > the node spent 21–220 cycles at 868 MHz under the witness against 2–5 without it.*

  A witness that refuses on 85–98 % of cycles is close to a blanket veto. **No sentence
  about integrity, trust, detection or "the estimate being checked" may be written from
  either campaign.** The corroboration witness has never refused in 28 runs, so its
  contribution is unmeasured, not zero.
- *The Mahalanobis Switching-Point Integrity Check* (§2, §5.1). Not re-tested. Claim 9
  stays as recorded, pre-correction and provisional. An exploratory probe recorded in
  `docs/TECH_DEBT.md` L.2 finds its eligible population near-empty under the default NLOS
  source. That is not a result.
- *The evidence gate* (`--escalationRequiresEvidence`, default true since `7ec0d91`). In the
  shipped default it is what absorbs the witness's refusals. Its own result (K3) was not in
  B3's scope.

**Not re-tested, and still provisional under the scope banner:** §4.3 (the dimensionless
axis), §5.1 (Claim 9), §5.3, §5.4, §5.4b, §7 (the structural finding) and §2's tier-2
`r80` (CR-15). Silence here is not confirmation.

**The corrections register is §10.** It lists every statement this record withdraws,
restates or scopes, in this report and in the documents it draws on. Each entry names where
the statement was made and the read-out and results directory that correct it.

---

## 1. Executive summary

**What was built.** A three-tier GPS-denied localization stack for search and rescue,
simulated in ns-3.47: 868 MHz LoRa (RSSI-inversion ranging), 2.4 GHz (two-way
time-of-flight), and UWB (two-way ranging), with an EKF, a confidence score (PCS)
driving tier escalation, and a Mahalanobis integrity gate rejecting suspect fixes.
Six arms, twenty seeds, a pre-registered requirement of **availability ≥ 0.95 at
accuracy ≤ 25 m**, and a decision rule fixed before any run.

> *2026-09-29, restated in §0.* "A confidence score (PCS) driving tier escalation": under
> every repaired PCS form, PCS decides nothing, and under `legacy` it asks to escalate on
> 14–19 % of fix cycles (§10, CR-1). The "Mahalanobis integrity gate" named here was not
> re-tested (§0.2).

**Results that stand.**

| result | figure | qualification | source |
|---|---|---|---|
| Tiering buys reliability in a degraded environment | `ca_tbl_full` **19/20** vs `static_2p4` **0/20** deployments, at **1.56×** the energy | **No verdict attached** — no reliability criterion was registered, and inventing one after seeing the result is what pre-registration prevents. §4.1 | `results/p10_grid` |
| One configuration in 320 is cheaper *and* equally reliable | M1 **0.9531** [0.9296, 0.9775] | **Five caveats, none optional** — a single isolated cell, a 4.5% margin, neighbours on the wrong side, and all ten adjudicable cells in the easiest environment. §4.2 | `results/p10_grid` |
| The dimensionless axis transfers | window held at `spacing/r80` 1.02–1.23 across a **26%** change in `r80` | A result about method; independent of any claim passing. §4.3 | `results/p10_env_v7`, `_v8` |

**Results falsified.**

- **The integrity gate does not pay** (Claim 9) — all four response policies lose
  availability against no gate at all, in eight of eight cells. Narrower than it
  sounds; §5.1.
- **The confidence model does not beat a flat threshold** (Claim 8) — 19/20 vs 18/20
  at 24.26 J vs 22.77 J.
- **The original operating window is withdrawn** (Claim 3) — it was inferred from a
  region where every pre-registered verdict was FAIL or INCONCLUSIVE.
- **Packet-level physics realism is re-scoped, not proven** (Claim 7) — the cited
  checks verify ranging precision and energy accounting, and a six-decibel sensitivity
  defect passed all of them. §5.4b.

> *2026-09-29, restated in §0.* Claim 8 is restated: under the repaired PCS forms the
> confidence arm is byte-identical to the flat threshold on 19 of 20 seeds (CR-8). Claim 9
> was not re-tested. The witness cross-check that B3 withdrew is a different mechanism
> (§0.2, CR-9).

**Defects the method found.** A six-decibel sensitivity-table indexing error on the
tier carrying 95% of active cycles; PCS confidence saturating so it cannot express a
diverged filter; a gate suite that overwrote the evidence it verified; a degeneracy
screen that conflated four mechanisms into one verdict. Two were found by checks
failing, two by results being arithmetically impossible. §6.

> *2026-09-29.* The PCS saturation also runs the other way. The repaired forms sit below
> every threshold (CR-3).

**The structural finding.** The project registered a **cost** criterion for an
architecture whose value proposition is **reliability**, and the two select for
opposite conditions — a cost win needs a qualifying competitor, a reliability win
needs the competitor to fail. Demonstrated, not argued: the screening rule and the
verdict rule carried disjoint cell sets, and all ten cells where the rule could
adjudicate sat in the *easiest* environment — the one where tiering is least needed.
§7.

---

## 2. The architecture

Design intent, in the past tense. Nothing in this section is a claim.

**Three tiers.** Tier 1, 868 MHz LoRa (SX1262-class), SF7, 125 kHz, 14 dBm — wide-area
coverage, ranging by inverting RSSI through a path-loss model. Tier 2, 2.4 GHz
(SX1280-class), SF7, 800 kHz, 12.5 dBm — mid-range refinement by symmetric two-way
time-of-flight; measured `r80` **1149.5 m**. Tier 3, UWB (DW3000-class), 499.2 MHz at
7.99 GHz, −16.47 dBm — precision two-way ranging, `r80` ≈ 13.5 m. *(As run: "at
facilities", from a dedicated tier-3 anchor ring. Under the corrected topology tier 3
lives on every node and is reached from a gateway or a finder — see the scope banner.)*

> *2026-09-29, scoped.* The tier-2 `r80` of 1149.5 m describes the split environment.
> Under the corrected environment the probe in `report/K1_DOMINATION_RESULTS.md` gives
> 156.0 m (CR-15).

**The estimator.** A four-state EKF (position, velocity) with sequential scalar
updates, process noise 1.0 m/cycle in position, fed by multilateration over the
active tier's anchors.

**The confidence score (PCS).** An unweighted sum of three terms, each bounded in
`[0,1]`: filter uncertainty `traceRef/(traceRef + traceSigma)`, geometry
`1/max(gdop,1)`, and line-of-sight fraction `nLos/nTotal`. Escalation was intended to
fire when PCS fell below a per-tier threshold — 1.55 on tier 1.

> *2026-09-29, restated in §0.* Under every repaired form PCS sits below every threshold on
> ≥ 99.98 % of fix cycles, so the downstream gates decide. CAL-01 finds no weighting of the
> three terms that ranks error better than the covariance trace (CR-1, CR-7).

**The integrity gate.** Each candidate fix was tested against the EKF prior by
Mahalanobis distance, `D² = dᵀ(P_prior + R_fix)⁻¹d`, against χ²₀.₉₉ = 9.21 at 2 DOF. A
fix exceeding it on an all-line-of-sight cycle was marked *suspect* and rejected. Four
response policies were built: `none` (reject only), `inflate`, `bounded`, `reinit`.

---

## 3. Method

**This section leads because it is the strongest part of the work.**

**Pre-registration.** Every campaign's predictions, falsifiers and abort conditions
were written to a `PREDICTION.md` and committed **before the first cell ran**;
directory timestamps are checkable against the commit. The decision rule — three
outcomes, with "no competitor meets the requirement" deliberately INCONCLUSIVE rather
than PASS — was fixed in advance and is applied verbatim, including where it returns
no verdict at all.

**The gate suite, G1–G10.** Build, determinism, null ablation, verifier, verifier
self-test, tracked-evidence-versus-fresh-run, tree-clean-at-exit,
must-stay-untracked, deliverables-tracked-and-committed, and cited-paths-resolve.

**Every gate has a negative fixture, built in `/tmp`, and every one has been observed
to fail.** A suite that has only ever passed proves nothing about what it would catch.
G6 in particular was re-verified *after* being repointed at new evidence, because a
gate that turns green by being repointed is indistinguishable from one defanged by it:
it fails on mutated evidence, and still distinguishes "the tracked evidence disagrees"
from "the fresh run errored".

The suite also runs **every** gate and reports a summary rather than exiting on the
first failure. It used to exit early, and while G6 was deliberately red, G7–G9 could
not be observed to run at all.

**Provenance.** Every run writes `run_id.txt` recording the commit, the count of
modified tracked files under the source tree, the build profile, and a SHA-256 digest
of each output file — written after the last stream closes. `dirty 0` means the
binary is rebuildable from a named commit.

**Clone reproduction.** A fresh clone builds `optimized` and reproduces
`results/baseline_v7/arm_summary.csv` **byte-for-byte** using the flags recorded in
that baseline's own sidecar. Default flags do **not** reproduce it — a property worth
stating, because it reads as a reproduction failure when it is a flag-set difference.

**Mechanism-aware degeneracy screening.** One seed is run and screened before twenty
are spent. "Degenerate" is meaningful only relative to a named mechanism — `tiering`,
`integrity`, `trigger`, `energy` — and the tool refuses a single verdict without one.
Over 3205 directories: `tiering` 1.1%, `integrity` 27.3%, `trigger` 40.0%,
`energy` 0.1%.

---

## 4. Results that stand

### 4.1 Tiering buys reliability in a degraded environment — **WITHDRAWN 2026-09-05**

### 4.2 One configuration in 320 is cheaper and equally reliable — **WITHDRAWN 2026-09-05**

**Both are withdrawn, for one reason: the directory they cite is not in this
repository.**

Both were sourced from `results/p10_grid` at `source 0a489df dirty 0`. That directory
was **absent from the tree** — not excluded by `.gitignore`, not listed in
`EXCLUDED.md`, simply not present — while `README.md` stated that every table in this
report is sourced from a file in the repository. No gate caught it, because no gate
asserts that a *cited* directory exists, and a check for a defect whose mode is
absence cannot be a scan of what is present (rule 9).

**What was withdrawn.**

- §4.1 claimed `ca_tbl_full` **19/20** deployments against `static_2p4` **0/20** at
  1.56× energy, at `RURAL_FOREST`, `dwell = 800`, `boxHalf = 2000`.
- §4.2 claimed the only PASS in 320 cells: **M1 = 0.9531, CI [0.9296, 0.9775]**, both
  arms meeting the requirement on 20/20 seeds at `RURAL_OPEN`, `dwell = 800`,
  `boxHalf = 1500`.

**What was measured instead**, on a regenerated grid that records its own provenance
(`results/p10_grid`, 320 cells, `source 6d44895`, 320/320 `dirty 0`) and a focused
20-seed run at §4.1's own configuration (`results/p10_reliability`, 40 cells):

| | claimed | measured on a sourced grid |
|---|---|---|
| §4.1 `ca_tbl_full` | 19/20 | **1/20** (legacy), **0/20** (propagation) |
| §4.1 energy ratio | 1.56× | **52×** (legacy), **21×** (propagation) |
| §4.2 cells meeting the requirement | 1 of 320 | **0 of 320** |

**This is not a finding that the claims are false.** It is a finding that they cannot
be checked. An energy ratio wrong by 13–33× is a different configuration, not a
different metric, and no invocation recorded anywhere in this repository reproduces the
published numbers — every `dwell = 800` sidecar in the tree was written by the
2026-09-05 regeneration. The flag set the original campaigns used is unrecoverable.

**What survives from them.** The qualitative shape of §4.1 does: `static_2p4` ranges
accurately (median R 3.0–3.2 m) and fails on availability (median A 0.78), while the
tiered arm buys availability at a large energy multiple. That is visible in the
regenerated grid. What does not survive is any number.

Full analysis: `report/P10_RELIABILITY_REPRODUCTION.md` and `report/A0_APPLIED.md`.

**Consequence for §4.** This section is headed *Results that stand*. Two of its four
entries no longer do, and the two that remain (§4.3, §4.4) should be read as the
section's actual content.

### 4.3 The dimensionless axis transfers

Sources: `results/p10_env_v7`, `results/p10_env_v8`.

Correcting the tier-2 sensitivity defect moved `r80` from 909.5 m to **1149.5 m**, a
26% change. The region where `static_2p4` loses availability while the tiered arms
retain it moved **~29% in physical units** — `boxHalf` ~1935–2515 to 2500–3000 — and
stayed at **`spacing/r80` 1.02–1.23** in dimensionless units.

The axis was chosen to be scale-free and it survived a 26% change in its own
denominator. This is a result about **method**, and it does not depend on any claim
passing.

> *2026-09-29.* Not re-tested. Both `r80` values here are the split environment's (CR-15).

### 4.5 The design requirement, measured — registered 2026-10-20

The requirement is `Architecture.md` §1.3: A ≥ 0.95 at R ≤ 25 m over **all** cycles, a cycle without a fix counting
against it. It is read from traces by `scripts/goal_report.py`. Every campaign here runs the realistic world: unknown
start, a real gyro bias and stride noise, finder position error, the 5-state IMU filter with a magnetometer. Each was
registered before any cell and scored once.

**PRESET-2** (`report/PRESET2_RESULTS.md`, seeds 101–120). The candidate set was not adopted by its rule
(DESIGN_DECISIONS 17). Its measured effect, CA-TBL, S → N:

| config | A(25 m) tag 0 | A(25 m) five-node | subject J |
|---|---|---|---|
| 5 km | 0.386 → 0.829 | 0.268 → 0.503 | 19.4 → 16.2 |
| endgame | 0.994 → 0.995 | 0.856 → 0.891 | 21.2 → 20.3 |
| 8-gateway ring | 0.790 → 0.959 | 0.505 → 0.658 | 17.8 → 14.7 |

**ANCHOR-SUPPLY** (`report/PREREG_ANCHOR_SUPPLY.md`, seeds 121–140), CA-TBL five-node A(25 m):
- **endgame:** four finders 0.873; **eight finders 0.970**;
- **5 km:** 0.41 with four finders and 0.58 with eight.

**What stands:**
- **The requirement is met for tag 0** in the endgame and on the 8-gateway ring (under N).
- **It is met for the five-node pool in the endgame with eight finders**, on average.
- **It is not met at the brief's 5 km three-gateway geometry** in any configuration tested.
- **Against comparators at 5 km**, five-node A(25 m):
  - **`fixed_threshold`:** level in every PRESET-2 and ANCHOR-SUPPLY cell.
  - **Poucet fusion:** level, except under ANCHOR-SUPPLY's R4, where CA-TBL leads by +0.093 [0.029, 0.159].
  - **Tag 0 under P8:** CA-TBL is **below** `fixed_threshold`, −0.036 [−0.066, −0.011].
- **In the endgame,** CA-TBL beats Poucet on the five-node pool in every finder variant (+0.06 to +0.17).
- **Energy:** CA-TBL uses about 1/20 of Poucet's subject energy (383–446 J less per run).
- **It beats both single-band arms everywhere.**
- **The escalation rule is not shown to beat `fixed_threshold`** (CR-19).

**FIDELITY-1** fired a registered abort (`report/PREREG_FIDELITY1.md`) and supports no claim. Its re-run, FIDELITY-1b,
is registered.

---

## 5. Results that do not stand

### 5.1 Claim 9 — the integrity gate does not pay, and the statement is narrower than it sounds

> *2026-09-29.* Not re-tested by B3. B3's C1 tested the witness cross-check, which is a
> different mechanism (§0.2, CR-9).

Sources: `results/c9_clean` (`source cd594c9 dirty 0`, 40 cells, 120 digest entries
verified) and `results/abl_clean` (`source ccaedb0 dirty 0`, 120 cells, 360 entries
verified).

Availability, `ca_tbl_full − ca_tbl_no_integrity`. Negative means the gate **loses**.
Estimator: mean of per-seed differences over 20 seeds, sign test beside it.

| policy | F1 | sign | F3 | sign |
|---|---|---|---|---|
| `none` | −0.0305 | 1/18/1 | −0.0540 | 1/19/0 |
| `inflate` | −0.0053 | 7/13/0 | −0.0085 | 3/17/0 |
| `bounded` | −0.0152 | 5/15/0 | −0.0136 | 2/18/0 |
| `reinit` | −0.0164 | 5/15/0 | −0.0172 | 2/18/0 |

**Negative in the mean in all eight cells**, with the sign test running against the
policy in all eight. Detection itself is sound — the gate fires 120–206 times per run
and the arms differ in every cell — but rejecting a fix subtracts availability without
supplying a better one.

The ordering `inflate < bounded ≈ reinit < none` in loss, roughly a 6× spread, is
reported as a **measured value with no mechanism claimed**. It is consistent with a
bounded response subtracting less than an unbounded rejection; no measurement
establishes that, and it is not asserted.

**What was actually falsified, stated carefully.** Both campaigns ran
`suspectForcesEscalation = false`. That flag removes a documented side effect under
which a suspect fix *also* forces an escalation. So these campaigns isolate
**rejection alone**: detect a bad fix, discard it, and do nothing else.

**The combined detect-and-escalate mechanism was not falsified.** Under
propagation-derived NLOS it moves `cep50M` from **92.15 m to 3.10 m**. The correct
statement is *rejection without escalation does not pay*, not *the integrity check
does not work*. The difference between those two sentences is the difference between
an accurate result and an overclaimed negative.

### 5.2 Claim 8 — the confidence model does not beat a flat threshold

`ca_tbl_full` 19/20 deployments at 24.26 J against `fixed_threshold` 18/20 at 22.77 J.
The flat variance threshold escalates on nearly the same occasions. Falsified thinly,
and reported as thin.

> *2026-09-29, restated.* Under the repaired forms it is not a thin loss but the same
> controller (CR-8).

### 5.3 Claim 3's original window — withdrawn

The ledger read *PROVEN, SCOPED — window 1.0–1.3*. Every pre-registered verdict in the
campaign that window was drawn from was **FAIL or INCONCLUSIVE**, and the INCONCLUSIVE
entries read *"`ca_tbl_full` does not meet (A,R)"*. A window was inferred from a region
in which the proposed system did not meet its own requirement. Withdrawn with its
reason; replaced by §4.2.

### 5.4 Two withdrawn tables, both pre-D1

**The 19/20-vs-14/20 deployment-reliability table** was measured with tier 2 six
decibels deaf. Regenerated (`results/p10_plane_v7`, `source 63bba4f dirty 0`):
`ca_tbl_full` 19/20 → **20/20**, `static_2p4` 14/20 → **19/20**. A five-seed
reliability gap became one seed, at 2.1× the energy, and `static_2p4` also wins the
energy metric — 12.679 J against 27.094 J. **The argument does not survive.**

**The integrity ablation** (positive 4, negative 1, tied 15) is withdrawn as **pre-D1,
not as wrong**. It reproduces exactly from its cited directory — reproducibility
established provenance, never validity. The mechanism: the two arms differ only at the
gate, whose trigger reads `nNlos`, and **tier-2 occupancy in that campaign is 99.6%**
(27 322 cycles against 108 on tier 1), so `nNlos` and the residuals come almost
entirely from the links the six-decibel error changes. Gate eligibility measured at
**151/28 718 = 0.53%** all-LOS fix cycles, yielding **9 suspects over 20 seeds**. A
sensitivity error that moves which anchors are heard moves `nNlos`, and it does not
cancel in the difference between the arms.

### 5.4b Claim 7 — packet-level physics realism, re-scoped from PROVEN

The ledger asserted *Physics realism on packets — PROVEN*, citing a CRLB ratio of
1.105, a UWB ranging floor of 0.154 m, and energy reconciled to ten significant
figures.

**Those three checks pass and are unchanged.** What they verify is **ranging
precision and energy accounting**. None of them reads the receiver sensitivity
tables — they measure ranging error and energy, both downstream of whether a packet
was received at all.

The six-decibel indexing error of §6.1 sat in those tables, on the tier carrying ~95%
of active cycles, and **passed every gate, verifier check and validation in the
project**. An unqualified fidelity claim cannot stand after a physics defect that the
fidelity checks did not catch.

Asked directly: **no check in the project would catch that defect today.** What found
it was a closed-form propagation model built to reproduce the engine and refusing to
be tuned when it could not — not a gate. The honest status is that packet-level
physics is **verified for ranging precision and energy accounting, and unverified for
receiver sensitivity**. §8.4 specifies the check that would close it.

### 5.5 PCS escalation under propagation NLOS

With PCS as the sole escalation trigger and the uncertainty term saturated,
**escalation cannot fire however large the error becomes.** Measured: `term1` 0.0006,
`term2 + term3` = 1.9173 against a threshold of 1.55, **0 escalations**, 1440/1440
cycles on tier 1, `cep50` 89.91 m — with **NEES 1.95**, a correctly calibrated filter
whose confidence metric cannot express its own uncertainty. Control: the same
configuration under `legacy` NLOS has `term3` 0.1932, sum 1.1932, and escalation
fires.

**Three qualifications narrow this, and the registered prediction was a miss:**

- The both-hold fraction spans **0.054 to 0.998** across ten populations rather than
  clustering as predicted.
- **Registered prediction 1 is a MISS.** The discriminating variable is the position
  error, not the LOS term: saturation is common where error is already 68–90 m and
  rare where it is 2.9–4.3 m. Saturation is **coupled to divergence rather than
  independently causing it**.
- In **nine of ten populations the suspect path supplies escalation**, so PCS's
  contribution is unobservable there. Only one population isolates PCS as the sole
  trigger, and it is the only one with zero escalations.

> *2026-09-29, scoped.* The zero-escalation observation belongs to its own population. It
> does not carry to the corrected environment (CR-2).

---

## 6. Defects the method found

### 6.1 A six-decibel sensitivity-table indexing error

`GetSensitivityThreshold` computed `sf - 7` against tables whose own headers declare
`index 0 = SF5`. At the configured SF7 the 2.4 GHz engine returned **−100.0 dBm**, the
SF5 cell, instead of −106.0 — on the tier carrying ~95% of active cycles. Indices 6
and 7 were unreachable.

**Found because a result was impossible.** A closed-form propagation model built to
reproduce the engine implied `P(deliver | NLOS)` **below zero** at 1500 m. Four
mechanisms were proposed and refuted before the model itself was found to be feeding
aggregate columns into a per-link formula; correcting that left a residual that
resolved to the table index. **No gate would have caught it** — the fidelity checks
verify ranging precision and energy accounting, both downstream of whether a packet
was received at all.

Effect: tier-2 `r80` **909.5 m → 1149.5 m**. Every result predating it was regenerated.

### 6.2 PCS unweighted-sum term saturation

Three terms each bounded by `[0,1]`, summed without weights against a threshold of
1.55. The uncertainty term tends to 0 as the covariance grows, so once it saturates,
geometry and LOS fraction alone hold PCS above threshold regardless of how uncertain
the filter is. It can fail to contribute; it can never veto.

**Found because a result was impossible**: 361 suspects and 0 escalations in the same
run, with a filter calibrated at NEES 1.95. Recorded, **not fixed** — a fix changes the
engine and invalidates the results tree.

> *2026-09-29, extended.* This still describes `legacy`. The repaired forms saturate the
> other way (CR-3).

### 6.3 The gate suite overwrote the evidence it verified

The suite regenerated three tracked result directories **in place**, silently, and
re-dated their sidecars to the last gate run rather than the run that produced the
cited numbers. A verification step that rewrites its own subject cannot establish
whether the subject was correct before it ran.

**Found because a check failed** — a diagnostic batch compared the tree at start and
end and caught six modified files. Fixed: regeneration goes to a temporary directory,
tracked copies are read-only, and a new gate fails the suite if it exits with a
modified tree.

### 6.4 The degeneracy screen conflated four mechanisms

The screen returned one verdict per run, derived mostly from tier occupancy. It failed
a valid pre-flight in which the integrity gate fired 86 times and the arms differed in
37 fields, and it flipped verdict on **1.4 percentage points** of that incidental
quantity — 97.8% passed, 99.2% failed, same configuration and flags.

**Found because a check failed on a campaign that was demonstrably live.** Fixed to
report per mechanism.

**And one of the corrected rules was wrong.** The energy rule flagged *any* non-zero
draw on a zero-cycle tier and reported **66.5%** of the corpus degenerate. An
unoccupied tier still draws CAD/idle listening energy — `static_2p4` draws 0.014 J on
868 MHz and 0.008 J on UWB while occupying neither — which is correct behaviour.
Requiring a material share (>1% of total, a threshold of ours and not physical) gives
**0.1%**. Caught before publication; recorded because a methodology section that
reports its own screen's false-positive rate is more credible than one that does not.

---

## 7. The structural finding

The project registered a **cost** criterion — PASS requires the tiered arm's energy to
be lower than a qualifying competitor's, with a bootstrap CI entirely below 1.0 — for
an architecture whose value proposition is **reliability**.

The two select for opposite conditions:

```
screening rule   selects cells where the single-band arm FAILS
verdict rule     requires the single-band arm to QUALIFY
                 (a cost win needs a competitor in the race; "no competitor meets
                  the requirement" is INCONCLUSIVE by the rule's own text)
```

**Demonstrated, not argued.** A pre-registered 320-cell grid was run; its screening
rule and its verdict rule carried **disjoint cell sets**, and the single cell that
passed sat in a combination the screen had rejected. All ten cells where the rule could
adjudicate at all were `RURAL_OPEN` — the easiest environment, the one where tiering is
least needed — because only there does a single-band arm also qualify.

The registered metric cannot express the claim the architecture makes. At
`spacing/r80` = 1.02 the tiered arm meets the requirement in **82% more deployments**
for **27% more energy**; M1 asks only which qualifying arm is cheapest, and reliability
enters solely through a binary qualifier. That limitation was named in the
pre-registration *before* the campaign ran, which is what makes it an observation about
the metric rather than an excuse constructed after an unwelcome result.

---

## 8. What a corrected study would register

Specified, untested, with the measured motivation attached to each.

1. **A joint criterion over availability and energy**, with the trade-off stated in
   advance — for example a maximum acceptable energy multiplier per additional
   deployment met — rather than one axis binarised into a qualifier. Motivation: §7,
   and the 82%-more-deployments-for-27%-more-energy cell the current metric scores as
   FAIL.
2. **A weighted or gated PCS.** A weighted sum, a `term1` gate that vetoes when the
   covariance term saturates, or `min` instead of sum. Motivation: §6.2 — all three are
   plausible and **none has been run**.
   > *2026-09-29, withdrawn.* All three have now been run (CR-4), and CAL-01 calibrated
   > the weighted form's three weights (CR-7).
3. **Mechanism-aware degeneracy screening as a mandatory one-seed pre-flight.**
   Motivation: §6.4, and the 27.3% / 40.0% of arm-cells that cannot speak to integrity
   or trigger claims respectively.
4. **A tier-2 fidelity check that reads the sensitivity tables.** Motivation: §6.1 — no
   check in the project would catch that defect today.

---

## 9. Limitations

**The one qualified citation.** `[M-24]` — integrity detection separation ratios, the
F0 suspect rate, and detection latency — rests on `results/campaign_20260810/stage1`,
which records `dirty 4`. It could not be repointed: at the time no injection campaign
anywhere recorded `dirty 0`, and the two campaigns that now do
(`c9_clean`, `abl_clean`) do not re-measure those particular statistics. Closing it
would require a fifth campaign of roughly 3.8 hours. **Every other number published as
a result traces to a `dirty 0` source.**

**Single-scenario.** One mobility model, one deployment geometry family, one seed
family. The environment sweep covers four `envFacility` levels and two NLOS sources;
it does not cover terrain, weather, or node mobility patterns beyond a facility tour.

**Simulation, not hardware.** Radio parameters come from datasheets with cited
`file:line` provenance, but no measurement campaign validates the engines against real
SX1262/SX1280/DW3000 hardware.

**PCS is recorded as defective and left unfixed.** §6.2's saturation is a live defect
in the shipped configuration. Fixing it changes the engine and invalidates every
result in this report, so it is recorded rather than corrected.

> *2026-09-29, restated.* `legacy`, the default, is still unfixed. Opt-in repaired forms
> exist behind `--pcsForm`, and they saturate low (CR-5).

**The reliability result in §4.1 has no verdict** because no reliability criterion was
registered. It is reported as a measurement, and a reader who wants a verdict must
supply the criterion — which is exactly the gap §8.1 exists to close.

**Screened-out evidence.** 27.3% of arm-cells cannot speak to integrity claims and
40.0% have no trigger population. Those runs are not defective; they were not designed
to exercise those mechanisms. Reading a number off them would be reading a number with
no population.

---

## 10. Corrections register

**Added 2026-09-29.** This register lists every statement that §0 withdraws, restates or
scopes, whether it was made in this report or in a document this record draws on. Entries
are appended and never edited. A statement that moves again gets a new entry. "B3" means
`report/B3_REGEN_RESULTS.md`. Every number is quoted from the read-out in the evidence
column; none is computed here.

| # | statement, and where it was made | correction | evidence |
|---|---|---|---|
| CR-1 | "A confidence score (PCS) driving tier escalation" — §1, §2 | **RESTATED.** Under every repaired form (v2, weighted, gated), PCS sits below every escalation threshold on ≥ 99.98 % of fix cycles. It decides nothing, and the downstream gates decide. Under `legacy` it sits above threshold on 81–86 % and asks to escalate on the remaining 14–19 % | B3 §C2 and "What this changes" item 2; `results/regen27_pcs_forms` |
| CR-2 | "Escalation cannot fire however large the error becomes" — §5.5 | **SCOPED** to the population it was measured in. In C2's population (legacy, propagation NLOS, `ca_tbl_no_integrity`, where PCS is the only trigger), the trigger is true on 3 321 of 23 213 fix cycles, and the arm escalates 93.2 times per 3 600 s on average | B3 item 4; `results/regen27_pcs_forms` |
| CR-3 | "It can fail to contribute; it can never veto" — §6.2, and §1's list of defects | **EXTENDED.** It is still true of `legacy`. The repaired forms saturate the other way. With `traceSigma` at 7.8×10⁴–1.2×10⁵ m², term 1 is about 10⁻⁴, so v2, weighted and gated sit below every threshold and every fix cycle wants to escalate. The defect is the scale of term 1 against the filter it reads, not only the unweighted sum | B3 item 5 and §C2; `results/regen27_pcs_forms` |
| CR-4 | "A weighted or gated PCS … **none has been run**" — §8.2 | **WITHDRAWN.** All three have been run twice: first in `report/PCS_FORMS_RESULTS.md`, then in B3 C2. In the regenerated campaign `gated` is worse than `legacy` on cep50 (+12.96 m, 15+/5−, p = 0.041: P2.1 SIGN FLIPS). The monotone-control negative holds for legacy, v2 and weighted, and is undefined for gated | B3 §C2 and item 6; `results/regen27_pcs_forms` |
| CR-5 | "PCS is recorded as defective and left unfixed" — §9 | **RESTATED.** The default, `legacy`, is still unfixed, on purpose (`MASTER_PLAN.md` A8). Opt-in repaired forms exist behind `--pcsForm`, and B3 measured them saturating low (CR-3). CAL-01's fitted weights are not adopted, and the default weights in code are unchanged | B3 item 7; `report/CAL01_LOOCV_RESULTS.md` §2; `results/cal01_loocv_20260928` |
| CR-6 | "`gated` beats `legacy`" and "`gated` buys the same accuracy for a fifth of the energy" — `report/PCS_FORMS_RESULTS.md`. "Every form loses, in every cell", with its quoted gaps — `docs/TECH_DEBT.md` K.8 | **SIGN FLIPS, WITHDRAWN and RESTATED.** `gated` is worse than `legacy` on cep50 (P2.1). "`gated` is cheaper" is withdrawn: its rows are byte-identical to `weighted`'s on 18 of 20 seeds (P2.3). K.8's statement belongs to the pre-correction binary. The regenerated record: the monotone-control negative reproduces in 6 of 8 form × arm cells (weighted `no_integrity` −0.0984), and the statistic is undefined for the two `gated` cells | B3 §C2 and "Elsewhere"; `results/regen27_pcs_forms` |
| CR-7 | "Confidence-awareness (PCS) earns its place — measured negative … give the hop and NLOS terms real inputs, then re-run the control" — `docs/TECH_DEBT.md` L.3; `HANDOFF_2026_09_09.md` §2 | **NOT SUPPORTED. CAL-01: CALIBRATION DOES NOT RESCUE PCS.** With the NLOS term live, leave-one-seed-out calibration ranks error no better than the covariance trace in any of three regimes. A = −0.0016 [−0.0066, +0.0030], +0.0045 [−0.0129, +0.0217] and −0.0011 [−0.0044, +0.0001] (98.33 %). The fitted weights are (3, 0, 0) to rounding. The NLOS term ranks error at \|ρ\| ≤ 0.06. The hop term cannot be given an input, because the measured tag's `Step()` receives the hop count as a literal 0. So the contribution, that PCS carries information about error beyond the trace it is computed from, is not supported by any linear calibration of its terms. Withdrawing the novelty claim is the owner's decision. The committed machinery's own acceptance flag, `CA_TBL_CAL_OK`, is not evidence (`report/CAL01_LOOCV_RESULTS.md` §4; `docs/TECH_DEBT.md` M.7) | `report/CAL01_LOOCV_RESULTS.md` §1–§5; `results/cal01_loocv_20260928`; B3 §C2 |
| CR-8 | Claim 8, "falsified thinly": `ca_tbl_full` 19/20 at 24.26 J against `fixed_threshold` 18/20 at 22.77 J — §1, §5.2 | **RESTATED.** Under each of v2, weighted and gated, `ca_tbl_no_integrity` is byte-identical to `fixed_threshold` on 19 of 20 seeds. That is not a thin loss but the same controller. In K1, `ca_tbl_full` dominates `fixed_threshold` at `k1a_box750` and `k1a_box1000`, is incomparable at `k1a_box1500`, and is dominated in the other eight cells. Claim 8 was not re-registered, so these are reported, not verdicts | B3 §C2, §C4 and item 3; `results/regen27_pcs_forms`, `results/regen27_k1` |
| CR-9 | "The integrity check earns its place — supported, narrowly": 8/8 energy, 7/8 accuracy, for the witness cross-check — `report/INTEGRITY_CHECK_RESULTS.md`; `docs/TECH_DEBT.md` L.3; `HANDOFF_2026_09_09.md` §2 | **WITHDRAWN in the shipped default.** B3 C1 found NO POPULATION: off and on rows are byte-identical on 8/8 seeds, although the IMU witness refused 203–232 escalations per seed. The evidence gate, default true since `7ec0d91`, absorbs the refusals. B3's registration omitted that default (`report/PREREG_B3_REGEN.md` §2; B3 is its erratum). C1_NOGATE removed the gate, and the refusals then reached every row. With the gate off, energy REPRODUCES (ΔtotalJ −104.86 J per 600 s, CI [−107.97, −101.12], 20/20 seeds) and accuracy is NOT SHOWN WORSE (ΔrmseM −0.66 m, CI [−9.62, +11.24]). But the IMU witness refuses on 85–98 % of cycles, a near-blanket veto on escalating into UWB. No sentence about integrity, trust or detection follows from either campaign. The original's sidecars read `deb52da`, which does not define the flags it ran | B3 §C1; `report/C1_NOGATE_RESULTS.md` §3, §7; `results/regen27_integrity`, `results/regen27_integrity_nogate` |
| CR-10 | "Tiering beats single-band — not supported; statistically indistinguishable from `static_868`", and K1's "`ca_tbl_full` dominates `static_868` in no cell" — `report/K1_DOMINATION_RESULTS.md`; `docs/TECH_DEBT.md` L.3 | **Still NOT SUPPORTED; restated as an envelope.** P4.1 is FALSIFIED in 3 of 11 cells: `k1a_box750`, `k1a_box1000` and `k1a_box1500` give DOMINATES, energy 20+/0− in each. Those three verdicts are a billing effect. With tier-1 replies billed, a tier-1 cycle costs 18.5–24.0 mJ and a tier-2 cycle 5.5–8.6 mJ. With floor-on-demand, the 78–88 % of cycles spent on 2.4 GHz replace tier-1 time, so escalating *leaves* the dearer tier. `static_2p4` dominates `ca_tbl_full` in 11 of 11 cells. The cells have not been re-run under the gateway-estimator default, which removes that reply bill | B3 §C4; `results/regen27_k1` |
| CR-11 | "The architecture beats published estimators", K10: 2 016 m against MLAT 20 831 m, 20+/0− — `report/K10_BASELINE_RESULTS.md`; `docs/TECH_DEBT.md` L.3 | **REPRODUCES on the corrected environment**, and is to be cited from the regenerated directory. The columns C3's verdicts read are byte-identical to the originals on all 120 rows per cell. K10's qualifier stays: a filter beats a memoryless solver, and the tiering is not exercised (`ca_tbl_full` escalates 0 times on 20/20 seeds in `wanonly_move`) | B3 §C3; `results/regen27_k10` |
| CR-12 | Availability WAN only < sparse < WAN + mesh — `report/K9_SAR_FACTORIAL_RESULTS.md` | **REPRODUCES** (0.041 / 0.319 / 0.647), and is restated as the mesh's, not tiering's: `static_868` reaches 0.451 in `sparse_move`, above `ca_tbl_full`'s 0.319 | B3 §C5; `results/regen27_k10` |
| CR-13 | "Gateways as edge servers — built, and it can only cost energy" — `docs/TECH_DEBT.md` K.4 | **RESTATED.** It is true on tier 2, and under the earlier no-reply billing. Under the default reply billing, estimation at the gateway lowers totalJ by 42.5 % (`static_868`) and 32.9 % (`fixed_threshold`, `ca_tbl_full`) on 20/20 seeds, with accuracy byte-identical. `static_2p4` costs 0.2 % more. It has been the default since `db2ac2e`. The two hybrid figures were measured with the downlink billed on the active tier, which is an open owner decision (`docs/TECH_DEBT.md` M.10). **2026-09-30: decided.** The downlink is billed on the fix's tier by default. The two hybrid figures describe `--edgeDownlinkTier=active`, which reproduces them, and have not been re-measured under the new default | B3 §C6; `results/regen27_edge`; `docs/TECH_DEBT.md` L.2.3, M.10 |
| CR-14 | The IMU ceiling, `static_uwb` cep95 8 413 → ~17 m and `static_2p4` 27 151 → ~537 m (`4302918`, `results/L2_imuon_*`), and the registration's disclosure that "some code path on the IMU-fusion invocation moved" — `report/PREREG_B3_REGEN.md` C7 | **REPRODUCES, as a ceiling and never as a system result**: 8 413 → 17 m and 27 151 → 537 m, 20+/0− each. It is not dead reckoning alone: `mac_tx` differs on 20/20 seeds on every arm. The disclosed seed-7 non-identity is the snapshot solver's `rmse_mlat` tail, not the IMU path | B3 §C7; `results/regen27_imu` |
| CR-15 | Tier-2 `r80` of 1149.5 m — §2, and the `spacing/r80` axis of §4.3 and §7 | **SCOPED.** The figure describes the split environment. Under the corrected environment, the probe in `report/K1_DOMINATION_RESULTS.md` gives 156.0 m for 2.4 GHz; 868 MHz is 1515.3 m under both. §4.3 and §7 rest on the old value and were not re-registered | `report/K1_DOMINATION_RESULTS.md`; B3 item 8 |
| CR-16 | "The proxy answers only inside 120 s and at this deployment entries are normally older, so it refuses (7 refusals / 0 answers at the spec bound)" — `docs/TECH_DEBT.md` L.2.5, as recorded 2026-09-09 | **RESTATED by measurement: true of a peer, false of the casualty.** Casualty, arc searcher regime: 265 of 265 held entries inside 120 s, with median age 11.5 s and 90th percentile 28.6 s. Grid-2 5 km sparse regime: 37 of 39. A peer: median 493.4 s (21 of 130 inside) and 384.9 s (5 of 20). The 120 s default stays | `report/PREREG_L25_PROXY_AGE.md`; `results/l25_proxy_age_20260929/ANALYSIS.md` |
| CR-17 | Every `--relayedRanging` figure recorded before `d24690c`: `MASTER_PLAN.md`'s C5 row, `b390ed6`'s depth sweep and `5f3ad42`'s routing counts | **SCOPED.** They were measured while every relay exchange shared the cycle's one instant and collided in the interference registry. With `--relayedRanging` on, the measured tag's 3+-anchor cycles fall from 107/112/106 of 120 to 2/0/11; with `--relaySlots` they are 103/104/107. They do not compare with a `--relaySlots` run. No `results/` sidecar uses `--relayedRanging=true`, so no tracked result moves. Whether `--relaySlots` becomes the default is an owner decision. **2026-09-30: decided.** `--relaySlots` is on by default in relayed runs. `--relaySlots=false` restores the collision these figures were measured under, and reproduces the commit before the change byte-for-byte | `docs/TECH_DEBT.md` M.1; no results directory |
| CR-18 | "All four [starved mechanisms] are the same fact: tier 1 blankets the box", and "`runFloor` is true on 99.9 % of cycles" — `docs/TECH_DEBT.md` L.2; `HANDOFF_2026_09_09.md` §6.2 | **RESTATED from an EXPLORATORY probe; not a result.** Coverage explains Case A and Case B only. The Mahalanobis check's eligible population is structurally near-empty under the default NLOS source, and `--nlosSource=propagation` changes that. Covariance Intersection stays near zero because a second copy of the same sequence number almost never arrives, whatever tier 1 does. Routing has a population under `--fixRouting`. `runFloor` holds on 600 of 720 cycles at defaults; the 99.9 % is the finder regime | `docs/TECH_DEBT.md` L.2, 2026-09-29 note; the probe's outputs are outside the tree |
| CR-19 | "A confidence score (PCS) driving tier escalation" as a contribution — §1, §2; the novelty claim CR-7 left to the owner | **WITHDRAWN 2026-10-20 (owner decision, the approved goal plan; `docs/TECH_DEBT.md` Q).** PCS is the covariance trace by every registered test (CAL-01, B3 C2). The tiering claim is restated: acquisition, availability and energy against single-band arms, not the escalation rule against `fixed_threshold`, with which CA-TBL is level on A(25 m) in PRESET-2 and ANCHOR-SUPPLY | `report/CAL01_LOOCV_RESULTS.md`; §4.5; `results/preset2_a1_2026-10-20`, `results/anchor_supply_2026-10-20` |
| CR-20 | "UWB ~3.7 dB and 868 MHz ~3.5 dB unattributed residuals" — `RULES.md` §5; G11a; TECH_DEBT 2.3 | **SUPERSEDED.** Measured against `probe_v7`, which predates `50fc8620` (tiers 2/3 into the forest). Against the engine as it runs, UWB's margin reconciles within 2.12 dB and 868 MHz is unchanged. The open residual is 2.4 GHz's slope, which decomposes into foliage density (0.8 on A_max), terrain and the IQ degradation, none in the model | `src/catbl-propagation-study/experiments/VAL_V8_REGISTRATION.md`, `IQ_TERM_REGISTRATION.md`; `results/probe_v8`; TECH_DEBT R.1, R.7 |
| CR-21 | Every preset result before `39c1dfee` | **SCOPED.** The preset now sets `--uwbSensitivityDbm=-99.3 --iqOverloadOnly` (DESIGN_DECISIONS 18). Earlier preset results reproduce with `--uwbSensitivityDbm=-104 --iqOverloadOnly=false` given (pinned X1: 274/275 files identical, the 275th the provenance lines). The size of the change awaits FIDELITY-1b | `report/PREREG_FIDELITY1.md`, `report/PREREG_FIDELITY1B.md` |
