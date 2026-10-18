# DEBT-A results: eight open TECH_DEBT items measured on the final preset (exploratory)

Registration: `report/PREREG_DEBT_A.md` (`e93f35c0`, before any cell). Amendment: `report/PREREG_DEBT_A_AMENDMENT1.md`
(`87413fa8`, before the amended cells ran).
- 320 cells, all `rc 0` and `dirty 0`.
- 240 cells are `source e93f35c0`; the 80 amended EDGEOFF/ARBOFF cells are `source 87413fa8`. The engine is the same:
  the amendment changed the runner only.
- Tracked: `results/debt_a_2026-10-18/`. Analyser: `scripts/debt_a_analyse.py`.

**Disclosure.** As registered, EDGEOFF and ARBOFF were refused at start-up by the preset's flag dependencies, so their
first 80 cells held no data. Amendment 1 added only the flags the simulator requires.

## 1. Predictions
| # | prediction | outcome | figure |
|---|---|---|---|
| D1 | NLOSDET: \|ΔCEP95\| ≤ 10 % (A, B) | **held** | A +14.7 [−37.9, +64.2] on 365; B +0.1 [−1.0, +1.0] on 5.0 |
| D2 | FOLB: B UWB share falls | **not held** | +0.0007 [−0.011, +0.012]: the UWB share is set by escalation, not reach |
| D3 | INTERF: A fix share rises | **not held** | −0.0015 [−0.019, +0.013] |
| D4 | RXPOW: A `static_868` CEP95 rises | **held** | +46.6 [−9.1, +97.2] m on 448 |
| D5 | SUBNOTDOA: A subjects within 25 m rise (H2, TDoA form) | **not held** | identical (0.309): the subjects' TDoA changes neither tag 0's run nor theirs |
| D6 | EDGEOFF: A `static_868` energy ≥ 1.25× | **held** | 1.352× |
| D7 | ARBOFF: B \|ΔCEP95\| ≤ 10 % | **not held** (at the bound) | −0.5 [−2.1, +0.6] m on 5.0: better, not worse |

## 2. What each item now knows (`ca_tbl_full` unless stated)
- **T27, NLOS truth in R (NLOSDET):** taking the oracle out of the filter (`--nlosRSource=detector`) costs nothing
  measurable at 5 km or in the endgame. Recommend adoption, so that no truth reaches the filter (owner, O4).
- **T12, foliage clamp (FOLB):** the truer foliage scaling improves accuracy.
  - 5 km CEP95: 365 → 330 m; after 300 s 260 → 241 m.
  - Endgame: 5.0 → 4.7 m.
  - `static_2p4` endgame: 12.3 → 10.6 m.
  - Mechanism: fewer marginal long links, not less UWB time. A physics correction; adoption is the owner's (O5).
- **T29, interference by distance (INTERF):** the most useful of the set.
  - Endgame CEP95 5.0 → 4.1 m; coverage 0.959 → 0.972.
  - 5 km after 300 s: 260 → 210 m.
  - Tag-0 energy −1.8 J at 5 km.
  - A physics correction (owner, O6).
- **T46, RSSI from received power (RXPOW):** 868 MHz alone gets worse (448 → 494 m at 5 km), as a truer, noisier RSSI
  range should. The hybrid is unharmed (5 km 365 → 361; endgame 5.0 → 4.4 m).
  - Its consistency (E[z²]) did not improve in dev: received links carry less foliage than the all-link expectation.
  - A realism correction with a known calibration caveat (owner, O7).
- **T26, H2 in its TDoA form (SUBNOTDOA):** falsified. Turning the subjects' tier-1 TDoA off leaves their within-25 m
  share unchanged (0.309), and tag 0 byte-identical in A. So the hybrid subjects' 5 km loss to static 2.4 GHz has
  neither H1, H2-range nor H2-TDoA as its cause. Recorded as a measured negative; the cause stays open as a finding.
- **T33, C6 under the fix-tier bill (EDGEOFF):** re-measured. Estimating on the gateway saves the node 26 % on
  `static_868` (11.05 → 8.16 J at 5 km) and 13–15 % on the hybrid (22.07 → 19.18 J at 5 km; 31.99 → 27.17 J in the
  endgame), with accuracy byte-identical. C6 holds under the current bill.
- **T42, identity-scoped arbitration (ARBOFF):** arbitration V2 costs energy and buys nothing measured here.
  - Turning it off cuts tag-0 endgame energy 27.2 → 22.2 J (−18 %) and halves UWB occupancy (0.049 → 0.026).
  - Endgame CEP95 is 0.5 m better without it; subjects' within-25 m is 0.931 → 0.938.
  - Claim 3 (the starvation and fairness property) is not supported on accuracy or energy. The arbitration's own
    objective, fairness between subjects, would need a per-subject grant-latency metric that is not in the traces.
    Recorded as a finding.

## 3. Table
| config | variant | arm | CEP95 | CEP95 after 300 s | coverage | fix share | UWB share | tag-0 J | subj within 25 m |
|---|---|---|---|---|---|---|---|---|---|
| A | BASE | static_868 | 447.6 | 412.4 | 0.433 | 0.788 | 0.000 | 8.16 | nan |
| A | BASE | static_2p4 | 402.7 | 89.1 | 0.919 | 0.046 | 0.000 | 13.38 | nan |
| A | BASE | ca_tbl_full | 365.1 | 259.8 | 0.692 | 0.851 | 0.006 | 19.18 | 0.309 |
| A | BASE | fixed_threshold | 361.7 | 239.6 | 0.718 | 0.851 | 0.006 | 18.92 | nan |
| A | NLOSDET | static_868 | 464.3 | 423.6 | 0.452 | 0.784 | 0.000 | 8.17 | nan |
| A | NLOSDET | static_2p4 | 400.6 | 91.1 | 0.925 | 0.045 | 0.000 | 13.32 | nan |
| A | NLOSDET | ca_tbl_full | 379.7 | 275.2 | 0.696 | 0.842 | 0.005 | 18.81 | 0.311 |
| A | NLOSDET | fixed_threshold | 331.3 | 199.5 | 0.704 | 0.854 | 0.004 | 17.83 | nan |
| A | FOLB | static_868 | 447.6 | 412.4 | 0.433 | 0.788 | 0.000 | 8.16 | nan |
| A | FOLB | static_2p4 | 400.0 | 95.7 | 0.920 | 0.046 | 0.000 | 13.94 | nan |
| A | FOLB | ca_tbl_full | 329.7 | 241.3 | 0.734 | 0.846 | 0.002 | 17.42 | 0.347 |
| A | FOLB | fixed_threshold | 327.1 | 240.1 | 0.736 | 0.843 | 0.005 | 18.56 | nan |
| A | INTERF | static_868 | 468.8 | 437.4 | 0.385 | 0.779 | 0.000 | 8.14 | nan |
| A | INTERF | static_2p4 | 403.5 | 69.8 | 0.935 | 0.048 | 0.000 | 13.41 | nan |
| A | INTERF | ca_tbl_full | 368.3 | 210.0 | 0.701 | 0.850 | 0.003 | 17.37 | 0.337 |
| A | INTERF | fixed_threshold | 412.2 | 279.6 | 0.714 | 0.837 | 0.003 | 17.38 | nan |
| A | RXPOW | static_868 | 494.2 | 468.4 | 0.439 | 0.783 | 0.000 | 8.24 | nan |
| A | RXPOW | static_2p4 | 402.7 | 89.1 | 0.919 | 0.046 | 0.000 | 13.38 | nan |
| A | RXPOW | ca_tbl_full | 360.9 | 249.6 | 0.662 | 0.840 | 0.001 | 16.22 | 0.322 |
| A | RXPOW | fixed_threshold | 363.7 | 256.1 | 0.661 | 0.839 | 0.005 | 17.89 | nan |
| A | SUBNOTDOA | static_868 | 449.0 | 414.5 | 0.446 | 0.784 | 0.000 | 8.15 | nan |
| A | SUBNOTDOA | static_2p4 | 402.7 | 89.1 | 0.919 | 0.046 | 0.000 | 13.38 | nan |
| A | SUBNOTDOA | ca_tbl_full | 365.1 | 259.8 | 0.692 | 0.851 | 0.006 | 19.18 | 0.309 |
| A | SUBNOTDOA | fixed_threshold | 361.7 | 239.6 | 0.718 | 0.851 | 0.006 | 18.92 | nan |
| A | EDGEOFF | static_868 | 447.6 | 412.4 | 0.433 | 0.788 | 0.000 | 11.05 | nan |
| A | EDGEOFF | static_2p4 | 402.7 | 89.1 | 0.919 | 0.046 | 0.000 | 13.26 | nan |
| A | EDGEOFF | ca_tbl_full | 365.1 | 259.8 | 0.692 | 0.851 | 0.006 | 22.07 | 0.309 |
| A | EDGEOFF | fixed_threshold | 361.7 | 239.6 | 0.718 | 0.851 | 0.006 | 21.85 | nan |
| A | ARBOFF | static_868 | 447.6 | 412.4 | 0.433 | 0.788 | 0.000 | 8.16 | nan |
| A | ARBOFF | static_2p4 | 402.7 | 89.1 | 0.919 | 0.046 | 0.000 | 13.38 | nan |
| A | ARBOFF | ca_tbl_full | 365.4 | 258.2 | 0.700 | 0.852 | 0.000 | 16.79 | 0.321 |
| A | ARBOFF | fixed_threshold | 361.6 | 235.5 | 0.713 | 0.851 | 0.000 | 16.67 | nan |
| B | BASE | static_868 | 27.5 | 24.6 | 0.593 | 0.964 | 0.000 | 9.34 | nan |
| B | BASE | static_2p4 | 12.3 | 10.6 | 0.939 | 0.833 | 0.000 | 8.98 | nan |
| B | BASE | ca_tbl_full | 5.0 | 4.0 | 0.959 | 0.996 | 0.049 | 27.17 | 0.931 |
| B | BASE | fixed_threshold | 4.6 | 3.9 | 0.959 | 0.996 | 0.055 | 28.25 | nan |
| B | NLOSDET | static_868 | 29.2 | 27.1 | 0.581 | 0.966 | 0.000 | 9.29 | nan |
| B | NLOSDET | static_2p4 | 10.0 | 8.1 | 0.936 | 0.836 | 0.000 | 8.92 | nan |
| B | NLOSDET | ca_tbl_full | 5.0 | 4.3 | 0.954 | 0.995 | 0.052 | 27.53 | 0.936 |
| B | NLOSDET | fixed_threshold | 4.8 | 4.1 | 0.951 | 0.995 | 0.055 | 29.21 | nan |
| B | FOLB | static_868 | 27.5 | 24.6 | 0.593 | 0.964 | 0.000 | 9.34 | nan |
| B | FOLB | static_2p4 | 10.6 | 9.8 | 0.940 | 0.796 | 0.000 | 8.77 | nan |
| B | FOLB | ca_tbl_full | 4.7 | 4.3 | 0.960 | 0.996 | 0.049 | 28.31 | 0.931 |
| B | FOLB | fixed_threshold | 5.2 | 4.3 | 0.954 | 0.996 | 0.053 | 31.39 | nan |
| B | INTERF | static_868 | 42.0 | 25.1 | 0.601 | 0.986 | 0.000 | 9.28 | nan |
| B | INTERF | static_2p4 | 10.4 | 8.4 | 0.928 | 0.825 | 0.000 | 8.92 | nan |
| B | INTERF | ca_tbl_full | 4.1 | 3.9 | 0.972 | 0.999 | 0.053 | 28.38 | 0.936 |
| B | INTERF | fixed_threshold | 4.3 | 4.1 | 0.967 | 1.000 | 0.053 | 28.89 | nan |
| B | RXPOW | static_868 | 46.8 | 38.0 | 0.570 | 0.965 | 0.000 | 9.34 | nan |
| B | RXPOW | static_2p4 | 12.3 | 10.6 | 0.939 | 0.833 | 0.000 | 8.98 | nan |
| B | RXPOW | ca_tbl_full | 4.4 | 3.9 | 0.964 | 0.996 | 0.053 | 29.09 | 0.939 |
| B | RXPOW | fixed_threshold | 5.1 | 4.7 | 0.948 | 0.996 | 0.055 | 29.04 | nan |
| B | SUBNOTDOA | static_868 | 29.0 | 26.6 | 0.593 | 0.965 | 0.000 | 9.34 | nan |
| B | SUBNOTDOA | static_2p4 | 12.3 | 10.6 | 0.939 | 0.833 | 0.000 | 8.98 | nan |
| B | SUBNOTDOA | ca_tbl_full | 4.9 | 4.0 | 0.959 | 0.996 | 0.048 | 27.36 | 0.930 |
| B | SUBNOTDOA | fixed_threshold | 4.5 | 3.8 | 0.957 | 0.996 | 0.055 | 27.93 | nan |
| B | EDGEOFF | static_868 | 27.5 | 24.6 | 0.593 | 0.964 | 0.000 | 14.19 | nan |
| B | EDGEOFF | static_2p4 | 12.3 | 10.6 | 0.939 | 0.833 | 0.000 | 8.40 | nan |
| B | EDGEOFF | ca_tbl_full | 5.0 | 4.0 | 0.959 | 0.996 | 0.049 | 31.99 | 0.932 |
| B | EDGEOFF | fixed_threshold | 4.6 | 3.9 | 0.959 | 0.996 | 0.055 | 33.07 | nan |
| B | ARBOFF | static_868 | 27.5 | 24.6 | 0.593 | 0.964 | 0.000 | 9.34 | nan |
| B | ARBOFF | static_2p4 | 12.3 | 10.6 | 0.939 | 0.833 | 0.000 | 8.98 | nan |
| B | ARBOFF | ca_tbl_full | 4.4 | 4.0 | 0.966 | 0.996 | 0.026 | 22.20 | 0.938 |
| B | ARBOFF | fixed_threshold | 4.3 | 4.0 | 0.967 | 0.996 | 0.020 | 20.41 | nan |

D1 NLOSDET ca_tbl_full dCEP95 A +14.7 [-37.9, +64.2] on 365.1, B +0.1 [-1.0, +1.0] on 5.0
D2 FOLB B ca_tbl_full dUWB share +0.0007 [-0.0114, +0.0122]
D3 INTERF A ca_tbl_full dfix share -0.0015 [-0.0185, +0.0131]
D4 RXPOW A static_868 dCEP95 +46.6 [-9.1, +97.2] on 447.6
D5 SUBNOTDOA A subjects within 25 m -0.0000 [-0.0001, +0.0000]
D6 EDGEOFF A static_868 energy ratio 1.352
D7 ARBOFF B ca_tbl_full dCEP95 -0.5 [-2.1, +0.6] on 5.0

## Predictions

- **D1**: held
- **D2**: not held
- **D3**: not held
- **D4**: held
- **D5**: not held
- **D6**: held
- **D7**: not held
