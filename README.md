# CA-TBL — Confidence-Aware Tier-Based Localization

A fork of [ns-3.47](https://www.nsnam.org/) adding a three-tier radio localisation
simulation for GPS-denied search and rescue. Upstream ns-3 is unmodified except where
noted; the contribution is listed under [Where the work is](#where-the-work-is).

## What this is

An ns-3.47 simulation of a three-tier radio localisation system for GPS-denied search
and rescue — 868 MHz LoRa for wide-area coverage, 2.4 GHz for mid-range refinement, UWB
for short-range precision — with a decision layer that escalates between tiers on a
position-confidence score. Six arms are compared against identical channel
realisations, with per-action energy accounting traceable to datasheet currents.

## The deployment, in one paragraph

**There is one population of tri-band nodes. Every node carries all three radios, and a
tier is a choice of *which radio to use*, not of *whom to talk to*.** The classes differ
in mobility and role only:

| class | mobile? | ids | duty cap | flag |
|---|---|---|---|---|
| **gateway** | **no** — the only immobile class | `101+` | 10 % | `--gwGrid` (default 3 → 9 gateways) |
| **finder** — a searcher carrying a radio | yes | `501+` | 2.5 % | `--nFinders` (default 0), `--finderMode` |
| **field node** — the subject being localised | yes | `1`, peers `2+` | 2.5 % | `--nTags` |

Four facility sites exist as **mobility targets only** and host no anchors; the field node
dwells at them.

**Why finders matter, and why the default of 0 is not the interesting configuration.** UWB
reaches tens of metres, so a *fixed* gateway can offer tier 3 only to something already
standing beside it. With `--nFinders=0`, measured mean UWB neighbours is **0.01** — tier 3
is effectively unreachable and escalating to it is pure cost. **A finder is what carries
tier 3 to the casualty**, so escalation to UWB is a statement about a searcher having
arrived. If a run shows tier-3 coverage without finders, suspect the topology before
believing the number.

> **Corrected 2026-09-07 (`99fdf1a`).** Before that date the simulator modelled three
> *disjoint* anchor populations — tier-1 gateways on a grid, tier-2 anchors ringed at 150 m
> and tier-3 at 10 m around the facility sites — and `Architecture.md` specified it that
> way, so the code implemented the wrong model faithfully. It survives as `--topology=1`
> **only** so pre-correction results reproduce. **Every result in `results/` and every
> number in the technical report predates the correction and is provisional pending
> re-run.**

## What it found, as of 2026-08-05

**The escalation architecture does not beat static provisioning.** Against single-band
2.4 GHz it loses on accuracy-per-joule at 0/20 seeds across three geometries and four
environments, and static tier-2 provisioning dominates all 13 operating points of both
decision rules' Pareto fronts. Against a flat variance threshold — the prior-art
baseline — it loses on a crediting-invariant measure at every geometry tested. The
confidence score carries no information beyond the EKF covariance trace it is computed
from (ρ = −0.199 against a monotone control at −0.200), and a redesigned score,
pre-registered before measurement, fails the same test.

**What survives.** Two mechanism findings independent of the decision layer: a
duty-cycle limit makes a tier-1 handoff cost a ranging slot regardless of cycle period,
and an absolute-target escalation rule converts infrastructure improvement into accuracy
regression. One operational result: only a hybrid delivers continuous availability
together with sub-10 m median accuracy — 1.54% outage against single-band 2.4 GHz's
46.25% at the sparse geometry.

The technical report is authoritative and current; this summary is date-stamped because
the work is ongoing.

## Where the work is

```
src/lora-physics-engine/model/ca-tbl-*.{h,cc}          policy, MAC, energy model, header
src/lora-physics-engine/model/lora-*-physics-engine.*  868 MHz / 2.4 GHz / UWB engines
scratch/ca_tbl_causal_sim.cc                           the scenario
scratch/ca_tbl_packet_validator.cc                     MAC unit suite
scratch/python_handoff/verify_causal_sim.py            the invariant gate
scripts/catbl_gates.sh                                 G1-G5 regression gates
scripts/                                               sweeps and analysis
```

## Build and reproduce

```bash
# --build-profile=optimized is REQUIRED, not a preference: every number in the
# report was produced under it, and the default profile enables logging, which is
# ~10x slower for the 3600 s runs below.
./ns3 configure --enable-examples --build-profile=optimized && ./ns3 build
bash scripts/catbl_gates.sh
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal"
python3 scratch/python_handoff/verify_causal_sim.py results/causal
```

**A run that actually exercises all three tiers** needs finders, because nothing else
brings UWB within range (see above). The default `--nFinders=0` does not:

```bash
mkdir -p /tmp/catbl_demo
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=600 --boxHalf=1000 \
  --gwGrid=3 --nFinders=4 --finderMode=1 --outdir=/tmp/catbl_demo"
```

Check it worked: `meanNbr_uwb` in `arm_summary.csv` should be well above zero for
`ca_tbl_full` (≈0.8 at these settings, against ≈0.01 with no finders), and `cycles_uwb`
non-zero. Do **not** write new runs into `results/` — that tree is tracked evidence.

`scripts/catbl_gates.sh` runs the regression gate suite: 88 gates at `4ca0dff9`
(G1–G21, G17b, G18b, G20b, G24 and G28–G90; recount with
`grep -c '^finish ' scripts/catbl_gates.sh`). The first five are build, determinism,
null ablation, the invariant verifier, and a self-test that checks the verifier *fails*
when its inputs are broken; the rest are per-mechanism gates, each with a negative
fixture. Every gate runs and a summary is printed at the end. Five are expected red and
none is a broken build — G4, G5, G6, G11 and G24; `RULES.md` §3 says why each one is.
G3 passes: it separates RNG contamination from the integrity check's treatment effect on
causal order (`scripts/catbl_g3_ablation.py`). The suite takes about 25 minutes.

## Documents

| file | contents |
|---|---|
| `CA-TBL_Technical_Report_SelfContained.md` | claim-by-claim record — **authoritative** |
| `report/PCS_REDESIGN_RESULTS.md` | working evidence chain, Parts 0–8, with corrections registers |
| `CA_TBL_AUDIT_REPORT.md` | independent adversarial re-audit of the result set |
| `CA_TBL_SIM_AUDIT.md`, `simstatus.md` | cited from source comments; kept at root for that reason |
| `Architecture.md` §3 | the deployment as built — **authoritative for topology** |
| `docs/DESIGN_DECISIONS.md` | choices the project brief does not specify, each with what it was chosen over |
| `RULES.md` | working rules; read first if you are changing anything |
| `OBVIOUS_ISSUES.md` | repository-hygiene faults found 2026-10-10, what was fixed and what is left |
| `docs/archive/` | superseded handoffs and scope documents, retained for their findings |

## A note on what is shipped

`results/` contains every `arm_summary.csv`, `run_id.txt` and probe sweep — 1746 files,
7.2 MB — so every table in the technical report is sourced from a file in this
repository. Per-cycle `trace_*.csv` are excluded (~13 GB) and regenerate from the seed;
runs are deterministic and `run_id.txt` records the invocation, the build and the source
commit for each.

The repository ships the evidence for the negative results as well as the positive ones,
including a corrections register of every claim stated and later revised. A repository
that ships its own disconfirming evidence is a stronger artefact than one that ships a
clean positive.

## Licence

ns-3 is licensed **GPLv2**, and this is a derivative work to which that licence applies.
See `LICENSE`. Upstream ns-3 documentation is preserved in
[`README.ns3.md`](README.ns3.md); upstream authorship is unmodified in the git history.
