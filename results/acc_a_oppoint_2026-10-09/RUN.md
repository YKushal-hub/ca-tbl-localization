# ACC-A-OPPOINT — how this directory was made

Registered in `report/PREREG_ACC_A_OPPOINT.md` (`5c28eb75`, committed 2026-10-09T03:42:05Z)
before any cell ran.

**Engine:** ACC-A's frozen binary, unchanged:
- `build/scratch/ns3.47-ca_tbl_causal_sim-optimized`, sha256 `38c306df…`;
- compiled from `81fb1485` per `results/acc_a_2026-10-09/RUN.md`;
- full library manifest in that file.

Before launch, the copy was checked against ACC-A's build manifest with `sha256sum -c`. It ran from
a clean worktree at `5c28eb75` as a detached process on the cloud machine (4 cores).

```
2026-10-09T03:42:24Z  (cd ~/campaigns/acc_oppoint && bash scripts/acc_a_oppoint_run.sh ~/campaigns/acc_oppoint_out), worktree at 5c28eb75, build = ACC-A's (sha256 manifest verified)
2026-10-09T03:59:53Z  (all cells done, runner exit 0)
```

All 100 runs exit 0. Every `run_id.txt` reads `source 5c28eb75 dirty 0 profile optimized` and
`build Oct  8 2026 20:52:37`. The source line is the git HEAD of the worktree the binary runs
from. The build stamp is the binary's own, and ACC-A's run_ids carry the same one.

**Analysis:** `python3 scripts/acc_a_oppoint_analyse.py <outdir> <ACC-A outdir>`.
- **First pass, at `5c28eb75`:** 100 problems. PREREG §1 expected `source 681e90c8`, which
  misread what the source line records.
- **The fix (`f42e9ec4`):** the check now tests the line the runs carry, plus the compile stamp.
- **Second pass:** "Problems: 0". Tables and verdicts are byte-identical to the first pass.
  `OPPOINT.md` and `oppoint_table.csv` are its outputs.

**What is kept:** each cell keeps `arm_summary.csv` and `run_id.txt`. `raw_manifest.sha256`
digests every raw file; traces are untracked.

`oppoint_table.csv` is tracked with LF line endings: the repository's attributes set `eol=lf`, as
for ACC-A's table. The manifest digests the analysis's CRLF original.
