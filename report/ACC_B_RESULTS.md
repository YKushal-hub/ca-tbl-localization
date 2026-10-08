# ACC-B results — hybrid range supply (N.35 + N.38)

2026-10-08. Exploratory grade. Registered in `report/PREREG_ACC_B_HYBRID.md` (`94a200ca`) before the
first evaluation cell. Runs were made outside the tree (`--outdir` in a scratch directory). They are not
tracked: they were made by a working build, and their `run_id.txt` says `source unknown, dirty 0`.

## 1. What was evaluated

- **Reference:** the merged tree before Part B (`100cc650`), `--specProfile=true`.
- **Candidate:** the build of `1c1886c7`, `--specProfile=true --dualBaseRelayTier=true --caseDInfoRatio=3`.
- **Runs:** seeds 1–20, 1800 s. A = 5 km; B = endgame (`--boxHalf=750 --finderMode=1`).
- **Arms:** `ca_tbl_full`, `fixed_threshold`, `static_2p4`, `poucet_fusion` and `lora_fusion`, each under both binaries.
- **Null control:** the candidate binary with the flags off is byte-identical to the reference on 334 of 334
  files (seed 1, A and B, three arms).

Verdicts come from a paired seed bootstrap (95 %).

## 2. The change (candidate minus reference, `ca_tbl_full`)

| | availability | within 25 m (5 nodes) | tag 0 within 25 m | tag 0 CEP50 (m) | tag 0 CEP95 (m) | J / node |
| --- | --- | --- | --- | --- | --- | --- |
| A | +0.011 [−0.000, +0.022] tie | +0.035 [−0.007, +0.079] tie | +0.046 tie | −1.22 tie | +0.76 tie | **+1.24 LOSS** |
| B | **+0.020 [+0.011, +0.029] WIN** | −0.010 [−0.036, +0.012] tie | +0.002 tie | −0.53 [−1.26, +0.33] tie | −0.07 tie | −0.15 tie |

Pooled values:
- **A:** availability 0.285 → 0.296; within 25 m 0.613 → 0.648; pooled CEP50 16.2 → 14.3 m.
- **B:** availability 0.562 → 0.582; pooled CEP95 16.2 → 14.9 m.

**Pre-registered predictions:**

| prediction | outcome |
| --- | --- |
| P1, availability WIN in both configurations | **partly**: WIN in B, a borderline tie in A |
| P2, endgame CEP50 and CEP95 WIN | **falsified**: both tie (the calibration seeds 21–30 had shown WIN) |
| P3, 5 km within 25 m ≥ 0 and tag 0 CEP50 ≤ 0 | holds |
| P4, no energy LOSS above +1.5 J | holds; A +1.24 J comes from the relayed exchanges on the 2.4 GHz radio |

No accuracy LOSS anywhere, so the falsifier did not fire.

## 3. Against the comparators, within one binary (rule 8)

Accuracy LOSSes of `ca_tbl_full`:

| | reference binary | candidate binary |
| --- | --- | --- |
| A vs `static_2p4` | within 25 m −0.071 [−0.131, −0.007] **LOSS** | −0.021 [−0.088, +0.037] tie |
| B vs `static_2p4` | tag 0 within 25 m −0.009 **LOSS**; CEP50 +0.85 m **LOSS** | −0.008 tie; +0.20 m tie |
| vs `fixed_threshold`, `poucet_fusion`, `lora_fusion` | none | none |

Under the candidate flags, `ca_tbl_full` has no accuracy LOSS against any arm in either configuration. It still
WINs against `poucet_fusion` and `lora_fusion` on availability and within 25 m in both configurations.

**Part of the A gap closed from the other side.** `--caseDInfoRatio` acts on every arm that runs Case D. At 5 km
it costs `static_2p4` 0.014 of within 25 m (LOSS) and 3.4 m of CEP50 (LOSS): k = 3 also refuses some of that
arm's lone 2.4 GHz ranges against a tight prior. The `ca_tbl_full` arm's own gain is the +0.035 in §2. In B
`static_2p4` is unchanged (all ties).

## 4. What was tried and rejected (calibration seeds 21–30, `ca_tbl_full`)

| flag | result | verdict |
| --- | --- | --- |
| N.36 `--relayCovScale` 4, 16 | no better than 1; A within 25 m −0.063 / −0.051 LOSS | kept at 1 |
| N.37 `--hybridNoLone868` | A tie; B availability −0.013, within 25 m −0.017 LOSS | superseded by N.38 |
| two-range Case D (NIS ≤ 9.21) | accuracy ties; B energy +8.0 J LOSS | not committed |
| N.38 at k = 1 | B within 25 m −0.019 LOSS (refuses useful 2.4 GHz ranges) | k = 3 chosen; k = 6 weaker |

## 5. Diagnoses that held

- **Relayed legs vanished under the dual base (N.35).** The active tier is 868 MHz on about two thirds of endgame
  cycles, and the relayed legs required the active tier.
- **Admitted relayed ranges are over-confident.** The realised error is 2.15× the claimed σ at depth 1 under N.35
  (static_2p4 1.11×). Scaling the relay covariance did not convert this into accuracy (§4).
- **A lone 868 MHz Case D update never helps (N.38).** It was flat in every prior-σ bin and harmful when the prior
  σ exceeded 80 m: +0.6 to +5.9 m per update, and +2 374 m summed over the 5 km subjects' first 360 s, seeds 1–20.
  The NIS test passes it because S is dominated by R.

## 6. Diagnoses that failed, and what remains

- **Per-cycle Δerror conditioned on cycle type is a selection effect, not an effect size.** It motivated N.37,
  which lost.
- **Relays' own fixes do not cause the error jumps** (H2): +0.11 m per cycle, the same as coasting.
- **The 5 km gap to `static_2p4` is mostly an initial-condition effect.** Subjects are seeded at truth with a
  100 m σ prior. `static_2p4` coasts on the IMU from there with almost no ranges, which is a 0.97 within-25 m share
  in the first 3 minutes against 0.83. By minute 12–15 the two arms are level. Narrowing the prior would change the
  scenario for every arm, and its comment explains why it is wide; it was not touched. It is recorded as TECH_DEBT
  N.38's open note.
- **Endgame accuracy did not move on seeds 1–20** (P2). The calibration WINs did not replicate. The larger accuracy
  lever measured so far is the filter (Part A: error-state replay 7.2 / 60.4 m at 5 km against 16.2 / 85.0 m),
  not range supply.
