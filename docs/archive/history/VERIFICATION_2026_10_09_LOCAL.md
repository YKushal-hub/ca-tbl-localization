# Local verification, 2026-10-09: `eadcb0b3`

This is the run that `LOCAL_RUN_2026_10_09.md` asks for, steps 1-6, done on the owner's local
machine with no contact with the cloud session. Every simulator run wrote to a temp directory
(`/tmp/lv_*`). Nothing in this file changes code, a preset or a registration.

**Commit verified:** `V = eadcb0b3cf52eba707acd0e1212bf86e7223c64c`
(`origin/work/2026-10-09-acc-part-a`, fetched 2026-10-09), in a new worktree,
`~/catbl_localverify`.

**Machine:**
- 12th Gen Intel(R) Core(TM) i5-12450H, `nproc` 12, 7.6 GiB RAM, WSL2 (kernel
  6.18.40.1-microsoft-standard-WSL2).
- Compiler: `g++-11 (Ubuntu 11.5.0-10ubuntu1~20~ppa3) 11.5.0`. The system default `gcc`/`g++` is
  9.4.0. **One deviation from Step 1:** configure ran as
  `CC=gcc-11 CXX=g++-11 ./ns3 configure --enable-examples --build-profile=optimized`. The
  null-control reference `~/catbl_ref_spec0` (e6b3da9) was built with `/usr/bin/g++-11`
  (its `PROVENANCE.txt`). Building with the same compiler is what lets Step 2 use it without a
  rebuild. Step 1 does not name a compiler.

## Summary

| step | result |
|---|---|
| 1 build | **pass**: configure rc 0, `scratch/ca_tbl_causal_sim` built rc 0 (then all targets, rc 0, for the suite) |
| 2 null control | NC1-NC5 and NC7-NC9 IDENTICAL. **NC6** differs in 4 traces, by at most 4.5e-15 relative, in `estXhp`/`estYhp` only. **The same difference is present on S1's `1596cb41`**: all 59 output files are byte-identical between the two. It predates S3 and Part A |
| 3 G120, G100 | **pass**: G120 7/7, self-test 10 as expected. G100 5/5, self-test 6 as expected |
| 4 full suite | 81 PASS, 9 FAIL, tree unchanged. The five expected reds (G4, G5, G6, G11, G24) plus **G10, G63, G87, G89**. G100 and G120 PASS. **G10 is new in `V`** (it passes on `1596cb41`). **G63, G87 and G89 also fail on `1596cb41`**, with the same check lines. See §4 |
| 5 k sweep | 80 cells (10 seeds × 4 k × A/B; each cell runs both arms, so 160 arm rows), **80 of 80 exited 0**. **In B, k = 10 has the lowest CEP95 of {1, 3, 10, 30} for both arms. In A it does not hold for `ca_tbl_full`**: its mean CEP95 rises 61.23 → 76.22 m (k = 1 → 10), mostly from one seed |

## Step 2: null control, every new flag off

```
NC_REF_DIR=~/catbl_ref_spec0/nc NC_OUT_ROOT=/tmp/lv_nc NC_CWD=~/catbl_localverify \
  bash ~/catbl_ref_spec0/nc.sh "$PWD/build/scratch/ns3.47-ca_tbl_causal_sim-optimized" localverify
```

I set `NC_OUT_ROOT` so outputs went to a temp directory, and `NC_CWD` so `run_id.txt` reads the
verified worktree. Neither changes what is compared.

| run | identical | different |
|---|---|---|
| NC1 | 7 | 0 |
| NC2 | 2 | 0 |
| NC3 | 2 | 0 |
| NC4 | 3 | 0 |
| NC5 | 3 | 0 |
| **NC6** (`--seed=7 --duration=900 --trace=true --tier1Tdoa=true`) | 9 | **4** |
| NC7 | 2 | 0 |
| NC8 | 2 | 0 |
| NC9 | 7 | 0 |

`NULL CONTROL: FAIL` (nc.sh exit 1), on NC6 alone. NC6's `arm_summary.csv` is IDENTICAL.

| NC6 file differing | cells | rows | columns | max relative difference |
|---|---|---|---|---|
| `trace_ca_tbl_full_seed7.csv` | 6 | 163, 192, 215, 270, 278, 285 | `estXhp`, `estYhp` | 3.27e-15 |
| `trace_ca_tbl_no_integrity_seed7.csv` | 6 | 163, 192, 215, 270, 278, 285 | `estXhp`, `estYhp` | 3.27e-15 |
| `trace_fixed_threshold_seed7.csv` | 10 | 79, 82, 176, 182, 183, 184, 265, 282, 289, 309 | `estXhp`, `estYhp` | 3.87e-15 |
| `trace_static_868_seed7.csv` | 8 | 92, 93, 107, 114, 178, 188, 223, 225 | `estXhp`, `estYhp` | 4.47e-15 |

The largest absolute difference is 1.02e-12 m. On this machine the difference is in **NC6**,
not in NC4 as the cloud saw (NC4 is IDENTICAL here). It is the same kind of difference: a few
cells of the high-precision position columns, about one ulp (TECH_DEBT 4.1).

**Comparison with S1's `1596cb41`, as Step 2 asks.** The binary is from S1's suite worktree
`~/catbl_suite`. It was checked out detached at `1596cb41`, built with the same g++-11, and ninja
had nothing to relink: `1596cb41` differs from the `e308c14f` already built there only in
`HANDOFF_2026_10_05.md`. I ran the same nc.sh with tag `localverify_s1` (`NC_CWD=~/catbl_suite`).
- Its verdict is the same: NC6 alone, the same 4 traces, the same first differing bytes
  (43787, 44765, 21269, 24410).
- `cmp` of `/tmp/lv_nc/localverify` against `/tmp/lv_nc/localverify_s1`: the same 59-file set,
  and **every output file under NC1-NC9 is byte-identical**.
- The only files that differ are the nine `NC*.stdout`, and only in the outdir path they print.
  After normalising the path, 0 lines differ.

**Diagnosis:** the NC6 difference predates S3 and Part A. With every new flag off, `V` and
`1596cb41` produce identical outputs. This matches S1's own record: NC6 traces already differed
by about one ulp at `a0829213` (S1 session, 2026-10-08).

## Step 3: the two new gates on their own

```
bash scripts/tof_field_gate.sh  /tmp/lv_g120 && bash scripts/tof_field_gate_selftest.sh /tmp/lv_g120
bash scripts/acc_a_gate.sh      /tmp/lv_g100 && bash scripts/acc_a_gate_selftest.sh     /tmp/lv_g100
```

```
  G120 ok      RUNS    6 runs exited 0
  G120 ok      DEFAULT 3 files; differing []; only one side []; traces 2 (need 2)
  G120 ok      COV     480 rows carry the old row unchanged plus 3 columns; other files differing []; problems []
  G120 ok      TRACE   pxx+pyy != traceSigma on 0 rows []; not PSD on 0 []
  G120 ok      REACH   rows with pxy != 0: 407 (need > 0); control header without pxx: True
  G120 ok      SIGMA   6 tier-2 fix rows, 0 with meanSigmaRawM < 83.6 []; control: 6 rows, max 9.63407958739
  G120 ok      BIAS    16 delivered 2.4 GHz rows, 16 with control mae < 500.0; problems []
  G120: 7 ok, 0 failed, 0 missing input
  G120 selftest: 10 as expected, 0 not          (P0 and N1-N9 each as expected)

  G100 ok      RUNS    3 runs exited 0
  G100 ok      DEFAULT 59 files; differing []; only one side []
  G100 ok      TAG0    median trace growth per cycle without a fix 0.0096 m^2 at 1, 0.0974 at 10 (ratio 10.15, need >= 5)
  G100 ok      SUBJ    median trace growth per cycle without a fix 0.3909 m^2 at 1, 5.976 at 10 (ratio 15.29, need >= 5)
  G100 ok      REACH   cycles without a fix (at 1, at 10): [('TAG0', 55, 57), ('SUBJ', 904, 895)] (need >= 20 each)
  G100: 5 ok, 0 failed, 0 missing input
  G100 selftest: 6 as expected, 0 not           (P0 and N1-N5 each as expected)
```

Both match the expected counts, and both gates match the cloud's commit message for `81fb1485`
(×10.2 tag 0, ×15.3 subjects). The suite's own run of G100 in Step 4 printed the same numbers.

## Step 4: the full suite

```
git status --short > /tmp/lv_before.txt
TMPDIR=/tmp/lv_suite_tmp bash scripts/catbl_gates.sh > /tmp/lv_suite.log 2>&1   # suite exit 1
git status --short > /tmp/lv_after.txt; diff ... && echo "tree unchanged"         # tree unchanged
```

`TMPDIR` was set to a fresh temp directory. **81 PASS, 9 FAIL; the tree is unchanged.**

| gate | verdict | expected? | on `1596cb41` |
|---|---|---|---|
| G4, G5 | FAIL | yes (K.6) | — |
| G6 | FAIL BY DESIGN | yes | — |
| G11 | FAIL | yes (J.1) | — |
| G24 | FAIL BY FINDING | yes (N.15) | — |
| **G10** | FAIL | **no** | **PASS** (0 absent, 93 documents) |
| **G63** | FAIL | **no** | **FAIL**, same three check lines |
| **G87** | FAIL | **no** | **FAIL**, same check line |
| **G89** | FAIL | **no** | **FAIL**, same three check lines |
| G100, G120 | PASS | must pass | (not in `1596cb41`) |

I re-ran the four unexpected reds' own scripts in `~/catbl_suite` at `1596cb41`. Its tree was
unchanged afterwards. The output is in `~/catbl_lv/s1_gates.log` on this machine.

### G10, new in `V`: six citations to files that do not exist

```
citation audit over 97 source documents (97 tracked, 0 NOT), 263 distinct cited paths
  TRACKED 172, TRACKED_ELSEWHERE 74, ALLOWED 11, UNTRACKED_BUT_EXISTS 0, ABSENT 6
  UNRESOLVED: `context.md` (ABSENT from disk) <- PREREG_ACC_A_FILTER.md
  UNRESOLVED: `gateways.csv` (ABSENT from disk) <- TIER1_VALIDATION.md
  UNRESOLVED: `report/ACC_A_RESULTS.md` (ABSENT from disk) <- PREREG_ACC_A_FILTER.md
  UNRESOLVED: `report/TOF_FIELD_RESULTS.md` (ABSENT from disk) <- PREREG_TOF_FIELD.md
  UNRESOLVED: `spec_profile.txt` (ABSENT from disk) <- PREREG_ACC_A_FILTER.md
  UNRESOLVED: `survey/samples.csv` (ABSENT from disk) <- TIER1_VALIDATION.md
```

On `1596cb41` the same audit passes: 93 documents, 245 paths, ABSENT 0.

**Diagnosis:** all six come from documents added after `1596cb41`, S2's and S3's (S2's
`PREREG_ACC_A_FILTER.md`, S3's `PREREG_TOF_FIELD.md` and `TIER1_VALIDATION.md`):
- two are results files that are not written yet;
- `context.md` lives on S2's cloud branch;
- `spec_profile.txt` is a per-run output name;
- the two data files are the external dataset's (not in the repository).

Each needs either an `ALLOWED` entry in `scripts/citation_audit.py` or a resolvable path. That
belongs to S2 or S3, not to this branch.

### G63, G87, G89: already red on `1596cb41`, from S1's preset change `e308c14f`

```
  G63 FAIL    UNION   rows checked 1186; bad 52 (need 0): [('end', '310.0000', '5', 0, 1, 1, '2.4GHz'), ...]
  G63 FAIL    KIND    fix kinds that do not fit the set: 27 (need 0): [('end', '460.0000', '1', 1, 1, 1), ...]
  G63 FAIL    TRACE   tag-0 rows matched to their trace row 314; bad 4 (need 0): [('end', '457.5000', ('UWB', 'UWB', '1', '5')), ...]
  G87 FAIL    LABEL   cycles labelled above 868 MHz 260 (need > 0); labelled with a tier that had no neighbour: 3 (need 0): [('412.5000', 'UWB'), ('430.0000', 'UWB')]
  G89 FAIL    OFF     runs with every flag off that print an N.26 line: ['e_off', 'k_off'] (need none)
  G89 FAIL    DUAL    node-cycles 1800.0 (need 360 x 5); cycles on 868 MHz with 2.4 GHz neighbours 213 (off: 213, need > 0 and 0); fused 2.4 GHz fixes 193 (need > 0)
  G89 FAIL    CROSS   cross-tier polls 70.0 (need > 0); UWB cycles polling beyond the UWB table 8 (off: 8, need > 0 and 0)
```

**Where the failure appears:**
- These lines are identical in this suite, in the re-run on `1596cb41`, and in S1's own full
  suite on `e308c14f` (run on this machine on 2026-10-08, 77 PASS, logged in
  `~/catbl_tmp_gates/fixes/suite.log`).
- All three gates passed on row 17's tree (`d7ada989`): 80 PASS, expected reds only.

**Diagnosis:**
- `e308c14f` put `--loraDualBase`, `--uwbCrossTierPoll`, `--tdoaInnovationGate`, `--mahalUseR`
  and `--integrityChi2=1992.9` into `--specProfile`. These three gates run their "off" or
  control cells on the preset, so those cells now carry the fixes. For example, G89's
  every-flag-off runs print an N.26 line, and its control shows the same 213 / 8 as the
  treatment. G63's and G87's fused sets and labels now include the dual base's fused 2.4 GHz
  and UWB.
- The gates' assumptions predate the preset change. They need either explicit flag-off
  controls or re-based expectations. That is S1's to fix, not this branch's.

**Correction to `LOCAL_RUN`'s premise.** "S1's evidence commit reports 80 PASS" (`1596cb41`) is
the suite on **row 17's** tree (`d7ada989`), from before the preset change. It is not the suite
on `1596cb41`'s own tree. On that tree the suite is 77 PASS with G63, G87 and G89 red, and this
file is the first written record of that.

## Step 5: exploratory `--imuQuScale` sweep, tuning seeds 21-30 (this machine only)

**Exploratory. Tuning seeds, this machine only.** Not paired with any cloud cell. Nothing was
chosen or changed because of it. The registered ACC-A campaign stands as registered.

The command is the one in `LOCAL_RUN_2026_10_09.md` Step 5, verbatim, run from the verified
worktree (`OUT=/tmp/lv_ksweep.Vff0`, `xargs -P 12`).

| cfg | k | arm | n | mean CEP50 (m) | mean CEP95 (m) |
|---|---|---|---|---|---|
| A | 1 | ca_tbl_full | 10 | 22.74 | 61.23 |
| A | 1 | static_2p4 | 10 | 15.43 | 50.35 |
| A | 3 | ca_tbl_full | 10 | 22.97 | 76.93 |
| A | 3 | static_2p4 | 10 | 14.01 | 44.84 |
| A | 10 | ca_tbl_full | 10 | 21.92 | 76.22 |
| A | 10 | static_2p4 | 10 | 13.67 | 40.73 |
| A | 30 | ca_tbl_full | 10 | 15.67 | 76.32 |
| A | 30 | static_2p4 | 10 | 14.36 | 58.08 |
| B | 1 | ca_tbl_full | 10 | 5.99 | 13.78 |
| B | 1 | static_2p4 | 10 | 4.75 | 10.81 |
| B | 3 | ca_tbl_full | 10 | 4.08 | 10.15 |
| B | 3 | static_2p4 | 10 | 3.77 | 8.65 |
| B | 10 | ca_tbl_full | 10 | 2.96 | **7.33** |
| B | 10 | static_2p4 | 10 | 3.07 | **7.72** |
| B | 30 | ca_tbl_full | 10 | 2.60 | 8.81 |
| B | 30 | static_2p4 | 10 | 2.48 | 9.93 |

The columns are `arm_summary.csv`'s `cep50M` and `cep95M`, averaged over seeds 21-30, as the
Step 5 script prints them. The run took 22:59:08Z-23:13:19Z (2026-10-08 UTC).

**Cells.** `LOCAL_RUN` says "out of 160", but its loop makes **80 cells** (10 seeds × 4 k × A/B),
and each runs both arms. **80 of 80 exited 0**, which gives the 160 arm rows above, 10 per line.

**The one statement Step 5 asks for:**
- **B:** k = 10 has the lowest mean CEP95 of {1, 3, 10, 30} for both arms: 7.33 m for
  `ca_tbl_full` and 7.72 m for `static_2p4`.
- **A:** k = 10 does **not** leave A unworsened for `ca_tbl_full`. Its mean CEP95 is 76.22 m
  against 61.23 m at k = 1 (+15.0 m). For `static_2p4` it improves, 50.35 → 40.73 m.
- **Per seed, the A rise is not broad.** `ca_tbl_full`'s paired k = 10 minus k = 1 CEP95 is
  [−18.4, +9.9, −33.2, **+162.8**, −18.4, +40.7, −0.1, +13.0, −3.7, −2.9] over seeds 21-30. It
  is worse on 4 of 10, with median −1.47 m; seed 24 carries most of the mean.
- k = 3 and k = 30 give the same A mean (76.93, 76.32).

Ten seeds, no intervals, one machine. This is a tuning observation, not a result. No k is
chosen here.

## Files on this machine (not tracked)

- `/tmp/nc_localverify.log`, `/tmp/nc_localverify_s1.log`, `/tmp/lv_nc/`: Step 2.
- `/tmp/lv_g120/`, `/tmp/lv_g100/`, `~/catbl_lv/g1{20,00}.log`: Step 3.
- `/tmp/lv_suite.log`, `/tmp/lv_before.txt`, `/tmp/lv_after.txt`, `~/catbl_lv/s1_gates.log`: Step 4.
- `/tmp/lv_ksweep.Vff0`, `~/catbl_lv/ksweep.log`: Step 5.
