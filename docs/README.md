# Documentation index

Documents are listed in order of precedence. When two disagree, the earlier one wins.

| # | document | authoritative for |
|---|---|---|
| 1 | [`CA-TBL_Technical_Report_SelfContained.md`](../CA-TBL_Technical_Report_SelfContained.md) | results and claims, with a corrections register |
| 2 | [`Architecture.md`](../Architecture.md) | the system design as built |
| 3 | [`Simulation.md`](../Simulation.md) | specifications, methodology, per-seed results, work programme |
| 4 | [`report/`](../report) | working evidence chains and registered campaigns; date-stamped, frequently superseded |
| 5 | [`archive/`](archive) | superseded handoffs and audits, kept for their findings; not current |

## Start here

| need | document |
|---|---|
| build and verify | [`INSTALL.md`](INSTALL.md) |
| working rules, the preset list, expected gate reds | [`../RULES.md`](../RULES.md) |
| contributing | [`../CONTRIBUTING.md`](../CONTRIBUTING.md) |
| what is open and where to start | [`WORK_PACKAGES.md`](WORK_PACKAGES.md) |
| strategy, built against specified, gaps | [`../MASTER_PLAN.md`](../MASTER_PLAN.md) |

## Design record

| document | contents |
|---|---|
| [`DESIGN_DECISIONS.md`](DESIGN_DECISIONS.md) | every choice the brief does not make, with what it was chosen over |
| [`SCENARIO_FLAGS.md`](SCENARIO_FLAGS.md) | every scenario flag, regenerated from `--PrintHelp` |
| [`SPEC_CONFORMANCE.md`](SPEC_CONFORMANCE.md), [`SPEC_CONFORMANCE_PLAN.md`](SPEC_CONFORMANCE_PLAN.md) | the specification against the build |
| [`CONTEXT_PACK.md`](CONTEXT_PACK.md) | parameter provenance, prior art, citation rules |
| [`RELAYED_RANGING_ERROR_MODEL.md`](RELAYED_RANGING_ERROR_MODEL.md), [`IMU_SHS_DESIGN.md`](IMU_SHS_DESIGN.md) | model derivations |

## Known defects and status

| document | contents |
|---|---|
| [`TECH_DEBT.md`](TECH_DEBT.md), [`TECH_DEBT_STATUS.md`](TECH_DEBT_STATUS.md) | every known defect and limit, and its status |
| [`BEHAVIOUR_ANOMALIES.md`](BEHAVIOUR_ANOMALIES.md) | behaviour nobody intended, and what was done |
| [`DEBT_IMPACT_2026_09_08.md`](DEBT_IMPACT_2026_09_08.md) | which results each defect touches |
| [`../report/GOAL_STATUS.md`](../report/GOAL_STATUS.md) | the design requirement, campaign by campaign |

## Provenance and integrity

| document | contents |
|---|---|
| [`../EXCLUDED.md`](../EXCLUDED.md) | what is deliberately not tracked, and why |
| [`../SHA_MAP.md`](../SHA_MAP.md) | old to new commit identifiers after the history rewrite |
| [`../RESULTS_NONREGENERABLE.tsv`](../RESULTS_NONREGENERABLE.tsv) | results that cannot be regenerated, with the reason |
| [`LOCAL_LINE_INTEGRATION.md`](LOCAL_LINE_INTEGRATION.md) | how a second line of work was integrated |

## Third-party

[`upstream/README.ns3.md`](upstream/README.ns3.md) is the upstream ns-3 README. `doc/` is the upstream ns-3 manual.
