# PREREG CAMP-FINAL-2 — the finished preset, with FIXES-N26, measured against every comparator

**Registered before any cell runs** (rule 13). Written 2026-10-08. Engine source at
registration: the commit that puts FIXES-N26 in the preset (102 settings; owner override,
`docs/DESIGN_DECISIONS.md` §9), with row 17's literal-term flag below it (off in the preset).
This commit adds this file, `scripts/camp_final2_run.sh` (CAMP-FINAL's registered runner with
`lora_fusion` added as an eighth arm, nothing else changed) and
`scripts/camp_final2_analyse.py` (CAMP-FINAL's analysis applied unchanged, plus one paired
comparison table). The outputs go to `results/camp_final2_2026-10-08/`, which does not exist
at registration.

**Why.** CAMP-FINAL measured the preset before FIXES-N26 joined it. The preset now differs,
so its comparison table must be re-measured on the preset as it stands. That table puts
CA-TBL against fixed-threshold switching, Poucet-style fusion, dual-band LoRa fusion,
single-band operation and single-hop confidence. It is the table the assembly-level claim
(`CLAUDE.md` §9) rests on.

**Disclosed before registration.**
- **CA-TBL's own cells repeat runs already measured.** The new preset alone reproduces
  FIXES-N26's registered ALL_end seed 1 byte for byte: arm_summary.csv and 59 of 60 output
  hashes. So `ca_tbl_full` in A_main and B_end is expected to reproduce FIXES-N26's ALL_main
  and ALL_end for seeds 1-20, and the predictions on `ca_tbl_full` lean on those numbers.
- **What is new is the comparators measured beside it, under this preset.** The preset's
  `--tdoaInnovationGate` and `--mahalUseR` reach other arms too, so their numbers can move
  from CAMP-FINAL's and LORA-CMP-2's.

**How the run is isolated.** As CAMP-FINAL's: a git worktree at this commit, with a byte copy
of the binary and libraries built at the engine commit (`LD_LIBRARY_PATH` pins them), so
every cell reads `source <this commit> dirty 0`. The copy's SHA-256 goes in RUN.md.

---

## 1. Cells (`camp_final2_run.sh`; `--specProfile=true`, 1 800 s, traces on)

CAMP-FINAL's cells, unchanged (`report/PREREG_CAMP_FINAL.md` §1), with `lora_fusion` added
to A_main and B_end:

| cell | what | arms | seeds |
|---|---|---|---|
| A_main | the preset at 5 km, `--reportAirtime` | the six, `poucet_fusion`, `lora_fusion` | 1-20 |
| B_end | the endgame (`--boxHalf=750 --finderMode=1`), `--reportAirtime` | the eight | 1-20 |
| C_n8 | N8_ring | `ca_tbl_full`, `static_868` | 1-20 |
| G_single, G_single_end | the single-hop baseline, 5 km and endgame | `ca_tbl_full` | 1-20 |
| H_flood | `--reversePath=false`, `--reportAirtime` | `ca_tbl_full` | 1-20 |
| E_<fault>_<recovery> | CAMP-SPEC's fault matrix | `ca_tbl_full`, `ca_tbl_no_integrity` | 1-10 |
| T_toa | `--toaTable` | `ca_tbl_full` | 1 |

## 2. Metrics

CAMP-FINAL's (§2 there), unchanged. Added, in PAIRED.md: `ca_tbl_full` minus each
comparator, paired by seed, mean with a 95 % seed bootstrap, on availability (the five
subjects), tag 0 CEP95 and subject energy, in A_main and B_end. The comparators are
`poucet_fusion`, `lora_fusion`, `fixed_threshold`, `static_868`, `static_2p4`, `static_uwb`
and `ca_tbl_no_integrity`.

## 3. Predictions, made before any cell

- **Q1.** `ca_tbl_full` in A_main and B_end reproduces FIXES-N26's ALL_main and ALL_end:
  availability 0.285 and 0.562 (± 0.001).
- **Q2.** Against `poucet_fusion` and against `lora_fusion`, `ca_tbl_full`'s availability
  difference has an interval above 0 in both A and B.
- **Q3.** Against `poucet_fusion`, `ca_tbl_full`'s subject energy is lower in both A and B,
  with intervals below 0.
- **Q4.** Against `static_868`, `ca_tbl_full`'s availability difference has an interval above
  0 in both A and B.
- **Q5.** Against `fixed_threshold`, `ca_tbl_full`'s availability difference has an interval
  above 0 in both A and B.
- **Q6.** In B, `ca_tbl_full` has UWB-delivered fixes on at least 15 of 20 seeds.
- **Q7.** Integrity precision under every fault stays below 0.5 for `ca_tbl_full`. At the
  preset's 1 992.9 the check is near silent (FIXES-N26 §3), and nothing here changes that.

Each is written up as held or failed in `report/CAMP_FINAL2_RESULTS.md`.

## 4. Abort conditions

As CAMP-FINAL's. Also: if a `ca_tbl_full` cell of A_main or B_end does not reproduce its
FIXES-N26 counterpart's arm_summary.csv row, stop and report it. A result that does not
reproduce is not a result.

## 5. What follows

Nothing is adopted. The tables replace CAMP-FINAL's preset numbers wherever a report cites
them, and `scripts/paper_tables.py` is re-run on them.
