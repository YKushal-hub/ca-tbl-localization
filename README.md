# CA-TBL — Confidence-Aware Tier-Based Localization

A fork of [ns-3.47](https://www.nsnam.org/) adding a three-tier radio localisation
simulation for GPS-denied search and rescue. Upstream ns-3 is unmodified except where
noted; the contribution is listed under [Where the work is](#where-the-work-is).

## What this is

An ns-3.47 simulation of a three-tier radio localisation system for GPS-denied search
and rescue — 868 MHz LoRa for wide-area coverage, 2.4 GHz for mid-range refinement, UWB
for short-range precision — with a decision layer that escalates between tiers on a
position-confidence score. Six arms are compared against identical channel realisations,
with per-action energy accounting traceable to datasheet currents.

## What it found

**The system locates a buried subject to 0.25 m within a median of 132.5 seconds** — an
order of magnitude inside the 18-minute high-survival window the avalanche literature
documents. **It does not need three tiers to do so.** A single 2.4 GHz band meets the
same budget, and is significantly faster at every accuracy the comparison can resolve.
The architecture's premise is sound in physics; its advantage does not survive
measurement at any requirement derivable from the operation.

That conclusion rests on five independently pre-registered lines of evidence — the
deployment envelope, the operating-point grid, the integrity ablation, an accuracy ladder
from 0.25 m to 25 m, and time-to-first-fix. Each is recorded with its registration, its
falsifier, and the result scored against it.

**Two findings frame the rest**, and both are conditions on the evidence base rather than
entries in it:

- **The requirement was never traceable to a standard.** `R = 25 m` came from sweep-width
  reasoning that this project's own pre-registration flagged as needing justification
  against the literature. Checked against primary sources, three of four operational
  figures did not survive — only the 25 cm probe grid appears in ICAR Recommendation 009.
- **Availability and time-to-fix disagree by an order of magnitude on the same traces.**
  At 0.25 m the tiered arm shows 4.65% availability and a 132.5 s median time to first
  fix, with zero seeds never achieving one. Both are correct; they answer different
  questions, and for a burial the operational question is elapsed time, not duty cycle.
  This project measured the other one.

**Claim ledger, final state** — full detail and evidence tags in `Simulation.md` §11:

| # | claim | status |
|---|---|---|
| 1 | Escalation triggers correctly | **withdrawn (W12)** — trigger inert under the corrected NLOS source |
| 2 | Energy reduction | **re-scoped** — 25.68× → **6.53×** |
| 3 | Architectural advantage | **withdrawn (W11)**, no replacement |
| 4 | Coverage-floor optimisation | **re-scoped** — 42.8% → **24.5%** |
| 5 | Specialized packet taxonomy | stands |
| 6 | Custom handshake / MAC | stands |
| 7 | Physics realism on packets | **re-scoped** — verified for ranging and energy, unverified for receiver sensitivity |
| 8 | Confidence model beats a flat threshold | **falsified** — and measured on a predicate since shown unable to fire |
| 9 | Integrity check improves accuracy | **falsified**, on clean provenance |

**One engineering result stands on its own.** A parameter-free repair to the escalation
predicate takes tier-3 service from zero to every seed and cuts policy thrashing from 148
oscillations per seed to 3.5. It costs 2.17× the energy of a single band and buys
+0.0016 availability, so it does not rescue the architecture — but the defect was real
and the repair is measured.

## Where the work is

```
src/lora-physics-engine/model/ca-tbl-*.{h,cc}          policy, MAC, energy model, header
src/lora-physics-engine/model/lora-*-physics-engine.*  868 MHz / 2.4 GHz / UWB engines
src/lora-physics-engine/model/submodules/              propagation, fading, foliage, terrain
src/catbl-propagation-study/                           independent analytic link budget (Python)
scratch/ca_tbl_causal_sim.cc                           the scenario
scratch/ca_tbl_packet_validator.cc                     MAC unit suite
scratch/python_handoff/verify_causal_sim.py            the invariant gate
scripts/catbl_gates.sh                                 the gate suite
scripts/catbl_concurrency_gate.py                      G16, anchor-survey concurrency
scripts/diagnostics/                                   the overnight diagnostic modules
scripts/doc_audit.py, scripts/citation_audit.py        documentation verification
scripts/claim3_m1.py, scripts/compute_rho.py           claim adjudication
```

`src/catbl-propagation-study/` is Python and deliberately **not** a CMake module: it is an
independently-implemented analytic model, kept separate so it cannot import from the
simulator it checks. Its absence from the build is a decision, not an omission.

## Build and reproduce

```bash
# --build-profile=optimized is REQUIRED, not a preference: every number in the
# documents was produced under it, and the default profile enables logging, which
# is ~10x slower for the 3600 s runs below.
./ns3 configure --enable-examples --build-profile=optimized && ./ns3 build
bash scripts/catbl_gates.sh
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=/tmp/catbl_repro"
python3 scratch/python_handoff/verify_causal_sim.py /tmp/catbl_repro
```

`scripts/catbl_gates.sh` runs **fifteen gates** — G1–G13 plus G9b and G12b — covering
build, determinism, null ablation, the invariant verifier, a self-test that checks the
verifier *fails* when its inputs are broken, tracked-file and citation checks, and
attribution checks over both commit metadata and tracked file contents. **G16**, the
anchor-survey concurrency gate, is a separate trace-level check
(`scripts/catbl_concurrency_gate.py`). **G14 and G15 were registered and deliberately
deferred** — see `src/catbl-propagation-study/DEFECTS.md`.

Some gates are **red by design and documented as such**; a green suite is not the
definition of done here, and each red gate carries its reason.

## Documents

| file | contents |
|---|---|
| `Simulation.md` | **the results document** — claim ledger, per-arm results, evidence tags `[M-nn]` |
| `Architecture.md` | system design, physics engines, the escalation predicate, verification architecture |
| `REPORT_1_Propagation_Module.md` | the independent propagation model and its reconciliation against the engine |
| `REPORT_2_Project_Retrospective.md` | what the project established, and what it got wrong |
| `src/catbl-propagation-study/DEFECTS.md` | the defect and method-failure register |
| `src/catbl-propagation-study/experiments/` | every pre-registration and its scored result |
| `CA-TBL_Technical_Report_SelfContained.md` | standalone claim-by-claim record |

## A note on what is shipped

`results/` contains every `arm_summary.csv` and `run_id.txt`, so every table in the
documents is sourced from a file in this repository. Per-cycle `trace_*.csv` are excluded
(~13 GB) and regenerate from the seed; runs are deterministic and `run_id.txt` records
the invocation, the build and the source commit for each.

The repository ships the evidence for the negative results as well as the positive ones,
including a register of every claim stated and later withdrawn, with its reason and its
mechanism. **Four claims were withdrawn or re-scoped by this project's own measurements,
two of them after a defect was found in evidence the project had already published.** A
repository that ships its own disconfirming evidence is a stronger artefact than one that
ships a clean positive.

## Licence

ns-3 is licensed **GPLv2**, and this is a derivative work to which that licence applies.
See `LICENSE`. Upstream ns-3 documentation is preserved in
[`README.ns3.md`](README.ns3.md); upstream authorship is unmodified in the git history.
