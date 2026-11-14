# PREREG TDOA-TIMING — the ceiling on better tier-1 arrival timing (nanosecond sync, two-pass CFO/STO)

Registered 2026-10-21, before any campaign cell. The runner (`scripts/tdoa_timing_run.sh`) and the analysis
(`scripts/tdoa_timing_analyse.py`) are committed with this file and are not edited after the first cell.

## 1. Question

Two proposals without new hardware improve the arrival stamps of tier-1 (868 MHz) TDoA:
- **nanosecond gateway synchronisation** (proposal list, "Müller et al. 2025");
- **two-pass CFO/STO refinement** of the arrival time.

How much could CA-TBL gain from either? The simulator's stamp is t = t_tx + d/c + e_pps + e_toa + τ_nlos
(`Tier1Tdoa::Stamp`):
- e_pps ~ N(0, 20 ns), the u-blox M8T figure;
- e_toa ~ N(0, the one-way Cramér-Rao bound).

Each proposal can only shrink these two terms, so the test removes them and measures the gain.

## 2. Mechanism

`--tdoaToaScale` (G168): it multiplies the arrival-time sigma in each stamp's draw and in the sigma the solver is
given. It defaults to 1 and is byte-identical there. It must be > 0, because a zero sigma is refused by the solver.

## 3. Design

| | |
|---|---|
| binary | `build/scratch` at `22af153e`'s source; copied with a SHA-256 manifest, verified before every cell |
| common flags | `--specProfile=true --duration=1800 --trace=true --arms=ca_tbl_full,static_868` |
| geometries | **A**: 5 km (preset box); **B**: endgame, `--boxHalf=750 --finderMode=1` |
| configurations | **L**: the preset; **S0**: `--tdoaPpsSigmaNs=0` (perfect sync); **T0**: `--tdoaPpsSigmaNs=0 --tdoaToaScale=0.01` (perfect sync, arrival sigma 1 % of the bound); **N**: `--tier1Tdoa=false` |
| seeds | 341–360 (fresh; the highest registered before is 340) |
| cells | 2 × 4 × 20 = 160 |

## 4. Metrics and statistics

- **Metrics** (from `scripts/goal_report.py` `cell()`):
  - primary: five-node A(25 m);
  - secondary: tag-0 A(25 m), tag-0 CEP95 and five-node J.
- **Statistics:** paired over seeds, bootstrap with 10 000 resamples (NumPy seed 20261021, one resample matrix per
  geometry), 95 % percentile intervals.

**Contrasts:**

| contrast | what it measures |
|---|---|
| SYNC = S0 − L | the ceiling on any gateway-sync improvement, nanosecond sync included |
| TIMING = T0 − L | the ceiling on any arrival-timing improvement, two-pass CFO/STO included |
| TDOA VALUE = L − N | what tier-1 TDoA contributes now, for context |

**Verdicts** (ca_tbl_full, five-node A(25 m), per geometry and contrast):
- **MATERIAL** if the interval lies above 0 and the mean is ≥ +0.02;
- **SMALL** if the interval lies above 0 and the mean is < +0.02;
- **NONE** otherwise.

static_868 is reported but carries no verdict.

## 5. Checks and abort conditions

The analysis exits 1, and no verdict is read, if any of these holds:
1. any `rc` is not 0;
2. the count is not 160 cells, or 20 seeds per configuration and arm;
3. any `run_id.txt` is not `dirty 0`, or more than one source appears;
4. the `TDOA TOA SCALE` line appears in a cell other than T0, or is missing from a T0 cell.

The null control in `22af153e`'s message is a precondition and has passed.

## 6. Predictions (before any cell)

- **D1.** SYNC is NONE in both geometries. 20 ns is 6 m of range difference, small against the arrival-time
  bound: about 15 m at 0 dB SNR and 82 m at −15 dB, assuming 125 kHz and SF12.
- **D2.** TIMING is NONE or SMALL in both geometries. Under the preset's three gateways a TDoA solve is rare: one
  to two per 600 s for ca_tbl_full on seed 7 (G168's runs).
- **D3.** TDOA VALUE is NONE or SMALL for ca_tbl_full.

## 7. Limitations

- **Simulator only.** In the field the arrival-time error sits above the bound (the threshold effect is not
  modelled), so TIMING may understate what refinement buys a real receiver. Even so, it bounds the gain that the
  rest of CA-TBL can turn into availability.
- **Three gateways only.** More gateways would make TDoA solves frequent.
- **No hardware is added.** Both proposals' own hardware claims are not assessed.

# RESULT — after the registration above (`c6f3b4ad`)

**Run.** Launched 2026-10-21T21:05:16Z from `c6f3b4ad`, dirty 0, and done at 21:47:07Z:
- 160 of 160 cells, every `rc` 0, one source, every section 5 check passed;
- scored once (`results/tdoa_timing_2026-10-21/TDOA_TIMING.md`).

**Housekeeping.** As in ORACLE-ESC: each cell was verified against its `run_id.txt` digest, then the files the
analysis does not read were deleted (`results/tdoa_timing_2026-10-21/prune_cells.py`, `prune.log`: 160 verified).

## Registered verdicts (ca_tbl_full, five-node A(25 m), paired 95 %)

| contrast | 5 km | endgame |
|---|---|---|
| SYNC (S0 − L) | **NONE**, +0.001 [−0.000, +0.002] | **NONE**, −0.000 [−0.001, +0.000] |
| TIMING (T0 − L) | **NONE**, −0.015 [−0.037, −0.001] | **NONE**, −0.001 [−0.004, +0.000] |
| TDOA VALUE (L − N) | **NONE**, +0.006 [−0.000, +0.016] | **NONE**, +0.000 [−0.001, +0.002] |

ca_tbl_full's tag 0 is unchanged by any configuration: 0.969 and 0.997.

## static_868 (reported, no registered verdict)

| contrast | endgame five-node | endgame tag-0 A(25 m) | endgame tag-0 CEP95 | endgame five-node J | 5 km |
|---|---|---|---|---|---|
| TIMING (T0 − L) | +0.074 [+0.032, +0.117] | +0.198 [+0.096, +0.303] | −37.8 m | **+4.57 J [+3.69, +5.33]** | +0.021 [+0.000, +0.047] |
| TDOA VALUE (L − N) | +0.153 | +0.607 | −147.3 m | — | — |

The energy rise under T0 is not explained here.

## Predictions

| prediction | outcome |
|---|---|
| D1 | Holds |
| D2 | Holds |
| D3 | Holds |

## Reading

1. **Better tier-1 timing would help an 868 MHz-only system** in the endgame (static_868: TIMING and TDOA VALUE
   both clear their intervals).
2. **It does nothing for CA-TBL.** CA-TBL takes its accuracy from 2.4 GHz time of flight, and its tag 0 does not
   move. So neither nanosecond gateway sync nor two-pass CFO/STO refinement is worth pursuing for CA-TBL as
   configured, with three gateways.
3. **One adverse interval, unexplained.** At 5 km, TIMING for ca_tbl_full lies below 0. Not tested: the solver is
   given a 1 % sigma while τ_nlos stays, so TDoA fixes may become overconfident.
