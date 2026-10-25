# Installation and verification

This guide takes a fresh clone to a built simulator, a verified build and a first run. Every command is run from the
repository root.

## 1. Requirements

| component | version | note |
|---|---|---|
| OS | Linux (tested on Ubuntu 22.04, native and WSL2) | |
| C++ compiler | GCC 11 or newer (C++17) | GCC 9 does not build this tree |
| CMake | 3.20 or newer | required by ns-3.47 |
| Python | 3.8 or newer | the build driver `./ns3` and every analysis script |
| Python packages | `numpy`, `pandas`, `scipy`, `matplotlib` | analysis and figure scripts; `scikit-learn` only for `ml_pipeline/` |

Disk: the source tree is about 0.5 GB with `results/` (about 380 MB of tracked campaign summaries); a build adds a few
GB. The `results/` directory is evidence, not input: the code builds and every suite runs without it.

## 2. Build

```bash
git clone https://github.com/YKushal-hub/ca-tbl-localization.git
cd ca-tbl-localization
./ns3 configure --enable-examples --build-profile=optimized
./ns3 build
```

**`--build-profile=optimized` is required.** Every published number was produced under it, and the default profile
enables logging and runs about ten times slower on a 3600 s scenario. The gate `G92` refuses a citation that was not
produced by an optimized build.

If the system compiler is older than GCC 11, select one explicitly: `CC=gcc-11 CXX=g++-11 ./ns3 configure ...`.
A full build takes tens of minutes on a laptop; an incremental rebuild after editing `scratch/ca_tbl_causal_sim.cc`
takes under a minute.

## 3. Verify the build

Build and run the twelve unit suites. They are separate programs, so build them explicitly: a stale suite binary
reports on a tree it was not built from.

```bash
SUITES="ca_tbl_packet_validator ca_tbl_session_test ca_tbl_routing_test ca_tbl_lifecycle_test \
        ca_tbl_netlayer_test ca_tbl_relay_admission_test ca_tbl_sparse_anchor_test ca_tbl_loocv_test \
        ca_tbl_tdoa_test ca_tbl_nlos_test ca_tbl_integrity_test ca_tbl_imu_test"
./ns3 build $SUITES
mkdir -p scratchpad/validator
for t in $SUITES; do
    arg=""; [ "$t" = ca_tbl_packet_validator ] && arg="--outdir=scratchpad/validator"
    printf '%s: ' "$t"; ./build/scratch/ns3.47-$t-optimized $arg 2>&1 | tail -1
done
```

Each suite ends with its pass/fail count; all should report zero failures.

The regression gates run every check, summarise them, and never stop at the first failure:

```bash
bash scripts/catbl_gates.sh
```

On a 12-core machine the full suite takes roughly 75 to 90 minutes. Use a scratch directory outside `/tmp` for
`TMPDIR`, and compare `git status --short` before and after: a gate that leaves the tree modified is a failed gate.

**Two reds are expected and are not build problems**, each by owner decision recorded in `RULES.md`:

- **G11**, the analytic link-budget model against the engine, kept red with its reasons; it is not to be tuned.
- **G93**, the code default's tier-2 sensitivity row; the preset carries the data-sheet row.

## 4. A first run

The preset `--specProfile=true` is the configuration all claims are drawn from. This runs all five arms for ten
minutes of simulated time in the endgame geometry and scores the design requirement
(A >= 0.95 at R <= 25 m):

```bash
mkdir -p /tmp/catbl_demo/B_endgame/s7
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --specProfile=true --seed=7 --duration=600 \
    --boxHalf=750 --finderMode=1 --trace=true --outdir=/tmp/catbl_demo/B_endgame/s7
python3 scripts/goal_report.py /tmp/catbl_demo
```

`--outdir` must already exist. Output is block-buffered, so a run in progress shows an empty log and no rows until it
finishes. A single arm is about six times faster (`--arms=ca_tbl_full`) and is byte-identical to that arm's row in
a full run. `./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --PrintHelp` lists every flag; `docs/SCENARIO_FLAGS.md`
explains them.

## 5. Reproducing a published result

Every tracked results directory contains a `run_id.txt` with the exact invocation, the source commit, the build
profile and a SHA-256 for each output file. To reproduce a cell, check out the commit it names, build with the
optimized profile, and run the recorded invocation into a new directory (never into `results/`, which is tracked
evidence). Per-cycle `trace_*.csv` files are not tracked; they regenerate from the same invocation, and runs are
deterministic. Sidecars that cite a commit that no longer exists resolve through `SHA_MAP.md`.

## 6. Where to go next

- `README.md`: what the system is and where the requirement stands.
- `RULES.md`: the working rules; read before changing anything.
- `Architecture.md`: the system as built.
- `docs/README.md`: the index of every document.
