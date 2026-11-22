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
