# PREREG C1_NOGATE — the witness cross-check with the evidence gate off

**Registered 2026-09-29, before the first cell runs.** Rule 13. This file, its driver
`scripts/b3/c1_nogate_driver.sh` and its analyser `scripts/b3/c1_nogate_analyse.py` are
committed together in one commit. Every output directory named below is created by that
driver after the commit, and the driver refuses to start unless HEAD is that commit, so the
directories' timestamps are checkable against it.

Branch `work/2026-09-27-b3-regen`, worktree `~/catbl_b3`. This commit touches `report/` and
`scripts/` only; `src/` and `scratch/` are byte-identical to `084a7ff`, the commit the binary
was built from.

---

## 0. Why

`report/B3_REGEN_RESULTS.md` §C1 withdrew the integrity claim (C1) with **NO POPULATION**. With
`--imuCrossCheck=true --corroborationCheck=true` the witness block
(`ca-tbl-policy.cc:1233-1269`) refused **1 713** escalations over 8 seeds (IMU 203–232 per
seed, corroboration 0), and still the `off` and `on` rows were **byte-identical on 8/8 seeds**.

The cause the report gives is **inferred from code, not measured**: `--escalationRequiresEvidence`
became default-true at `7ec0d91` (2026-09-08 07:20), after the original C1 binary was built;
the witness block runs before the evidence gate (`:1239` against `:1271`); the node sat at
2.4 GHz with `cycles_uwb = 0` and `cyclesInUwbCoverage = 0` on every seed, so every refused
escalation was 2.4 GHz → UWB, which the evidence gate refuses anyway. The report says that
C1 with the gate off *"needs its own registration pinning `--escalationRequiresEvidence=false`"*.
The owner asked for that run. This is it.

**Three suppress-only mechanisms are involved and are kept distinct** (`CLAUDE.md` §2):

| name here | what it is | where | state in this campaign |
|---|---|---|---|
| **witness cross-check** (the treatment) | `--imuCrossCheck` + `--corroborationCheck` | `ca-tbl-policy.cc:1233-1269` | off in `off`, on in `on` |
| **evidence gate** | `--escalationRequiresEvidence`, `Architecture.md` §9.3 | `ca-tbl-policy.cc:1271-1358`; flag `ca_tbl_causal_sim.cc:7068-7081`, default **true** at `:6693` | **off in both** (pinned false) |
| **Switching-Point Integrity Check** | Mahalanobis vs χ²₀.₉₉ = 9.21 | `ca-tbl-policy.cc` | default, on in both (`suspects` column) |

What this is **not**:

- **Not a correction of B3's C1.** B3's verdict stands as recorded: the witness check adds
  nothing in the shipped default configuration, where the evidence gate is on. This campaign
  asks the separate question the original C1 answered, in the configuration it ran in.
- **Not the shipped default.** The default is gate-on. A REPRODUCES verdict here is about a
  non-default configuration and must be written as such.
- **Not a code change.** Same binary as B3 (§2).

## 1. Seeds, the original, and the sampler

The original claim (`report/INTEGRITY_CHECK_RESULTS.md`) used **8 seeds, 1–8**: energy
−15.360 J [−21.433, −9.233], 8/8; rmse −0.743 m [−1.052, −0.416], 7/8. Those numbers are
quoted **only to name the claim**. The original ran on a different binary, in the pre-K.2
environment, and before `4779bf7` changed tier-1 reply billing; `LoRaDistributionSampler`
stays unseeded (`CLAUDE.md` §5). **No magnitude here is compared with the original's.**

This campaign runs **seeds 1–20**. Seeds 1–8 are additionally analysed on their own, with the
same rules, **for comparability of the VERDICT only** — did the verdict the original reached
on these seeds hold here — never as a magnitude comparison.

Every endpoint is a paired difference inside one binary, same seed, same invocation apart
from the treatment flags.

## 2. The binary and provenance

- Cwd `~/catbl_b3`, binary `./build/scratch/ns3.47-ca_tbl_causal_sim-optimized`, the `argv[0]`
  B3 used. Built from `084a7ff`, `--build-profile=optimized`, g++-11.
- Checked 2026-09-29 before this commit: `git diff 084a7ff HEAD -- src scratch` is empty,
  `git status --porcelain --untracked-files=no` is empty, and `./ns3 build ca_tbl_causal_sim`
  reports `ninja: no work to do`.
- **SHA-256 over the binary and every `build/lib/libns3.47-*-optimized.so`, first 16 hex:
  `9c6493f7a992a821`** — before and after that build, and equal to the digest the B3 driver
  logged at its start (`~/catbl_b3_logs/B3_PROGRESS.log`, line 2). So this is **the binary
  that produced `results/regen27_integrity/`**, and byte-identity comparisons against those
  cells are admissible as control-flow statements (§4 steps 5–6). The driver refuses to start
  on any other digest (A2).
- Every sidecar must read `source <prefix of this commit> dirty 0 profile optimized`, and its
  invocation line must end with the registered arguments and `--outdir=<cell>`. The dirty
  count is tracked modifications under `src/` and `scratch/` only
  (`ca_tbl_causal_sim.cc:7929-7936`).

## 3. The cells

| | |
|---|---|
| `off` | `--seed=S --duration=600 --boxHalf=1000 --nFinders=4 --finderMode=1 --sendHandshakeFrames=true --trace=false --arms=ca_tbl_full --escalationRequiresEvidence=false` |
| `on` | the `off` arguments, then `--imuCrossCheck=true --corroborationCheck=true` |
| Base | C1's registered invocation, verbatim (`report/PREREG_B3_REGEN.md` §C1); the only addition is `--escalationRequiresEvidence=false`, on **both** settings |
| Seeds | 1–20, one process per seed per setting |
| Output | `results/regen27_integrity_nogate/{off,on}_s{1..20}` — **40 cells**, all new directories; no tracked results directory is opened for writing (rule 7) |
| Run | `cd ~/catbl_b3 && bash scripts/b3/c1_nogate_driver.sh FIRST LAST`, at most **4** concurrent simulations; each cell's stdout to `run.log` beside it |
| Tracked | `arm_summary.csv` and `run_id.txt` only. `run.log` and `trace_*.csv` are ignored (`.gitignore:75-80`); the driver re-checks this on real files at start |

Every other flag takes its default in this binary: wide-area environment, tier-1 replies
billed (`--tier1ChargeReplies` default true since `4779bf7`), the Mahalanobis check on,
`--escalationRequiresEvidenceT2` off.

**Wall time.** B3's C1 timing probe took 21 s per process at 7-way concurrency. 40 processes at
4-way is about 10 waves; the driver is run in two passes (seeds 1–10, then 11–20) so each pass
fits one foreground call. The window selects which incomplete cells a pass launches; it never
changes a cell.

## 4. Analysis, in this order

`python3 scripts/b3/c1_nogate_analyse.py results/regen27_integrity_nogate`, committed here.
It is the only analysis that produces a verdict.

**Step 0 — completeness (rule 12).** All 40 cells, each: `arm_summary.csv` exactly 2 lines
(header + the `ca_tbl_full` row of that seed), 40 data rows in all, 20 per setting;
`run_id.txt` digest block verifies and names `arm_summary.csv`; registered invocation and
source line (§2); 0 `trace_*.csv`; `rmseM > 0` (0.0 is the simulator's value for an **empty**
error vector, `ca_tbl_causal_sim.cc:6052`, not a measurement); `run.log` holds **exactly one**
`INTEGRITY:` line in every `on` cell and **none** in every `off` cell (the line prints only when
a witness flag is set, `:6350-6367`, so this checks that the treatment was what was
registered). Any failure: the analyser refuses to pool and exits 2. **No partial-n analysis is
reported as a result.**

**Step 1 — population (rule 4), before any endpoint.** From each `on` `run.log`:
`INTEGRITY: escalations refused -- IMU inconsistency N, weak corroboration M`. Denominator: one
600 s run of 240 cycles per seed; a refusal is one cycle's event, so none straddles the window.
And, per seed, whether the `off` and `on` data rows are byte-identical.

- **NO POPULATION** if N + M = 0 on all 20 seeds, **or** the rows are byte-identical on all 20
  seeds. That is a **verdict**, not an abort: C1 (gate off) is withdrawn as unsupported and the
  endpoints are printed for the record with no verdict attached.
- **What NO POPULATION by byte identity would imply.** With the evidence gate off, the thing B3
  inferred was absorbing every refusal is gone. If the witness still refuses hundreds of
  escalations and no output moves, **B3's inferred cause is falsified**: something other than
  the evidence gate neutralises the refusals — a later step that re-forms or overrides
  `wantEscalate`, or escalations that change nothing downstream — and the B3 report's §C1
  explanation takes a corrections-register entry. The witness check then has no causal path to
  any output under this invocation in either configuration, "earns its place" cannot be claimed
  anywhere it has been tested, and the next step is a code-path trace of what follows a refusal,
  not another campaign.
- Differing rows alone do not establish a population (the converse does not hold); the refusal
  counts are what do.
- Seeds with byte-identical rows, when not all are, enter step 2 as **exact ties**: excluded
  from the sign test's n and counted on the line, included in the bootstrap mean.

**Step 2 — primary endpoints.** Per seed, **d = on − off** for
- `totalJ` — the node's energy on all three radios, sleep included, over the 600 s run;
- `rmseM` — as the simulator computes it over its per-cycle error vector (`:6052`).

Negative d favours the witness check. For each: mean, median, sd, range; **exact two-sided
sign test** (binomial, p = ½, ties excluded from n and reported); **percentile seed-bootstrap
95 % CI** of the mean, resampling seeds, B = 20 000, `random.Random(20260929)`.

**Step 3 — verdict rules.** For each endpoint:

- **FAVOURS-ON**: CI entirely below 0 **and** sign test p < 0.05 with a negative majority.
- **FAVOURS-OFF**: CI entirely above 0 **or** sign test p < 0.05 with a positive majority.
- otherwise **NO-DETECTED-DIFFERENCE**.

Asymmetric on purpose: the claim must pass both tests; its falsification needs either.

| | FAVOURS-ON | NO-DETECTED-DIFFERENCE | FAVOURS-OFF |
|---|---|---|---|
| **P1.1 energy** | **REPRODUCES** | NOT SUPPORTED (stays withdrawn) | **SIGN FLIPS** — withdrawn, corrections-register entry |
| **P1.2 accuracy** | IMPROVES (the original's own form reproduces) | NOT SHOWN WORSE — *not* an equivalence result; no non-inferiority margin is registered | **WORSE** — falsifies "accuracy no worse" |

**Joint verdict, C1 with the gate off:** **REPRODUCES** iff P1.1 REPRODUCES and P1.2 is not
WORSE (**REPRODUCES IN FULL** if P1.2 IMPROVES); **FALSIFIED** if P1.1 SIGN FLIPS or P1.2 is
WORSE; otherwise **NOT SUPPORTED**.

**The binding verdict is the one on seeds 1–20.** The same rules are applied to seeds 1–8 and
reported for comparability of the verdict only; if they disagree, the 20-seed verdict binds and
the disagreement is reported, not reconciled. Power, stated so a reader does not have to work
it out: with 20 untied seeds a sign test reaches p < 0.05 only at 15/20 or better
(p = 0.041); with 8, **only at 8/8** (p = 0.0078) — 7/8 gives p = 0.070. The original's rmse
result (7/8) would therefore not have met this rule on its own sign test.

**Step 4 — secondary, descriptive, no verdict.** Per seed, off → on: `escalations`,
`deescalations`, `cycles_868`, `cycles_2p4`, `cycles_uwb`, `cyclesInUwbCoverage`, `fixCycles`,
`noFixCycles`, `suspects`, `j_uwb`; with mean differences and sign counts for `escalations`,
`cycles_uwb`, `fixCycles`, `j_uwb`. These say *how* any effect arises; they carry no verdict.

**Step 5 — manipulation check M0 (control flow, same binary).** The `off` cells here (gate off)
against `results/regen27_integrity/off_s1..8` (gate on, B3), seeds 1–8: rows byte-identical or
not, and `escalations`, `cycles_uwb` in each. If identical on 8/8, **the flag had no effect** on
the control arm, and whatever step 1 finds, B3's inferred cause is falsified: removing the gate
changed nothing, so the gate cannot have been what absorbed the refusals.

**Step 6 — supplementary S2 (control flow, same binary, no verdict).** The `on` cells here
against B3's `off`/`on` cells, seeds 1–8. If identical, the witness alone reproduces exactly what
the evidence gate did — both acting as a total veto on the 2.4 GHz → UWB step in this
configuration.

## 5. Predictions

What the original claim predicts, and what this registration expects:

- **Population: PRESENT.** Refusal counts > 0 on every seed and `off`/`on` rows differing on
  most seeds. This is the direct test of B3's inferred cause.
- **M0: the flag takes effect.** In B3 every `off` row has `escalations = 1` and
  `cycles_uwb = 0`. With the gate off the policy is expected to attempt 2.4 GHz → UWB, so the
  `off` rows here differ from B3's on most of seeds 1–8, with `escalations > 1`.
- **P1.1 energy: REPRODUCES** — `totalJ` lower under the witness on most seeds.
- **P1.2 accuracy: not WORSE**; the original's own form (IMPROVES) is registered as the half
  more likely to move.

**What a REPRODUCES verdict would and would not mean — registered now so it cannot be
over-read later.** In B3 the IMU witness refused on 203–232 of 240 cycles, because the
step-and-heading engine ended each run 315–724 m from truth against tolerances of 15–136 m.
The drift constants are ours and uncalibrated (`docs/TECH_DEBT.md` K.9). A witness that refuses
on almost every cycle is close to a blanket veto on escalation; and B3 recorded zero UWB
coverage cycles on every seed. So if energy falls, the likely mechanism is *vetoing
escalations into a tier with no coverage saves energy*, which is what K3 measured for the
evidence gate, and **not** *the witness detects an untrustworthy estimate*. This design does not
test the witness's discrimination, and a REPRODUCES verdict supports the energy and accuracy
statements only. No claim about integrity, trust or detection may be written from it.

## 6. Falsifiers

- **F1.** P1.1 SIGN FLIPS or P1.2 WORSE → C1 **FALSIFIED** with the gate off;
  corrections-register entry.
- **F2.** NO POPULATION → C1 withdrawn in this configuration too. If it is reached by byte
  identity while M0 shows the flag acted, B3's inferred cause is falsified (step 1).
- **F3.** M0 byte-identical on 8/8 → B3's inferred cause is falsified (step 5), independently
  of the verdict.
- P1.1 NOT SUPPORTED → the claim stays withdrawn. "The witness earns its place" is not
  restored by anything short of REPRODUCES.

## 7. Abort conditions

- **A1 — provenance.** At start: HEAD is the commit that last touched this file, the driver
  and the analyser; no tracked modification anywhere in the worktree; `src/` and `scratch/`
  equal `084a7ff`; git tracks nothing under `results/regen27_integrity_nogate`. Before every
  launch: HEAD unchanged. At every cell's end: the sidecar is this registration's. Any failure
  stops further launches; cells in flight finish and are not citable until the cause is
  written down.
- **A2 — binary.** Digest `9c6493f7a992a821` at start and before every launch; anything else
  stops launches.
- **A3 — disk.** Less than 10 GB free stops launches.
- **A4 — a cell fails** (non-zero exit, or step-0 conditions 1–3 unmet at its end): logged,
  moved to `~/catbl_b3_logs/c1_nogate/incomplete/` (outside the worktree), never analysed; the
  pass continues so every other cell is observed (rule 11); the next pass re-runs it. A cell
  that fails twice leaves the campaign **INCOMPLETE, no verdict**, with the cause recorded.
- **A5 — treatment not as registered.** An `INTEGRITY:` line in an `off` log, or other than
  exactly one in an `on` log, fails step 0; nothing is analysed or re-run until the cause is
  written down.
- NO POPULATION is a verdict, not an abort.

## 8. What would make this registration wrong

- A cell directory older than this commit (checkable: directory mtimes against the commit time).
- Any analysis run on a set that failed step 0.
- A driver or analyser other than the ones committed here (each driver pass logs HEAD to
  `~/catbl_b3_logs/c1_nogate/PROGRESS.log`, and the driver refuses to start on any tracked
  modification).

## 9. Disclosures

- **Pre-registration exposure.** No process with `--escalationRequiresEvidence=false` on this
  invocation has been run on this binary before this commit. What *is* known is B3's gate-on
  C1, in full: its refusal counts, its byte identity, its escalation and tier-cycle columns and
  its SHS end-of-run errors. The predictions and the §5 interpretation limit are informed by
  those, and say so.
- **The analyser is new.** `analyse_integrity_ci.py`, which B3's C1 used, hard-codes seeds 1–8
  and a one-sided sign test. The analyser registered here follows it (seed-paired percentile
  bootstrap, B = 20 000, completeness before pooling) and changes what this registration needs
  changed: 20 seeds, a two-sided exact sign test with ties reported, the population and
  manipulation steps, and the verdict rules above.
- **Driver and analyser were exercised before this commit against a stub simulator** in a
  throwaway sparse clone under `~/catbl_b3_logs/c1_nogate_test/` (outside the worktree; the
  stub writes synthetic rows, so nothing it produced is a measurement). Observed there: 40/40
  cells in two passes (seeds 1–10, 11–20), concurrency never above 4, a third pass launching
  nothing, and git offering only `arm_summary.csv` and `run_id.txt` for staging. Each of these
  was observed to fail as intended: a binary digest other than `9c6493f7a992a821` (A2 refusal);
  a tracked modification (A1 refusal); the analyser on 20 of 40 cells, an `INTEGRITY:` line
  added to an `off` log, one changed byte in a summary (digest), and `rmseM = 0.0` (each: refuse
  to pool, exit 2); and all 40 rows made byte-identical in pairs (NO POPULATION, no verdict).
