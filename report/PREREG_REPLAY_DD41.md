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
