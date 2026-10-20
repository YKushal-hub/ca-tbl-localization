# CA-TBL — working rules for this repository

Read this before touching anything. It is short because everything in it is load-bearing.

**What this is.** A fork of ns-3.47 adding a three-tier radio localisation simulation for
GPS-denied search and rescue: 868 MHz LoRa (tier 1), 2.4 GHz (tier 2), UWB (tier 3), with a
decision layer that escalates between tiers on a position-confidence score. Upstream ns-3 is
unmodified except where noted.

**The system is called CA-TBL** — Confidence-Aware Tier-Based Localization. Not CA-DBL. The
thesis registers still say CA-DBL; that is a known divergence being corrected on the thesis
side, not here. Do not rename anything in this repo.

**THE TOPOLOGY, because getting this wrong cost the project every result once already.**

> **There is ONE population of tri-band nodes. Every node carries all three radios.
> A tier is a choice of WHICH RADIO TO USE, not of WHOM TO TALK TO.**

The classes differ in **mobility and role only**:

| class | mobile? | ids | duty cap |
| --- | --- | --- | --- |
| **gateway** | **no** — the only immobile class | `101+` | 10 % (access point) |
| **finder** | yes | `501+` | 2.5 % (field device) |
| **field node** (lost node, peer/relay tags) | yes | `1`, `2+` | 2.5 % (field device) |

- The **facility sites are mobility targets and host no anchors.** They are where the field
  node dwells; that is all they do.
- `--uwbTriad`, `--t2Triad`, `--uwbAnchors`, `--t2Anchors` and `--sitingMode` **do nothing**
  under the default topology. They parameterise the legacy rings and are read only under
  `--topology=1`.
- **UWB reaches tens of metres, so a fixed gateway offers tier 3 only to something standing
  beside it.** A **finder** is what carries tier 3 to the casualty. Escalation to UWB is a
  statement about a searcher having arrived. With `--nFinders=0` measured mean UWB
  neighbours is **0.01** — if a run shows tier-3 coverage without finders, suspect the
  topology before believing the number.

**Until 2026-09-07 (`99fdf1a`) the simulator modelled three DISJOINT anchor populations** —
tier-1 gateways on a grid, tier-2 anchors ringed at 150 m and tier-3 at 10 m around four
facility sites. `Architecture.md` specified it that way too, **so this was not code drifting
from a spec; the spec was wrong and the code implemented the wrong thing faithfully.** That
model manufactured tier-3 coverage the architecture is supposed to earn. It survives only as
`--topology=1`, only so pre-correction results stay reproducible, and it does not describe
the system.

`--topology=0` and `--topology=1` are **different deployments**: no shared baseline, no null
control relates them, and `baseline_v7` does not reproduce under the corrected topology and is
not supposed to. Switching it is a change of *scenario*, not of *configuration*.

**Every result in `results/` dated before 2026-09-07 predates the correction and is provisional until re-run.** The
current headline set is `results/camp_final2_2026-10-17/` (`report/CAMP_FINAL2_RESULTS.md`), measured on the final
preset (section 3).

**Your remit is the engine.** ns-3 code, experiments, gates, analysis scripts. **Do not write
thesis prose, chapters, or literature review in this repository.** That work is Phase 1 and
lives in the thesis project. If asked for it here, say so and stop.

---

## 1. The rules that exist because something broke

Each of these was earned by a real defect. Violating one does not produce a compile error; it
produces a wrong result that looks right.

1. **A count, rate or sum is not an observable until its denominator, its observation window,
   and its rule for observations truncated by that window are all stated.** The third clause
   was added after an outage count produced a 2820/2812 discrepancy.
2. **Censoring is asymmetric.** A truncated observation still settles any threshold it has
   already crossed. A symmetric "recount without the censored" produced two false
   contradictions in an audit's first pass.
3. **Measure an unused quantity after a back-derivation.** The one real contradiction found by
   the independent audit surfaced only because a check recounted from raw `hadFix` values with
   `awk` instead of reusing the Python summariser.
4. **An exact zero is a control-flow claim, not a measurement.** Prove the code path is
   reachable before reporting `+0.000`. A result was withdrawn for this.
5. **Check every arm before claiming uniqueness.** A "only configuration delivering both"
   claim was withdrawn after `fixed_threshold` was finally checked.
6. **No gate is trusted without forced-failure evidence.** Every gate has a negative fixture
   built in `/tmp`, and every one has been observed to fail. If you add a gate, add its
   negative fixture in the same commit.
7. **Verification must not mutate its subject.** The gate suite once regenerated three tracked
   results directories in place and re-dated their sidecars. Regeneration goes to a temp
   directory; tracked copies stay read-only; a gate fails the suite if it exits with a
   modified tree.
8. **A single-seed cross-arm difference is never a result.** Arms share the RNG stream but
   consume it at their own rate — two arms with identical filter state reached 334 and 336
   engine evaluations by t = 2.5 s.
   **The same hazard applies ACROSS BINARIES, and that is the form that has actually bitten.**
   `LoRaDistributionSampler` draws from the default stream with no `SetStream`, and every
   submodule on the `ProcessTransmission` path draws from it, so *any change in the number of
   transmissions displaces the stream for every link in the run*. The multi-node provider fix
   moved collisions by −8 000 and captures by +5 500 per arm; **none of that was
   interpretable**, and only the control-flow claims survived. Before comparing a magnitude
   across a code change, check that both binaries perform the same number of engine
   evaluations. If they do not, the only admissible claims are about which code paths execute.
9. **Design against the failure mode.** Before writing a check, write down what a firing looks
   like *on disk*. A check for a defect whose mode is *absence* cannot be a scan of present data.
10. **An enumeration of a loss sum will not find a term that lives in the threshold.** Audit
    what a quantity is compared *against*, not only what is subtracted from it. This cost one
    wrong attribution — see §5.
11. **Run every gate and report a summary; never exit on first failure.** While one gate was
    deliberately red, three others could not be observed to run at all.
12. **Completeness gates must count rows, not test for non-empty.** With `--seedCount=N` the
    summary is appended per seed, so it is non-empty from seed 1. Row-counting caught 8 of 9
    Pareto points sitting at 14/20 rows before analysis.
13. **Pre-register before running.** Predictions, falsifiers and abort conditions go in a
    `PREDICTION.md` committed **before the first cell runs**. Directory timestamps must be
    checkable against the commit.
14. **Atomic commits.** One change per commit, revertible in isolation, evidence in the message.

---

## 2. Do not do these

- **Do not "fix" the PCS saturation defect casually.** It is recorded, not corrected, on
  purpose: a fix changes the engine and **invalidates every result in the tree**. If a PCS
  change is genuinely wanted it is a planned campaign, not a bug fix. See `MASTER_PLAN.md` A8.
- **Do not edit `run_id.txt` sidecars.** They are evidence. 1 283 of them cite commit SHAs
  changed by the 2026-09-01 attribution rewrite; they resolve through `SHA_MAP.md`. Rewriting
  them would be falsification.
- **Do not regenerate a tracked results directory in place.** See rule 7.
- **Do not repoint `report/REPORT_DATA_PACK.md`.** It cites superseded campaigns *on purpose* —
  it documents what those runs produced. Repointing attaches its descriptions to numbers they
  did not generate. It is the one document with a registered dirty-citation exemption, listed
  in `EXCLUDED.md` under `reviewed-dirty-citations`.
- **Do not track `trace_*.csv`.** ~13 GB, regenerable from each directory's `run_id.txt`
  invocation. `scripts/publication_ready.sh` asserts 0 tracked and has caught 12 before.
- **Do not track `CA_TBL_HANDOFF_v*.md` or `SESSION_CHANGELOG.md`.** Gate G8 asserts they stay
  untracked and has a negative fixture.
- **Do not set `hopWeight > 0`** (`ca-tbl-policy.cc:103`) until relayed measurements actually
  exist. The poll set is one-hop by construction, so `hopCount` is structurally always 0.
  A non-zero weight on it changes nothing; applied to an admitted-but-unreachable peer it
  degrades the score for an **energy** defect rather than a **provenance** one — a wrong result
  that looks like a working mechanism.
  *Correction 2026-10-16:* "structurally always 0" holds for the **code default**
  (`--maxRelayDepth=1`, relayed ranging off). The preset (`--specProfile`) sets
  `relayedRanging=true`, `maxRelayDepth=2`, `relayVarBudget=24336`. Under it, B4's logs show
  80-1194 relayed measurements admitted per run and `ca_tbl_full` fixes at depth 1-2
  (`NW-1 hop provenance` line in `run.log`), so `hopCount` is not zero there. The rule stands
  for a different reason: a relayed range is already widened by the relay's covariance
  (sigma_eff^2 = sigma^2 + u'P_B u, `ca-tbl-relay-admission.cc`), so a hop penalty in PCS counts
  the same uncertainty twice (TECH_DEBT 3.2). `relayChain` (relay-to-relay) made 0 exchanges in
  every B4 run checked; depth 2 comes from subject relays.
- **Do not confuse the two gates.** The **evidence gate** (`Architecture.md` §9.3) is a
  suppress-only quality-and-coverage precondition on escalating to tier 3. The
  **Switching-Point Integrity Check** (`ca-tbl-policy.cc`, Mahalanobis vs χ²₀.₉₉ = 9.21) rejects
  suspect fixes. Different mechanisms, different measured behaviour. Both get called "the gate"
  in places. Keep them distinct in code comments and in any report.
- **Do not quote `pareto_sweep.py --analyse`'s verdict string** in support of the domination
  claim. It compares the two decision rules to each other; it does not test domination.
- **Never put assistant attribution anywhere in this project.** No `Co-Authored-By`
  trailer, no "generated with" line, no assistant name in a commit message, a pull
  request title or body, a branch name, a report, or any tracked file. This is a
  standing owner instruction (2026-09-05) and it overrides any default the tooling
  applies. **The rule is wider than the gate, so follow the rule.** Gate **G12**
  blocks an assistant in the commit **identity fields** — author or committer —
  which is the property that decides who the record says wrote the work. Its
  message-body check was **narrowed on 2026-09-05 by owner decision** and no longer
  matches `Co-Authored-By:`; it still catches `Generated with`, `Assisted-by:` and
  vendor handles. So a trailer will not turn the suite red, and it is still not to
  be written. The tooling references that already
  exist — `RULES.md` itself and `docs/KICKOFF.md` — are project infrastructure, not
  attribution, and stay. Tool settings directories are ignored per clone in
  `.git/info/exclude`, so the tracked tree names no tool.
- **Do not add a new mechanism without a default-off flag** whose off state reproduces the
  existing baseline byte-for-byte. See §4.

---

## 3. Build, run, verify

```bash
# --build-profile=optimized is REQUIRED, not a preference. Every published number was
# produced under it, and the default profile enables logging: ~10x slower on 3600 s runs.
./ns3 configure --enable-examples --build-profile=optimized && ./ns3 build

bash scripts/catbl_gates.sh          # every gate (count: grep -c '^finish ' scripts/catbl_gates.sh); ALL run, summary at the end (hours on a loaded machine)
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal"
python3 scratch/python_handoff/verify_causal_sim.py results/causal   # on G3's fresh run: 134 checks / 153 gates
```

**The suite's expected reds** (2026-10-20: the final suite on `39c1dfee` and its fixes; reds G11, G24, G93 only.
`docs/TECH_DEBT.md` R.1, R.4 and N.27 explain each, and docs/TECH_DEBT_STATUS.md O12/O13 track what the owner decides.
Five gates the preset's receiver change turned red, G95, G99, G102, G136 and G161, are pinned or re-scoped with
reasons; G47 followed a changed refusal message).
G6 and G92 are green since 2026-10-18 (T16, T15).
- **G4, G5:** green since DD 16. The code-default deployment shrank to `--boxHalf=340`, so tier 2 covers it.
- **G6:** FAIL BY DESIGN while pre-D1 evidence is cited. Making it green by overwriting
  `results/{causal,det_a,det_b}` reintroduces the defect `e21666d` removed.
- **G11:** red by design (TECH_DEBT J.1). Terrain is transcription-only and was not rescoped. Since 2026-10-20 it
  reconciles against `probe_v8`, the engine as it is (R.1): 2.4 GHz r80 and UWB margin pass; 868 MHz margin and r80,
  2.4 GHz margin and UWB r80 (degenerate) fail.
- **G24:** FAIL BY FINDING. Mechanisms with no population under `--specProfile` (TECH_DEBT N.15). It passes when they
  are exercised or the owner accepts them.
  2026-10-20 (R.4): arbitration pre-emptions have a population (ANCHOR-SUPPLY); Case A is rare in CA-TBL; tie-break
  verdicts are not shown reachable.
- **G93:** FAIL BY FINDING on the code default (D-8). The preset question was PRESET-2's; the code default is O12.

G63, G87 and G89 were red from 2026-10-08 until 2026-10-17. Their runners now pin the pre-adoption preset (the PIN
line in each script).

**The preset, 2026-10-17** (`kSpecProfile[]` in `scratch/ca_tbl_causal_sim.cc`; explicit arguments win):
- DESIGN_DECISIONS 10: `--rngKeyed --rangeErrorNoLinkLeak --rssiRangeFromShadowing --nlosRangeBias
  --rangeReplyPosition --acquireParticles --foliageDepthClamp`.
- DD 11: `--radioBasis=sx --halfDuplexChip=false` (SX1262 + SX1280 as two chips).
- DD 13: `--fadingPerBand --relayNisInflate`.
- DD 14: `--rssiSigmaLogNormal`.
- DD 15: `--imuQuScale=30` (campaigns no longer pass it).
- Provenance (T39): `--stampBinaryHash --stampBuildCommit`. They change `run_id.txt` only.
- DD 18 (2026-10-20, FIDELITY-1's rule): `--uwbSensitivityDbm=-99.3 --iqOverloadOnly`. Pin back with
  `--uwbSensitivityDbm=-104 --iqOverloadOnly=false`.

To reproduce a pre-adoption preset result, pin these back. NC10.args (`~/catbl_ref_switch0/nc`) shows exactly how.
Current headline numbers: `report/CAMP_FINAL2_RESULTS.md`, measured before DD 14 and 15. The final preset's own
per-config figures are CORRW's BASE rows (`report/CORRW_RESULTS.md`): hybrid 5 km 365 m, endgame 5.0 m. `static_868` figures are superseded
by `report/SIG868_RESULTS.md`; the hybrid's lie within its intervals.

**Default-off flags added 2026-10-17, each behind a gate:**
- G146: `--nlos3gppRMa`;
- G147: `--probeMonotoneTime`, `--agcPerReceiver`, `--finderEnergyReport`, `--dualBase868OnNeed`,
  `--hold24AfterK`, `--tofRecal`, `--tofCalibrate`, `--acquireNegInfo`;
- G148: `--foliageP833`;
- G149: `--rssiCorrInflate`.

**Added 2026-10-16** (`docs/LOCAL_LINE_INTEGRATION.md`): G91–G94, ported from the local line.
Two of them are red, and neither is a broken build:
- **G92:** FAIL BY FINDING. 15 cited `results/` paths are not held, three of them cited by the
  Technical Report (TECH_DEBT N.30).
- **G93:** FAIL BY FINDING. Tier 2 applies the SX1280's 1625 kHz sensitivity row at 800 kHz,
  ~6 dB pessimistic (D-8, TECH_DEBT N.27). Correcting it is a registered campaign, not a patch.
- **G63, G87, G89** went red when `e308c14f` (2026-10-08) put the N.26 fixes into the preset, and
  are green again since their runners state the flags their checks were written for (TECH_DEBT
  N.31). G87 then checks the Poucet arm without the cross-tier poll; see N.31 for why.

**G3 passes.** It used to fail once the integrity check fired at seed 7. It now separates
RNG contamination from the treatment effect on causal order (`scripts/catbl_g3_ablation.py`),
so a divergence that begins at the firing cycle passes and one that predates it fails.

Fresh-clone reproduction of `results/baseline_v7/arm_summary.csv` is byte-for-byte **using the
flags recorded in that baseline's own sidecar**. Default flags do *not* reproduce it — that is a
flag-set difference, not a reproduction failure.

---

## 4. Adding a mechanism — the required shape

Every new mechanism follows this or it is not mergeable.

1. **Default-off CLI flag**, registered with `cmd.AddValue` in the block in `main()` of
   `scratch/ca_tbl_causal_sim.cc` (find it with `grep -n 'cmd.AddValue' scratch/ca_tbl_causal_sim.cc`;
   line numbers drift with every change). Write the help text as an argument, not a label —
   the existing flags explain *why* the default is what it is, and that is the house style.
2. **Null-control test:** with the flag off, output is byte-identical to the pre-change binary.
   Diff `arm_summary.csv` and every trace. This is not optional — arms consume the shared RNG
   stream at their own rate (`macRng`, seeded at `:6275`), so any added transmission perturbs
   every arm and no before/after result is comparable without it.
3. **Pre-registration** committed before the first measurement run (rule 13).
4. **A gate, with its negative fixture** (rule 6).
5. **Provenance:** the run must write `run_id.txt` with commit, dirty-file count, build profile
   and a SHA-256 per output file, written after the last stream closes. `dirty 0` or it is not
   citable.

---

## 5. Landmines in the physics engine

Things that are true, non-obvious, and have already caused wrong conclusions.

- **Four loss terms are hard-assigned `0.0` on the line *after* the line that computes them:**
  `antennaGainDb` (`lora-physics-engine.cc:340` → zeroed `:341`), `dopplerPenaltyDb`
  (`:536` → `:537`), `multipathPenaltyDb` (`:489` → `:596`), `soilAttenuationDb` (→ `:593`).
  Reading the submodule that computes a term is **not** enough; the assignment that discards it
  is in the caller. In the `probe_v7` configuration 8 of 16 terms in the loss sum are
  structurally zero.
- **Effective sensitivity is not a constant.** It is reduced by `linkHysteresisMarginDb`
  (1.5 dB, `lora-physics-parameters.cc:401`) whenever the link was previously receiving. It sits
  **outside the loss sum** and is invisible to anyone enumerating `additional_loss`. The reported
  margin's denominator therefore moves with the outcome, which mimics a distance-dependent
  effect. This cost one wrong attribution.
- **Terrain hardcodes 868 MHz** (`lora-terrain-submodule.cc:69-70`) and applies that wavelength
  to every tier, UWB at 7.99 GHz included. Terrain does not scale with frequency.
- **Receiver sensitivity is unverified.** A six-decibel indexing error (`sf - 7` against tables
  whose index 0 is SF5) sat on the tier carrying ~95 % of active cycles and **passed every gate,
  verifier check and validation in the project.** It was found by an independent closed-form
  model refusing to reconcile, not by a gate. No check in the project would catch it today.
  Fixed at `0d5e747`; `r80` moved 909.5 → 1149.5 m and every prior result was regenerated.
- **The CRLB gate no longer runs on tier 1 — CLOSED (D-3), verified 2026-09-06.** Tier 1 is
  RSSI inversion, not a time-of-arrival estimator, so a ToA Cramér–Rao bound does not apply and
  applying one is a category error. `lora-ranging-submodule.cc:151` now takes the shadowing
  standard deviation and explicitly discards the ToA inputs (`bandwidthHz`, `snrDbInBand`,
  `spreadingFactor` are `(void)`-cast with the reason stated). **This entry said "open defect"
  until 2026-09-06 while the register said D-3 was done** — the contradiction is recorded in
  `docs/TECH_DEBT.md` §2.5 rather than silently corrected, because a stale line in the
  authoritative rules file is exactly the failure §8 of this document warns about.
- **`LoRaDistributionSampler` ignores its seed** (`lora-distribution-sampler.cc:8-17`): 5 random
  variables created, 0 `SetStream` calls. Recorded as a **decision not to fix**. Determinism
  holds at defaults and at six non-default flag settings, so no result is known to be affected.
- **Two unattributed residuals stay open deliberately:** UWB ~3.7 dB and 868 MHz ~3.5 dB
  near-field. Conclusions are drawn by recomputing under ±4 dB of margin offset chosen to bracket
  exactly these. **Corrected 2026-10-20 (TECH_DEBT R.1, VAL-V8):** both were measured against
  `probe_v7`, an engine configuration that predates `50fc8620`. Against the engine as it is, UWB
  reconciles within 2.12 dB and 868 MHz is unchanged (−4.95 dB at 100 m). The open residual is now
  2.4 GHz: −6.4 dB inside 50 m, crossing to +6.9 dB at 6 km. That is a slope, which no
  distance-independent term explains. The ±4 dB bracket does not cover it.
- **Every engine subtracts an IQ "SNR degradation" from rx power after the loss sum**
  (`lora-iq-impairment-submodule.cc:48`). It is 0.6–6 dB and saturated at ~6 dB on UWB, and it is
  in no loss census. Against a data-sheet sensitivity it double-counts the receiver's impairments.
  `--iqOverloadOnly` removes it (TECH_DEBT R.2).

---

## 6. The invariant that makes the experiment valid

**Every transmission is made inside the per-cycle loop, by a radio that is awake that cycle, and
is charged to the tier that made it. A tier an arm never uses accrues its sleep current and
nothing else: no transmit, receive, CAD or idle energy.** Measured 2026-10-20 (seed 7, 600 s,
four finders): `static_868` bills 0.002376 J on 2.4 GHz and 0.001386 J on UWB, all of it sleep.

The older wording, "exactly one tier transmits per cycle", does not hold, and each exception is
deliberate (TECH_DEBT K.3):
- **The tier-1 floor.** When the active tier is 2.4 GHz or UWB and the floor is wanted, 868 MHz
  ranges in the same cycle and is charged. So tier-1 energy is not proportional to `cycles_868`:
  in `baseline_v8` 868 MHz transmitted on ~96 % of every arm's cycles, whatever the active tier.
- **The dual base** (`--loraDualBase`, in the preset since 2026-10-08) ranges 868 MHz and
  2.4 GHz every cycle.
- **The `poucet_fusion` comparator** ranges all three tiers every cycle, on purpose. It is the
  cost the one-tier rule is compared against.

This was not always true. An earlier pipeline generated all three tiers' data unconditionally and
selected afterwards, under which *the energy saved by not escalating was structurally
unmeasurable*. Fixing it is what made the energy claim testable at all.

**Any change that moves a transmission outside the per-cycle loop, or charges a tier for a
transmission it did not make, breaks the only thing that makes the energy axis mean anything.** Check this first in
any review.

---

## 7. Where things are

```
src/lora-physics-engine/model/
  ca-tbl-policy.{h,cc}        EKF · PCS · evidence gate · integrity gate · escalation FSM
  ca-tbl-mac.{h,cc}           CSMA/CA · duty governor · neighbour table · 9 message types
  ca-tbl-energy-model.{h,cc}  per-action joule accounting from datasheet currents
  ca-tbl-header.{h,cc}        message taxonomy, wire sizes, access categories, ACK policy
  lora-physics-engine.*       868 MHz    | lora-2p4-physics-engine.*  2.4 GHz
  lora-uwb-physics-engine.*   UWB        | submodules/               ~35 per engine
scratch/
  ca_tbl_causal_sim.cc        the scenario: geometry, mobility, arms, sweeps, CLI flags
  ca_tbl_packet_validator.cc  MAC unit suite (one of the unit suites; counts drift, see G1/G9 output)
  python_handoff/verify_causal_sim.py    the invariant gate
scripts/
  catbl_gates.sh              the regression gates (`grep -c '^finish ' scripts/catbl_gates.sh`; 111 PASS at the 2026-10-18 merge)
  publication_ready.sh        manifest assertions; parses EXCLUDED.md as its source of truth
report/
  PCS_REDESIGN_RESULTS.md     working evidence chain, Parts 0-8, with corrections registers
  PHASE10_PREREGISTRATION.md  the registered decision rule and its amendment
docs/
  WORK_PACKAGES.md            what to build next, with entry points  ← start here
  DESIGN_DECISIONS.md         choices the project brief does NOT specify, each with what it
                              was chosen over — finder mobility, the 8 m stand-off, the duty
                              category, the unresolved gateway-count divergence
  TECH_DEBT.md                everything known wrong/unfinished, ordered by risk to results
  RELAYED_RANGING_ERROR_MODEL.md  C5 precondition: why a relayed range is not a range
  CONTEXT_PACK.md             parameter provenance, prior art, citation rules
  archive/                    superseded handoffs, retained for their findings
MASTER_PLAN.md                strategy: built-vs-specified, gaps, tracks, sequencing
Architecture.md               system design as built
Simulation.md                 specifications, methodology, per-seed results, work programme
CA-TBL_Technical_Report_SelfContained.md   claim-by-claim record — AUTHORITATIVE for results
EXCLUDED.md                   what is deliberately not tracked, and why
SHA_MAP.md                    old→new commit SHAs after the attribution rewrite
```

---

## 8. Document precedence, and a staleness warning

When two documents disagree:

1. `CA-TBL_Technical_Report_SelfContained.md` — authoritative for **results and claims**
2. `Architecture.md` — authoritative for **system design as built**
3. `Simulation.md` — specifications and the work programme
4. `report/*.md` — working evidence chains; **date-stamped and frequently superseded**
5. `docs/archive/*` — retained for findings, **not current**

**Working documents go stale and have caused a real error.** `report/PCS_REDESIGN_RESULTS.md`
§7.10 (dated 2026-08-05) states *"S7 environmental fault injection. Not implemented, no flag."*
That is false as of 2026-08-27: `--injectRangeBias`, `--injectFaultAnchors` and
`--injectBiasFraction` exist and 162 results directories use them.

**Before treating any "not implemented" note as current, check two things:** the
`cmd.AddValue` block in `scratch/ca_tbl_causal_sim.cc` (find it with
`grep -n 'cmd.AddValue' scratch/ca_tbl_causal_sim.cc`; line numbers drift with every change), and
`grep -rh "<flag>" results/*/run_id.txt`. The code and the sidecars are the ground truth; the
prose is a snapshot.

**EVERY DOCUMENT DATED BEFORE 2026-09-07 DESCRIBES THE LEGACY TOPOLOGY.** That is the single
largest staleness axis in the repository and it cuts across all five tiers of the precedence
list above. Anything discussing "tier-2 anchors", "tier-3 anchors", "the triads", "anchors per
facility site", "co-located precision tiers" or `--sitingMode` is describing `--topology=1`.

The **current** topology is stated in three places and nowhere else:

1. this file, the topology block at the top;
2. `Architecture.md` §3 — the design as built;
3. `BuildDeployment()` in `scratch/ca_tbl_causal_sim.cc:197` — **the ground truth.**

`report/*.md` and `docs/archive/*` were deliberately **not** rewritten. They are dated records
of what was measured under the deployment that was in force at the time, and editing them to
match a later model would falsify the evidence chain — the same principle as
`run_id.txt` sidecars and `report/REPORT_DATA_PACK.md` in §2. Read them as history.

**Line numbers in `docs/archive/` are stale.** `CA_TBL_PHASE8_SCOPE.md` was written 2026-08-04
and its citations have drifted by ~100 lines in `ca_tbl_causal_sim.cc`. Its *findings* hold; its
line numbers do not. `docs/WORK_PACKAGES.md` carries the current ones. Re-grep before trusting
any line reference older than the last commit that touched the file.

---

## 9. Honest positioning — do not overclaim in code comments or reports

None of the following is novel, and each is cited in `Architecture.md` §13.2: the LoRa + UWB
radio combination (Poucet, IPIN 2021), mesh LoRa localization (LoRaWAPS), threshold-based band
switching, multi-hop uncertainty propagation (DV-Hop family; Huang et al. 2013), Byzantine-robust
localization (SeRLoc, HiRLoc, distance bounding).

The contribution is **the assembly and its quantification**, plus the measurement apparatus.
Where a constant is ours rather than cited, the source comment must say so — that discipline is
what lets a reader separate *measured/cited* from *our choice*, and it is enforced.

**The fault model is environmental, not adversarial**: cold-sagged battery skewing RSSI, antenna
detuning from ice, canopy multipath, sensor drift. Do not introduce adversarial framing
(attacker, spoofing, jamming, Byzantine) outside a citation to that literature.
