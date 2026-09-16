# `results/` — what regenerates, what does not, and what git already holds

Written so the record survives deleting the directory. Every figure below is measured, not
asserted: the classification was computed from the sidecars and then **tested by re-running
directories and comparing digests**, because a sidecar recording a commit is a claim about
reproducibility, not a demonstration of it.

## 1. Most of `results/` is already in git

| | files | size | on deletion |
|---|---|---|---|
| **tracked** | **2,731** | **11 MB** | **restored by `git checkout -- results/`** |
| untracked | ~16,800 | **18 GB** | lost unless regenerated |

The tracked set is **1,519 `arm_summary.csv`**, **1,191 `run_id.txt`** and 16 `.md` documents —
**every summary figure the deliverables cite.** At the time of writing, no tracked file under
`results/` is modified or staged, so deletion loses no uncommitted work.

The 18 GB at risk is **12,866 `trace_*.csv`** plus run logs: per-cycle detail, deliberately
excluded by `.gitignore`.

## 2. What predicts reproducibility — measured, and it is not what it looked like

The first proxy tried was *"does the simulator source differ from HEAD"*. It flagged **2,999 of
3,277** directories, and it is **wrong**: the recent engine changes are flag-gated, and their
byte-identity null controls already showed the default path is unaffected.

Five directories were re-run and their `arm_summary.csv` compared:

| source commit | era | dirty | result |
|---|---|---|---|
| `5359227` | post-D1 | 0 | **IDENTICAL** |
| `abl_clean` (older) | post-D1 | 0 | **IDENTICAL** |
| `56ec9b6` | post-D1 | **3** | **IDENTICAL** |
| `a059203` | **PRE-D1** | 4 | **DIFFERS** |
| `b49df46` | **PRE-D1** | **0** | **DIFFERS** |

**The D1 sensitivity-table correction (`0d5e747`, 2026-08-26) is the sole predictor.** It changed
the 2.4 GHz and UWB receiver sensitivity tables, which every packet outcome depends on, so no
pre-D1 output can be reproduced by current code — and it should not be, because D1 is a *fix*.

**A non-zero `dirty` counter is not fatal by itself.** `siting_repair` ran at `dirty 3` and
reproduces exactly, because those uncommitted changes were committed immediately afterwards
(`293dc3a`) and HEAD therefore contains them. Dirty is fatal only when the uncommitted state was
never committed — which is also true of the pre-D1 runs, but D1 alone already decides them.

## 3. The classification

| class | dirs | regenerable | why |
|---|---|---|---|
| **OK** | **2,004** | **yes** | post-D1, sidecar records commit and invocation |
| PRE_D1 | 1,047 | no | built before the D1 physics correction |
| NO_SOURCE_LINE | 164 | no | sidecar exists but records no source commit |
| NO_SIDECAR | 62 | no | no `run_id.txt` — nothing recorded how it was made |
| | **3,277** | | |

**1,273 directories (~7 GB) can never be regenerated.** Their full list is in
`RESULTS_NONREGENERABLE.tsv`, tracked beside this file. Largest groups:

| campaign | dirs | reason |
|---|---|---|
| `campaign_20260810` | 340 | PRE_D1 |
| `campaign_recovery` | 240 | PRE_D1 |
| `p1_condfloor` | 140 | PRE_D1 — **Claim 4's evidence** |
| `p10_env` | 140 | PRE_D1 |
| `p10` | 72 | PRE_D1 |
| `overnight_20260805_003718` | 60 | NO_SOURCE_LINE |
| `p10_plane`, `p10_plane_fix` | 40 | PRE_D1 — **Claim 2's evidence** |

**This loss is smaller than it appears.** The pre-D1 directories are evidence the project has
already superseded: `[M-11]`, `[M-16]` and Claim 3's replacement cell were all re-measured on
post-D1 binaries precisely because their originals were pre-D1. Their `arm_summary.csv` and
`run_id.txt` — the parts the documents cite — remain tracked in git regardless.

## 4. How to regenerate

`scripts/regenerate_results.sh` replays each directory's own `run_id.txt` invocation verbatim,
rewriting only `--outdir`. It classifies before running and **refuses** the 1,273 it cannot
honestly reproduce, rather than emitting plausible-looking substitutes.

```bash
bash scripts/regenerate_results.sh --list                  # the table in §3
bash scripts/regenerate_results.sh --verify <dir>          # re-run and diff against disk
bash scripts/regenerate_results.sh --dir <dir>             # regenerate one
bash scripts/regenerate_results.sh --all                   # all 2,004
```

`--all` is a large job: 2,004 directories at roughly 1–5 minutes each is **on the order of days**
of single-threaded compute. Regenerate what a specific question needs; do not treat `--all` as a
routine step.

**Prerequisite:** `./ns3 configure --build-profile=optimized && ./ns3 build`. The optimized profile
is required — the default enables logging and will not reproduce these timings, and every recorded
sidecar names the optimized binary.

## 5. Recommended order if the directory is deleted

1. `git checkout -- results/` — restores the 2,731 tracked files, 11 MB, instantly. **Every number
   in `Simulation.md`, `REPORT_1`, `REPORT_2` and the technical report is sourced again at this
   point.**
2. Regenerate traces only for the directories a live question needs, with `--dir` or `--verify`.
3. Accept the 1,273 as gone, and cite this file as the record of what they were.
