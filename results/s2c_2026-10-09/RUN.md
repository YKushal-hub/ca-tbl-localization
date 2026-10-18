# s2c_2026-10-09 — how this directory was made

Registered at `8d441fe4` before any cell ran. Engine: the build of `a6b2414d`'s sources (`--unknownStart`, `--traceSubjectConsistency`, `--measVarScale`; G130). Binary (frozen copy, checked with `sha256sum -c` against its
manifest before launch):

```
b4730af13c1b18eb9c110f035c83999173b8ad9f0f30e14a1be0072fb5aeba36  build/scratch/ns3.47-ca_tbl_causal_sim-optimized
```

Run log:

```
2026-10-09T04:26:15Z  (cd ~/campaigns/s2c && bash scripts/s2c_run.sh ~/campaigns/s2c_out), worktree at 8d441fe4, build = build_s2c (sha256 verified)
2026-10-09T05:42:35Z  (all cells done, runner exit 0)
```

`run_id.txt` lines (the `source` line is the campaign worktree's HEAD, not the build commit; the compile
stamp pins the binary):

```
    116 source 8d441fe4 dirty 0 profile optimized
    116 build Oct  9 2026 04:20:33 
```

Each cell keeps `arm_summary.csv` and `run_id.txt`; `raw_manifest.sha256` digests every raw file
(traces untracked). Tables: the analysis output in this directory.

## Analysis passes, disclosed

**First pass, the registered script at `8d441fe4`:** "Problems: 0", but P2, P10 and P11 were
printed as "not evaluable". Two defects in the script:
1. **Verdict rendering.** Those three verdicts are NumPy booleans, and the renderer tested
   `v is True` / `v is False`, which a NumPy boolean never satisfies. The same test kept them out
   of the problem count.
2. **Two bootstraps per quantity.** P9's verdict re-drew a bootstrap instead of reusing its table
   row's. `cs.ci` resamples from one seeded generator, so a second call draws a different sample.
   P9 sits on the boundary: the table row reads [−0.000, 0.089], and the verdict's own draw had a
   lower bound above 0, so the first pass printed "held".

**The correction (`3f3e12e9`):**
- verdicts are cast to `bool` before rendering;
- `paired()` is memoised per (cell, arm, metric), so a verdict reuses the interval its row
  printed.

**Second pass, the corrected script:**
- every table is byte-identical to the first pass (checked with `diff`);
- P2 held, P9 failed, P10 failed, P11 held;
- "Problems: 0".

`S2C.md` and `s2c_table.csv` are the second pass's outputs. The first pass's `S2C.md` is kept as
`S2C_firstpass.md`. No cell, metric, threshold or prediction changed.

`s2c_table.csv` is tracked with LF line endings (the repository attributes set `eol=lf`, as for ACC-A's table); the manifest digests the analysis's CRLF original.


**Run-log exit value, corrected note (2026-10-09).** The run log's closing line quotes `runner exit $?` *after* a `$(date ...)` substitution in the same `echo`, so it prints `date`'s status (always 0), not the runner's (`bash -c 'false; echo "$(date) exit $?"'` prints 0). It is not evidence that the runner exited 0. The evidence is each run's own `rc` file: every one reads 0, and the analysis checks them.
