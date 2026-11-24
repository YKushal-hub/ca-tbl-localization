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

### Coverage and UWB share (not registered; `scripts/realistic_coverage.py`, 2026-11-23)

Share of crew cycles whose error is inside 1.73 x sqrt(traceSigma) (95 % circle, isotropic), ca_tbl_full. The same script
on the original runs returns the paper's recorded 83-97 % (REALISTIC-5 emergency) and 0.53 (quiet, office).

| Cell | R5 original | R5 replay | R6 original | R6 replay |
|---|---|---|---|---|
| E_ROAD1 | 0.931 | 0.833 | 0.908 | 0.852 |
| E_ROAD3 | 0.915 | 0.839 | 0.920 | 0.848 |
| E_ROAD3F4 | 0.898 | 0.806 | 0.899 | 0.786 |
| E_RAND3 | 0.827 | 0.715 | 0.876 | 0.720 |
| E_CLUS3 | 0.967 | 0.459 | 0.961 | 0.445 |
| Q_ROAD1 / Q_ROAD3 / Q_RAND3 / Q_CLUS3 | 0.998 / 0.934 / 0.822 / 0.527 | 0.998 / 0.836 / 0.657 / 0.181 | - | - |

**DD 41 on lowers coverage in almost every cell, sharply at the office site.** Position availability rises, reported uncertainty
becomes more over-confident. Cause not isolated. UWB active on 0.9-2.6 % of crew cycles in REALISTIC-6 (Mueller, crew-10/15: 0.6-1.6 %).

### Registration error: PARETO-1/2 (2026-11-23)

The registration says the frozen runners are used unchanged. For PARETO-1/2 that is wrong: they take the deployment from
the preset, and DD 27 (279fa49c8, 2026-11-20 13:35) changed it from 12 gateways / 12 searchers to 3 / 2 after they ran.
272 PARETO-1 runs on the wrong site were stopped, never analysed, and kept as `INVALID_pareto1_wrong_deployment`. The replay
uses `scripts/pareto{1,2}_dd41_run.sh`, which add only `$PIN_PRE_DD27`. Checked before launch: with the pin and DD 41
off, `arm_summary.csv` is byte-identical to the recorded PARETO-1 A_BASE/s401, B_BEN1000/s402 and PARETO-2
A_BEN1000/s421. REALISTIC-1 to -6 postdate DD 27.

### DD 41-off controls for REALISTIC-4, -3, -2, -1 (2026-11-23)

Same build, `--interferenceFrameKey=false`, the frozen runners' own commands, two cells per campaign, against the original outputs:
`arm_summary.csv` and `subject_summary.csv` byte-identical in all 8 cells (`results/replay41/control/RESULT.txt`). With
REALISTIC-5/6 (2 each) and PARETO (3), 15 of 15 control cells reproduce.

### PARETO-1 (done 2026-11-23 11:15 UTC, 560/560 cells rc 0, pinned to its original deployment)

`results/replay41/pareto1_analysis.md`. Three source stamps (documentation-only commits; `git diff -- src scratch` empty; binary frozen).

| Hypothesis | Recorded (pre-DD 41) | Replay (DD 41 on) | Detail |
|---|---|---|---|
| P1 (go/no-go) | NO-GO: no BEN feasible at 5 km (0.874-0.942) | **NO-GO, different reason**: every BEN is feasible (0.980-0.986), but 2.4 GHz only is feasible too (0.984) at 7.87 J, and BEN spends 1.54-1.65x that | E* was lora_fusion (12.67 J) |
| P2 (BEN cheaper than FT12 in A) | HOLDS | HOLDS | BEN1000 -4.80 J [-5.19, -4.45], 16.89 -> 12.09; five-worker A -0.003 [-0.005, -0.001] |
| P3 (the skip carries the saving) | HOLDS | HOLDS | lost worker's 868 MHz energy (arm_summary j_868) BNS100 9.50 vs BEN100 3.45 J (A), 11.12 vs 5.11 (B); same column originally 9.19/3.29, 9.98/3.91; BNS100 spends +1.00 J [0.63, 1.37] against FT12 |

Lost-worker CEP95 at 5 km 5.8-7.4 m under BEN against 3.6-4.6 m (was 7.3-10.3 against 4.0-4.3). The accuracy loss that
motivated TB-2 is much smaller with the defect fixed.

### PARETO-2 (done 2026-11-23 13:49 UTC, 400/400 cells rc 0, pinned to its original deployment)

`results/replay41/pareto2_analysis.md`; P2 and CEP95 contrasts by `scripts/pareto2_replay_contrasts.py`. Two source stamps (documentation only).

| Hypothesis | Recorded (pre-DD 41) | Replay (DD 41 on) | Detail |
|---|---|---|---|
| P1 | NO-GO (2.4 GHz only feasible, 6.14 J) | NO-GO, same reason | E* = 2.4 GHz only, 8.04 J; TB-2 1.79-1.89x |
| P2 (TB-2 more accurate than the first form, five-worker A25 in A) | HOLDS +0.024 [0.007, 0.046] | **FAILS** -0.005 [-0.027, 0.009] | 0.978 vs 0.984; one seed (432, -0.181) dominates |
| P3 (BEN1000 cheaper than FT12) | HOLDS -1.16 J (-9 %) | HOLDS -2.93 J [-3.37, -2.49] (17.32 -> 14.39, -17 %) | five-worker A -0.010 [-0.031, 0.002] |

Not registered: lost-worker CEP95 at 5 km TB-2 3.3 m, first form 8.5 m (-5.18 m [-9.39, -1.85]), flat threshold 3.3 m.

### REALISTIC-1 to -4 (done 2026-11-24 01:17 UTC; 380/380, 240/240, 180/180, 200/200 cells rc 0, every cell dirty 0)

`results/replay41/realistic{1,2,3,4}_analysis.md`. REALISTIC-4 carries two source stamps (documentation-only commit; binary frozen). Mirror of the
registered rules; verdicts by each campaign's own analyser.

| Hypothesis | Recorded (pre-DD 41) | Replay (DD 41 on) | Detail |
|---|---|---|---|
| R1 H1 | FAILS | FAILS | ROAD3 no searchers +295 s [-1175, 1680] |
| R1 H2 | HOLDS (not evidence: 2.4 GHz-only party could not work) | HOLDS, same caveat | ROAD1 -1288, ROAD3 -1566, CLUS3 -3386 s |
| R1 H3 | FAILS narrowly | **HOLDS** | lowest lower bound -0.009 |
| R2 E1, E2, E3 | HOLD (E3 not evidence) | HOLD, same caveat | E3 -2012 s [-2124, -1798] |
| R2 Q1 | FAILS | FAILS | office -0.002 [-0.124, 0.130] (was -0.330) |
| R3 S1 | FAILS | **HOLDS** | ROAD3 no searchers -2067 s [-3032, -1109]; 2.4 GHz only -1952 [-2792, -1050] |
| R3 S2 | FAILS | FAILS | ROAD3 no searchers +179 s [-599, 984] |
| R3 S3, S4 | HOLD | HOLD | |
| R4 F1 | FAILS (CLUS3 +119 s) | FAILS | CLUS3 +319 s [171, 564] |
| R4 F2 | HOLDS | **FAILS** | ROAD3 -480 s [-1169, 251], upper bound above 120 s |
| R4 F3 | FAILS (CLUS3 -0.030) | FAILS, other site | ROAD3F4 lower bound -0.021 against -0.02 |

Reference site RING12 (reported, in no hypothesis): ours reached 25 m in 965 s (20 % never) with searchers, 1595 s (20 %) without; 2.4 GHz only 0 s.

**Deviations in how the last three campaigns were run (2026-11-23/24).** To use idle CPUs, REALISTIC-2 and -1 were run in
parallel (`queue2.sh`); a memory guard deadlocked all runs 22:04-01:30 IST (no finished cell lost, partial cells redone). They
were finished by memory-budgeted copies of the frozen runners (`mkrunner.py`, `admit.sh`, `queue3.sh`) that change only the
start condition (a memory budget replaces the job count), skip a run that survived the restart, and (REALISTIC-1) run RING12 last.
Commands, seeds and frozen binaries are unchanged; every cell's own `run_id.txt` reads `dirty 0`.
