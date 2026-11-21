# COLLAPSE-1 — pre-registration (committed before the first cell)

**Question.** In a collapsed light-industry building, does the CA-TBL system locate trapped workers through the
rubble, for 20 workers and for 30 as well as for 1? How does it compare with each radio alone? How do the answers
change with building height and with the published spread of rubble loss? It uses radio, IMU and GPS only; no
building plans or site priors reach the system (World/system split, `docs/HANDOFF_COLLAPSE_MULTIWORKER.md`).

## Fixed design (every number sourced in `docs/SOURCES_COLLAPSE.md`)

| Item | Value | Row |
|---|---|---|
| Building | light industry (IND2), square footprint 39.74 m | HAZUS-IND-AREA |
| Workers per floor | 28.8, so N = 20 and 30 trapped workers, plus N = 1 | HAZUS-IND-AREA |
| Storeys | 2, 5, 8 (HAZUS typical low- and mid-rise; 8 = Rana Plaza) | HAZUS-STOREYS, RANA-WORKERS |
| Storey height | 3.048 m (concrete frame) | HAZUS-STOREY-C |
| Pile height | 0.15 × intact height: 0.914 / 2.286 / 3.658 m | PILE-ALPHA |
| Burial depth | uniform in [0.1 m, pile height]; the lower end is a modelling choice (a covered device), reported | — |
| Rubble loss and delay | brackets low / central / high | OES-*, UWB8-*, ETA-*, SLOW-BR |
| Stations | 4 on a ring at half the footprint + 1.5 × intact height: 29.01 / 42.73 / 56.45 m (`--boxHalf` = R / 0.6: 48.36 / 71.22 / 94.08) | NIOSH-CZ |
| Searchers | the preset's 2 finders, dispatched at the collapse (t = 1 s), tasked by need (`--finderTasking=need`) | DD 27 |
| Duration | 1800 s (the first half hour) | — |

**Flags on every cell** (REALISTIC-6's device-side, search-party and re-acquisition set, plus the collapse
mechanisms): `--specProfile=true --trace=true --gatewayEdgeServer=false --edgeUplinkRanges=false
--subjectEdgeUplink=false --rangeHeightCorrect=true --finderSweepFromDispatch=true --finderDigestOnArmTier=true
--finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500 --interferenceFrameKey=true --collapse=true
--gwRidge=false --baseCampFrac=0 --nGateways=4 --searchLaunchAtS=1 --relaySlots=false`.
- N = 20 and 30 add `--nTags=N --measuredSubjects=N --identityQueryManifests=1..N --finderTasking=need`.
- N = 1 adds `$PIN_ONE_WORKER` (`scripts/preset_pins.sh`).

**Arms:** `ca_tbl_full` (ours), `static_868`, `static_2p4`.

**Cells:**
- storeys {2, 5, 8} × bracket {low, central, high} × N {1, 20};
- plus N = 30 at the central bracket for each storey count;
- 21 cells.

**Seeds:** 801–810 (new; never used before). 210 runs.

**Binary:** frozen at launch (`scripts/collapse1_run.sh` copies binary and libraries and checks their sha256 before
every cell).

## Measures (per arm, run and worker; `scripts/collapse1_analyse.py`)

- **L2:** fraction of the N workers whose CEP50 over the run is ≤ 2 m (`subject_summary.csv`).
- **L5:** the same at 5 m.
- **Median CEP50** over workers; worst worker's CEP50.
- **First-fix time:** time of a worker's first trace row with error ≤ 2 m.
- **Energy:** median J per worker over the 1800 s.
- **Digging** (`scripts/collapse/score_dig.py`):
  - expected survivors with 2 teams (INSARAG-HEAVY), t_dig 2 h and radius 2 m;
  - policies `spt` (shortest expected dig time from the system's own uncertainty) and `id`;
  - survival from De Bruycker 1983 Table 2 (DEBRUY-T2).

## Hypotheses (decided before any cell)

- **H1 (locates the crew):** for N = 20, ca_tbl_full's L2 ≥ 0.80 in every low and central cell (all storey
  counts), as the mean over seeds.
- **H2 (scales):** worker 1's CEP50 with N = 20 is not worse than with N = 1 by more than 1 m. Paired by seed;
  median of the paired difference ≤ 1 m in every bracket and storey count.
- **H3 (better than each radio alone):** ca_tbl_full's L2 ≥ static_868's and ≥ static_2p4's. Paired by seed;
  one-sided paired bootstrap, 95% interval of the mean difference above −0.05, in every N = 20 cell.
- **H4 (physics limit, predicted):** in the high bracket at 5 and 8 storeys, every arm's L2 is < 0.5 for
  N = 20. Rubble of 1–3.7 m at 30.7 dB/m (868 MHz) and 100 dB/m (2.4 GHz) exceeds the link margin for most workers.
- **H5 (digging order helps):** for N = 20, `spt` gives at least `id`'s expected survivors (paired by seed;
  mean difference ≥ 0) in every cell.
- **H6 (30 workers):** ca_tbl_full's L2 at N = 30 is within 0.10 of its L2 at N = 20 (central, each storey
  count).

Each hypothesis is reported as stated. A failure is reported, not rescued by a new cell. Anything added after
the first cell is labelled exploratory.

## Known limits (stated now)

- Depth (z) is not estimated by the 2-D filters (plan item B2); CEP50 is horizontal.
- Direct-path loss under deep rubble (Alsindi 2009) is not modelled.
- The 2.4 GHz LOS ranging spread is wider than Zhou 2025's measurement (VAL-4b): conservative for static_2p4.
- Interference is the engines' model with the DD 41 fix. There is no external interferer at the site.
- Stations ring the pile evenly: a sourced standoff, an idealised layout.
