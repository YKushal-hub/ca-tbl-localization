# CA-TBL — context pack

Parameter provenance, prior-art positioning, and citation rules, in one place so nothing gets
invented. Every entry here is either cited to a real source or explicitly labelled an engineering
choice. **There is no third category.**

Consolidated from `Architecture.md` §13 and the thesis reference registers.

---

## 1. Citation rules — binding

- **Never invent** a reference, author, venue, year, DOI, ISBN, URL, dataset, benchmark value,
  or equation attribution. If supporting literature is unavailable, **say so** rather than
  producing a citation.
- Preferred venues: IEEE, ACM, Springer, Elsevier, MDPI, Nature, ScienceDirect, arXiv where
  appropriate, government publications, official standards, official vendor documentation.
- **Never cite:** blogs, Medium, Quora, StackOverflow, marketing pages, AI-generated sources.
  One ResearchGate URL in the thesis pool must be replaced with the publisher record or dropped.
- IEEE numeric style, consistently, in both the text and the bibliography.
- **Every technical claim carries `[n]` or is explicitly labelled an engineering assumption.**
- In source comments: where a constant is ours rather than cited, the comment must say so. This
  is enforced as a documentation discipline so a reader can separate *measured/cited* from
  *our choice*.

---

## 2. Parameter and model sources — values actually set in the simulator

These set numbers in the code. Do not change a value here without changing its citation.

| Tag | Source | Sets |
|---|---|---|
| R-01 | Zhou et al., *arXiv:2509.23125* (2025) — SX1280 RF time-of-flight, open field, ~1000 samples × 9 points | `tofLosFloorM` = **3.0 m** (tier 2 ranging floor) |
| R-02 | Benouakta et al., *Electronics* (MDPI), vol. 12, no. 21, art. 4487, 2023 — UWB-LoRa transceiver, 10–33 cm, DW3000-class | `uwbLosFloorM` = **0.10 m** (tier 3 ranging floor) |
| R-03 | Qorvo, *DW3000 Family User Manual* | 1024 preamble symbols → **30.1 dB** processing gain |
| R-04 | **Gazette of India, G.S.R. 853(E), 10 Dec 2021, Table-II** | Duty cycle **2.5 %** field devices / **10 %** network access points; 500 mW e.r.p.; APC mandatory; 865–868 MHz |
| R-05 | Semtech, LoRa time-on-air formula (datasheet / AN1200.13) | ToA for both LoRa tiers; low-data-rate optimisation auto-enabled when `T_sym` > 16 ms |
| R-06 | Semtech, *SX1262 datasheet* Rev 2.1, Power Consumption table | Tier-1 currents: TX 45 mA @ +14 dBm, RX 5.3 mA, idle 1.5 mA, sleep 1.2 µA |
| R-07 | Semtech, *SX1280 datasheet* Rev 3.2 | Tier-2 currents: TX 24 mA @ +12.5 dBm, RX 7.0 mA, idle 1.4 mA, sleep 1.2 µA |
| R-08 | Qorvo, *DW3000 datasheet / user manual* | Tier-3 currents: TX 82 mA, **RX 94 mA**, idle 12 mA (PLL), sleep 0.7 µA |
| R-09 | IEEE 802.15.4z HRP UWB PHY | UWB frame timing: SHR, SFD, PHR, payload |
| R-10 | Cramér–Rao Lower Bound, standard estimation theory | Ranging-estimator validation target. **Applies to tiers 2 and 3 only** — see §5 |
| R-11 | Semtech LR1121 multi-band transceiver | Dual-band single-die feasibility. Supports **"one board, two radio ICs"**, never "one chip" |
| R-12 | IEEE 802.11e / WMM access categories | Priority-differentiated contention windows — **cited as adapted, not invented** |
| R-13 | Gudmundson, correlated shadowing model | Spatially-correlated shadowing submodule. **Limitation:** the source states the fit is good for large-to-moderate cells and degrades for microcells — state this wherever it is applied to confined, indoor or tunnel profiles |
| R-14 | Julier & Uhlmann, Covariance Intersection | Conservative fusion under unknown cross-correlation. **Specified, not implemented** |
| R-15 | Perkins & Bhagwat, DSDV | Sequence-number generational supersession. **Specified, not implemented** |
| R-16 | AODV / DSR reactive routing | Reverse-path caching for query responses. **Specified, not implemented** |
| R-17 | Meshtastic / flooding-protocol literature | Packet-ID dedup, hop limit, rebroadcast jitter |

**The DW3000 RX > TX asymmetry (94 mA vs 82 mA) is the single most consequential number in the
energy model.** A UWB exchange is dominated by *listening*. The lever that saves UWB energy is
therefore reducing the **number of activations** — which is exactly what the decision layer
controls. Any change touching UWB energy must preserve this.

---

## 3. Prior art and positioning

| Tag | Work | Relationship |
|---|---|---|
| R-18 | **POUCET** — Vey, Spies, Pestourie, Genon-Catalot, Van den Bossche, Val, Dalce, Schrive, *IPIN 2021*, doi:10.1109/IPIN51156.2021.9662587 | **Closest prior art.** GPS + LoRa 868 + LoRa 2.4 + UWB for firefighters and soldiers, validated by French National Defense. Establishes the radio combination works in the field. Uses dropped-beacon infrastructure; no mesh, no hop-aware confidence, no trust layer on the switching decision, no identity-scoped search. Ranging frame structures adopted from it |
| R-19 | **LoRaWAPS** — Li, Xu, Liu, Shi, *Applied Sciences*, 2023, 14.49 m mean error | Establishes mesh LoRa localization as prior art. Single-hop-to-anchor confidence model; no precision tier |
| R-20 | **LIMA** (MDPI) — LoRaWAN-compliant mesh routing, "star-of-meshes" | Mesh/WAN hybrid prior art; coverage and energy focus, not localization accuracy |
| R-21 | **Bianco et al.**, *IEEE IoT Journal*, 2021 — LoRa for SAR, mountain path-loss models | Closest **domain** precedent. Single-hop, path-loss only, positioned against ARVA/RECCO — the real-world SAR baseline |
| R-22 | **Huang, Yu, Anderson**, *IEEE Trans. Ind. Electron.*, 2013 — error propagation in multi-hop sensor network localization | Multi-hop error propagation is a mature field. Cited as foundation, **not claimed** |
| R-23 | DV-Hop algorithm family (Springer) | Hop-count-dependent error correction; prior art for hop-aware confidence |
| R-24 | SeRLoc (2004), HiRLoc (2006), distance bounding (1993) | Byzantine-robust localization as a 20-year field. Cited so the trust layer is not presented as novel in kind |
| R-25 | To & Duda, "Simulation of LoRa in NS-3: Improving LoRa Performance with CSMA", *IEEE ICC 2018* | ns-3 LoRa simulation precedent |

### Late-surfacing prior art — must be positioned, not ignored

Found in a database verification pass after the original literature register was written. None
invalidates a claim; each changes how one must be argued.

| ID | Work | Consequence |
|---|---|---|
| **NOV-01** | Huang et al. 2013 (IEEE TIE), Shi et al. 2005 (IEEE JSAC), Savvides et al. 2005 (IEEE TMC) | Between them these establish, analytically and against the CRLB, that localization error grows with hop count, that a fundamental accuracy bound is set by hop count and reference-link count, and that error trends against density and beacon uncertainty are characterised algorithm-independently. **Do not argue that hop-count degradation is uncharacterised.** The defensible contribution is *composing* that degradation into a single transported confidence scalar gating a radio-tier decision |
| **NOV-02** | `albinsaid2025design`, IEEE IoT-J 2025 — SX1280 system-level coordination, sleep–wake scheduling, initiator/responder role assignment, 5 m accuracy, nine-month coin-cell standby | Close prior art on the **tier-2 resource-coordination axis**. Its contribution is energy coordination, not confidence-driven escalation, but the adjacency is close enough that omitting it is a review risk |
| **NOV-03** | `muller2025nanosecond`, Sensors 2025 — GNSS-free time sync over LoRa 2.4 GHz, 30 ps TDEV, 3 ns daily max, explicitly motivated by GNSS-denied localization | A credible GNSS-free alternative to GPS-PPS anchor synchronisation now exists. Relevant only if the TDoA path is revived — see the decision note in §5 |
| **NOV-04** | `muppala2021feasibility`, ICL-GNSS 2021 — standalone GNSS-free TDoA over LoRaWAN using an added stationary sync node, with timing-resolution, time-on-air and duty-cycle constraint analysis | Nearest prior art to the tier-1 TDoA design **including its regulatory constraint analysis** |

### The honest positioning statement — reproduce this, do not soften it

Not novel, and each is cited above: the LoRa + UWB radio combination (R-18); mesh LoRa
localization (R-19); threshold-based band switching (commercial dual-radio systems and
duty-cycling literature); multi-hop uncertainty propagation (R-22, R-23); Byzantine-robust
localization (R-24).

**What this work contributes** is the assembly and its quantification: a self-deployable tiered
topology with a coverage-and-quality-gated escalation decision, evaluated causally with
per-action energy accounting across a dimensionless density sweep, with the operating window
measured rather than asserted — plus the measurement apparatus itself.

---

## 4. Provenance defects — open, must not propagate

| ID | Defect | Required handling |
|---|---|---|
| **PROV-01** | `RURAL_OPEN` (n = 2.2) and `SUBURBAN` (n = 2.75) were attributed to `petajajarvi2015coverage`, which reports an **on-water exponent of 1.76** and supports neither value. Rademacher 2021 quotes the Oulu fit as n = 2.65, PL(d0) = 132.25 dB | **RELABELLED OURS, 2026-10-08** (`lora-physics-parameters.cc:21`): no source for 2.2 / 2.75 was found. They are engineering assumptions. Never cite Petäjäjärvi for them (TECH_DEBT N.42) |
| **PROV-02** | "112,404 samples" and "combined slope n ≈ 1.58" attributed to `rademacher2021pathloss` | **CONFIRMED from the PDF (arXiv 2109.07768v1), 2026-10-08.** §IV: n = 1.58, PL1km = 132.41 dB, shadowing σ = 9.9 dB. The counts differ inside the paper: 175,492 samples obtained, 112,372 after post-processing (§III), and 112,404 in Table I. Cite 112,372 with the section. The fit is **single-slope, urban (Bonn)**. The engine uses its n as the FAR slope beyond 1 km of a dual-slope model, so write "adapted from", never "from" (`lora-physics-parameters.cc:19`) |
| **PROV-03** | Radio-part basis. Specification selects **LR1121** (868 + 2.4 GHz) + DW3000-class; the energy model uses **SX1262 + SX1280 + DW3000** currents | State as a conformance delta. Not a physics error — the parts are close — but it must not be silently inherited |
| **PROV-04** | Environment profiles calibrated against German, Finnish and Brazilian campaigns; deployment context is Indian regulatory and terrain | Record as a threat to validity. No fix required, but must be stated |
| **PROV-05** | `lira2026indoor` recorded with a 2026 date and a 433 MHz measurement frequency against an 868 MHz deployment | Verify the year is not a transposition; state the frequency mismatch if retained |
| **PROV-06** | `decawave2016aps006` documents **DW1000**; the design uses DW3000-class | State the device difference at the point of citation |
| **PROV-07** | Gudmundson correlation applied across all nine environment profiles, but the source's fit degrades for microcells | State the applicability limitation wherever it is applied to confined, indoor or tunnel profiles |

**One credential in the tree, deliberately untouched.** `utils/coverity-report.sh:66` contains
`--form password=4jk2BVX9`. This is **upstream ns-3 code**, already public in every ns-3 release,
and is not a secret of this project. Rewriting history over an upstream file to remove something
that is not confidential would be a large irreversible operation for no gain.

---

## 5. Specification-versus-implementation divergences

Anything written about the system must respect these or it will describe a system that does not
exist. Full analysis in `MASTER_PLAN.md` §2.3.

| Item | Specified | Implemented |
|---|---|---|
| Tier-1 observable | TDoA multilateration (Chan, Foy) | **RSSI inversion.** No timing layer, no sync, no TDoA solver. This is why the CRLB gate does not apply to tier 1 (R-10) and why NOV-03/NOV-04 are currently moot |
| Estimation location | Anchor/Gateway Node | **Gateways, by default since 2026-09-29** (`--gatewayEdgeServer`): the tag is billed one tier-1 broadcast and one downlink per fix. Before that, and under `--gatewayEdgeServer=false`, the field tag ran the EKF, PCS, gates and FSM. Getting a tier-2 or tier-3 range to the gateway's estimator is not billed (`docs/TECH_DEBT.md` L.2.3) |
| Predict-step prior | Inertial dead-reckoning (IMU) | **Constant velocity, 4-state.** No IMU anywhere. Consequence: the integrity check's "independent prior" is propagated from previous radio fixes, so it is not independent in the sense the specification requires |
| PCS composition | Weighted sum, weights by LOOCV | **Unweighted sum** of three [0,1] terms (legacy), or `log10(σ_ref/σ_eff)` (v2). The missing weighting is the direct cause of the saturation defect |
| Fusion | Covariance Intersection + sequence supersession | Neither implemented |
| Network layer | Mesh routing, confidence-weighted paths, identity-scoped query, arbitration | None implemented. `hopWeight = 0`; the MAC rejects hopped frames by design |

**Naming.** The system is **CA-TBL** (Confidence-Aware Tier-Based Localization). The thesis
registers still say CA-DBL; *Dual-Band* is factually wrong for a three-tier system. The repo is
correct; the thesis side is being corrected under `TERM-CHANGE-01`.

---

## 6. Measured values — cite these, do not re-derive or estimate

| Quantity | Value | Source |
|---|---|---|
| Tier-1 `r80` | 1515.3 m | probe sweep, `Architecture.md` §2.1 |
| Tier-2 `r80` | **1149.5 m** (was 909.5 m before the sensitivity fix) | probe sweep |
| Tier-3 `r80` | 12.8 m — **note:** the technical report §2 says ≈ 13.5 m; the documents disagree and one is stale | probe sweep |
| Tier-2 CRLB ratio | 1.105 at 1600 m | CRLB gate |
| UWB ranging precision | median 0.154 m over 142 sustained-coverage cycles, stable to 6 % across 3 realisations | sustained-coverage analysis |
| Shadowing decorrelation | 0.90 against Gudmundson | validation gate |
| Tier-1 ranging ToA | 56.58 ms (SF7, 125 kHz, CR4/5, 20 B) — matches computed ToA to 5 s.f. | `airtime_868 / steps` |
| Escalation handshake ToA | 41.22 ms (12 B) | same formula |
| Off-time armed by one tier-1 frame at 2.5 % | 2.263 s against a 2.5 s cycle | R-04 + R-05 |
| Measured duty cycle, tier 1 | 0.0226 against the 0.025 cap | `arm_summary.csv` |
| MAC contention | 200 000 TX / 40 000 collisions / **14 564 captures** | reference run |
| Energy reconciliation | `rx_uwb` 630.315 J model vs 629.71 J analytic — 10 s.f. | R-06/07/08 |
| Coverage fraction | 11.49 % at b1000, 3.39 % at b3000, independent of gateway count | trace analysis |

**Sixteen parameters carry individual robustness verdicts** from the OFAT campaign. Two (P12,
P16) are recorded as **analytic**, so their ROBUST classification must not be over-read.

---

## 7. Two open residuals, deliberately unclosed

- **UWB ~3.7 dB unattributed**, eight candidates ruled out with numbers, saturating by 20 m,
  inside the region where terrain is provably inactive.
- **868 MHz ~3.5 dB near-field**, six candidates ruled out, shrinking monotonically as
  `nlos_frac` rises — the signature of two opposite-signed errors partially cancelling.

They stay open deliberately. The campaign's robustness rule is how conclusions were drawn without
closing them: **every conclusion was recomputed under ±4 dB of margin offset chosen to bracket
exactly these residuals.** Any new conclusion must do the same.
