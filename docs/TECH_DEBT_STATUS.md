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
| T2 | ~~L.2 note~~ | ~~The casualty's table entry carries the previous cycle's estimate (published before `Step`): every entry age is understated by one cycle (2.5 s)~~ **DONE (code)** `--publishAfterStep`, gate G150 (927/927 entries fresh). Preset adoption: owner (O1). | ENG |
| T3 | ~~M.9~~ | ~~The RECEDING eager push is not counted per subject~~ **CLOSED (moot)** subjects never call StartHandshake, so no subject push exists; G75's count is complete. | ENG |
| T4 | ~~M.11 / RULES.md~~ | ~~Stale counts in RULES.md §7 ("58 gates at c79fffd", `cmd.AddValue` line ranges, the unit-suite count)~~ **DONE** RULES.md counts replaced by the commands that produce them. | ENG (doc) |
| T5 | ~~I.4~~ | ~~`--bidirectionalDigest`, `--finderPseudoAnchor`, `--corroborationCheck` silently need `--sendHandshakeFrames`; their help text does not say so~~ **DONE** help text of --bidirectionalDigest and --finderPseudoAnchor states the dependency (verified by run); --corroborationCheck already did. | ENG (doc) |
| T6 | ~~H.3 / I.5~~ | ~~No gate asserts that a mechanism's input is non-zero whenever its firing counter is~~ **DONE** gate G151: all 12 adopted preset settings change the output; inert fixture called INERT. | ENG (gate) |
| T7 | ~~N.29~~ | ~~M-01's SF12 "inversion" is the validation harness colliding with itself (0.1 s spacing)~~ **DONE** harness spaces frames at 2× time on air; SF12 500 m PDR 0.015 → 1.000 (seed 7). | ENG |
| T8 | 4.1 / 4.2 | The 1-ulp trace divergence is undiagnosed; `-march=native` makes byte identity machine-dependent. Suggested test: rebuild with `-march=x86-64` | ENG (diagnosis) |
| T9 | 6 | `src/lorawan` link fix never upstreamed; the optimized profile is not enforced | ENG (minor) |
| T10 | 2.1 | Four loss terms (antenna gain, Doppler, multipath, soil) are computed then zeroed: wire them in or delete them | OWNER → CAMP |
| T11 | 2.2 | Terrain uses 868 MHz for every tier; `--terrainPerTierFreq` exists, default off, not in the preset | CAMP |
| T12 | N.41 | Foliage scaling on 2.4 GHz/UWB is capped by the sampler clamp (×1.8 instead of ×2.17/×5.28) | CAMP (engine fix + campaign) |
| T13 | N.27 / G93 | Tier 2's sensitivity table is the 1625 kHz row at 800 kHz (~6 dB pessimistic); `--t2SensitivityRowKHz` measured (D8-MAIN), not default | OWNER |
| T14 | N.28 | DC-1's gyro bias is the bias instability: every preset accuracy figure assumes a near-perfect gyro. The EKF5 remedy is on another author's branches (not merged) | OWNER (merge EKF5?) → CAMP |
| T15 | 1.3 / N.30(main) / G92 | 15 cited `results/` paths are not in the repo, 3 of them cited by the Technical Report | ENG (re-point or exclude) |
| T16 | I.6 / G6 | G6 red while pre-D1 evidence is cited; the remedy is regenerating to new paths and moving citations | CAMP |
| T17 | N.15 / G24 | G24 FAIL BY FINDING: mechanisms with no population under the preset | OWNER (accept) or ENG |
| T18 | J.1 / 4.4 / G11 | The analytic model and engines disagree (the 2.4 GHz gap grows with distance). Must not be tuned; research | LIMIT |
| T19 | 2.3 | UWB ~3.7 dB and 868 MHz ~3.5 dB near-field residuals unexplained | LIMIT |
| T20 | N.40 | Tier-2 ToF floor 3.0 m by default; every tier-2 accuracy claim is conditional on it (Wolf/Müller presets exist) | OWNER (preset ToF model) |
| T21 | N.15 tail / N.32 | The integrity check detects almost nothing at χ² 1992.9 (recall ≤ 0.03); re-fit after a consistent covariance | CAMP |
| T22 | O.12 / N.51 | 5 km consistency 0.69: residual 868 MHz-fix overconfidence | LIMIT |
| T23 | O.13 / O.10 | Under calibrated Wolf the hybrid costs ~10× 2.4 alone (UWB escalation); `--rssiCorrInflate` fixes accuracy, not adopted (missed by 0.3 m) | OWNER |
| T24 | O.9 | RSSI ranging's path-loss model does not follow the foliage model | ENG (diagnose) |
| T25 | O.8 | P.833 depth is drawn, not path geometry; one forest only | LIMIT |
| T26 | N.35–N.37 | 5 km within-25 m loss of the hybrid's subjects against static 2.4 GHz unexplained; H2 TDoA form untested | CAMP |
| T27 | N.52 | The preset keys R on NLOS truth (`--nlosSource=propagation`); `--nlosRSource=detector` exists | CAMP |
| T28 | N.50 | `--unknownStart` is not the code default (all recent campaigns pass it) | OWNER |
| T29 | M.1 open | The interference registry never attenuates an interferer by distance; relay rounds sit in the MAC-run slice | CAMP (engine) |
| T30 | M.2 / L.2.2 | The UWB CIR confusion matrix cannot fill under the channel model; needs an obstruction model of our own | OWNER |
| T31 | N.1 / N.20 | `--silenceStopsRelay`, `--deadBatteryNoDraw` built, default off, not in the preset | OWNER |
| T32 | N.3 | Two copies of the per-node cycle core (tag 0 and subjects); per-subject SHS not built | ENG (refactor, large) |
| T33 | M.10 | C6's 20-seed edge-server energy figure not re-measured under the fix-tier downlink bill | CAMP |
| T34 | N.8 / N.13 | Explicit DENY frames cost airtime (design choice); Case C shapes coarse and rarely reach the base station | OWNER |
| T35 | N.11 | Converting RSSI variability to a ToF-tier range sigma is OURS and open (flag out of preset) | LIMIT |
| T36 | N.42 | Lim, Hofer, Robyns values still lack a source | LIMIT |
| T37 | H.4 / B | Constants ours and uncited (finder GNSS sigma, corroboration k=3/30 m/0.5) | LIMIT (record) |
| T38 | K.9 | SHS gait calibration diverges where fixes are sparse (debt by owner decision) | OWNER |
| T39 | M.3 | `--stampBinaryHash`/`--stampBuildCommit` exist, not in the preset | ENG (preset entry + NC) |
| T40 | 5.1 | 137 published commits carry the wrong author email; fixing needs a force-push to protected `main` | OWNER |
| T41 | 5.3 | No gate catches stale documents | ENG (gate, partial) |
| T42 | L.3 claim 3 | Identity-scoped arbitration has never been measured as a claim | CAMP |
| T43 | C.5 | Self-deployable mesh is not built (fixed gateways plus finders) | OWNER (scope) |
| T44 | 3.5 | Anchors are hidden terminals to each other (beacon loss is an upper bound) | LIMIT (stated) |
| T45 | E1 | Same-site field test | HW |

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
| O1 | adopt `--publishAfterStep` into the preset? (T2) | G150; correctness only, entries 2.5 s fresher |
