# PREREG CAMP-FINAL — the finished preset, measured for the spec's comparison table

**Registered before any cell runs** (rule 13). Written 2026-10-04. Engine source at
registration: `d49f99c`, with the preset as it stands there (97 settings). This
commit adds this file, `scripts/camp_final_run.sh` (the runner) and
`scripts/camp_final_analyse.py` (the analysis; CAMP-SPEC's tables are
`scripts/camp_spec_analyse.py`'s functions, imported unchanged), and nothing else. The
outputs go to `results/camp_final_2026-10-04/`, which does not exist at registration.

**Why.** The owner's calls of 2026-10-04 (docs/DESIGN_DECISIONS.md §9) put one final
registered campaign in place of re-registering the pre-N.21 preset campaigns (GEO-01 stage 2,
CAL-02, CAMP-SPEC, FUSE-01, FUSE-01b). Every preset number before this one was measured on a
preset that no longer exists. The spec's Phase 5 asks for one comparison table: this system
against fixed-threshold switching, Poucet-style fusion and single-hop confidence, on error
against hop count, PCS-error correlation and switch-decision accuracy under induced
degradation; and its other sections ask for the ToA table, the digest's and the reverse
path's airtime as numbers, and the session times.

**The preset at registration.** The spec's three gateways (N3_star); half-duplex per chip;
both colliding frames judged; requests over the mesh; no fuse (FUSE-02); CAL-03's PCS, weights (2.995, 0, 0.005).

**How the run is isolated.** As CAL-03's: a git worktree at this commit, with a byte copy of
the binary and libraries built at `d49f99c` (`LD_LIBRARY_PATH` pins them), so every
cell reads `source <this commit> dirty 0`. The copy's SHA-256 is in RUN.md.

**Disclosed before registration.** The analysis was exercised, without changing the
predictions below, on a throwaway grid outside results/ (seeds 1-2, 120 s, every cell). That
grid is not data for this registration. Single-seed observations made while building the
flags (G81-G87, N.23, N.24) are named where a prediction leans on them.

---

## 1. Cells (`camp_final_run.sh`; `--specProfile=true`, 1 800 s, traces on)

| cell | what | arms | seeds |
|---|---|---|---|
| A_main | the preset at 5 km, `--reportAirtime` | the six, and `poucet_fusion` | 1-20 |
| B_end | the endgame (`--boxHalf=750 --finderMode=1`), `--reportAirtime` | the seven | 1-20 |
| C_n8 | the sensitivity geometry, N8_ring (`--nGateways=8 --gwLayout=legacy`) | `ca_tbl_full`, `static_868` | 1-20 |
| G_single, G_single_end | the single-hop confidence baseline (CAMP-SPEC §3's flags), 5 km and endgame | `ca_tbl_full` | 1-20 |
| H_flood | the preset with `--reversePath=false`, `--reportAirtime` (responses flooded) | `ca_tbl_full` | 1-20 |
| E_<fault>_<recovery> | CAMP-SPEC's fault matrix (§2 there), unchanged | `ca_tbl_full`, `ca_tbl_no_integrity` | 1-10 |
| T_toa | `--toaTable` (30 s): the MAC §2 table | `ca_tbl_full` | 1 |

201 cells. Each arm resets its own random stream, so a cell's `ca_tbl_full` is the same run
whatever arms share the cell, and A_main's `ca_tbl_full` pairs with H_flood's by seed.

## 2. Metrics (`camp_final_analyse.py`)

CAMP-SPEC's (§4 there), unchanged, with `poucet_fusion` a seventh arm: availability, CEP50/95,
TTFF, tier occupancy, subject energy, PCS against the trace control, the base station's
session times; the fault matrix's integrity precision and recall and CEP95 in and out of the
window; the single-hop baseline by hop depth. Added:
- **C_n8 against A_main** on `ca_tbl_full`'s availability and CEP95.
- **The batch precision spread** (Transport §5), per seed, defined or UNDEFINED.
- **UWB-delivered fixes** of `ca_tbl_full`, tag 0 and the subjects.
- **The digest against full pushes**, per tier: saving = FULL_PUSH_CF − (DIGEST + ENTRY_PUSH)
  per seed, mean with a 95 % seed bootstrap.
- **The reverse path against flooding**: A_main's `ca_tbl_full` against H_flood's, paired by
  seed: RESPONSE airtime, and RESPONSE airtime per answer the base station received.
- **The ToA table** (T_toa).

`poucet_fusion`'s tier occupancy reads 868 MHz on every cycle: its policy tier never moves
(TECH_DEBT N.24). Its energy, not its occupancy, is what it is compared on.

## 3. Predictions, made before any cell

- **P1.** At 5 km (A) `ca_tbl_full` has no UWB-delivered fix on any seed, and the batch
  precision spread is UNDEFINED on every seed (FUSE-02's off cells: 1 UWB fix in 20 seeds).
- **P2.** PCS against the trace control is INDISTINGUISHABLE for `ca_tbl_full` in A and in
  B: CAL-03's score is almost the trace term alone, and CAL-03's held-out A at 5 km was
  INDISTINGUISHABLE.
- **P3.** Integrity precision under every fault is below 0.5 for `ca_tbl_full` (CAMP-SPEC:
  0.14-0.26).
- **P4.** In the endgame (B) `poucet_fusion`'s tag 0 CEP50 is below `ca_tbl_full`'s, and its
  subject energy is at least five times `ca_tbl_full`'s (G87's single run: 2.98 against
  3.62 m, 209.7 against 10.9 J for tag 0).
- **P5.** The single-hop baseline's median error at depth 0 at 5 km is within 10 % of the
  preset's depth-0 median in A.
- **P6.** At 5 km (A) on 868 MHz the digest exchange takes MORE airtime than the full-push
  counterfactual: the saving's interval lies below 0 (N.23, G86's single run).
- **P7.** At 5 km the reverse path spends less RESPONSE airtime per answer delivered than
  flooding: the interval of A − H lies below 0.
- **P8.** At 5 km `ca_tbl_full`'s availability is within 0.02 of `static_868`'s (FUSE-02's
  off cells: 0.166 against 0.165).

Each is written up as held or failed in `report/CAMP_FINAL_RESULTS.md`.

## 4. Abort conditions

- A cell exits non-zero: stop, fix the cause, never drop the cell.
- An A, B or H cell writes no `airtime_*` file: stop.
- `results/camp_final_2026-10-04/` exists before the runner starts: it refuses.

`--resume` re-runs only the cells without an `rc`, with the registered command line.

## 5. What follows

Nothing is adopted by this campaign: it measures the finished preset. Its tables replace
every earlier preset number in the reports that cite one, which keep their dated text.
