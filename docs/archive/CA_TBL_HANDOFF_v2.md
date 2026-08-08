# CA-TBL Simulation — Session Handoff v2

**Paste this entire file as your first message in the new chat.** Attach
`finalyearprojectcontext.md` and `LoRa Localization System.md` alongside it.

Supersedes `CA_TBL_HANDOFF.md`, which is now wrong in several load-bearing places —
see §11 for the list, because those errors are instructive rather than merely stale.

Written 2026-07-30. Everything below either traces to a run in the session that
produced it, to a citation, to arithmetic shown inline, or is explicitly marked
unverified. Nothing is marked verified on the strength of a passing check alone.

---

## 0. Your role and the working rules

You are continuing an ns-3 simulation for a final-year Cyber-Physical Systems
project. These rules are not stylistic — each was adopted after a specific failure,
and abandoning one is how this project previously produced a confident-looking
report on invalid numbers.

1. **Correctness before progress.** Do not build a layer on a layer that fails its
   own checks.
2. **Report negative results, do not tune them away.** The user has explicitly
   chosen this. Two of the six claims currently fail. Write them up and diagnose
   them. Never sweep parameters on the data used to evaluate them.
3. **Every number traces to a run, a datasheet, a citation, or arithmetic shown in
   the response.** Mark anything unverified as unverified.
4. **Lead with what is wrong.** State defects first when reviewing.
5. **Distinguish spec from invention.** The design document marks each mechanism as
   borrowed or contributed, and that distinction IS the novelty claim. Carry the
   citation into the code comment. When you pick a value the document does not
   specify, say so in the comment and in your reply.
6. **Build a detector before trusting a result.** Every defect in §3 was found by a
   check added on purpose, not by inspection.
7. **A passing verifier is not a sound result.** Twice this session a check passed
   while the thing it was supposed to protect was broken, and once a check was
   *correct* while its stated diagnosis was wrong. Ask what a check can and cannot
   see before quoting it.
8. **User preferences:** concise, direct, no filler, no praise openers, bullets and
   tables over prose, no visuals unless asked, code without comments unless the
   file's existing style is heavily commented (the CA-TBL files are — match them).

---

## 1. Environment — read before running anything

- Repo: `\\wsl.localhost\ubuntu-22.04\home\kushal\ns\ns-3-dev` (ns-3.47, WSL Ubuntu 22.04).
- **The agent sandbox cannot mount that UNC path.** `mcp__workspace__bash` fails with
  `UNC paths are not supported`, including with an absolute `cd`. You have **no shell
  and no ability to run Python**. Read/Write/Edit/Glob/Grep on the repo work normally.
- **The user runs all builds, simulations and scripts and pastes back the output.**
  Give exact copy-pasteable commands in one block, ordered, and say which failure
  stops the chain. The user's shell is PowerShell driving WSL — avoid `python3 -c`
  with quotes and semicolons, it gets mangled; use a heredoc or a file.
- ns-3 builds *every* file in `scratch/`. One broken scratch file blocks the whole
  tree, including unrelated runs.
- Compiler: `-Wall -Wpedantic -std=c++23`, not `-Werror`.

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

1. Self-deployable mesh topology (not dropped-beacon, not fixed-anchor)
2. Hop-aware confidence degradation feeding the switch decision
3. **A trust/integrity check at the LoRa→UWB switching seam** — the central claim.
   Framed as *environmental fault tolerance* (cold-sagged battery skewing RSSI,
   antenna detune from ice/damage, canopy multipath), **not** an adversary.
4. Identity-scoped, priority-based multi-target arbitration

**Closest prior art, to cite rather than hide: Poucet** (French National Defense;
LoRa 868 + LoRa 2.4 + UWB + GPS + altimeter, field-validated). Poucet has the radio
combination and the fusion. It does not have mesh multi-hop, hop-degradation
confidence, a trust layer on the switching decision, or identity-scoped multi-target
search. Honest framing: "Poucet proved dual-band LoRa+UWB works in the field; we
extend it to a self-deployable mesh with hop-aware confidence and switching-point
integrity, removing its infrastructure dependency."

Explicitly **not** claimed: LoRa+UWB fusion (Poucet), mesh LoRa localization
(LoRaWAPS), threshold switching (AirTag/Janus/GPS duty-cycling), multi-hop
uncertainty propagation (DV-Hop family), Byzantine-robust localization
(SeRLoc/HiRLoc/distance bounding), identity-based access control (standard IoT).

### The six things the simulation must prove

1. Architectural advantage — hybrid beats single-band
2. Procedure advantage — the handoff is worth it
3. Handoff triggers correctly
4. Custom handshake
5. Specialized packets
6. Real-life physics applied to packets

---

## 3. Defects found and fixed in the last session

This is the most useful section. Several are mine, introduced and then caught.

### 3.1 Constant ranging error on tiers 2 and 3 (the original blocker)

`ComputeRangingErrorM()`'s ToF and UWB branches used the configured noise stddev
*as* the model — no bandwidth, no SNR, no distance. Reported 2.4 GHz error was
0.245 / 0.242 / 0.237 m at 100 / 430 / 900 m, i.e. the configured sigma read back
out, against a CRLB of ~15 m. Tier 2 consequently dominated every accuracy-per-joule
comparison non-physically.

**Fixed.** Both branches are now `sigma = max(CRLB(beta_rms, SNR_effective), floor)`,
two-way `/sqrt(2)`, NLOS bias retained additively.

### 3.2 The old handoff's justification for the fix was wrong in two places

- It claimed "LoRa `snr_db` is already post-despread, so do not add processing gain
  again on tier 2." **False.** All three engines compute
  `noiseFloorDbm = -174 + 10*log10(occupiedBW) + NF + shift` and
  `internalSnrDb = rxPowerDbm - noiseFloorDbm`. That is in-band, pre-integration
  SNR. No gain is applied anywhere upstream. The ToA CRLB is a function of E/N0 at
  the correlator output, so the integration gain must be added inside the ranging
  model: `10*log10(2^SF)` for tier 2, `10*log10(PSR)` for tier 3.
- It predicted tier-2 sigma of "~1 m close-in and ~5 m at 900 m". Neither follows
  from BW = 800 kHz. Arithmetic (`beta_rms = 800e3/sqrt(12) = 230,940 Hz`,
  `c = 2.99792458e8`, SNR 16.9 dB, two-way): with the 21.07 dB SF7 dechirp gain,
  1.30 m; without it, 14.76 m.

### 3.3 The prime suspect for the UWB PDR dip was refuted by reading the code

`lora-two-ray-ground-submodule.cc` is not an interference model and has no nulls:
piecewise FSPL below the crossover, `40*log10(d)` above, with
`dc = 4*pi*txH*rxH/lambda`. For UWB at 7.99 GHz (`lambda = 0.0375210 m`), txH 1.5,
rxH 6.0 → `dc = 3014 m`. Every probe distance was below it, so the branch was pure
FSPL — smooth and monotone.

### 3.4 Sub.21 runs unconditionally in BOTH ranging modes

`m_nodePositionUncertainty->Compute()` is called in the Tier-2 group regardless of
`rangingModelMode`, and writes a nonzero `state.propagationLossBiasDb` that Step 14
adds into `additional_loss`. The old comment at the ranging call site claimed Sub.21
was skipped in `RANGING_PHYSICS_DERIVED`. It is not.

Consequence: moving the tier-2/3 ranging call past Step 14 (necessary, because it now
needs the post-link-budget SNR) would have leaked Sub.21's RSSI-inversion bias into
the 2.4 GHz and UWB link budgets. The call is therefore **split** — PART 1 at
Step 4.5 zeroes the field, PART 2 at Step 15 computes the error. Do not recombine
them.

### 3.5 Probe antenna heights were wrong on two of three tiers

The probe used 6.0 m for all three while `BuildDeployment()` uses 15.0 / 6.0 / 2.5 m.
Since `dc = 4*pi*txH*rxH/lambda`, the probe was measuring a different propagation
regime than the arms run in — and the r80 values the deployment geometry is *sized
from* came from the wrong geometry. Fixed.

### 3.6 The azimuth-rotation decorrelation fix was too weak at short range

Rotating azimuth at radius `d` moves the node `2*pi*d/trials` per trial, so the swept
path is proportional to `d`. At UWB 10 m the realised sample stddev of the shadowing
draws was **0.21x** the effective sigma — on the order of 20 independent draws, not
2000. This is `simstatus.md` Bug #1 recurring where the previous fix does not reach.

**Fixed** by cycling each trial across 64 well-separated gateway sites on a
plastic-number low-discrepancy lattice spanning ±2000 m, one gateway id per site per
tier, so each trial draws a different large-scale realisation while azimuth continues
to decorrelate the local channel.

**Important limitation of the detector I added for this.** The shadowing-independence
gate audits the *shadowing* field only. It passed at 0.86 on tier 1 while tier-1 PDR
was still biased by ~15 points at 1000 m, because terrain, foliage density and the
correlated obstruction state are separately spatially keyed and were all frozen at one
location. Evidence: tier-1 r80 moved 500 m → 1000-1500 m when site cycling was added,
with the gate reporting "OK" both times. **A passing shadowing gate does not mean the
point is decorrelated.**

### 3.7 r80 was estimated by grid-snapping and was unstable

Two runs of an identical configuration gave tier-1 r80 = 1500 m and 1000 m, because
PDR at 1500 m straddles the 0.80 threshold (0.813 vs 0.797 — a 1.8 sigma difference
at n=2000, so ordinary binomial noise crossing a hard threshold on a coarse grid).
Fixed by linear interpolation on the bracketing pair. Still censored at the sweep
edge: if the last point is above threshold, the value is a lower bound.

### 3.8 Ranging sigma was computed and thrown away

`ComputeRangingErrorM()` returned sigma "for the EKF R-matrix diagonal"; all three
engine call sites ignored the return, and `CaTblPolicy` built R from fixed per-tier
constants `losSigmaM = {50, 15, 0.1} m`. Against the measured error those were
over-confident on tier 1 by up to 58x (50 m assumed against 2907 m actual at 6 km)
and under-confident on tier 2 by ~5x. The filter was trusting tier-1 garbage and
discounting good tier-2 ranges.

**Fixed.** `LinkPhysicsState.rangingSigmaM` → `result.rangingSigmaM` →
`CaTblRangeMeasurement.sigmaM` → R. Per-tier constants demoted to a fallback for
`RANGING_SYNTHETIC_GPS_DENIED`. Effect: `static_868` RMSE fell 414.5 → ~137 m.

### 3.9 `dt = 1.0 s` was an illegal duty cycle

~56.6 ms tier-1 airtime per cycle is a 5.66% duty cycle against the WPC G.S.R.
853(E) Table-II 2.5% field-node cap. Every arm to that point ran illegally. Default
is now `dt = 2.5 s` → 2.24%. Minimum compliant is 2.264 s. This halves the cycles in
a 3600 s run (3600 → 1440), so older tables are not comparable on energy or counts.

### 3.10 MY REGRESSION: `nominalRangeM[UWB] = 10` made the label unsatisfiable

I "corrected" the geometric coverage labels from r50 to r80. With `--uwbTriad = 10 m`
the anchors sit ON a 10 m ring at z = 2.5 while the node is at z = 1.5, so even at the
exact facility centre the slant range is `sqrt(10^2 + 1^2) = 10.0499 m` — outside a
10.0 m label radius. `visUwb` was identically zero and `trigF1` was 0.000 on all six
arms.

**My reasoning error, worth internalising:** I argued r50 "labels as covered a
distance where half the links fail". That treats a fix as needing one good link. It
needs three of N, and this deployment sites N > 3 on purpose. At p = 0.5 with N = 5,
`P(>=3 of 5) = (10+5+1)/32 = 0.5`, so **r50 is exactly the coin-flip boundary**,
which is what "could plausibly have solved" means. Reverted to `{3000, 900, 30}`.

**Never set a label radius equal to the anchor ring radius it is labelling.**

### 3.11 MY REGRESSION: the tier-1 fallback blinded de-escalation

`ca-tbl-policy.cc` already had a coverage timeout — when `meas.size() < 3` it counts
the dropout toward demotion so a node cannot strand itself on a tier it walked out of.
Adding the concurrent tier-1 floor made `meas.size()` always >= 3, that branch never
fired, and the policy latched on UWB after 4 escalations for the remaining 1436
cycles.

Arithmetic that identified it: `ca_tbl_full` energy 40.2242 J against
`static_uwb` 28.3002 + `static_868` 12.1908 = 40.491, and RMSE 120.729 m against
`static_868`'s 135.853 m — paying UWB energy every cycle, falling back to tier 1 for
the fix.

**Fixed** by adding `activeTierRanges` to `CaTblPolicy::Step()` so de-escalation is
driven by whether the *active tier* delivered, while the EKF fuses whatever is
available. There is a dedicated branch: if the active tier did not deliver, count
toward demotion and **bypass the PCS decision** — because PCS on that cycle was
computed from the fallback fix and describes tier-1 confidence, so comparing it to
`pcsThresholdUp[m_tier]` would read "escalate further" at the moment the correct
action is to come down.

### 3.12 THE BIG ONE: wall-clock time was gating an RNG draw

`lora-microdsp-submodule.cc` measured `std::chrono::high_resolution_clock` around a
block of arithmetic, then:

```cpp
if (res.microDspRuntimeUs < 15.0) {
    res.microDspRuntimeUs = m_sampler.Sample(DistributionParam::Uniform(15.0, 45.0));
}
```

The timed block completes in well under a microsecond, `count()` truncates to 0, the
branch is taken and one sample is consumed. When the OS preempted the thread or a page
fault landed in that window, elapsed time exceeded 15 us, the branch was **skipped,
no sample was drawn**, and that engine's RNG stream shifted by one draw for the rest
of the run.

The simulation was not reproducible from its seed, with the divergence point set by
machine load. Seed 7 gave 157 UWB fixes on one run and 171 on the next; `static_868`
RMSE ranged over 128.3-149.1 m across four runs of the same seed, including across a
build that only added a metric column.

**Every arm table produced before this fix is void.** `<chrono>` is now removed from
that file so reintroducing a clock read is a compile error. The quantity was also
conceptually wrong — it models an embedded DSP's runtime, which has no relationship
to how fast the host executes the function — so it is now sampled unconditionally.

If determinism ever breaks again: `grep -rn "chrono\|clock()\|time(" src/` and look
for any wall-clock value that reaches a branch, not just one that reaches output.

### 3.13 MY BAD TEST: the first determinism check was invalid

I compared a run with traces on against a run with `--trace=false`. Two different
commands is not a determinism test. (The trace flag was later proven irrelevant:
untraced `nofix = 1315` matches traced `125/1440` fixes exactly.) Always diff
identical invocations.

### 3.14 `rmseFixedM` was still confounded, and so was its replacement's first version

Conditioning RMSE on "had a fix" removes the coverage confound and leaves the
reconvergence one: post-outage cycles carry a drifted prior, and only an arm *with*
outages pays that term. `static_2p4` never has an outage. So `rmseFixedM` still
cannot be compared across arms with different coverage.

The UWB ranging detector is now the **sustained-coverage median** (fix on this cycle
and the previous one), checked against 1.0 m with the probe sigma at ~0.14 m. That is
tighter than the check it replaced, not weaker: a tier that is not ranging lands in
the tens of metres.

---

## 4. Current state of the four numbered claims plus two

| # | Claim | Status | Blocker |
|---|---|---|---|
| 1 | Hybrid > single-band | **FAILS** | `static_2p4` beats `ca_tbl_full` 3.90x on acc/J. Real, but on n=1 and a confounded energy axis. |
| 2 | Handoff worth its complexity | **FAILS** | `fixed_threshold` beats `ca_tbl_full` 1.71x. Same caveats. |
| 3 | Trigger correctness | ~40% | `trigF1` 0.264 is barely above the always-yes degenerate score. Thresholds hand-set, no LOOCV. |
| 4 | Custom handshake | substrate done | `CaTblPhyProvider` not wired to real engines. |
| 5 | Specialized packets | substrate done | same |
| 6 | Real physics on packets | ~85% | MAC has emergent collisions against a stub PHY; 0 captures means the capture path is untested. |
| — | **Central integrity claim** | **unexercised** | `susp = 1` in 1440 cycles. Cannot be claimed at any confidence. |

---

## 5. Verified results — safe to quote, with their caveats

### 5.1 Determinism

Two runs of `--seed=7 --duration=3600 --trace=false` produce byte-identical
`arm_summary.csv`. This is the precondition for everything else and it is the first
build in the project's history for which it has been demonstrated.

### 5.2 Arm table, seed 7, dt = 2.5 s, 1440 steps, deterministic build

```
arm                    rmse_m  rmseFix_m  cep50_m   cep95_m   energy_J  esc  susp  nofix  trigF1     acc/J
static_868            137.158    137.158   99.384   260.049    12.1908    0     0      0   0.000  5.981e-04
static_2p4              2.206      2.206    1.752     3.883    16.4370    0     0      0   0.000  2.757e-02
static_uwb            456.020      3.291   93.786  1154.633    28.3492    0     0   1315   0.199  7.735e-05
fixed_threshold         3.298      3.298    2.193     6.386    25.0518  199     0      0   0.000  1.210e-02
ca_tbl_no_integrity     5.434      5.434    2.717     8.742    36.1196  180     0      0   0.279  5.095e-03
ca_tbl_full             3.955      3.955    2.487     8.173    35.7864  177     1      0   0.264  7.065e-03
```

`acc/J = 1/(rmse * J)`. **This is one seed.** Nothing here is a result until the
20-seed sweep with confidence intervals.

### 5.3 The strongest single measurement in the project

**UWB sustained-coverage median error 0.164 m over 101 cycles** (fix on this cycle and
the previous one), against a probe ranging sigma of ~0.10-0.14 m. The ranging model
and the pentagon anchor geometry are both vindicated by this: `gdop med = 1.03`.

### 5.4 The most interesting finding so far

`static_2p4` achieves a better fix-conditional RMSE (2.21 m) than always-on UWB
(3.29 m) **while ranging roughly 20x less precisely**. UWB's sustained error is at its
ranging floor; the gap is post-outage reconvergence, which only an arm with coverage
gaps pays. **Coverage beats precision at this deployment scale.**

Caveats that must travel with it: n = 1; and it depends on siting choices nobody has
swept (`uwbTriad = 10 m`, four facility sites in a ±1000 m box, `t2Triad = 150 m`
against a tier-2 r80 of >= 900 m). Densify the UWB siting and it could invert.

### 5.5 Probe, 2000 trials/point, 64 decorrelation sites

| tier | r80 | r50 | notes |
|---|---|---|---|
| 868 MHz | ~1522 m | ~3217 m | range err 51.2 m @100 m → 2906 m @6 km, ~50% of true distance throughout |
| 2.4 GHz | **>= 900 m (CENSORED)** | >= 900 m | sweep ends at 900 m with PDR 0.804 and no falloff |
| UWB | ~13.5 m | ~28.0 m | near-pure LOS (`nlos_frac <= 0.004`) yet a tenth of the 100 m LOS reference spec |

All three tiers pass monotonicity, the shadowing gate, and the CRLB gate.

**868 range error is ~50% of true distance at every range.** Tier-1 fixes come from
multilateration across 9 gateways, not from link ranging. State this before quoting
any tier-1 RMSE.

**UNVERIFIED:** the tier-2 probe row was never observed after `tofLosFloorM` went
1.0 → 3.0 m. Expect ~3.0 m flat instead of ~0.8 m. The arm result (`static_2p4`
2.206 m) is consistent with it but is not the same measurement.

### 5.6 S4 MAC validator: 38/38

`./ns3 run "ca_tbl_packet_validator --outdir=results/s4"`

ToA SF12/125 kHz/50 B = 2.302 s; digest vs full push at 8 peers = 2.138 s vs 8.364 s;
priority CW separation URGENT 1.89 ms vs LOW 15.96 ms = 8.4x; 284 tx → 1129 collision
events, 859 deliveries, **0 captures**; duty gate 2.413% against 2.5% and 9.707%
against 10%; flood reached 4 of 8 at hop_limit 3.

Caveats: 0 captures is correct but means the capture path is untested (the stub gives
every link identical power). Only `CW_urgent = 0-3` comes from the spec; HIGH/NORMAL/
LOW are the 802.11e doubling pattern, uncalibrated, as are `slotTimeS` and the 6 dB
capture threshold. The flood test is a line topology so it never tests the
broadcast-storm case jitter exists to prevent.

---

## 6. THE BLOCKING DEFECT — fix this first

**The energy axis measures anchor count, not radio cost.**

| tier | anchors | energy (J) | J per anchor |
|---|---|---|---|
| 868 | 9 | 12.1908 | 1.3545 |
| 2.4 GHz | 12 | 16.4370 | 1.3695 |
| UWB | 20 | 28.3492 | 1.4175 |

A 4% spread in per-anchor cost against a 2.2x spread in anchor count. The arm loop
blind-polls every anchor of the tier every cycle — including the 15 UWB anchors at
other facilities, kilometres away — and the energy model correctly charges the full
RX timeout for each non-responder. So SX1262 vs SX1280 vs DW3000 current draw
contributes almost nothing to `totalJ`.

`totalJ` is the denominator of every `acc/J` number, so **both claim-1 and claim-2
verdicts currently rest on a confounded axis, in whichever direction they fall.**

The fix is to gate polling through the S4 MAC's discovery/neighbour cache — the
`CaTblPhyProvider` wiring that claims 4, 5 and 6 also need. This is one job that
unblocks four claims.

---

## 7. Second and third blockers

### 7.1 Tier-2 r80 has never been measured

`dists2p4` ends at 900 m with PDR 0.804. Tier 2's reach is the entire reason claim 1
fails, so this is the most load-bearing unmeasured number in the project. Extend
`dists2p4` past 900 m (say 1200, 1600, 2000, 2500 m) and re-run the probe. Cheap.

### 7.2 EKF process noise is under-specified for long coasting gaps

Post-gap first fixes: n = 12, median 2.148 m, **RMSE 28.093 m** — so one or two carry
~90 m error. GDOP cannot explain that: at the worst observed geometry, position error
from ranging alone is `9.84 * 0.142 = 1.4 m`. A 90 m post-fix error requires the
filter to have accepted a good measurement and moved only partway, i.e. a prior
covariance far too small for the drift actually accumulated. `Q` does not grow with
coasting time.

`measurement rejections on fix cycles: 0` is **luck, not correctness**. The
Mahalanobis gate compares the residual against `P + R`; a slightly different `P` and
it would have rejected the one measurement capable of correcting the drift, and the
filter would have diverged permanently with no route back.

Covariance inflation after an outage is probably the right engineering. Justify it
from the arithmetic above and validate across 20 seeds — **not** because a verifier
line goes red.

---

## 8. Everything else outstanding

1. **The UWB engine runs the LoRa CSS chain.** `GetSensitivityThreshold(spreadingFactor,
   CR)` and `m_cssPhy->ComputeDemodulation(...)` applied to HRP UWB, which has no
   chirp to dechirp. Pre-existing and structural. It sets the r80 = 13.5 m that drives
   the entire coverage argument, so it is not a cosmetic approximation.
2. **`nlosSigmaInflation = 3.0`** is the only uncited number left in the R-matrix
   path. Introduced this session to stand in for the unmodelled NLOS excess-delay
   bias. Sweep in S7.
3. **PCS thresholds `1.55 / 1.70 / 2.30`** are hand-set, carried from the Python
   reference. LOOCV is promised in the design doc and not implemented.
4. **`hopWeight = 0`.** Do not claim hop-provenance confidence degradation until S5.
5. **Trigger ground truth is geometric, not error-based.** `shouldUwb := visUwb >= 3`
   asks "could UWB have solved here", which is necessary but not sufficient for
   "escalating was right". Report `trig_f1` as a coverage-tracking score.
6. **Case A/B/C resolution** (2 anchors / 1 anchor + 1 peer / 0 anchors) is not
   implemented; below 3 ranges the filter coasts.
7. **MCU energy is not modelled**, only radio. Regulator efficiency and PA ramp
   excluded.
8. **SF12 is unusable in this simulator** — PDR 0.255-0.355 at 500 m against SF7's
   1.00, inverted from physics, almost certainly collision-dominated. The genuinely
   long-range tier-1 case cannot currently be simulated.
9. **Tier-1 anchor height sensitivity untested.** 15.0 m is the deployment value; the
   earlier suspicion that raising it hurt coverage turned out to be a decorrelation
   artifact, so this is now a secondary sensitivity question rather than a suspected
   bug.
10. **`--vegFacility` / `--envFacility` unswept**, and both were chosen in the
    direction that helps UWB. The siting justification holds independently of outcome,
    but if the architectural conclusion depends on these values, that dependence is
    the finding.
11. **Verifier check 12** (`always-on UWB costs more than 868-only`) currently passes
    for a partly wrong reason — see §6. It will keep passing once the energy axis is
    fixed only if UWB really is the most expensive radio.

---

## 9. What the verifier can and cannot see

Current state: **72 checks passed, 3 DOCUMENTED FINDINGS, 2 WARNINGS, 0 FAILURES.**

Read that with three things in mind.

- **The two WARNINGS are claims 1 and 2 failing.** A clean run is not a good result.
- **One failure became a NOTE by my hand**, not by the simulation improving. The old
  check required `static_uwb` to have the lowest RMSE of any arm; it was written as a
  detector for a *dead UWB tier* (the pre-S2.5 bug where it completed zero exchanges
  in 600 cycles) and could no longer serve that purpose once r80 fell to 13.5 m. I
  replaced it with the sustained-coverage median detector, which is tighter. The
  argument is independent of which way the numbers fell, but you should know the
  count moved because I moved it.
- **The shadowing gate is necessary, not sufficient** (§3.6). It audits one spatial
  field out of four.

New verifier machinery this session: a `NOTE` / DOCUMENTED FINDINGS category for a
gate whose failure is the measured result, so a permanent known failure does not train
the reader to skip the gate.

---

## 10. Recommended order for the new session

1. Read this file, then `CA_TBL_SIM_AUDIT.md`, `CA_TBL_S1_S2_README.md`, and
   `simstatus.md` Part 6 (16 bugs, each of which had produced a confident wrong
   answer). Note that the two READMEs are current only through S2.5 and know nothing
   about S4 or anything in §3 here.
2. **Extend `dists2p4` past 900 m and re-run the probe** (§7.1). Cheapest
   high-value measurement available. Also confirms the tier-2 range error is ~3.0 m
   after the floor change (§5.5, unverified).
3. **Wire `CaTblPhyProvider` to the three physics engines** and gate anchor polling
   through the MAC neighbour cache (§6). This unblocks the energy axis and claims
   4, 5 and 6 together.
4. Re-run the single seed, confirm determinism still holds, then **20-seed sweep with
   confidence intervals**. Nothing in §5.2 is a result until this exists.
5. Floor sensitivity: `tofLosFloorM` at 1.0 vs 3.0 m, both reported. The 3.0 m value
   is cited to field measurement (§12) but it moves claim 1 toward the project's own
   thesis, so the sensitivity is not optional.
6. Siting sweeps: `--boxHalf`, `--uwbTriad`, `--vegFacility`/`--envFacility`.
7. Then S5 (mesh + multi-hop, which is what makes `hopWeight` legitimate), S6
   (multi-target + identity), S7 (LOOCV on the PCS thresholds, fault injection to
   actually exercise the integrity check).
8. Only after S7 does the central novelty claim have any evidence at all. It has
   fired once.

---

## 11. Where the previous handoff is now wrong

Keep it for the novelty-verification thread and the Poucet analysis, which remain
correct and useful. Do not trust it on:

- §6 "THE BLOCKING DEFECT" — fixed, and its stated justification was wrong twice
  (§3.2).
- §7 "UWB PDR non-monotonicity / prime suspect two-ray ground null" — refuted (§3.3),
  and the dip itself was a decorrelation artifact (§3.6).
- §8 "Verified results" r80 values — all superseded by the 64-site probe (§5.5).
- §8 "~9 gateways per 4 km^2" — the pitch survives at r80 >= 1000 m, but the number
  was derived from a probe with the wrong antenna heights and undecorrelated
  shadowing. Re-derive before quoting.
- §10.1 `dt = 1.0 s` illegal — fixed, default is 2.5 s.
- §10.2 the probe's CRLB check omitting coherent gain — fixed.
- Any claim that the simulation is reproducible from its seed. It was not, until
  §3.12.

---

## 12. Citations added this session

- **`tofLosFloorM = 3.0 m`** — Zhou et al., "Impact of Environmental Factors on LoRa
  2.4 GHz Time of Flight Ranging Outdoors", arXiv:2509.23125 (2025). SX1280 RF ToF on
  an open sports field, LOS, 40 cm antenna height, ~1000 samples at each of nine
  reference points spanning ~28-57 m. Standard deviation of absolute ranging error:
  1.27 m at the single best point, "the majority of points fall in the 2.5-3.2 m
  range". Supersedes Semtech's "meter-level, up to 2 m" product claim, which is a
  best-case marketing figure.
- **`uwbLosFloorM = 0.10 m`** — Benouakta et al., Electronics 2023, 10-33 cm for
  DW3000-class hardware. The previous 0.05 m sat below the interval it claimed to
  cite.
- **`uwbPreambleSymbols = 1024`** — DW3000 Family User Manual lists PSR settings
  16/64/128/1024/4096; processing gain scales with PSR and PAC size. 1024 gives
  30.1 dB, which is what makes UWB decodable at all: raw UWB SNR is negative
  (-17.4 dB at 2 m) because the noise floor over 499.2 MHz is ~-87 dBm against a
  -104 dBm sensitivity gate.

Still uncited: `tofNlosFloorM = 3.6` (1.2x the LOS floor), `uwbNlosFloorM = 0.25`,
`nlosSigmaInflation = 3.0`, the PCS thresholds, HIGH/NORMAL/LOW contention windows,
`slotTimeS`, the 6 dB capture threshold.

---

## 13. Command reference

```bash
cd ~/ns/ns-3-dev
./ns3 build 2>&1 | tail -40

# S4 MAC unit tests -- deterministic, ~1 s. Run first: if the protocol fails its own
# spec there is no point running physics on top of it.
mkdir -p results/s4
./ns3 run "ca_tbl_packet_validator --outdir=results/s4" 2>&1 | tail -80

# Geometry + physics pre-flight. Seconds. Require: monotonicity OK, shadowing
# decorrelation OK, and CRLB OK on all three tiers.
mkdir -p results/causal
./ns3 run "ca_tbl_causal_sim --probe=true --outdir=results/causal" 2>&1 | tail -100

# Determinism gate. IDENTICAL flags on both runs -- this is not optional, see 3.13.
mkdir -p results/causal_rep
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal --trace=false" 2>&1 | tail -30
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal_rep --trace=false" 2>&1 | tail -5
diff results/causal/arm_summary.csv results/causal_rep/arm_summary.csv && echo "DETERMINISTIC"

# Traced run for the trace-dependent checks, then the verifier.
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal" 2>&1 | tail -30
python3 scratch/python_handoff/verify_causal_sim.py results/causal

# Seed sweep, once the energy axis is fixed and determinism holds.
./ns3 run "ca_tbl_causal_sim --seedStart=1 --seedCount=20 --duration=3600 --outdir=results/causal --trace=false"
```

`ca_tbl_causal_sim` knobs: `--env --envFacility --vegWide --vegFacility --boxHalf
--uwbTriad --t2Triad --uwbAnchors --t2Anchors --speed --dwell --dt --duration --seed
--seedStart --seedCount --trace --probe --outdir`

Current defaults, all derived from the probe rather than chosen: `boxHalf=1000`,
`uwbTriad=10`, `t2Triad=150`, `uwbAnchors=5`, `t2Anchors=3`, `env=1`
(RURAL_FORESTED), `envFacility=0` (RURAL_OPEN), `vegWide=3` (DENSE_FOREST),
`vegFacility=1` (SPARSE_SHRUBS), `duration=3600`, `dt=2.5`.

---

## 14. First message to send

> Read `CA_TBL_HANDOFF_v2.md`, then `CA_TBL_SIM_AUDIT.md`, `CA_TBL_S1_S2_README.md`,
> and `simstatus.md` Part 6.
>
> Two tasks, in order:
> (a) Extend `dists2p4` in `RunProbe()` past 900 m so tier-2 r80 is measured rather
>     than censored. It is the number claim 1 turns on.
> (b) Wire `CaTblPhyProvider` to the three physics engines and gate anchor polling
>     through the MAC neighbour cache, so `totalJ` stops measuring anchor count.
>     See section 6.
>
> I have no shell — give me exact commands and I will run them and paste the output.
> My shell is PowerShell driving WSL, so avoid inline `python3 -c` with quotes.
> Report negative results rather than tuning them away. Claims 1 and 2 currently
> fail; that is expected and I want them diagnosed, not fixed by parameter choice.
