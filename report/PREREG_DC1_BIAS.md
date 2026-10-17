# PREREG DC1-BIAS — the preset with a gyro that has a turn-on bias

**Registered before any cell runs** (rule 13). Written 2026-10-17. Engine: `main` at `93695ac`
(`--imuGyroBiasSigmaDps`, landed `3f0f618f` with its null control). This commit adds this file,
`scripts/dc1_bias_run.sh` and `scripts/dc1_bias_analyse.py`, and nothing else. Outputs go to
`results/dc1_bias_2026-10-17/`, which does not exist at registration.

**Why.** TECH_DEBT N.28: DC-1 draws its per-run gyro bias with sigma 8.816 °/h — the MPU-6050's
bias *instability*, the floor of a running gyro's Allan deviation — not the turn-on and
temperature residual a dead reckoner faces. That is ≈ 2.4° of heading error in 1000 s. **The
preset turns DC-1 on** (`imuPredict`), and with it Case D (one range plus the IMU), the IMU-model
integrity witness and every subject's witness. So every preset result — CAMP-FINAL, the LoRa
comparisons, FIXES-N26, the paper tables — was produced with a near-perfect gyro. This campaign
measures what a realistic bias moves. It adopts nothing: changing the default is the owner's call.

## 1. Cells (`dc1_bias_run.sh`)

`--specProfile=true --duration=1800 --trace=true`, all six comparator arms per invocation
(`ca_tbl_full,static_868,static_2p4,fixed_threshold,poucet_fusion,lora_fusion`), configurations
A (5 km, `--reportAirtime=true`), B (endgame, `--boxHalf=750 --finderMode=1 --reportAirtime=true`)
and C (N8 ring, `--nGateways=8 --gwLayout=legacy`), as D8-MAIN.

| level | flag | what it represents |
|---|---|---|
| **L0** | none | the preset as it stands: 8.816 °/h (0.00245 °/s) |
| **L2** | `--imuGyroBiasSigmaDps=0.15` | the BMI270 data sheet: power-on calibration, 10 K of drift (the 10 K is **ours**) |
| **L3** | `--imuGyroBiasSigmaDps=0.5` | calibration-limited: the BMI270's zero-rate offset tolerance |

× seeds **1–20**, paired by seed: **180 cells.** (A 0.05 °/s level was drafted and dropped before
registration, on cell cost: ≈ 20 min per six-arm cell at 11-way parallel on this machine.) The bias is drawn from the same stream in the
same order at every level; only its magnitude moves. The scale error is unchanged.

## 2. Quantities (`dc1_bias_analyse.py`), `main`'s own estimators

`lora_cmp_analyse.per_seed` per (config, level, arm) — availability, tag 0 CEP50 / CEP95, subjects'
CEP95, TTFF, subject J — and `camp_final_analyse.uwb_fixes` for `ca_tbl_full`; each level paired
with L0 by seed, with `camp_spec_analyse.ci`. The comparison table's contrasts, CA-TBL − dual-band
LoRa fusion and CA-TBL − Poucet, at every level.

## 3. Predictions, made before any cell

| # | prediction | basis |
|---|---|---|
| **R1** | L0 cells reproduce D8-MAIN's **off** cells (`arm_summary.csv`, all six rows) byte for byte, 60 of 60 | same preset, same binary content (the commits between are documents and scripts only) |
| **P1** | `ca_tbl_full` tag 0 CEP95 rises at L3 against L0 in A and in B — paired interval above 0 | 0.5 °/s is ~200× the L0 sigma; Case D and the sparse cases solve against the IMU-propagated prior |
| **P2** | in B, `ca_tbl_full`'s mean tag 0 CEP95 does not decrease from L0 to L2 to L3 | heading error grows with the bias; the endgame is where Case D and the IMU prior carry fixes |

**Reported, not predicted:** availability (a fix counts whatever its error, so it need not fall);
every arm's paired change; the comparison contrasts at every level — whether any changes sign or
significance; energy; UWB-delivered fixes; the integrity witness's SUSPECT share is not computed.

**R1 failing does not abort**: every level comes from one binary, so the paired comparisons
stand; it is reported first.

## 4. Abort conditions

Not 180 cells, or a cell's `rc` not 0; a `run_id.txt` not `dirty 0`, or more than one source;
a L2 or L3 cell whose log lacks its `DC-1: gyro bias sigma` line, or a L0 cell with one; a
(config, level, arm) with fewer than 20 seeds.

## 5. Disclosure

Before this commit: a 10 s smoke run at seed 99 with `--specProfile --imuGyroBiasSigmaDps=0.15`
(the flag is accepted under the preset; that seed drew −0.115 °/s). D8-MAIN (`52b8ebac`) ran and
its off cells exist; **its results had not been read when this was committed** — the D8-MAIN scorer
had not been run.

## 6. What this cannot establish

Anything about DC-1's scale error, gyro noise density, or the SHS gait engine; whether a 5-state
filter that estimates the bias recovers the loss (`docs/LOCAL_LINE_INTEGRATION.md` §4.3); what
the default should be.
