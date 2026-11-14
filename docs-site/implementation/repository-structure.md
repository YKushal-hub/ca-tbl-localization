# Repository structure

The repository is a fork of ns-3.47. **Upstream ns-3 is unmodified except where noted.** The project's code is below.

```text
src/lora-physics-engine/model/
  ca-tbl-policy.{h,cc}        EKF · score · evidence gate · integrity check · escalation state machine
  ca-tbl-mac.{h,cc}           CSMA/CA · duty governor · neighbour table · 9 message types
  ca-tbl-energy-model.{h,cc}  per-action joule accounting from datasheet currents
  ca-tbl-header.{h,cc}        message taxonomy, wire sizes, access categories, ACK policy
  ca-tbl-imu-*.{h,cc}         the 5-state filter and the synthetic IMU
  lora-physics-engine.*       868 MHz   | lora-2p4-physics-engine.*  2.4 GHz
  lora-uwb-physics-engine.*   UWB       | submodules/               about 35 per engine
scratch/
  ca_tbl_causal_sim.cc        the scenario: geometry, mobility, arms, preset, flags
  ca_tbl_packet_validator.cc  and eleven other unit suites (ca_tbl_*_test.cc)
  python_handoff/verify_causal_sim.py   the invariant gate
scripts/
  catbl_gates.sh              regression gates G1-G166
  publication_ready.sh        manifest assertions
  *_run.sh, *_analyse.py      registered campaigns: runner and scorer
  goal_report.py              scores the design requirement
report/                       PREREG_*.md: registered campaigns; working evidence chains
results/<campaign>/           tracked summaries, sidecars, raw-file SHA-256 manifests
docs/                         the repository's design record (not this site)
docs-site/                    this site
```

## Documents and their precedence

When two documents disagree, the earlier one wins.

| # | document | authoritative for |
|---|---|---|
| 1 | [Technical Report](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/CA-TBL_Technical_Report_SelfContained.md) | results and claims, with a corrections register (CR-1 to CR-28) |
| 2 | [`Architecture.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/Architecture.md) | the system design as built |
| 3 | [`Simulation.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/Simulation.md) | specifications, methodology, per-seed results, work programme |
| 4 | [`report/`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/report) | working evidence chains; date-stamped and frequently superseded |
| 5 | [`docs/archive/`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/docs/archive) | superseded handoffs; kept for findings, not current |

[`RULES.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/RULES.md) holds the working rules and the preset list; [`EXCLUDED.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/EXCLUDED.md) records what is
deliberately untracked and why (for example about 13 GB of per-cycle `trace_*.csv`, regenerable from each directory's
`run_id.txt`).

!!! warning "Working documents go stale"
    A "not implemented" note in a dated report may be false today. The code (the `cmd.AddValue` block in
    `scratch/ca_tbl_causal_sim.cc`) and the `run_id.txt` sidecars are the ground truth; prose is a snapshot.
