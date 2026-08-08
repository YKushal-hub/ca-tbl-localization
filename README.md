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
./ns3 configure --enable-examples && ./ns3 build
bash scripts/catbl_gates.sh
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal"
python3 scratch/python_handoff/verify_causal_sim.py results/causal
```

`scripts/catbl_gates.sh` runs five gates: build, determinism, null ablation, the
invariant verifier, and a self-test that checks the verifier *fails* when its inputs are
broken. G3 currently fails by design — the integrity check fires once at seed 7, so
byte-identity between the ablation arms is no longer available; the divergence begins at
the firing cycle and not before, which is the treatment effect rather than RNG
contamination.

## Documents

| file | contents |
|---|---|
| `CA-TBL_Technical_Report_SelfContained.md` | claim-by-claim record — **authoritative** |
| `report/PCS_REDESIGN_RESULTS.md` | working evidence chain, Parts 0–8, with corrections registers |
| `CA_TBL_AUDIT_REPORT.md` | independent adversarial re-audit of the result set |
| `CA_TBL_SIM_AUDIT.md`, `simstatus.md` | cited from source comments; kept at root for that reason |
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
