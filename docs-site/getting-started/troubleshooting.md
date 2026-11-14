# Troubleshooting

Each entry below comes from the build guide, the working rules or a defect recorded in the repository.

| symptom | cause and remedy |
|---|---|
| `./ns3 configure` or the build fails with a C++17 or compiler error | The tree needs GCC 11 or newer; GCC 9 does not build it. Select a compiler: `CC=gcc-11 CXX=g++-11 ./ns3 configure ...`. |
| CMake error about version | ns-3.47 requires CMake 3.20 or newer. |
| a run takes about ten times longer than a published one | The default build profile enables logging. Reconfigure with `--build-profile=optimized`. |
| `run_id.txt` shows a different build profile and gate G92 refuses a citation | The binary was not built optimized. Rebuild; do not cite the run. |
| a unit suite reports on code you have since changed | Suites are separate programs. Rebuild them explicitly with `./ns3 build $SUITES`; a stale binary reports on the tree it was built from. |
| the log is empty while the simulator runs | Output is block-buffered; rows appear when the run finishes. |
| the run fails at start because the output directory is missing | `--outdir` must already exist. `mkdir -p` it first. |
| `goal_report.py` prints nothing for a directory | It expects `<dir>/<GROUP>_<name>/s<seed>/` with `trace_*` files; run with `--trace=true`. |
| G11 or G93 red after `catbl_gates.sh` | Expected; each is red by owner decision recorded in `RULES.md`. Any other red is a regression. |
| `git status` shows modified results after running gates | A gate that exits with a modified tree has failed. Regenerate into a scratch directory outside the repository, never in place. |
| a result in a tracked directory cannot be reproduced with default flags | The baseline was produced with the flags in its own `run_id.txt`; default flags differ. Use the recorded invocation. |
| `scikit-learn` import error | Only `ml_pipeline/` needs it; the simulator and the analysis scripts do not. |

If a result looks wrong, check [limitations](../research/limitations.md) before concluding the build is broken:
several known model limits produce results that look right.
