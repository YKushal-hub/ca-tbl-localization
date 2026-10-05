# CAMP-FINAL — results: the finished preset measured for the spec's comparison table

**Registered in `report/PREREG_CAMP_FINAL.md` (`481e1de`, engine `d49f99c`) before any cell
ran.** All 201 cells exit 0 and read `source 481e1de7 dirty 0`. They ran from a worktree at the
registration commit with a frozen copy of the binary. One container restart interrupted the
run at 195/201, and `--resume` re-ran only the six cells without an `rc` (`results/camp_final_2026-10-04/RUN.md`). The tables
are `results/camp_final_2026-10-04/CAMP_FINAL.md`, produced by the registered analysis applied
unchanged ("Problems: 0").

The preset is 97 settings:
- the spec's three gateways (N3_star);
- half-duplex per chip;
- both colliding frames judged;
- requests over the mesh;
- no fuse (FUSE-02);
- CAL-03's PCS.

These tables replace every earlier preset number (CAMP-SPEC, GEO-01 stage 2, CAL-02,
FUSE-01/01b). Those reports keep their dated text. No magnitude here is attributed to a
difference from them: the preset, the geometry and the binary all changed between the two.

**Summary.**
- **At 5 km**, every switching arm localises about a sixth of the subjects' cycles
  (availability 0.17-0.19). `ca_tbl_full` never forms a UWB-delivered fix, and the batch
  session never becomes precise.
- **In the endgame**, availability is 0.42 and tag 0 CEP95 is 18.4 m.
- **`fixed_threshold` matches or beats `ca_tbl_full`** on availability and CEP in both
  configurations, at lower subject energy. The availability intervals overlap in both.
- **`poucet_fusion` has the highest availability** in both configurations. It costs 37×
  (A) and 17× (B) `ca_tbl_full`'s subject energy.
- **The integrity check flags 77-97 % of fixes with no fault injected** (§4). Its precision
  under faults is therefore near the base rate.

---

## 1. The seven arms at 5 km (A) and in the endgame (B), seeds 1-20

| arm | A availability | A CEP50/95 tag 0 (m) | A subject J | B availability | B CEP50/95 tag 0 (m) | B subject J |
|---|---|---|---|---|---|---|
| `ca_tbl_full` | 0.174 [0.152, 0.197] | 70.9 / 209.6 | 12.37 | 0.423 [0.405, 0.441] | 8.4 / 18.4 | 24.91 |
| `ca_tbl_no_integrity` | 0.177 [0.154, 0.201] | 76.5 / 217.3 | 12.61 | 0.464 [0.451, 0.479] | 7.9 / 17.5 | 17.77 |
| `fixed_threshold` | 0.186 [0.163, 0.210] | 69.3 / 179.4 | 11.04 | 0.439 [0.425, 0.455] | 7.7 / 16.6 | 15.44 |
| `static_868` | 0.165 [0.143, 0.189] | 289.2 / 559.2 | 6.10 | 0.371 [0.356, 0.386] | 9.1 / 20.6 | 6.43 |
| `static_2p4` | 0.031 [0.022, 0.041] | 24.5 / 54.9 | 14.21 | 0.271 [0.261, 0.282] | 4.9 / 10.8 | 5.81 |
| `static_uwb` | 0.000 | (no fix) | 446.19 | 0.000 | (no fix) | 418.19 |
| `poucet_fusion` | 0.238 [0.206, 0.271] | 161.8 / 219.4 | 462.46 | 0.506 [0.482, 0.529] | 6.4 / 13.5 | 416.24 |

How the columns are computed:
- **Availability** is the mean over the five subjects, with a 95 % seed bootstrap.
- **CEP** is the mean over seeds of each seed's tag 0 CEP. `static_uwb`'s CEP belongs to an
  estimate that never fixed, so it is not reported.
- **`poucet_fusion`'s policy tier never moves** (TECH_DEBT N.24), so it is compared on
  energy, not on occupancy.

Tier occupancy for `ca_tbl_full` (868 / 2.4 GHz / UWB):
- **A:** 0.183 / 0.815 / 0.002.
- **B:** 0.080 / 0.857 / 0.062.

UWB-delivered fixes for `ca_tbl_full`, tag 0 and subjects: **0 in A and 0 in B**, on 0 of 20
seeds each. In B, UWB occupies 6.2 % of cycles and delivers no fix.

Base-station batch session:
- **A:** first coarse fix, median 162 s. It never becomes precise (median 1 800 s = never).
  The precision spread is UNDEFINED on 20 of 20 seeds.
- **B:** first coarse fix, median 34 s. It never becomes precise. The spread is defined on
  9 of 20 seeds, median 663 s.

**Sensitivity geometry (C_n8, N8_ring) against A_main, `ca_tbl_full`:**

| metric | N3_star (A_main) | N8_ring (C_n8) |
|---|---|---|
| availability | 0.174 | 0.281 |
| CEP95 (m) | 209.6 | 43.7 |

## 2. The spec's comparison axes

**Error against hop count (G; `ca_tbl_full`, tag 0, pooled fix cycles):**

| configuration | depth | fix cycles | median error (m) | ρ(PCS, error) |
|---|---|---|---|---|
| preset 5 km | 0 / 1 / 2 | 5 475 / 1 142 / 7 | 27.3 / 16.1 / 459.6 | −0.351 / −0.087 / 0.204 |
| preset endgame | 0 / 1 / 2 | 9 560 / 2 648 / 1 898 | 6.9 / 5.0 / 6.2 | −0.188 / −0.305 / −0.330 |
| single-hop 5 km | 0 | 5 521 | 24.0 | −0.192 |
| single-hop endgame | 0 | 13 909 | 6.7 | −0.323 |

**PCS against the trace control.** A = ρ_control − ρ_PCS per seed, so > 0 means the PCS
ranks error better.

| cell | arm | A, mean [95 %] | seeds | verdict |
|---|---|---|---|---|
| A | `ca_tbl_full` | −0.0013 [−0.0115, 0.0068] | 19 | INDISTINGUISHABLE |
| A | `ca_tbl_no_integrity` | 0.0017 [−0.0008, 0.0058] | 19 | INDISTINGUISHABLE |
| B | `ca_tbl_full` | 0.0283 [0.0183, 0.0390] | 20 | **BETTER** |
| B | `ca_tbl_no_integrity` | 0.0001 [−0.0000, 0.0002] | 20 | INDISTINGUISHABLE |
| C | `ca_tbl_full` | 0.0043 [−0.0024, 0.0109] | 20 | INDISTINGUISHABLE |

In A, one seed has fewer than 30 fix cycles and is excluded by CAMP-SPEC's rule.

**Switch-decision accuracy under induced degradation (E, seeds 1-10, tag 0).**
- Integrity precision is 0.12-0.31 and recall is 0.56-0.91 for `ca_tbl_full`. Full table:
  `results/camp_final_2026-10-04/CAMP_FINAL.md` §E.
- Pooled CEP95 for `ca_tbl_full` is 359-538 m in the window and 573-665 m outside it.
- For `ca_tbl_no_integrity` it is 62-72 m in the window and 65-72 m outside it.

## 3. Airtime and the ToA table

**The digest against full pushes** (`ca_tbl_full`). Saving = full-push counterfactual −
(DIGEST + ENTRY_PUSH), so > 0 means the digest saves airtime.

| cell | tier | saving (s), mean [95 %] |
|---|---|---|
| A_main | 868 MHz | −30.01 [−35.66, −24.35] |
| A_main | 2.4 GHz | 3.58 [2.99, 4.13] |
| B_end | 868 MHz | −58.78 [−61.28, −56.11] |
| B_end | 2.4 GHz | 5.82 [5.29, 6.35] |

On 868 MHz the digest costs more airtime than pushing the whole table. On 2.4 GHz it saves
airtime (TECH_DEBT N.23).

**The reverse path against flooding** (5 km, A_main against H_flood, paired by seed):

| quantity | reverse path | flooding | A − H, mean [95 %] |
|---|---|---|---|
| RESPONSE airtime (s) | 73.41 | 58.51 | 14.91 [11.70, 18.13] |
| per answer delivered (s) | 3.044 | 2.444 | 0.600 [0.233, 0.969] |

In this configuration the reverse path costs more RESPONSE airtime than flooding, both in
total and per answer delivered. *Hypothesis, not measured:* per-hop unicast with ACKs and
retries costs more than one flooded broadcast when the reverse route is short.

**ToA table:** `results/camp_final_2026-10-04/T_toa/s1/toa_table.csv`, 180 rows (MAC §2).

## 4. Observation outside the registration: the integrity check's base rate

This was measured after the registered analysis. Counted from the trace's `suspect` column,
which is the policy's Mahalanobis > χ²₀.₉₉ result on eligible cycles. These are
`ca_tbl_full`'s tag 0 fix cycles with **no fault injected**:

| cell | SUSPECT / fix cycles | share | per seed |
|---|---|---|---|
| A_main | 5 131 / 6 624 | 0.775 | 0.21-0.93 |
| B_end | 13 666 / 14 106 | 0.969 | 0.95-0.98 |
| C_n8 | 10 233 / 10 974 | 0.932 | 0.86-0.96 |

The check flags most fixes whether or not a fault is present. So under faults its precision
(0.12-0.31) sits near the share of fixes inside the fault window, and its recall follows the
flag rate rather than the fault.

CAMP-SPEC recorded the same behaviour at eight gateways (TECH_DEBT N.15). It is recorded
there again with these counts and is not changed here: a different threshold or eligibility
rule is an engine change for its own registered campaign.

## 5. Predictions

| | registered | measured | verdict |
|---|---|---|---|
| **P1** | A: no UWB-delivered fix on any seed; spread UNDEFINED on every seed | 0 fixes on 0/20 seeds; UNDEFINED 20/20 | **held** |
| **P2** | PCS against the trace INDISTINGUISHABLE in A and in B | A INDISTINGUISHABLE; B **BETTER** (0.0283 [0.0183, 0.0390]) | **failed** (in B) |
| **P3** | integrity precision < 0.5 under every fault | max 0.307 (`rb_common`) | **held** |
| **P4** | B: `poucet_fusion` CEP50 < `ca_tbl_full`'s, subject energy ≥ 5× | 6.4 < 8.4 m; 416.24 / 24.91 = 16.7× | **held** |
| **P5** | single-hop depth-0 median within 10 % of the preset's at 5 km | 24.0459 against 27.3102 m: −11.95 % | **failed** |
| **P6** | A, 868 MHz: digest saving interval below 0 | −30.01 [−35.66, −24.35] | **held** |
| **P7** | reverse path RESPONSE airtime per answer below flooding's: A − H interval below 0 | +0.600 [0.233, 0.969] | **failed** (opposite sign) |
| **P8** | A: `ca_tbl_full` availability within 0.02 of `static_868`'s | 0.17396 − 0.16458 = 0.0094 | **held** |

Five held, three failed:
- **P2** failed in the system's favour: in the endgame the calibrated PCS ranks error better
  than the bare trace.
- **P5** missed its band by 1.95 points, in the single-hop baseline's favour.
- **P7** failed with the opposite sign: the reverse path does not save RESPONSE airtime here.

P5's and P8's unrounded values were computed from the same cells with the registered
analysis's own functions (`camp_spec_analyse.seeds`, `trace`, `arm_row`, `subj_rows`).

## 6. What follows

Nothing is adopted (PREREG §5). Open items this campaign leaves:
- **The integrity check's base rate** (§4; TECH_DEBT N.15).
- **The digest's 868 MHz cost** (N.23).
- **The reverse path's airtime against flooding** (P7).

`results/` before 2026-09-07 remains provisional, as `CLAUDE.md` states.
