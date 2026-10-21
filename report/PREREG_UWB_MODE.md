# PREREG UWB-MODE — tier 3 on one DW3000 operating mode, chosen on the requirement

**Registered before any cell runs** (rule 13). Written 2026-10-21. This commit adds this file,
`scripts/uwb_mode_run.sh` and `scripts/uwb_mode_analyse.py`, and nothing else. Outputs go to
`~/ns/catbl-work/uwb_mode_2026-10-21/`, which does not exist at registration.

## Why

Tier 3 mixed three DW3000 operating modes at once (TECH_DEBT 2.4):

| what the model takes from the mode | legacy (every result so far) | DW3000 Data Sheet v1.1, Table 9, ch 9 |
|---|---|---|
| frame airtime (MAC, energy) | 128-symbol preamble at 6.81 Mb/s | — |
| ranging integration | 1024 symbols | — |
| sensitivity (preset, DD 18) | −99.3 dBm | the **850 kb/s, 1024-symbol** row |

`--uwbMode` (`447a8348`, G164) puts all three on one row:
- **6m8:** 64-symbol preamble, 6.81 Mb/s, ranging over 64 symbols, −93.2 dBm;
- **850k:** 1024-symbol preamble, 850 kb/s, ranging over 1024 symbols, −99.3 dBm.

**Legacy is not a candidate.** It bills a frame the receiver could not decode at the sensitivity it is given. The
preset will carry one consistent mode either way. Which one is a **deployment choice the engineer makes when
configuring the radio**, not a property of the world, so choosing it on the requirement is legitimate. That is the
difference from DESIGN_DECISIONS 18, where the physics was not chosen by its effect. The owner asked to "do both
and document the best result".

## Cells

- **L:** the preset as it stands (`94bddffc` + `447a8348`); the reference, reported, not a candidate.
- **M6:** L + `--uwbMode=6m8`.
- **M8:** L + `--uwbMode=850k`.

The mode sets the sensitivity and replaces the preset's `--uwbSensitivityDbm=-99.3`.

**Configurations:**
- A: 5 km.
- B: endgame, `--boxHalf=750 --finderMode=1`.
- C: the 8-gateway ring, `--nGateways=8 --gwLayout=legacy`.

All five arms; `--duration=1800 --trace=true --traceCovariance=true --reportAirtime=true`. Seeds 261–280, fresh:
180 cells, one frozen binary checked before every cell.

## Predictions (no pilot was run)

Basis: the mechanism only. M8 has 6.1 dB more sensitivity and 12 dB more coherent ranging gain than M6, at about
10.4× the airtime per frame (G164 BILL: 1317 against 126 µs for 20 bytes). M8 and L differ only in what a frame costs.

| # | prediction | basis |
|---|---|---|
| U1 | M8 raises CA-TBL five-node A(25 m) against M6 in B: interval above 0 | reach and ranging precision |
| U2 | M8 raises CA-TBL subject J against M6 in B: interval above 0 | airtime ×10.4 on every UWB frame |
| U3 | L − M8 CA-TBL five-node A(25 m) in B lies inside [−0.02, 0.02] | same decode and ranging, different billing |
| U4 | M6 lowers CA-TBL five-node A(25 m) against L in B: interval below 0 | 6.1 dB less sensitivity, 12 dB less ranging gain |
| U5 | M8's CA-TBL mean five-node A(25 m) in B ≥ 0.95 | PRESET-3 NEW 0.982 |

**Against them, stated now:** in G164's seed search, tag 0 used UWB in 0–20 of 900 cycles in the endgame. If tier 3
carries few cycles, every UWB contrast may sit inside rule-8 noise, and U1, U2 and U4 fail on width. That would be
reported as such.

## Adoption rule (registered; between M6 and M8 only)

The first step that separates them decides, judged on `ca_tbl_full`, paired M8 − M6, 95 % bootstrap interval:
1. five-node A(25 m) in B, the interval excluding 0: the higher wins;
2. else tag-0 A(25 m) in A, the same: the higher wins;
3. else five-node A(25 m) in C: the higher wins;
4. else subject J in B: the lower wins;
5. else **850k**: the row whose sensitivity the preset already carries, so the smallest change to what is decodable.

The winner replaces `--uwbSensitivityDbm=-99.3` in `kSpecProfile` with `--uwbMode=<winner>` (DESIGN_DECISIONS 23).
**The cost of moving from L to the winner is reported as the size of the correction, not weighed against it.** L is
inconsistent, and a correction is not refused because the inconsistent model looked better.

## Abort conditions

- not 180 cells, or a cell's `rc` not 0;
- a sidecar not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 261–280;
- a cell whose UWB mode line does not match its variant, or whose preset lines are missing;
- any window solver failure;
- an arm with fewer than 20 seeds.

## What this cannot establish

- **Other DW3000 rows.** The no-data and PDoA rows, and other preambles, are not run.
- **Real hardware.** Table 9 is a conducted 1 % PER figure; no link is measured here.

---

# RESULT — after the registration above (`2f76d9c9`)

**Run.** Launched 2026-10-21T06:54:38Z from `2f76d9c9`, dirty 0. Done 08:51:56Z. 180 of 180 cells, every `rc` 0, one
source, no window solver failure; **problems none**. Scored once with the committed scorer
(`results/uwb_mode_2026-10-21/UWB_MODE.md`).

| # | verdict | measured (CA-TBL, paired, 20 seeds) |
|---|---|---|
| U1 | FAILS | M8 − M6 endgame five-node +0.003 [−0.004, 0.013] |
| U2 | **FAILS, opposite sign** | M8 − M6 endgame subject J **−0.65 J [−1.02, −0.30]** |
| U3 | FAILS (on width) | L − M8 endgame five-node −0.014 [−0.037, 0.001] |
| U4 | FAILS | M6 − L endgame five-node +0.011 [−0.010, 0.037] |
| U5 | HOLDS | M8 endgame five-node mean **0.964** |

## Adoption, by the registered rule

| step | M8 − M6 | separates |
|---|---|---|
| 1 five-node A(25 m), B | +0.003 [−0.004, 0.013] | no |
| 2 tag-0 A(25 m), A | 0.000 [0.000, 0.000] | no |
| 3 five-node A(25 m), C | +0.002 [0.000, 0.007] | no |
| 4 subject J, B | **−0.65 J [−1.03, −0.30]** | **yes: 850k is lower** |

**`--uwbMode=850k` is adopted** (DESIGN_DECISIONS 23).

## What it says

- **The UWB mode barely moves availability.** Tier 3 carries little: CA-TBL's UWB airtime per endgame run is
  0.004–0.05 s. Every A(25 m) contrast between modes is within ±0.015.
- **U2 was wrong about the mechanism.** 850k frames cost 10.4× the airtime, yet subject energy fell. Tier-3 frames
  are a negligible share of the bill, and 850k's 6.1 dB of extra sensitivity and 12 dB of ranging gain saved more
  elsewhere than its frames cost. The prediction priced the frames and not what they buy.
- **The exact zero in step 2 is not a measurement artefact.** Tag-0 traces differ between L and M8 in 20 of 20 seeds
  in every configuration (endgame median error 0.96 → 0.91 m). The set of cycles within 25 m is identical in every
  seed: UWB refines positions already well inside 25 m, and the misses are cycles neither mode places inside it.
- **The correction costs nothing measurable against L.** M8 − L, CA-TBL: endgame five-node +0.014 [−0.002, 0.038];
  5 km and ring within ±0.001; subject J −0.19 [−0.69, 0.32].

## CA-TBL against the comparators under the adopted mode (M8)

| configuration | fixed_threshold, five-node | Poucet fusion, five-node | static 2.4 GHz, five-node | subject J against Poucet |
|---|---|---|---|---|
| A, 5 km | −0.029 [−0.063, 0.000] | **+0.131 [0.074, 0.189]** | **+0.088 [0.024, 0.151]** | −445 J |
| B, endgame | −0.004 [−0.012, 0.003] | **+0.060 [0.026, 0.096]** | +0.028 [−0.017, 0.075] | −393 J |
| C, ring | −0.008 [−0.083, 0.059] | +0.039 [−0.044, 0.111] | +0.019 [−0.057, 0.079] | −445 J |

The PRESET-3 reading stands on fresh seeds: CA-TBL is level with `fixed_threshold`, ahead of Poucet fusion at about
1/30 its subject energy, and ahead of static 2.4 GHz at 5 km.
Against `fixed_threshold` CA-TBL now spends less in the endgame: subject J −1.22 J [−2.25, −0.29] at level
availability.
