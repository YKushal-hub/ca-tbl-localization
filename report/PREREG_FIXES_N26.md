# PREREG FIXES-N26 — the four N.26 fixes measured on the preset, and the rule that adopts them

**Registered before any cell runs** (rule 13). Written 2026-10-05. Engine: the build of
`999f676` (the four flags, `185d8d1`, with the policy inputs fixed at `999f676`; gate G89 at
`6ee6e9e`). This commit adds this file, `scripts/fixes_n26_run.sh` and
`scripts/fixes_n26_analyse.py`, and nothing else. Outputs go to
`results/fixes_n26_2026-10-05/`, which does not exist at registration.

**Why.** The owner asked for CA-TBL's measured shortcomings to be fixed (2026-10-05). They are
TECH_DEBT N.26, N.15 and N.25, with the numbers in `HANDOFF_2026_10_05.md` §2. Each fix is a
default-off flag (§3 there). This campaign measures them, singly and together, against the
preset and against the comparators. A registered rule then decides whether the preset takes
them.

## 1. The fixes

| tag | flags | addresses |
|---|---|---|
| D | `--loraDualBase=true` | one tier per cycle (both LoRa tiers ranged and fused, UWB ranges joined) |
| U | `--uwbCrossTierPoll=true` (40 m) | UWB never discovering the finder that is there |
| T | `--tdoaInnovationGate=true` | wrong three-gateway TDoA roots fused unchallenged (N.25) |
| M | `--mahalUseR=true --integrityChi2=<fitted>` | the integrity distance mis-specified (N.15, 2026-10-05) |
| ALL | D + U + T + M | the candidate |

## 2. Phases and cells (`fixes_n26_run.sh`; `--specProfile=true`, 1 800 s, traces on)

**Phase 1, calibration** (`cal`): CAL_main and CAL_end, `ca_tbl_full` with D, U, T and
`--mahalUseR=true`, seeds **21-40** (never evaluated). 40 cells.

**The fit** (`fixes_n26_analyse.py fit`):
`integrityChi2 = max(9.21, p99)`, where p99 is the 99th percentile of tag 0's integrity
distance over fix cycles the check judged (distance > 0) at t ≥ 120 s, pooled over both
cells.
- The 120 s warm-up is OURS. It is the EKF's acquisition from its 100 m prior. A 60 s dry run
  showed that acquisition alone can set the tail.
- The floor of 9.21 keeps the check at least as strict as chi-square.
- The fit is written to `fit.json` before phase 2 runs.

**Phase 2, evaluation** (`eval`), seeds 1-20 (fault matrix 1-10):

| cells | arms / flags | seeds |
|---|---|---|
| ALL_main, ALL_end, ALL_n8 | `ca_tbl_full`, ALL | 1-20 |
| D_, U_, T_, M_ × main/end | `ca_tbl_full`, that fix alone | 1-20 |
| E_{rb_common, rb_single, txdrop, antenna}_downweight | `ca_tbl_full`, ALL, CAMP-SPEC's faults | 1-10 |
| X_main, X_end | `ca_tbl_full,static_868,poucet_fusion` at A_main / B_end seed 1 | 1 |
| X_lf | `lora_fusion` at LORA-CMP-2's F_end seed 1 | 1 |

That is 263 cells. The main and endgame cells carry `--reportAirtime=true`, as A_main and B_end
did.

**Comparators**, paired by seed:
- CA-TBL preset, `fixed_threshold`, Poucet and RSSI + TDoA: CAMP-FINAL A_main, B_end, C_n8;
- RSSI-only: LORA-CMP R_*;
- dual-band LoRa fusion: LORA-CMP-2 F_*.

**The pairing crosses a binary change (rule 8).** The X cells must reproduce those campaigns'
`arm_summary.csv` and `subject_summary.csv` rows byte-for-byte. The two snapshot-diagnostic
columns are excluded, as LORA-CMP-2 §7 amended. The null control at `999f676`: nine NC runs,
`arm_summary.csv` identical in 9 of 9, traces at most 9.97e-13 relative in the TECH_DEBT 4.1
columns.

**Isolation.** A git worktree at this commit, holding a byte copy of `999f676`'s build. Its
SHA-256 is listed in RUN.md.

## 3. Metrics (`fixes_n26_analyse.py eval`)

- **Per configuration:** LORA-CMP's per-seed quantities (availability, tag 0 CEP50/95,
  subject CEP95, TTFF, subject J) for every row, plus UWB-delivered fixes (tag 0 and
  subjects, seeds with any) for the CA-TBL rows.
- **Paired differences:** each row minus the CA-TBL preset, and ALL minus dual-band LoRa
  fusion and minus Poucet.
- **The integrity check:**
  - the fault-free share of tag 0 fix cycles SUSPECT, for the preset, ALL and M;
  - CAMP-SPEC's fault-matrix table (precision, recall, CEP95 in and out of the window) for
    ALL.

## 4. The adoption rule (registered)

The preset (`kSpecProfile`) takes ALL — the four flags, `--mahalUseR=true` and the fitted
`--integrityChi2` — if **all three** hold in **both** A (5 km) and B (endgame):
1. **availability:** the paired gain over the CA-TBL preset has an interval lower bound above 0;
2. **accuracy:** tag 0's CEP95 mean per-seed change (ALL / preset − 1) is at most +0.10;
3. **energy:** ALL's mean subject J is not above the preset's.

The analysis prints the verdict. Adoption, if it holds, is its own commit, with G24 and the
register updated. If any term fails, nothing is adopted, and the single-fix rows say which fix
carries what.

## 5. Predictions, made before any cell

**Disclosed:**
- Single-seed smoke runs while building the flags (seed 7 endgame 900 s; seed 1 5 km). These
  are in the commit messages of `185d8d1` and in TECH_DEBT N.15.
- G89's runs (the same seeds).
- A 60 s dry run of this whole pipeline outside `results/`, from which the warm-up rule came.

None is data for this registration.

| # | prediction |
|---|---|
| **Q1** | the fitted `integrityChi2` lies between 9.21 and 60 |
| **Q2** | A and B: ALL's availability gain over the preset has its interval above 0 |
| **Q3** | B: ALL's mean availability is at least lora_fusion's 0.501 − 0.01 |
| **Q4** | B: ALL's mean subject J is below the preset's 24.91 J |
| **Q5** | B: ALL has UWB-delivered fixes on at least 10 of 20 seeds (the preset: 0 of 20) |
| **Q6** | A: ALL's tag 0 CEP95 mean per-seed change is at most +0.10 (TDoA gated) |
| **Q7** | fault-free, ALL's SUSPECT share is at most 0.05 in A and B (preset 0.775 / 0.969) |
| **Q8** | E: ALL's integrity precision exceeds CAMP-FINAL's under every fault (0.149 / 0.307 / 0.305 / 0.247 for antenna / rb_common / rb_single / txdrop, downweight) |
| **Q9** | the registered rule ADOPTs |

Each is written up as held or failed in `report/FIXES_N26_RESULTS.md`.

## 6. Abort conditions (the analysis's "Problems")

Any of the following stops the write-up. The cause is fixed, and no cell is dropped:
- not 263 evaluation cells, or a cell exits non-zero;
- a `run_id.txt` does not read one source with `dirty 0`;
- an X cell's compared rows differ;
- a fix cell lacks its N.26 line, or shows a dual base that did not range, or the wrong
  threshold;
- a row has fewer than 20 seeds.
