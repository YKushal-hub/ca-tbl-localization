# CA-TBL — working rules for this repository

Read this before touching anything. It is short because everything in it is load-bearing.

**What this is.** A fork of ns-3.47 adding a three-tier radio localisation simulation for
GPS-denied search and rescue: 868 MHz LoRa (tier 1), 2.4 GHz (tier 2), UWB (tier 3), with a
decision layer that escalates between tiers on a position-confidence score. Upstream ns-3 is
unmodified except where noted.

**The system is called CA-TBL** — Confidence-Aware Tier-Based Localization. Not CA-DBL. The
thesis registers still say CA-DBL; that is a known divergence being corrected on the thesis
side, not here. Do not rename anything in this repo.

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
- **Do not set `hopWeight > 0`** (`ca-tbl-policy.cc:83`) until relayed measurements actually
  exist. The poll set is one-hop by construction, so `hopCount` is structurally always 0.
  A non-zero weight on it changes nothing; applied to an admitted-but-unreachable peer it
  degrades the score for an **energy** defect rather than a **provenance** one — a wrong result
  that looks like a working mechanism.
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
  applies. Gate **G12** enforces it and it is not advisory: it inspects commit
  identity fields AND message bodies, and it was added by `fbf2153` after the
  2026-09-01 rewrite. Note that G12 also matches PROSE about attribution, so a
  commit message explaining the policy trips it — write about it in a tracked
  document instead of in a commit subject. The tooling references that already
  exist — the tool settings directory in `.gitignore` and `EXCLUDED.md`, `RULES.md` itself, and
  `docs/KICKOFF.md` — are project infrastructure, not attribution, and stay.
- **Do not add a new mechanism without a default-off flag** whose off state reproduces the
  existing baseline byte-for-byte. See §4.

---

## 3. Build, run, verify

```bash
# --build-profile=optimized is REQUIRED, not a preference. Every published number was
# produced under it, and the default profile enables logging: ~10x slower on 3600 s runs.
./ns3 configure --enable-examples --build-profile=optimized && ./ns3 build

bash scripts/catbl_gates.sh          # G1-G11; runs ALL gates and reports a summary
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal"
python3 scratch/python_handoff/verify_causal_sim.py results/causal   # 141 checks / 161 gates
```

**G3 currently fails by design.** The integrity check fires once at seed 7, so byte-identity
between the ablation arms is no longer available. The divergence begins at the firing cycle and
not before — that is the treatment effect, not RNG contamination. A red G3 is not a broken build.

**G11b is permanently red.** Terrain is transcription-only and was not rescoped.

Fresh-clone reproduction of `results/baseline_v7/arm_summary.csv` is byte-for-byte **using the
flags recorded in that baseline's own sidecar**. Default flags do *not* reproduce it — that is a
flag-set difference, not a reproduction failure.

---

## 4. Adding a mechanism — the required shape

Every new mechanism follows this or it is not mergeable.

1. **Default-off CLI flag**, registered with `cmd.AddValue` in the block at
   `scratch/ca_tbl_causal_sim.cc:3178-3450`. Write the help text as an argument, not a label —
   the existing flags explain *why* the default is what it is, and that is the house style.
2. **Null-control test:** with the flag off, output is byte-identical to the pre-change binary.
   Diff `arm_summary.csv` and every trace. This is not optional — arms consume the shared RNG
   stream at their own rate (`macRng`, seeded at `:1932`), so any added transmission perturbs
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
- **The CRLB gate should not run on tier 1** (open defect). Tier 1 is RSSI inversion, not a
  time-of-arrival estimator; `lora-ranging-submodule.cc:151` says so in its own comment and calls
  applying a ToA Cramér–Rao bound to it a category error.
- **`LoRaDistributionSampler` ignores its seed** (`lora-distribution-sampler.cc:8-17`): 5 random
  variables created, 0 `SetStream` calls. Recorded as a **decision not to fix**. Determinism
  holds at defaults and at six non-default flag settings, so no result is known to be affected.
- **Two unattributed residuals stay open deliberately:** UWB ~3.7 dB and 868 MHz ~3.5 dB
  near-field. Conclusions are drawn by recomputing under ±4 dB of margin offset chosen to bracket
  exactly these.

---

## 6. The invariant that makes the experiment valid

**Exactly one tier transmits per cycle, and only that tier is charged.** An arm that never
escalates accrues exactly zero energy on the tiers it never used.

This was not always true. An earlier pipeline generated all three tiers' data unconditionally and
selected afterwards, under which *the energy saved by not escalating was structurally
unmeasurable*. Fixing it is what made the energy claim testable at all.

**Any change that moves a transmission outside the per-cycle loop, or charges a tier that did not
transmit, breaks the only thing that makes the energy axis mean anything.** Check this first in
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
  ca_tbl_packet_validator.cc  MAC unit suite (38/38)
  python_handoff/verify_causal_sim.py    the invariant gate
scripts/
  catbl_gates.sh              G1-G11 regression gates
  publication_ready.sh        manifest assertions; parses EXCLUDED.md as its source of truth
report/
  PCS_REDESIGN_RESULTS.md     working evidence chain, Parts 0-8, with corrections registers
  PHASE10_PREREGISTRATION.md  the registered decision rule and its amendment
docs/
  WORK_PACKAGES.md            what to build next, with entry points  ← start here
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
`cmd.AddValue` block at `scratch/ca_tbl_causal_sim.cc:3178-3450`, and
`grep -rh "<flag>" results/*/run_id.txt`. The code and the sidecars are the ground truth; the
prose is a snapshot.

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
