# PREREG B3 — regenerate the claim-bearing evidence on one commit

**Registered before the first cell runs.** Rule 13. This file and its driver,
`scripts/b3_driver.sh`, are committed together in one commit. Every output directory named
below is created by that driver after the commit, so its timestamps are checkable against it.

Branch `work/2026-09-27-b3-regen`, worktree `~/catbl_b3`, cut from `084a7ff`. This commit
touches `report/` and `scripts/` only; `src/` and `scratch/` are byte-identical to `084a7ff`.

---

## 0. Why, and what this is not

`docs/TECH_DEBT.md` §L.3, §I.6 and §K.2: *no result in the tree reproduces under* the
corrected environment. The owner authorised regeneration on 2026-09-27. This registers the
**seven claim-bearing campaigns**, in the order §L.3 and §K.2 require: the integrity result
first because its sign could move, then the PCS forms because `gated` was adopted on their
evidence, then the rest.

What this is **not**:

- **Not an in-place regeneration.** Every cell writes to a new `results/regen27_<name>/`
  directory. No tracked results directory is opened for writing (rule 7). The old
  directories remain the record of what those runs produced.
- **Not a code change.** The binary is built from `084a7ff` in this worktree,
  `--build-profile=optimized`, `CMAKE_CXX_COMPILER=/usr/bin/g++-11`. Re-checked on
  2026-09-28 immediately before this commit: `./ns3 build` reports `ninja: no work to do`,
  and the eight unit suites pass on that build — `ca_tbl_packet_validator` 38,
  `ca_tbl_session_test` 29, `ca_tbl_routing_test` 21, `ca_tbl_lifecycle_test` 16,
  `ca_tbl_netlayer_test` 15, `ca_tbl_relay_admission_test` 14, `ca_tbl_sparse_anchor_test`
  11, `ca_tbl_loocv_test` 8; 0 failed.
- **Not the Bar-1 tree.** Routing, relay depth 2 and anchor grant gossip are being wired
  on another branch at the time of registration and are **not** in this binary. If they
  land default-off with a byte-identical null control (`CLAUDE.md` §4), these results stay
  valid for default-flag invocations of the merged tree. If any of them lands default-on,
  this campaign describes `084a7ff` and must be re-run for the merged tree.
- **Not the write-up.** Results, repointed citations and corrections-register entries go
  in later commits, per §I.6 step 3.

## 1. The sampler decision, stated once and binding on every section below

`LoRaDistributionSampler` **stays unseeded** (`CLAUDE.md` §5, a decision not to fix;
`TECH_DEBT` §1.1). Any change in transmission count moves every link's draws. So:

- **Every cross-binary magnitude comparison between a regenerated number and its
  pre-correction original is inadmissible.** An original number is quoted in §4 only to
  name the claim being re-tested, never as a baseline to difference against.
- **A regenerated number stands on its own registered control**: a paired comparison
  inside the same binary, same seed, same invocation apart from the treatment flag.
- The predictions in §4 are therefore predictions of the **verdict under the claim's own
  control** (reproduces / sign flips / withdrawn), not of a magnitude.
- Byte-identity to an original *is* admissible, because it is a control-flow fact: it says
  nothing on that path moved. A magnitude difference may not be attributed to anything.

A worked example of why this matters, from files already tracked: `results/L1_imuoff_*`
(`808885a`) and `results/k10_wanmesh_move_*` (`5d20de9`) carry **identical invocations**.
Their accuracy columns agree (mean `cep95M` 3261.4 m and `fixCycles/steps` 0.647 on
`ca_tbl_full` over the 20 seeds in both, recomputed for this registration) while `totalJ`
does not (82.5 J against 54.3 J). The cause is known and it is not physics: `4779bf7`,
which sits between the two commits, changed the `--tier1ChargeReplies` default to true, so
one family bills tier-1 replies and the other does not (§2). The tree already mixes energy
accounting across commits inside one claim chain. Regenerating on one commit removes that;
it does not license comparing across it.

## 2. Provenance, and which SHA a sidecar will carry

Every cell runs from cwd `~/catbl_b3` as `./build/scratch/ns3.47-ca_tbl_causal_sim-optimized`,
the `argv[0]` the originals recorded. At startup the simulator records
`git rev-parse --short HEAD` and a dirty count
(`scratch/ca_tbl_causal_sim.cc:7929-7936`), which is
`git status --porcelain --untracked-files=no -- src scratch | wc -l`: tracked
modifications under `src/` and `scratch/` only. Untracked files — every `results/regen27_*`
output as it appears among them — do not count, and nothing outside those two directories
counts.

Sidecars will therefore cite **this commit**, not `084a7ff`. That is acceptable and
exact rather than approximate: this commit adds files under `report/` and `scripts/` only,
so `src/` and `scratch/` at this commit are byte-identical to `084a7ff`, and the binary
built from `084a7ff` *is* the binary of this commit. The driver accepts a completed cell
only if its sidecar reads `source <prefix of this commit> dirty 0 profile optimized` and
its invocation line ends with the registered arguments; anything else is abort A1.

**Environment.** HEAD defaults: `--envFacility` / `--vegFacility` follow the wide-area
setting (`envInt = 1`, `ENV_RURAL_FORESTED`; `vegWideInt = VEG_DENSE_FOREST`, i.e. 3;
`ca_tbl_causal_sim.cc:6574-6636`). The K-series invocations (K1, K9, K10, L1/L2) already
pinned `--envFacility=1 --vegFacility=3`, which is that same environment, so for them the
regeneration changes only the commit. The integrity and PCS-forms invocations did not pin,
so they now take the corrected environment — for those two it is the physics that moves,
and that is the point.

**Billing.** `--tier1ChargeReplies` defaults to **true** since `4779bf7`
(`ca_tbl_causal_sim.cc:6744`). Its help text still says "FALSE by default" (`:7482`); that
text is stale, and it is recorded here rather than corrected because correcting it touches
`scratch/`, which this registration must not. The C1–C4 originals were produced by binaries
that do not contain `4779bf7`, so they billed tier 1 as one broadcast with no replies;
every regenerated cell bills the replies. For those four campaigns energy therefore moves
for a reason that is not physics at all — one more reason §1 forbids cross-binary
magnitude comparison. The C6 and C7 originals (`4779bf7` itself, `4302918`) already
billed replies.

## 3. How multi-seed cells run

**Every multi-seed cell runs exactly as its original ran.** The K10 moving cells and the
IMU ceiling were run as four processes of five seeds (`--seedStart` 7, 12, 17, 22,
`--seedCount=5`, per their sidecars) and are regenerated that way. K1 was one process of
twenty per cell (`--seedCount=20`, seeds 7–26, per its sidecars) and is regenerated that
way. EDGE has no multi-seed original; it runs as one process of twenty per setting, the K1
shape, so that no cell in this campaign rests on a seed-order-independence assumption its
original did not already make.

An earlier draft split K1 and EDGE into 4 × 5 chunks behind a byte-identity pre-flight
("P0"). That was dropped before registration: it moved the K1 invocation away from its
sidecar, and the scheduling gain was about 2% of wall time (§9).

For analysis, a K10 or IMU cell is the concatenation of its four chunk CSVs, header once,
and must count exactly 120 data rows before anything is pooled (rule 12). A K1 or EDGE cell
is one file of 121 lines. That concatenation and a changed root path are the only changes
permitted to the named analysers.

## 4. The campaigns, in registered order

Common to all: `--trace=false` except C2, whose monotone control reads per-cycle `pcs`,
`traceSigma`, `errM` and `hadFix` from `trace_<arm>_seed<S>.csv`
(`scratch/python_handoff/analyse_pcs_forms.py:100-118`). `trace_*.csv` is excluded by
`.gitignore` (`/results/**` with only `arm_summary.csv`, `run_id.txt`,
`probe_range_sweep.csv` and `*.md` re-included) and is never tracked; the driver asserts at
start that a trace under `results/regen27_*` would be ignored and an `arm_summary.csv`
would not. Each cell's stdout goes to `run.log` beside it, which is ignored too.

### C1 — INTEGRITY: the switching-point integrity check earns its place

| | |
|---|---|
| Original | `report/INTEGRITY_CHECK_RESULTS.md`, `~/catbl_work/cc/{off,on}_s1..8` at `deb52da` (pre-K.2 split environment). Energy **−15.360 J** [−21.433, −9.233], 8/8; rmse **−0.743 m** [−1.052, −0.416], 7/8; escalations −5.625 [−8, −3] |
| Invocation | the sidecar's, verbatim: `--seed=S --duration=600 --boxHalf=1000 --nFinders=4 --finderMode=1 --sendHandshakeFrames=true --trace=false --arms=ca_tbl_full`, and for `on` add `--imuCrossCheck=true --corroborationCheck=true` |
| Seeds | 1–8, one process per seed per arm setting |
| Output | `results/regen27_integrity/{off,on}_s{1..8}` — **16 cells** |
| Complete when | each `arm_summary.csv` counts 2 lines (header + 1 row), plus §6 |
| Analysis | `scratch/python_handoff/analyse_integrity_ci.py results/regen27_integrity` — unchanged, the script takes its root as `argv[1]` |

- **Population check first (rule 4).** Each `on` cell's `run.log` ends with
  `INTEGRITY: escalations refused -- IMU inconsistency N, weak corroboration M`
  (`ca_tbl_causal_sim.cc:6364-6366`). If N + M = 0 on all eight seeds, the check never
  fired under the corrected environment: the verdict is **NO POPULATION** and the claim is
  withdrawn as unsupported, not reported as "no effect". Byte-identical `off`/`on` rows on
  all eight seeds mean the same thing. The converse does not hold — `--imuCrossCheck` can
  move rows without refusing anything — so differing rows alone do not establish a
  population. `run.log` is untracked; the write-up records the sixteen counts.
- **Which witness does the refusing is reported, not predicted.** The original found
  corroboration 16 against IMU 1 at seed 7; see the disclosure in §9.
- **P1.1 energy — REPRODUCES.** Paired mean on−off `totalJ` < 0 with the seed-bootstrap
  95% CI (B = 20 000) excluding zero.
- **P1.2 accuracy — REPRODUCES in sign; registered as the half most likely to move.** The
  corrected environment removes most tier-2 reach at `boxHalf=1000` (2.4 GHz r80 1149.5 →
  156.0 m, `report/K1_DOMINATION_RESULTS.md`), and the accuracy gain came from refusing
  escalations whose fixes were poor.
- **Falsifiers.** For each half separately: CI includes zero → that half **WITHDRAWN**;
  CI entirely above zero → **SIGN FLIPS**, withdrawn, and a corrections-register entry.

### C2 — PCS_FORMS: the D-1 repair, and the monotone control

| | |
|---|---|
| Original | `report/PCS_FORMS_RESULTS.md` from `results/pcs_forms` (pre-topology, split environment; the 20 legacy sidecars cite `9723dfc`, the other 60 `1e8e527`). `gated` vs `legacy` on `ca_tbl_no_integrity`: Δcep50 **−3.799 m**, ΔtotalJ +28.2 J, 0+/20−. Monotone control: **every form loses in every cell**, whole 95% CI below zero (e.g. legacy/`no_integrity` −0.2832, gated/`no_integrity` −0.0186) |
| Invocation | `scripts/sweeps/run_pcs_forms.sh` and the `results/pcs_forms/*/seed_*` sidecars, verbatim: `--seed=S --duration=3600 --boxHalf=2000 --nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=F --arms=static_2p4,ca_tbl_full,ca_tbl_no_integrity,fixed_threshold,static_868,static_uwb --trace=true` |
| Seeds × forms | 1–20 × {legacy, v2, weighted, gated}; legacy first, as originally |
| Output | `results/regen27_pcs_forms/<form>/seed_S` — **80 cells** |
| Complete when | 7 lines (header + 6 arms) **and** exactly 6 `trace_*.csv` files per cell, plus §6 |
| Analysis | `scratch/python_handoff/analyse_pcs_forms.py` with `ROOT = "results/regen27_pcs_forms"`, nothing else changed |

- The original's §7 abort check (legacy reproduces `baseline_v7`) **cannot apply**: K.2
  changed the environment default deliberately, so `baseline_v7` cannot reproduce under
  this binary. It is replaced by §2's provenance check. Recorded, not silently dropped.
- **P2.1 repair — REPRODUCES.** `gated` beats `legacy` on `cep50M` in `ca_tbl_no_integrity`:
  mean paired Δcep50M < 0, a majority of seeds negative, two-sided sign test p < 0.05 (the
  analyser's `sign(cep)` column).
- **P2.2 monotone control — REPRODUCES the negative.** Advantage |ρ_PCS| − |ρ_control| has
  its whole 95% CI below zero in all eight form × arm cells.
- **P2.2 reading rule, registered because the analyser's metric is sign-blind.**
  `analyse_pcs_forms.py:178` scores the advantage as |ρ_PCS| − |ρ_control|. That is right
  only while both correlations are negative. If ρ_PCS turns **positive** — confidence
  rising with error, which `ROOTCAUSE_ENVELOPE` Q2 found in the sparse regime — a large
  |ρ_PCS| scores as an advantage and the script prints `PCS BEATS CONTROL` for a score that
  is anti-informative. So a cell counts as PCS beating its control only if the advantage CI
  is entirely above zero **and** the printed mean `rho_pcs` is negative. A cell whose mean
  `rho_pcs` is positive is reported as **WRONG SIGN**, which supports the negative rather
  than withdrawing it. This is a rule for reading the analyser's printed columns; the
  analyser is not edited.
- **P2.3 — REPRODUCES.** `gated` costs less energy than `weighted` on
  `ca_tbl_no_integrity`. Computed directly from the `arm_summary.csv` rows, not by the
  analyser, which prints only mean differences against `legacy`: per-seed
  `totalJ(gated) − totalJ(weighted)`, 20 pairs, two-sided sign test p < 0.05 with a
  majority negative.
- **Falsifiers.** P2.1 not significant → the repair result is **WITHDRAWN**. Any P2.2 cell
  that beats its control under the reading rule → the negative is withdrawn for that cell
  and PCS is reported as beating its control there. `ROOTCAUSE_ENVELOPE` Q2 makes this a
  live possibility rather than a formality. P2.3 not significant → "`gated` buys the same
  accuracy for a fifth of the energy" is **WITHDRAWN**; significant with a majority
  positive → **SIGN FLIPS**.

### C3 — K10: the architecture beats published estimators (moving subject)

| | |
|---|---|
| Original | `report/K10_BASELINE_RESULTS.md`, `results/k10_*_move_s{7,12,17,22}` at `5d20de9`. cep95 vs MLAT: WAN-only 6 665 vs 13 762 (19+/1−), sparse 6 759 vs 38 599 (18+/2−), WAN+mesh **2 016 vs 20 831 (20+/0−)**, p ≤ 0.0004. vs `static_868`: indistinguishable, p 0.82–1.00 |
| Common flags | `--duration=14400 --seedCount=5 --trace=false --arbitrateDutyBudget=true --sendHandshakeFrames=true --terrainPerTierFreq=true --uwbCoverageIncludesFinders=true --nlosSource=propagation --envFacility=1 --vegFacility=3 --pcsForm=gated --floorOnDemand=true --escalationRequiresEvidence=true --corroborationCheck=true --bidirectionalDigest=true --mahalUseR=true --escalationRequiresEvidenceT2=true --boxHalf=6000 --baseCampFrac=0.9 --subjectKeepOutM=3000` |
| `wanonly_move` | + `--gwGrid=3 --nFinders=0` |
| `sparse_move` | + `--gwGrid=1 --nFinders=4 --finderMode=1 --finderDispersal=1.0 --finderPseudoAnchor=true` (gwGrid=1 is clamped to 2, i.e. 4 gateways — K9 §5) |
| `wanmesh_move` | + `--gwGrid=3 --nFinders=4 --finderMode=1 --finderDispersal=1.0 --finderPseudoAnchor=true` |
| Seeds | 7–26 as `--seedStart` 7, 12, 17, 22, appended last, as the sidecars record |
| Output | `results/regen27_k10/<cell>_s{7,12,17,22}` — **12 cells** |
| Complete when | 31 lines per chunk (5 seeds × 6 arms + header); 120 rows per concatenated cell |
| Analysis | paired two-sided sign test per seed on `cep95M` of `ca_tbl_full` against `cep95_mlat`, `cep95_wcl`, `cep95_prox`, `cep95_lkp` of the same row, and against `static_868` `cep95M` at the same seed |

- The K9 flags are **identical** to these three cells (`results/k9_*_move_s*` sidecars,
  `032290a`; K9's `meshonly` is K10's `sparse`), so C3 also carries C5, and
  `wanmesh_move` is C7's control. No separate K9 runs.
- **Still cells are not regenerated.** K10 §2 was corrected by K13 (`--speed` froze the
  search party). `--finderSpeed` still defaults to following `--speed`, so the K10 still
  invocation would re-measure the frozen party. Not in the claim list; stated here so the
  omission is visible.
- **P3.1 — REPRODUCES.** CA-TBL beats MLAT on `cep95M` in all three moving cells, p < 0.05.
- **P3.2 — REPRODUCES the qualifier.** CA-TBL is not significantly different from
  `static_868` in any moving cell.
- **Byte-identity expectation E3 (control flow, admissible under §1).** `wanmesh_move` has
  the same invocation as `results/L1_imuoff_s*` (`808885a`, which contains `4779bf7`).
  Registered: `regen27_k10/wanmesh_move_s{7,12,17,22}/arm_summary.csv` byte-identical to
  `results/L1_imuoff_s{7,12,17,22}/arm_summary.csv`. **Already known for seed 7** from the
  timing probe (§9), so only seeds 8–26 are a test. A difference is not an abort; it names
  a code path on this invocation that moved after `808885a`, and is recorded.
  `wanonly_move` and `sparse_move` have no post-`4779bf7` original and carry no such
  expectation.
- **Falsifier.** P3.1 fails in `wanmesh_move` → the headline "beats published estimators"
  is **WITHDRAWN** for the deployment it was quoted on. Fails elsewhere only → restated
  per cell.

### C4 — K1: does tiering dominate a single band?

| | |
|---|---|
| Original | `report/K1_DOMINATION_RESULTS.md`, `results/k1{a,b,c}_*` at `5bbac5d`. `ca_tbl_full` DOMINATED by `static_868` at box 1000–3000 and in `k1c_brief`, incomparable at 750; `static_2p4` dominates `ca_tbl_full` in every cell |
| Criterion | `report/PREREG_K1_DOMINATION.md` §1: no worse on both `rmseM` and `totalJ`, strictly better on one, two-sided sign test on paired per-seed differences, α = 0.05 |
| Common flags | `--duration=3600 --seedCount=20 --trace=false --arbitrateDutyBudget=true --nFinders=4 --finderMode=1 --sendHandshakeFrames=true --terrainPerTierFreq=true --uwbCoverageIncludesFinders=true --nlosSource=propagation`, then the cell's flags below, in the sidecars' order |
| `k1a_box{750,1000,1500,2000,3000}` | + `--gwGrid=3 --boxHalf=B --envFacility=1 --vegFacility=3 --pcsForm=gated --floorOnDemand=true --escalationRequiresEvidence=true --corroborationCheck=true --bidirectionalDigest=true` |
| `k1b_box{750,1000,1500,2000,3000}` | + `--gwGrid=3 --boxHalf=B --envFacility=1 --vegFacility=3 --pcsForm=legacy --floorOnDemand=false` |
| `k1c_brief` | + `--nGateways=3 --gwRingFrac=0.6 --boxHalf=2500 --envFacility=1 --vegFacility=3 --pcsForm=gated --floorOnDemand=true --escalationRequiresEvidence=true --corroborationCheck=true --bidirectionalDigest=true` |
| Seeds | 7–26, one process per cell (`--seedCount=20`), as the sidecars record |
| Output | `results/regen27_k1/<cell>` — **11 cells** |
| Complete when | 121 lines per cell (header + 20 seeds × 6 arms), plus §6 |
| Analysis | `scratch/python_handoff/k1_analyse.py results/regen27_k1/*` — it takes cell directories as arguments; unchanged |

- **P4.1 — REPRODUCES the negative.** `ca_tbl_full` dominates `static_868` in no cell.
- **P4.2 — REPRODUCES.** `static_2p4` dominates `ca_tbl_full` in every cell.
- **Falsifier.** `ca_tbl_full` dominates `static_868` in any cell → "tiering not
  supported" is revised for that cell and reported as an envelope. Per K1's own F1, the
  sweep is **not** widened in search of a kinder cell.

### C5 — AVAILABILITY: the mesh carries availability (no new runs)

| | |
|---|---|
| Original | `report/K9_SAR_FACTORIAL_RESULTS.md` at `032290a`: `ca_tbl_full` availability **4.1%** (WAN only) → 26.4% (sparse + mesh) → **70.4%** (WAN + mesh) |
| Data | C3's three moving cells, each concatenated per §3 |
| Metric | `fixCycles / steps` on `ca_tbl_full`, per seed; mean over 20 seeds; paired-by-seed two-sided sign test `wanmesh_move` vs `wanonly_move` |

- **P5.1 — REPRODUCES.** Ordering WAN only < sparse < WAN + mesh on the mean, and
  `wanmesh_move` above `wanonly_move` with p < 0.05.
- **Falsifier.** Not significant → "the mesh carries the availability" is **WITHDRAWN**.

### C6 — EDGE: estimation at the gateway saves node energy at no accuracy cost

| | |
|---|---|
| Original | commit `4779bf7` message, **seed 7 only**, default configuration: node-side 25.73 / 30.61 / 31.56 J → edge 14.88 / 20.72 / 21.57 J on `static_868` / `fixed_threshold` / `ca_tbl_full` (42% / 32% / 32%); accuracy byte-identical. No sidecar is tracked |
| Invocation | `--duration=3600 --seedCount=20 --trace=false --tier1ChargeReplies=true`, and for `edge` add `--gatewayEdgeServer=true`; every other flag at its default, as the original |
| Why both carry `--tier1ChargeReplies=true` | It has been the default since `4779bf7` itself (§2), so the original's node-side run was the default configuration. Passing it to both makes the pair differ by the treatment flag alone, and keeps the cells meaning the same thing if the default moves. It is inert under `edge` by construction: the reply charge is `m_tier1ChargeReplies && !m_edgeServer` (`ca-tbl-energy-model.cc:408`) |
| Seeds | 7–26, one process per setting — an upgrade from the original single seed (rule 8) |
| Output | `results/regen27_edge/{nodeside,edge}` — **2 cells** |
| Complete when | 121 lines per cell (header + 20 seeds × 6 default arms), plus §6 |

- **P6.1 — REPRODUCES, as a control-flow claim.** Every accuracy column (`rmseM`, `cep50M`,
  `cep95M`, `maxErrM`, `rmseFixedM`, `cep50FixedM`, `fixCycles`) is byte-identical between
  `nodeside` and `edge` for every seed × arm. The claim is that the estimator does not
  change, only where it runs.
- **Reachability for P6.1 (rule 4).** The edge path runs only when
  `gatewayEdgeServer && d.hadFix` (`ca_tbl_causal_sim.cc:5436`), so P6.2's energy
  difference is the evidence that the path executed; an identical accuracy column is then
  a control-flow result, not an unexercised one. On any arm where P6.2 shows no energy
  difference, P6.1 is not claimed.
- **P6.2 — REPRODUCES.** `totalJ` lower under `edge` on `static_868`, `fixed_threshold` and
  `ca_tbl_full` in 20/20 seeds. The fraction is reported; no magnitude is predicted.
- **Falsifiers.** Any accuracy difference → "at no accuracy cost" is withdrawn as a
  structural claim and restated as a measured paired difference. P6.2 not significant on
  any of the three arms → the saving is **WITHDRAWN** for that arm.

### C7 — IMU CEILING: a perfect dead reckoner collapses the tail on low-availability arms

| | |
|---|---|
| Original | commit `4302918` message and `results/L2_imuon_*` against `results/L1_imuoff_*`: `static_uwb` cep95 8 413 → ~17 m, `static_2p4` 27 151 → ~537 m |
| Invocation | the `results/L2_imuon_s*` sidecars, verbatim: C3 `wanmesh_move` flags, `--seedStart=S`, then `--imuFusion=true --imuIdeal=true` |
| Control | C3 `wanmesh_move` — identical invocation without the two flags, same binary, same seeds |
| Output | `results/regen27_imu/on_s{7,12,17,22}` — **4 cells** |
| Complete when | 31 lines per chunk; 120 rows per concatenated cell |

- **This is a CEILING, never a system result** (`TECH_DEBT` K.9, owner decision
  2026-09-08). Every figure is an upper bound on what a flawless IMU could contribute.
- **P7.1 — REPRODUCES.** `cep95M` falls under the ceiling on `static_uwb` and `static_2p4`,
  paired sign test p < 0.05 on each.
- `ca_tbl_full`'s direction is reported whatever it is; no prediction.
- **No byte-identity expectation.** Disclosed in §9: the probe's seed-7 rows on this
  invocation are **not** byte-identical to `results/L2_imuon_s7`, although C3's
  `wanmesh_move` reproduces `L1_imuoff`. Some code path on the IMU-fusion invocation moved
  after `4302918`; which one is not established here. That is why C7 is regenerated rather
  than reused; it is recorded, not investigated, by this registration.
- **Falsifier.** Not significant on both low-availability arms → **WITHDRAWN**.

## 5. Cell count and order

| order | campaign | processes | shape |
|---|---|---|---|
| 1 | C1 INTEGRITY | 16 | 8 seeds × 2 settings; 1 arm, 600 s |
| 2 | C2 PCS_FORMS | 80 | 20 seeds × 4 forms; 6 arms, 3600 s, traced |
| 3 | C3 K10 (+C5, +C7 control) | 12 | 3 cells × 4 chunks of 5 seeds; 6 arms, 14 400 s |
| 4 | C4 K1 | 11 | 11 cells × 1 process of 20 seeds; 6 arms, 3600 s |
| 5 | C5 AVAILABILITY | 0 | reads C3 |
| 6 | C6 EDGE | 2 | 2 settings × 1 process of 20 seeds; 6 arms, 3600 s |
| 7 | C7 IMU CEILING | 4 | 4 chunks of 5 seeds; 6 arms, 14 400 s |
| | **total** | **125 processes** | |

The driver dispatches cells in this order into a pool of at most **six** concurrent
simulations — a hard cap that `B3_MAXJ` may lower and cannot raise — because the machine
has 12 cores and 7 GB and another workflow builds and runs in the main tree concurrently.
A later campaign's cells start while an earlier campaign's last cells are still running;
the order is the dispatch order, and every cell is independent of every other.

## 6. Completeness rule (rule 12)

A cell is complete only when **all** of these hold, and the driver skips a cell only then:

1. `arm_summary.csv` counts exactly the registered number of lines (2, 7, 31 or 121) —
   not "non-empty". With `--seedCount` the file is appended per seed, so it is non-empty
   from seed one.
2. `run_id.txt` carries its `digest sha256` block with a line for `arm_summary.csv`, and
   every line of that block verifies with `sha256sum -c` in the cell directory. The block
   is written after the last stream closes (`AppendOutputDigest`,
   `ca_tbl_causal_sim.cc:6523-6569`, called at `:8501`), so a verifying block marks a
   finished process whose files are the ones it wrote.
3. Exactly the registered number of `trace_*.csv` files: 6 for C2, **0** for every other
   campaign.
4. The sidecar's invocation line ends with
   `./build/scratch/ns3.47-ca_tbl_causal_sim-optimized <registered arguments> --outdir=<cell>`,
   and its source line reads `source <prefix of this commit> dirty 0 profile optimized`.
5. The process exited 0.

A cell whose process exits non-zero, or which fails 1–3 when it ends, is logged as FAIL
with its exit status and moved to `~/catbl_b3_logs/incomplete/` — outside the worktree, so
its CSV can never be staged — and is never analysed. A completed cell that fails 4 is left
in place and triggers A1.

The driver logs `START`, and `END` with exit status, seconds and verdict, for every cell,
and after every `END` one completeness line for that cell's campaign — `n/N cells complete`
and data rows in complete cells against the registered total — to
`~/catbl_b3_logs/B3_PROGRESS.log`, outside the worktree. The same lines, recomputed with
full digest verification, close every pass; `bash scripts/b3_driver.sh --status` prints
them on demand.

## 7. Abort conditions

- **A1 — provenance.** At start: HEAD must be the commit that last touched this file and
  the driver; `git status --porcelain --untracked-files=no` must be empty for the whole
  worktree; `git diff 084a7ff HEAD -- src scratch` must be empty; git must track nothing
  under `results/regen27_*`. Before every launch: HEAD unchanged. At every cell's end, and
  when a pass scans existing cells: condition 4 of §6. Any failure stops all further
  launches; cells already running finish; completed cells are kept and are not citable
  until the cause is written down.
- **A2 — binary moved.** The driver records one SHA-256 over the binary and every
  `build/lib/libns3.47-*-optimized.so` at start and re-checks it before every launch. A
  change stops all further launches.
- **A3 — disk.** Less than 10 GB free stops launches. C2's traces are ~0.4 MB each (probe:
  381–405 KB at 3600 s), about 0.2 GB for the campaign; 210 GB were free at registration.
- **A4 — a cell fails** (§6). Recorded as FAIL and not retried in the same pass; the pass
  continues so every other cell is still observed (rule 11), and the driver exits non-zero
  at the end. Re-invoking the driver re-runs exactly the cells that are not complete.
- C1's NO POPULATION outcome is a **verdict**, not an abort.
- An interrupted driver is not an abort either. On Ctrl-C the cells in flight keep running
  (background jobs of a non-interactive shell ignore SIGINT) and hold the driver's lock
  until they exit, so a second driver cannot start over them; on a closed terminal they
  die and leave incomplete cells, which the next invocation moves aside and re-runs.

## 8. What would make this registration wrong

- If `src/` or `scratch/` differ between `084a7ff` and this commit. Checkable:
  `git diff --stat 084a7ff <this commit> -- src scratch` must be empty; the driver refuses
  to start otherwise.
- If a cell's directory predates this commit. Checkable against the commit time.
- If any analysis is run on a cell that failed §6.
- If the driver that ran is not the one committed here. Each pass's `DRIVER START` line
  records HEAD, and the driver refuses to start with any tracked modification.

## 9. Disclosures and exclusions

**Pre-registration exposure.** Before this file was committed, timing probes ran one
process from each campaign that runs (C1 `on`, C2 `gated`, C3 `wanmesh_move`, C4
`k1a_box750` and `k1b_box750`, C6 `nodeside`, C7) on this binary into `~/catbl_probe/b3/`
— untracked, outside `results/`, not citable — at seed 1 for C1 and C2 and seed 7
otherwise, plus two short seed-order probes for the dropped split design (§3). They were
used for wall time and row counts. Three outcome facts were seen, and are
disclosed rather than left unstated: the C1 `on_s1` log reports
`IMU inconsistency 203, weak corroboration 0`; the C3 `wanmesh_move` seed-7 rows are
byte-identical to those of `results/L1_imuoff_s7`; the C7 seed-7 rows are not
byte-identical to those of `results/L2_imuon_s7`. No probe magnitude is quoted anywhere
above, and E3 is registered as a test on seeds 8–26 only.

**Probe wall times**, seconds per process for one seed and all of the cell's arms, seven
probes running concurrently: C1 21, C2 167, C6 201, C4 (box 750) 345, C3 `wanmesh_move`
406, C7 418. That puts the campaign at roughly 130 000 process-seconds, about 6 hours of
wall time at six-way concurrency with the cores otherwise free.

**Not regenerated here**, listed so that the omissions are visible:

- The K10 still cells (see C3).
- The technical report's §7 observation, *82% more deployments for 27% more energy*
  (`results/p10_grid`, 320 cells). `TECH_DEBT` I.6 names it the figure most worth
  re-establishing, but it carries no verdict — the report presents it as a limitation of
  the registered metric — and `results/p10_grid` is absent from the tree, so there is no
  sidecar to copy an invocation from. It needs its own registration.
- The technical report's §4.3, *the dimensionless axis transfers* (`results/p10_env_v7`,
  `p10_env_v8`): a method result, not in the §L.3 claim table. It needs its own
  registration.
- The §4.1 reliability table (`results/p10_reliability`): withdrawn on 2026-09-05; no
  claim rests on it.
- Hop-aware provenance: **no population** (`hopWeight = 0`, depth capped at 1,
  `TECH_DEBT` L.3). There is nothing to run until L.1.2 lands.
