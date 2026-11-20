# REALISTIC-1 — pre-registration (DRAFT, to be committed before the first cell)

**Question.** On realistic infrastructure, which design finds an injured worker, how fast, and at what battery cost?
Plan: `~/catbl_work/tii_paper/PLAN_REALISTIC.md`, Stage 1. Deployment: DESIGN_DECISIONS 27.

**Binary.** Frozen at launch by `scripts/realistic1_run.sh` (SHA-256 checked before every cell); source = the commit
adding this file; `dirty 0` on every cell.

**Timeline (every run, every design).** The lost worker (node 1) leaves the route at 30 min and stops 10 min later
(injured). The alarm (overdue report) is at **50 min**, identical for all designs. Searchers, if any, leave the camp at
the alarm. Runs end at 2 h, so "never" means not within 70 min of the alarm.

**Where the position is calculated.** On each worker's own device, for every design (`--gatewayEdgeServer=false
--edgeUplinkRanges=false --subjectEdgeUplink=false`; owner decision 2026-11-20). Stations measure the 868 MHz broadcast
and reply; the device runs the filter, so it keeps a position from its motion sensor and any searcher or peer it can
range to when no station hears it. Battery counts each station reply; processor energy is not modelled (stated
limitation). Exploratory check (seed 7, one station): positions identical to station-side estimation, battery +7 % for
our system and +20 % for 868 MHz only.

**Height.** Ground is flat; stations stand on 15 m masts and people carry devices at 1.5 m. The radio model uses true
3-D distances; the filter estimates horizontal position only, so every run projects each mast range onto the ground
with the surveyed heights (`--rangeHeightCorrect=true`, DESIGN_DECISIONS 28, gate G172). Terrain elevation enters only
as signal loss; slopes and multi-level sites are out of scope.

**Sites.**

| site | stations | layout |
|---|---|---|
| ROAD1 | 1 | on the access road (line), camp at the trailhead |
| ROAD3 | 3 | along the access road (the DD 27 preset) |
| CLUS3 | 3 | clustered round the site office, open terrain (quarry / open pit) |
| RING12 | 12 | the earlier ring with 12 finders (`PIN_PRE_DD27`), **reference only** |

Searchers: **2** (F2) or **none** (F0). Seeds **501–520**. Designs: our system (`ca_tbl_full`), flat-threshold
switching, 868 MHz only, 2.4 GHz only, all radios on (one run, all five arms), and the TII-2 rule (a second run).
Plus, at the three realistic sites with 2 searchers, the five arms again under the field-measured 2.4 GHz error law
(`--tofErrorModel=muller`, Mueller et al. 2021; cells `<SITE>_F2_MUL`). 380 runs, ~8–10 h with 6 in parallel.

**Measures** (`scripts/realistic1_analyse.py`, committed with this file): from the alarm, time to the lost worker's
first estimate within 25 m and within 5 m, and time until a searcher is within 10 m of them; the share of all five
workers' cycles with an estimate within 250 m; battery (J per hour per worker). "Never" is counted as 4200 s (the
time left after the alarm) and its share is reported beside every mean. Paired by seed, 95 % bootstrap intervals.

## Predictions (fixed now)

- **H1.** In every realistic cell (ROAD1, ROAD3, CLUS3 × 2 or 0 searchers), our system's time to 25 m is no worse
  than the better single-radio design by more than **2 min** (paired upper bound), and each single-radio design is
  worse than ours by more than **5 min** (paired lower bound) in at least one realistic cell.
- **H2.** With 2 searchers, in every realistic site, a searcher reaches the worker no later under our system than
  under 2.4 GHz only by more than 2 min, and sooner by more than 5 min in at least one site.
- **H3.** In every realistic cell, our system's 250 m availability is no worse than the better single-radio design's
  by more than **0.02**.
- **H4 (reported, no threshold).** Battery per hour of every design, and TII-2 minus our system with its interval.
- **H5 (reported, no threshold).** The same measures under the field-measured 2.4 GHz error, with our system minus
  2.4 GHz only on time to 25 m and searcher reach, per realistic site.

**Exploratory pilot (seed 7, not citable)** motivated the design: with one station on the road, 2.4 GHz only and
868 MHz only never reached 25 m after the alarm while our system did within 2.5 min; with the 2.4 GHz-only design the
searchers never reached the worker in any site with searchers, which suggests the position does not reach them
without the long-range network.

**Abort / completeness.** Any rc ≠ 0, not `dirty 0`, a manifest mismatch, an alarm not at 3000 s, or fewer than 20
seeds in a cell → incomplete; H1–H3 are not evaluated until complete. No cell dropped or chosen.

**Reporting.** `REALISTIC1.md` by the analyser; a Result section is appended here after the run, whatever it says.
