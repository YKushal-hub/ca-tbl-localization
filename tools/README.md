# tools/ — the per-item workflow's helpers

Kept in the tree since 2026-10-04 so the work can continue on a fresh machine. Until then
they lived in `~/catbl_tmp_gates/` and `~/catbl_ref_spec0/` on the original one.

| tool | what it does |
|---|---|
| `nullcontrol/rebuild_reference.sh [<refdir>]` | Builds the frozen reference binary (`e6b3da9`) in a worktree and runs NC1..NC9 into `<refdir>` (default `~/catbl_ref_spec0`). Run once per machine: byte-identity is machine-dependent (TECH_DEBT 4.2). |
| `nullcontrol/nc.sh '<binary>' <tag>` | The null control. Runs NC1..NC9 with the candidate binary and compares every output file, `run_id.txt` excepted, byte for byte. Prints `NULL CONTROL: PASS` or `FAIL`. |
| `nullcontrol/args/NC*.args` | The nine argument sets. |
| `run_suites.sh` | Rebuilds and runs the twelve unit suites (361 checks at 2026-10-04). |
| `regen_flags.py [--bullet FLAG TEXT] [--row ROW]` | Regenerates `docs/SCENARIO_FLAGS.md` §3 from `--PrintHelp`. Optionally adds a new flag's header bullet and §1 row. Needs the built binary. |
| `upd_register.py ROW STATUS EVIDENCE PLAN NOTE` | Rewrites one row of `docs/SPEC_CONFORMANCE.md` and recounts the status table. |

The per-item workflow, in order:
1. Add a default-off flag. Inert combinations are FATAL; new outputs are digested.
2. Build: `CC=gcc-11 CXX=g++-11 ./ns3 build scratch/ca_tbl_causal_sim`.
3. Run `tools/nullcontrol/nc.sh` until it PASSes.
4. Add the gate trio: `scripts/<name>_gate.{sh,py}` and `_gate_selftest.sh`, registered in
   `scripts/catbl_gates.sh`.
5. Run `tools/run_suites.sh` if the library changed.
6. Update the docs, using the two helpers above and dated TECH_DEBT entries.
7. Run the full suite detached (`TMPDIR=<not /tmp> bash scripts/catbl_gates.sh`), and
   compare `git status --short` before and after.
8. Make one atomic commit, with no assistant attribution (`CLAUDE.md` §2).
