# PART B — hybrid range supply and the 5 km subjects (system 2)

Self-contained. Do not read Part A to do this. Time budget: **4.5–5 h of work**; compute runs beside the code.
Written 2026-10-08. Evidence references `context.md` (C§n) and the scoreboard in §2.

## 1. Goal, and what this part owns

**Goal.** Raise what the hybrid arms (`ca_tbl_full`, `fixed_threshold`, `ca_tbl_no_integrity`) can *measure*
and use, without touching the estimator: restore the precise ranges the dual base lost in the endgame, and
find out why the 5 km subjects are worse than `static_2p4`'s while fixing 10–40× more often. The outcome
that closes the one remaining significant loss of `ca_tbl_full` (5 km, five-node within 25 m, against
`static_2p4`) and adds accuracy in the endgame.

**What is measured (closed loop, 20 seeds, new preset).**
- Endgame, tag 0, precise ranges (reported σ < 40 m) per cycle: `static_2p4` **2.67** (≥ 3 on 50 % of
  cycles), `ca_tbl_full` **1.80** (≥ 3 on 27 %). The gap is peer ranges: **1.52 vs 0.23 per cycle**.
  Gateways (0.48 vs 0.45) and finders (0.67 vs 1.11) are not the problem.
- The cause is probably the relay chain. In one cell's `run.log` (B, seed 1): tag→relay legs offered by
  depth 1/2/3+ **1 176 / 160 / 77 for `static_2p4`** against **114 / 88 / 12 for `ca_tbl_full`**; relayed
  ranges admitted **1 252 vs 181**. `ca_tbl_full`'s tag 0 is on 2.4 GHz as its *active tier* on only 36 % of
  cycles (86 % before the dual base was put in the preset; 100 % for `static_2p4`); the subject chain logs
  "on 868 MHz, where A1 admits nothing: 963" subject-cycles against 0. **Hypothesis H1:** relay ranging is
  keyed to the active tier, relay admission A1 (tier) admits nothing on 868 MHz, and the dual base left the
  active tier on 868 MHz most of the time, although the 2.4 GHz radio is awake and ranging.
- 5 km subjects 2–5, median error (m): `ca_tbl_full` 19.7 / 19.5 / 37.3 / 32.1, `static_2p4` 15.9 / 16.3 /
  18.2 / 23.4. They fix on 38 / 24 / 11 / 1 % of cycles against about 1 %. Time on 2.4 GHz as active tier:
  85 / 72 / 71 / 33 % against 100 %. **Hypothesis H2:** the extra tier-1 fixes and TDoA solves the subjects
  fuse add error to a dead-reckoned estimate that `static_2p4`'s subjects keep clean. **H3:** same cause as H1.
  The cause is **not established**; step B2 decides.

| owns | files and symbols |
|---|---|
| Part B only | in `scratch/ca_tbl_causal_sim.cc`: the ranging lambdas and the dual-base block of `RunArm` (`rangeTier`, `otherLoraMeas`, `dualBase`, `dualFused`), the relay-chain machinery (`struct RelayChain`, `RangeShortRelays`, `RangeForSubject`, the `chain->awake` masks), the subject loop; `src/lora-physics-engine/model/ca-tbl-relay-admission.{h,cc}` |
| shared, append-only | the `cmd.AddValue` block and `SpecOptions` (your block: `// --- S2-B accuracy flags ---`); `scripts/catbl_gates.sh` (your gates: **G110–G119**) |
| numbers reserved | TECH_DEBT **N.35–N.39**; `report/PREREG_ACC_B_*.md`; `report/ACC_B_RESULTS.md`; `results/acc_b_*` |
| never touch | `kSpecProfile` (S1 only); `ca-tbl-policy.{h,cc}` EKF and `ca-tbl-imu-model.*` (Part A's); `docs/SCENARIO_FLAGS.md` by hand |

**Independence rule.** You are judged against the unmodified preset on your own binary. Your flags are read
only by hybrid arms: static, `poucet_fusion` and `lora_fusion` rows must be byte-identical with your flags on
or off (a free check). With every flag off, every output is byte-identical to the binary without them.

## 2. Baseline you are measured against

The 102-setting preset (`e308c14f`: FIXES-N26's four fixes are in `--specProfile`). Closed loop, 20 seeds,
A = 5 km, B = endgame (`--boxHalf=750 --finderMode=1`), `--duration=1800`. My cells (exploratory, not
citable) are identical to the tracked FIXES-N26 `ALL_main` / `ALL_end` cells, 320 of 320 values.
Five-node fix availability / five-node within 25 m / tag-0 CEP50 / CEP95 (m) / J per node:

| arm | A | B |
|---|---|---|
| `ca_tbl_full` | 0.285 / 0.613 / 16.2 / 85.0 / 16.7 | 0.562 / 0.968 / 4.8 / 16.1 / 23.0 |
| `fixed_threshold` | 0.274 / 0.625 / 16.7 / 72.9 / 16.9 | 0.545 / 0.964 / 4.6 / 17.6 / 22.0 |
| `static_2p4` | 0.031 / 0.683 / 12.5 / 79.4 / 14.2 | 0.271 / 0.968 / 4.3 / 13.4 / 5.8 |
| `static_868` | 0.165 / 0.319 / 52.3 / 1 807 / 6.1 | 0.371 / 0.613 / 7.8 / 29.5 / 6.4 |
| `poucet_fusion` | 0.238 / 0.499 / 29.7 / 290.0 / 462.5 | 0.506 / 0.862 / 5.4 / 18.9 / 416.2 |
| `lora_fusion` | 0.226 / 0.513 / 36.1 / 2 308.6 / 16.8 | 0.501 / 0.885 / 5.9 / 18.5 / 9.5 |

Fix availability is already the best of every arm; **do not lose it** (the guard in P-B4). The one significant
loss: A five-node within 25 m against `static_2p4`, −0.070 at this baseline. Per-node fix availability, A, new
preset: tag 0 0.66, subjects 0.39 / 0.24 / 0.12 / 0.02; B: 0.99, 0.78 / 0.55 / 0.39 / 0.10.

## 3. Rules for the fast lane

Kept: default-off flag; null control (NC1–NC9) before reading any measurement; a one-page pre-registration
committed **before** the first measured cell of the fix (after B2, before B4); a gate with a negative fixture
per flag; 20-seed paired evaluation; every run with `--outdir=<temp dir>`. Tune on seeds 21–40, evaluate on
1–20. Deferred: the frozen-worktree `dirty 0` campaign and adoption into the preset (S1's call). Say
"exploratory grade". Commits: **YKushal <kushalyalamanchi@gmail.com>**, one change per commit, no attribution
trailer anywhere.

## 4. Step 0 — set up (50 min; the reference rebuild runs in the background)

```bash
git fetch origin
git checkout -b acc/b-hybrid origin/work/2026-09-27-spec-b1        # the main line, includes e308c14f
git checkout origin/work/2026-10-09-acc-reruns -- analysis/accuracy_replay context.md plan.md \
    plan_part_a_filter.md plan_part_b_hybrid.md
git add -A && git commit -m "import S2 accuracy research files"
CC=gcc-11 CXX=g++-11 ./ns3 configure --enable-examples --build-profile=optimized
./ns3 build scratch/ca_tbl_causal_sim                              # ~15 min on 4 cores
(nohup bash tools/nullcontrol/rebuild_reference.sh $HOME/catbl_ref_spec0 > ~/ref.log 2>&1 &)   # 20–40 min
cd analysis/accuracy_replay
for s in $(seq 1 20); do
  ./runcell.sh BASE_A $s --arms=ca_tbl_full,fixed_threshold,static_868,static_2p4
  ./runcell.sh BASE_B $s --arms=ca_tbl_full,fixed_threshold,static_868,static_2p4 --boxHalf=750 --finderMode=1
  ./runcell.sh BASEC_A $s --arms=poucet_fusion,lora_fusion
  ./runcell.sh BASEC_B $s --arms=poucet_fusion,lora_fusion --boxHalf=750 --finderMode=1
done      # set BIN and LIBS to your build; parallelise with xargs -P<cores>
python3 score.py                     # edit the item list at the bottom to your cell names
```
The preset already contains the four fixes; do not add them. Check your baseline reproduces §2 (check
`ca_tbl_full` first) and write down any difference. The comparator rows (`static_*`, `poucet_fusion`,
`lora_fusion`) are the same for every setting of your flags; run them once.

## 5. The work

### B1 — Instrument tag 0 and the subjects (45 min)

The existing exploratory patch logs only tag 0's `Step()` input (`analysis/accuracy_replay/rangelog_patch.txt`;
apply with `git apply`, **do not commit it as is**: build it as a default-off flag `--rangeLog` in your block
if you want to keep it, with a null control). Extend it:
1. **Subjects.** Same logger before the subject `Step()` calls (`sb.policy.Step(` in the subject loop, ~line
   10387; plain relays at `tags[pt].policy.Step(`, ~line 9719). One file per subject:
   `ranges_<arm>_seed<N>_subj<k>.csv`.
2. **Chain and poll counters, one row per cycle per node**, `chain_<arm>_seed<N>.csv`: `t, node, activeTier,
   awakeMask, floorOn, dualOn, polls868, replies868, polls2p4, replies2p4, relayLegsOffered(depth 1/2/3+),
   admitted, rejectedA1..A5, holdersEligible, holdsFix`. Insert the counters in `rangeTier` (where `received`
   is set), `RelayChain::RangeShortRelays` and `RangeForSubject`, and where `SubjectHolds` is called.
No RNG draw, no change to any decision. NC1–NC9 with the patch.

### B2 — Diagnose, one page (75 min; cut at 2 h)  ·  write TECH_DEBT N.35

Answer these in order, each with a number from the baseline cells (seeds 1–10, configurations A and B, arms
`ca_tbl_full` and `static_2p4`):

| # | question | test | decides |
|---|---|---|---|
| Q1 | which tier do relay legs use for tag 0 and for subjects, and how often? | share of relay legs by tier, from `chain_*.csv`, hybrid vs `static_2p4` | H1 |
| Q2 | does admission A1 refuse relay legs on 868 MHz? | `rejectedA1` by the tier of the cycle's active tier | H1 |
| Q3 | do holders exist and are they awake on 2.4 GHz when the hybrid's tag 0 is on 868? | `holdersEligible`, `awakeMask` | H1 vs a wake-state cause |
| Q4 | are relay legs also lost to the variance budget (A2) or geometry (A3)? | `rejectedA2/A3` (hybrid: 12 and 21 in one cell; small) | rules out H3 |
| Q5 | do subjects' extra 868 MHz fixes and TDoA solves hurt them? | per subject, replay (`replay.py`; feed the subject logs) with and without the floor ranges and the TDoA fixes; compare with `static_2p4`'s subjects | H2 |
| Q6 | how much of the subjects' error is time off 2.4 GHz? | per subject-cycle: error against active tier | H2/H3 |

**Contradiction to resolve first.** Hybrid subjects fix far more often (B: 0.78 / 0.55 / 0.39 / 0.10 vs
0.18 / 0.13 / 0.11 / 0.09) yet tag 0 is offered 6.6× fewer relay legs. So "holder has a fix" is not the
limiting condition; look at the tier of the exchange and the awake masks first.

Output: one page in TECH_DEBT N.35 naming the cause (or causes) with the numbers. If B2 does not name a
cause by the cut, ship the page as the deliverable and stop.

### B3 — Fix the cause B2 names (75–90 min; cut at 2.5 h)  ·  gates G110, G111

Behind one default-off flag per cause. Candidate fixes, in the order the evidence favours:

**F1 — `--dualBaseRelayTier` (for H1).** Under the dual base, run the relay chain (tag 0's
`RangeShortRelays` and the subjects' `RangeForSubject`) on 2.4 GHz when the active tier is 868 MHz, and give
admission A1 that tier. The dual base already deliberately breaks the one-tier-per-cycle rule (RULES.md §6)
and bills every radio that ranged; bill the relay exchange on the 2.4 GHz radio it used. Check the `awake`
masks: subjects already get `dualS ? 3u`; make sure the plain relays' mask includes 2.4 GHz.
**F2 — `--subjectFloorGate` (for H2).** For subjects only: drop their 868 MHz floor fix, or their TDoA fix,
on cycles that have ≥ 2 precise ranges. Name and constants are OURS; say so in the help text.
**F3 — only if neither fits:** whatever Q5/Q6 point to; write the flag and the reasoning into N.35.

Each flag: NC1–NC9 with the flag off; gate (G110 for F1, G111 for F2; `scripts/acc_b_gate.{sh,py}` and
`_selftest.sh`) checking: off ⇒ identical outputs; on ⇒ the counter it is meant to move moves (relay legs
offered on 2.4 GHz > 0 when the active tier is 868; the subject floor fixes dropped > 0); static,
`poucet_fusion`, `lora_fusion` rows byte-identical on vs off; negative fixture: run the gate against two
runs that are both "off" labelled on-vs-off and the "moved" check must fail.

**Pre-register now** (`report/PREREG_ACC_B_HYBRID.md`, one page, committed before B4): populations (seeds
1–20, 720 cycles; tag 0 for CEP; five nodes for availability and the precise-range counts), the predictions
and falsifiers of §6 (fill the thresholds from B2's numbers), estimator (paired seed bootstrap), the
acceptance scoreboard of §7, "what this cannot establish".

### B4 — Evaluate (50 min compute, 30 min write-up)

Cells: hybrid arms only (`ca_tbl_full,fixed_threshold`) × `{A, B}` × seeds 1–20, flags off and on, into
`results/acc_b_off` and `acc_b_on` (the off cells are your baseline cells). Comparator arms are reused from
the baseline (check by `cmp` that `static_2p4` is byte-identical on and off in a flags-on cell with
`--arms=ca_tbl_full,static_2p4`). `scoreboard_pairs.py` (edit the cell names): `ca_tbl_full` minus every
other arm, plus the precise-range counts per cycle by anchor class (`precise_ranges.py <cell> <arm>`
reproduces the §1 numbers from the range logs).
Write `report/ACC_B_RESULTS.md` (exploratory grade); TECH_DEBT N.36 (outcome), N.37 (subjects); commit; push.

### B5 — Dual base on demand, energy (90 min; only if B3 finished early)  ·  gate G112

`--loraDualBaseOnDemand` (default off): range 2.4 GHz only when the 868 MHz floor has fewer than three ranges
or a 2.4 GHz neighbour was heard within the TTL. Aim: bring 5 km energy back toward the old preset's 12.4 J
per node (16.7 J today) without losing the availability the dual base buys (+0.118 at 5 km, FIXES-N26 §2).
This is the follow-up FIXES-N26 named. Predictions: A subject energy ≤ 13.6 J with fix availability gain ≥
+0.08 over the old preset's 0.174. Falsifier: either missed. Separate pre-registration.

## 6. Registered predictions and falsifiers (closed loop, paired against the unmodified preset)

| id | prediction (thresholds to be finalised from B2) | falsifier |
|---|---|---|
| P-B1 | B, tag 0: precise ranges per cycle ≥ 2.4 (1.80 today); peer ranges ≥ 1.0 per cycle (0.23 today); relay legs offered ≥ 1 000 per 720 cycles (214 today; `static_2p4` 1 413) | precise ranges rise < 20 % |
| P-B2 | B, tag 0: CEP50 and CEP95 each ≥ 10 % lower | no significant change in either |
| P-B3 | A: five-node within 25 m against `static_2p4`: no significant LOSS (−0.070 today); every subject's median error ≤ 25 m (19.5–37.3 m today) | the loss stays significant |
| P-B4 | fix availability not below the baseline's by more than 0.01 in A or B; energy per node ≤ +15 % | either breached ⇒ reject the flag; B2 stays the deliverable |
| P-B5 | static, `poucet_fusion`, `lora_fusion` rows byte-identical on vs off | any difference ⇒ a flag leaks into a non-hybrid arm: fix before reading results |

## 7. Acceptance scoreboard (fixed now)

For A and B, `ca_tbl_full` against each of `fixed_threshold`, `static_2p4`, `static_868`, `poucet_fusion`,
`lora_fusion`, paired over seeds 1–20, 95 % seed bootstrap, same binary: five-node fix availability — WIN
against every arm; five-node within 25 m and tag-0 CEP50, CEP95 — no LOSS against any arm and WIN against at
least three; energy reported, not scored. Your part is judged on the cells it can move: the A five-node
within-25 loss and the endgame precise-range supply. Expected (opinion): the loss closes if H1 or H2 is
confirmed; the endgame accuracy gain is small because the endgame is nearly saturated.

## 8. Timetable

| clock | work |
|---|---|
| 0:00–0:50 | Step 0 (reference rebuild runs beside) |
| 0:50–1:35 | B1 |
| 1:35–2:50 | B2 (cut at 3:00) |
| 2:50–4:20 | B3 and pre-registration (cut at 5:00) |
| 4:20–5:20 | B4 compute and note, TECH_DEBT, commit, push |
| optional | B5 |

Compute: hybrid arms only, two settings: 40 cells per setting × ~2.5 min = 100 core-minutes (25 min on 4
cores, 10 min on 12). Baseline comparators once: ~150 core-minutes.

## 9. Risks

| risk | effect | mitigation |
|---|---|---|
| B2 names a cause whose fix is not flag-sized | B3 becomes a finding | the diagnosis page is a valid deliverable |
| the dual base's relay ranging costs energy | P-B4 energy bound breached | bill honestly; the flag is rejected, not tuned away |
| a flag leaks into a non-hybrid arm | invalid comparisons | P-B5 byte-identity check on every run |
| `-march=native` | byte-identity is per machine | rebuild the reference here; never compare across machines |
| Part A lands first and shifts the baseline | your gain shrinks or grows | you are judged on your binary; the merge test measures the interaction |

## Appendix — after both parts (not a dependency)

Whoever finishes second merges `acc/a-filter` and `acc/b-hybrid` (two append-only conflicts expected: the
flag block and `catbl_gates.sh`; keep both sides), builds, runs NC1–NC9 with every flag off, runs the 2 × 2
{A on/off} × {B on/off} scoreboard on all seven arms, then tests whether a consistent covariance makes the
integrity check usable (re-fit `--integrityChi2` on seeds 21–40 with the `fixes_n26_run.sh` fit procedure,
then the fault matrix against `fixed_threshold`). That last test is the only route to beating
`fixed_threshold` in anything; a tie is a valid result. Then the full gate suite once (~25 min).
