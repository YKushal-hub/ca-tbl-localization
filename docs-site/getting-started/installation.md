# Installation

Every command is run from the repository root. This page is derived from
[`docs/INSTALL.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/docs/INSTALL.md), which is the maintained source.

## Requirements

| component | version | note |
|---|---|---|
| OS | Linux (tested on Ubuntu 22.04, native and WSL2) | other platforms not verified |
| C++ compiler | GCC 11 or newer (C++17) | GCC 9 does not build this tree |
| CMake | 3.20 or newer | required by ns-3.47 |
| Python | 3.8 or newer | the build driver `./ns3` and every analysis script |
| Python packages | `numpy`, `pandas`, `scipy`, `matplotlib` | analysis and figure scripts; `scikit-learn` only for `ml_pipeline/` |

Disk: about 0.5 GB for the source with `results/` (about 380 MB of tracked campaign summaries); a build adds a few
GB. `results/` is evidence, not input: the code builds and every suite runs without it. No hardware, external
service or data download is required.

## Build

```bash
git clone https://github.com/YKushal-hub/ca-tbl-localization.git
cd ca-tbl-localization
./ns3 configure --enable-examples --build-profile=optimized
./ns3 build
```

!!! warning "The optimized profile is required"
    Every published number was produced under `--build-profile=optimized`. The default profile enables logging and
    runs about ten times slower on a 3600 s scenario. Gate G92 refuses a citation that an optimized build did not
    produce.

If the system compiler is older than GCC 11: `CC=gcc-11 CXX=g++-11 ./ns3 configure ...`. A full build takes tens of
minutes on a laptop; an incremental rebuild after editing `scratch/ca_tbl_causal_sim.cc` takes under a minute.

## Verify the build

The twelve unit suites are separate programs; build them explicitly, since a stale suite binary reports on a tree it
was not built from.

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

Each suite ends with its pass and fail count; the requirement is zero failures. The full regression gates:

```bash
bash scripts/catbl_gates.sh
```

This takes roughly 75 to 90 minutes on a 12-core machine, runs every gate and never stops at the first red. Two reds
are expected and are not build problems: **G11** (the analytic link-budget model against the engine, kept red with
its reasons) and **G93** (the code default's tier-2 sensitivity row). See [testing](../development/testing.md).

Next: [quick start](quickstart.md).
