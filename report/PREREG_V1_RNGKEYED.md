# PREREG V1-RNGKEYED — keyed random numbers change the realisation, not the distribution (exploratory grade)

**Registered before any V1 measurement cell runs** (rule 13). Written 2026-10-16. Engine: the commit that
adds `--rngKeyed` and gate G138. Owner's master plan v3 (kept outside the repository), stage V1; TECH_DEBT 1.1.

## 1. What is disclosed
- Development runs (not measurements) established the mechanism before this registration:
  - Five submodules hold copies of their engine's sampler. They now draw in the source's link context.
  - Two engine-wide states were drawn inside whichever link was evaluated first; under `--rngKeyed` both get
    their own keyed sub-sequence:
    - the weather update, which is now keyed by epoch floor(t / interval);
    - the external-interferer map, which is now laid out once and **centred on the deployment origin**
      instead of on the first evaluated gateway. This is a deliberate model change inside keyed mode and is
      the most likely source of any distribution shift.
  - G138 (seed 7, 300 s, preset): with the perturbation, 0 of the outputs move when keyed and 90 move when
    unkeyed; unkeyed draws = 0.
- MAC objects get one stream each under `--rngKeyed`. Physical coupling (an extra transmission really
  interferes, really occupies airtime) is not removed and should not be.

## 2. Cells
Binary frozen with a SHA-256 manifest; outdir outside the repository; `--foliageDepthClamp=true`.
| group | invocation | runs |
|---|---|---|
| PROBE | `--probe=true --seed=s --foliageDepthClamp=true --rngKeyed={false,true}`, s = 1..10 | 20 |
| PAIR | `--specProfile=true --duration=900 --unknownStart=true --fixedVarThreshold=12 --foliageDepthClamp=true --boxHalf=750 --finderMode=1 --arms=static_868,static_2p4,ca_tbl_full,fixed_threshold --rngKeyed={false,true} --seed=s`, s = 1..10 | 20 |

## 3. Predictions
| # | prediction | confidence |
|---|---|---|
| **V1-1** | Per band (868, 2.4, UWB), mean r80 over the 10 PROBE seeds keyed is within ±5 % of unkeyed (UWB: within ±1 m) | medium |
| **V1-2** | Per band, the pooled PDR curves differ by at most 0.05 at every probe distance | medium |
| **V1-3** | PAIR: the across-seed SD of the paired difference CEP95(`ca_tbl_full`) − CEP95(`fixed_threshold`) is smaller keyed than unkeyed (reported as a ratio) | low |
| **V1-4** | PAIR: arm means of CEP95 keyed vs unkeyed differ by less than their seed-to-seed SD for every arm (no systematic shift) | medium |

V1-1 and V1-2 are the acceptance criteria for using `--rngKeyed` in later campaigns. If either fails, the
cause is located (the interferer centring first) before any campaign uses the flag. V1-3 and V1-4 are reported.

## 4. Abort conditions
A run without `rc 0`, a manifest mismatch, a `run_id.txt` without `dirty 0`, or keyed runs printing
`RNGKEYED unkeyed_draws` other than 0.
