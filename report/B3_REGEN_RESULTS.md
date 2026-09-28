# B3 regeneration — results

**2026-09-28.** Registered in `report/PREREG_B3_REGEN.md` at `7b17bd6`, before any cell
ran. Branch `work/2026-09-27-b3-regen`, worktree `~/catbl_b3`, binary
`./build/scratch/ns3.47-ca_tbl_causal_sim-optimized` built from `084a7ff` (`src/` and
`scratch/` byte-identical at `7b17bd6`). 125 processes, 0 failed.

The registered verdict rules are applied **verbatim**. Where an analyser prints something
its question does not support, both readings are given below and the one the registration
binds is named. Every number is a paired comparison inside this one binary, same seed, same
invocation apart from the treatment (prereg §1). No regenerated magnitude is differenced
against a pre-correction original; originals are named only to identify the claim, and the
only cross-binary statements made are byte-identity statements, which are control-flow
facts.

---

## Verdict table

Signs: every paired difference is quoted so that the direction of the claim is explicit on
the line. Sign tests are exact, two-sided, ties (exact zeros) excluded from *n* and counted
separately, because an exact zero is a control-flow claim (rule 4). p = 1.9×10⁻⁶ is 20/20.

| # | claim (prereg) | verdict | number and test | denominator · window |
|---|---|---|---|---|
| C1 | the witness cross-check earns its place (P1.1 energy, P1.2 accuracy) | **WITHDRAWN — NO POPULATION** (byte-identity clause) | `off`/`on` rows byte-identical on **8/8** seeds. ΔtotalJ +0.000 J, CI [+0.000, +0.000]; ΔrmseM +0.000 m, CI [+0.000, +0.000]. The check **did** refuse 203–232 escalations per seed (IMU; corroboration 0) and none of them changed any output | 8 paired seeds · 600 s, 240 cycles per run · bootstrap B = 20 000 over seeds |
| C2 | P2.1 `gated` beats `legacy` on cep50, `ca_tbl_no_integrity` | **SIGN FLIPS** | Δcep50 (gated − legacy) **+12.96 m** mean, +10.96 m median, **15+/5−, p = 0.041**: gated is *worse* | 20 paired seeds · cep50 over all 1 440 cycles · 3 600 s |
| C2 | P2.2 PCS does not beat the monotone control, all 8 form × arm cells | **REPRODUCES in 6/8, INCONCLUSIVE in 2/8** | legacy, v2, weighted × 2 arms: advantage CI entirely below zero. `gated` × 2 arms: statistic **undefined** — PCS = 0.0 on 20 752 of 20 755 and 20 765 of 20 768 hadFix cycles; the analyser prints "indistinguishable" for a NaN. No cell beats its control; no WRONG SIGN cell | hadFix cycles only, 20 708–23 213 of 28 800 per cell · seed bootstrap 10 000 |
| C2 | P2.3 `gated` cheaper than `weighted`, `ca_tbl_no_integrity` | **WITHDRAWN** | 1−/1+, **18 exact zeros**, p = 1.00; 18/20 seeds byte-identical rows | 20 paired seeds · totalJ over 3 600 s |
| C3 | P3.1 CA-TBL beats MLAT on cep95, every moving cell | **REPRODUCES** | 19+/1− (p = 4.0×10⁻⁵), 18+/2− (p = 0.0004), 20+/0− (p = 1.9×10⁻⁶); medians 6 665 vs 13 762, 6 759 vs 38 599, **2 016 vs 20 831 m** | 20 paired seeds per cell · cep95 over 5 760 cycles · 14 400 s |
| C3 | P3.2 not distinguishable from `static_868`, every moving cell | **REPRODUCES** | 1+/1− with 18 exact ties (p = 1.00), 10+/10− (p = 1.00), 11+/9− (p = 0.82) | as above |
| C4 | P4.1 `ca_tbl_full` dominates `static_868` in no cell | **FALSIFIED in 3/11 cells** → revised for those cells and reported as an envelope (the registered falsifier); reproduces in 8/11 | `k1a_box750`, `k1a_box1000`, `k1a_box1500`: **DOMINATES** (energy 20+/0− in each; rmse 18+/2−, 11+/9−, 13+/7−). Envelope in §C4 | 20 paired seeds per cell · rmseM over 1 440 cycles, totalJ over 3 600 s |
| C4 | P4.2 `static_2p4` dominates `ca_tbl_full` in every cell | **REPRODUCES** (11/11) | energy 0+/20− in 10 cells, 2+/18− in `k1c_brief` | as above |
| C5 | P5.1 availability WAN only < sparse < WAN+mesh; WAN+mesh > WAN only | **REPRODUCES** | means **0.041 < 0.319 < 0.647**; wanmesh − wanonly 20+/0−, p = 1.9×10⁻⁶ | fixCycles / steps, 5 760 cycles per seed · 14 400 s · 20 seeds |
| C6 | P6.1 accuracy byte-identical nodeside vs edge | **REPRODUCES** (control flow) on the 5 arms where the edge path executed; **not claimed** on `static_uwb`, where it did not | 7 accuracy columns identical, 20/20 seeds, every arm | 20 seeds × 6 arms · 3 600 s |
| C6 | P6.2 edge lowers totalJ on `static_868`, `fixed_threshold`, `ca_tbl_full` | **REPRODUCES** | 20/20 each, p = 1.9×10⁻⁶: **42.5 %**, **32.9 %**, **32.9 %** lower (mean of per-seed fractions) | 20 paired seeds · totalJ over 3 600 s |
| C7 | P7.1 an ideal dead reckoner collapses cep95 on `static_uwb`, `static_2p4` — **a ceiling** | **REPRODUCES** (as a ceiling, never a system result) | `static_uwb` median 8 413 → 17 m, `static_2p4` 27 151 → 537 m, both 20+/0−, p = 1.9×10⁻⁶. `ca_tbl_full` 11+/9−, p = 0.82, not significant | 20 paired seeds · cep95 over 5 760 cycles · 14 400 s |

**Headline.** The mesh (C5), the filter against snapshot estimators (C3), the edge-server
tier-1 saving (C6) and the IMU ceiling (C7) survive regeneration. The witness cross-check
(C1) has no measurable effect in the registered configuration and is withdrawn. The PCS
repair (C2) reverses: `gated` is significantly worse than `legacy`, and under each of the
three repaired forms `ca_tbl_no_integrity` is byte-identical to the flat variance
threshold, `fixed_threshold`, on 19 of 20 seeds. K1's
"tiering never dominates `static_868`" fails in three cells, for a reason that is about
billing rather than about escalation paying (§C4).

---

## 0. Provenance and completeness, checked independently of the driver

The driver's own completeness lines (`~/catbl_b3_logs/B3_SUMMARY.txt`) were not reused.
Recounted from the files (rule 3):

- **125/125** sidecars read `source 7b17bd6 dirty 0 profile optimized`; **125/125**
  `digest sha256` blocks verify with `sha256sum -c` in their cell directory.
- Line counts: 16 × 2 (C1), 80 × 7 (C2), 16 × 31 (C3 chunks + C7 chunks), 13 × 121
  (C4 + C6) — the registered numbers. Every C2 cell holds exactly 6 `trace_*.csv`; no other
  campaign holds any.
- Every invocation line ends with the registered arguments (spot-read for every cell type;
  the driver checked all 125).
- The earliest cell directory is dated 2026-09-28 11:16; `7b17bd6` was committed at 11:13:33.
- **The part of the RULES.md §6 invariant the energy axis needs** (`docs/TECH_DEBT.md`
  K.3 separates it from the literal sentence) **holds on all 2 536 regenerated rows**: on every row where a
  tier has `cycles_<tier> == 0`, its `tx_`, `rx_`, `cad_` and `idle_` energies are exactly
  0.0 and `j_<tier>` equals `sleep_<tier>` exactly (840, 912 and 2 116 such rows for 868,
  2.4 and UWB). The check has a negative fixture (a copied row with `rx_uwb` set non-zero)
  and was observed to fail on it. Its first draft demanded `j == 0` and flagged every
  unused radio's sleep current; the predicate was wrong, not the rows.

## 1. Every analysis command

Run from `~/catbl_b3` with Python 3.8.10. Outputs were written to
`~/catbl_b3_logs/analysis/` (outside the tree, not citable; the commands regenerate them).

```bash
# C1 -- registered analyser, unchanged
python3 scratch/python_handoff/analyse_integrity_ci.py results/regen27_integrity
# C1 -- population check and byte identity (prereg C1, rule 4)
grep -h "INTEGRITY: escalations refused" results/regen27_integrity/on_s*/run.log
grep -h "SHS: engine at" results/regen27_integrity/on_s*/run.log
for s in 1 2 3 4 5 6 7 8; do cmp <(tail -n +2 results/regen27_integrity/off_s$s/arm_summary.csv) \
                                 <(tail -n +2 results/regen27_integrity/on_s$s/arm_summary.csv) \
                             && echo "s$s identical"; done

# C2 -- registered analyser, ROOT changed and nothing else (prereg C2)
sed 's|^ROOT = "results/pcs_forms"$|ROOT = "results/regen27_pcs_forms"|' \
    scratch/python_handoff/analyse_pcs_forms.py > ~/catbl_b3_logs/analysis/analyse_pcs_forms_regen27.py
diff scratch/python_handoff/analyse_pcs_forms.py ~/catbl_b3_logs/analysis/analyse_pcs_forms_regen27.py   # the ROOT line only
python3 ~/catbl_b3_logs/analysis/analyse_pcs_forms_regen27.py
# C2 P2.3 -- registered procedure, computed from arm_summary rows
python3 scripts/b3/b3_regen_analyse.py p23

# C3 (+ expectation E3), C5, C6, C7 -- registered procedures; no analyser is named for them
python3 scripts/b3/b3_regen_analyse.py k10
python3 scripts/b3/b3_regen_analyse.py avail
python3 scripts/b3/b3_regen_analyse.py edge
python3 scripts/b3/b3_regen_analyse.py imu

# C4 -- registered analyser, unchanged
python3 scratch/python_handoff/k1_analyse.py results/regen27_k1/*

# SUPPLEMENTARY, not registered -- they compute no verdict
python3 scripts/b3/b3_c2_diagnostics.py      # C2 denominators, PCS constancy, trigger rate, identities
python3 scripts/b3/b3_identity_checks.py     # which columns differ: E3, K10 and L2 originals, edge, wanonly
python3 scripts/b3/b3_energy_invariant.py    # RULES.md section 6 on every row, and per-cycle tier cost
python3 scripts/b3/b3_energy_invariant.py ~/catbl_b3_logs/analysis/fixture/arm_summary_bad.csv   # must exit 1
# default drift between the originals' commits and this one
git merge-base --is-ancestor 7ec0d91 <commit>          # for deb52da 9723dfc 1e8e527 5bbac5d 4779bf7 5d20de9 808885a 4302918
git show deb52da:scratch/ca_tbl_causal_sim.cc | grep -c '"nFinders"\|"imuCrossCheck"\|"corroborationCheck"'
# is the rmse_mlat difference (E3, C7) a code change? -- the solver, textually, at each commit
for c in 4302918 808885a 7b17bd6; do git show $c:scratch/ca_tbl_causal_sim.cc \
    | awk '/^BaselineMultilaterate\(/{m=1} m{print} m&&/^}/{exit}' | sha256sum; done
```

`scripts/b3/b3_regen_analyse.py` is the prereg's written procedure and nothing more: it
concatenates the four chunk CSVs of a K10/IMU cell (header once), refuses a cell that does
not count exactly 120 rows with every arm holding seeds 7–26 once, and then runs the
registered sign tests. That concatenation is the only transformation the prereg permits
(§3).

---

## C1 — the witness cross-check

**What is being tested.** `--imuCrossCheck` and `--corroborationCheck`: the block
commented SWITCHING-POINT INTEGRITY at `ca-tbl-policy.cc:1233-1269`. It is **neither** the
Mahalanobis check that `RULES.md` §2 calls the Switching-Point Integrity Check (that one
runs in both arms here; `suspects` column) **nor** the evidence gate (`Architecture.md`
§9.3). All three are suppress-only; they are kept distinct below.

**Registered analyser.** `analyse_integrity_ci.py` on 8 complete pairs:

| | off → on, every seed | mean Δ | 95 % bootstrap CI | registered reading |
|---|---|---|---|---|
| rmseM | identical, 4.762–13.634 m | +0.000 m | [+0.000, +0.000] | CI includes zero → P1.2 **WITHDRAWN** |
| totalJ | identical, 7.070–7.381 J | +0.000 J | [+0.000, +0.000] | CI includes zero → P1.1 **WITHDRAWN** |
| escalations | 1 → 1 | +0 | [+0, +0] | — |

**Population check (rule 4, the registered first step).** Every `on` log reports
`IMU inconsistency N, weak corroboration 0`, with N = 203, 214, 206, 208, 211, 216, 232,
223 for seeds 1–8 — 1 713 across the eight 600 s runs, each one cycle's refusal, so no
event straddles the window. N + M > 0 on every seed, so the NO POPULATION branch is **not**
reached by the refusal-count clause. It **is** reached by the next sentence of the
registration: *"Byte-identical `off`/`on` rows on all eight seeds mean the same thing."*
All eight are byte-identical. **Binding verdict: NO POPULATION → withdrawn as unsupported.**

The exact zero is therefore established as a control-flow fact of the strongest kind: the
refusal path is reachable and was reached 1 713 times, and not one refusal reached an
output. The analyser and the population rule agree: its `improved in 0 of 0 seeds, p = nan`
and its degenerate [0, 0] intervals are the same byte identity, not an interval estimate.

**Why nothing moved — inferred from code and rows, not measured** (the evidence-gate
refusal counters are not printed):

1. In both arms the node escalates once, to 2.4 GHz, within the first three cycles, and
   stays there for 237–239 of 240 cycles; `cycles_uwb` = 0 and `cyclesInUwbCoverage` = 0 on
   every seed. Every later escalation request is therefore 2.4 GHz → UWB.
2. That step is subject to the **evidence gate**, `escalationRequiresEvidence`, which
   **defaults to true** in this binary (`ca_tbl_causal_sim.cc:6693`) and refuses it when
   the estimate is too uncertain or fewer than three UWB-map anchors lie within the
   predicate radius (`ca-tbl-policy.cc:1271-1353`).
3. The witness block runs **before** the evidence gate (`:1239` against `:1271`). When the
   IMU witness refuses, it takes a refusal the evidence gate would otherwise have made. The
   corroboration witness never votes (M = 0) because it runs only if the IMU witness has not
   already refused (`:1258`).
4. The IMU witness refuses because **the dead reckoner is wrong, not the filter.** At the end
   of each run the SHS engine is 315, 724, 413, 542, 451, 522, 485 and 709 m from truth
   (seeds 1–8; one end-of-run snapshot each), against tolerances of 15–136 m, while the
   filter's RMSE over the run is 4.8–13.6 m. The drift constants are ours and uncalibrated
   (`docs/TECH_DEBT.md` K.9).

**A registration defect, recorded rather than corrected.** Prereg §2 lists the defaults
that moved between the originals and this binary — the environment and the tier-1 reply
billing — but **not** `--escalationRequiresEvidence`, which went from `false` to `true` at
`7ec0d91` (2026-09-08 07:20). The C2 originals' commits (`9723dfc`, `1e8e527`) and K1's
(`5bbac5d`) do not contain `7ec0d91` (`git merge-base --is-ancestor`); the C3, C6 and C7
originals' commits do, and the K10, `k1a` and `k1c` invocations pin the flag anyway. The C1
original's binary was built on 2026-09-07 at 21:36, before `7ec0d91` was committed (its
recorded SHA cannot be trusted; see the next paragraph), so for C1 this is inferred from the
date. A same-name diff of the variable initialisers in `main()` (`deb52da`, `9723dfc`,
`1e8e527`, `5bbac5d` against `7b17bd6`) finds only `envFacilityInt`, `vegFacilityInt`,
`escalationRequiresEvidence` and, for `5bbac5d`, `tier1ChargeRepliesArg`; the only new
default-on boolean since any of them is `tier1ChargeRepliesArg`; and the policy-config
constructor (`ca-tbl-policy.cc`) only gained fields, all default-off or parameters, and
changed none. So **C1, C2 and the five `k1b` cells ran with the evidence gate on where
their originals ran with it off.** The verbatim invocation is what the prereg binds, so no
verdict changes; what changes is what the verdict is about. C1 says the witness check adds nothing **in the shipped default**
configuration. Whether it earns its place **with the evidence gate off** — the question the
original answered — was not re-tested and needs its own registration pinning
`--escalationRequiresEvidence=false`.

**A provenance defect in the C1 original.** Its sidecars (`~/catbl_work/cc/*/run_id.txt`,
untracked) read `source deb52da dirty 0`, and the prereg cites them as "at `deb52da`". But
`deb52da` does not define `--nFinders`, `--imuCrossCheck` or `--corroborationCheck`
(0 matches in its `ca_tbl_causal_sim.cc`), so that binary cannot have been built from it.
The sidecar records a binary under `~/projects/ca-tbl-localization/build/`, built
2026-09-07 21:36; `deb52da` is, as of this analysis, the HEAD of the Windows-side clone at
`C:/projects/ca-tbl-localization`, clean under `src/` and `scratch/` — consistent with a
run launched from that clone. The simulator records `git rev-parse` of the **working directory**, not of the
tree the binary was built from, so a run launched from one clone with another clone's
binary gets a clean, wrong SHA. No tracked sidecar cites `deb52da`. B3 itself is not
exposed: every cell ran from `~/catbl_b3` with a cwd-relative binary, and the driver hashed
that binary before every launch (prereg §7, A2).

## C2 — PCS forms and the monotone control

**P2.1, registered analyser part 1** (`d cep50M` is form − legacy; positive = form worse):

| arm | form | d cep50M | d totalJ | d escalations | sign(cep) |
|---|---|---|---|---|---|
| `ca_tbl_no_integrity` | v2 | +14.447 m | +12.118 J | +138.2 | 16+/4−, p = 0.0118 |
| | weighted | +14.447 m | +12.118 J | +138.2 | 16+/4−, p = 0.0118 |
| | **gated** | **+12.961 m** | **+12.103 J** | +138.3 | **15+/5−, p = 0.0414** |
| `ca_tbl_full` (reported, not registered) | v2 / weighted | +3.826 m | +3.629 J | +33.2 | 10+/10−, p = 1.00 |
| | gated | +6.747 m | +3.606 J | +33.2 | 11+/9−, p = 0.82 |

The registered prediction is a negative mean, a negative majority and p < 0.05. All three
fail in the opposite direction, and significantly. The prereg's P2.1 falsifier names only
the not-significant branch (→ WITHDRAWN); a significant reversal is not named, and is
labelled **SIGN FLIPS** here, the label the same registration uses for P2.3's significant
reversal. Either way the repair claim is withdrawn. p = 0.041 is one test among several at
α = 0.05, and it is reported at that strength.

**P2.2, registered analyser part 2** (advantage = |ρ_PCS| − |ρ_control|; per-seed Spearman
over hadFix cycles; seed bootstrap, 10 000 resamples):

| form | arm | ρ_PCS | ρ_control | advantage | 95 % CI | analyser prints | registered reading |
|---|---|---|---|---|---|---|---|
| legacy | full | −0.0281 | −0.2654 | −0.2349 | [−0.2819, −0.1846] | LOSES | negative reproduces |
| legacy | no_integrity | −0.0337 | −0.2126 | −0.1801 | [−0.2320, −0.1283] | LOSES | negative reproduces |
| v2 | full | −0.1395 | −0.2478 | −0.1083 | [−0.1424, −0.0747] | LOSES | negative reproduces |
| v2 | no_integrity | −0.0839 | −0.1307 | −0.0522 | [−0.0721, −0.0324] | LOSES | negative reproduces |
| weighted | full | −0.0274 | −0.2478 | −0.2073 | [−0.2649, −0.1466] | LOSES | negative reproduces |
| weighted | no_integrity | −0.0321 | −0.1307 | −0.0984 | [−0.1368, −0.0606] | LOSES | negative reproduces |
| gated | full | nan | −0.2407 | nan | [nan, nan] | "indistinguishable" | **INCONCLUSIVE — undefined** |
| gated | no_integrity | nan | −0.1273 | nan | [nan, nan] | "indistinguishable" | **INCONCLUSIVE — undefined** |

**Where the analyser is wrong for its question.** Under `gated`, PCS is exactly 0.0 on
20 752 of 20 755 hadFix cycles (`full`) and 20 765 of 20 768 (`no_integrity`), constant on
every used cycle in 18/20 and 17/20 seeds. Spearman on a constant vector is undefined;
`pearson()` returns NaN (`analyse_pcs_forms.py:70-71`), the NaN enters the mean, and a NaN
interval satisfies neither `lo > 0` nor `hi < 0`, so the script prints
`indistinguishable` (`:188-193`) and its closing paragraph reads that as "carries nothing
the covariance trace does not". **Which reading binds:** the prereg binds the analyser's
printed columns under its reading rule. The printed ρ_PCS is not negative and the CI is not
above zero, so neither cell beats its control and the negative is withdrawn nowhere; the CI
is also not below zero, so P2.2's "all eight cells" is **not met** for these two. The
correct statement is that the statistic does not exist for them — a score that is a
constant on 99.98 % of fix cycles cannot rank anything. The analyser's docstring also says
the hadFix count "is reported per cell"; it is not printed. The denominators in the table
header come from `scripts/b3/b3_c2_diagnostics.py`: 21 212 / 23 213 (legacy),
20 708 / 20 708 (v2, weighted), 20 755 / 20 768 (gated), of 28 800 cycles each.

No mean ρ_PCS is positive, so no cell is WRONG SIGN; the sparse-regime reversal of
`report/ROOTCAUSE_ENVELOPE.md` Q2 did not appear at `boxHalf=2000`.

**P2.3, computed from `arm_summary.csv` rows** as registered: totalJ(gated) −
totalJ(weighted) on `ca_tbl_no_integrity` is +2.56 J at seed 2, −2.86 J at seed 12 and
**exactly 0.0 on the other 18**; 1−/1+, p = 1.00 → **WITHDRAWN**. The 18 zeros are
byte-identical rows. Reachability (rule 4): the gated branch did execute — its PCS is 0.0
where weighted's is ≈ 1.0 on the same cycles — it simply changed no decision.

**Why the three repaired forms collapse to one controller** (supplementary, from the
traces; `b3_c2_diagnostics.py`):

- The escalation trigger `pcs < pcsThresholdUp[tier]` is true on **20 708 / 20 708** hadFix
  cycles under v2 and under weighted, and on **20 752 / 20 755** and **20 765 / 20 768** under
  gated. Under legacy it is true on 4 008 / 21 212 (18.9 %, `full`) and 3 321 / 23 213
  (14.3 %, `no_integrity`).
- The cause is the uncertainty term. Median `traceSigma` is 7.8×10⁴–1.2×10⁵ m² against
  `traceRef` = 10 m², so term 1 ≈ 10⁻⁴: below `pcsGateFloor` (0.10, gated → 0.0), and
  leaving weighted ≤ 1.0 and v2 ≈ −2.2 — under every escalation threshold. **The saturation
  defect has reversed sign.** Legacy saturates high on most cycles (modal PCS 2.0001) and
  asks to escalate on 14–19 % of fix cycles; every repaired form saturates low and asks on
  every one, so under them the score decides nothing and the downstream gates decide
  everything.
- Consequences, all byte-identity counts over 20 seeds: v2 ≡ weighted on every arm 20/20
  (their traces differ only in `pcs` and `sigmaGeoM`); gated ≡ v2 on each policy arm 18/20;
  and **`ca_tbl_no_integrity` is byte-identical to `fixed_threshold` on 19/20 seeds under
  each of v2, weighted and gated** (escalation counts equal 20/20 under gated). Under the
  repaired forms, in this environment, the confidence-aware arm *is* the flat
  variance-threshold baseline.
- The prereg's replacement for the original's §7 abort check (legacy reproducing
  `baseline_v7`) was §2's provenance check, which passed (§0).

## C3 — K10, the architecture against published estimators

cep95 in metres, 20 seeds per cell, subject `ca_tbl_full`. Positive = CA-TBL better.

| cell | CA-TBL | `static_868` | MLAT | WCL | PROX | LKP |
|---|---|---|---|---|---|---|
| `wanonly_move` median / mean | 6 665 / 7 935 | 6 665 / 7 935 | 13 762 / 15 231 | 8 700 / 9 423 | 8 941 / 9 580 | 8 413 / 8 948 |
| sign test | — | 1+/1−, 18 ties, p 1.00 | **19+/1−, p 4.0×10⁻⁵** | 16+/4−, p 0.012 | 16+/4−, p 0.012 | 16+/4−, p 0.012 |
| `sparse_move` median / mean | 6 759 / 8 010 | 3 649 / 9 728 | 38 599 / 56 969 | 4 537 / 4 874 | 4 693 / 5 013 | 8 413 / 8 948 |
| sign test | — | 10+/10−, p 1.00 | **18+/2−, p 0.0004** | 8+/12−, p 0.50 | 9+/11−, p 0.82 | 13+/7−, p 0.26 |
| `wanmesh_move` median / mean | 2 016 / 3 261 | 2 076 / 3 266 | 20 831 / 29 456 | 2 963 / 3 244 | 3 403 / 3 512 | 8 413 / 8 948 |
| sign test | — | 11+/9−, p 0.82 | **20+/0−, p 1.9×10⁻⁶** | 13+/7−, p 0.26 | 13+/7−, p 0.26 | 20+/0−, p 1.9×10⁻⁶ |

**P3.1 REPRODUCES** in all three cells; the falsifier (failure in `wanmesh_move`) is not
triggered. **P3.2 REPRODUCES**: indistinguishable from `static_868` everywhere.

**Denominators and censoring (rules 1–2).** The EKF's cep95 is over every cycle of the run.
A rival's is over every cycle **from its first estimate onward**: it holds its last estimate
when it cannot solve, and is not charged for cycles before it has one
(`ca_tbl_causal_sim.cc:5934-5993`). That asymmetry can only favour the rivals, so it cannot
have produced P3.1. The simulator's `Percentile()` returns 0.0 for an empty vector (`:1383`),
which would score a rival that never estimated as perfect; `mlat_solved` is ≥ 1 on every
`ca_tbl_full` row and no rival cep95 is 0.0, so no such row exists.

**The `wanonly_move` ties are control flow.** `ca_tbl_full` escalates **0 times on 20/20
seeds** (`cycles_2p4` = `cycles_uwb` = 0); its row equals `static_868`'s on 17 seeds, and
on seeds 11, 21 and 26 differs only through the Mahalanobis check (`suspects`). Without
finders, `ca_tbl_full` in that cell is `static_868` plus that check; the tiering is not
exercised at all. The WCL and PROX results (better in `sparse_move`, not significantly) are
reported, not registered; with them the MLAT result reads as K10 read it: a filter beats a
memoryless solver.

**Admissible identity with the originals.** Every column the C3 verdicts read is
byte-identical to `results/k10_wanonly_move_s7` and its siblings on all 120 rows per
cell — which is why the medians above equal those K10 published. The rows differ only in
the energy columns of the four tier-1-using arms (80 rows; the reply billing of `4779bf7`,
prereg §2), in four columns the originals do not have (`meanErrM`, `meanErrFixedM`,
`tFirstFixS`, `tFirst100mS`), and in `rmse_mlat` on 0 / 9 / 20 rows.

**Expectation E3 fails as registered and names its code path.** `wanmesh_move` against
`results/L1_imuoff_s7` and siblings: seed 7 identical (seen in the probe), and 6 of the 19
tested seeds fully identical; seeds 9, 10, 12, 15–19, 21, 22, 24–26 are not. **The only
differing column is `rmse_mlat`**, on 20 rows, from the 7th to the 11th significant digit
of values between 4.8×10⁴ and 1.0×10⁶ m, with `cep95_mlat` and `mlat_solved` identical on
every one. The code is not what moved: `BaselineMultilaterate()`, the rival block,
`rmseOf` and `Percentile()` are textually identical at `808885a`, `4302918` and `7b17bd6`,
and every EKF column — which reads the same measurements — is identical. The same code
evaluated differently in the tail of an ill-conditioned solve, where an RMSE of 10⁵–10⁶ m
is dominated by a few near-singular solutions; a build-host or toolchain difference
(`docs/TECH_DEBT.md` §4.1–4.2) is the candidate and is not established. Not an abort, not a
verdict input.

## C4 — K1, does tiering dominate a single band?

`k1_analyse.py`, criterion `report/PREREG_K1_DOMINATION.md` §1. Medians of paired
differences, comparator − subject, positive = `ca_tbl_full` better.

| cell | vs `static_868`: rmse | energy | verdict | vs `static_2p4` |
|---|---|---|---|---|
| `k1a_box750` | +47.1 m, 18+/2−, p 0.0004 | +11.05 J, 20+/0− | **DOMINATES** | DOMINATED |
| `k1a_box1000` | +23.3 m, 11+/9−, p 0.82 | +10.44 J, 20+/0− | **DOMINATES** | DOMINATED |
| `k1a_box1500` | +22.4 m, 13+/7−, p 0.26 | +8.80 J, 20+/0− | **DOMINATES** | DOMINATED |
| `k1a_box2000` | −22.4 m, 8+/12−, p 0.50 | +0.38 J, 12+/8−, p 0.50 | tie | DOMINATED |
| `k1a_box3000` | −6.0 m, 10+/10− | −4.04 J, 4+/16−, p 0.012 | DOMINATED | DOMINATED |
| `k1b_box750` | +47.6 m, 19+/1− | −7.35 J, 0+/20− | incomparable | DOMINATED |
| `k1b_box1000` | +40.7 m, 17+/3−, p 0.0026 | −6.39 J, 0+/20− | incomparable | DOMINATED |
| `k1b_box1500` | +45.1 m, 15+/5−, p 0.041 | −6.37 J, 0+/20− | incomparable | DOMINATED |
| `k1b_box2000` | +9.6 m, 11+/9−, p 0.82 | −6.53 J, 0+/20− | DOMINATED | DOMINATED |
| `k1b_box3000` | −45.7 m, 9+/11−, p 0.82 | −9.54 J, 0+/20− | DOMINATED | DOMINATED |
| `k1c_brief` | −27.3 m, 8+/12−, p 0.50 | −5.07 J, 6+/14−, p 0.12 | tie | DOMINATED |

**P4.1 is falsified in three cells.** Per the registered falsifier, "tiering not
supported" is revised for `k1a_box750`, `k1a_box1000` and `k1a_box1500` and reported as an
envelope: in the `k1a` configuration (gated PCS, floor on demand, evidence gate,
corroboration, bidirectional digest) `ca_tbl_full` dominates `static_868` at `boxHalf`
750–1500, ties at 2000 and is dominated at 3000; in `k1b` (legacy, floor every cycle) it is
incomparable at 750–1500 and dominated at 2000–3000; `k1c_brief` ties. Per K1's own F1 the
sweep is not widened. **P4.2 reproduces in 11/11.**

**What the three DOMINATES verdicts are made of** (supplementary; per-cycle cost inside this
binary, `b3_energy_invariant.py`). On the single-tier arms a tier-1 cycle costs
18.5–24.0 mJ and a tier-2 cycle 5.5–8.6 mJ across the `k1a` cells, because tier-1 replies
are billed (`--tier1ChargeReplies`, default true since `4779bf7`; at seed 7 of
`k1a_box750`, `rx_868` is 22.4 J of `static_868`'s 34.6 J). `ca_tbl_full` spends 78–88 % of its cycles on
2.4 GHz. With floor-on-demand on (`k1a`) that time replaces tier-1 time and the arm is
cheaper than `static_868` on 20/20 seeds; with the floor running every cycle (`k1b`,
`docs/TECH_DEBT.md` K.3) it adds to it and the arm is dearer on 20/20. So the three cells
where tiering dominates are cells where escalating **leaves the more expensive tier**, which
is the opposite of the premise the energy axis was built to test — a cheap coarse tier and a
dear precise one. They are real under the registered criterion and billing; they are not
evidence that escalation to a precision tier pays, and `static_2p4` dominates `ca_tbl_full`
in the same cells.

Two further caveats. The criterion has no availability axis: `static_2p4` dominates while
fixing on a median 447–1 082 of 1 440 cycles against `ca_tbl_full`'s 1 236–1 395 (its
`rmseM` carries the dead-reckoned cycles, so the loss is only partly on the accuracy axis).
And `static_uwb` fixes on a median of 0 cycles in every K1 cell, so "DOMINATES
`static_uwb`" is a comparison against an arm that does not localise. Checked against every arm (rule 5):
against `fixed_threshold`, `ca_tbl_full` dominates at `k1a_box750` and `k1a_box1000`, is
incomparable at `k1a_box1500` and is dominated in the other eight cells.

## C5 — availability (C3's cells, no new runs)

`fixCycles / steps` on `ca_tbl_full`. Denominator: `steps` = 5 760 on every seed (every
2.5 s cycle of 14 400 s). A per-cycle ratio; no event straddles the window, so no
censoring rule applies.

| cell | mean | median | min–max |
|---|---|---|---|
| `wanonly_move` | 0.0407 | 0.0407 | 0.0243–0.0714 |
| `sparse_move` | 0.3191 | 0.2639 | 0.0453–0.8104 |
| `wanmesh_move` | **0.6473** | 0.7039 | 0.2535–0.8422 |

Ordering holds on the mean; `wanmesh_move − wanonly_move` is positive on **20/20** seeds,
p = 1.9×10⁻⁶. **P5.1 REPRODUCES.**

Every arm, checked (rule 5): the availability belongs to the mesh, not to tiering.
`static_868` reaches 0.628 in `wanmesh_move` and **0.451 in `sparse_move`, above
`ca_tbl_full`'s 0.319**; `static_2p4` reaches 0.000 / 0.023 / 0.051 and `static_uwb`
0.000 / 0.0001 / 0.0002 across the three cells.

## C6 — estimation at the gateway

| arm | nodeside − edge, totalJ | per-seed saving | lower under edge | Δ `j_868` | Δ `j_2p4` |
|---|---|---|---|---|---|
| `static_868` | +11.02 J | **42.5 %** (41.6–43.9) | 20/20, p 1.9×10⁻⁶ | +11.02 J | 0 |
| `fixed_threshold` | +10.10 J | **32.9 %** (29.9–35.5) | 20/20, p 1.9×10⁻⁶ | +10.30 J | −0.20 J |
| `ca_tbl_no_integrity` | +10.12 J | 32.9 % (30.2–35.3) | 20/20 | +10.32 J | −0.20 J |
| `ca_tbl_full` | +10.11 J | **32.9 %** (30.2–35.3) | 20/20, p 1.9×10⁻⁶ | +10.31 J | −0.20 J |
| `static_2p4` | −0.01 J | **−0.2 %** | **0/20** — dearer on 20/20 | 0 | −0.015 J |
| `static_uwb` | 0.00 J | 0 | 0/20, 20 exact ties | 0 | 0 |

(Δ = nodeside − edge; a negative Δ means edge costs more.)

**P6.2 REPRODUCES** on all three registered arms. **P6.1 REPRODUCES** as a control-flow
claim: the seven accuracy columns are byte-identical on 20/20 seeds on every arm, and the
only columns that differ anywhere are `totalJ`, `accuracyPerJoule` and the per-tier `j_`,
`rx_`, `idle_` energies. Per the registered reachability rule it is claimed on the five
arms whose energy moved and **not on `static_uwb`**, where it did not (the edge path runs
only on a fix, `ca_tbl_causal_sim.cc:5436`, and `static_uwb` has almost none).

**The saving is a tier-1 mechanism, and it costs on tier 2.** Under edge the node skips the
per-anchor tier-1 replies (`m_tier1ChargeReplies && !m_edgeServer`,
`ca-tbl-energy-model.cc:408`) and pays one downlink per fix on the active tier. Tier 2's
two-way ranging is unchanged, so on tier 2 edge is a pure addition: `static_2p4` is dearer
on 20/20 seeds (−0.2 %), and every policy arm pays ≈ 0.20 J more on 2.4 GHz while saving
≈ 10.3 J on 868 MHz. The size of the saving is the size of the reply bill that `4779bf7`
made the default; under the earlier no-reply billing the same mechanism could only add the
downlink, which is what `docs/TECH_DEBT.md` K.4 says.

## C7 — the IMU ceiling

**A ceiling: an ideal dead reckoner, an upper bound on what a flawless IMU could
contribute, never a system result.** Control: C3 `wanmesh_move`, same binary, seeds,
invocation without `--imuFusion=true --imuIdeal=true`. Positive = cep95 lower under the
ceiling.

| arm | control median / mean | ceiling median / mean | sign test |
|---|---|---|---|
| `static_uwb` | 8 413 / 8 851 m | **17** / 1 733 m | **20+/0−, p 1.9×10⁻⁶** |
| `static_2p4` | 27 151 / 35 305 m | **537** / 510 m | **20+/0−, p 1.9×10⁻⁶** |
| `ca_tbl_full` | 2 016 / 3 261 m | 1 664 / 1 808 m | 11+/9−, p 0.82 |
| `ca_tbl_no_integrity` | 1 467 / 2 878 m | 1 270 / 1 592 m | 12+/8−, p 0.50 |
| `fixed_threshold` | 1 472 / 2 429 m | 1 775 / 1 942 m | 10+/10−, p 1.00 |
| `static_868` | 2 076 / 3 266 m | 1 737 / 1 856 m | 10+/10−, p 1.00 |

**P7.1 REPRODUCES** on both low-availability arms. `ca_tbl_full`'s direction is lower on
median and mean and not significant.

**The ceiling is not dead reckoning alone** (rule 3, an unused quantity). `mac_tx` differs
between control and ceiling on 20/20 seeds on every arm, so the treatment changes what is
transmitted and therefore what is measured; `static_2p4`'s `fixCycles / steps` rises
0.051 → 0.162 under the ceiling. The mechanism of that rise is not established here.

**The disclosed seed-7 non-identity is resolved.** Regenerated rows against
`results/L2_imuon_s7` and siblings: 96/120 rows byte-identical, and the **only** differing
column is `rmse_mlat` (24 rows, 12 seeds) — the same column, and the same unchanged code,
as E3. Every column the IMU fusion can reach is identical; prereg §9's "some code path on
the IMU-fusion invocation moved" is, on this evidence, the snapshot solver's RMSE tail.

---

## What this changes

### In `CA-TBL_Technical_Report_SelfContained.md` (not edited here)

None of the seven B3 claims is stated in that report; its newest content predates them.
These are the statements in it that B3's evidence now contradicts or scopes:

1. **Scope banner (lines 15–37), restate.** "Every result below is provisional pending
   re-run" should name what was re-run: seven claim-bearing campaigns regenerated on one
   commit under the corrected topology and environment, with the verdicts in the table
   above. The report needs a section carrying them, including the two reversals.
2. **§1 and §2, "a confidence score (PCS) driving tier escalation", restate.** Measured in
   C2: under every repaired form PCS sits below every escalation threshold on ≥ 99.98 % of
   fix cycles, so it decides nothing and the downstream gates decide; under `legacy` it sits
   above threshold on 81–86 % and asks to escalate on the remaining 14–19 %.
3. **§5.2, Claim 8 ("falsified thinly"), restate.** Under the repaired forms
   `ca_tbl_no_integrity` is byte-identical to `fixed_threshold` on 19/20 seeds: not a thin
   loss but the same controller. In K1, `ca_tbl_full` dominates `fixed_threshold` at
   `k1a_box750` and `k1a_box1000`, is incomparable at `k1a_box1500`, and is dominated in the
   other eight cells. Claim 8 was not re-registered in B3; these are reported, not verdicts.
4. **§5.5, "escalation cannot fire however large the error becomes", scope.** In C2's
   population — legacy, propagation NLOS, `ca_tbl_no_integrity`, where PCS is the only
   trigger — the trigger is true on 3 321 of 23 213 fix cycles and the arm escalates 93.2
   times per 3 600 s on average. The zero-escalation observation belongs to its own
   population and does not carry to the corrected environment.
5. **§6.2, PCS saturation, extend.** "It can fail to contribute; it can never veto" still
   describes `legacy`. The repaired forms saturate the other way: with `traceSigma` at
   7.8×10⁴–1.2×10⁵ m², term 1 ≈ 10⁻⁴ and v2, weighted and gated sit below every threshold,
   so every fix cycle wants to escalate. The defect is the scale of term 1 against the
   filter it reads, not only the unweighted sum.
6. **§8.2, "A weighted or gated PCS … none has been run", withdraw.** All three have been
   run twice. In the regenerated campaign `gated` is significantly worse than `legacy` on
   cep50 (P2.1 SIGN FLIPS), "`gated` buys the same accuracy for a fifth of the energy" is
   withdrawn (P2.3), and the monotone-control negative holds for legacy, v2 and weighted and
   is undefined for gated.
7. **§9, "PCS is recorded as defective and left unfixed", restate** to say that opt-in
   repaired forms exist behind `--pcsForm` and that B3 measured them saturating low (item 5).
8. **Not changed by B3**, stated so that silence is not read as confirmation: §5.1 Claim 9
   (Mahalanobis rejection alone) was not re-tested — C1 is a different mechanism; §2's
   tier-2 `r80` of 1 149.5 m was not re-measured (`report/K1_DOMINATION_RESULTS.md` has
   156.0 m); §4.3 and §7 need their own registrations (prereg §9).

### Elsewhere (not edited here; corrections-register entries)

- `report/INTEGRITY_CHECK_RESULTS.md`: withdrawn in the shipped default configuration (C1);
  its sidecars' `deb52da` cannot be the binary's source. `docs/TECH_DEBT.md` L.3 row
  "supported, narrowly" → withdrawn; re-test with the evidence gate pinned off.
- `report/PCS_FORMS_RESULTS.md`: "`gated` beats `legacy`" → SIGN FLIPS; `gated` is
  byte-identical to `weighted` on 18/20 seeds. `docs/TECH_DEBT.md` K.8's "every form loses,
  in every cell" and its quoted gaps belong to the pre-correction binary; the regenerated
  record is: loses in 6 of 8 cells (weighted `no_integrity` −0.0984), undefined for gated.
- `report/K1_DOMINATION_RESULTS.md` and L.3 "tiering beats single-band — not supported":
  restate as the envelope in §C4, with the billing explanation.
- `report/K10_BASELINE_RESULTS.md` and L.3 "beats published estimators": reproduces on the
  corrected environment and may be cited from `results/regen27_k10`; K10's own qualifier
  stays.
- `report/K9_SAR_FACTORIAL_RESULTS.md` availability: reproduces (0.041 / 0.319 / 0.647).
- `docs/TECH_DEBT.md` K.4 "it can only cost energy": true for tier 2 and under no-reply
  billing; under the default billing it saves 32.9–42.5 % on every tier-1-using arm.
- `report/PREREG_B3_REGEN.md` §2 omits the `escalationRequiresEvidence` default flip, and
  §9's IMU disclosure is resolved to `rmse_mlat`. The registration is not amended after the
  fact; this report is the erratum.

### What would settle the open items

- C1 with `--escalationRequiresEvidence=false` pinned, registered first.
- The simulator's `source` line should describe the binary's tree, not the working
  directory's (C1 provenance defect); until then, launch only from the tree that built the
  binary, as B3 did.
- A registered PCS campaign cannot be informative while `traceSigma` sits four orders of
  magnitude above `traceRef`: every form saturates, legacy high and the repaired forms low.
  That is the order `docs/TECH_DEBT.md` K.8 already gives — the control first, then
  calibration.
