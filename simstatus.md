# Multi-Modal LoRa/UWB Localization System — Complete Technical Report

> **HISTORICAL — not maintained, and not covered by the citation audit.** This is the prior
> study's report, retained for its defect record. Superseded by `Architecture.md` and
> `Simulation.md`. Figures here may predate the D1 six-decibel sensitivity-table correction
> (`0d5e747`) and the post-D1 regeneration of every campaign. Do not cite numbers from this file.

**Team 41 | Final Report**

---

## Part 1 — The Big Picture

### 1.1 What This System Is Trying to Do

The goal is to track the location of low-power, battery-operated devices (asset trackers, livestock tags, personnel badges — anything that needs to know "where am I" without GPS, or as a backup to GPS) across a large, sparsely-covered rural area, and then figure out which combination of technologies delivers the best accuracy for the least cost and complexity.

Three questions drive the whole exercise:
1. **Which network backhaul architecture works best** — should devices talk directly to a handful of long-range gateways (WAN), relay through each other in a mesh (MESH), or some mix (HYBRID)?
2. **Which short-range refinement radio is worth adding** — a 2.4 GHz radio doing round-trip-time (RTT) ranging, or an Ultra-Wideband (UWB) radio?
3. **Is an inertial measurement unit (IMU) — an accelerometer/gyroscope chip that can estimate motion between fixes — worth the extra cost and power draw?**

To answer these honestly, the project built a physics-based simulator, generated millions of synthetic but physically realistic tracking events, and measured how each combination of choices actually performs. A very large fraction of the total effort — documented in Part 6 — went into catching and fixing places where the simulation was quietly cheating (giving results too good, or too strange, to be believed) before trusting any of the final numbers.

### 1.2 The Two-Stage Localization Concept

Every device in this system localizes itself in two stages, and understanding this split is the key to understanding everything else in this report.

**Stage 1 — Coarse localization, always runs.** Every device periodically transmits a short radio packet on the "India sub-GHz" LoRa band (~865 MHz). If enough gateways (or relay paths, depending on topology) can hear that packet, the system can compute a rough position using **Time Difference of Arrival (TDOA)** — comparing exactly when the same packet arrived at multiple different receivers to triangulate where it came from. LoRa's long range makes it possible to cover many square kilometers with only a handful of gateways, but that range comes at a cost: the timing precision achievable at 865 MHz over many kilometers is coarse, so Stage 1 alone typically locates a device to within tens to a few hundred meters, not centimeters.

**The handoff decision.** After Stage 1 produces a position estimate, the system checks two things: is the estimated error small enough (≤ 50 m) to be worth refining, and is a short-range refinement anchor actually nearby? If both are true, the device attempts Stage 2.

**Stage 2 — Short-range refinement, conditional.** If the handoff succeeds, the device switches to a second, short-range radio — either 2.4 GHz RTT ranging or UWB ranging — against a nearby fixed anchor (think: a beacon mounted at a warehouse gate, loading dock, or checkpoint). This second radio only ever has to cover tens to a couple hundred meters, so it can achieve dramatically better precision than Stage 1 — down to sub-meter in good conditions.

**IMU fusion, optional.** If the device also carries an IMU, its dead-reckoning output (estimated motion since the last fix) can be blended with whichever position signal is currently available, via an Extended Kalman Filter (EKF, explained in Part 4.4). Critically, IMU fusion can help even when Stage 2 fails — it can "bridge" a gap using motion estimation until the next real fix arrives.

### 1.3 Why Split It This Way

No single radio does everything well. LoRa's whole design point is long range at low power, which is fundamentally incompatible with fine timing precision. Short-range radios (2.4 GHz, UWB) can be extremely precise, but only within their own limited range — they cannot replace a wide-area backhaul on their own. Splitting the problem into "get roughly there cheaply, then get precise where it's cheap to do so" is the standard architecture pattern for GPS-denied hybrid positioning systems, and this project's whole purpose is to quantify exactly how much each piece is actually worth, rather than assume it.

---

## Part 2 — The Building Blocks (The Test Grid)

The final study is a grid of **3 topologies × 5 methods × 3 Stage-2 density tiers**. Each axis is explained below.

### 2.1 The Three Network Topologies

| Topology | What it means | Backhaul mechanism |
|---|---|---|
| **WAN** | Wide-area network | Every device talks directly to one of 10 fixed gateways spread across the area. No relaying. |
| **MESH** | Mesh network | Devices relay data through each other (up to 2 hops) to eventually reach any gateway. No device needs a gateway in direct range. |
| **HYBRID** | Mix of both | Devices use direct gateway access when available, and fall back to mesh relay otherwise. |

The simulated area is **2,500 km² (50 km × 50 km)**, populated with **500 devices and 10 gateways**, all randomly placed. This same physical layout — the same device and gateway positions — is reused across all three topology labels within a given simulation run; topology only changes how data gets *routed*, not where anyone physically is.

### 2.2 The Five Localization Methods

| Method | Stage 1 | Stage 2 | IMU |
|---|---|---|---|
| **M0** | India-band TDOA | None (reference floor) | No |
| **M1** | India-band TDOA | 2.4 GHz RTT | No |
| **M2** | India-band TDOA | 2.4 GHz RTT | Yes |
| **M3** | India-band TDOA | UWB | No |
| **M4** | India-band TDOA | UWB | Yes |

M0 exists purely as a reference: it shows what accuracy Stage 1 delivers with *no* refinement of any kind, so every other method's improvement can be measured against a clean baseline.

### 2.3 Stage-2 Facility Density Tiers

Short-range refinement anchors (the physical beacons Stage 2 ranges against) aren't spread uniformly across the whole 2,500 km² area — that would be absurdly expensive and doesn't match how real deployments work. Instead, anchors exist only at specific facility sites (warehouses, gates, checkpoints), modeled as a dense local cluster around each site. Three density tiers were tested to see how much this choice matters:

| Tier | Anchor density | Median distance to nearest anchor (when at a facility) |
|---|---|---|
| **D100** | 100 anchors/km² | ~50 m |
| **D50** | 50 anchors/km² | ~71 m |
| **D25** | 25 anchors/km² | ~100 m |

### 2.4 The Full Grid

3 topologies × 5 methods × 3 density tiers = 45 combinations (M0 is density-invariant since it never touches Stage 2, so in practice this is 3 M0 baselines + 36 genuinely distinct Method×Topology×Density cells).

---

## Part 3 — The Physics Underneath: How the Radio Environment Is Modeled

Every one of the tens of millions of simulated packets in this study passes through a shared physics engine that models real radio propagation phenomena. This section explains what each piece represents and how it's calculated.

### 3.1 Path Loss — How Signal Strength Fades With Distance

The foundational calculation everywhere in this system is the **log-distance path loss model**:

```
PathLoss(d) = PathLoss(1 meter) + 10 × n × log10(d)
```

`PathLoss(1 meter)` is the unavoidable free-space loss right next to the antenna — a physical constant that depends only on radio frequency, not on environment. For LoRa's ~868 MHz band, this comes out to **≈31.19 dB**, which matches the textbook Friis free-space formula almost exactly.

`n` is the **path loss exponent (PLE)** — how much *extra* loss accumulates per decade of distance because of ground reflections, clutter, and obstructions. This is the single most important environment-specific number in the whole model:

| Environment | PLE (n) | Meaning |
|---|---|---|
| Rural / Open Field | ~1.3–2.0 | Very little clutter, closer to ideal free-space |
| Rural Forested | 2.3 | Moderate loss from vegetation, the primary environment used for this study |
| Suburban | 2.75 | More structures and clutter |
| Urban Moderate / Urban Dense | 3.0–3.5+ | Significant building clutter |

A small change in `n` produces a *huge* change in usable range, because it's inside a logarithm multiplied by 10 — going from n=2.3 to n=3.5 at 10 km distance changes the path loss by over 45 dB, easily the difference between "link works fine" and "link never connects at all." Getting this one number right for the right environment turned out to matter enormously (see Part 6).

### 3.2 Shadowing and Fading — Modeling Randomness, Not Just Averages

Real radio links don't just follow the smooth average curve above — they fluctuate randomly around it. Two separate random effects are modeled:

- **Shadowing**: a slow-moving random offset (typically log-normal, with a standard deviation around 7–11 dB) representing large obstacles (a building, a hill) blocking part of the signal. This must be drawn freshly for each distinct link/scenario — a shadowing value that never varies is a modeling bug, not a feature (see Part 6, Bug #1).
- **Fading**: fast, small-scale signal fluctuation from multipath (the same signal arriving via multiple reflected paths and interfering with itself). Two statistical models are used depending on whether a direct line-of-sight (LOS) path exists:
  - **Rician fading** — used when there IS a dominant LOS path; the signal has a "floor" it rarely dips far below.
  - **Rayleigh fading** — used when there is NO dominant path (deep NLOS); much more volatile, with occasional deep fades.

### 3.3 NLOS (Non-Line-of-Sight) and Its Couplings

Whether a given link has a clear line of sight or not isn't just a label — it's coupled into several other calculations, because in reality NLOS conditions make *everything* worse simultaneously:
- NLOS increases shadowing's standard deviation by roughly 1.25–1.5×.
- NLOS forces the fading model from Rician (has a floor) to Rayleigh (no floor, more volatile).
- NLOS increases the timing spread of a received signal (delay spread), which matters for how precisely TDOA timing can be measured.

### 3.4 Interference

Beyond the desired signal's own noise and fading, other nearby radios can collide with it. This is modeled with a small population of synthetic "external interferer" devices, each with:
- A random position within a defined radius,
- A random duty cycle (what fraction of the time it's transmitting — typically very low, ~1% median, since real ad-hoc radio populations are mostly quiet),
- A stochastic "is it active right now" draw for every packet evaluated (this must be a genuine per-packet coin-flip, not a fixed always-on or always-off assumption — see Bug #7 in Part 6).

When an interferer happens to be active and its signal is strong enough to matter, it degrades the desired signal's effective SNR. A **capture effect** model is layered on top: even with an interferer present, a sufficiently *stronger* desired signal can still be correctly received — real receivers aren't destroyed by any interference, only by interference comparable to or stronger than the wanted signal.

### 3.5 Node Position Uncertainty (the GPS-Denied Modeling Piece)

One submodule specifically models the fact that a device may not know its own exact starting position with certainty (imagine dead-reckoning drift since its last confirmed fix). This is injected as a random radial offset with a configurable magnitude — this is the piece explicitly built to validate how the system behaves under varying degrees of "I'm not entirely sure where I am."

### 3.6 Other Submodules Modeled

The engine also includes: building entry/wall/floor attenuation (for indoor scenarios), body shadowing (a person's own body partially blocking their device's antenna), temporal blockage (a passing vehicle or object briefly obstructing a link), two-ray ground reflection (relevant for very low antenna heights over open, reflective ground), tunnel/soil attenuation for confined or underground scenarios, and imperfect spreading-factor orthogonality (crosstalk between simultaneous LoRa transmissions using different spreading factors). Not every submodule is active in every scenario — most only matter for the specific environments they describe (e.g., tunnel attenuation is irrelevant unless the environment is actually a tunnel).

---

## Part 4 — How Position Is Actually Computed

### 4.1 Stage 1: TDOA via Levenberg-Marquardt

For a device's Stage-1 fix to work at all, at least 3 gateways must directly receive its packet (fewer than 3 direct receptions makes 2D position mathematically underdetermined). This is called being **"TDOA-eligible."** A critical point (see Part 6, Bug #17) is that this eligibility check must count only **direct** radio links — a device whose packet is only relayed through mesh peers to reach a gateway cannot use that path for TDOA, because relaying introduces unpredictable, millisecond-scale processing delays at each hop that completely destroy the nanosecond-level timing precision TDOA needs. Whether a device can get a genuine TDOA fix is therefore a pure function of its own direct radio geometry — it does not depend on which network topology label is attached to the deployment.

Given ≥3 gateways with genuine timing measurements (each carrying its own random timing-jitter noise, calibrated below), the actual position is computed by feeding the measured arrival-time differences into a **Levenberg-Marquardt (LM) nonlinear least-squares solver** — the same general-purpose optimization technique widely used for exactly this kind of triangulation problem. The solver iterates from an initial guess (the centroid of the reached gateways) toward the position that best explains the observed timing differences. If the gateway geometry is poor (e.g., all gateways roughly in a line from the device's perspective), the solver can fail to converge to a sane answer — this is tracked explicitly as a **solver failure**, not silently absorbed into the result.

**The timing jitter itself** — how precisely a receiver can timestamp exactly when a signal arrived — was calibrated using the Cramér-Rao Lower Bound (CRLB), the standard theoretical floor for how well *any* estimator can perform given the signal's bandwidth and SNR:

```
sigma_t = 1 / (2*pi * beta_rms * sqrt(SNR))
beta_rms = Bandwidth / sqrt(12)   [for LoRa's 125 kHz channel: ~36,084 Hz]
```

At a realistic, moderate LoRa link SNR (~20 dB — a generous but entirely typical working condition for SF12 long-range links), this comes out to **σ_t ≈ 400 nanoseconds** per gateway. Converting that to distance via the speed of light, and combining it with realistic gateway geometry across this project's actual 10-gateway/2,500 km² layout, the resulting Stage-1 accuracy for devices that do get a valid fix lands at a median of **~220 meters**.

### 4.2 The Handoff Decision

After Stage 1 produces an estimate, two independent gates must both pass for Stage 2 to be attempted:
1. **Accuracy gate**: the Stage-1 estimated error must be ≤ 50 m.
2. **Availability gate**: a Stage-2 anchor (at the relevant density tier) must actually be within range.

If gate 1 fails, or gate 1 passes but gate 2 fails, the device stays on its Stage-1 (and possibly IMU-bridged) result. Only when both gates pass does genuine Stage-2 RF fusion occur.

### 4.3 Stage 2: Short-Range Refinement

**2.4 GHz RTT ranging** measures distance by timing a round-trip signal exchange and uses a continuous received-signal-strength (RSSI) threshold to decide success — accuracy degrades gradually with distance rather than cutting off sharply, out to an effective range of roughly 430 m.

**UWB ranging** uses ultra-wideband pulses, which give much finer timing resolution (hence better raw precision — noise around 5–10 cm baseline, growing slowly with distance) but has a harder physical range ceiling: **100 m in line-of-sight conditions, 30 m in non-line-of-sight**, with a realistically "fuzzy" (not perfectly sharp) edge to reflect that real signal fades near the range limit are themselves random, not deterministic.

Both technologies' success gates are pure **coverage** gates (is the link close enough / strong enough to connect at all), not precision gates — ranging *precision* only comes into play once a link has already succeeded.

### 4.4 IMU Fusion via Extended Kalman Filter (EKF)

When a device carries an IMU, its accelerometer and gyroscope readings are fused with whatever position signal is currently available through an EKF — a standard recursive estimator that combines a motion-based prediction (where the IMU says the device has moved to since the last fix) with a measurement update (whatever the current radio fix says), weighting each according to its known uncertainty. The IMU parameters used are standard, citable consumer-grade MEMS figures:
- Accelerometer noise density: **0.00294 m/s²/√Hz** (~300 micro-g/√Hz, a common consumer MEMS spec)
- Gyroscope bias: **5°/hour** (typical uncalibrated consumer-grade drift)
- Assumed pedestrian/asset movement speed: **1.4 m/s** (the standard reference walking speed used throughout pedestrian dead-reckoning literature)

Critically, IMU fusion helps in **two distinct situations**, which must be measured separately (see Part 5.1): it can smooth/improve a result when Stage 2 *does* succeed, and — more importantly for the final numbers — it can **salvage a usable position estimate even when Stage 2 fails entirely**, by bridging with dead-reckoning until the next fix.

---

## Part 5 — How Success Was Measured

### 5.1 The Three-State Framework

Every single simulated device-fix falls into exactly one of three states, and keeping these separate turned out to be essential (see Part 6, Bug #13):

| State | Condition | What position is reported |
|---|---|---|
| **A — LORA_ONLY** | Handoff never triggered (Stage-1 error > 50m, or not TDOA-eligible at all) | Raw Stage-1 result |
| **B — HANDOFF_FAILED (no IMU) / IMU_ONLY (with IMU)** | Handoff triggered, but Stage-2 RF link failed | Raw Stage-1 (no IMU) or IMU-bridged estimate (with IMU) |
| **C — FUSED** | Handoff triggered AND Stage-2 RF succeeded | Stage-2 (± IMU) refined result |

### 5.2 Metrics Used

- **TDOA Eligibility %** — fraction of devices with ≥3 direct gateway links (a pure geometry number, identical across topologies by the argument in 4.1).
- **Convergence %** — of eligible devices, what fraction the LM solver actually produces a sane answer for.
- **Handoff Success Rate** — `Count(State C) / Count(State B + State C)` — of devices that *attempted* Stage 2, what fraction actually connected. This must never be confused with "State B+C combined," which instead measures the (topology-dependent, method-independent) Stage-1 trigger rate — conflating the two was one of the most consequential bugs found in this project (Part 6, Bug #13).
- **Conditional Accuracy** — mean error among State C rows only (i.e., how good is Stage 2 when it works).
- **Population MAE (Mean Absolute Error)** — average error across the full relevant population (valid/converged rows), the metric that reflects real-world deployment impact.
- **Population Median / P90** — see the Invariance Theorem immediately below; these behave very differently from MAE at low handoff rates.
- **Solver Failure Rate** — fraction of attempted TDOA solves that fail to converge at all.

### 5.3 The Invariance Theorem

This is a genuinely important, provable mathematical property of this system, and understanding it prevents misreading the results:

> **If the fraction of devices affected by fusion (the handoff rate) is smaller than a given percentile's threshold (e.g., below 50% for the median, below 90% for the 90th percentile), and fusion only ever improves a device's error (never worsens it past its original value), then that percentile is *exactly*, mathematically unchanged by fusion — not "approximately similar," identical.**

Why: a percentile is just "whatever value sits at a specific rank in the sorted list." If only the smallest values in the list get changed (and they stay the smallest), the value sitting further up the list at the percentile's rank never moves. In this project's final results, handoff rates are consistently in the 1–2% range — far below both 50% and 90% — so both the population Median and P90 are pinned exactly to their Stage-1-only (M0) values across every method and every density tier. **MAE is therefore the only metric in this study that reflects Stage 2's population-level value** — the median/P90 columns are structurally incapable of showing it at these handoff rates, by design of the math, not by coincidence.

### 5.4 Statistical Rigor

Two additional disciplines were applied throughout: (1) **paired random draws** — the same underlying Stage-1 scenario is reused across every method being compared within a topology, so differences in outcome can be attributed to the method itself rather than to unrelated random noise between separate simulation runs; and (2) **10-fold cross-validation** for reporting confidence intervals on every metric, rather than single point estimates.

---

## Part 6 — The Validation Journey: Why These Numbers Can Be Trusted

Getting to a trustworthy final result required finding and fixing a long sequence of subtle bugs, each of which — if left in place — would have produced a confident-looking but wrong conclusion. This is condensed to the most consequential findings; each one materially changed a numeric result or a conclusion, not merely cosmetic.

| # | Issue found | Why it mattered | Resolution |
|---|---|---|---|
| 1 | Shadowing was a fixed constant per scenario instead of a fresh random draw | Made the whole simulated dataset deterministic where it should be stochastic — every downstream statistic built on it was meaningless | Made shadowing a genuine per-link random draw |
| 2 | Suburban path-loss exponent measured 2.12 in output vs. spec 2.75 | ~28 dB path-loss error at long range — enough to flip roughly half the dataset between "packet received" and "packet lost" | Root-caused and corrected the exponent application |
| 3 | Several submodules (body shadowing, temporal blockage, position uncertainty) computed a value but never added it to the final result | Real physical effects were silently invisible in every output dataset | Traced and fixed the missing wiring for each |
| 4 | Topology reachability rates were hardcoded guesses (`WAN 72%, MESH 31%, HYBRID 54%`) | The entire topology comparison was circular — assuming the answer, not measuring it | Replaced with a real Monte Carlo network simulation computing genuine per-node reachability |
| 5 | A test harness used the wrong environment (dense Urban instead of Rural Forested) when reproducing a baseline | Made an unrelated interference model look broken when the real cause was a copy-paste environment mismatch | Traced to the exact `ForScenario()` parameter mismatch and corrected it |
| 6 | Interference model: the final penalty formula mathematically couldn't depend on desired-signal strength at all (it canceled out algebraically); the "was this a collision" flag was also never set | Real capture-effect physics (a strong signal surviving interference) was structurally impossible to represent | Rewrote the penalty calculation and correctly wired the collision/capture flags |
| 7 | Stage-1 `hok` (handoff trigger) was a flat, topology-level coin flip, not tied to any individual device's real position | Broke the entire premise of training a model on geometry — and made "MESH is best" true by construction, regardless of any real physics | Rebuilt the pipeline so Stage-1 eligibility comes from real, per-device network simulation output |
| 8 | Stage-2 anchor distance was reusing the sparse, kilometers-apart Stage-1 gateway distance instead of a realistic short-range facility distance | Made Stage-2 handoff geometrically almost impossible to model correctly at any technology's real range | Introduced a proper, density-configurable local anchor-distance model, decoupled from Stage-1 |
| 9 | UWB's assumed range (150 m LOS / 50 m NLOS) was more optimistic than published commercial hardware specs | Would have overstated UWB's real-world coverage relative to 2.4 GHz | Corrected to citation-backed 100 m LOS / 30 m NLOS |
| 10 | A closed-form CRLB formula was proposed to replace the actual TDOA solver, but its underlying noise assumption didn't match the same solver's own already-validated reference numbers | Would have silently disconnected the Stage-1 error model from the very solver validation that established it in the first place | Used the real Levenberg-Marquardt solver with per-gateway timing draws directly, instead of an approximation |
| 11 | Percentile metrics (Median, P90) were computed only over the successfully-fused subset of rows | Produced mathematically impossible results (sub-meter P90 despite most devices failing to get any fusion at all) | Recomputed over the full population; this investigation is what established the Invariance Theorem in Part 5.3 |
| 12 | "Handoff Success Rate" for IMU-enabled methods was accidentally counting IMU-bridged fallback (State B) as if it were genuine Stage-2 RF success (State C) | Made 2.4 GHz and UWB look identical to each other whenever IMU was present, hiding the real technology difference | Split the metric cleanly into Handoff Success Rate (State C only) and IMU-Only Rate (State B) |
| 13 | A widely-used Stage-1 error column turned out to be dominated by an internal test-harness artifact (a deliberately cycled 4-level validation parameter), not genuine channel variability | The entire Stage-1 accuracy figure being used was an artifact of a submodule-testing script, not a representative field distribution | Switched the Stage-1 error source to the genuine, geometry-driven TDOA solver output |
| 14 | A simulation "fast mode" flag was silently zeroing out the realistic timing-jitter noise before it could be used | Made every TDOA solve artificially perfect (zero error), an impossible result | Traced to the exact code branch and corrected the fidelity setting |
| 15 | The timing-jitter magnitude (σ_t) was tuned specifically to reproduce an external reference number, rather than derived from the actual hardware/SNR physics | Would have silently hidden a real, important finding — that this project's much sparser gateway geometry genuinely produces worse raw TDOA precision than a denser reference deployment | Re-derived σ_t independently from CRLB theory; confirmed by two separate derivations converging on the same value |
| 16 | MESH's TDOA eligibility was computed by allowing devices to "borrow" reachability through 2-hop mesh relay paths | Physically impossible — relay delays destroy the nanosecond timing TDOA needs — and made MESH's Stage-1 accuracy look far better than physics allows | Restricted TDOA eligibility to direct radio links only, for every topology |

The cumulative effect of these fixes was substantial: early, buggy versions of this analysis reported MESH topology achieving 99.999% TDOA coverage and sub-meter population-wide accuracy improvements from Stage 2 — both were artifacts of the bugs above, not real findings. The final, validated numbers in Part 7 are considerably more modest, and considerably more trustworthy.

---

## Part 7 — Final Results

### 7.1 Stage-1 Baseline (M0)

All three topologies, once correctly restricted to direct-link-only TDOA, produce **statistically identical** Stage-1 performance — this is itself an important, physically-necessary finding, not a null result to be dismissed (see Part 8.1).

| Topology | TDOA Eligible | Converged (of all devices) | Converged Median Error | Population Median | Population P90 | MAE (converged rows) |
|---|---|---|---|---|---|---|
| WAN | 35.5% | 32.0% | 219.7 m | 9999 m* | 9999 m* | 551.7 m |
| MESH | 35.5% | 32.1% | 219.6 m | 9999 m* | 9999 m* | 553.3 m |
| HYBRID | 35.5% | 32.1% | 219.4 m | 9999 m* | 9999 m* | 552.2 m |

\* This is the correct, expected result, not an error: since fewer than half of all devices converge to a valid TDOA solution, the population's 50th and 90th percentile both land in the "no solution" category. Reading the Converged Median (rightmost accuracy figures) and MAE columns is the meaningful way to interpret Stage-1 performance for this deployment density.

### 7.2 Handoff Rates (% of all devices reaching genuine Stage-2 fusion)

| Density | RTT (M1/M2) | UWB (M3/M4) |
|---|---|---|
| D100 (densest) | ~1.7% | ~1.8% |
| D50 | ~1.5% | ~1.5% |
| D25 (sparsest) | ~1.4% | ~1.0% |

Rates are statistically indistinguishable across the three topologies (a direct consequence of Stage-1 being topology-independent, per 7.1). Within a given topology, D100 > D50 > D25 for every method, and IMU-enabled methods show identical rates to their non-IMU counterparts (M1=M2, M3=M4 exactly) — confirming IMU has no effect on whether the *radio link itself* connects, only on what happens afterward.

### 7.3 Population Accuracy Results (MAE, converged rows)

| Method | WAN | MESH | HYBRID |
|---|---|---|---|
| M0 (baseline) | 551.7 m | 553.3 m | 552.2 m |
| M1 (RTT) | 550.0 m | 551.7 m | 550.6 m |
| M2 (RTT + IMU) | 549.8 m | 551.4 m | 550.3 m |
| M3 (UWB) | 549.9 m | 551.5 m | 550.4 m |
| M4 (UWB + IMU) | 549.8 m | 551.4 m | 550.3 m |

Best case: **~2 m population-wide improvement (~0.4%)**, achieved by M2 or M4 at the D100 density, in any topology. This modest population-level number is the honest, mathematically consistent consequence of only ~1–2% of devices ever reaching Stage 2 — but for that specific 1–2%, the improvement is dramatic: from roughly 35 m (their individual Stage-1 error, since these are specifically the best-positioned devices who cleared the ≤50 m gate) down to sub-5-meter Stage-2-refined accuracy.

---

## Part 8 — Comparative Analysis: What Excels Where, and Why

### 8.1 Topology (WAN vs. MESH vs. HYBRID)

**Finding: topology has zero measurable effect on raw TDOA localization accuracy, by physical necessity — but MESH has a large, real, and separately-measured advantage in data delivery.**

TDOA fundamentally requires *direct* radio reception at multiple gateways simultaneously; a relayed packet cannot carry the nanosecond timing information TDOA needs, no matter how the network is architected. Since all three topologies share the identical physical device and gateway layout, "how well can this device's raw signal reach gateways directly" is a fixed geometric fact, unaffected by what we choose to call the network. This is why WAN, MESH, and HYBRID produce statistically identical Stage-1 and Stage-2 numbers throughout Part 7 — the correct, physically-forced result, not a limitation of the study.

MESH's genuine value lies elsewhere: an earlier stage of this project's network simulation (measuring, correctly, *data delivery* rather than TDOA timing) found that **~99.9% of MESH devices can get their data to some gateway within 2 relay hops, versus only ~35.5% of WAN devices reaching one directly.** In a real deployment, this matters enormously for whether a device's position report — however it was computed — ever reaches the operator at all. This report's accuracy comparison and that separate delivery-rate finding measure two different things and should be read together, not conflated.

### 8.2 Technology: 2.4 GHz RTT vs. UWB

**Finding: UWB wins in dense Stage-2 deployments, RTT wins in sparse ones, and the reason is entirely about coverage shape, not precision.**

- At **D100** (anchors ~50 m apart on average), UWB's 100 m LOS ceiling comfortably covers nearly all nearby devices, and its very hard cutoff rarely matters at this density — UWB shows slightly higher handoff rates than RTT here (1.8% vs. 1.7%).
- At **D25** (anchors ~100 m apart on average), UWB's hard ceiling starts cutting off a meaningful fraction of devices that are just beyond 100 m, while RTT's soft RSSI-based threshold still connects some of them at longer range with degraded (but usable) precision. RTT overtakes UWB here (1.4% vs. 1.0%).
- Once a link *does* connect, UWB's raw ranging precision is genuinely better (sub-decimeter-scale noise vs. tens-of-centimeters for RTT) — but this project's final, corrected understanding is that this precision advantage does not translate into a *coverage/success-rate* advantage; success is governed purely by whether a link connects at all (a coverage gate), and precision only matters for the resulting position quality once connected (see Part 6, Bug #12's correction).

**Recommendation implication:** the "right" technology depends on planned facility anchor density. Denser deployments favor UWB's precision without coverage penalty; sparser deployments favor RTT's graceful range degradation.

### 8.3 IMU: When It Helps, and How Much

**Finding: IMU does not change whether Stage-2 connects (confirmed exactly: M1=M2 and M3=M4 handoff rates), but it meaningfully improves outcomes for devices whose Stage-2 attempt fails.**

IMU fusion's value shows up specifically in **State B (handoff triggered, but the RF link failed)** — instead of falling back to the raw, unrefined Stage-1 estimate, an IMU-equipped device can bridge with dead-reckoning until its next real fix, producing a meaningfully better interim estimate than doing nothing. Because this project's overall handoff rates are low (~1–2%), and the RF-success/RF-failure split within that small population varies further by technology and density, IMU's population-level contribution to the headline MAE number is small in absolute terms — but it is real, consistent, and comes at essentially no cost to the RF-technology comparison, since it never trades off against RF success.

### 8.4 Density: The Dominant Secondary Lever

Facility anchor density (D100 > D50 > D25) is the single largest driver of how many devices ever reach Stage 2 at all — a bigger lever, within the Stage-2 layer, than the choice between RTT and UWB. This makes intuitive sense: no ranging technology, however precise, helps a device that never gets within range of an anchor in the first place.

### 8.5 The One Lever That Matters Most, Overall

**Gateway density (Stage 1) dominates everything downstream.** With only 10 gateways across 2,500 km², just 35.5% of devices are even TDOA-eligible, and only ~32% get a usable Stage-1 fix at all. Every subsequent number in this report — the handoff rate, the population MAE improvement, the value of any given technology choice — is bounded by how many devices clear the ≤50 m Stage-1 accuracy gate in the first place. Doubling or tripling gateway density would very plausibly move the needle on final system accuracy far more than any Stage-2 technology decision covered in this report.

---

## Part 9 — Overall Verdict

1. **Stage-1 gateway density is the primary architectural lever.** It determines the size of the population that can benefit from anything downstream, and this project's results consistently show it dwarfing the impact of any Stage-2 or IMU decision.
2. **Network topology (WAN/MESH/HYBRID) does not affect localization accuracy** — this is a mathematically necessary consequence of TDOA requiring direct links, confirmed by identical results across all three topologies. Topology choice should instead be driven by **data delivery requirements** — MESH's ~99.9% delivery rate vs. WAN's ~35.5% is the relevant comparison for that decision, not anything in this accuracy study.
3. **Between 2.4 GHz RTT and UWB, the right choice depends on planned Stage-2 anchor density**: UWB for dense facility deployments, RTT for sparse ones. Neither is a universal winner.
4. **IMU is worth including whenever cost/power budget allows.** It never hurts (it doesn't affect RF success), and it consistently helps recover otherwise-lost fixes, at a real but population-level-modest scale given this deployment's overall handoff rates.
5. **Stage 2, overall, delivers a genuine but modest population-wide accuracy gain (~2 m, ~0.4%) at this specific gateway density** — but a dramatic, sub-5-meter improvement for the specific ~1–2% of devices that reach it. Whether that trade-off is worth the hardware cost depends on whether the use case cares about population-average accuracy or about guaranteeing excellent accuracy for a meaningful minority.

---

## Part 10 — Limitations and Open Questions

- **Facility anchor placement was modeled statistically (Poisson-distributed), not against a specific real deployment plan.** Real facility locations, if known, would give a more precise answer than the density-tier sensitivity analysis used here.
- **The gateway density used throughout (10 gateways / 2,500 km²) is one specific, sparse assumption.** The finding that Stage-1 density dominates strongly suggests it would be worth re-running this entire study at 2–3 different gateway densities to map out the sensitivity curve, rather than treating this single density as universal.
- **Mobility and IMU drift were modeled with standard reference parameters**, not measured against this specific project's actual target hardware; if a specific IMU chip is chosen for production, its real datasheet parameters should replace the generic consumer-MEMS figures used here.
- **This study measures accuracy; it does not measure cost, power consumption, or deployment complexity** for any of the technology choices — those factors matter for a real procurement decision and are outside this report's scope.