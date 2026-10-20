# TECH_DEBT status — every entry, classified (2026-10-18)

Built by reading all of `docs/TECH_DEBT.md` on the merged tree (`integrate/2026-10-18-kushaal-kn`). Each open item gets
an id (T1, T2, …) so it can be struck off here when closed. Struck items keep their line, with the commit that closed
them.

**Classes:**
- **ENG:** engineering I can do. Each needs a default-off flag or a doc/gate change, plus a null control.
- **CAMP:** needs a registered campaign to decide.
- **OWNER:** needs a decision only the owner can make.
- **LIMIT:** a measured limit or a research question; no change is known to fix it.
- **HW:** needs hardware.

## Open

| id | entry | what is open | class |
|---|---|---|---|
| T1 | ~~K.3~~ | ~~RULES.md §6 still states "exactly one tier transmits per cycle"; the floor, the dual base (preset) and `poucet_fusion` all transmit two tiers in a cycle. Restate the invariant that actually holds~~ **DONE** RULES.md §6 restated (2026-10-18). | ENG (doc) |
| T2 | ~~L.2 note~~ | ~~The casualty's table entry carries the previous cycle's estimate (published before `Step`): every entry age is understated by one cycle (2.5 s)~~ **DONE (code)** `--publishAfterStep`, gate G156 (927/927 entries fresh). Preset adoption: owner (O1). | ENG |
| T3 | ~~M.9~~ | ~~The RECEDING eager push is not counted per subject~~ **CLOSED (moot)** subjects never call StartHandshake, so no subject push exists; G75's count is complete. | ENG |
| T4 | ~~M.11 / RULES.md~~ | ~~Stale counts in RULES.md §7 ("58 gates at c79fffd", `cmd.AddValue` line ranges, the unit-suite count)~~ **DONE** RULES.md counts replaced by the commands that produce them. | ENG (doc) |
| T5 | ~~I.4~~ | ~~`--bidirectionalDigest`, `--finderPseudoAnchor`, `--corroborationCheck` silently need `--sendHandshakeFrames`; their help text does not say so~~ **DONE** help text of --bidirectionalDigest and --finderPseudoAnchor states the dependency (verified by run); --corroborationCheck already did. | ENG (doc) |
| T6 | ~~H.3 / I.5~~ | ~~No gate asserts that a mechanism's input is non-zero whenever its firing counter is~~ **DONE** gate G157: all 12 adopted preset settings change the output; inert fixture called INERT. | ENG (gate) |
| T7 | ~~N.29~~ | ~~M-01's SF12 "inversion" is the validation harness colliding with itself (0.1 s spacing)~~ **DONE** harness spaces frames at 2× time on air; SF12 500 m PDR 0.015 → 1.000 (seed 7). | ENG |
| T8 | ~~4.1 / 4.2~~ | ~~The 1-ulp trace divergence is undiagnosed; `-march=native` makes byte identity machine-dependent. Suggested test: rebuild with `-march=x86-64`~~ **CLOSED (diagnosed)** native vs portable build: summaries identical at defaults, last-digit trace columns only (≤1.7e-12); under the preset only the MLAT comparator flips (5–20 %); see 4.1 note. | ENG (diagnosis) |
| T9 | ~~6~~ | ~~`src/lorawan` link fix never upstreamed; the optimized profile is not enforced~~ **DONE** G1 refuses a non-optimized configuration (fixture: debug cache); the lorawan fix stays in-tree (upstreaming is external). | ENG (minor) |
| T10 | 2.1 | Four loss terms (antenna gain, Doppler, multipath, soil) are computed then zeroed: wire them in or delete them | OWNER → CAMP |
| T11 | ~~2.2~~ | ~~Terrain uses 868 MHz for every tier; `--terrainPerTierFreq` exists, default off, not in the preset~~ **CLOSED** the preset already sets `--terrainPerTierFreq=true` (kSpecProfile, physics fix D-4); only the code default is off. | CAMP |
| T12 | ~~N.41~~ | ~~Foliage scaling on 2.4 GHz/UWB is capped by the sampler clamp (×1.8 instead of ×2.17/×5.28)~~ **DONE (code + DEBT-A)** G154; improves accuracy; adoption O5. | CAMP (engine fix + campaign) |
| T13 | N.27 / G93 | Tier 2's sensitivity table is the 1625 kHz row at 800 kHz (~6 dB pessimistic); `--t2SensitivityRowKHz` measured (D8-MAIN), not default | OWNER |
| T14 | N.28 | DC-1's gyro bias is the bias instability: every preset accuracy figure assumes a near-perfect gyro. The EKF5 remedy is on another author's branches (not merged) | OWNER (merge EKF5?) → CAMP |
| T15 | ~~1.3 / N.30(main) / G92~~ | ~~15 cited `results/` paths are not in the repo, 3 of them cited by the Technical Report~~ **DONE** G92 PASSES: 13 lost paths in RESULTS_NONREGENERABLE.tsv with reasons and successors, 3 allow-listed in EXCLUDED.md; citing documents note it. | ENG (re-point or exclude) |
| T16 | ~~I.6 / G6~~ | ~~G6 red while pre-D1 evidence is cited; the remedy is regenerating to new paths and moving citations~~ **DONE** results/causal_v12 + det_a_v12 (clean, e93f35c0) and G6 repointed; a fresh run matches. | CAMP |
| T17 | N.15 / G24 | G24 FAIL BY FINDING: mechanisms with no population under the preset -- *2026-10-18:* two zeros now exempt as structural (O.15); 3 genuine zeros remain for O2. | OWNER (accept) or ENG |
| T18 | ~~J.1 / 4.4 / G11~~ | ~~The analytic model and engines disagree (the 2.4 GHz gap grows with distance). Must not be tuned; research~~ **CLOSED (diagnosed)** path loss agrees; the gap is the engine's outcome-dependent effective sensitivity (−107.2 → −99.8 dBm, soft-degradation penalty) + engine-only terrain (0 → 4.8 dB). G11 stays red by design; a fair criterion is an owner change (O3). *2026-10-20, attribution corrected (TECH_DEBT R.10):* the −107.2 → −99.8 dBm "effective sensitivity" is a residual that absorbs the IQ degradation (0.6 → 6 dB, subtracted after the loss sum) and link hysteresis; the soft-degradation penalty runs only under SDR fidelity and is inactive here. G11 is re-pointed to probe_v8 (R.1). | LIMIT |
| T19 | ~~2.3~~ | ~~UWB ~3.7 dB and 868 MHz ~3.5 dB near-field residuals unexplained~~ **SUPERSEDED 2026-10-20 (R.1, R.7, R.10)** measured against a pre-09-08 engine; against today's, UWB reconciles within 2.12 dB, and 2.4 GHz's gap is foliage density, terrain and the IQ degradation. The soft-degradation penalty named earlier is inactive under ULTRA_FAST. | LIMIT |
| T20 | N.40 | Tier-2 ToF floor 3.0 m by default; every tier-2 accuracy claim is conditional on it (Wolf/Müller presets exist) | OWNER (preset ToF model) |
| T21 | ~~N.15 tail / N.32~~ | ~~The integrity check detects almost nothing at χ² 1992.9 (recall ≤ 0.03); re-fit after a consistent covariance~~ **CLOSED (measured limit)** INTEG2: fit 9303.8 (not lower); recall 0.046 → 0.012; the fault-free tail (p99 21 322 at 5 km) blinds any threshold. | CAMP |
| T22 | O.12 / N.51 | 5 km consistency 0.69: residual 868 MHz-fix overconfidence -- *recorded limit; no change known to fix it (see its TECH_DEBT entry).* | LIMIT |
| T23 | O.13 / O.10 | Under calibrated Wolf the hybrid costs ~10× 2.4 alone (UWB escalation); `--rssiCorrInflate` fixes accuracy, not adopted (missed by 0.3 m) | OWNER |
| T24 | ~~O.9~~ | ~~RSSI ranging's path-loss model does not follow the foliage model~~ **CLOSED (diagnosed)** hypothesis refuted: the inversion has no foliage term; P.833 admits 11 % more links at unchanged per-range error; F3 was within noise. Exposed T46. | ENG (diagnose) |
| T25 | O.8 | P.833 depth is drawn, not path geometry; one forest only -- *recorded limit; no change known to fix it (see its TECH_DEBT entry).* | LIMIT |
| T26 | ~~N.35–N.37~~ | ~~5 km within-25 m loss of the hybrid's subjects against static 2.4 GHz unexplained; H2 TDoA form untested~~ **CLOSED (measured negative)** H2-TDoA falsified (subjects' TDoA off changes nothing); cause remains an open finding. | CAMP |
| T27 | ~~N.52~~ | ~~The preset keys R on NLOS truth (`--nlosSource=propagation`); `--nlosRSource=detector` exists~~ **MEASURED (DEBT-A)** detector costs nothing; adoption O4. | CAMP |
| T28 | N.50 | `--unknownStart` is not the code default (all recent campaigns pass it) | OWNER |
| T29 | ~~M.1 open~~ | ~~The interference registry never attenuates an interferer by distance; relay rounds sit in the MAC-run slice~~ **DONE (code + DEBT-A)** G153; endgame 5.0 → 4.1 m; adoption O6. | CAMP (engine) |
| T30 | M.2 / L.2.2 | The UWB CIR confusion matrix cannot fill under the channel model; needs an obstruction model of our own | OWNER |
| T31 | N.1 / N.20 | `--silenceStopsRelay`, `--deadBatteryNoDraw` built, default off, not in the preset | OWNER |
| T32 | N.3 | Two copies of the per-node cycle core (tag 0 and subjects); per-subject SHS not built | ENG (refactor, large) |
| T33 | ~~M.10~~ | ~~C6's 20-seed edge-server energy figure not re-measured under the fix-tier downlink bill~~ **CLOSED** C6 re-measured: edge saves 26 % (868 alone), 13–15 % (hybrid). | CAMP |
| T34 | N.8 / N.13 | Explicit DENY frames cost airtime (design choice); Case C shapes coarse and rarely reach the base station | OWNER |
| T35 | N.11 | Converting RSSI variability to a ToF-tier range sigma is OURS and open (flag out of preset) -- *recorded limit; no change known to fix it (see its TECH_DEBT entry).* | LIMIT |
| T36 | N.42 | Lim, Hofer, Robyns values still lack a source -- *2026-10-18:* Hofer's urban branch sourced (TR 38.901 UMa, verified); Lim/Robyns stay OURS with 3GPP comparison and a CFO finding (likely 10-100x too small). | LIMIT |
| T37 | H.4 / B | Constants ours and uncited (finder GNSS sigma, corroboration k=3/30 m/0.5) -- *recorded limit; no change known to fix it (see its TECH_DEBT entry).* | LIMIT (record) |
| T38 | K.9 | SHS gait calibration diverges where fixes are sparse (debt by owner decision) | OWNER |
| T39 | ~~M.3~~ | ~~`--stampBinaryHash`/`--stampBuildCommit` exist, not in the preset~~ **DONE** both stamps in the preset; NC10 identical. | ENG (preset entry + NC) |
| T40 | 5.1 | 137 published commits carry the wrong author email; fixing needs a force-push to protected `main` | OWNER |
| T41 | ~~5.3~~ | ~~No gate catches stale documents~~ **DONE (the checkable part)** G158: documents name only real flags; RULES.md preset list ⊆ kSpecProfile; fake-flag fixture. | ENG (gate, partial) |
| T42 | ~~L.3 claim 3~~ | ~~Identity-scoped arbitration has never been measured as a claim~~ **MEASURED** arbitration V2: −18 % energy when off, no accuracy loss; claim 3 unsupported on these metrics; O8. | CAMP |
| T43 | C.5 | Self-deployable mesh is not built (fixed gateways plus finders) | OWNER (scope) |
| T44 | 3.5 | Anchors are hidden terminals to each other (beacon loss is an upper bound) -- *recorded limit; no change known to fix it (see its TECH_DEBT entry).* | LIMIT (stated) |
| T45 | E1 | Same-site field test | HW |
| T46 | ~~O.9 diagnosis~~ | ~~The simulated 868 MHz RSSI range omits foliage and fading from the 'measured' RSSI (error = shadowing only, ~7 dB, vs 9–17 dB realised margin spread)~~ **DONE (code + DEBT-A)** G155; 868 alone worse as expected; adoption O7. | CAMP |

## Closed or deliberate (no action)

**Closed in code, in the preset, or by result:**

| | entries |
|---|---|
| §1 | 1.1 (`--rngKeyed`, preset), 1.2 (rule 8) |
| §2 | 2.4 (G93 checks values), 2.5 |
| §3 | 3.1 (relayed ranging in preset; residual is T29), 3.3 (→ N.3) |
| §4–§5 | 4.3 (G12 green), 5.2 (sidecars standard) |
| A–E | A, B, C.2–C.4, D, E |
| H–I | H.1 (UWB fixes in the endgame), H.2 (V7), H.5, I.1 (population exists; detection is T21), I.2, I.3 |
| J–K | J.2, K.1, K.2, K.4, K.6 (DD 16), K.7, K.8 (claim withdrawn) |
| L–M | L.1.*, L.2.1, L.2.3–L.2.5, M.4–M.8, M.10 decision, M.11 (except T4) |
| N | N.2, N.4, N.5, N.6, N.7 (moot), N.9, N.10, N.12, N.14, N.16, N.17, N.18, N.19, N.21–N.26, N.30(acc) (DD 15), N.31(main), N.31(acc), N.53 (merge chose acc-part-b), N.54, N.55, N.56 |
| O | O.1–O.5, O.7, O.10, O.11, O.14 |

**Deliberate, do not fix (TECH_DEBT §7, RULES.md §2):**
- the PCS saturation defect;
- `hopWeight` = 0 (3.2);
- `REPORT_DATA_PACK` citations;
- sidecars are evidence;
- A8 closed by result;
- the unseeded sampler at code defaults.

## Owner decisions collected while striking
| id | decision | evidence |
|---|---|---|
| O1 | adopt `--publishAfterStep` into the preset? (T2) | G156; correctness only, entries 2.5 s fresher |
| O2 | accept G24's three zero populations (Case A fixes, arbitration pre-emptions, grant tie-break verdicts)? (T17) | O.15; they follow from N.26's fused sets |
|    | *2026-10-20 (R.4, ANCHOR-SUPPLY):* pre-emptions HAVE a population (17 in 80 endgame cells), so G24's zero is its sample size; Case A is reachable but rare in CA-TBL (1 in 41 half-hour runs); tie-break verdicts are 0 in 160 cells, not shown reachable | R.4 |
| O3 | change G11's criterion to the pre-penalty engine margin and give the model the engine's terrain mean? (T18) | J.1 diagnosis; scripts/g11_term_diagnosis.py |
| O4 | adopt `--nlosRSource=detector` (no NLOS truth in the filter)? (T27) | DEBT-A D1: no measurable cost |
| O5 | adopt `--foliageScaleBounds` (foliage scaling as the parameter file states)? (T12) | DEBT-A: 5 km 365 → 330 m |
| O6 | adopt `--interferenceDistance` (interferers attenuated by distance)? (T29) | DEBT-A: endgame 5.0 → 4.1 m |
| O7 | adopt `--rssiRangeFromRxPower` (RSSI range includes foliage/fading)? (T46) | DEBT-A: 868 alone 448 → 494 m; hybrid unharmed |
| O8 | keep `--arbitrationV2` (and requester escalation, queue gossip) in the preset? (T42) | DEBT-A: off saves 18 % endgame energy, no accuracy loss |
|    | *2026-10-20, decided by PRESET-2 (DESIGN_DECISIONS 17):* O1, O4–O7 and the D-8 row were registered as one set. The rule failed on the endgame interval, so they are **not adopted**. Arbitration **stays** (off costs +2.06 J once the requester rule can refuse) | `report/PRESET2_RESULTS.md` |
| O9 | refactor the two per-node cycle copies into one (T32)? multi-day, every code path, no result change | N.3 |
| O10 | adopt PRESET-2's set anyway, as an owner override (as DESIGN_DECISIONS 9 did)? | every measured effect upward: 5 km tag 0 0.39 → 0.83, energy −4..−17 %; `report/PRESET2_RESULTS.md` |
| O11 | deploy eight finders, and task them in pairs to save energy? | ANCHOR-SUPPLY: endgame five-node 0.873 → 0.970 with eight; pairs −35 % energy, same availability |
| O13 | keep the Switching-Point Integrity Check in the preset? | INTEG3: not the 5 km cause; costs +0.036 five-node on the ring with no fault; its value needs a fault-injected campaign |
| O12 | make the D-8 row the CODE default (G93 green)? It breaks byte reproduction of every default-config result from its sidecar | D8-MAIN; G93 |
