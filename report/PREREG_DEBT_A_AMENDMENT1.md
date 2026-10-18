# PREREG DEBT-A, amendment 1 (committed before the amended cells run)

**What failed.** All 80 cells of two variants exited 1 at start-up. The simulator refused the registered flags because
of the preset's dependency rules, so they produced no data:
- **EDGEOFF** (`--gatewayEdgeServer=false`): "--edgeUplinkRanges needs --gatewayEdgeServer=true". The preset bills the
  range uplinks of an edge estimator, which do not exist with the estimator on the node.
- **ARBOFF** (`--arbitrationV2=false`): "--requesterEscalation needs ... --arbitrationV2", and then "--gossipQueue
  needs ... --arbitrationV2". The preset's requester escalation and queue gossip ride on the arbiter.

**Amendment.** The variants keep their intent; only the flags the simulator requires are added:
- **EDGEOFF:** `--gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false`. The uplinks are the
  edge estimator's own traffic.
- **ARBOFF:** `--arbitrationV2=false --requesterEscalation=false --gossipQueue=false`. Both mechanisms are parts of the
  arbitration being switched off.

Predictions D6 and D7 are unchanged. Every other cell (240, all `rc 0`) stands as run. The amended cells are re-run by
the runner's resume, which skips `rc 0` cells. A 30 s smoke run of each amended flag set exited 0 before this
commit; it is not data.
