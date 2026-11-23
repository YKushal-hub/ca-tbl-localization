# REPLAY-DD41 — re-running the earlier campaigns under the DD 41 default (registered before the first cell)

**Why.** DD 41 (`--interferenceFrameKey`) became the global default by owner decision on 2026-11-22. Every earlier
multi-subject campaign was recorded with the defect it fixes: copies of one broadcast collided at the second and
later receivers. The owner decided to re-run them.

**Design.**
- **Campaigns, in this order:** REALISTIC-5 and REALISTIC-6 (the earlier paper's main results), PARETO-1,
  PARETO-2, REALISTIC-4, -3, -2, -1.
- **Runners and analysers:** each campaign's own frozen runner (`scripts/<name>_run.sh`), unchanged: same cells,
  same seeds, same flags. The only difference is the binary, built from this branch, where `--interferenceFrameKey`
  is on by default. Each campaign's own analyser (`scripts/<name>_analyse.py`), unchanged.
- **Output:** `~/catbl_work/campaigns/replay41/<name>`. Queue: `scripts/replay_dd41_queue.sh`.
- The extension's other switches (collapse, DD 46–53) are off by default and are not given by these runners.

**Hypotheses.** Each campaign's registered hypotheses, re-evaluated by its own rule on the new runs. Reported per
campaign:
- each verdict, with the original verdict beside it;
- which verdicts change.

No hypothesis is added or altered. A verdict that changes replaces the recorded one in the paper. The earlier
recorded results stay in the repository, marked as pre-DD 41.

## Results

### REALISTIC-5 (run 2026-11-22 14:18–15:55 UTC, 280/280 cells rc 0)

The analyser flags "more than one source" (4097e290f, d6a8a2b55). The run-time stamp read HEAD while
documentation-only commits landed. The binary was frozen by the manifest, and `git diff 4097e290f d6a8a2b55 -- src
scratch` is empty. Summary: `results/replay41/realistic5_analysis.md`.

| Hypothesis | Recorded (pre-DD 41) | Replay (DD 41 on) | Detail |
|---|---|---|---|
| G1 (searcher reaches the worker no later than with 2.4 GHz only) | HOLDS | **FAILS** | CLUS3 +182 s [168, 197] later than 2.4 GHz only; elsewhere 1044–1860 s sooner |
| G2 (time to 25 m within 2 min of the better single radio) | HOLDS | **FAILS** | ROAD3 −383 s [−1181, +429]: the upper bound exceeds the 120 s margin; elsewhere 0 to −1616 s |
| G3 (250 m availability within 0.02) | FAILS (CLUS3 −0.005) | **HOLDS** | +0.23 to +0.46 everywhere |
| G4 (8 h quiet shift) | HOLDS | HOLDS | — |

**Controls and corrections (2026-11-23).**
- **Control for "the binary is the only difference":** the registration asserted it without a test. The frozen replay
  binary with `--interferenceFrameKey=false` reproduces the originals' `arm_summary.csv` and `subject_summary.csv`
  byte for byte in REALISTIC-5 E_ROAD3_ARMS/s561 and E_CLUS3_ARMS/s562 (2 cells). The replay with it on differs.
- **Interruption:** the first queue ran without `prune_runs.sh` and exhausted the disk during REALISTIC-6 (73 cells
  complete). Completed cells are kept; partial cells are re-run by the frozen runner. REALISTIC-5 was then pruned
  (47 to 4.1 GB, every file hashed in RAW_MANIFEST.sha256 first); its analysis is byte-identical after pruning.
- **`QUEUE.md`'s first `rc=0` line** reported the exit status of `date`, not of the runner (fixed). REALISTIC-5's
  completeness rests on its 280 `rc` files and RUN.md's `done` line.

### REALISTIC-6 (done 2026-11-23 07:41 UTC, 280/280 cells rc 0; interrupted and resumed once)

`results/replay41/realistic6_analysis.md`. Three source stamps (documentation-only commits; no src/scratch difference;
binary frozen). Control: with `--interferenceFrameKey=false` the replay binary reproduces the original
`arm_summary.csv` byte for byte in E_ROAD3_ARMS/s601 and E_CLUS3_ARMS/s602.

| Hypothesis | Recorded (pre-DD 41) | Replay (DD 41 on) | Detail |
|---|---|---|---|
| H1 | HOLDS | HOLDS | searcher, ours - 2.4 only: ROAD1 -826 s, ROAD3 -1640, RAND3 -986, ROAD3F4 -1655, CLUS3 -118 [-329, 28] |
| H2 | HOLDS | HOLDS | t25: -958 to -1677 s; CLUS3 0 |
| H3 | FAILS (CLUS3 -0.017) | **HOLDS** | A250 +0.24 to +0.44 |

### Registration error: PARETO-1/2 (2026-11-23)

The registration says the frozen runners are used unchanged. For PARETO-1/2 that is wrong: they take the deployment from
the preset, and DD 27 (279fa49c8, 2026-11-20 13:35) changed it from 12 gateways / 12 searchers to 3 / 2 after they ran.
272 PARETO-1 runs on the wrong site were stopped, never analysed, and kept as `INVALID_pareto1_wrong_deployment`. The replay
uses `scripts/pareto{1,2}_dd41_run.sh`, which add only `$PIN_PRE_DD27`. Checked before launch: with the pin and DD 41
off, `arm_summary.csv` is byte-identical to the recorded PARETO-1 A_BASE/s401, B_BEN1000/s402 and PARETO-2
A_BEN1000/s421. REALISTIC-1 to -6 postdate DD 27.
