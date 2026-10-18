# V1-RNGKEYED results — keyed draws work; the registered distribution check failed on 10 seeds and passed on 30 (registered, exploratory)

Registration: `report/PREREG_V1_RNGKEYED.md` at `5c5ab1b9`, committed before any cell. 40 runs, all `rc 0`,
`source 5c5ab1b9 dirty 0`, frozen binary with a SHA-256 manifest. Tracked per cell: `arm_summary.csv` or
`probe_range_sweep.csv`, and `run_id.txt` (`results/v1_2026-10-16/`).

## 1. Verdicts (registered, seeds 1-10)
| # | prediction | outcome |
|---|---|---|
| V1-1 | r80 keyed within ±5 % of unkeyed per band (UWB ±1 m) | **not held**: 868 MHz +8.1 % (1513.6 → 1635.5 m); 2.4 GHz −1.4 %; UWB −0.6 m |
| V1-2 | pooled PDR curves within 0.05 at every distance | **held** (max 0.032, 0.018, 0.026) |
| V1-3 | paired-difference SD smaller keyed | **not held**: ratio 3.35, driven by one keyed seed (s5: −32.9 m) |
| V1-4 | no systematic shift in arm CEP95 | **held** (shift/SD 0.08–0.29) |

**Abort condition, as recorded:** the keyed PROBE cells carry no `RNGKEYED` residue line, because the probe
returns before main's closing report. The line was missing, not nonzero. This is fixed on the next branch,
where the probe prints it. 20 exploratory keyed probe runs there all print `unkeyed_draws=0`.

## 2. Locating V1-1's cause (exploratory, after the read-out; not registered)
The registration required the cause to be located before any campaign uses the flag.
- **Keyed generators.** A moment test against the ns-3 streams (400 000 draws each; uniform, normal,
  truncated normal, log-normal, exponential, Nakagami m = 0.5/1/3) agrees within 4 standard errors for
  every law. The draws are exact.
- **Per-term probe** (`--probeTerms`, next branch), 3 seeds. The 868 MHz margin difference of about +1.9 dB
  sits in two terms:
  - TX hardware variation, one persistent draw per node per run (−0.03 vs −0.79 dB);
  - fading, frozen per site after the probe's first distance (below).

  Both are few-sample terms.
- **20 more seeds** (11–30): the 868 MHz r80 mean is **1575.4 m in both modes**. Over all 30 seeds:
  - 868 MHz: 1555 vs 1595 m (+2.6 %, SE about 19 m each);
  - 2.4 GHz: 150.8 vs 147.9 m;
  - UWB: 7.5 vs 6.9 m.

  The 10-seed 868 MHz shift was sampling. UWB's −0.6 m is within the registered ±1 m but about 2.7 SE;
  it is noted, not explained.
- **Decision under the registration:** cause located (sampling, plus few-sample persistent terms). Later
  campaigns may use `--rngKeyed=true` with the r80 values above as its reference.

## 3. Defects found while doing this (recorded in TECH_DEBT by the next branch)
1. **The 868 MHz ranging error leaks into 868 MHz reception.**
   - The 868 ranging branch writes `propagationLossBiasDb = 10 n log10(est/true)`, and Step 14 adds it to
     the link budget. The 2.4 GHz and UWB engines zero it first; the 868 engine does not.
   - Probe, 3 seeds: 868 r80 is 1492 m by default, 1194 m with `--rssiRangeFromShadowing` (the shadowing
     counted twice), and 1710 m with the leak removed, whether S1 is on or off.
   - **Consequence:** every 868 MHz reach and availability figure carries a spurious random loss.
     B4-5's large S1 effect (868 CEP95 826 → 1974 m at 5 km) is partly this leak.
   - Fix: `--rangeErrorNoLinkLeak`, next branch.
2. **The probe restarts time at each distance,** so a link's fading, which updates only when time has advanced
   0.15 s past its last update, is frozen after the first distance. That gives fewer independent fading
   samples per r80 than the 2000 trials suggest.

## 4. Analysis output (`scripts/v1_analyse.py`, seeds 1-10)
# V1-RNGKEYED results (exploratory)

## PROBE

| band | r80 unkeyed (m) | r80 keyed (m) | change | max abs PDR difference | V1-1 | V1-2 |
|---|---|---|---|---|---|---|
| 868MHz | 1513.6 | 1635.5 | +8.1 % | 0.032 | not held | held |
| 2.4GHz | 149.9 | 147.8 | -1.4 % | 0.018 | held | held |
| UWB | 7.3 | 6.7 | -7.6 % | 0.026 | held | held |

## PAIR (config B, 900 s, 10 seeds)

| arm | CEP95 unkeyed mean (SD) | CEP95 keyed mean (SD) | shift / max SD | V1-4 |
|---|---|---|---|---|
| static_868 | 10.2 (3.5) | 11.3 (9.6) | 0.12 | held |
| static_2p4 | 45.6 (35.2) | 35.2 (26.2) | 0.29 | held |
| ca_tbl_full | 11.7 (6.5) | 13.1 (7.5) | 0.18 | held |
| fixed_threshold | 12.8 (5.5) | 13.9 (13.4) | 0.08 | held |

Paired difference CEP95(ca_tbl_full) - CEP95(fixed_threshold): SD unkeyed 3.54 m, keyed 11.86 m, ratio 3.35

## Predictions

- **V1-1**: not held
- **V1-2**: held
- **V1-3**: not held (ratio 3.35)
- **V1-4**: held

Abort conditions: /home/nethi/catbl_work/campaigns/v1/PROBE_ktrue/s1: unkeyed draws None; /home/nethi/catbl_work/campaigns/v1/PROBE_ktrue/s10: unkeyed draws None; /home/nethi/catbl_work/campaigns/v1/PROBE_ktrue/s2: unkeyed draws None; /home/nethi/catbl_work/campaigns/v1/PROBE_ktrue/s3: unkeyed draws None; /home/nethi/catbl_work/campaigns/v1/PROBE_ktrue/s4: unkeyed draws None; /home/nethi/catbl_work/campaigns/v1/PROBE_ktrue/s5: unkeyed draws None; /home/nethi/catbl_work/campaigns/v1/PROBE_ktrue/s6: unkeyed draws None; /home/nethi/catbl_work/campaigns/v1/PROBE_ktrue/s7: unkeyed draws None; /home/nethi/catbl_work/campaigns/v1/PROBE_ktrue/s8: unkeyed draws None; /home/nethi/catbl_work/campaigns/v1/PROBE_ktrue/s9: unkeyed draws None
