# CA-TBL — 6–7 hour plan to make `ca_tbl_full` the best arm, in two independent parts

2026-10-08. Supersedes `plan.md` for the next working session; `plan.md` keeps the long-range
list. Evidence: `context.md` (C§n) and §1 here. ML is out of scope. The evidence-fix stream's
items (S3 in `WORKSTREAMS_2026_10_09.md`: ToF field-error flags, citations, tier-1 validation,
UWB-reach write-up) are **not repeated**.

## 0. Verdict first

| goal | reachable in 6–7 h? | basis |
|---|---|---|
| Best **fix availability** of every arm | **Already true** on the new preset; keep it | §1: 0.285 / 0.562 vs next best 0.239 / 0.506 |
| Best **accuracy** vs `poucet_fusion` and `lora_fusion` | **Already true** with a consistent IMU covariance (Qu × 10, applied to all arms) | §1: significant on every accuracy metric, both configurations |
| **No loss** to `static_2p4` and `fixed_threshold` on accuracy | **Likely**, needs Part A's error-state filter and Part B | §1: today one significant loss (5 km, five-node within 25 m, −0.067 [−0.129, −0.016]) |
| **Strictly beat** `static_2p4` / `fixed_threshold` on accuracy | **Not at the endgame**: it is saturated at ≈ 2–3 m and the arms tie. At 5 km, possible, not certain | §1 |
| Beat `fixed_threshold` on anything in fault-free runs | **No principled route.** Same controller; PCS adds nothing (CAL-01). Only the integrity check can differ, and only under faults. A bet, in the join step | C§4.3, §6 here |
| Lowest energy | **No.** `static_2p4` is 5.7 J per node against 26 J (endgame) | §1 |
| A perfect IMU model | Not needed. Part A is built to be insensitive to IMU quality | C§6.8 |

"Best" below means: **a significant win or a statistical tie on every accuracy and availability
metric against every arm, and at least one significant win against each**, measured on the same
binary, paired by seed. Strictly "best on everything" is not a coherent target (the superset
argument in `LORA_CMP2_RESULTS.md` §3.3: an always-on fusion uses a superset of the information).

---

## 1. Where the new preset stands (research for this plan)

Baseline = the 102-setting preset (`e308c14f`: FIXES-N26's four fixes now in `--specProfile`).
My cells with `--specProfile` plus those four flags are identical to the tracked FIXES-N26 `ALL_main` and
`ALL_end` cells (320 of 320 compared summary values), and the rebuilt binary at Qu × 1 is identical to
my first build (96 of 96). Closed loop, 20 seeds,
`--duration=1800`, A = 5 km, B = endgame. **Exploratory: built from a patched tree, not citable.**

**Five-node fix availability / five-node within 25 m / tag-0 CEP50 / CEP95 (m) / J per node**

| arm | A | B |
|---|---|---|
| `ca_tbl_full`, preset | 0.285 / 0.613 / 16.2 / 85.0 / 16.7 | 0.562 / 0.968 / 4.8 / 16.1 / 23.0 |
| `ca_tbl_full`, IMU Qu × 10 (all arms) | 0.278 / 0.638 / 11.7 / 61.7 / 17.3 | 0.575 / 0.977 / 2.7 / 8.0 / 26.0 |
| `fixed_threshold`, Qu × 10 | 0.279 / 0.627 / 13.9 / 100.4 / 16.7 | 0.544 / 0.975 / 2.8 / 9.1 / 19.9 |
| `static_2p4`, Qu × 10 | 0.024 / 0.705 / 11.1 / 67.8 / 14.3 | 0.269 / 0.969 / 2.7 / 7.6 / 5.7 |
| `poucet_fusion`, Qu × 10 | 0.239 / 0.519 / 31.8 / 215.6 / 462.1 | 0.506 / 0.845 / 3.7 / 14.2 / 412.3 |
| `lora_fusion`, Qu × 10 | 0.224 / 0.525 / 34.2 / 2 226.7 / 16.9 | 0.505 / 0.863 / 3.9 / 15.1 / 8.8 |

("Qu × 10" multiplies the IMU process-noise increment handed to the filter, in a throwaway
patch, `analysis/accuracy_replay/rangelog_quscale_patch.txt`. Part A makes it a flag.)

**Paired verdicts, `ca_tbl_full` (Qu × 10) minus each arm (Qu × 10), 95 % seed bootstrap.**

| vs | 5 km | endgame |
|---|---|---|
| `poucet_fusion` | **WIN** every metric (CEP95 −95.8 m [−151, −52]) | WIN fix avail, five-node within 25 m, CEP50, CEP95; tie tag-0 within 25 m |
| `lora_fusion` | **WIN** every metric except energy (tie) | WIN all accuracy and availability; energy loss |
| `static_2p4` | fix avail **WIN** (+0.254); five-node within 25 m **LOSS** (−0.067 [−0.129, −0.016]); CEP50/95 tie | fix avail WIN (+0.306); accuracy tie; energy loss |
| `fixed_threshold` | tie on everything | fix avail WIN (+0.031); accuracy tie; energy loss (+6.1 J) |

Reading it:
1. **One honest lever has already moved the board.** Multiplying the IMU increment covariance by
   10 takes the endgame from 4.8 / 16.1 m to 2.7 / 8.0 m (−44 % / −50 %) and 5 km from 16.2 / 85.0
   to 11.7 / 61.7 m (−28 % / −27 %), for every arm. It works because the covariance was wrong
   (C§6.5), not because `ca_tbl_full` changed.
2. **The 5 km loss to `static_2p4` is a subject effect.** Tag 0 ties. Subjects 2–5 under
   `ca_tbl_full` have median error 19.7 / 19.5 / 37.3 / 32.1 m against `static_2p4`'s
   15.9 / 16.3 / 18.2 / 23.4 m, while fixing on 38 / 24 / 11 / 1 % of cycles against about 1 %.
   The extra tier-1 fixes are not helping them. Cause **not established**.
3. **Endgame range supply.** Precise ranges (reported σ < 40 m) per cycle for tag 0: `static_2p4`
   2.67 (≥ 3 on 50 % of cycles), `ca_tbl_full` preset 1.80 (≥ 3 on 27 %). The shortfall is peer
   (other subject) ranges: 1.52 against 0.23 per cycle. The old preset had 0.64; the dual base
   moved the hybrid off 2.4 GHz as its active tier (86 % → 36 % of cycles). Cause **not
   established**.
4. **Replay is now trustworthy for `ca_tbl_full` on this preset**: replayed baseline 15.2 / 87.8 m
   (A) and 4.8 / 15.8 m (B) against the simulator's 16.2 / 85.0 and 4.8 / 16.1. For `static_2p4`
   at 5 km it is not (21.0 vs 12.5 m CEP50), so closed loop decides that comparison.
5. **Error-state filter, replay on this preset's range streams** (tag 0, 20 seeds):

   | filter | A CEP50 / CEP95 | B CEP50 / CEP95 |
   |---|---|---|
   | baseline | 15.2 / 87.8 | 4.8 / 15.8 |
   | Qu × 10 + gates | 10.4 / 86.7 | 2.8 / 10.5 |
   | bias + scale states + gates, prior 100 m | **7.2 / 60.4** | **1.5 / 4.7** |
   | same, prior 1 m | 4.9 / 47.0 | 1.4 / 4.1 |

   Closed-loop Qu × 10 against the replay's Qu × 10: CEP50 11.7 vs 10.4 m (A) and 2.7 vs 2.8 m (B);
   CEP95 61.7 vs 86.7 m (A) and 8.0 vs 10.5 m (B). The replay predicts direction and the median
   well, and is pessimistic on the tail.
6. **Two ideas tested and dropped** (replay, this preset): dropping or inflating the 868 MHz ranges
   to absorb their long-range bias makes things worse or changes nothing (A CEP50 7.2 → 18.5 m
   when dropped). The 868 MHz ranges carry real information.
7. **Untested here:** the integrity check's distance on a consistent covariance. My replay's
   fix solver diverged on the 868 MHz ranges, so the number is not usable. It is a join-step
   experiment (§4.3).

---

## 2. How the two parts stay independent

| | Part A — filter consistency | Part B — hybrid range supply and subjects |
|---|---|---|
| runs on | system 1 | system 2 |
| scope | the shared estimator: **every arm** gets it, so comparisons stay fair | the hybrid arms only (`ca_tbl_full`, `fixed_threshold`, `ca_tbl_no_integrity`) |
| files | `ca-tbl-policy.{h,cc}` (EKF class, `Step`), `ca-tbl-imu-model.{h,cc}`, the two `SetImuControl` call sites | the ranging and dual-base block of `RunArm` (`rangeTier`, `otherLoraMeas`), neighbour/poll logic, subject loop |
| flags | block headed `// --- S2-A accuracy flags ---` | block headed `// --- S2-B accuracy flags ---` |
| gates | G100–G109 | G110–G119 |
| TECH_DEBT | N.30–N.34 | N.35–N.39 |
| pre-registration | `report/PREREG_ACC_A_*.md` | `report/PREREG_ACC_B_*.md` |
| results | `results/acc_a_*` | `results/acc_b_*` |
| baseline it is measured against | the 102-setting preset, flags off | the same |
| ends with | a standalone adoption verdict | a standalone adoption verdict |

(Numbers are inside S2's reserved ranges in `WORKSTREAMS_2026_10_09.md` §3.)

**Rules that make it independent.**
- Each part is judged against the *unmodified preset on its own binary* (rule 8), never against
  the other part.
- Neither part edits the other's files. The only shared edits are append-only: the flag block in
  `ca_tbl_causal_sim.cc` and `scripts/catbl_gates.sh`. Expect two trivial conflicts at merge;
  keep both sides.
- Part A's flags apply to all arms; Part B's flags are read only by hybrid arms. A flag from
  either part off must leave the other part's outputs byte-identical.
- Both start from the same commit: `git rebase --onto origin/work/2026-09-27-spec-b1 a0829213
  work/2026-10-09-acc-reruns` (the branch carries only documents and `analysis/`, so no conflict).
  Check that the branch head still includes `e308c14f`.
- The scoreboard scripts are shared and already written: `analysis/accuracy_replay/score.py`,
  `scoreboard_pairs.py`, `runcell.sh`, `runq.sh`. Neither part waits for the other for tooling.

**Interaction, handled at the join, not as a dependency.** Part A can change how well Part B's
extra ranges are used; Part B changes which ranges exist. Each part's own verdict is valid alone.
§4 measures them together once.

---

## 3. The fast lane: what is kept, what is deferred

The repo's rules (RULES.md §1, §4) cannot all be met in full in 6–7 hours. Kept: **default-off
flag; null control NC1–NC9 on both configurations; a one-page pre-registration committed before
the first measured cell; one gate with a negative fixture per mechanism; paired 20-seed
evaluation; `--outdir` in a temp directory for every run.** Deferred to a later session: the
formal frozen-worktree campaign with `dirty 0` sidecars, the adoption into the preset (S1's
call), and the full registered write-up. Results from this session are therefore **exploratory
grade**, and each part's note says so.

Time per mandatory step (estimates, 4-core machine; a 12-core machine is ~2.5× faster):
build 15 min · NC1–NC9 reference rebuild + check ≈ 20 min · 20-seed A+B evaluation of 4 arms ≈
25 min · full gate suite ≈ 25 min (run once, at the join).

---

## 4. Part A — filter consistency (system 1) ≈ 4.5–5 h

**Why.** The EKF's covariance is wrong by 3–54× in variance because persistent IMU errors enter as
independent increments (C§6.5). Fixing it is the largest measured lever (§1 items 1 and 5) and it
makes the innovation gates safe, which are currently harmful on an inconsistent covariance.

| step | what | time | stop rule |
|---|---|---|---|
| A0 | rebase, build optimized, rebuild null-control reference, run baseline 20-seed A and B for all 6 arms into `results/acc_a_base` (temp dir first) | 50 min | — |
| A1 | **`--imuQuScale`** (default 1): multiply the IMU increment covariance at both `SetImuControl` sites (tag 0 and subjects). NC1–NC9. Gate G100 + fixture | 40 min | — |
| A1e | scan k ∈ {3, 10, 30}, all arms, 20 seeds; choose k\* | 35 min compute, overlaps A2 | — |
| A2 | **`--imuErrorStates`** (default off): a 5-state filter `[x, y, ψ, b, s]`, `Δ = (1−s) R(−ψ) u`, `ψ += b·dt`, priors from `CaTblImuParams`, **plus a bias random walk `q_b` (OURS, swept)** because a real gyro bias drifts. Same external interface as `CaTblEkf` (about 20 methods; `X, Y, Pxx, Pxy, Pyy, TracePos, Prior*, Update, FusePositionCov, PositionNis, MahalanobisFromPriorWithR, RevertToPrior, InflatePos`); expose `Vx/Vy = u/dt`. | 2–2.5 h | cut at 3 h; ship A1 only |
| A2v | **Verification against the Python reference**: `analysis/accuracy_replay/replay.py::AugEkf` fed the same logged stream must agree with the C++ filter to < 0.5 m median. NC1–NC9. Gate G101 + fixture (flag off byte-identical; with `q_b → 0` and the generator's priors, the filter matches the reference stream) | 40 min | — |
| A3 | **`--imuBiasScale`, `--imuScaleScale`** (default 1): multipliers on `CaTblImuParams`. Closed-loop stress at ×1, ×10, ×30, and a mis-declared prior (filter σ\_b 0.1× and 10× the truth) | 30 min code + 30 min compute | — |
| A4 | 20-seed A and B, **all arms**, flags off / A1 / A2 → `results/acc_a_*`; `scoreboard_pairs.py`; `PREREG_ACC_A_*.md` committed **before** this step; `TECH_DEBT` N.30–N.34 | 60 min | — |

**Registered predictions** (closed loop, tag 0, paired against the unmodified preset; the replay
numbers are in §1 and the margins are deliberately wide):
- A1 at k\* ≈ 10: A CEP50 ≤ 13 m and CEP95 ≤ 70 m; B CEP50 ≤ 3.2 m and CEP95 ≤ 10 m (measured at
  k = 10: 11.7 / 61.7 and 2.7 / 8.0 — a replication, not a discovery).
- A2: A CEP50 ≤ 9 m, CEP95 ≤ 62 m; B CEP50 ≤ 2.0 m, CEP95 ≤ 6 m. Median `error² / trace(P)` ≤ 10
  (37.5 baseline, 6.3 in replay).
- A2 at gyro bias ×10 and ×30: B CEP50 within 25 % of the ×1 value (replay 2.1 / 2.3 / 2.4 m).
- No arm's availability moves by more than 0.01 (the flags change the covariance, not the ranges;
  escalation counts will move and are reported).

**Falsifiers.** A1: B CEP95 improvement < 20 % at every k. A2: improvement over A1 < 20 % in B,
or any stress case worse than the unmodified preset by > 10 %, or the C++ filter disagrees with the
reference by > 0.5 m (a porting error). Any of these: stop at the previous step and say so.

**Honest limits.** The filter's model is the simulator's IMU generator (constant bias, constant
scale, angle random walk). The stress cases (bias random walk, mis-declared priors, ×10 and ×30
bias) are what stand in for "a realistic IMU", which you said cannot be built now. They show the
estimator does not depend on the generator being right to within an order of magnitude; they do not
show it works on hardware.

**Side effects to report, not to fix.** PCS thresholds were fitted on the old covariance (CAL-03).
Their inputs change; the PCS code does not. Do not touch the PCS form (RULES.md §2). The
integrity check's fault-free SUSPECT share is reported before and after.

**Deliverable.** A standalone verdict: with A1 (or A1 + A2) on, `ca_tbl_full`'s row of the §5
scoreboard, and every other arm's, on the part's own binary.

---

## 5. Part B — hybrid range supply and subjects (system 2) ≈ 4.5–5 h

**Why.** Two measured shortfalls that a covariance fix does not obviously touch (§1 items 2, 3):
the endgame range supply (peer ranges 1.52 → 0.23 per cycle against `static_2p4`), and the 5 km
subjects, who are worse than `static_2p4`'s while fixing 10–40 × more often.

| step | what | time | stop rule |
|---|---|---|---|
| B0 | rebase, build, null-control reference, baseline 20-seed cells for all 6 arms into `results/acc_b_base` | 50 min | — |
| B1 | **Instrument.** Extend the range log to subjects (`pmeas` before `tags[pt].policy.Step`, `sb.policy.Step`), and log per cycle, per tier and anchor class: polls attempted, replies received, the responder's radio state | 45 min | — |
| B2 | **Diagnose, one page.** (a) Why are endgame peer 2.4 GHz ranges 6.5× fewer under the dual base? Test in order: responder's 2.4 GHz radio asleep (subjects are only awake on their active tier); neighbour entry expiry; half-duplex chip collisions with the 868 MHz exchange; poll order. (b) Why are 5 km subjects worse than `static_2p4`'s? Replay each subject with and without its 868 MHz ranges and its TDoA fixes; compare time on 2.4 GHz (hybrids 33–85 % vs 100 %). Write `TECH_DEBT` N.35 | 75 min | cut at 2 h |
| B3 | **Fix the cause B2 names**, behind one default-off flag. Decision tree: *responder asleep* → keep the 2.4 GHz receiver on for hybrid nodes under the dual base, energy measured; *entry expiry* → extend TTL for 2.4 GHz peers; *half-duplex* → order the exchanges (2.4 GHz first when it is the productive tier); *subjects over-fusing 868 MHz* → raise subjects' gate or drop their floor fixes when ≥ 2 precise ranges exist. NC1–NC9. Gate G110 + fixture. **Pre-register after B2 and before any measured cell** | 75–90 min | cut at 2.5 h; ship B2 as a finding |
| B4 | 20-seed A and B, hybrids on/off → `results/acc_b_*`; `scoreboard_pairs.py`; `TECH_DEBT` N.36 | 50 min | — |
| B5 (only if B3 finished early) | **`--loraDualBaseOnDemand`**: range 2.4 GHz only when the floor has < 3 ranges or a 2.4 GHz neighbour was heard. Aimed at energy, which the owner-override preset made worse (16.7 J vs the old 12.4 J at 5 km). Gate G111 | 90 min | optional |

**Registered predictions (set after B2, drafted here).**
- Endgame precise ranges per cycle for tag 0 ≥ 2.4 (today 1.80; `static_2p4` 2.67); peer share
  ≥ 1.0 per cycle (today 0.23).
- 5 km subjects' five-node within-25 m: no LOSS against `static_2p4` (today −0.067); subject
  median error ≤ 25 m on every subject (today 19.5–37.3 m).
- Fix availability not lower than the preset's by more than 0.01 in either configuration.
- Energy per node ≤ +15 % over the preset in either configuration.

**Falsifiers.** Endgame precise ranges rise < 20 %, or fix availability falls > 0.01, or energy
rises > 15 %: the fix is rejected and B2's finding is the deliverable.

**Honest limits.** The cause of both shortfalls is unknown today. B2 may name a cause whose fix is
not a flag-sized change; then B3 is a finding and the cure is a later work package. This part has
the larger uncertainty and the smaller expected accuracy gain (the endgame is saturated at ≈ 2–3 m).
Its expected value is mostly in the 5 km subjects and in endgame range supply.

**Deliverable.** A standalone verdict on the part's own binary, plus the diagnosis page.

---

## 6. Join — about 1–1.5 h, after both parts

Not a dependency: each part is already judged alone. This step merges and measures.

| step | what | time |
|---|---|---|
| J1 | merge both branches into one; resolve the two append-only conflicts (flag block, gate list); build; NC1–NC9 with every flag off | 30 min |
| J2 | full scoreboard, 20 seeds, A and B, all 7 arms, parts on / off (2 × 2); `scoreboard_pairs.py` | 40 min compute |
| J3 | **the integrity bet**: with the consistent covariance (Part A), re-fit `--integrityChi2` on seeds 21–40 (`scripts/fixes_n26_run.sh` fit procedure) and run the fault matrix (`E` cells, seeds 1–10) for `ca_tbl_full` against `fixed_threshold` | 45 min compute |
| J4 | full gate suite once; record the result and the expected reds only | 25 min |

**Why J3 is the only route to beating `fixed_threshold`.** In fault-free runs the two share every
mechanism but PCS and the integrity check, and PCS ranks error no better than the covariance
trace (CAL-01). The integrity check is silent at its fitted threshold of 1 992.9 (recall ≤ 0.031,
FIXES-N26 §3) because its distance is heavy-tailed. The hypothesis in N.15 is that the covariance it
reads is the cause; Part A directly tests that. If the fault-free p99 of the distance falls near
χ²₀.₉₉ = 9.21, detection becomes possible and `ca_tbl_full` can win in the fault matrix. If it does
not, the verdict against `fixed_threshold` is a tie, stated plainly.

---

## 7. Acceptance scoreboard (fixed now, applied at the join and at each part's end)

For each configuration (A and B), `ca_tbl_full` against each of `fixed_threshold`, `static_2p4`,
`static_868`, `poucet_fusion`, `lora_fusion`, paired over seeds 1–20, 95 % seed bootstrap, same
binary, all arms with the same filter flags:

| metric | pass |
|---|---|
| five-node fix availability | WIN against every arm |
| five-node within 25 m | no LOSS against any arm; WIN against ≥ 3 |
| tag-0 CEP50, CEP95 | no LOSS against any arm; WIN against ≥ 3 |
| energy | reported, not scored |
| fault matrix (J3) | WIN or tie against `fixed_threshold`; a tie is reported as a tie |

Predictions for what this will show (opinion, from §1): fix availability and the two fusion
comparators — yes; `static_2p4` at the endgame — tie; `fixed_threshold` — tie unless J3 succeeds.
After Part A, `poucet_fusion` and `lora_fusion` also gain from the consistent filter (replay,
endgame: `poucet_fusion` 1.5 / 4.1 m against `ca_tbl_full` 1.4 / 4.1 m), so the endgame win over
`poucet_fusion` may become a tie.

---

## 8. Timetable (wall clock, two systems in parallel)

| | system 1 (Part A) | system 2 (Part B) |
|---|---|---|
| 0:00–0:50 | A0 | B0 |
| 0:50–1:30 | A1 | B1 (45 min) |
| 1:30–4:00 | A2 + A2v (A1e runs beside it) | B2 (75 min), B3 (75–90 min) |
| 4:00–5:00 | A3, A4 | B4 |
| 5:00–5:30 | note, commit, push | B5 if early; note, commit, push |
| 5:30–7:00 | **join J1–J4** | |

Each part is cut at the stop rule in its table if it slips; the cut still ships a verdict. Total
work if done on one machine: ≈ 10 h.

## 9. Risks

| risk | effect | mitigation |
|---|---|---|
| A2's port disagrees with the reference | wasted 2–3 h | A2v comes before any simulator run; cut to A1 |
| Closed loop does not reproduce the replay for A2 | smaller gain | the falsifier names it; A1 alone is already measured |
| B2 finds a deep protocol cause | B3 becomes a finding | the deliverable is the diagnosis; no flag shipped |
| Parts interact (A makes B's ranges less necessary) | B's gain shrinks at the join | J2 is a 2 × 2; B's value is judged alone and jointly |
| `-march=native` | byte-identity is per machine | each system rebuilds its own null-control reference; never compare outputs across machines |
| Results are exploratory grade | not citable | the formal rerun is S1's registered campaign on the merged preset |

## 10. Not in this plan

ML of any kind; the S3 items; any change to PCS; hop-provenance; UWB discovery work (with the
four preset fixes there are UWB fixes on all endgame seeds and the earlier case for a UWB cap is
weaker); adopting any flag into `kSpecProfile` (S1 only).
