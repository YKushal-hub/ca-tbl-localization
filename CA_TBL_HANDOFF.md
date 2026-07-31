# CA-TBL Simulation — Session Handoff

**Paste this entire file as your first message in the new chat.** Attach
`finalyearprojectcontext.md` and `LoRa Localization System.md` alongside it.

Written 2026-07-30 at the end of a session that hit its context limit mid-task.
Everything below is either verified by a run or explicitly marked as unverified.

---

## 0. Your role and the working rules that produced this state

You are continuing work on an ns-3 simulation for a final-year Cyber-Physical
Systems project. The previous session established a set of working rules. They are
not stylistic — each one was adopted after a specific failure, and abandoning them
is how this project previously produced a confident-looking report built on invalid
numbers.

1. **Correctness before progress.** Do not build a new layer on top of a layer that
   fails its own checks. The pre-S2.5 results existed for weeks because nobody
   checked whether the arms could physically function.
2. **Report negative results, do not tune them away.** The user has explicitly
   chosen this. If the proposed system loses to a baseline, write it up and
   diagnose it. Never sweep parameters on the same data used to evaluate them.
3. **Every number traces to a run, a datasheet, a citation, or arithmetic shown in
   the response.** Mark anything unverified as unverified.
4. **Lead with what is wrong.** When reviewing, state defects first.
5. **Distinguish spec from invention.** The design document marks each mechanism as
   borrowed or contributed. That distinction IS the novelty claim. When you write
   code, carry the citation into the comment. When you pick a value the document
   does not specify, say so explicitly in the comment and in your reply.
6. **Build a detector before trusting a result.** Every defect found this session
   was found by a check that was added on purpose, not by inspection.
7. **User preferences:** concise, direct, no filler, no praise openers, bullets and
   tables over prose, no visuals unless asked, code without comments unless the
   file's existing style is heavily commented (the CA-TBL files are — match them).

---

## 1. Environment — read this before running anything

- Repo: `\\wsl.localhost\ubuntu-22.04\home\kushal\ns\ns-3-dev` (ns-3.47, WSL Ubuntu 22.04).
- **The agent sandbox cannot mount that UNC path.** `mcp__workspace__bash` fails with
  `UNC paths are not supported`. You have **no shell**. Read/Write/Edit/Glob/Grep on
  the repo work normally.
- **The user runs all builds and simulations and pastes back the output.** Give exact
  copy-pasteable commands, one block, ordered, and say which failure stops the chain.
- ns-3 builds *every* file in `scratch/`. One broken scratch file blocks the whole
  tree, including unrelated runs. (`diag_tier_linkbudget.cc` had never compiled —
  `VegetationClass` instead of `VegetationType`; fixed this session.)
- Compiler is strict-ish: `-Wall -Wpedantic -std=c++23`, not `-Werror`.

---

## 2. What the project is

A GPS-denied localization system for alpine/forest search-and-rescue. Three radio
tiers, one decision layer.

| Tier | Radio | Role |
|---|---|---|
| 1 | LoRa 868 MHz (SX1262) | wide-area mesh, coarse TDoA/RSSI. Always on. Coverage floor. |
| 2 | LoRa 2.4 GHz (SX1280) | mid-range two-way ToF refinement |
| 3 | UWB (DW3000, Ch9 @ 7.99 GHz) | short-range precision, accuracy ceiling |

A field node runs tier 1 continuously and **escalates** to tier 2 or 3 only when its
position confidence justifies the energy. Anchors are GPS/PPS-disciplined and run the
EKF and the arbitration.

### The locked novelty claim — four parts in combination, none individually new

Every individual mechanism has prior art; the previous sessions verified this
exhaustively. The claim is the **assembly**:

1. Self-deployable mesh topology (not dropped-beacon, not fixed-anchor)
2. Hop-aware confidence degradation feeding the switch decision
3. **A trust/integrity check at the LoRa→UWB switching seam** — the central claim.
   Reframed as *environmental fault tolerance* (cold-sagged battery skewing RSSI,
   antenna detune from ice/damage, canopy multipath), **not** an adversary. "Who
   would hack a hiker's beacon" does not survive review.
4. Identity-scoped, priority-based multi-target arbitration

**Closest prior art you must cite, not hide: Poucet** (French National Defense,
LoRa 868 + LoRa 2.4 + UWB + GPS + altimeter, field-validated for military/firefighter
use). Poucet has the radio combination and the fusion. It does **not** have mesh
multi-hop, hop-degradation confidence, a trust layer on the switching decision, or
identity-scoped multi-target search. The honest framing is: "Poucet proved dual-band
LoRa+UWB works in the field; we extend it to a self-deployable mesh with hop-aware
confidence and switching-point integrity, removing its infrastructure dependency."

Explicitly **not** claimed: LoRa+UWB fusion (Poucet), mesh LoRa localization
(LoRaWAPS), threshold switching (AirTag/Janus/GPS duty-cycling), multi-hop
uncertainty propagation (DV-Hop family, 15+ yrs), Byzantine-robust localization
(SeRLoc/HiRLoc/distance bounding), identity-based access control (standard IoT).

### The six things the simulation must prove

1. Architectural advantage — hybrid beats single-band
2. Procedure advantage — handoff is worth it
3. Handoff triggers correctly
4. Custom handshake
5. Specialized packets
6. Real-life physics applied to packets

---

## 3. Key documents in the repo

| File | What it is |
|---|---|
| `simstatus.md` | Prior study's full report. **Read Part 6** — 16 bugs found and fixed, each of which had produced a confident wrong answer. Part 5.3 has the Invariance Theorem. §4.1 has the CRLB calibration (β_rms = 36,084 Hz) that you will reuse. |
| `CA_TBL_SIM_AUDIT.md` | Claim-by-claim status + S1–S7 build order. Current through S2.5. **Does not yet document S4 or the ranging defect.** |
| `CA_TBL_S1_S2_README.md` | S1/S2/S2.5 detail, including why the first arm run is void. Current through S2.5. **Does not yet document S4.** |
| `finalyearprojectcontext.md` | The specification. §2.1–2.9 = MAC/link layer. §3.1–3.5 = network layer. §1.6 = PCS, §1.7 = integrity check. Also the full novelty-verification thread. |
| `LoRa Localization System.md` | 382 KB. Lines 1–700 are the architecture; the rest is a LoRa textbook/lit review useful for citations only. |

---

## 4. Build order and status

| Step | What | Status |
|---|---|---|
| S1 | Make the tier decision causal | **DONE** |
| S2 | Per-tier energy model | **DONE** |
| S2.5 | Tiered deployment geometry | **DONE**, results blocked |
| S3 | Baseline arm sweep, 20+ seeds, CIs | arms exist, sweep not run |
| S4 | Packets + MAC layer | **DONE and validated 38/38** |
| S5 | Mesh + multi-hop routing | **NOT STARTED** |
| S6 | Multi-target + identity | **NOT STARTED** |
| S7 | Trigger validation + fault injection | **NOT STARTED** |

### Claim status

| # | Claim | Status | Blocker |
|---|---|---|---|
| 1 | Hybrid > single-band | **blocked** | ranging defect (§6) makes tier 2 win non-physically |
| 2 | Handoff worth it | ~45% | needs valid arms + 20 seeds + CIs |
| 3 | Trigger correctness | ~40% | labels exist; needs LOOCV calibration, fault injection; also blocked by UWB PDR dip corrupting `visUwb` |
| 4 | Custom handshake | **substrate done** | needs physics-backed scenario |
| 5 | Specialized packets | **substrate done** | needs physics-backed scenario |
| 6 | Real physics on packets | ~85% | MAC exists with emergent collisions; not yet wired to real engines |

---

## 5. Files written this session

### New — S4 packet/MAC layer
| File | Contents |
|---|---|
| `src/lora-physics-engine/model/ca-tbl-header.{h,cc}` | 9-byte MAC header as ns-3 `Header` (packet_id 4, origin_id 2, hop_limit 1, msg_type 1, tier 1). 7-class/9-code taxonomy. Access categories + CW bounds (§2.3.1). Selective-ACK policy (§2.8). Per-type payload sizing. Semtech ToA. `CaTblCarriesPositionFix()` scopes §3.3 routing to position-fix traffic only. |
| `src/lora-physics-engine/model/ca-tbl-mac.{h,cc}` | `CaTblMac`: CAD-CSMA/CA with priority backoff, WPC duty gate, APC loop, flood dedup (bounded LRU + jitter + hop_limit), explicit-ACK retransmit, §2.9 mobility handshake. `CaTblChannel`: in-flight transmission tracking + capture arbitration → **emergent collisions**. `CaTblPhyProvider` is the interface to the physics engines (not yet implemented against them). |
| `scratch/ca_tbl_packet_validator.cc` | 6 test groups against a deterministic stub PHY, so a failure is a protocol bug not a channel draw. |

Both new sources + headers registered in `src/lora-physics-engine/CMakeLists.txt`.

### Rewritten
- `scratch/ca_tbl_causal_sim.cc` — tiered geometry, facility-tour mobility, ground-truth
  coverage labels, `--probe` pre-flight with monotonicity + CRLB gates.
- `scratch/python_handoff/verify_causal_sim.py` — fix-coverage now a **hard fail at 50%**
  (was a warning at 20%, which is how the void results got through), plus UWB-must-be-most-accurate,
  UWB-must-be-most-expensive, confusion-matrix completeness, static-arm trigger bounds.

### Fixed
- `scratch/diag_tier_linkbudget.cc` — `VegetationClass` → `VegetationType`. Had never compiled; was blocking the whole tree.

---

## 6. THE BLOCKING DEFECT — fix this first

**`src/lora-physics-engine/model/submodules/lora-ranging-submodule.cc`,
function `ComputeRangingErrorM()`.**

Three branches:
- `RANGING_TIER_868` (~L86–113): derives `sigmaM` from `state.propagationLossDb`,
  `propParams.pathLossExponent`, `state.shadowingStdEff`. **Correct** — varies 57× across range.
- ToF branch (~L116–126): `bias + noise` from `params.tofLosNoise` / `tofNlosNoise` / `tofNlosBias`.
- UWB branch (~L129–139): `bias + noise` from `params.uwbLosNoise` / `uwbNlosNoise` / `uwbNlosBias`.

**The ToF and UWB branches contain no SNR, no bandwidth, and no distance term.**
`RANGING_PHYSICS_DERIVED` is a misnomer on two of three tiers.

### Proof it is a constant, not merely suspicious

`src/lora-physics-engine/model/lora-physics-parameters.h:972-977`:
```
tofLosNoise  = Normal(0.0, 0.3,  -1.0, 1.0)
tofNlosNoise = Normal(0.0, 1.2,  -4.0, 4.0)
tofNlosBias  = Normal(1.5, 0.8,   0.0, 6.0)
uwbLosNoise  = Normal(0.0, 0.05, -0.2, 0.2)   // cited: Benouakta 10-33 cm, Electronics 2023
uwbNlosNoise = Normal(0.0, 0.25, -1.0, 1.0)
uwbNlosBias  = LogNormal(log(0.3), 0.6, 0.0, 3.0)
```
For a normal truncated at ±3σ, E|X| ≈ 0.8σ:
- 2.4 GHz: 0.8 × 0.30 = **0.24 m** — measured 0.245 / 0.242 / 0.237, identical at every distance
- UWB: 0.8 × 0.05 = **0.04 m** — measured 0.040 / 0.041 / 0.037

The reported "range error" is the configured σ read back out.

### Why it blocks claim 1

Tier 2 gets 0.24 m ranging at 900 m and 16.9 dB SNR, so it strictly dominates:
`static_2p4` acc/J = 3.73e-2 vs `ca_tbl_full` 2.71e-2. **Claim 1 appears to fail for a
non-physical reason.** This is an artifact and must not be reported as a finding.

### The agreed fix (user decision: "Fix the submodule properly")

```
σ = max( CRLB(BW, SNR_post_integration) , σ_hw_floor )
```
- CRLB: `σ_t = 1/(2π·β_rms·√(2·SNR))`, `β_rms = BW/√12`, `σ_range = c·σ_t`, `/√2` for symmetric two-way.
  Sanity: BW = 125 kHz → β_rms = 36,084 Hz, which reproduces `simstatus.md` §4.1 exactly. Use that as your check that the formula is wired right.
- Keep the cited σ values as **hardware floors**, which is what they should always have been.
  Benouakta 10–33 cm becomes UWB's floor and the citation survives intact.
- **UWB needs post-integration SNR**, not raw wideband SNR. UWB raw SNR is *negative*
  (−17.4 dB at 2 m) because the noise floor over 499.2 MHz is ≈ −87 dBm while the
  sensitivity gate is −104 dBm — decoding below the wideband noise floor is what a
  correlator receiver is for. DW3000-class preamble accumulation (~1024 symbols ≈ 30 dB)
  brings the floor to ≈ 0.039 m, which is why 0.040 m looked plausible.
- LoRa `snr_db` is already post-despread, so do **not** add processing gain again on tier 2.

Requires adding `bandwidthHz` and an SNR argument to `ComputeRangingErrorM()` and
updating the three engine call sites (868 / 2p4 / UWB engines).

**Predicted effect (unverified):** UWB roughly unchanged; 2.4 GHz goes from flat 0.24 m
to ~1 m close-in and ~5 m at 900 m. That is what removes tier 2's illegitimate dominance.
The SX1280 datasheet claims 1–2 m with heavy averaging, so ~1 m close-in is the target.

**Blast radius, accepted by the user:** invalidates banked results that used tier-2/tier-3
ranging error — parts of the 5M-row dataset and some of the 246 artifacts in `results/`.
Regenerate rather than reconcile.

---

## 7. SECOND BLOCKER — UWB PDR non-monotonicity (investigate in parallel)

At n=2000 trials: PDR **0.099 at 30 m** rising to **0.385 at 50 m** (+0.286 ≈ 26σ; binomial
SE at n=2000 is 0.011). Not noise.

Why it blocks claim 3: `visUwb` — the geometric ground-truth label the trigger's
precision/recall/F1 is scored against — assumes UWB coverage is a monotone function of
distance. A dip at 30 m means the labels are wrong at specific distances.

**Prime suspect:** two-ray ground null. The probe uses gateway z = 6.0 m and node z = 1.5 m.
`LinkPhysicsState` has an `isTwoRay` flag (`lora-physics-parameters.h:24`) — log it per
distance point in `RunProbe()` and confirm or eliminate before looking anywhere else.
Second suspect: a distance-bucketed lookup boundary at 30 m.

Note the deployment uses UWB anchor height 2.5 m, not the probe's 6.0 m, so the null
distance differs between probe and arms. That inconsistency is worth fixing regardless.

---

## 8. Verified results — safe to quote

### S4 validator: 38/38 pass
```
./ns3 run "ca_tbl_packet_validator --outdir=results/s4"
```
| Result | Value |
|---|---|
| ToA cross-check (two independent Semtech implementations) | agree to < 1e-9 s |
| ToA SF12/125 kHz/50 B | **2.302 s** — inside the 1.5–2.5 s band the design doc uses to justify the digest mechanism |
| Digest vs full push, 8 peers, SF12 | **2.138 s vs 8.364 s** — the "here's the airtime it saves" number |
| Priority CW separation | URGENT **1.89 ms** vs LOW **15.96 ms** mean backoff per deferral = **8.4×** |
| Emergent collisions | 284 tx → 1129 collision events, 859 deliveries, **0 captures** |
| Duty gate, field node | 2.413% against 2.5% cap, 315 frames refused |
| Duty gate, gateway | 9.707% against 10% cap, 179 frames refused |
| Flood | 4 of 8 nodes reached at hop_limit=3; dedup fired; terminated |
| §2.9 handshake | approaching = DISCOVERY+DIGEST; receding = TRIGGERED_UPDATE, no DIGEST |

Caveats to carry forward:
- **0 captures is correct but means the capture path is untested.** The stub gives every
  audible link identical power, so nothing can clear the 6 dB margin. Needs real engines.
- Only `CW_urgent = 0-3` comes from the spec. HIGH/NORMAL/LOW (0-7/0-15/0-31) are the
  802.11e doubling pattern, chosen by the previous session. Uncalibrated, unswept. Same for
  `slotTimeS` and the 6 dB capture threshold — collision rate is sensitive to the last one.
- The flood test is a line topology, so it never tests the broadcast-storm case that
  jitter exists to prevent. Needs a dense topology.

### Probe, n=2000, after the azimuth fix
| tier | r80 | r50 | range err | notes |
|---|---|---|---|---|
| 868 MHz | 1000 m | 2000 m | 52 m @100 m → 2973 m @6 km | varies correctly; CRLB ratio 2.817 = OK |
| 2.4 GHz | 900 m | 900 m | flat 0.24 m | never fell off within sweep; CRLB violated 62.6× |
| UWB | 5–10 m (unstable) | 30 m | flat 0.04 m | monotonicity violation; CRLB check unreliable (see §6) |

**868 range error is ~50% of true distance at every range.** Tier-1 fixes come from
multilateration geometry across 9 gateways, not from link ranging. State this before
quoting any tier-1 RMSE.

### Derived deployment result worth reporting
**~9 gateways per 4 km² is the density needed for reliable 3-gateway coverage at SF7 in
forest**, derived from measured 868 r80 = 1000 m and a 3×3 grid at 900 m pitch (worst-case
point 640 m from its three nearest, PDR ≈ 0.95). This is `simstatus.md` §8.5's "gateway
density dominates" conclusion, now grounded in measurement rather than assumption.
`--boxHalf` sweeps the curve.

---

## 9. VOID — do not quote

`results/causal/arm_summary.csv` and all `results/causal/trace_*.csv`.

Two generations of invalid results:

**Gen 1 (pre-S2.5).** One 100 m anchor triangle shared by all three tiers, node
random-walking a 1.2 km box. `static_uwb` had `noFixCycles = 600/600`, was therefore never
charged for a ranging exchange, and came out the **cheapest** arm — inverting the energy
axis. 497–600 of 600 cycles produced no fix in five of six arms.

**Gen 2 (post-S2.5, this session).** Geometry fixed and fix coverage improved, but blocked
by the ranging defect. For reference only:
```
arm                   rmse_m   cep50_m  energy_J  nofix/3600  acc/J
static_868            303.781  243.141    30.44         2     1.08e-4
static_2p4              0.653    0.140    41.07         0     3.73e-2   <-- artifact
static_uwb            114.002    7.458    70.84      3240     1.24e-4
fixed_threshold        28.343    0.242    63.31      2466     5.57e-4
ca_tbl_no_integrity    11.633    0.235    66.57      2682     1.29e-3
ca_tbl_full             0.555    0.227    66.42      2647     2.71e-2
```
Archive these before regenerating:
```bash
mkdir -p results/causal_VOID_gen2 && mv results/causal/arm_summary.csv results/causal/trace_*.csv results/causal_VOID_gen2/ 2>/dev/null
```

---

## 10. Other outstanding findings

1. **`dt = 1.0 s` at SF7 gives 5.66% duty on 868 — illegal.** WPC G.S.R. 853(E) Table-II caps
   field nodes at 2.5%. Arithmetic: ~0.056 s ToA per 1 s cycle. **Minimum compliant cycle is
   ~2.26 s.** Every arm currently runs an illegal tier-1 duty cycle. Either raise `dt` or route
   the ranging exchange through the MAC duty gate. Either way it is a constraint the
   architecture must state, and it caps the achievable fix rate.
2. **The probe's own CRLB check omits coherent processing gain**, so its UWB verdict (156.8×) is
   overstated. Fix the check when you fix the model.
3. **`hopWeight` is 0** in `CaTblPolicyConfig` — PCS has a hop-provenance term wired but disabled
   because the scenario is single-hop. **Do not claim hop-provenance confidence degradation until S5.**
4. **Integrity gate requires `n_los_frac == 1.0`** (all links LOS). Fired 3 times in 3600 cycles
   in gen-2, 0 times in gen-1. The central novelty claim is barely exercised. Sweep the LOS
   cut-off in S7.
5. **Thresholds `1.55 / 1.70 / 2.30` are hand-set**, carried verbatim from the Python reference.
   LOOCV calibration is promised in the design doc and not implemented. Any result sensitive to
   these is not yet a result.
6. **Trigger ground truth is geometric, not error-based.** `shouldUwb := visUwb >= 3` asks "could
   UWB have solved here", which is necessary but not sufficient for "escalating was right" —
   escalating into coverage the node is about to leave scores as correct. Report `trig_f1` as a
   coverage-tracking score, not decision optimality. Error-based label belongs in S7.
7. **Case A/B/C resolution** (2 anchors / 1 anchor + 1 peer / 0 anchors) from the design is not
   implemented; below 3 ranges the filter coasts.
8. **MCU energy is not modelled**, only radio. Regulator efficiency and PA ramp excluded.
9. **SF12 is unusable in this simulator.** `results/metric2_pdr_distance_seed7.csv` shows SF12 PDR
   0.255–0.355 at 500 m vs SF7's 1.00 — inverted from physics, almost certainly collision-dominated
   (~30× airtime). Means the genuinely long-range tier-1 case cannot currently be simulated.
   Worth a separate investigation.
10. **`--vegFacility` / `--envFacility` sensitivity is unswept.** The previous session changed both
    in the direction that helps UWB (facility sites are clearings: `ENV_RURAL_OPEN` +
    `VEG_SPARSE_SHRUBS`, vs `ENV_RURAL_FORESTED` + `VEG_DENSE_FOREST` for tier 1). The justification
    is siting and holds independently of outcome, **but it must be swept.** If the architectural
    conclusion only holds at the favourable settings, that dependence is the finding.

---

## 11. Bugs the previous session introduced and fixed — do not reintroduce

1. **Duty-cycle denominator.** Normalised trailing airtime by *elapsed* time instead of the
   observation window, making the first frame of any run ~100% duty and refusing legitimate startup
   traffic. Correct reading: an airtime budget per observation period (0.025 × 3600 = 90 s/hour for a
   field node). A regression guard is now in `TestDutyCycle()`.
2. **Probe held one position for all trials at each distance point.** The engine's shadowing is
   Gudmundson spatially-correlated, so 400 trials shared one shadowing draw; only fast fading and the
   interferer coin re-rolled. Produced PDR curves that *rose* with distance on all three tiers. Fixed
   by rotating azimuth per trial. This is `simstatus.md` Bug #1 reappearing in the probe rather than
   the engine.
3. **400 trials was too few to size geometry from.** UWB r80 moved 10 m → 5 m between two runs of the
   same seed. Now 2000.
4. **A retransmission needs a fresh `packet_id`** or every receiver's dedup cache discards it as
   already-seen. Interaction between §2.6 and §2.8; handled in `CaTblMac::OnAckTimeout`.

---

## 12. Recommended order for the new session

1. Read `CA_TBL_SIM_AUDIT.md`, `CA_TBL_S1_S2_README.md`, and `simstatus.md` Part 6.
2. Read `lora-ranging-submodule.cc` and `lora-physics-parameters.h:957-979`.
3. **Fix `ComputeRangingErrorM()`** per §6. Add `bandwidthHz` + SNR params, CRLB-derive both
   branches with the cited σ as floors, update the three engine call sites.
4. **In parallel, instrument `isTwoRay` in `RunProbe()`** to resolve §7. Align probe antenna heights
   with the deployment's (2.5 m UWB, 6.0 m tier-2) while you are there.
5. Fix the probe's CRLB check to account for coherent processing gain on UWB.
6. Re-run the probe. Require: monotonicity OK on all three tiers, CRLB OK on all three, and
   2.4 GHz range error that *varies* with SNR.
7. Re-derive geometry from the new r80 values. Raise `dt` to ≥ 2.26 s for tier-1 duty compliance.
8. Re-run the arms + verifier. Require verifier exit 0.
9. **Then** read the arm table. If `ca_tbl_full` still loses to a static arm, report and diagnose —
   check `cyclesInUwbCoverage` is in 10–30%, whether escalation handshake cost dominates the gain,
   and whether the PCS thresholds sit anywhere near the PCS distribution the scenario produces
   (histogram the `pcs` column in the traces).
10. Then S3 (20-seed sweep with CIs), then wire `CaTblPhyProvider` to the three real engines to close
    claims 4/5/6, then S5/S6/S7.

---

## 13. Command reference

```bash
cd ~/ns/ns-3-dev
./ns3 build 2>&1 | tail -40

# S4 MAC unit tests — deterministic, ~1 s. Run first; if the protocol fails its
# own spec there is no point running physics on top of it.
mkdir -p results/s4
./ns3 run "ca_tbl_packet_validator --outdir=results/s4" 2>&1 | tail -80

# Geometry + physics pre-flight. Seconds, not minutes.
mkdir -p results/causal
./ns3 run "ca_tbl_causal_sim --probe=true --outdir=results/causal" 2>&1 | tail -70

# Arms. 3600 s simulated ran in under 2 minutes wall-clock.
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal" 2>&1 | tail -30
python3 scratch/python_handoff/verify_causal_sim.py results/causal

# Seed sweep, once invariants pass
./ns3 run "ca_tbl_causal_sim --seedStart=1 --seedCount=20 --duration=3600 --outdir=results/causal --trace=false"
```

`ca_tbl_causal_sim` knobs: `--env --envFacility --vegWide --vegFacility --boxHalf --uwbTriad
--t2Triad --uwbAnchors --t2Anchors --speed --dwell --dt --duration --seed --seedStart
--seedCount --trace --probe --outdir`

Current defaults, all derived from the probe rather than chosen: `boxHalf=1000`,
`uwbTriad=10`, `t2Triad=150`, `uwbAnchors=5`, `t2Anchors=3`, `env=1` (RURAL_FORESTED),
`envFacility=0` (RURAL_OPEN), `vegWide=3` (DENSE_FOREST), `vegFacility=1` (SPARSE_SHRUBS),
`duration=3600`, `dt=1.0` (**illegal — see §10.1**).

---

## 14. First message to send

> Read `CA_TBL_HANDOFF.md`, `CA_TBL_SIM_AUDIT.md`, `CA_TBL_S1_S2_README.md`, and
> `simstatus.md` Part 6. Then read `src/lora-physics-engine/model/submodules/lora-ranging-submodule.cc`
> and `lora-physics-parameters.h:957-979`.
>
> Two agreed tasks, in parallel:
> (a) Fix `ComputeRangingErrorM()` so the ToF and UWB branches are CRLB-derived with the
>     cited sigmas demoted to hardware floors. Add `bandwidthHz` and SNR params; update all
>     three engine call sites. Use post-integration SNR for UWB, raw post-despread SNR for 2.4 GHz.
> (b) Instrument `state.isTwoRay` in `RunProbe()` to diagnose the UWB PDR dip at 30 m, and
>     align the probe's antenna heights with the deployment's.
>
> I have no shell — give me exact commands and I will run them and paste the output.
> Report negative results rather than tuning them away.
