# LIMITS results — tier-1 reach (L1) and the two-tier arm (L2)

Registered in `report/PREREG_LIMITS_2026-11-14.md` (commit `83519595`) before any measurement cell. Campaign output:
`~/ns/catbl-work/limits_2026-11-14/` (`L1/`, `L2/`, `probe/`, `null/`, `pilot/`, `gate167/`, `suite.log`). Both campaigns ran the
binary built from commit `fcf1cdc0` (`build source fcf1cdc0 dirty 0`; sha256 `00cd4508…89f1`, copied to `L1/bin` and `L2/bin` and
re-verified before every cell), launched from a detached clean worktree at `83519595` (`source 83519595 dirty 0` in every
sidecar; analyser: 0 problems in both studies). Analyser: `scripts/limits_analyse.py`; probe table: `scripts/limits_probe_table.py`.
Tables below are copied from `~/ns/catbl-work/limits_2026-11-14/L1/LIMITS_L1.md` and `~/ns/catbl-work/limits_2026-11-14/L2/LIMITS_L2.md`; every interval is a paired 95 % bootstrap over the 20 seeds
361-380. A(25 m) is the share of ALL 720 cycles of the 1800 s window with error <= 25 m (a no-fix cycle counts by the estimate
held); five-node pools tag 0 and four subjects. Times are IST.

## Answers in one place

1. **L1 explained?** Partly, and the unexplained part is large. The 868 MHz link budget under the preset reproduces the engine's
   own r80 (2338 m mean over probe seeds 361-364, env 1). Against the field figures it leaves **about 31 dB unattributed** even when
   every lossy setting the engine offers is applied at once; none of the quantities the deck lists (antenna height, SF, body-level
   nodes) can be shown to explain it, and the engine has no mechanism by which antenna height changes forest loss at all (section 1a).
2. **Tier-1 reach shrunk to the field value (+30 dB, probe r80 245 m):** CA-TBL five-node A(25 m) falls from 0.962 to 0.949 in config A
   (median seed 0.975 -> 0.969) and stays 0.993 in config B; `static_868` collapses 0.337 -> 0.015 (A) and 0.799 -> 0.585 (B);
   `static_2p4` does not move at all. **Tiering keeps its gain over `static_868` and gains nothing over `static_2p4`** (availability
   difference +0.000 [-0.033, 0.018] in A, +0.000 in B at +30 dB) **while costing 12-14 J more subject energy.**
3. **L2, two-tier arm:** without tier 2, five-node A(25 m) is 0.350-0.356 against CA-TBL's 0.962 in config A, forest (paired
   +0.606 [0.565, 0.647]); the UWB rung on top of 868 MHz adds +0.019 [-0.007, 0.045] over `static_868`. Tier 2 is needed in rural open, forest and
   suburban (all intervals above 0) and **hurts in urban moderate** (two-tier better than CA-TBL in E3 in both configs). The arm is a
   lower bound on tier 2's necessity (section 2).

## 1a. Explaining 2390 m against the field

### The engine's link budget (868 MHz, SF7 / 125 kHz, preset)

`--specProfile=true --probe=true --probeTerms=true --probeMonotoneTime=true`, frozen PRESET-5 binary, `~/ns/catbl-work/limits_2026-11-14/probe/`.
r80 is the distance at which 80 % of probe trials deliver. Mean terms of the loss sum, env 1 (rural forested), seed 361:

| quantity | value | source |
|---|---|---|
| TX power | 14 dBm (`hwtx` term −0.24 dB) | `phy[CA_TBL_TIER_868]`, scenario |
| SF / BW | SF7 / 125 kHz | same |
| sensitivity | −123 dBm; effective −124.5 dBm whenever the link was already receiving (hysteresis 1.5 dB, outside the loss sum) | `lora-physics-parameters.cc:391,401` |
| maximum total loss | 137 dB (138.5 with hysteresis) | 14 − (−123) |
| antenna gain | 0 dBi both ends: `antennaGainDb` computed then zeroed | `lora-physics-engine.cc:340-341` (CLAUDE.md §5) |
| heights | probe anchors 15 m (gateway height Normal(30, 10) in the parameters), node 1 m. **They do not enter the loss** outside RuralOpen with two-ray enabled: `ComputePathLossDb` is log-distance from d0 = 1 m, heights only reach the terrain obstruction term | `lora-propagation-submodule.cc:10-46`, `lora-two-ray-ground-submodule.cc:11` |
| path-loss model | log-distance, FSPL(1 m) = 31.2 dB, exponent n ~ Normal(2.3, 0.1) in RuralForested (2.2 open, 2.75 suburban; **OURS**, not from Petäjäjärvi: CONTEXT_PACK PROV-01) | `lora-physics-parameters.cc:21-35` |
| shadowing | SD ~7 dB, mean 0; fading mean ~1.3 dB (SD 3.9); sets the delivery curve's width (margin SD 9.7 dB at 250 m, 12.9 at 2 km) | probe_terms |
| foliage | dense forest, 0.20 dB/m, saturating at 12 dB (depth clamp); SD ~4 dB | probe_terms |
| NLOS excess | 0.4 dB at 250 m, 3.0 dB at 2 km | probe_terms |
| terrain | 0 to 0.8 dB (2 km); hard-codes 868 MHz | `lora-terrain-submodule.cc:69-70` |

| d (m) | path loss | foliage | NLOS | fading | terrain | sum of terms | total loss from margin (137 − margin) | PDR | margin SD |
|---|---|---|---|---|---|---|---|---|---|
| 250 | 86.4 | 12.0 | 0.4 | 1.3 | 0.0 | 99.9 | 98.6 | 1.000 | 9.7 |
| 1000 | 100.2 | 12.4 | 2.0 | 1.3 | 0.1 | 115.8 | 114.6 | 0.96 | 11.5 |
| 2000 | 107.1 | 12.2 | 3.0 | 1.5 | 0.8 | 124.6 | 123.5 | 0.86 | 12.9 |
| 3000 | 111.4 | 12.2 | 4.8 | 1.4 | 1.5 | 131.5 | 130.6 | 0.71 | 14.1 |

The 1.1-1.3 dB by which the sum of terms exceeds the margin-implied loss is the hysteresis term (CLAUDE.md §5). r80 (2400 m) is where
the mean margin (about 11 dB) equals 0.84 x the margin SD (about 13 dB): the engine is internally consistent.

**How the environment class enters** (probe seed 361-364 mean r80, 868 MHz / 2.4 GHz / UWB, metres):

| `--env` | 868 MHz r80 | 2.4 GHz r80 | UWB r80 | 868 total loss at 250 m |
|---|---|---|---|---|
| 0 rural open | 4262 | 1160 | 8.8 | 87.8 dB |
| 1 rural forested (campaign) | 2338 (2408, 2392, 2194, 2356) | 325 | 6.1 | 99.1 dB |
| 2 suburban | 1063 | 200 | 4.8 | 107.4 dB |
| 3 urban moderate | 710 | 175 | 13.9 | 108.2 dB |
| 4 urban dense (seed 361) | 712 | 182 | 13.5 | 107.8 dB |
| env 1, tropical vegetation (seed 361) | 1675 | 163 | 5.4 | 104.0 dB |
| env 1, tropical + ITU-R P.833 foliage | 2585 | 221 | 5.4 | 97.1 dB |
| env 2 + tropical (seed 361) | 828 | 132 | 3.9 | 111.6 dB |

The environment class changes the path-loss exponent (or the urban formula), the NLOS term and the foliage constants. **The most
lossy tier-1 setting the engine can express (suburban + tropical foliage) is 12.5 dB above the campaign's forest at 250 m.**

### The field figures

Reference numbering of the deck: [17] = Ferreira, Ortiz, Costa, Foubert, Amadou, Mitton, *A study of the LoRa signal propagation in
forest, urban, and suburban environments*, Ann. Telecommun. 75 (2020) 333-351, doi:10.1007/s12243-020-00789-w (`ferreira2020study`
in `Presentation_Report_Guideline_LaTeX_Source/.../ref.bib`; the deck's limitation line cites it for ~250 m); [18] = Petäjäjärvi et al.,
*On the coverage of LPWANs: range evaluation and channel attenuation model for LoRa technology*, ITST 2015 (`petajajarvi2015coverage`,
the 15 km figure).

**What I could verify and what not.** The full texts were not retrievable from this machine (HAL and ResearchGate refused automated
fetches). From the abstracts and secondary summaries only: Ferreira reports about 250 m in forest and about 208 m in high-density urban
at 14 dBm and the maximum spreading factor (taken as SF12), about 2 km in a vehicular scenario; Petäjäjärvi reports over 15 km on
ground (a node on a car roof rack) and about 30 km on water, 868 MHz, 14 dBm, maximum SF, commercial equipment in Oulu. **Node and
gateway heights, antennas and the exact terrain of Ferreira's forest measurements are NOT verified**; nothing below uses them as input.
Both field figures are *maximum reach* (the farthest distance with any reception); the engine's r80 is the distance of 80 % delivery,
a strictly easier-to-meet definition for the field, so the comparison understates the gap.

### Comparison, quantified

At the field's SF12 (sensitivity about −137 dBm) the 14 dBm budget is about 151 dB. The same engine with its sensitivity moved by 14 dB (`--tier1ExtraLossDb=-14`, seeds 362-364)
reaches r80 = 4505 m (4599, 4263, 4653). To bring that to 250 m needs +44 dB (the +30 dB at SF7 plus the 14 dB SF12 advantage); the engine's loss at 250 m is
98.6 dB where a 151 dB limit at 250 m implies 52 dB more.

| item | dB at 250 m | status |
|---|---|---|
| SF7 (engine) against SF12 (field) | +14 dB in the **engine's** favour (the gap is 14 dB *larger* than the 9.6x suggests; 2338 -> ~4500 m at SF12 = ~18x) | quantified by probe |
| environment class: forest -> suburban / urban moderate / urban dense | +8.3 / +9.1 / +8.7 | quantified by probe; **not** attributing the field to any of them |
| vegetation: dense -> tropical foliage; + suburban | +4.9; +12.5 together | quantified by probe |
| antenna height (engine 15 m / 1 m against the field's unknown) | **0 dB: the engine has no height term in this path** | code reading; cannot be attributed *in the engine* |
| body / ground-level nodes, clutter beyond the class | not modelled | cannot be attributed |
| engine's most lossy expressible tier-1 setting | +12.5 dB | so **at least 44 − 12.5 = 31.5 dB** of the field gap has no engine setting that reproduces it |

**Residual stated plainly: about 31 dB (SF12-equivalent) is unattributed, and the engine's tier-1 loss model cannot represent the
field's forest result at all.** The Bonn urban comparison (`report/TIER1_VALIDATION.md`) shows the engine's UrbanModerate class within
−0.7..+1.7 dB of a measured median at 250-500 m, so the engine is not wrong everywhere; Bonn's gateways were elevated (12-55 m) and the
sensors rooftop-mounted, while the engine's RuralForested class is 8-15 dB *less* lossy than Bonn and has never been validated against a
forest measurement. The picture that fits all three facts is that the field's ~250 m comes from a ground-to-ground or obstructed
geometry the engine's log-distance model does not have. **That is an inference, not a measurement,** and it is exactly why the L1
campaign treats the difference as one number to sweep rather than a term to attribute.

## 1b. Reach-sensitivity campaign

`--tier1ExtraLossDb` (commit `5ec5373f`) adds X dB to the 868 MHz sensitivity table. Probe r80, env 1, seeds 361-364 (mean):

| offset | 0 | +10 | +15 | +20 | +30 | +40 | +46 |
|---|---|---|---|---|---|---|---|
| 868 MHz r80 (m) | 2338 | 1284 (s361) | 891 | 604 (s361) | 245 (256, 265, 228, 233) | 111 (s361) | 0 (s361) |

2.4 GHz and UWB r80 are unchanged at every offset (325 m, 6.1 m), which is the flag's isolation check (and gate G167's PROBE check).
Campaign: offsets 0 (the ENV-SWEEP's E1 cells, reused), +15, +30; configs A and B; `--env=1`; seeds 361-380; 1800 s; arms `ca_tbl_full`,
`fixed_threshold`, `static_868`, `static_2p4` (`poucet_fusion` was dropped, registered; **its behaviour under reduced tier-1 reach is not
measured**). 80 new cells, rc 0, 0 problems.

**Null control (rule 2/7 of §4):** the flag-off binary against the frozen PRESET-5 binary: 6 cells of 600 s (seeds 361-365, configs A and B,
env 0-3, default arms in two, the sweep's five in four): **1620 files, 0 differ** (`run_id.txt`, `run.log` excluded);
and the ENV-SWEEP's own cell A/E1/seed 361 at 1800 s with all five arms: **279 files, 0 differ** (`L1/VERIFY_A_E1_s361_flagoff`).
That licenses reusing the sweep's E1 cells as the 0 dB point.

### Results (A(25 m) five-node, tag 0; CEP95 tag 0; subject J — means over 20 seeds)

| config | offset dB | arm | A(25 m) tag 0 | A(25 m) five-node | runs >= 0.95 | CEP95 tag 0 (m) | subject J |
|---|---|---|---|---|---|---|---|
| A | 0 | ca_tbl_full | 0.989 | 0.962 | 0.90 | 4.6 | 13.74 |
| A | 0 | fixed_threshold | 0.989 | 0.963 | 0.90 | 4.6 | 13.69 |
| A | 0 | static_868 | 0.889 | 0.337 | 0.00 | 59.4 | 8.52 |
| A | 0 | static_2p4 | 0.985 | 0.951 | 0.65 | 5.9 | 6.09 |
| A | 15 | ca_tbl_full | 0.987 | 0.914 | 0.80 | 4.3 | 14.13 |
| A | 15 | fixed_threshold | 0.986 | 0.970 | 0.95 | 4.3 | 13.36 |
| A | 15 | static_868 | 0.169 | 0.058 | 0.00 | 452.7 | 8.87 |
| A | 15 | static_2p4 | 0.985 | 0.951 | 0.65 | 5.9 | 6.09 |
| A | 30 | ca_tbl_full | 0.986 | 0.949 | 0.75 | 5.8 | 18.51 |
| A | 30 | fixed_threshold | 0.986 | 0.957 | 0.75 | 5.7 | 18.36 |
| A | 30 | static_868 | 0.022 | 0.015 | 0.00 | 762.0 | 14.32 |
| A | 30 | static_2p4 | 0.985 | 0.951 | 0.65 | 5.9 | 6.08 |
| B | 0 | ca_tbl_full | 0.998 | 0.993 | 1.00 | 1.5 | 25.67 |
| B | 0 | fixed_threshold | 0.998 | 0.993 | 1.00 | 1.5 | 25.47 |
| B | 0 | static_868 | 0.975 | 0.799 | 0.00 | 12.6 | 9.85 |
| B | 0 | static_2p4 | 0.999 | 0.993 | 1.00 | 1.5 | 10.22 |
| B | 15 | ca_tbl_full | 0.999 | 0.994 | 1.00 | 1.5 | 24.83 |
| B | 15 | fixed_threshold | 0.999 | 0.994 | 1.00 | 1.5 | 24.40 |
| B | 15 | static_868 | 0.950 | 0.748 | 0.00 | 20.0 | 9.33 |
| B | 15 | static_2p4 | 0.999 | 0.993 | 1.00 | 1.5 | 10.22 |
| B | 30 | ca_tbl_full | 0.999 | 0.993 | 1.00 | 1.5 | 24.23 |
| B | 30 | fixed_threshold | 0.999 | 0.994 | 1.00 | 1.6 | 23.76 |
| B | 30 | static_868 | 0.966 | 0.585 | 0.00 | 24.1 | 8.46 |
| B | 30 | static_2p4 | 0.999 | 0.993 | 1.00 | 1.5 | 10.22 |

CA-TBL minus comparator (paired), five-node A(25 m) and subject J:

| config | offset | comparator | Δ A(25 m) five-node | Δ CEP95 tag 0 (m) | Δ subject J (J) |
|---|---|---|---|---|---|
| A | 0 | static_868 | 0.626 [0.592, 0.660] | -54.8 [-128.9, -11.3] | 5.22 [4.74, 5.84] |
| A | 15 | static_868 | 0.856 [0.784, 0.911] | -448.3 [-584.7, -330.8] | 5.26 [4.51, 6.10] |
| A | 30 | static_868 | 0.933 [0.901, 0.955] | -756.1 [-1043.2, -496.8] | 4.19 [3.55, 4.83] |
| B | 0 | static_868 | 0.195 [0.167, 0.223] | -11.1 [-22.5, -4.9] | 15.82 [14.87, 16.78] |
| B | 15 | static_868 | 0.246 [0.215, 0.283] | -18.5 [-34.5, -6.1] | 15.50 [14.77, 16.19] |
| B | 30 | static_868 | 0.408 [0.373, 0.443] | -22.7 [-34.8, -14.1] | 15.77 [14.93, 16.53] |
| A | 0 | static_2p4 | 0.011 [-0.013, 0.030] | -1.3 [-3.1, 0.2] | 7.66 [7.12, 8.28] |
| A | 15 | static_2p4 | -0.037 [-0.110, 0.018] | -1.6 [-3.5, -0.3] | 8.05 [7.18, 9.05] |
| A | 30 | static_2p4 | -0.003 [-0.033, 0.018] | -0.0 [-0.9, 0.8] | 12.42 [11.48, 13.27] |
| B | 0 | static_2p4 | 0.000 [-0.001, 0.002] | 0.1 [-0.1, 0.2] | 15.44 [14.54, 16.39] |
| B | 15 | static_2p4 | 0.001 [-0.000, 0.002] | 0.0 [-0.1, 0.2] | 14.60 [13.89, 15.28] |
| B | 30 | static_2p4 | 0.000 [-0.001, 0.002] | 0.0 [-0.1, 0.1] | 14.01 [13.21, 14.79] |
| A | 0 / 15 / 30 | fixed_threshold | -0.001 / -0.056 [-0.124, -0.001] / -0.008 | | |

Each arm, 0 dB minus +30 dB (positive = loses when tier-1 reach shrinks), five-node A(25 m): CA-TBL A 0.014 [-0.012, 0.045], B -0.000 [-0.001, 0.001];
`static_868` A 0.321 [0.282, 0.359], B 0.213 [0.182, 0.247]; `static_2p4` 0.000 [0.000, 0.000] in both.

### Registered predictions (L1)

| # | prediction | verdict | measured |
|---|---|---|---|
| P1 | probe r80 at +30 dB in [200, 320] m, seeds 362-364 | **HOLDS** | 265, 228, 233: mean 242 m |
| P2 | `static_868` falls 0 -> +30 dB in A and B | **HOLDS** | A 0.321 [0.282, 0.358], B 0.213 [0.182, 0.247] |
| P3 | CA-TBL five-node A(25 m) in B at +30 dB >= 0.95 | **HOLDS** | 0.993 |
| P4 | CA-TBL five-node A(25 m) in A at +30 dB < 0.95 | **HOLDS, by 0.001** | 0.949 (the 20-seed median is 0.969: this is on the line, not a collapse) |
| P5 | CA-TBL beats `static_868` at every offset and config | **HOLDS** | lowest lower bound 0.166 |
| P6 | CA-TBL five-node A(25 m) in A non-increasing with offset | **FAILS** | 0.962, 0.914, 0.949. The dip at +15 dB is three seeds (0.43, 0.64, 0.70 against 0.97, 0.96, 0.77 at 0 dB); medians 0.975, 0.972, 0.969. Paired 0 minus 15: 0.048 [0.002, 0.114]; 0 minus 30: 0.014 [-0.012, 0.044]; 15 minus 30: -0.034 [-0.108, 0.026]. A heavy per-seed tail, not a monotone trend. |
| P7 | `static_2p4` falls 0 -> +30 dB in A | **FAILS** | exactly 0: `static_2p4` is unchanged. The zero is a control-flow fact (its tag-0 868 MHz radio has tx, rx and CAD of exactly 0 in these cells (checked, s361), and the same cells' `static_868` and CA-TBL rows did move, so the flag was live in those runs); I predicted a control-plane dependence that is not there. |
| P8 | CA-TBL subject J in A moves < 25 % at +30 dB | **FAILS** | 13.74 -> 18.51 J (+35 %); `static_868` 8.52 -> 14.32 J |

### Reading it (what I will and will not claim)

- Does CA-TBL still meet 0.95 at the field-value reach? **Config B: yes** (0.993). **Config A: on the line** (0.949 mean, 0.969 median; 0.75
  of runs at or above 0.95, against 0.90 at 0 dB).
- Does tiering gain or lose? Against the single-band **868 MHz** arm it gains more as reach shrinks (+0.626 -> +0.933 in A). Against the
  single-band **2.4 GHz** arm there is **no availability gain at any offset or config** (all intervals contain 0), and CA-TBL costs
  **+7.7 to +12.4 J (A) and +14.0 to +15.8 J (B)** more subject energy. This is for forest, `--env=1`, the four arms measured; `poucet_fusion` was not run, so nothing is claimed about the
  fusion comparator or about "unique" availability.
- Limits of the instrument: the shift is a sensitivity shift, so tier-1 RSSI ranges are not biased by it; a field-like loss would also bias
  them. These results are optimistic about tier-1 accuracy at short reach. Tier 1's *reach* mattering little to CA-TBL here is because the
  2.4 GHz legs, not tier 1, carry the fixes in this deployment; that reading is an inference from `static_2p4` ~ CA-TBL, not a separate measurement.

## 2. The two-tier arm

### What was built (commits `a5743159`, `fcf1cdc0`)

`--arms=ca_tbl_two_tier`, by name only (the default list is unchanged, gate G167). Policy: `CaTblPolicyConfig::skipTier2` with helpers
`UwbEntryTier/TierAbove/TierBelow`, applied at every site that meant "one step below UWB"; the grant, the UWB evidence test, on-behalf grants and
query-triggered escalation apply on the 868 MHz -> UWB step; demotion is UWB -> 868. Scenario: the arm maps to `CA_TBL_MODE_NO_INTEGRITY` (checked: with
`ca_tbl_full` all 117 files of a 600 s preset cell identical after name normalisation), the dual base's 2.4 GHz legs are not ranged, woken or billed awake for tag 0 and
the four subjects, the dual base's 868 MHz half stays.

**Choices, each favouring the two-tier arm or neutral (documented in the code):**
1. **No grant gate.** The pilot with the gate kept was degenerate: 0 confirmations against 7-11 timeouts, because arbiters are the anchors' and finders' 2.4 GHz radios and
   grant only on UWB-reach evidence recorded from UWB exchanges or 2.4 GHz ranges under 29.5 m. A node without a 2.4 GHz radio is never granted; keeping the gate would measure the
   arbitration plumbing. The arm enters UWB on the policy's own decision, skipping the latency, denials and slot arbitration the adopted arm pays for.
2. The evidence gate keeps its neighbour-presence half (an 868-ranged neighbour within 20 m) and drops its estimate-quality veto (sigma <= 5 m).
3. The infrastructure (gateway and finder radios, the 2.4 GHz request-over-mesh path) keeps its 2.4 GHz radios. A real two-radio fielding would lose that too.
4. The same PCS thresholds: up at tier 1, down at UWB.

So the arm is **a lower bound on tier 2's necessity**. It also removes the **2.4 GHz dual-base ranging** that the adopted arm performs on every cycle; what the comparison
measures is "no 2.4 GHz radio on the subject", which includes both the ladder rung and that ranging. The campaign cannot separate them.

### Controls

- N2 determinism and flag identity: the arm alone with the flag absent and with `--tier1ExtraLossDb=0` (gate G167 `off` against `zero`, seed 361, 600 s): 58 files byte-identical.
- N3 per-arm stream independence on a 1800 s cell (`L2/VERIFY_A_E1_s361_pair`): the pair run's `ca_tbl_full` row and all 55 of its files equal the sweep's A/E1/s361; its `ca_tbl_two_tier` row and 55 files equal the filtered
  two-tier-only cell. That licenses reusing the sweep's per-arm results as comparators.
- Energy invariant: `verify_causal_sim.py` on the pair run audits both arms (`arms_audited 2/2`, 44 checks pass). It reports 5 failures: 3 are `ca_tbl_full`'s (a nextTier/activeTier feed-forward
  mismatch on 4 steps; the legacy-NLOS-flag check; the 868 duty cycle 4.69 %); the two-tier arm fails only the legacy-NLOS-flag check (the same check, on a preset run read as `nlos_source legacy`) and the 868 duty
  cycle (2.99 % against 2.5 %). None concerns the one-tier-per-row or the energy-monotonicity checks, which pass for it. These checks are not specific to the new arm; they fail for `ca_tbl_full` in the same run.
- Gate G167 (`scripts/two_tier_gate.sh`, `.py`, `two_tier_gate_selftest.sh`): real run set ok (0 tier-2 cycles, tx, rx, CAD, idle on tag 0 and the 5 subject rows; tier-2 joules are the sleep current alone; the arm reached UWB in 37
  cycles and `ca_tbl_full` used tier 2 in 129, so the zero is not vacuous); 12 negative fixtures built from a copy of the run set, each observed to fail (13 as expected, 0 not).

### Results

A(25 m) five-node, tag 0, CEP95 tag 0, subject J, means over 20 seeds per cell (`~/ns/catbl-work/limits_2026-11-14/L2/LIMITS_L2.md`). Comparator arms from the ENV-SWEEP.

| config | env | two-tier | CA-TBL | `static_868` | `static_2p4` |
|---|---|---|---|---|---|
| A | E0 open | 0.509 | 0.960 | 0.479 | 0.964 |
| A | E1 forest | 0.356 | 0.962 | 0.337 | 0.951 |
| A | E2 suburban | 0.089 | 0.905 | 0.090 | 0.766 |
| A | E3 urban moderate | 0.053 | 0.024 | 0.033 | 0.024 |
| B | E0 | 0.664 | 0.992 | 0.587 | 0.987 |
| B | E1 | 0.826 | 0.993 | 0.799 | 0.993 |
| B | E2 | 0.779 | 0.990 | 0.720 | 0.988 |
| B | E3 | 0.678 | 0.163 | 0.541 | 0.144 |

(five-node A(25 m); the full table with tag 0, CEP95 and J is in `~/ns/catbl-work/limits_2026-11-14/L2/LIMITS_L2.md`.)

CA-TBL (`ca_tbl_full`) minus `ca_tbl_two_tier`, paired:

| config | env | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|---|
| A | E0 | 0.452 [0.391, 0.517] | 0.147 [0.040, 0.288] | -368.9 [-684.2, -116.6] | -5.35 [-10.01, -1.21] |
| A | E1 | 0.606 [0.565, 0.647] | 0.085 [0.009, 0.207] | -42.1 [-102.6, -9.8] | -9.71 [-12.81, -6.66] |
| A | E2 | 0.816 [0.765, 0.861] | 0.933 [0.892, 0.963] | -404.6 [-493.6, -331.0] | -2.51 [-5.32, -0.04] |
| A | E3 | -0.029 [-0.049, -0.009] | -0.121 [-0.207, -0.047] | -356.3 [-1215.8, 450.9] | 8.17 [4.28, 11.16] |
| B | E0 | 0.328 [0.280, 0.374] | 0.121 [0.019, 0.258] | -20.9 [-35.9, -9.0] | -48.46 [-54.89, -42.30] |
| B | E1 | 0.167 [0.135, 0.201] | 0.051 [0.010, 0.112] | -13.7 [-26.4, -5.5] | -41.71 [-46.51, -37.23] |
| B | E2 | 0.211 [0.175, 0.248] | 0.033 [0.011, 0.071] | -18.2 [-31.7, -10.2] | -47.33 [-51.26, -43.59] |
| B | E3 | -0.515 [-0.565, -0.466] | -0.541 [-0.630, -0.436] | 73.1 [33.8, 108.4] | 0.21 [-2.45, 2.69] |

Two-tier minus `static_868`, five-node A(25 m): A E0 0.030 [-0.014, 0.076], E1 0.019 [-0.007, 0.045], E2 -0.000 [-0.022, 0.021], E3 0.020 [0.001, 0.040]; B E0 0.077 [0.035, 0.117], E1 0.028 [-0.011, 0.063],
E2 0.059 [0.012, 0.103], E3 0.136 [0.065, 0.200]. Subject energy of the two-tier arm is 2.5-3x `static_868`'s in config A (E1 23.5 J against 8.5 J) and about 7x in B E0-E2 (E1 67.4 J against 9.9 J), and 2.6-3.0x CA-TBL's in B E0-E2.

**Mechanism** (tag 0, 20 seeds pooled, denominators in the table; UWB cycle = active tier UWB; 'no UWB fix' = the cycle's fix tier is not UWB; 'no anchor in reach' = no UWB anchor within the deployment's nominal UWB range, truth-based):

| config | env | arm | share of all cycles with a UWB fix | UWB cycles | of which no UWB fix | of which no anchor in reach |
|---|---|---|---|---|---|---|
| A | E1 | two-tier | 0.001 | 900 | 0.983 | 0.914 |
| A | E1 | CA-TBL | 0.000 | 35 | 1.000 | 0.429 |
| B | E1 | two-tier | 0.022 | 2423 | 0.866 | 0.641 |
| B | E1 | CA-TBL | 0.009 | 324 | 0.611 | 0.228 |
| A | E0 | two-tier | 0.003 | 611 | 0.923 | 0.858 |
| A | E2 | two-tier | 0.001 | 354 | 0.960 | 0.859 |
| A | E3 | two-tier | 0.003 | 233 | 0.807 | 0.794 |
| B | E3 | two-tier | 0.056 | 1961 | 0.585 | 0.556 |

(14 400 tag-0 cycles per config x env cell; the two-tier arm spends 1.6-17 % of them on UWB (A E1: 6.3 % against CA-TBL's 0.24 %, 26x; B E1: 16.8 % against 2.3 %, 7x), and almost none of those cycles yields a UWB fix.) The
two-tier node is on UWB with no anchor in reach on 91 % (A, forest) of its UWB cycles, and at UWB reach of about 6 m (probe, E1) that is the expected result: it pays UWB's 0.63 J/cycle
for no fix. Reading: **the UWB rung without tier 2 behind it is mostly a way to spend energy**; the gain of the adopted arm over `static_868` in A comes from the 2.4 GHz ranging, not from UWB.

**E3 (urban moderate), the reversal.** In urban moderate CA-TBL is worse than `static_868` in B (0.163 against 0.541) and than the two-tier arm in both configs. Removing 2.4 GHz
improves it. I have not diagnosed why (NLOS-biased 2.4 GHz ranges are the obvious suspect; the engine's 2.4 GHz r80 there is 175 m) and make no claim beyond the measurement.

### Registered predictions (L2)

| # | prediction | verdict | measured |
|---|---|---|---|
| Q1 | full − two-tier five-node A(25 m), A, E1, interval above 0 | **HOLDS** | 0.606 [0.565, 0.647] |
| Q2 | the same in >= 3 of 4 environments in A | **HOLDS** | E0, E1, E2 (E3 reversed: -0.029 [-0.049, -0.009]) |
| Q3 | the E1 difference is smaller in B than in A | **HOLDS** | A 0.606, B 0.167 |
| Q4 | CEP95 tag 0, A, E1: full better | **HOLDS** | -42.1 [-102.6, -9.8] m |
| Q5 | subject J, A, E1: two-tier cheaper than full | **FAILS** | full − two-tier = -9.71 [-12.81, -6.66] J: the two-tier arm costs **more** |
| Q6 | >= half of two-tier's tag-0 UWB cycles in A, E1 have no UWB fix | **HOLDS** | 885/900 = 0.983 |

## Gate suite

`bash scripts/catbl_gates.sh` run once, unpiped, from this worktree after both campaigns, seed 7, 3600 s: **136 of 141 gates PASS, 5 FAIL; log `~/ns/catbl-work/limits_2026-11-14/suite.log`**.
`git status --porcelain` before and after the suite is identical (no tracked file modified). G167 (the new gate, with its 12 negative fixtures) PASSES, as does G3
(which CLAUDE.md lists as red by design: it is green on this tree).

| gate | result | cause |
|---|---|---|
| G11 | FAIL | G11b, the documented permanent red (terrain transcription-only) |
| G24 | FAIL | "FAIL BY FINDING: mechanisms with no population under the preset" (TECH_DEBT N.15), the gate's own message |
| G93 | FAIL | FAIL BY FINDING: tier 2's default-flag sensitivity is the 1625 kHz row at 800 kHz (D-8); the preset adopts the 812.5 kHz row (`t2SensitivityRowKHz`), and the gate reads the default configuration |
| G157 | FAIL | "the inert fixture was called effective: the comparison cannot see 'no effect'" |
| G165 | FAIL | the real run set fails one check: steering-entry age median 15.0 s with the feature, 30.0 s without, required `< off / 2` (15.0), a boundary tie; its selftest then reports 1 fixture not as expected (P0) |

G93, G157 and G165 are not on CLAUDE.md's list of known reds. None of them exercises the new code: they run default-arm or preset runs in which the flag is off and the arm unselected, and the
null controls above show those runs byte-identical to the frozen binary's (1620 + 279 files), so their verdicts cannot differ from the base commit's. **That is an argument from byte-identity, not a re-run
of these three gates on the base commit**; it was not independently confirmed and I report them as red.

## Provenance and what is not claimed

- Source files: `scratch/ca_tbl_causal_sim.cc` (flag, arm), `src/lora-physics-engine/model/ca-tbl-policy.{h,cc}` (`skipTier2`), `scripts/limits_run.sh`, `scripts/limits_analyse.py`, `scripts/limits_probe_table.py`,
  `scripts/two_tier_gate.{sh,py}`, `scripts/two_tier_gate_selftest.sh`, `scripts/catbl_gates.sh` (G167), `report/PREREG_LIMITS_2026-11-14.md`.
- The L1 flag-only commit (`5ec5373f`) was assembled from the full diff by script and was not compiled on its own; the binary was built from `fcf1cdc0`, which contains both.
- Nothing about `poucet_fusion` under reduced tier-1 reach, about tier-1 ranging accuracy under a real loss, or about any environment other than forest in L1 is claimed.
- The Ferreira and Petäjäjärvi setups were not read from full texts (section 1a).
