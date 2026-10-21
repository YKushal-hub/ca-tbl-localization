# CA-TBL — Confidence-Aware Tier-Based Localization

A fork of [ns-3.47](https://www.nsnam.org/) that adds a three-tier radio localisation simulation for GPS-denied search
and rescue. Upstream ns-3 is unmodified except where noted. The contribution is listed under
[Where the work is](#where-the-work-is).

## What this is

An ns-3.47 simulation of a three-tier radio localisation system for GPS-denied search and rescue:
- 868 MHz LoRa (SX1262) for wide-area coverage;
- 2.4 GHz LoRa (SX1280) time of flight for mid-range refinement;
- UWB (DW3000) for short-range precision.

A decision layer escalates between tiers on a position-confidence score, and a 5-state IMU filter carries the position
between fixes. Five arms are compared against identical channel realisations, with per-action energy accounting
traceable to datasheet currents:
- `ca_tbl_full`, the system;
- `fixed_threshold`, a flat variance threshold;
- `poucet_fusion`, LoRa + UWB fusion after Poucet, IPIN 2021;
- `static_868` and `static_2p4`, single band.

**The design requirement** (`Architecture.md` §1.3): availability **A ≥ 0.95 at R ≤ 25 m**, the share of all cycles
with a position within 25 m. A cycle with no fix counts against it.

## The deployment, and the preset

**Every node carries all three radios; a tier is a choice of radio, not of peer.** Classes differ in mobility and
role only:

| class | mobile? | ids | duty cap |
|---|---|---|---|
| **gateway** | no; surveyed, on a 15 m mast | `101+` | 10 % |
| **finder**, a searcher with GNSS | yes | `501+` | 2.5 % |
| **subject**, a field node being localised; tag 0 is node 1 | yes | `1`–`5` | 2.5 % |

**Claims are drawn from the preset, `--specProfile=true`** (`kSpecProfile[]` in `scratch/ca_tbl_causal_sim.cc`; any
flag given explicitly wins). It is the brief's deployment:
- a 5 km search box, 5 subjects all measured;
- 3 gateways in the star layout;
- 8 finders tasked in pairs.

It also fixes the realistic world and the decisions recorded in `docs/DESIGN_DECISIONS.md`:
- the IMU: 0.15 °/s gyro bias, 5 % stride noise, a magnetometer, and the 5-state filter with a 60-cycle window;
- every receiver on its data-sheet row: 2.4 GHz on SX1280 Table 6-1 at 812.5 kHz; UWB on DW3000 Table 9 at
  850 kb/s (DD 23);
- the integrity check off (DD 19);
- dead nodes neither relay nor draw (DD 24).

`RULES.md` lists every preset entry and how to pin one back.

## Where the requirement stands (2026-10-21, CA-TBL, preset, fresh 20-seed sets)

| geometry | tag 0 A(25 m) | five-node A(25 m) |
|---|---|---|
| endgame (`--boxHalf=750 --finderMode=1`) | **0.997** | **0.96–0.98** |
| 8-gateway ring (`--nGateways=8 --gwLayout=legacy`) | **0.98–0.99** | 0.91–0.94 |
| the brief's 5 km, 3 gateways | 0.93–0.98 | 0.83–0.84 |

Sources: `report/PREREG_PRESET3.md`, `report/PREREG_UWB_MODE.md`, `report/PREREG_DISSEM.md`.
- **Met:** tag 0 in the endgame and on the ring, and the endgame five-node pool.
- **Short:** the 5 km five-node pool. No node there is within 1.5 km of a gateway, so the shortfall is anchor supply
  (DISSEM's addendum). `report/PREREG_DEPLOY_REQ.md` measures what deployment would meet it.

**Against the comparators:**
- **Poucet fusion:** CA-TBL is above it on the five-node pool in the endgame (+0.05 to +0.06) at about 1/30 of its
  subject energy.
- **`fixed_threshold`:** CA-TBL is level with it on availability.
- **Single-band arms:** CA-TBL is above static 868 MHz everywhere. It is above static 2.4 GHz on tag 0 at 5 km and on
  the ring, and level with it on tag 0 in the endgame.

**Not claimed** (Technical Report CR-24):
- that the escalation rule beats a flat variance threshold;
- that the confidence score carries information beyond the covariance trace (the PCS novelty claim is withdrawn);
- that the integrity check pays.

The contribution is the assembly and its quantification, and the measurement apparatus (`Architecture.md` §13.2).

`CA-TBL_Technical_Report_SelfContained.md` is authoritative for results and claims, with a corrections register (CR-1
to CR-26). `report/GOAL_STATUS.md` tracks the requirement campaign by campaign.

## Where the work is

```
src/lora-physics-engine/model/ca-tbl-*.{h,cc}          policy (EKF, PCS, gates, escalation), MAC, energy model, header
src/lora-physics-engine/model/lora-*-physics-engine.*  868 MHz / 2.4 GHz / UWB engines (submodules/: ~35 per engine)
scratch/ca_tbl_causal_sim.cc                           the scenario: geometry, mobility, arms, preset, 315 flags
scratch/ca_tbl_packet_validator.cc                     MAC unit suite
scratch/ca_tbl_sensitivity_probe.cc                    each engine's sensitivity accessor, for G93
scripts/catbl_gates.sh                                 the regression gates, G1-G167 (141 blocks)
scripts/*_run.sh, scripts/*_analyse.py                 registered campaigns: runner and scorer, committed before any cell
report/PREREG_*.md                                     every registered campaign: predictions, rule, abort conditions, result
results/<campaign>/                                    tracked summaries, sidecars and raw-file SHA-256 manifests
```

## Build and reproduce

```bash
# --build-profile=optimized is REQUIRED: every cited number was produced under it (G92 refuses a citation that was
# not), and the default profile enables logging, about 10x slower.
./ns3 configure --enable-examples --build-profile=optimized && ./ns3 build
bash scripts/catbl_gates.sh       # every gate runs and is summarised; it never stops at the first red
```

**One preset run**, all five arms, the endgame, 10 minutes of simulated time, scored on the requirement.
`goal_report.py` reads a campaign layout, `<dir>/<GROUP>_<name>/s<seed>/`:

```bash
mkdir -p /tmp/catbl_demo/B_endgame/s7
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --specProfile=true --seed=7 --duration=600 \
  --boxHalf=750 --finderMode=1 --trace=true --outdir=/tmp/catbl_demo/B_endgame/s7
python3 scripts/goal_report.py /tmp/catbl_demo
```

Do **not** write new runs into `results/`: that tree is tracked evidence. Each run writes `run_id.txt`, with the
invocation, build profile, source commit, dirty count and a SHA-256 per output file. `dirty 0` or it is not citable.

**Expected reds in the full suite**, each by owner decision and recorded in `RULES.md`:
- **G11:** the analytic model against the engine, kept red with its reasons;
- **G93:** the code default's tier-2 sensitivity row; the preset carries the data-sheet row.

## Documents

| file | contents |
|---|---|
| `CA-TBL_Technical_Report_SelfContained.md` | claim-by-claim record: **authoritative for results** |
| `Architecture.md` | the system as built: **authoritative for design** |
| `RULES.md` | working rules, the preset list, the expected reds; **read first if you are changing anything** |
| `docs/DESIGN_DECISIONS.md` | every choice the brief does not make, with what it was chosen over (DD 1–24) |
| `docs/TECH_DEBT.md`, `docs/TECH_DEBT_STATUS.md` | every known defect and limit, and its status |
| `docs/SCENARIO_FLAGS.md` | all 315 scenario flags, regenerated from `--PrintHelp` |
| `docs/WORK_PACKAGES.md` | what is open, with entry points |
| `report/GOAL_STATUS.md` | the requirement, campaign by campaign |
| `report/PREREG_*.md` | registered campaigns, each with its result |
| `Simulation.md`, `MASTER_PLAN.md` | specifications and the work programme; strategy |
| `SETUP_NEW_MACHINE.md` | a fresh clone to a green suite |
| `HANDOFF_*.md`, `*_AUDIT*.md`, `VERIFICATION_*.md`, `LOCAL_RUN_*.md`, `WORKSTREAMS_*.md`, `simstatus.md`, `docs/archive/` | **dated records**: true on their date, kept for their findings, not current |

## A note on what is shipped

`results/` holds 25 050 tracked files (about 350 MB): every campaign's `arm_summary.csv`, `subject_summary.csv`,
`run_id.txt` and a SHA-256 manifest of its raw files. Every table in the Technical Report is sourced from a file in
this repository. Per-cycle `trace_*.csv` are not tracked (tens of GB); they regenerate from each cell's `run_id.txt`,
and runs are deterministic.

The repository ships the evidence for its negative results as well as its positive ones: withdrawn claims, failed
predictions, and campaigns whose registered rule refused a mechanism (most recently DISSEM).

## Licence

ns-3 is licensed **GPLv2**, and this is a derivative work to which that licence applies. See `LICENSE`. Upstream ns-3
documentation is preserved in [`README.ns3.md`](README.ns3.md); upstream authorship is unmodified in the git history.
