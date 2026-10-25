# Quick start

One run of the preset: all five arms, the endgame geometry, ten minutes of simulated time, scored on the design
requirement. It needs the [build](installation.md) to be done and nothing else.

```bash
mkdir -p /tmp/catbl_demo/B_endgame/s7
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --specProfile=true --seed=7 --duration=600 \
    --boxHalf=750 --finderMode=1 --trace=true --outdir=/tmp/catbl_demo/B_endgame/s7
python3 scripts/goal_report.py /tmp/catbl_demo
```

| flag | meaning |
|---|---|
| `--specProfile=true` | the preset every claim is drawn from; any flag given explicitly wins over it ([configuration](../implementation/configuration.md)) |
| `--seed=7` | the random seed; runs are deterministic |
| `--duration=600` | simulated seconds; the campaigns use 1800 |
| `--boxHalf=750 --finderMode=1` | the endgame geometry: a box of 750 m half-width, finders converging directly |
| `--trace=true` | write the per-cycle traces that `goal_report.py` reads |
| `--outdir` | **must already exist** |

`goal_report.py` reads a campaign layout, `<dir>/<GROUP>_<name>/s<seed>/`, and prints the requirement per arm:
A(25 m) for tag 0 and for the five-node pool, the fraction of runs at or above 0.95, CEP95 for tag 0 and the mean
subject energy in joules.

!!! note
    Output is block-buffered: a run in progress shows an empty log and no rows until it finishes. One arm is about
    six times faster (`--arms=ca_tbl_full`) and is byte-identical to that arm's row in a full run.

!!! warning "Do not write new runs into `results/`"
    That tree is tracked evidence. Each run writes `run_id.txt` with the invocation, build profile, source commit,
    dirty count and a SHA-256 for each output file; a run with a dirty tree is not citable.

`./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --PrintHelp` lists every flag, and
[`docs/SCENARIO_FLAGS.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/docs/SCENARIO_FLAGS.md) explains them.

## Reproducing a published result

Each tracked results directory carries a `run_id.txt`. Check out the commit it names, build with the optimized
profile, and run the recorded invocation into a new directory. Sidecars that cite a commit that no longer exists
resolve through [`SHA_MAP.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/SHA_MAP.md). See [execution and provenance](../implementation/execution-pipeline.md).
