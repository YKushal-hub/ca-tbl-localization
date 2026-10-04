# CAMP-SPEC — results: the spec configuration measured

**Registered in `report/PREREG_CAMP_SPEC.md` (`c9db895`, engine `cef0fc6`) before any cell
ran.** All 260 cells exit 0 and read `source c9db895 dirty 0`. The tables are
`results/spec_2026-10-03/CAMP_SPEC.md`, the registered analysis applied unchanged; the
provenance is in `RUN.md`. The preset is `--specProfile`: GEO-01's fallback geometry and
CAL-02's calibrated PCS. Without the LR1121 basis, that is the spec configuration as far
as the build can express it.

**In one paragraph.** At the spec's 5 km scale the preset localises a third of the
subjects' cycles. It never forms a UWB fix, and it never makes a session precise. On every
headline metric it is beaten by the simplest arms: `static_868` for availability,
`ca_tbl_no_integrity` for CEP. Where finders converge on the subjects (the endgame), the
system works much better: availability 0.63, CEP95 9.8 m, sessions precise on 18 of 20
seeds. But there too the integrity check costs more than it saves, and the calibrated PCS
ranks error worse than the bare covariance trace. Each result has its cause recorded:
- floor-first selection: TECH_DEBT N.14;
- the trace inverted at 5 km: N.16;
- integrity firing on most fixes: N.15;
- the sparse geometry: GEO-01.

---

## 1. The preset at 5 km (A) and in the endgame (B): the six arms, seeds 1-20

| arm | A availability | A CEP50/95 tag 0 (m) | A subject J | B availability | B CEP50/95 tag 0 (m) | B subject J |
|---|---|---|---|---|---|---|
| `ca_tbl_full` | 0.333 [0.311, 0.357] | 22.1 / 46.0 | 14.7 | 0.630 [0.606, 0.652] | 5.6 / 9.8 | 39.1 |
| `ca_tbl_no_integrity` | 0.331 | **18.4 / 31.8** | 14.5 | **0.654** | **3.5 / 6.1** | 38.2 |
| `fixed_threshold` | 0.371 | 18.8 / 36.3 | 11.3 | 0.637 | 4.3 / 7.3 | 23.0 |
| `static_868` | **0.392** | 19.0 / 35.8 | 8.1 | 0.587 | 5.5 / 9.3 | 10.0 |
| `static_2p4` | 0.042 | 14.3 / 39.8 | 11.8 | 0.444 | 3.4 / 6.0 | 5.2 |
| `static_uwb` | 0.000 | (no fix) | 446.0 | 0.000 | (no fix) | 410.7 |

Availability is the mean over the five subjects, with 95 % seed-bootstrap intervals.
`static_uwb`'s CEP is of an estimate that never fixed, so it is not reported. Its energy
is a UWB radio held awake for 1 800 s.

- **UWB-delivered fixes:** none in A, in any arm. One in B (`ca_tbl_full`, over 20 seeds).
- **Sessions:**
  - A: the base station's batch session reaches first coarse at a median 165 s and
    precise never; the precision spread is UNDEFINED on 20 of 20 seeds.
  - B: first coarse at 53 s; the spread is defined on 18 of 20 seeds.
- **PCS against the trace control** (A = ρ(−trace, error) − ρ(PCS, error); > 0 means PCS
  ranks error better):
  - A: `ca_tbl_full` +0.36 [+0.16, +0.55] BETTER, because the trace is inverted at 5 km
    (N.16); `ca_tbl_no_integrity` INDISTINGUISHABLE.
  - B: both WORSE, −0.42 and −0.39. Where the trace works, the calibrated PCS (which
    dropped it) ranks error worse.

## 2. GEO-01 stage 3 (C against A, `ca_tbl_full`)

N8_ring (the preset) against N8_arc240 (the runner-up), under the full preset:
- availability 0.333 against 0.322;
- CEP95 46.0 against 48.0 m.

**The order holds.** Stage 2's fallback stands under the full preset.

## 3. The routing-weight sweep (D, endgame, seeds 1-10)

| α, β, γ, slack | delivered / originated | diverged hops |
|---|---|---|
| **0, 0, 1, 1 (jitter only)** | **0.594** | 6 804 |
| 1, 0.01, 1, 1 | 0.589 | 7 136 |
| 1, 0.01, 1, 2 | 0.585 | 6 312 |
| 1, 1, 1, 2 | 0.482 | 20 355 |
| 1, 1, 1, 1 (the default) | 0.479 | 20 067 |
| 0, 1, 0, 1 (RSSI variance only) | 0.422 | 23 910 |
| 1, 0, 0, 1 (NLOS only) | 0.284 | 28 706 |
| 1, 0, 0, 2 | 0.215 | 31 740 |

- The registered criterion picks **(0, 0, 1, 1)**: 0.594 against the default's 0.479.
- At equal weights the dB² RSSI-variance term dominates the cost, as the FixRouter comment
  warns, and steers fixes onto worse routes. Shrinking it to 0.01 recovers almost all of
  the gain.
- The registration does not adopt the pick into the preset. The preset measured here
  used the default, so adopting it is the owner's call (handoff §3).

## 4. The fault matrix (E, seeds 1-10; tag 0; label = the fault window)

| fault | recovery | integrity precision | recall | CEP95 in window (m) | CEP95 outside (m) | no-integrity arm, in window |
|---|---|---|---|---|---|---|
| `rb_common` (30 m) | downweight | 0.257 | 0.982 | 33.5 | 41.3 | 27.5 |
| `rb_common` | none | 0.257 | 0.978 | 50.4 | 68.1 | 27.5 |
| `rb_single` (30 m) | downweight | 0.257 | 0.982 | 33.5 | 41.2 | 27.2 |
| `rb_single` | none | 0.257 | 0.969 | 39.6 | 64.0 | 27.2 |
| `txdrop` (6 dB) | downweight | 0.239 | 0.964 | 42.5 | 42.3 | 34.2 |
| `txdrop` | none | 0.234 | 0.958 | 35.9 | 52.2 | 34.2 |
| `antenna` (10 dB) | downweight | 0.147 | 0.884 | 39.9 | 44.0 | 30.9 |
| `antenna` | none | 0.142 | 0.846 | 43.1 | 53.7 | 30.9 |

- The check's recall is high, 0.85-0.98, but its precision is 0.14-0.26: SUSPECT fires on
  most cycles outside the window too.
- Downweighting (the spec's response) beats reject-and-coast on in-window error for the
  bias faults: 33.5 against 50.4 and 39.6 m.
- The arm without the check does better still, inside and outside the window.

## 5. The single-hop baseline (G, against A and B; `ca_tbl_full`, tag 0)

| configuration | hop depth | fix cycles | median error (m) | ρ(PCS, error) |
|---|---|---|---|---|
| preset, 5 km | 0 | 12 653 | 16.5 | −0.112 |
| preset, 5 km | 1 | 401 | 9.7 | −0.353 |
| single-hop, 5 km | 0 | 12 931 | 15.4 | −0.117 |
| preset, endgame | 0 | 4 500 | 5.0 | −0.148 |
| preset, endgame | 1 | 9 819 | 4.3 | −0.113 |
| single-hop, endgame | 0 | 14 327 | 4.5 | −0.119 |

- Relayed fixes are the *better* ones where they occur (9.7 against 16.5 m at 5 km).
- But removing relaying changes the median error little (15.4 and 4.5 m), because relays
  carry few fixes at 5 km and many redundant ones in the endgame.
- Confidence calibration across hop counts is weak everywhere (ρ −0.11 to −0.35).

## 6. Predictions

- **P1 held.** No UWB-delivered fix at 5 km, in any arm. The precision spread is
  UNDEFINED on 20 of 20 seeds.
- **P2 failed.** It said INDISTINGUISHABLE or WORSE for `ca_tbl_full` in A and B. A was
  BETTER, against an inverted trace (N.16); B was WORSE.
- **P3 held.** Integrity precision is below 0.5 under every fault (0.14-0.26).
- **P4 failed.** In the endgame `static_uwb` has the highest subject energy (410.7 J) but
  no fix at all, so not the lowest CEP50. A UWB-only node needs three UWB anchors within
  tens of metres, and the endgame gives it at most one finder (MS-3).
- **P5 held.** At 5 km the single-hop baseline's depth-0 median error (15.4 m) is within
  10 % of the preset's (16.5 m).

The DC-7 consequence registered in §2 of the registration **failed**: downweight's
in-window CEP95 is better than reject-and-coast's, not no better, for both bias faults.

## 7. What it means for the owner

- The mechanisms built to the spec are present and exercised (G24 lists the five that are
  not). Their *combination* at the spec's scale is outperformed by simpler arms. The
  causes are structural and recorded:
  - floor-first selection discards the UWB and sparse fixes (N.14);
  - the integrity check flags most fixes (N.15, §4 here);
  - the covariance trace is inverted where 868 MHz dominates (N.16);
  - eight gateways cover a third of 10 km² (GEO-01).
- The routing weights have a measured better setting, (0, 0, 1, 1), not adopted.
- In the endgame the system does what the spec describes: sessions become precise, grants
  are confirmed, and one UWB fix forms. But the integrity check and the calibrated PCS
  still cost accuracy there.
