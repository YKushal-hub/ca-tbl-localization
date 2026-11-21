# Handoff: collapse extension and the 20-worker end-to-end system

**Branch:** `work/2026-11-22-multiworker`. Status as of **2026-11-22**. This file is the single entry point for a new
session. Read it fully, then `RULES.md`, then `docs/PLAN_COLLAPSE.md`. Update §9 (status log) and the tables below
whenever a substantial goal is done, then commit and push the branch (see §3.4).

---

## 1. What the owner asked for (problem statement)

1. **Scenario.** An earthquake collapses an industrial building (factory). 20–30 workers are trapped. Each wears
   the CA-TBL tri-band device (868 MHz LoRa SX1262, 2.4 GHz LoRa SX1280 with time-of-flight, UWB DW3000).
   - Locate every worker and dig them out as fast as possible.
   - Workers' devices talk to each other, so one contact with a rescuer can locate the whole connected group.
2. **Allowed system inputs:** radio, IMU and GPS only.
   - No barometer, no building plans, no floor heights, no site-specific priors.
   - Everything about the rubble must be estimated by the system from radio. Generic published ranges may be used
     as priors.
3. **Must work end to end for 20 people, and also be shown for 1 person.**
4. **Every number the simulation produces must be comparable to a proven scientific number.** The output must be
   validated against published measurements or theory, so that we can say it is accurate.
5. **Every input number must be sourced.** Each needs a reviewed or official primary, with a verbatim quote,
   page and calculation (rules in §2).
6. **This extends the earlier CA-TBL work; it does not replace it.**
   - The earlier simulator and its results must keep working.
   - New mechanisms are default-off.
   - The branch must be merge-safe with `main`.
7. **Target venue:** IEEE Transactions on Industrial Informatics (TII). The earlier single-worker results
   (REALISTIC-1..6) are the other half of the paper.
8. **No assistant attribution anywhere:** commits, PR text, branch names or tracked files. This overrides any
   tool default. A gate checks commit messages.

## 2. Rules that bind every change (summary; the full text is in `RULES.md` and the owner's user-level rules file)

- **Every numeric value** used in code, scenarios, analysis or the paper has a row in `docs/SOURCES_COLLAPSE.md`.
  - A row holds: ID, value and unit, what it is used for, full citation with DOI/URL and edition, exact location,
    verbatim quote, the calculation with intermediate numbers, and the date checked.
  - Values come from the **primary** document, never from search snippets or memory.
  - If the primary is unreachable, mark the row PENDING; the value is not used until sourced.
- **Brackets.** Where sources disagree, keep low / central / high values, each from a named source, and test
  every conclusion across all three. **Never tune a value to improve results.**
- **Scenario choices** are sourced, or swept across a sourced range.
- **Row IDs in code.** Every constant in new code carries a comment with its row ID. Gate G177 compares row values
  with code values.
- **World/system split.**
  - The World (ground truth: rubble, geometry, materials) is never read by system-side estimation code.
  - In the collapse program this is enforced by file split: the system reads `obs_*.csv` only, and only
    `scripts/collapse/score_*.py` reads `truth_*.csv`. Gate G178 checks it.
- **Each mechanism** gets:
  - a default-off flag, with the off state byte-identical to before;
  - a gate with negative fixtures (a selftest that must catch doctored inputs);
  - a `docs/DESIGN_DECISIONS.md` entry;
  - a pre-registration in `report/PREREG_*.md` before any citable run;
  - paired statistics (paired bootstrap, as in REALISTIC-1..6).
- **Never run the simulator without `--outdir`.** Without it, it writes into tracked `results/causal`.
- **Never commit in a checkout that a campaign is running from.**

## 3. Where everything is

### 3.1 Worktrees and branches (WSL, user `nethi`)

| Path | Branch | Role |
|---|---|---|
| `~/catbl_multi` | `work/2026-11-22-multiworker` | **current work**, based on current `main` (`c8fa4a14`) |
| `~/catbl_final` | `work/2026-11-21-collapse` (and `work/2026-11-21-tii-final`) | where the collapse work started; built on `main`'s pre-rewrite history; do not merge it directly |
| `~/catbl_mainref` | detached at `main` `c8fa4a14` | reference build of `main` alone, for the merge identity check |
| — | `work/2026-11-21-collapse-onmain` | the 33 collapse/realistic commits re-parented onto `main`'s rewritten history (same trees; map in `docs/SHA_MAP_COLLAPSE_ONMAIN.md`) |

**Why the re-parenting.** On 2026-11-14 `main`'s history was rewritten to strip 22 attribution trailers
(`SHA_MAP.md`, last section). `work/2026-11-21-collapse` descends from the pre-rewrite commits. Merging it directly
would bring those trailers back. So its own 33 commits were recreated on the rewritten parents, with identical trees,
messages, authors and dates. Then they were merged into `main` on this branch.

### 3.2 Key files

| Area | File |
|---|---|
| Collapse plan (approved) | `docs/PLAN_COLLAPSE.md` (copy of `~/catbl_work/tii_paper/PLAN_COLLAPSE.md`) |
| Source log (every number) | `docs/SOURCES_COLLAPSE.md` |
| Design decisions | `docs/DESIGN_DECISIONS.md` (DD 36–38 for the collapse work) |
| World rubble model | `src/lora-physics-engine/model/ca-tbl-rubble-world.{h,cc}` |
| Engine hooks | `SetRubbleWorld()` in `lora-physics-engine`, `lora-2p4-physics-engine`, `lora-uwb-physics-engine` (`.h/.cc`); null by default |
| Collapse World generator | `scratch/ca_tbl_collapse_sim.cc` |
| Rubble unit program | `scratch/ca_tbl_rubble_selftest.cc` |
| System estimators | `scripts/collapse/estimate_collapse.py`, `scripts/collapse/system_priors.py` |
| Scorer (reads truth) | `scripts/collapse/score_collapse.py` |
| Derivation scripts | `scripts/collapse/nist6055_slopes.py`, `scripts/collapse/eta_uwb_brackets.py` |
| Gates | `scripts/rubble_gate.{sh,py}` + `_selftest.sh` (G177); `scripts/collapse_world_gate.{sh,py}` + `_selftest.sh` (G178); both in `scripts/catbl_gates.sh` |
| Merge proof tools | `scripts/replay_cell.sh` (re-run a tracked cell against its digests); `scripts/merge_identity_check.sh` (main binary vs branch binary, same args) |
| Earlier simulator | `scratch/ca_tbl_causal_sim.cc` (single measured subject, ~23k lines) |
| Earlier state notes | `~/catbl_work/STATE_2026-11-21_collapse.md`, `~/catbl_work/STATE_2026-11-20.md` (outside the repo) |

### 3.3 Build and run

```bash
cd ~/catbl_multi
CC=gcc-11 CXX=g++-11 ./ns3 configure --build-profile=optimized --enable-examples --disable-tests   # gcc 9 is too old
./ns3 build ca_tbl_causal_sim ca_tbl_collapse_sim ca_tbl_rubble_selftest
bash scripts/rubble_gate.sh && bash scripts/rubble_gate_selftest.sh          # G177
bash scripts/collapse_world_gate.sh && bash scripts/collapse_world_gate_selftest.sh   # G178
```

Scripts are stored non-executable; always run them with `bash`. Python estimators need `numpy` and `scipy`
(`pip3 install --user scipy`).

**One collapse run, end to end** (20 workers, central rubble; all geometry flags are required, with no hidden
defaults):

```bash
O=/tmp/col_run   # never inside the repo
build/scratch/ns3.47-ca_tbl_collapse_sim-optimized --seed=3 --outdir=$O --bracket=central \
  --footX=30 --footY=30 --pileH=3.66 --nVictims=20 --depthMin=0.5 --depthMax=3.5 \
  --gridStep=5 --holdHeights=0.3,1.5 --nStations=4 --stationStandoff=10 --stationH=6 \
  --slots=18 --slotS=60 --gpsSigmaH=3.27 --gpsSigmaV=6.63 --localSigma=0.10
python3 scripts/collapse/estimate_collapse.py $O --every 3     # system side: obs_* only
python3 scripts/collapse/score_collapse.py $O --curve          # analysis side: truth
```

`pileH = 3.66` is row PILE-ALPHA: 0.15 × 8 storeys × 3.05 m. The storey height is still PENDING (HAZUS primary not
fetched). Footprint, depth range, grid and station layout are **placeholders**; they must be set from rows or swept
in a pre-registration (§6.1).

### 3.4 Pushing (WSL cannot reach GitHub)

The Windows clone `C:\projects\ca-tbl-localization` talks to GitHub. Push a WSL branch through it:

```bash
cd /mnt/c/projects/ca-tbl-localization
git.exe fetch "$(wslpath -w ~/projects/ca-tbl-localization)" work/2026-11-22-multiworker:work/2026-11-22-multiworker
git.exe push origin work/2026-11-22-multiworker
```

Fetch from `~/projects/ca-tbl-localization`, the repository every WSL worktree belongs to. A worktree's `.git` is a
pointer file that Windows git cannot follow.

Direct pushes to `main` are rejected; use a PR (`gh.exe pr create`). The owner authorised pushing this working branch
and opening a PR on 2026-11-22.

## 4. What is implemented (done, committed, gated)

### 4.1 Earlier work, unchanged and merged here

The realistic-site single-worker study is merged here, including:
- DD 27–35 and gates G169–G176;
- results REALISTIC-1..6 and PARETO-1/2;
- the paper draft `~/catbl_work/tii_paper/tii/tii_main.tex` (outside the repo).

The flags are default-off. DD 27 changed what `--specProfile=true` means: three gateways on a line and two
dispatched searchers. `PIN_PRE_DD27` in `scripts/preset_pins.sh` restores `main`'s preset (12-gateway ring,
12 finders).

### 4.2 Collapse extension: World (ground truth)

| Item | Status | Where / numbers |
|---|---|---|
| Sourced rubble loss brackets | done | 868 MHz 6.63 / 11.90 / 30.71 dB/m; 2.4 GHz 25.45 / 38.21 / 98.60; 8 GHz 59.15 / 91.68 / 219.7 (rows OES-*, UWB8-*) |
| Solid fraction η (derived, since no pile-density primary exists) | done | 0.3522 / 0.5459 / 0.7706 = A_rubble(2 GHz) / A_solid(2 GHz) (rows ETA-*) |
| ToF slowing | done | 0.454 / 0.704 / 1.541 m per metre of pile (row SLOW-BR) |
| Lowest-loss path (exit through the pile surface) | done (DD 37) | Snell critical angle and Fresnel transmission per crossing (ITU-R P.2040-4 eqs. 30–35); pile index n = 1 + η(√ε′ − 1) |
| Engine hooks | done (DD 36/37) | loss into tier-3 sum; ToF bias on 2.4 GHz/UWB; 868 RSSI range sees the loss; null by default, byte-identical |
| Collapse generator | done (DD 37) | box pile, N workers, surface grid points × hold heights, ring stations; keyed RNG; obs/truth split |
| Validation targets in G177 | done | 868 MHz totals for 2–5 m span NIST TN 1542 / 1540 bounds (30 / 80 dB); detection range 10.92 m vs Oestges ~10 m |

### 4.3 Collapse extension: System (estimation)

| Item | Status | Notes |
|---|---|---|
| Air path-loss law fitted from the system's own point-to-station links | done (DD 38) | no rubble on those links |
| `tof` / `rss` / `joint` (B1: ToF bias tied to RSS excess through ρ = A/(n − 1)) | done (DD 38) | ρ prior 55.33–65.21 dB/m at 2.44 GHz, spans all brackets |
| Generic depth prior (worker below the surface) | done | |
| Local frame (σ 0.10 m) and GPS frame (GPS SPS PS Table 3.8-3) | done | the rescuer frame itself is not solved yet (B3) |

**First numbers** (3 seeds × 3 brackets, 20 workers, **not citable**). Fraction within 2 m horizontally:

| Bracket | joint | rss | tof |
|---|---|---|---|
| low / central | 0.55–0.80 | 0.45–0.75 | 0.25–0.55 |
| high | 0.25–0.40 | — | — |

Depth error median is 1.3–2.5 m, so depth is the weak axis. In the high bracket 15–35% of workers are not heard at
868 MHz.

## 5. Verification state of this branch (the PR)

- **Build:** clean with gcc-11.
- **Our side unchanged.** Three reference runs on our side (static_868 device-side, seed 7, 600 s; preset
  ca_tbl_full, seed 7, 300 s; ca_tbl_full, seed 11, 900 s) give **178 files byte-identical** to the pre-merge build.
  Only `run_id.txt` (build stamp) differs.
- **G177 and G178** pass, with their selftests (14 and 5 doctored inputs, all caught).
- **`main`'s side: VERIFIED (2026-11-22).**
  - `bash scripts/merge_identity_check.sh ~/catbl_mainref/build/scratch/ns3.47-ca_tbl_causal_sim-optimized DIR`
    ran `main`'s own binary and this branch's binary + `$PIN_PRE_DD27` on three of `main`'s campaign configurations:
    LIMITS L1 (`--tier1ExtraLossDb=15`), LIMITS L2 (`ca_tbl_two_tier`) and ENV-SWEEP E0 (600 s).
  - **614 output files byte-identical** (223 + 113 + 278). `run_id.txt` and `spec_profile.txt` are excluded: the
    latter prints the preset text, which the pin changes.
  - The tracked cells' recorded digests (`results/limits_2026-11-14/...`, `results/envsweep_2026-11-14/...`) are not
    reproduced on this machine by **`main`'s own binary either**: 30 diagnostic files differ in the LIMITS cell, while
    `arm_summary.csv` matches. The merged binary's replay equals `main`'s replay file for file, apart from the pin's
    `spec_profile.txt`.
  - So the difference predates this branch. The likely cause is `NS3_NATIVE_OPTIMIZATIONS=ON` (`-march=native`): the
    cells were recorded on another machine (`/home/kushal/...`), and floating-point detail in diagnostic traces
    differs by CPU.
  - Not investigated further. If cross-machine digests matter, rebuild with native optimisations off on both
    machines.
- **Known pre-existing issues (not introduced here):**
  - `main` already has two gates numbered G167 (ORACLE-ESC and LIMITS).
  - 8 stale row references in `docs/SCENARIO_FLAGS.md`; fixed on this branch.

## 6. What is NOT implemented, and how to implement it

Order matters: §6.1–6.3 are what the owner asked for most recently (20 workers end to end, 1 worker, validated
outputs).

### 6.1 The 20-worker end-to-end system (and the 1-worker case) — REVISED 2026-11-22

**Correction.** The earlier simulator already localises several workers. `--measuredSubjects=N --nTags=N` runs the full
per-worker cycle for tags 2..N: ranging, the tier-1 floor, fix selection, `Step()` with its own escalation, the
handshake, battery, IMU and gateway downlink. It writes `subject_summary.csv` and `subjtrace_*_subj<id>.csv`. The
preset already uses 5.

**Tested.** `--specProfile=true --nTags=20 --measuredSubjects=20 --relaySlots=false
--identityQueryManifests=1,...,20` runs: 300 s simulated in 3.9 s, 20 rows in `subject_summary.csv`. The relay
schedule (`--relaySlots=auto`) refuses crews above ~7, so it must be off.

**Decision.** The 20-person system **and** the 1-person case run in `scratch/ca_tbl_causal_sim.cc` with the collapse
World behind a new default-off flag. The preset's engine physics then apply unchanged: `CA_TBL_MAKE_CONFIGS` sets ~20
engine switches, including `rangeErrorNoLinkLeak`, `rssiRangeFromShadowing`, `nlosRangeBias`, `iqOverloadOnly` and
`rngKeyed`. So do the MAC, relays, finders, energy and every gate. `scratch/ca_tbl_collapse_sim.cc` stays as a fast
**link-level** tool for the rubble physics only. It does not apply all preset switches; §6.2 explains what that
changes.

**How to implement (main simulator):**
1. **Flags.** `--collapse` (default false), plus the required geometry: `--collapseFootX/Y`, `--collapsePileH`,
   `--collapseDepthMin/Max`, `--collapseBracket`, `--collapseHoldH`. Refuse if `--collapse` is given without them.
   Default off means byte-identical: no RNG draw and no engine hook.
2. **World.** Build one `CaTblRubbleWorld` per run with a box pile centred on the deployment centre. Call
   `SetRubbleWorld` on the three engines in `RunArm()`, where they are constructed (`engine868`, `engine2p4`,
   `engineUwb`, around line 7218). Do it in the link probe too, if `--collapse` should size anything (it must not
   change the probe otherwise).
3. **Workers.** Tags are static and buried: (x, y) uniform in the footprint, z = pileH − depth.
   - Find where tag trajectories and positions are generated (`FieldNode::Position()`, z fixed at 1.5 m) and add a
     collapse branch: no motion, its own keyed stream.
   - The 2-D estimators are untouched. Depth estimation is a later mechanism (B2).
4. **Rescuers.** `--nFinders` finders on the pile surface (z = pileH + hold height), dispatched from the base camp at
   the incident (`--searchLaunchAtS`). Finder mode 2's spiral is the existing search.
5. **Stations.** `--nGateways` gateways placed around the pile at a standoff. Use the existing `--gwLayout` options,
   or add a ring at standoff.
6. **Incident.** All tags stop at t = 0 (collapse). The alarm is immediate.
7. **Outputs.** Unchanged. `subject_summary.csv` gives, per worker: error, availability, time to first fix, energy.
   Add `collapse_truth.csv` (positions, depths, path rubble metres per anchor), written only under `--collapse`.
8. **Gate G179.**
   - `--collapse` off: byte-identical to before.
   - On, N = 1 and N = 20: every worker gets a subject row.
   - The engines' rubble loss on a known geometry equals `CaTblRubbleWorld::Path`.
   - Refusals for missing geometry.
   - Selftest with doctored outputs.
9. **The 1-person case** is the same command with `--nTags=1 --measuredSubjects=1`.

**Then, on that base:** worker cooperation (B4), digging order (B6) and the 72 h energy check use the main
simulator's existing mesh, energy model and finder tasking.

### 6.2 Validation of every output against published numbers ("we can say this is accurate")

**Started 2026-11-22:** `scripts/collapse/score_validate.py`. It takes a `--rubble=on` run and the same run with
`--rubble=off` (keyed links, so pairs differ only by the rubble). First results (seed 21, 20 workers, depth
0.5–5 m, pile 5.5 m), 7/12 checks pass:

- **PASS** VAL-2: 868 MHz excess for workers 2–5 m deep spans NIST's bounds across brackets (−2.2 to 102.0 dB;
  central 9.2–66.3, median 45.1).
- **PASS** VAL-6 low / central: 2.4 GHz ToF excess per rubble metre equals SLOW-BR exactly (0.454, 0.704).
  - High bracket: 2.175 vs 1.541 on only 27 links. The engines' ToF sigma grows as SNR falls, so paired
    differences carry extra noise. Replace the 5% criterion with a bootstrap CI applied to every bracket, and record
    that change.
- **PASS** VAL-4 2.4 GHz LOS ToF SD 2.988 m, inside Zhou 2025's 1.27–3.2 m.
- **FAIL (marginal)** VAL-4 UWB LOS SD 0.099 m vs Benouakta 0.10–0.33 m. The engine uses the best-case 0.10 m and
  truncates at ±3σ (−1.4%), so the simulated UWB sits at the optimistic edge. Record as a finding.
- **FAIL** VAL-1: output excess vs model, up to 17.6 dB. Cause found:
  - the receiver's ADC clipping / EVM stage (`lora-physics-engine.cc` ~line 668) degrades strong no-rubble signals
    by up to 18 dB;
  - the 868 MHz range-error leak (`pb`) is on in the standalone generator because it does not apply the preset's
    `rangeErrorNoLinkLeak` / `iqOverloadOnly` switches.
  The rubble hook itself is exact: air-only links show 0.00 dB difference, and component dumps
  (`COLLAPSE_DUMP868=1`) show only `tl` changing, by the rubble term. **Fix:** compare total path loss (tl), not
  receiver-degraded power, or run validation through the main simulator with the preset (§6.1).

Build `scripts/collapse/validate_outputs.py` and a section in `docs/SOURCES_COLLAPSE.md` (rows VAL-*). Each
simulator output quantity is compared with a published value, with a tolerance stated before running:

| Output | Compare with | Status |
|---|---|---|
| Rubble excess loss at 868 MHz / 2.4 GHz for 1–5 m | NIST TN 1540/1542 bounds; Oestges measured 80 dB/m; Bimpas 60–100 dB at 2.45 GHz (PENDING primary) | partly in G177 |
| Detection range vs dynamic range | Oestges Table III | in G177 |
| ToF range error, line of sight, 2.4 GHz and UWB | Zhou 2025 (SX1280 outdoors), Benouakta 2023 (DW3000 10–33 cm) | TODO: run the generator with the rubble model off and compare the error distribution |
| ToF bias through walls | Kolakowski 2018 (~1 ns per two 10 cm walls); Albaidhani 2016 (PENDING) | TODO |
| Floor penetration (standing structure) | ITU-R P.1238-13 Table 5 (9/19/24 dB at 0.9 GHz) | TODO (needs a floors mode in the World) |
| LoRa PDR vs SNR / distance (air) | SX1262/SX1280 datasheet sensitivity; Bor 2016 testbed | TODO |
| Collisions vs offered load | pure-ALOHA theory; Bor 2016 LoRaSim figures | TODO with B5 |
| Energy per cycle and lifetime | datasheet currents; nRF52840 PS | TODO with B5 / Phase 0 |
| Localisation error | published through-rubble localisation tests (DiCarlofelice 2015 at 868 MHz under L'Aquila rubble: PENDING primary; FTM buried-victim study, IEEE Access 2025) | TODO |
| Survival / rescue timing | De Bruycker 1983; INSARAG capacity (rows INSARAG-*) | PENDING primary |

### 6.3 Remaining World inputs (sources needed)

- **Pending primaries:**
  - Bimpas 2003;
  - Alsindi 2009 tables: needed for direct-path loss under deep rubble, which is **not modelled yet** (a stated
    limitation);
  - DiCarlofelice 2015;
  - Albaidhani 2016;
  - HAZUS storey heights (FEMA blocks automated download; get the PDF manually or a mirror);
  - FEMA US&R void types (the Kansas NFPA 1670 deck is available, as training material only);
  - extrication time;
  - battery datasheet;
  - De Bruycker 1983;
  - a typical-performance GPS source.
- **Footprint and depth distribution** are not sourced. Sweep them in the pre-registration: footprint from an
  occupant-load factor if a code source is found; depth uniform over [0.5, pile height]; report sensitivity.

### 6.4 Theory and paper

- Cramér–Rao bound for depth vs hold-height spread and number of contacts (B2), before improving depth estimation.
- Rescuer-frame solve from UWB inter-rescuer ranges + GPS + IMU (B3), replacing `--localSigma`.
- Pre-registrations: COLLAPSE-1 (designs × brackets × N = 20/30 × dig sites), COLLAPSE-2 (sensitivity),
  COLLAPSE-3 (72 h battery). New seeds.
- Paper section: collapse results next to REALISTIC-1..6; cite each row's source with the same page or equation.

## 7. How to add the next mechanism (checklist)

1. Source every new number first (`docs/SOURCES_COLLAPSE.md` row, VERIFIED or PENDING).
2. Implement behind a default-off flag. Constants carry row IDs.
3. Gate + selftest with doctored fixtures. Add both to `scripts/catbl_gates.sh`.
4. Null control: earlier outputs byte-identical. Use `/tmp/.../refruns.sh`-style reference runs, or
   `scripts/merge_identity_check.sh`.
5. Write the `docs/DESIGN_DECISIONS.md` entry.
6. Commit (no attribution), update §9 of this file, push the branch.

## 8. Open questions for the owner

- Collapse results will not make a Monday 2026-11-23 submission. Owner to decide: later submission with the
  collapse study, or paper on the earlier results first.
- De Bruycker 1983 PDF and HAZUS manual need a browser download (automated fetch blocked).
- Whether tracked cells must reproduce across machines. They do not today, even with `main`'s own binary (§5).

## 9. Status log (append; newest last)

- 2026-11-21: source log started; NISTIR 6055, P.1238-13, GPS SPS, Bor 2016, pile height rows; rubble World (DD 36),
  gate G177.
- 2026-11-22: lowest-loss path with Snell/Fresnel and the collapse generator (DD 37, G178); first estimators (DD 38).
- 2026-11-22: branch `work/2026-11-22-multiworker` from current `main`.
  - Collapse work re-parented onto `main`'s rewritten history (no attribution trailers re-enter).
  - Merged, with one conflict (generated `docs/SCENARIO_FLAGS.md`, regenerated: 361 flags).
  - G177/G178 added to the suite.
  - Our side byte-identical; `main`'s side byte-identical under `PIN_PRE_DD27` (614 files, §5).
  - Branch pushed; PR #35 opened.
- 2026-11-22: output validation script (VAL-1/2/4/6; 7/12 pass, causes of the failures found).
  - The main simulator already runs 20 measured workers (`--measuredSubjects=20`).
  - Plan revised (§6.1): the collapse World goes into the main simulator behind `--collapse`.
