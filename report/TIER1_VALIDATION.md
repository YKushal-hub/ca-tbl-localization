# Tier-1 validation — the engine's 868 MHz loss against public LoRa measurements

**2026-10-09, S3 (HANDOFF_2026_10_08 item 3; WORKSTREAMS_2026_10_09 §6.5).** The figure is
`analysis/tier1_validation/tier1_validation.png`, the numbers are in
`analysis/tier1_validation/tier1_validation.{csv,txt}`, and the script is
`analysis/tier1_validation/tier1_validation.py`. Probe evidence is in
`results/tier1val_2026-10-09/env{1,3}/` (`run_id.txt`: `source 5f8bbf1e dirty 0`, run with
`--outdir` in /tmp and copied).

## 1. Data, and why not Aernouts 2018

- **Used:** Rademacher et al., VTC2021-Fall (arXiv 2109.07768), public set
  github.com/mclab-hbrs/lora-bonn at `74d9e29a`, `survey/samples.csv` (sha256 `0332836a…`) and
  `gateways.csv` (`c7b2e2e2…`). Licence dl-de/by-2-0. Not vendored.
  - Bonn is urban. There are 9 gateways at 12-55 m with 3 dBi antennas, and RFM95 sensors on
    garbage-truck roofs at SF12 / 125 kHz, 14 dBm.
  - The file has 131,057 rows. The paper reports 112,372 after its post-processing (§III), and
    the public notebook applies no further filter.
- **Not used:** Aernouts et al. 2018 (doi 10.3390/data3020013) is on Zenodo, which this cloud
  environment's network policy blocks.
- **Path loss** follows the dataset notebook: rpp = rssi + min(snr, 0), and
  PL = 14 + 3 − 2 − rpp.

**The data's own fit reproduces.** Applying the paper's procedure (median per 10 m group, least
squares) to the public file gives n = 1.597, PL1km = 132.51 dB and residual SD 9.82 dB. The paper
reports 1.58, 132.41 and 9.9 (§IV).

## 2. Engine side

`--probe=true` at the scenario's 868 MHz PHY (SF7 / 125 kHz, 14 dBm; sensitivity −123 dBm,
`lora-physics-parameters.cc:391`), 2000 trials per distance, for two environment classes:
- `--env=3`, **UrbanModerate**, the class matching Bonn;
- `--env=1`, **RuralForested**, the scenario's default for tier 1.

**Total loss** is reconstructed from the probe's `mean_margin_db`, which covers all trials:
L = tx − sens − margin, up to +1.5 dB. The range exists because the margin is taken against an
effective sensitivity 1.5 dB lower whenever the link was receiving (RULES.md §5). The probe's
`mean_pl_db` is the path-loss *term* only, 12.6-29.9 dB (RuralForested) and 24.3-53.3 dB
(UrbanModerate) below the total's lower edge over 100-6000 m. Reading it as the
total would be wrong.

## 3. Result: engine total loss − measured median

| distance (m) | measured n | UrbanModerate (dB) | RuralForested (dB) |
|---|---|---|---|
| 250 | 107 | −0.7 .. +0.8 | −9.4 .. −7.9 |
| 500 | 1 046 | +0.2 .. +1.7 | −15.4 .. −13.9 |
| 1 000 | 5 975 | +6.2 .. +7.7 | −14.6 .. −13.1 |
| 1 500 | 9 075 | +3.6 .. +5.1 | −15.3 .. −13.8 |
| 2 000 | 11 485 | +3.9 .. +5.4 | −13.0 .. −11.5 |
| 3 000 | 15 985 | +5.9 .. +7.4 | −7.2 .. −5.7 |
| 4 000 | 14 492 | +9.3 .. +10.8 | −1.3 .. +0.2 |
| 6 000 | 10 251 | +12.4 .. +13.9 | +5.3 .. +6.8 |

How to read it:
- **The matching class agrees where the data are uncensored.** At 250-500 m UrbanModerate is
  within −0.7..+1.7 dB of the Bonn median. That is well inside the measured 10-90 % spread.
- **Beyond ~1 km the engine is lossier, by 4-14 dB. That figure is an upper bound on the
  disagreement, not a measurement of it.**
  - Packets are recorded only when received, so the measured median past ~1 km is the median of
    the survivors. It is biased low, and the measured median flattens at 141-145 dB past
    1.5 km.
  - The SF12 limit sits near 152 dB (15 dB + 137 dBm, SX1276 datasheet).
  - The engine's figures are over all trials, censored or not (rules 1-2). The size of the
    censoring bias is not estimated here.
- **The scenario's RuralForested class cannot be validated against urban data.** It is 8-15 dB
  less lossy than Bonn to 2 km, which is the expected direction for open rural ground against a
  city. That is a statement about environments, not a validation.

## 4. What this does and does not establish

- **Does:** the engine's urban 868 MHz class reproduces an independent urban measurement
  campaign at short range, and its long-range excess is no larger than 4-14 dB, an upper bound
  because of the censoring.
- **Does not:** validate RuralForested, the class the search scenario uses. No public rural LoRa
  path-loss set was reachable from this environment. Aernouts 2018's rural set is Sigfox
  (868 MHz, also on Zenodo) and is the next candidate.
- **For S2's WP-A6** (WORKSTREAMS §2 E): nothing here shows the path-loss model wrong for the
  scenario. The tier-1 PHY sweep's numbers are not moved by this result.

**Differences not corrected for:**
- Heights: the engine's gateways are Normal(30, 10) m and its node 1 m; Bonn's gateways are
  12-55 m and its sensors on truck roofs.
- Antenna gain: Bonn's gateway gain is 3 dBi, removed by the notebook's definition; the engine's
  is 0.
- The engine's dual-slope and NLOS settings for UrbanModerate are OURS (TECH_DEBT N.42 for the
  exponents).
