# Configuration and the preset

There is **no public library API**: the project is a simulator driven by a command-line program and scored by Python
scripts. The command-line interface is the configuration surface.

```bash
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --PrintHelp     # every flag, with its reasoning
```

[`docs/SCENARIO_FLAGS.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/docs/SCENARIO_FLAGS.md) is regenerated from that output. There are about 315 flags.

## The preset

`--specProfile=true` applies the **preset**: the table `kSpecProfile[]` in `scratch/ca_tbl_causal_sim.cc`. Claims are
drawn from the preset. Any flag given explicitly wins over it, so a result from an earlier preset reproduces by
pinning that flag back. [`RULES.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/RULES.md) lists every preset entry and how to pin one back.

## Flags that define a run

| flag | code default | preset | meaning |
|---|---|---|---|
| `--seed` | 7 | | random seed; runs are deterministic |
| `--seedCount` | 1 | | number of consecutive seeds; the summary is appended per seed |
| `--duration` | 3600 | | simulated seconds |
| `--outdir` | `results/causal` | | output directory; must exist |
| `--arms` | all (four arms) | | comma-separated arms; `poucet_fusion` and `lora_fusion` are **not** in `all` and run only by name |
| `--trace` | | | write per-cycle traces |
| `--boxHalf` | 340 | 5000 | search-box half-width, metres |
| `--finderMode` | | 2 | 1 converge; 2 two-phase search |

## Preset entries that matter most

Values are the preset's; the code default is shown where the flag's help text states it.

| flag | code default | preset | what it does |
|---|---|---|---|
| `--nGateways`, `--nFinders`, `--gwLayout` | | 12, 12, legacy (ring) | the adopted deployment |
| `--finderTasking` | empty (every finder steers on tag 0) | pairs | finders $2k$, $2k{+}1$ search subject $(k \bmod 5)+1$ |
| `--imuFilter` | cv | ekf5 | the 5-state filter |
| `--ekf5Window` | 0 (off) | 60 | window-smoother length, cycles |
| `--unknownStart` | false | true | every filter starts at the box centre |
| `--acquireParticles` | false | true | particle-filter acquisition |
| `--tier1Tdoa` | false | true | add a TDoA position where three gateways hear |
| `--tdoaInnovationGate` | false | true | gate the TDoA position at $\chi^2_{0.99}(2)$ |
| `--loraDualBase` | false | true | range both LoRa bands every cycle, pool into one fix |
| `--uwbCrossTierPoll` | false | true | poll finders heard on LoRa |
| `--uwbMode` | legacy | 850k | tier 3 on one DW3000 Table 9 row |
| `--t2SensitivityRowKHz` | 0 (1625 kHz table) | 812.5 | tier 2 on its own SX1280 Table 6-1 row |
| `--pcsForm` | | calibrated | the score's form; weights and thresholds from CAL-03 |
| `--pcsByteScale` | | 4 | score byte over [0, 4] |
| `--integrityCheck` | true | **false** | the Mahalanobis fix test, off (DD 19) |
| `--gatewayEdgeServer` | true | | estimator on the gateways |

## Adding a mechanism

Every new mechanism must (1) be a **default-off flag** whose off state reproduces the existing baseline byte for
byte, (2) pass a **null-control** diff of `arm_summary.csv` and every trace, (3) be pre-registered before its first
measurement run, (4) have a gate with a negative fixture, and (5) write `run_id.txt` with `dirty 0`. See
[contributing](../development/contributing.md).
