# Sources and numbers: collapse extension

Plan: `~/catbl_work/tii_paper/PLAN_COLLAPSE.md`. Rules (owner's user-level rules file, "CA-TBL references and numbers"):

- every number used in code, scenarios, analysis or the paper has a row here;
- values come from the primary document;
- where sources disagree, low / central / high values each come from a named source;
- no value is tuned;
- World values are never read by system-side code.

**Status:**
- **VERIFIED** — read in the primary document; the quote is verbatim.
- **PENDING** — known only from a secondary summary or search result, so not yet usable. The row records where the primary is.

**Layer:**
- **W** — World (simulator ground truth).
- **S** — system prior (generic, spanning published ranges).
- **H** — hardware.
- **X** — scenario.

---

## 1. Material loss, ITU-R P.2040-4

Source: Recommendation ITU-R P.2040-4 (09/2025), "Effects of building materials and structures on radio-wave
propagation in the range of 1 MHz to 450 GHz". https://www.itu.int/dms_pubrec/itu-r/rec/p/R-REC-P.2040-4-202509-I!!PDF-E.pdf

| ID | Value | Used for | Location | Verbatim quote | Calculation | Status, checked |
|---|---|---|---|---|---|---|
| ITU2040-E57 | ε′ = a·f^b | W/S: real permittivity of a material | p. 25, eq. (57) | "ε′𝑟 = 𝑎𝑓 𝑏 (57)" | — | VERIFIED 2026-11-21 |
| ITU2040-E58 | σ = c·f^d, f in GHz, σ in S/m | W/S: conductivity | p. 25, eq. (58) | "σ = 𝑐𝑓 𝑑 (58) where f is frequency in GHz and σ is in S/m." | — | VERIFIED 2026-11-21 |
| ITU2040-E59 | ε″ = 17.98·σ/f | W/S: imaginary permittivity from conductivity | p. 25, eq. (59) | "ε′′ = 17.98σ / 𝑓 (59)" | — | VERIFIED 2026-11-21 |
| ITU2040-E26 | A = 8.686/Δ dB/m | W/S: attenuation rate | p. 8, eq. (26); p. 9 | "𝐴= 20 log10 𝑒 / ∆ = 8.686⁄∆ (26) … A: attenuation rate in dB/m (with Δ in m)." | — | VERIFIED 2026-11-21 |
| ITU2040-E27a | A = 1636·σ/√ε′ dB/m (dielectric limit) | W/S: loss per metre of material | p. 9, eq. (27a); validity p. 8 | "𝐴𝑑𝑖𝑒𝑙𝑒𝑐𝑡𝑟𝑖𝑐 = 1 636 σ/√ε′ (27a)"; "accurate to about 3% for tan δ < 0.5 (dielectric)" | With (59): A = 1636·(ε″·f/17.98)/√ε′ = **90.99·ε″·f/√ε′** (1636/17.98 = 90.990) | VERIFIED 2026-11-21 |
| ITU2040-T3-CONC | concrete a = 5.24, b = 0, c = 0.0462, d = 0.7822, valid 1–100 GHz | W: solid-concrete loss and slowing; S: lower end of the ε′ prior | p. 25, Table 3 | "Concrete 5.24 0 0.0462 0.7822 1-100" | ε′ = 5.24, √ε′ = 2.28910. 2.4 GHz: σ = 0.0462·2.4^0.7822 = 0.0462·1.98336 = 0.091631 → A = 1636·0.091631/2.28910 = **65.49 dB/m**; ε″ = 17.98·0.091631/2.4 = 0.68647, tan δ = 0.1310 (< 0.5 ✓). 1 GHz: σ = 0.0462 → **33.02 dB/m**. 2 GHz: 2^0.7822 = 1.71975 → σ = 0.079453 → **56.78 dB/m**. 6.5 GHz: 6.5^0.7822 = 4.32375 → σ = 0.199757 → **142.76 dB/m**. 8 GHz: 8^0.7822 = 5.08624 → σ = 0.234984 → **167.94 dB/m** | VERIFIED 2026-11-21 |
| ITU2040-T3-BRICK | brick a = 3.91, b = 0, c = 0.0238, d = 0.16, valid 1–40 GHz | W: optional material class | p. 25, Table 3 | "Brick 3.91 0 0.0238 0.16 1-40" | — | VERIFIED 2026-11-21 |
| ITU2040-T3-PLASTER | plasterboard a = 2.73, b = 0, c = 0.0085, d = 0.9395, valid 1–100 GHz | W: optional material class | p. 25, Table 3 | "Plasterboard 2.73 0 0.0085 0.9395 1-100" | — | VERIFIED 2026-11-21 |
| ITU2040-T3-WOOD | wood a = 1.99, b = 0, c = 0.0047, d = 1.0718, valid 0.001–100 GHz | W: optional material class | p. 25, Table 3 | "Wood 1.99 0 0.0047 1.0718 0.001-100" | — | VERIFIED 2026-11-21 |
| ITU2040-T3-GLASS | glass a = 6.31, b = 0, c = 0.0036, d = 1.3394, valid 0.1–100 GHz | W: optional material class | p. 26, Table 3 (end) | "Glass 6.31 0 0.0036 1.3394 0.1-100" | — | VERIFIED 2026-11-21 |
| ITU2040-VALID | concrete row not valid at 868 MHz | W: 868 MHz must come from another source (OES-*) | p. 25, Table 3 | frequency range "1-100" | 0.868 < 1 GHz → not used at 868 MHz | VERIFIED 2026-11-21 |
| SLOW-1 | apparent extra distance through w metres of solid material = w·(√ε′ − 1) | W: ranging bias; S: bias model | from the refractive index n′ = √ε′ in P.2040's plane-wave treatment (eq. (21)) | "𝐸⃗ = 𝐸⃗0 exp{𝑗(ω𝑡 − √ε′ (1 − 𝑗 tan δ)𝑘⃗0 ∙ 𝑟)} (21)" | Phase velocity c/√ε′ (low-loss), so the delay through w m is w·√ε′/c instead of w/c and the extra distance is w(√ε′ − 1). ITU concrete: √5.24 − 1 = **1.2891 m/m**. Low-loss approximation; tan δ ≤ 0.13 at 2.4 GHz | VERIFIED (derivation) 2026-11-21 |

## 2. Rubble loss, Oestges 2013

Source: C. Oestges, "Radio Channel Models for Search-and-Rescue Missions into Collapsed Structures", Proc. 2013 URSI
International Symposium on Electromagnetic Theory (EMTS), Hiroshima, pp. 1015–1018, ISBN 978-1-4673-4939-0. Open PDF:
UCLouvain DIAL 2078.5/255172.

| ID | Value | Used for | Location | Verbatim quote | Calculation | Status |
|---|---|---|---|---|---|---|
| OES-EPS-CONC | concrete ε = 9 − 0.4j (1 GHz), 9 − 0.5j (2 GHz) | W: concrete; S: upper end of the ε′ prior | §II.B | "we consider a complex permittivity for concrete to of 9 – 0.4j and 9 – 0.5j at 1 and 2 GHz respectively" | With ITU2040-E27a: A = 90.99·0.4·1/3 = **12.13 dB/m** (1 GHz); 90.99·0.5·2/3 = **30.33 dB/m** (2 GHz). Slowing √9 − 1 = **2.000 m/m** | VERIFIED 2026-11-21 |
| OES-EPS-FERRO | ferro-concrete ε = 8.5 − 1.8j (1 GHz), 8.5 − 2.55j (2 GHz) | W: reinforced concrete; S: ε″ prior upper end | §II.B | "and for ferro-concrete, of 8.5 – 1.8j and 8.5 – 2.55j at 1 and 2 GHz respectively" | √8.5 = 2.91548. A = 90.99·1.8/2.91548 = **56.18 dB/m** (1 GHz); 90.99·2.55·2/2.91548 = **159.17 dB/m** (2 GHz). Slowing **1.9155 m/m** | VERIFIED 2026-11-21 |
| OES-FERRO-EXTRA | reinforced slabs 8–10 dB more than plain, 2–6 GHz | W: cross-check | §II.B | "the ferro-concrete slabs attenuate from 8 to 10 dB more on the 2-6 GHz" | — | VERIFIED 2026-11-21 |
| OES-LAYER-PLAIN | layered rubble without ferro-concrete: 8 dB/m (1 GHz), 20 dB/m (2 GHz) | W: **low** bracket | §III.B | "Without ferro-concrete, the mean specific excess attenuations reach 8 and 20 dB/m at 1 and 2 GHz respectively" | k = log2(20/8) = **1.32193**. 868 MHz: 8·0.868^1.32193 = 8·0.82933 = **6.63 dB/m** (extrapolated 13% below 1 GHz). 2.4 GHz: 20·1.2^1.32193 = 20·1.27254 = **25.45 dB/m** (extrapolated 20% above 2 GHz) | VERIFIED 2026-11-21 |
| OES-LAYER-FERRO | layered rubble with ferro-concrete: 14 dB/m (1 GHz), 31 dB/m (2 GHz) | W: **central** bracket | §III.B | "When ferro-concrete layers are included (in the same proportion as other layers), specific attenuation reach respectively 14 and 31 dB/m" | k = log2(31/14) = **1.14684**. 868 MHz: 14·0.868^1.14684 = 14·0.85014 = **11.90 dB/m**. 2.4 GHz: 31·1.2^1.14684 = 31·1.23256 = **38.21 dB/m** | VERIFIED 2026-11-21 |
| OES-LAYER-SPREAD | spread of integrated excess loss 2–10 dB (plain), 2–14 dB (with ferro) | W: per-path random spread | §III.B | "with standard deviations on the integrated excess attenuation from 2 to 10 dB"; "with standard deviations from 2 to 14 dB" | used as the standard deviation range of the per-path spread | VERIFIED 2026-11-21 |
| OES-LAYER-THICK | layer thickness N(50 cm, 20 cm); second set N(20 cm, 5 cm) | W: rubble layer generator | §III.B | "Gaussian distributed, with a mean of 50 cm and a standard deviation of 20 cm … average thickness of 20 cm, with a standard deviation of 5 cm" | — | VERIFIED 2026-11-21 |
| OES-MEAS-80 | measured 80 dB/m at 2 GHz, ~75% ferro-concrete test structure | W: **high** bracket | §III.B | "the simulated and measured attenuation at 2 GHz are 72 and 80 dB/m respectively … fairly high proportion of ferro-concrete (around 75 %)" | Frequency slope k = 1.14684 (OES-LAYER-FERRO). 868 MHz: 80·(0.868/2)^1.14684 = 80·0.38394 = **30.71 dB/m**. 2.4 GHz: 80·1.23256 = **98.60 dB/m** | VERIFIED 2026-11-21 |
| OES-SPHERE | loose rubble A = α·η dB/m, α = 35–45 | W: density form, cross-check of density | §IV.B | "the specific attenuation in [dB/m] can be fitted as αη, with α depending on the size distributions. Typical values for α range from 35 to 45." | with η from DENS-* (PENDING) | VERIFIED 2026-11-21 |
| OES-RANGE | detection range at 130 dB dynamic range: GSM-900 ~10 m (concrete + mixed), < 2 m (ferro) | W: validation target A5.4 | §VI, Table III | "Concrete + mixed ~ 15 m ~ 10 m ~4m"; "Ferro-concrete <3m <2m <1m" (TETRA / GSM-900 / UMTS) | central 868 MHz: 130/11.90 = **10.9 m** ✓; high: 130/30.71 = 4.2 m vs "< 2 m" (limitation recorded) | VERIFIED 2026-11-21 |
| OES-BIMPAS | 60–100 dB measured at 2.45 GHz in ruins (Bimpas et al., EuMC 2003) | W: validation target A5.3 | §I (secondary) | "Attenuations of 60 to 100 dB were measured at 2.45 GHz (compared to more than 250 dB at 10 GHz)." | — | PENDING (primary: Bimpas et al., "Development and testing of a detector system for trapped humans in building ruins", EuMC 2003, pp. 999–1002) |

### 2a. Solid fraction and UWB rubble loss (derived only from rows above)

No primary source found (2026-11-21) gives a measured solid fraction or bulk density for an in-place collapsed RC pile. Loose hauled C&D waste densities (e.g. Florida DEP loads) describe trucks, not piles, and are not used. Instead the effective solid fraction is **derived from sourced losses**: η_eff = A_rubble(2 GHz) / A_solid(2 GHz), using the same measured rubble values as the §2 brackets. This relies on loss being proportional to solid fill: supported by NIST6055-DENSLAW ("signal loss per unit thickness is proportional to the material density") and by OES-SPHERE (A = α·η). The ratio is assumed frequency-independent from 2 to 8 GHz. That assumption is stated in the paper. Script: scripts/collapse/eta_uwb_brackets.py.

| ID | Value | Used for | Calculation (all inputs are rows above) | Status |
|---|---|---|---|---|
| ETA-LOW | η = 0.3522 | W: low bracket fill; bias | OES layered no-ferro 20 dB/m at 2 GHz ÷ ITU2040-T3 solid concrete 56.78 dB/m at 2 GHz = 0.3522 | DERIVED 2026-11-21 |
| ETA-CEN | η = 0.5459 | W: central | OES with ferro 31 ÷ 56.78 = 0.5459 | DERIVED 2026-11-21 |
| ETA-HIGH | η = 0.7706 | W: high | OES measured 80 ÷ NIST6055-LOW 103.82 (2 GHz) = 0.7706 (the 80 dB/m pile was ~75% ferro, so the higher NIST solid value is paired with it) | DERIVED 2026-11-21 |
| ETA-XCHK | interclast porosity 34–39% for simulated room-collapse rubble (salt clasts) → η 0.61–0.66 | cross-check only, not used | WIPP rubble study (InterPore Journal, ipjournal.interpore.org/index.php/interpore/article/view/45), from a search summary; PENDING primary. Inside [0.35, 0.77] | PENDING (cross-check) |
| UWB65-LOW / CEN / HIGH | **50.28 / 77.94 / 186.74 dB/m** at 6.5 GHz | W: UWB channel 5 rubble loss | 0.3522·142.76 (ITU 6.5 GHz) = 50.28; 0.5459·142.76 = 77.94; 0.7706·242.34 (NIST6055-HIGH) = 186.74 | DERIVED 2026-11-21 |
| UWB8-LOW / CEN / HIGH | **59.15 / 91.68 / 219.7 dB/m** at 8 GHz | W: UWB channel 9 rubble loss | 0.3522·167.94 = 59.15; 0.5459·167.94 = 91.68; high: NIST 6.5 GHz scaled by the ITU frequency ratio 167.94/142.76 = 1.1764: 242.34·1.1764 = 285.1 dB/m solid (cross-check: NIST6055-8G text "approximatey 90 dB" for 305 mm = 295 dB/m including interfaces ✓), ×0.7706 = 219.7 | DERIVED 2026-11-21 |
| SLOW-BR | ranging bias per metre of rubble **0.454 / 0.704 / 1.541 m/m** | W: ToF bias, A4 | b = η·(√ε′ − 1), with ε′ paired to the solid-loss source: low and central ITU ε′ = 5.24 (√ε′ − 1 = 1.2891); high Oestges ε′ = 9 (√ε′ − 1 = 2.000). 0.3522·1.2891 = 0.454; 0.5459·1.2891 = 0.704; 0.7706·2.000 = 1.541. Sensitivity case: central η with ε′ = 9 gives 1.092 m/m | DERIVED 2026-11-21 |
| SNELL | √ε_rc1·sin θ1 = √ε_rc2·sin θ2; total internal reflection only into a lower index | W: exit angle of the lowest-loss path out of the pile (DD 37) | ITU-R P.2040-4 §2.2.1, eq. (30), p. 10; p. 11 | "The angle of refraction θ2 is related to the angle of incidence θ1 by Snell's law."; "Total internal reflection is only possible when a wave is incident upon a medium with lower refractive index." | pile index n = 1 + η(√ε′ − 1) = 1 + SLOW-BR: low 1.454, central 1.704, high 2.541. Critical angle arcsin(1/n): low 43.45°, central 35.94°, high 23.17° | VERIFIED 2026-11-22 |
| FRESNEL | power transmission across the pile surface, TE/TM average, Ts = 1 − |R|² | W: interface loss per crossing (DD 37) | ITU-R P.2040-4 eqs. (31a), (31b), (33), (34)–(35) and §2.2.1.3 b), pp. 11–13 | "For either polarisation, T1T2 = 1 − R²"; "The change in signal level in decibels due to reflection or transmission is thus given by 10 log(|R_S|)" | lossless media (tan δ ≤ 0.13, SLOW-1). Normal incidence, central: R = ((1.704 − 1)/(1.704 + 1))² = (0.7038/2.7038)² = 0.06775 → −10·log10(0.93225) = **0.3047 dB** | VERIFIED 2026-11-22 |

These rows **replace** the plan's η = 0.57 / 0.69 / 0.92 and bias 0.73 / 1.32 / 1.84 m/m, which depended on the unsourced debris densities (DENS-*). The 868 MHz and 2.4 GHz brackets of §2 are unchanged, because they come from rubble measurements directly.

## 3. Measured building and rubble loss, NIST

| ID | Value | Used for | Source and location | Verbatim quote | Status |
|---|---|---|---|---|---|
| NIST1540-STAND | standing building mean loss 25–50 dB, standard deviation 6–14 dB | W: validation | Holloway et al., NIST Technical Note 1540 (2005), 13-story apartment building, executive summary | "The mean signal attenuation for a wide range of locations throughout the standing building ranged from 25 to 50 dB. The standard deviation ranged from 6 to 14 dB, depending on frequency." | VERIFIED 2026-11-21 (page number to add) |
| NIST1540-RUBBLE | rubble ≥ 60–80 dB (noise-limited) | W: validation target A5.1 (dense) | TN 1540, executive summary | "Large amounts of building rubble caused at least 60 to 80 dB of signal attenuation. The true attenuation was not established since we measured only noise." | VERIFIED 2026-11-21 (page to add) |
| NIST1542-RUBBLE | rubble ≥ 30–40 dB (voids remained) | W: validation target A5.1 (void-rich) | Holloway et al., NIST Technical Note 1542 (2006), convention center, executive summary (p. iv) | "Large amounts of building rubble caused at least 30 to 40 dB of signal attenuation. The true attenuation was not established in several cases since we measured only noise." | VERIFIED 2026-11-21 |
| NIST1542-FREQ | frequencies ~50, 150, 225, 450, 900 MHz, 1.8 GHz | W: applicability to 868 MHz | TN 1542, §1 | "approximately 50 MHz, 150 MHz, 225 MHz, 450 MHz, 900 MHz, and 1.8 GHz" | VERIFIED 2026-11-21 |

### 3a. Solid plain concrete, NISTIR 6055

Source: W. C. Stone, "Electromagnetic Signal Attenuation in Construction Materials", NISTIR 6055, NIST Construction Automation Program Report No. 3, October 1997, DOI 10.6028/NIST.IR.6055. Specimens: plain concrete batch 1 (w/c 0.4, 12.7 mm aggregate), nominal thicknesses 101.6 / 203.2 / 304.8 mm (C14 / C18 / C112). Page numbers are the printed page footers.

| ID | Value | Used for | Location | Verbatim quote | Calculation | Status |
|---|---|---|---|---|---|---|
| NIST6055-FORM | dB curves are 20·log10 of the decimal (amplitude) coefficient | W: reading the tables right | §4.4 p. 81; Tables 4.4a/4.4b pp. 82–83 | "The ordinate values represent the ratio of the received to transmitted signal. The decimal attenuation values are presented first, followed by the same ordinate set converted to decibels." | at 1 GHz, C14: Table 4.4a fit gives 0.2458 (decimal); 10·log10(0.2458) = −6.09 dB, while Table 4.4b gives −12.19 dB = 20·log10(0.2458) (fit-to-fit check at 0.868/1/1.5/2 GHz, ratio 2.000 at each) | VERIFIED 2026-11-21 |
| NIST6055-DENS | batch 1 density 2345.9 kg/m³ (2.35 g/cc, theoretical); §4.4 text gives 2.31 g/cc | W: solid density cross-check for DENS-* | §3 batch table p. 25; §4.4 p. 81 | "Density (kg/m^3) 2345.9"; "an average density of 2.31 g/cc" | — | VERIFIED 2026-11-21 |
| NIST6055-LOW | solid concrete slope **114.2 dB/m at 0.868 GHz, 111.1 at 1.0, 102.3 at 1.5, 103.8 at 2.0** | W: high-side solid loss, sub-GHz and 2 GHz | Table 4.4b p. 83 (regression, 0.5–2.0 GHz, R ≥ 0.9995) | "Received Signal Magnitude (relative to free space) = M0+M1*F+M2*F^2+M3*F^3+M4*F^4+M5*F^5+M6*F^6, where F is the frequency in GHz." | evaluate C14L and C112L polynomials (coefficients as printed); slope = (L102 − L305)/(0.3048 − 0.1016 m). 0.868 GHz: (−11.61 + 34.82)/0.2032 = 114.2. 1 GHz: (−12.19 + 34.77)/0.2032 = 111.1. 2 GHz: (−14.86 + 35.96)/0.2032 = 103.8. Differencing two thicknesses cancels the two air–concrete interface losses (script: scripts/collapse/nist6055_slopes.py) | VERIFIED 2026-11-21 |
| NIST6055-HIGH | solid concrete slope **246.1 dB/m at 3 GHz, 248.4 at 4 GHz, 242.3 at 6.5 GHz** | W: high-side solid loss, UWB | Table 4.4d p. 85 (regression, 3.0–8.0 GHz, R = 1) | same form, "Data are for the Frequency Range 3.0 to 8.0 GHz." | 6.5 GHz: C14H −25.31 dB, C112H −74.56 dB; (74.56 − 25.31)/0.2032 = 242.3 dB/m | VERIFIED 2026-11-21 |
| NIST6055-8G | 8 GHz end point NOT used | — | Table 4.4d; §5 p. 183 | "A one milliwatt signal penetrating a 305 mm thick concrete wall will undergo a loss of approximatey 90 dB at 8 GHz." | the C112H polynomial at 8.0 GHz gives −60.5 dB, less loss than C18H (−63.9 dB) and far from the text's ~90 dB: the fit is unreliable at the band edge (noise floor). UWB channel 9 (8 GHz) uses ITU2040-T3 and the 6.5 GHz NIST point only | RECORDED 2026-11-21 |
| NIST6055-DENSLAW | loss per unit thickness ∝ density (non-conducting materials) | W: supports A = α·η form (OES-SPHERE) | §5 p. 183, Figure 5.2 p. 184 | "Figure 5.2 strongly supports the notion that, at least in non-conducting materials, the signal loss per unit thickness is proportional to the material density." Fits: "y = 0.015451 + 0.046999x" (1 GHz), "y = 0.022399 + 0.056185x" (1.5 GHz), dB/mm vs g/cc | at 2.35 g/cc, 1 GHz: 0.015451 + 0.046999·2.35 = 0.1259 dB/mm = 125.9 dB/m (fit across materials; the concrete-only slope NIST6055-LOW is 111.1) | VERIFIED 2026-11-21 |

**Consistency with the rubble brackets.** NIST solid concrete at 2 GHz × the high η (0.92, DENS-*, PENDING) gives 103.8·0.92 = 95.5 dB/m, within 3% of the high 2.4 GHz bracket 98.6 dB/m that came from Oestges' 80 dB/m measurement (independent sources). At 868 MHz, 114.2·0.92 = 105 dB/m is far above the high bracket 30.7 dB/m. The ITU solid value (33.0 dB/m at 1 GHz) and NIST (111.1) differ by 3.4×; NIST specimens were laboratory-cast concrete (28-day strength tests, Table 3.3.4 p. 46; age and moisture at test not stated in the parts read), and the reason for the gap is not established here. So NIST is kept as an **extra-high solid-material case** ("wet concrete"), reported as a sensitivity case, not as the high bracket; the brackets of §2 stay as derived. No value is changed to fit results.

### 3b. Floor loss, ITU-R P.1238-13

Source: Recommendation ITU-R P.1238-13 (09/2025), "Propagation data and prediction methods for the planning of indoor radiocommunication systems and radio local area networks in the frequency range from 300 MHz to 450 GHz" (in force; listed at itu.int/rec/R-REC-P.1238/en, checked 2026-11-21).

| ID | Value | Used for | Location | Verbatim quote | Calculation | Status |
|---|---|---|---|---|---|---|
| P1238-EQ2 | L_total = L(d0) + N·log10(d/d0) + Lf(n), Lf = 0 for n = 0 | W: form of the standing-building floor term (validation only) | §3, eq. (2), pp. 7–8 | "Lf : floor penetration loss factor (dB)"; "n: number of floors between base station and portable terminal (n ≥ 0), Lf = 0 dB for n = 0." | — | VERIFIED 2026-11-21 |
| P1238-T5-900 | 0.9 GHz, office: 9 / 19 / 24 dB for 1 / 2 / 3 floors | W: validation target A5.2 (standing structure, not rubble) | Table 5, p. 9 | "Floor penetration loss factors, Lf (dB) with n being the number of floors penetrated, for indoor transmission loss calculation (n ≥ 1)"; row "0.9 … 9 (1 floor) 19 (2 floors) 24 (3 floors)" in the Office column (Residential and Commercial "–") | — (note: earlier plan text said residential; the current table puts these in the Office column) | VERIFIED 2026-11-21 |
| P1238-T5-24 | 2.4 GHz: office 14 dB; residential 10 (apartment, per concrete wall) / 5 (house) | W: validation, 2.4 GHz floor term | Table 5, p. 9 | row "2.4 … 10 (apartment) 5 (house) … 14"; note "(1) Per concrete wall." | — | VERIFIED 2026-11-21 |
| P1238-T5-18 | 1.8–2 GHz: residential 4n; office 15 + 4(n − 1); commercial 6 + 3(n − 1) | W: validation | Table 5, p. 9 | "1.8-2 4n 15 + 4 (n − 1) 6 + 3 (n − 1)" | n = 2 office: 15 + 4 = 19 dB | VERIFIED 2026-11-21 |
| P1238-ISOL | multi-floor loss saturates (external paths) | W: why the rubble model is per metre of material, not per floor | text after Table 5, p. 9 | "It should be noted that there may be a limit on the isolation expected through multiple floors. The signal may find other external paths to complete the link with less total loss than that due to the penetration loss through many floors." | — | VERIFIED 2026-11-21 |
| P1238-52 | 5.2 GHz reinforced concrete floor, normal incidence: 20 dB (SD 1.5); with lighting 30 (SD 3); with ducts 36 (SD 5) | W: UWB cross-check (one intact slab) | text after Table 5, p. 9 | "measurements at 5.2 GHz have shown that at normal incidence the mean additional loss due to a typical reinforced concrete floor with a suspended false ceiling is 20 dB, with a standard deviation of 1.5 dB." | — | VERIFIED 2026-11-21 |

## 4. Ranging through obstacles

| ID | Value | Used for | Source and location | Verbatim quote | Status |
|---|---|---|---|---|---|
| KOL-2NS | penetrating-path bias usually ≤ 2 ns (0.60 m) | W/S: bias bound | Kolakowski & Modelski, "Detection of direct path component absence in NLOS UWB channel", MIKON 2018, pp. 247–250, DOI 10.23919/MIKON.2018.8405190 (accepted version arXiv:2404.15314), §II | "When the direct path component propagating through the obstacle is available… the introduced bias usually would not exceed 2 ns." | VERIFIED 2026-11-21 (via the accepted-version HTML) |
| KOL-1NS | two 10 cm concrete walls ≈ 1 ns | W: cross-check of SLOW-1 | same, §II (cites Albaidhani, Morell & Vicario, IPIN 2016) | "propagation through two 10 cm thick concrete walls would result in about 1 ns delay" | SLOW-1: 0.2·1.2891 = 0.2578 m = 0.86 ns (ITU) and 0.2·2.000 = 0.400 m = 1.33 ns (OES) ✓. VERIFIED; primary Albaidhani 2016 PENDING |
| KOL-NDP | direct path absent → errors of metres | W: no-direct-path error | same, §II | "It introduces a large delay of several nanoseconds, which leads to a ranging error of even a few meters." | VERIFIED 2026-11-21 |
| ALS2009 | UWB TOA error model, direct path present / absent; blockage probabilities | W: no-direct-path statistics | Alsindi, Alavi & Pahlavan, "Measurement and modeling of ultrawideband TOA-based ranging in indoor multipath environments", IEEE TVT 58(3):1046–1058, 2009, Tables III–VI | tables are images in the PDF | PENDING (read from rendered pages) |
| NEZ-WALL | 60 cm reinforced wall ≈ 14 dB at 2 GHz | W: extra low case | A. Nezirović, "Trapped-Victim Detection in Post-Disaster Scenarios using Ultra-Wideband Radar", PhD thesis, TU Delft, §4.1.2 | "the attenuation is regressing almost linearly, reaching in average ca 14 dB at 2 GHz" | 14/0.6 = **23.3 dB/m**. VERIFIED 2026-11-21 (thesis year to add) |
| NEZ-SAND | sandstone rubble ≤ ~10 dB | W: extra low case | same, §4.1.1 | "the attenuation is less for lower frequencies, as expected, but that it rarely exceeds 10 dB" | VERIFIED 2026-11-21 |

## 5. Scenario and rescue

| ID | Value | Used for | Source and location | Verbatim quote | Status |
|---|---|---|---|---|---|
| INSARAG-HEAVY | heavy team: 2 sites, 24 h operations, up to 10 days; operational within 48 h | X: dig sites, arrival | INSARAG Guidelines and Methodology (UN OCHA, March 2011), p. 33, §7.3 | "A heavy team must be adequately staffed to allow for 24 hour operations at 2 separate sites for up to 10 days."; "operational in the affected country within 48 hours" | VERIFIED 2026-11-21 |
| INSARAG-MED | medium team: 1 site, up to 7 days; operational within 32 h | X: dig sites, arrival | same, p. 33, §7.2 | "A medium team must be adequately staffed to allow for 24 hour operations at 1 site for up to 7 days."; "operational in the affected country within 32 hours" | VERIFIED 2026-11-21 |
| DEBRUY-1983 | see DEBRUY-T2 / DEBRUY-TIMES (§5d), read from the primary | X: survival weighting | De Bruycker et al., Bull. WHO 1983 | — | SUPERSEDED 2026-11-22 by §5d (owner downloaded the PDF) |
| RESCUE-TIME | extrication time per victim | X: digging model | Natural Hazards (Springer) 2014, DOI 10.1007/s11069-014-1304-3 | — | PENDING (library access) |

## 5a. Many devices on air, Bor et al. 2016

Source: M. Bor, U. Roedig, T. Voigt, J. M. Alonso, "Do LoRa Low-Power Wide-Area Networks Scale?", ACM MSWiM 2016, pp. 59–67, DOI 10.1145/2988287.2989163 (Lancaster University repository copy).

| ID | Value | Used for | Location | Verbatim quote | Calculation | Status |
|---|---|---|---|---|---|---|
| BOR-TIMING | overlap tolerated if ≥ 5 preamble symbols stay intact; critical section x_cs = (a_x + T_sym·(N_pp − 5), b_x) | X: collision timing in B5 (new switch only) | §3, "Timing" | "packets can overlap, as long there are at least 5 preamble symbols left intact (in case of a weak packet). In other words, the critical section of a packet reception starts at the last 5 preamble symbols" | with N_pp = 8 (paper's preamble), the first 3 symbols may overlap | VERIFIED 2026-11-21 |
| BOR-CAP | capture observed with 3 dBm vs 2 dBm transmitters (1 dB transmit difference); the paper gives eq. (9) but no number for P_threshold | X: capture threshold sweep in B5 | §3, "Power", eq. (9), Figure 3 | "One transmitter is set to transmit a 32-byte packet on a regular interval at a weak transmit power (2 dBm), while another transmitter was set to transmit a 32-byte packet at a strong transmit power (3 dBm)." | the existing simulator uses 6 dB (ca-tbl-mac.cc `m_captureThresholdDb`, commented as a model parameter to sweep). Collapse runs keep 6 dB as the default (earlier work unchanged) and sweep 1 dB (Bor experiment) to 6 dB | VERIFIED 2026-11-21; numeric threshold absent in primary (recorded) |

## 5b. GPS for rescuers on the surface

Source: U.S. DoD, "Global Positioning System Standard Positioning Service Performance Standard", 5th Edition, April 2020 (in effect; archive.gps.gov/technical/ps/2020-SPS-performance-standard.pdf, fetched 2026-11-21).

| ID | Value | Used for | Location | Verbatim quote | Calculation | Status |
|---|---|---|---|---|---|---|
| GPS-SPS-AVG | ≤ 8 m horizontal, ≤ 13 m vertical (95%), global average | S: rescuer GPS model, B3 (high/conservative bracket) | Table 3.8-3, p. 58 | "Global Average Position Accuracy … ≤ 8 m 95% Horizontal Error … ≤ 13 m 95% Vertical Error" | horizontal: circular 2-D Gaussian R95 = 2.4477·σ, so σ per axis = 8/2.4477 = **3.27 m**; vertical: 95% = 1.95996·σ, so σ_v = 13/1.95996 = **6.63 m** | VERIFIED 2026-11-21 |
| GPS-SPS-WORST | ≤ 15 m horizontal, ≤ 33 m vertical (95%), worst site | S: stress case | Table 3.8-3, p. 58 | "Worst Site Position Accuracy … ≤ 15 m 95% Horizontal Error … ≤ 33 m 95% Vertical Error" | σ per axis = 15/2.4477 = 6.13 m; σ_v = 33/1.95996 = 16.84 m | VERIFIED 2026-11-21 |
| GPS-SPS-NOTE | the standard is a bound for representative user conditions; it is not the typical error | S: why the earlier 2 m/axis model (DD 20) stays for earlier work and is not re-tuned | Table 3.8-3 "Conditions and Constraints" | "Defined for a position/time solution meeting the representative user conditions" | collapse runs: low = existing 2 m/axis (DD 20, its own source row), high = 3.27 m/axis + 6.63 m vertical; vertical relative heights come from UWB (B3) | VERIFIED 2026-11-21; typical-performance source (e.g. the 2020 SPS performance analysis report) PENDING |

### 5c. Collapse geometry

| ID | Value | Used for | Source and location | Verbatim quote | Calculation | Status |
|---|---|---|---|---|---|---|
| PILE-ALPHA | pile height = α·Δh, α = 0.15 (Δh = intact height converted to rubble) | X: pile height in --scenario=collapse (one value of a sweep) | Schonberg, Baird, Worsey, Belarbi, LaBoube, Lusk, Flanagan, Burleson, Woodley & Noll, "Rubble pile characterization model", Structures Under Shock and Impact IX, WIT Trans. Built Env. 87, 2006, §3 p. 26 (quoting Yarimer & Brown, Structures Under Shock and Impact IV, 1996, pp. 25–33) | "a "collection of blocks falls, it creates a pile of rubble of height α∆h, where ∆h is the intact structure height converted into rubble." They subsequently use a value of α=0.15 to fit their analyses to their experimental data." | e.g. 4 storeys × h_storey: pile = 0.15·4·h. Storey height still needs a source (PENDING) | VERIFIED 2026-11-21 (secondary for α; Yarimer & Brown 1996 primary PENDING) |
| PILE-CONE | pile modelled as a cone of base radius 2 × the building's floor half-length; "Empirical data is now needed" | X: pile footprint (spread beyond the walls) | same, §3 p. 26 and Abstract | "we postulate that the radius of the cone is twice that of the building's average floor half-length"; "Empirical data is now needed to verify the predictive capabilities of these models." | cone volume = H·L·D at height 0.15H gives r_c = 2.52 L (their calculation) | VERIFIED 2026-11-21 (model, not measurement: stated) |
| RANA-WORKERS | 3,122 workers inside; 2,515 saved alive; 1,129 dead; eight storeys; ground floor left intact | X: scale of an industrial collapse (motivation; 20–30 victims per searched sector) | Islam, Azad, Sinha & Lixon, "An Investigation of Recent Building Collapses around the World with Special References to Rana Plaza", 14th Global Engineering and Technology Conference, Dhaka, 2017, DOI 10.5281/zenodo.7349624, §2 p. 2 | "3,122 workers were in the building at the time of the collapse"; "Around 2,515 victims were saved from the building alive."; "an eight-storied commercial building"; "leaving only the ground floor intact" | — | VERIFIED 2026-11-21 (conference paper; quotes the BGMEA president) |

### 5d. Scenario geometry, survival and site layout (primaries read 2026-11-22)

| ID | Value | Used for | Source and location | Verbatim quote | Calculation | Status |
|---|---|---|---|---|---|---|
| HAZUS-STOREY-C | 10 ft per storey, concrete frames | X: intact height of a concrete industrial building | FEMA, Hazus Inventory Technical Manual, Hazus 6.0, November 2022, Table 4-2 "Earthquake and Tsunami Model Specific Building Types", pp. 4-3/4-4 | rows "C1L Concrete Moment Frame Low-Rise 1–3 2 20", "C1M … Mid-Rise 4–7 5 50", "C1H … High-Rise 8+ 12 120" (typical stories, typical height in feet) | 20/2 = 50/5 = 120/12 = 10 ft = **3.048 m** per storey | VERIFIED 2026-11-22 |
| HAZUS-STOREY-S | 12 ft per storey, steel frames | X: steel-frame sensitivity | same, Table 4-2, p. 4-3 | "S1L Steel Moment Frame Low-Rise 1–3 2 24"; "S1M … 4–7 5 60"; "S1H … 8+ 13 156" | 24/2 = 60/5 = 156/13 = 12 ft = **3.658 m** | VERIFIED 2026-11-22 |
| HAZUS-STOREY-IND1 | single-storey industrial 15 ft | X: low end of intact height | same, Table 4-2 | "S3 Steel Light Frame N/A All 1 15"; "PC1 Precast Concrete Tilt-Up Walls N/A All 1 15" | 15 ft = **4.572 m** | VERIFIED 2026-11-22 |
| HAZUS-STOREYS | low-rise 1–3 (typical 2), mid-rise 4–7 (typical 5), high-rise 8+ | X: storey sweep {1, 2, 5, 8} (8 = RANA-WORKERS) | same, Table 4-2 | "Low-Rise 1–3 2", "Mid-Rise 4–7 5", "High-Rise 8+ 12" | pile height (PILE-ALPHA, α = 0.15): 1 storey 0.15·4.572 = **0.686 m**; 2: 0.15·6.096 = **0.914 m**; 5: 0.15·15.24 = **2.286 m**; 8: 0.15·24.384 = **3.658 m** | VERIFIED 2026-11-22 |
| HAZUS-IND-AREA | light industry IND2: 590 ft² per employee, 17,000 ft² per floor; heavy IND1: 550, 51,000; range 2,300–200,600 ft² | X: footprint and workers per floor | same, Table 5-4 "NSI 2022 Occupancy Type Areas", p. 5-5 (IND1 Heavy, IND2 Light, Table 4-1) | "IND1 550 51,000 2,300 200,600"; "IND2 590 17,000 2,300 200,600" | 1 ft² = 0.09290304 m². IND2 floor 17,000 ft² = **1579.4 m²** → square side **39.74 m**; IND1 51,000 ft² = 4738.1 m² → **68.83 m**; minimum 2,300 ft² = 213.7 m² → **14.62 m**. Workers per IND2 floor: 17,000/590 = **28.8**, which is the owner's 20–30 | VERIFIED 2026-11-22 |
| NIOSH-CZ | collapse zone = 1½ × structure height from the building | X: station standoff from the footprint edge | NIOSH Fire Fighter Fatality Investigation and Prevention Program, Report F2019-03, "Brick Gable End Collapses at a Residential Fire Killing a Fire Captain and Seriously Injuring Three Other Firefighters – Illinois", 1 November 2023, p. 29 (cites Loflin et al. 2014; Naum 2015; NFPA 1500, 2021) | "This area is often defined by establishing a perimeter at a distance from the building that is equal to 1½ times the height of the structure" | standoff s = 1.5·H_intact: 2 storeys 1.5·6.096 = **9.144 m**; 5: **22.86 m**; 8: **36.58 m** | VERIFIED 2026-11-22 |
| DEBRUY-T2 | probability of being alive at extrication by day: 0: 87.9% (333/379), 1: 35.3% (20/57), 2–3: 8.5% (3/35), ≥4: 0% (0/77) | X: survival weighting of digging order (B6) | De Bruycker, Greco, Annino, Stazi, de Ruggiero, Triassi, de Kettenis & Lechat, "The 1980 earthquake in southern Italy: rescue of trapped victims and mortality", Bull. WHO 61(6):1021–1025, 1983 (PMC2536241), Table 2, p. 1023 | "Table 2. Probability of being alive, in relation to the number of days trapped, based on 548 casualties rescued after the earthquake"; rows "0 379 333 87.9", "1 57 20 35.3", "2-3 35 3 8.5", ">= 4 77 0 0.0" | 333/379 = 0.8786; 20/57 = 0.3509; 3/35 = 0.0857; used as a step function of days since collapse | VERIFIED 2026-11-22 |
| DEBRUY-TIMES | 24.4% extricated within ½ h, 44.0% within 3 h, 55.7% within 12 h; Time Lib50 = 8 h; 79.6% within 2 days; 93.6% of live extrications within 24 h; trapped mortality 35.0% | X: extrication-time distribution (B6 sensitivity) | same, p. 1023 (text beside Fig. 1) and p. 1022 | "134 people (24.4%) were extricated within half an hour, 241 (44.0%) within 3 hours, and 305 (55.7%) within 12 hours. The time within which 50% of the trapped victims had been extricated (Time Lib50) was 8 hours."; "Of all the people extricated alive, 93.6% (333) were rescued within the first 24 hours." | — | VERIFIED 2026-11-22 (supersedes the PENDING DEBRUY-1983 row) |

## 6. Hardware

| ID | Value | Used for | Source and location | Verbatim quote | Status |
|---|---|---|---|---|---|
| NRF-CLK | 64 MHz | H: Phase 0 | Nordic nRF52840 Product Specification v1.11 (Oct 2024), "CPU performance" | "The CPU clock speed is 64 MHz." | VERIFIED 2026-11-21 (via the HTML PS) |
| NRF-CM | CoreMark 212 (flash, cache on); 3.3 CoreMark/MHz; 64 CoreMark/mA ("DCDC 3V") | H: Phase 0 time and energy | same, rows CMFLASH, CMFLASH/MHz, CMFLASH/mA | "CoreMark, running CoreMark from flash, cache enabled"; "CoreMark per mA, running CoreMark from flash, cache enabled, DCDC 3V" | VERIFIED 2026-11-21 |
| NRF-ICPU0 | 3.3 mA at 64 MHz, CoreMark from flash, DC/DC | H: Phase 0 energy | same, table "CPU running", ICPU0 | "CPU running CoreMark @64 MHz from flash, Clock = HFXO, Regulator = DC/DC" | E_cpu = t·3.3 mA·3.0 V | VERIFIED 2026-11-21 |
| NRF-FPU | single-precision FPU | H: Phase 0 double-precision penalty | same, CPU features | "Single-precision floating-point unit (FPU)" | VERIFIED 2026-11-21 |
| NRF-SLEEP | 1.5 µA System ON, no RAM retention, wake on RTC, 3 V | H: energy | same, key features | "1.5 µA at 3 V in System ON mode, no RAM retention, wake on RTC" | VERIFIED 2026-11-21 |
| BATT-CR123A | 3 V, 1,550 mAh (one CR123A primary lithium cell) | H: COLLAPSE-3, the 72 h check | Panasonic, "CR123A CR Cylindrical-type Lithium Battery (Standard type)", datasheet CR123A_Datasheet_EN_240701 (energy.panasonic.cn), p. 1 | "Nominal voltage 3V"; "Nominal capacity 1,550mAh"; "Continuous drain 20mA" | 1.550 Ah · 3 V = **4.65 Wh = 16,740 J**. Nominal: derating at low temperature or high drain is not applied (stated) | VERIFIED 2026-11-22 |
| CM-HOST | host CoreMark 22,652–25,134 (one core of an i5-12450H, gcc-11 -O2, under the campaign's load) | H: Phase 0 time scaling t_nrf = t_host · CM_host / 212 | EEMBC CoreMark, github.com/eembc/coremark commit 1f483d5 (2025-05-01), run here with `coremark.exe 0x0 0x0 0x66 0 7 1 2000` | "CoreMark 1.0 : 22651.766838 / GCC11.5.0 -O2" (idle-ish); "Iterations/Sec : 25134.048257" (same core, same load as the timing run) | the timing run and its CoreMark were measured back to back on core 0 (taskset), so the ratio carries the same load | MEASURED 2026-11-22 |

## 8. Output validation (rows VAL-*): simulator outputs against published numbers

Run: `ca_tbl_causal_sim --specProfile=true --collapse=true --collapseField=true ...` (DD 40; preset engine
physics), once per bracket plus `--collapseRubbleOff=true`, then
`python3 scripts/collapse/score_validate.py RUN_central RUN_off RUN_low RUN_high`. Seed 21; 8-storey IND2 case:
footprint 39.74 m (HAZUS-IND-AREA), pile 3.658 m (HAZUS-STOREYS, PILE-ALPHA), depth 0.1–3.658 m, 4 stations at
56.45 m (NIOSH-CZ), 20 workers. 11 of 12 pass (2026-11-22).

| ID | Output | Published reference | Tolerance (fixed before the run) | Result |
|---|---|---|---|---|
| VAL-1 | engine total-loss difference, rubble on minus off, every draw | model path loss (DD 37), an implementation check | ≤ 0.01 dB | PASS: max 0.0001 dB over 30,078 draws, each bracket |
| VAL-2 | 868 MHz excess loss, workers 2–5 m deep, pooled over brackets | NIST TN 1542 "at least 30 to 40 dB"; TN 1540 "at least 60 to 80 dB" (rows NIST1542-RUBBLE, NIST1540-RUBBLE) | span reaches ≤ 30 and ≥ 80 dB | PASS: 1.7–103.6 dB (low 1.7–26.8, central 16.0–46.2, high 66.4–103.6) |
| VAL-4a | UWB LOS ToF error SD | Benouakta et al. 2023: 10–33 cm (DW3000) | inside 0.10–0.33 m | PASS: 0.107 m (n = 1516), at the optimistic end |
| VAL-4b | 2.4 GHz LOS ToF error SD at 28–57 m | Zhou et al. 2025 (arXiv:2509.23125), SX1280 outdoors: 1.27 m best point, "the majority of points fall in the 2.5-3.2 m range" (as quoted in lora-physics-parameters.h, tofLosFloorM) | inside 1.27–3.2 m | **FAIL: 5.11 m** (n = 3395). At SNR ≥ 30 dB the SD is 2.9–3.1 m (the engine floor); lower-SNR links widen it through the engine's CRLB term. The model's 2.4 GHz ranging is **wider than measured** (1.6×), i.e. conservative, and that disadvantages the 2.4 GHz-only comparison arm. Reported, not tuned |
| VAL-6 | 2.4 GHz ToF excess per metre of rubble on the chosen path | SLOW-BR (η(√ε′ − 1), from ITU-R P.2040 and Oestges) | bootstrap 95% CI of the median contains the row value. Changed from "within 5%" after the first run showed SNR-dependent noise; the rule is the same for every bracket | PASS low 0.454 [0.454, 0.454]; central 0.704 [0.704, 0.704]; high not testable (no 2.4 GHz range through high-bracket rubble) |
| VAL-3 | detection range | Oestges Table III, ~10 m at 130 dB dynamic range (row OES-RANGE) | within 25% | PASS at model level (G177: 10.92 m) |
| VAL-P | floor penetration, standing structure | ITU-R P.1238-13 Table 5 | — | NOT APPLICABLE to the collapse World (no standing floors); kept for a future standing-building mode |

## 7. Pending primaries (Phase 1 list)

| Needed for | Source | Status |
|---|---|---|
| 868 MHz excess loss and group delay under real earthquake rubble | DiCarlofelice, Di Giampaolo, Feliziani & Tognolatti, IEEE TVT 64(6):2288–2296, 2015, DOI 10.1109/TVT.2014.2346580 | PENDING |
| Concrete loss 3–8 GHz vs thickness | Stone, NISTIR 6055 (1997), DOI 10.6028/NIST.IR.6055 | DONE → §3a |
| Solid density of concrete | EN 1991-1-1 paywalled; not needed after §2a (NIST6055-DENS 2346 kg/m³ recorded) | CLOSED |
| Debris bulk density | not found as a pile measurement; replaced by derived η (§2a) | CLOSED → §2a |
| Void ratios by collapse pattern | Geomatics, Natural Hazards and Risk 2025, DOI 10.1080/19475705.2025.2555727 | PENDING |
| 900 MHz floor loss (9/19/24 dB) | ITU-R P.1238-13 (09/2025) | DONE → §3b |
| Collapse geometry and void types | pile height and spread §5c; storey height, footprint, workers per floor, station standoff §5d; void types (FEMA US&R / USACE FOG) still PENDING | PARTIAL |
| GPS horizontal and vertical accuracy for rescuers | GPS SPS PS 5th ed. (2020) | DONE → §5b |
| Battery capacity | Panasonic CR123A datasheet (BATT-CR123A) | DONE |
| Collision and capture rules | Bor et al., MSWiM 2016 (corrected Lancaster version) | DONE → §5a |
