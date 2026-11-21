# Plan: radio-only localization and rescue of 20–30 workers trapped after an industrial collapse

## Context
**This is an extension of the existing CA-TBL work, not a replacement.** Everything done so far stands and remains central to the paper, and the collapse study builds directly on it:
- the tri-band architecture;
- device-side estimation;
- uncertainty-triggered escalation;
- the energy model and duty-cycle compliance;
- re-acquisition, the mast-height and side-of-road fixes, and the search-party fixes;
- the realistic-site emergency and quiet-shift results (REALISTIC-1 to -6, PARETO-1/2);
- the pre-registration and gate discipline.

The collapse scenario reuses the same code paths (filter, particle search, mesh tables, gain-per-joule, arbiter, energy accounting). It is added as a new scenario mode and new default-off modules, so every earlier result reproduces byte for byte.

**In the paper,** the open-site lost-worker results and the collapse results are two evaluations of one system: one worker on a realistic site, and many workers in a collapse. The collapse part supplies the strongest informatics contributions; the earlier part supplies the architecture, energy and decision-rule evidence.

The collapse extension targets IEEE TII. A factory collapses; 20–30 workers wearing the tri-band devices are trapped; each must be located and dug out as fast as possible. The owner's direction:
- **workers' devices talk to each other**, so one contact with a rescuer can locate the whole connected group;
- **everything must be estimated from radio alone**. The system is a general implementation: no barometer, no building plans or floor geometry, no site-specific priors (no "side of the road", no last position before the collapse).

That is where the work should stand out: the three bands see the same rubble differently, and that difference can be used to estimate the rubble itself.

**Allowed inputs on the system side (owner decision): radio, IMU and GPS only.**
- **Radio:** 868 MHz signal strength and timing differences at stations; 2.4 GHz and UWB time-of-flight and signal strength; ranges between devices.
- **IMU:** accelerometer, gyro and magnetometer on every device.
  - Used for: static detection (slow cycle); the collapse instant as a sharp impact followed by stillness (generic, not site knowledge); dead reckoning of moving rescuers between fixes.
  - The worker's own radio+IMU track before the collapse is system-estimated, so its last fix may be used as a prior. It is an ablation switch: results are reported with and without it.
- **GPS:**
  - rescuers on the open rubble surface, for absolute coordinates and as a height reference across the rescuer frame;
  - buried devices are assumed to have no GPS.
  - The GPS accuracy model (horizontal and vertical) is taken from a cited source: the GPS Standard Positioning Service Performance Standard (current edition), extracted in Phase 1 with page and table. Until then, results are also given in the rescuers' local radio frame. The repo's existing rescuer GNSS model ("2 m per axis", DD 20) is re-sourced in Phase 1.
- **Not allowed:** barometer, building plans, floor heights, site-specific priors (e.g., "side of the road"), any World parameter.

## Step 0 after approval (before any implementation)
Write **only the reference rules** below into the owner's user-level rules file (read by every session), under a new heading "CA-TBL references and numbers". Then stop and wait for the owner's go. The full plan stays in this plan file and moves to `~/catbl_work/tii_paper/PLAN_COLLAPSE.md` at the go.

Text to add to the user-level rules file:
- Every numeric value used in code, scenarios, analysis or the paper must have a row in `docs/SOURCES_COLLAPSE.md` (and for earlier work, the existing design notes). A row holds:
  - an ID;
  - the value and unit;
  - what it is used for (which model, file and switch);
  - the full citation with DOI or URL and edition or revision;
  - the exact location (page, section, table, equation or figure);
  - a verbatim quote of the line it came from;
  - the calculation from the quoted value to the value used, with intermediate numbers;
  - the date checked.
- Values must be read from the primary document, not from summaries, search snippets or memory. If the primary is unreachable or lacks the number, say so in the row; the value is not used until sourced.
- Where sources disagree, keep low, central and high values, each from a named source, and test every conclusion across all three. Never tune a value to improve results.
- A scenario choice must also be backed by a source, or swept across the range a source reports.
- Every constant in new code carries a comment with its source-row ID. A check script compares row values with code constants.
- World/system split: the simulator's ground-truth parameters (rubble, geometry, materials) must never be read by system-side estimation code. The system uses only radio, IMU and GPS measurements and generic priors spanning published ranges.
- When a value or citation is used in the paper, cite the row's source with the same page or equation reference.

**Two layers, kept strictly apart:**
- **World (the simulator's ground truth):** rubble loss, signal slowing, burial depths, collapse layout, rescue capacity. Every number comes from a reviewed or official source with its location and calculation (Part A). Where sources disagree, low, central and high values each come from a named source, and every conclusion is tested across all three.
- **System (what the devices and rescuers know):** only their radio measurements (868 MHz signal strength, 2.4 GHz and UWB time-of-flight and signal strength, inter-device ranges) plus generic building-material physics, used as broad priors spanning the published ranges (Part B). No site knowledge. The system never sees the World's parameters.

**Rule:** every number in code has a row in `docs/SOURCES_COLLAPSE.md` (quote, page, calculation). Each mechanism is a default-off switch with a byte-identical off state, a gate with failing fixtures, and a design note (repository rules).

---

## Part A — World physics (sourced numbers, verified in planning)

### A1. Material loss: ITU-R Recommendation P.2040-4 (09/2025)
- **Equations:** (57) ε′ = a·f^b; (58) σ = c·f^d (f in GHz, σ in S/m); (59) ε″ = 17.98·σ/f (all p. 25). (26) A = 8.686/Δ dB/m; (27a) A = 1636·σ/√ε′, accurate to ~3% for tan δ < 0.5 (pp. 8–9).
- **Table 3 (p. 25), concrete:** a = 5.24, b = 0, c = 0.0462, d = 0.7822, valid 1–100 GHz.
- **Combined form used throughout:** A = 90.99·ε″·f/√ε′ dB/m.
- **Solid concrete:**
  - 2.4 GHz: σ = 0.0462·2.4^0.7822 = 0.09163 S/m → **A = 65.5 dB/m** (tan δ = 0.131);
  - 6.5 GHz: **142.8 dB/m**;
  - 8 GHz: **167.9 dB/m**;
  - 1 GHz: 33.0; 2 GHz: 56.8.

### A2. Rubble loss: Oestges, "Radio channel models for search-and-rescue missions into collapsed structures", URSI EMTS 2013, Hiroshima, pp. 1015–1018 (UCLouvain DIAL 2078.5/255172)
- **§II.B, measured permittivity:** concrete "9 – 0.4j and 9 – 0.5j at 1 and 2 GHz"; ferro-concrete "8.5 – 1.8j and 8.5 – 2.55j".
- **Converted with A1's form:**

| Material | 1 GHz | 2 GHz |
|---|---|---|
| Concrete | 12.1 dB/m | 30.3 dB/m |
| Ferro-concrete | 56.2 dB/m | 159.2 dB/m |

- **§III.B, layered rubble** (5 materials; layer thickness N(50, 20) cm or N(20, 5) cm):
  - "Without ferro-concrete … 8 and 20 dB/m at 1 and 2 GHz";
  - "When ferro-concrete layers are included … 14 and 31 dB/m";
  - spread of the integrated loss 2–10 / 2–14 dB;
  - measured validation, ~75% ferro: "simulated and measured attenuation at 2 GHz are 72 and 80 dB/m".
- **§IV.B, loose rubble:** "specific attenuation … fitted as α·η", η = filling factor, "α range from 35 to 45".

**Scaling to our bands** (power law between Oestges' 1 and 2 GHz points, k = log2(A₂/A₁)):
- Without ferro-concrete: k = log2(20/8) = 1.322.
- With ferro-concrete: k = log2(31/14) = 1.147.

**Brackets per metre of rubble:**

| Bracket | 868 MHz | 2.4 GHz | UWB 6.5–8 GHz |
|---|---|---|---|
| Low | 8·0.868^1.322 = **6.6 dB/m** | 20·1.2^1.322 = **25.4 dB/m** | ITU solid × η, with low η |
| Central | 14·0.868^1.147 = **11.9 dB/m** | 31·1.2^1.147 = **38.2 dB/m** | ITU solid × central η |
| High | 80·(0.868/2)^1.147 = **30.7 dB/m** | 80·1.2^1.147 = **98.6 dB/m** | ITU solid concrete 142.8–167.9 dB/m |

The extrapolations of 13% below 1 GHz and 20% above 2 GHz are stated as such in the paper.

- **Lower cross-check:** Nezirović, TU Delft PhD, §4.1.2: 60 cm reinforced wall "ca 14 dB at 2 GHz" (≈ 23 dB/m); sandstone rubble "rarely exceeds 10 dB". Reported as an extra low case.

### A3. Rubble density
- η = bulk density / solid density.
- Solid concrete 24 kN/m³ (EN 1991-1-1 Annex A, Table A.1; *Phase 1 confirm*) → 2447 kg/m³.
- Debris bulk densities to extract in Phase 1: 1.4 t/m³ (UN-based estimate; primary document to find) and 1.69–2.25 t/m³ (ScienceDirect S2949750726000088). This gives **η = 0.57 / 0.69 / 0.92**.
- **Consistency check:** α·η = 35·0.57 = **20** to 45·0.92 = **41 dB/m**, inside the 1–2 GHz brackets ✓.
- Survivable voids: pancake collapses have void ratios 0.91–0.99 (Geomatics, Nat. Hazards & Risk 2025, DOI 10.1080/19475705.2025.2555727; *Phase 1 confirm definition*).

### A4. Slowing (ranging bias) in the World
- b = (solid metres crossed)·(√ε′ − 1).
- Per metre of rubble, b = η·(√ε′ − 1): **low 0.57·1.289 = 0.73 m/m; central 0.69·1.915 = 1.32 m/m; high 0.92·2.000 = 1.84 m/m**.
- Cross-check, Kolakowski & Modelski, MIKON 2018 (DOI 10.23919/MIKON.2018.8405190), §II: two 10 cm walls give 0.86–1.33 ns from our formula vs "about 1 ns"; bound "usually would not exceed 2 ns" ✓.
- Direct path lost when the straight-path loss exceeds the receiver's dynamic range. Then the "direct path absent" error applies: "a ranging error of even a few meters" (Kolakowski), with Alsindi, Alavi & Pahlavan, IEEE TVT 58(3) 2009, Tables III–VI read from the rendered pages in Phase 1.

### A5. Validation targets the World must hit (the rubble gate, before any experiment)
1. **868 MHz totals** for 2–5 m burials span NIST's measured lower bounds:
   - "at least 30 to 40 dB" (TN 1542, convention centre, voids remained);
   - "at least 60 to 80 dB" (TN 1540, apartment building).
   - TN 1540 also gives the standing building's mean loss as 25–50 dB, standard deviation 6–14 dB.
2. **900 MHz floors:** 9/19/24 dB for 1/2/3 floors (ITU-R P.1238; current edition confirmed in Phase 1).
3. **2.45 GHz:** 60–100 dB in ruins (Bimpas et al., EuMC 2003, quoted by Oestges; primary extracted in Phase 1); central 38.2 dB/m × 2 m = 76 dB ✓.
4. **GSM-900 detection range** with 130 dB dynamic range (Oestges Table III): about 10 m (concrete + mixed) vs ours 130/11.9 = 10.9 m ✓. Ferro-concrete "< 2 m" vs our high bracket 4.2 m, a stated limitation of the high bracket at 868 MHz.

If a check fails, the bracket is corrected from the sources, never tuned to the results.

### A6. Scenario numbers (World only)
- **Rescue capacity** (INSARAG Guidelines, March 2011, p. 33, §7.2–7.3): heavy team "24 hour operations at 2 separate sites for up to 10 days", operational within 48 h; medium team 1 site, within 32 h. **Dig sites: 1/2/4.**
- **Survival over time** (De Bruycker et al., Bull. WHO 1983): 93.6% of live extrications within 24 h; 80% of all trapped extricated within 2 days; trapped mortality 35.0%. The full table is extracted in Phase 1 and the fit shown.
- **Extrication time:** Natural Hazards 2014 USAR study (DOI 10.1007/s11069-014-1304-3), Phase 1.
- **Collapse geometry and void types:** FEMA US&R / USACE Structures Specialist FOG; industrial case geometry from reviewed Rana Plaza sources (Barua, Safety Science 135, 2021), Phase 1.

### A7. Device hardware (World and energy)
- **nRF52840 PS v1.11:** 64 MHz; CoreMark 212; 64 CoreMark/mA at "DCDC 3V"; ICPU0 = 3.3 mA running CoreMark from flash with DC/DC; System ON sleep 1.5 µA; "Single-precision floating-point unit".
- **Radios:** SX1262 / SX1280 / DW3000 datasheets already in the repo.
- **Battery:** a manufacturer cell datasheet, Phase 1.

---

## Part B — The system: everything from radio

### B1. Radio-only rubble self-calibration (main new method)
On every link between a buried device and a node it ranges with, the system has up to:
- excess loss per band, E_b = measured path loss − free-space loss at the measured distance, for b ∈ {868, 2.4, UWB};
- time-of-flight distances from 2.4 GHz and UWB.

**Physics (same generic form as A1, with no site values):**
- E_b = A_b(ε′, ε″)·w
- r_ToF = d + w·(√ε′ − 1)

Here w is the solid material on the path, the unknown. **The ratio of excess losses across bands cancels w:** E_2.4/E_868 = A_2.4/A_868, which pins down the material's frequency behaviour. Then w = E_b/A_b, and the timing bias w·(√ε′ − 1) is removed from the distance with its uncertainty.

**Priors are generic, not site-specific:** ε′ between 5.24 (ITU) and 9 (Oestges); ε″ spanning Oestges' concrete to ferro-concrete values. All are posterior-estimated per link and shared across links of the same pile.

**Estimation:** per-link Bayesian update inside the existing filter. The corrected range enters with inflated variance, and links whose bands disagree beyond the model are down-weighted (the existing witness test and relay gating).

**Related work, which this extends:**
- joint ToA + RSS NLOS mitigation ("Target localization in NLOS environments using RSS and TOA");
- wall extra-delay models (arXiv 1203.2835 / 1203.2890);
- ray-tracing wall correction with known wall parameters (Plouhinec & Uguen);
- cooperative localization with unknown path-loss exponent or unknown range bias (Jin, TU Darmstadt thesis 2021; Li et al., arXiv 1904.00715; calibration-free ML, JWCN 2018, DOI 10.1186/s13638-018-1335-7).

**Novel:**
- the cross-band (sub-GHz + 2.4 GHz + UWB) physics link used to estimate rubble on each path;
- correcting timing bias with that estimate;
- doing it inside a buried cooperative network.

**Measured contribution:** distance and position error with vs without self-calibration, across the World's low, central and high brackets.

### B2. Depth from radio alone
- Positions are estimated in 3-D (x, y, z) for static trapped devices, reusing the particle search (`PfInit/PfUpdate`) and re-acquisition (DD 34).
- **Geometry requirement** (Outlier Detection and Optimal Anchor Placement for 3-D Underwater Optical Wireless Sensor Network Localization, arXiv 1810.03110): anchors all at the same height cannot resolve 3-D position "regardless of how many". So depth needs height spread among known positions. Rescuers' devices and dropped beacons sit at different heights on the irregular pile surface, or are held high and low.
- **Generic physical prior:** a buried device is below the rescuers. This removes the above/below mirror.
- **Second, independent depth cue from B1:** self-calibrated rubble thickness w on paths from several surface points. More material straight above means deeper burial (a radio "tomography" of the pile).
- **The theoretical limit is computed first:** a Cramér–Rao bound for depth vs number of contacts, rescuer height spread and ranging accuracy (bracketed). It tells which operational advice ("hold the device high and low") matters and how much.

### B3. Rescuers' own positions, from radio, IMU and GPS
- Rescuer devices range each other by UWB (sub-metre, A2/A4 physics in the open) and form a local 3-D frame.
- **GPS** places that frame in absolute coordinates and constrains its height. The accuracy model is from the GPS SPS Performance Standard (Phase 1). Vertical GPS error is larger than horizontal, so relative heights come mainly from inter-rescuer UWB ranges.
- **IMU** dead reckoning carries each rescuer between fixes (the existing step-and-heading filter).
- Results are reported both in absolute coordinates and in the rescuers' local frame.

### B4. Worker-to-worker network map
- Each device shares its measured neighbour ranges (with B1 corrections) over the existing mesh (`ca-tbl-mac.cc`). The group's positions are solved by cooperative localization (Patwari et al. 2005; Wymeersch, Lien & Win, Proc. IEEE 2009, SPAWN-type).
- **Honest limit** (Eren et al., INFOCOM 2004; Aspnes et al., TMC 2006): a unique map needs ≥ 3 well-spread known positions in 2-D, ≥ 4 non-coplanar ones in 3-D. A single rescuer supplies them over time by moving.
- **Rescuer movement and beacon drops chosen to maximise information gained** (D-optimal; Joshi & Boyd 2009; mobile anchors, Chen et al., IEEE TWC 2010), including vertical information.
- **Measured:** victims located only through other victims, and how long the map takes to pin down.

### B5. Many devices on air
- K-nearest relaying (the current schedule refuses crews above ~7).
- LoRa capture and collisions (Bor et al., MSWiM 2016).
- Slower cycle when the device's own motion sensor reports no movement (the device's own sensor, not site knowledge).
- Station broadcasts simulated against the 10% legal limit.

### B6. Precise-radio allocation and digging order
- The existing gain-per-joule (`CaTblTraceReduction` / `BenefitTierGain`) and arbiter (`ca-tbl-grant-coord.cc`) choose which victim to range.
- Digging cost = extrication time + search time proportional to the area of the 95% position region (radio uncertainty only).
- A "range more or dig now" decision.
- Digging order: nearest; best-located; severity-weighted completion time (Wex et al., EJOR 2014), weighted by the survival fit from A6.

---

## Phase 0 — Can workers compute on their own battery? (decision gate, 1–2 days)
- **Benchmark:** replay logged cycles through the filter, smoother, particle search and (once written) the B1 update. Measure host time; run CoreMark on the host.
- **Chip time:** t_nrf = t_host × CoreMark_host / 212.
- **Double-precision penalty:** k_dp = t_double / t_float (FPU is single-precision only).
- **Energy:** E_cpu = t_nrf·k_dp·3.3 mA·3.0 V. Budget: battery Wh / 72 h.
- **Decision rule:** on-worker if computing ≤ 10% of radio energy, ≤ 20% of the cycle time, and the total fits 72 h. Otherwise split (device filter on the worker; network map and particle search on rescuers). Rescuer-only if even the filter fails.

## Implementation order
1. `docs/SOURCES_COLLAPSE.md`: every row of Part A (quote, page, calculation), plus the Phase 1 extractions:
   - DiCarlofelice et al., IEEE TVT 2015 (868 MHz under L'Aquila rubble);
   - Bimpas 2003; NISTIR 6055;
   - EN 1991-1-1 Table A.1; debris densities;
   - ITU-R P.1238 current edition; Alsindi 2009 tables;
   - De Bruycker full table; extrication times;
   - FEMA void types; battery datasheet.
2. **World:** `lora-rubble-submodule` in all three engines (like the floor-attenuation and building-entry submodules). It covers per-cell material, η, A(f), bias and direct-path loss. **Rubble gate:** every number in A1–A4 as unit fixtures, plus the A5 validation targets and failing fixtures.
3. **World:** `--scenario=collapse` (voids, victims, dig sites, team arrival) with its gate.
4. **Theory:** Cramér–Rao bounds for depth and for the self-calibration (B1/B2), before building the estimators.
5. **System:**
   - B1 self-calibration (gate: on synthetic links with known w, the estimate recovers w and removes the bias within its stated uncertainty);
   - B2 3-D static estimator;
   - B3 rescuer frame.
6. Phase 0 decision.
7. B4 network map with planned rescuer moves (gate: synthetic networks with exact solutions; failing fixtures for coplanar anchors, a disconnected group, too few anchors).
8. B5, B6, then the pre-registered experiments.

## Experiments (pre-registered, new seeds)
- **COLLAPSE-1:** 20 and 30 victims; World brackets low/central/high; dig sites 2 and 4.
  - Designs: each radio alone, always-on, three-radio without self-calibration or cooperation, with self-calibration, with the network map, and full (with allocation and digging order).
  - 20 seeds.
- **COLLAPSE-2:** sensitivity; stations 0/2/4; dig sites 1/2/4; victims 10–30; rescuer height spread; field 2.4 GHz error (Mueller 2021).
- **COLLAPSE-3:** 72 h battery.

**Measures:**
- position and depth error;
- victims within 1 m and 2 m over time;
- all located;
- located only via other victims;
- reached alive;
- wasted digging;
- airtime;
- battery.

## Verification
- **Each switch:** off-state byte-identical; gate + self-test; full suite green apart from the recorded reds (G11, G24, G93).
- **World gates use published numbers:**
  - 65.5 dB/m concrete at 2.4 GHz;
  - Oestges-derived brackets;
  - 868 MHz totals inside the NIST bounds;
  - 9/19/24 dB floors;
  - bias 0.73/1.32/1.84 m per metre.
- **System gates use synthetic truth:** self-calibration recovers w; depth error matches the Cramér–Rao bound within tolerance; the network map is exact on noise-free graphs.
- **No-leak check:** a gate fails if any system-side code reads a World parameter (rubble bracket, η, ε, geometry).

## Fit for IEEE Transactions on Industrial Informatics (what reviewers will look for, and how this plan answers it)
1. **Industrial problem:** worker safety in industrial structural collapse (factories; the Rana Plaza case). The system is an IIoT network of worn devices, stations and rescuer devices.
2. **Informatics contribution, not only radio:**
   - cross-band self-calibration (estimation);
   - cooperative network inference;
   - information-driven rescuer movement and beacon placement;
   - allocation of scarce precise ranging;
   - uncertainty-aware rescue scheduling.
   All are decision layers on top of measurements.
3. **Theory:** Cramér–Rao bounds for depth and for self-calibration; identifiability conditions (rigidity; anchor height spread); computational complexity of each algorithm, and on-device cost (Phase 0).
4. **Comparison with the state of the art,** implemented on the same simulator:
   - at least one published cooperative estimator: a SPAWN-type belief propagation (Wymeersch 2009) and an unknown-path-loss RSS cooperative method (Jin 2021 / Li et al. 1904.00715);
   - a single-band baseline: the LoRaAid-style RSS trilateration and the smartphone-BLE-style approach (Tai & Yu 2022);
   - plus each radio alone and always-on.
5. **Practical constraints:** 72 h energy, legal duty-cycle limits (G.S.R. 853(E)), scaling to 20–30 devices (and 10/15 crews), computation on a Cortex-M4F-class chip.
6. **Rigour:** every physical number sourced with location and calculation; World/system separation with a no-leak gate; pre-registered campaigns; paired statistics; bracketed physics.
7. **Validation:** own SX1262/SX1280 through-concrete measurements (Phase 4) strongly recommended. Without them the paper is competitive but at risk at TII; with them it is a strong TII submission.
8. **Length:** 10 pages at submission. The paper focuses on B1, B2, B4 and B6 with the collapse experiments; the energy and Phase 0 analysis are condensed.

## Novelty
**Gaps found open in the sweep:**
- 72 h energy-aware multi-band victim devices;
- anchoring a victim network from sparse rescuer contacts with planned moves;
- uncertainty-driven digging order;
- duty-cycle limits with many victims.

**New in this revision:**
- **cross-band rubble self-calibration from radio alone (B1);**
- **radio-only depth** from height spread plus the rubble-thickness cue (B2).

**Closest work:** Li, Mukhopadhyay & Alouini, arXiv 2504.10666 (2025); LoRaAid (2023); Sahin & Arslan, ICC 2024; Tai & Yu, Sensors 2022; FTM buried-victim study, IEEE Access 2025; Jin, TU Darmstadt 2021; Wex 2014; Chen & Miller-Hooks, TR-B 2012.

## Risks
- **Self-calibration may be weakly identifiable** when only one band survives the rubble. Then it falls back to bracket-wide uncertainty, and that is reported.
- **Depth from radio may stay coarse** with small height spread. The Cramér–Rao step shows this before experiments.
- **World measurements vary up to ~7×,** hence the brackets.
- **Simulation only;** an own SX1262/SX1280 through-concrete measurement would be the strongest addition for TII.
- **Effort:** about 3–4 weeks.
- **Earlier results stay as they are:** the extension's switches default to off, so every earlier campaign and gate reproduces unchanged. The full gate suite is run after each module to prove it.
