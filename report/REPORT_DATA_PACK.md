# CA-TBL — Report Data Pack

Extraction only. No conclusions drawn, no prose rewritten, no edits to
`CA-TBL_Technical_Report_SelfContained.md`. Numbers that could not be produced are
marked `NOT AVAILABLE` with the reason.

Generated 2026-08-10. Pre-registration governing these results: `b49df46` (2026-08-09
22:35) and amendment `a059203` (2026-08-09 23:58).

---

## §1 — Provenance for every result directory quoted

| dir | rid | source sha | dirty | profile | digest |
|---|---|---|---|---|---|
| `results/p10_plane/*` | `67c16a40c7279f04` (seed 1) | `a059203` | 3 | optimized | yes |
| `results/p10_plane_fix/*` | `4eeadc1975b121d2` (seed 1) | `a059203` | 3 | optimized | yes |
| `results/baseline_v6` | `bd99a6548154dc85` | NOT AVAILABLE | NOT AVAILABLE | NOT AVAILABLE | no |
| `results/causal` (extended probe) | `1bd305afcc3f9144` | `a2c32f524` | 0 | optimized | yes |
| envelope sweep | NOT AVAILABLE | | | | |

Full invocations (line 2 of each `run_id.txt`):

```
p10_plane/seed_1      ...ca_tbl_causal_sim-optimized --seed=1 --duration=3600 --boxHalf=2000 --gwGrid=5 --trace=true --outdir=results/p10_plane/seed_1
p10_plane_fix/seed_1  ...ca_tbl_causal_sim-optimized --seed=1 --duration=3600 --boxHalf=2000 --gwGrid=5 --escalationRequiresEvidence=true --trace=true --outdir=results/p10_plane_fix/seed_1
baseline_v6           ...ca_tbl_causal_sim-optimized --seed=7 --duration=3600 --nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=legacy --trace=true --outdir=results/v6_ladder/D_prop_arb
causal                ...ca_tbl_causal_sim-optimized --seed=7 --duration=3600 --outdir=results/causal
```

**Reportable gaps.**
- `results/baseline_v6/run_id.txt` predates the `source`/`profile` fields and the
  digest block. It carries only the rid and the invocation. Its recorded `--outdir`
  is `results/v6_ladder/D_prop_arb`, i.e. the directory is a **copy** of that run.
- The envelope sweep (§8) was never launched, so no directory exists.
- `dirty 3` on both plane directories: three tracked files were modified at run time.

---

## §2 — Seed generation and what a seed controls

### 2.1 `RngSeedManager`

| call | value | file:line |
|---|---|---|
| `RngSeedManager::SetSeed(seed)` | the `--seed` argument | `scratch/ca_tbl_causal_sim.cc:1821` (`RunArm`), `:1086` (`RunProbe`) |
| `RngSeedManager::SetRun(1)` | constant `1` | `scratch/ca_tbl_causal_sim.cc:1822`, `:1087` |
| `RngSeedManager::ResetNextStreamIndex()` | — | `scratch/ca_tbl_causal_sim.cc:1848` (`RunArm`), `:1090` (`RunProbe`) |

`SetRun` is fixed at 1; the seed is the only varied input.

### 2.2 `ResetNextStreamIndex()` — why it is present

Quoted verbatim from `scratch/ca_tbl_causal_sim.cc:1823-1847`:

> WITHOUT THIS LINE NO TWO ARMS SHARE A CHANNEL REALISATION, AND NONE EVER DID.
>
> `LoRaDistributionSampler`'s constructor takes a `uint32_t` seed and never uses it
> (`lora-distribution-sampler.cc:8`). It creates five `CreateObject<...RandomVariable>()`
> objects with no `SetStream()` call anywhere in the file, so each one takes the next
> automatic stream index. `RngSeedManager::SetSeed()` does NOT reset that counter --
> `ResetNextStreamIndex()` is a separate call (`rng-seed-manager.cc:71` vs `104`) -- so
> arm 6's engines begin where arm 5's engines stopped, and every arm draws from
> different substreams.
>
> Measured: `ca_tbl_full` and `ca_tbl_no_integrity`, identical EKF state and identical
> poll counts, reached **334 vs 336** engine evaluations by t=2.5 -- the first cycle,
> before either arm's policy had executed a single decision. At t=7.5 the same six
> anchors returned 6 decodes on one arm and 2 on the other.
>
> This invalidated every cross-arm comparison the project has made, including the
> integrity ablation, whose two arms differ by 8.1% RMSE while the check itself fires
> zero times.

The `364` figure requested does not appear in the source at this location.
`NOT AVAILABLE` — only `334` and `336` are recorded.

> NOT SUFFICIENT ON ITS OWN. Two file-scope caches also survive across arms --
> `g_fadingComponents` (`lora-fading-submodule.cc:6`) and `g_foliageDynamicCache`
> (`lora-foliage-submodule.cc:8`) -- and the second stores a `Time`, which
> `Simulator::Destroy()` resets to zero underneath it.

### 2.3 Stochastic components

| component | stream | file:line |
|---|---|---|
| gateway / anchor / facility placement | **deterministic**, no RNG — closed-form from `boxHalf`, `gwGrid`, ring radii | `ca_tbl_causal_sim.cc:252-320` |
| mobility (`Tourist` heading noise, target choice) | explicitly-streamed `RandomVariableStream` | `ca_tbl_causal_sim.cc:744-885` |
| channel realisation per link (shadowing, fading, Markov regime) | `LoRaDistributionSampler`, five auto-indexed streams per engine | `lora-distribution-sampler.cc:8` |
| NLOS determination | 4-state Markov regime inside the engine | see §2.4 note |
| packet loss / PDR | same engine draws | `lora-distribution-sampler.cc` |
| EKF process + measurement noise | **deterministic** — the EKF adds no noise; `processNoiseStdPos/Vel` scale `Q` analytically | `ca-tbl-policy.cc:197-204` |

`nNlos` vs `hiddenChannelState`: the trace carries three separate columns —
`nNlos` (the operative flag the policy consumed), `nNlosPropagation`, and
`nHiddenChannel`. Measured populations at `boxHalf=1000`, seed 7: operative 75.8%,
propagation 4.5%, hiddenChannel 74.6%. The operative flag tracks the **hidden-channel
Markov state**, not the propagation-derived NLOS condition.

### 2.4 What is common across arms at a given seed — **and what is not**

**The six arms do NOT see identical channel realisations.** Quoted verbatim from
`scratch/ca_tbl_causal_sim.cc:1765-1782`:

> The engines seed a private `LoRaDistributionSampler` from `config.randomSeed`, so all
> arms read the SAME STREAM -- but they consume it at arm-dependent RATES, because
> which tier is polled, how many anchors are addressed, and (since the MAC was wired)
> how much beacon traffic is evaluated all depend on the arm's own behaviour. Arm A's
> tenth draw and arm B's tenth draw are the same number; they are not applied to the
> same link at the same time.
>
> Consequence, and it is not small: a cross-arm RMSE difference has ALWAYS carried a
> channel-draw component, so no single-seed comparison between arms isolates the
> policy. The 20-seed sweep is not a refinement of the single-seed table, it is the
> minimum evidence for any claim that compares two arms.

What IS common at a given seed: the deployment geometry (deterministic), the
trajectory (explicitly streamed, `ca_tbl_causal_sim.cc:1767` — "every arm walks the
IDENTICAL TRAJECTORY"), and the automatic stream assignment per arm (after
`ResetNextStreamIndex`). What is NOT common: the alignment of channel draws to links
and times.

### 2.5 Seed range

20 seeds, `--seed=1` … `--seed=20`, contiguous. One process per seed
(`scripts/p10_plane.sh` (preserved as-run from `~/p10_plane.sh`; see `scripts/p10_plane.sh.PROVENANCE.md`), `seq 1 20`).

### 2.6 Determinism evidence

`results/det_a` vs `results/det_b` — G2 of `scripts/catbl_gates.sh`, two runs of the
same binary at the same seed, `diff` of `arm_summary.csv`. Result: **PASS** on the
2026-08-09 16:13 suite. `results/det_v6a` / `det_v6b` exist as the stored pair.
Additionally measured this session: two **independently rebuilt** binaries produced
byte-identical traces across all six arms.

---

## §3 — Deployment plane at `boxHalf=2000`, all six arms, 20 seeds

**Population and denominator.** Availability = (cycles with `hadFix == 1` AND
`errM <= 25 m`) / **all 1440 cycles of the run**. A no-fix cycle counts against
availability. `cep50M` and `rmseM` are over fix-cycles only. 20 seeds, contiguous 1-20.
Source: `results/p10_plane/` (before) and `results/p10_plane_fix/` (after).


### 3.1 BEFORE — no predicate

| arm | avail_mean | avail_min | avail_max | seeds_meeting_spec | cep50M | rmseM | outage | totalJ |
|---|---|---|---|---|---|---|---|---|
| `static_868` | 0.0691 | 0.0437 | 0.0958 | 0/20 | 100.60 | 135.92 | 0.0% | 14.81 |
| `static_2p4` | 0.9426 | 0.7451 | 0.9896 | 14/20 | 2.73 | 4.01 | 5.7% | 9.68 |
| `static_uwb` | 0.0551 | 0.0361 | 0.0785 | 0/20 | 0.16 | 4.49 | 94.4% | 769.45 |
| `fixed_threshold` | 0.9691 | 0.8840 | 0.9924 | 18/20 | 3.56 | 9.91 | 0.8% | 255.06 |
| `ca_tbl_no_integrity` | 0.9756 | 0.9111 | 0.9938 | 19/20 | 4.89 | 9.61 | 0.2% | 622.94 |
| `ca_tbl_full` | 0.9742 | 0.9111 | 0.9979 | 18/20 | 4.79 | 9.69 | 0.2% | 622.92 |

### 3.2 AFTER — `--escalationRequiresEvidence=true`

| arm | avail_mean | avail_min | avail_max | seeds_meeting_spec | cep50M | rmseM | outage | totalJ |
|---|---|---|---|---|---|---|---|---|
| `static_868` | 0.0691 | 0.0437 | 0.0958 | 0/20 | 100.60 | 135.92 | 0.0% | 14.81 |
| `static_2p4` | 0.9426 | 0.7451 | 0.9896 | 14/20 | 2.73 | 4.01 | 5.7% | 9.68 |
| `static_uwb` | 0.0551 | 0.0361 | 0.0785 | 0/20 | 0.16 | 4.49 | 94.4% | 769.45 |
| `fixed_threshold` | 0.9828 | 0.8889 | 0.9986 | 18/20 | 3.09 | 6.54 | 0.9% | 22.77 |
| `ca_tbl_no_integrity` | 0.9903 | 0.9097 | 1.0000 | 18/20 | 2.69 | 6.33 | 0.3% | 24.24 |
| `ca_tbl_full` | 0.9917 | 0.9097 | 1.0000 | 19/20 | 2.69 | 6.04 | 0.3% | 24.26 |

### 3.3 Per-seed availability, BEFORE

| seed | `static_868` | `static_2p4` | `static_uwb` | `fixed_threshold` | `ca_tbl_no_integrity` | `ca_tbl_full` |
|---|---|---|---|---|---|---|
| 1 | 0.0486 | 0.9840 | 0.0424 | 0.9826 | 0.9667 | 0.9667 |
| 2 | 0.0660 | 0.9833 | 0.0521 | 0.9826 | 0.9812 | 0.9812 |
| 3 | 0.0597 | 0.9576 | 0.0639 | 0.9924 | 0.9924 | 0.9972 |
| 4 | 0.0757 | 0.9792 | 0.0431 | 0.9771 | 0.9882 | 0.9882 |
| 5 | 0.0535 | 0.9431 | 0.0583 | 0.9778 | 0.9667 | 0.9667 |
| 6 | 0.0958 | 0.9549 | 0.0361 | 0.9868 | 0.9917 | 0.9917 |
| 7 | 0.0729 | 0.9549 | 0.0542 | 0.9917 | 0.9931 | 0.9979 |
| 8 | 0.0639 | 0.8965 | 0.0417 | 0.9514 | 0.9590 | 0.9590 |
| 9 | 0.0528 | 0.9646 | 0.0597 | 0.9785 | 0.9715 | 0.9715 |
| 10 | 0.0604 | 0.9736 | 0.0569 | 0.9569 | 0.9674 | 0.9674 |
| 11 | 0.0639 | 0.9896 | 0.0681 | 0.9708 | 0.9757 | 0.9764 |
| 12 | 0.0840 | 0.9285 | 0.0444 | 0.9653 | 0.9840 | 0.9840 |
| 13 | 0.0437 | 0.7764 | 0.0410 | 0.9361 | 0.9563 | 0.9451 |
| 14 | 0.0826 | 0.7451 | 0.0625 | 0.8840 | 0.9111 | 0.9111 |
| 15 | 0.0944 | 0.9847 | 0.0653 | 0.9854 | 0.9938 | 0.9938 |
| 16 | 0.0472 | 0.9667 | 0.0486 | 0.9806 | 0.9868 | 0.9903 |
| 17 | 0.0833 | 0.9465 | 0.0785 | 0.9625 | 0.9660 | 0.9660 |
| 18 | 0.0750 | 0.9778 | 0.0500 | 0.9556 | 0.9847 | 0.9549 |
| 19 | 0.0958 | 0.9826 | 0.0694 | 0.9840 | 0.9875 | 0.9875 |
| 20 | 0.0625 | 0.9625 | 0.0660 | 0.9806 | 0.9882 | 0.9882 |

### 3.4 Per-seed availability, AFTER

| seed | `static_868` | `static_2p4` | `static_uwb` | `fixed_threshold` | `ca_tbl_no_integrity` | `ca_tbl_full` |
|---|---|---|---|---|---|---|
| 1 | 0.0486 | 0.9840 | 0.0424 | 0.9917 | 0.9965 | 0.9972 |
| 2 | 0.0660 | 0.9833 | 0.0521 | 0.9889 | 0.9986 | 0.9986 |
| 3 | 0.0597 | 0.9576 | 0.0639 | 0.9931 | 0.9993 | 0.9993 |
| 4 | 0.0757 | 0.9792 | 0.0431 | 0.9931 | 0.9972 | 0.9972 |
| 5 | 0.0535 | 0.9431 | 0.0583 | 0.9903 | 0.9993 | 0.9993 |
| 6 | 0.0958 | 0.9549 | 0.0361 | 0.9896 | 0.9993 | 0.9993 |
| 7 | 0.0729 | 0.9549 | 0.0542 | 0.9889 | 0.9986 | 0.9993 |
| 8 | 0.0639 | 0.8965 | 0.0417 | 0.9806 | 0.9868 | 0.9868 |
| 9 | 0.0528 | 0.9646 | 0.0597 | 0.9986 | 1.0000 | 1.0000 |
| 10 | 0.0604 | 0.9736 | 0.0569 | 0.9965 | 0.9993 | 0.9993 |
| 11 | 0.0639 | 0.9896 | 0.0681 | 0.9951 | 0.9986 | 1.0000 |
| 12 | 0.0840 | 0.9285 | 0.0444 | 0.9826 | 0.9958 | 0.9958 |
| 13 | 0.0437 | 0.7764 | 0.0410 | 0.9125 | 0.9326 | 0.9590 |
| 14 | 0.0826 | 0.7451 | 0.0625 | 0.8889 | 0.9097 | 0.9097 |
| 15 | 0.0944 | 0.9847 | 0.0653 | 0.9972 | 0.9993 | 0.9993 |
| 16 | 0.0472 | 0.9667 | 0.0486 | 0.9903 | 1.0000 | 0.9972 |
| 17 | 0.0833 | 0.9465 | 0.0785 | 0.9931 | 0.9972 | 0.9972 |
| 18 | 0.0750 | 0.9778 | 0.0500 | 0.9951 | 0.9993 | 0.9993 |
| 19 | 0.0958 | 0.9826 | 0.0694 | 0.9965 | 1.0000 | 1.0000 |
| 20 | 0.0625 | 0.9625 | 0.0660 | 0.9931 | 0.9993 | 0.9993 |

### 3.5 AFTER — deployment breakdown

- seeds where `static_2p4` fails and `ca_tbl_full` meets spec: [5, 8, 12, 13, 17]
- seeds where both fail: [14]

| arm | worst-case availability | seed |
|---|---|---|
| `static_868` | 0.0437 | 13 |
| `static_2p4` | 0.7451 | 14 |
| `static_uwb` | 0.0361 | 6 |
| `fixed_threshold` | 0.8889 | 14 |
| `ca_tbl_no_integrity` | 0.9097 | 14 |
| `ca_tbl_full` | 0.9097 | 14 |


---

## §4 — Energy diagnosis, `ca_tbl_full`, before and after

Mean over 20 seeds. Source: `results/p10_plane/`, `results/p10_plane_fix/`.


| quantity | BEFORE | AFTER |
|---|---|---|
| UWB occupancy (% cycles) | 79.5% | 4.8% |
| fixes sourced from UWB (%) | 5.6% | 3.9% |
| escalations per run | 216.2 | 6.8 |
| demotions per run | 214.4 | 5.8 |
| usedFloor firings | 1106.6 | 84.5 |
| cep50M | 4.79 | 2.69 |
| totalJ | 622.92 | 24.26 |
| availability | 0.9742 | 0.9917 |
| seeds meeting spec | 18/20 | 19/20 |

- escalation efficiency (fixes from UWB / UWB occupancy): BEFORE 0.0708, AFTER 0.8270
- joules attributable to the defect: 622.92 - 24.26 = **598.66 J** (96.1% of the before-total); ratio 25.68x


Per-tier joule split (`of which 868 / 2.4GHz / UWB`): **NOT AVAILABLE** from the
trace `cumJ` column, which is a scalar total. The per-tier breakdown exists in
`arm_summary.csv` but under column names this extraction did not resolve; it was not
inferred.

---

## §5 — Claim 3, against the simulator's own definition

**Canonical definition**, verbatim from `scratch/ca_tbl_causal_sim.cc:700-706`:

> Trigger scoring against the geometric ground truth (claim 3).
> "Should be on UWB" is defined as `visUwb >= 3`: the tier is geometrically solvable.
> Confusion counts are accumulated per cycle over the active tier.
> `tp` — on UWB while `visUwb >= 3`
> `fp` — on UWB while `visUwb < 3` — escalated into dead air
> `fn` — not on UWB while `visUwb >= 3` — missed a free refinement

Positive class `visUwb >= 3`; prediction `activeTier == UWB`; population **all cycles**.
Emitted as `trig_tp/fp/fn/tn/precision/recall/f1` in `arm_summary.csv`.

Pooled across 20 seeds, `ca_tbl_full`:

| | tp | fp | fn | tn | precision | recall | f1 |
|---|---|---|---|---|---|---|---|

| BEFORE | 1778 | 21116 | 40 | 5866 | 0.0777 | 0.9780 | 0.1439 |
| AFTER | 1280 | 90 | 538 | 26892 | 0.9343 | 0.7041 | 0.8030 |

An independent recomputation from the raw traces reproduced both rows to four decimal
places.

### 5.1 Per-seed, BEFORE

| seed | tp | fp | fn | tn | precision | recall | f1 |
|---|---|---|---|---|---|---|---|

| 1 | 95 | 1068 | 1 | 276 | 0.0817 | 0.9896 | 0.1509 |
| 2 | 86 | 1066 | 10 | 278 | 0.0747 | 0.8958 | 0.1378 |
| 3 | 90 | 1063 | 3 | 284 | 0.0781 | 0.9677 | 0.1445 |
| 4 | 90 | 1085 | 1 | 264 | 0.0766 | 0.9890 | 0.1422 |
| 5 | 101 | 1056 | 0 | 283 | 0.0873 | 1.0000 | 0.1606 |
| 6 | 68 | 1083 | 0 | 289 | 0.0591 | 1.0000 | 0.1116 |
| 7 | 87 | 1064 | 1 | 288 | 0.0756 | 0.9886 | 0.1404 |
| 8 | 81 | 1041 | 5 | 313 | 0.0722 | 0.9419 | 0.1341 |
| 9 | 91 | 1081 | 0 | 268 | 0.0776 | 1.0000 | 0.1441 |
| 10 | 88 | 1077 | 4 | 271 | 0.0755 | 0.9565 | 0.1400 |
| 11 | 109 | 1072 | 1 | 258 | 0.0923 | 0.9909 | 0.1689 |
| 12 | 70 | 1051 | 2 | 317 | 0.0624 | 0.9722 | 0.1174 |
| 13 | 60 | 968 | 0 | 412 | 0.0584 | 1.0000 | 0.1103 |
| 14 | 98 | 890 | 2 | 450 | 0.0992 | 0.9800 | 0.1801 |
| 15 | 100 | 1073 | 3 | 264 | 0.0853 | 0.9709 | 0.1567 |
| 16 | 71 | 1064 | 1 | 304 | 0.0626 | 0.9861 | 0.1176 |
| 17 | 111 | 1068 | 2 | 259 | 0.0941 | 0.9823 | 0.1718 |
| 18 | 78 | 1100 | 1 | 261 | 0.0662 | 0.9873 | 0.1241 |
| 19 | 108 | 1059 | 0 | 273 | 0.0925 | 1.0000 | 0.1694 |
| 20 | 96 | 1087 | 3 | 254 | 0.0811 | 0.9697 | 0.1498 |

### 5.2 Per-seed, AFTER

| seed | tp | fp | fn | tn | precision | recall | f1 |
|---|---|---|---|---|---|---|---|

| 1 | 82 | 4 | 14 | 1340 | 0.9535 | 0.8542 | 0.9011 |
| 2 | 28 | 5 | 68 | 1339 | 0.8485 | 0.2917 | 0.4341 |
| 3 | 45 | 2 | 48 | 1345 | 0.9574 | 0.4839 | 0.6429 |
| 4 | 64 | 5 | 27 | 1344 | 0.9275 | 0.7033 | 0.8000 |
| 5 | 89 | 6 | 12 | 1333 | 0.9368 | 0.8812 | 0.9082 |
| 6 | 29 | 10 | 39 | 1362 | 0.7436 | 0.4265 | 0.5421 |
| 7 | 66 | 0 | 22 | 1352 | 1.0000 | 0.7500 | 0.8571 |
| 8 | 44 | 4 | 42 | 1350 | 0.9167 | 0.5116 | 0.6567 |
| 9 | 73 | 9 | 18 | 1340 | 0.8902 | 0.8022 | 0.8439 |
| 10 | 64 | 0 | 28 | 1348 | 1.0000 | 0.6957 | 0.8205 |
| 11 | 76 | 3 | 34 | 1327 | 0.9620 | 0.6909 | 0.8042 |
| 12 | 57 | 2 | 15 | 1366 | 0.9661 | 0.7917 | 0.8702 |
| 13 | 49 | 2 | 11 | 1378 | 0.9608 | 0.8167 | 0.8829 |
| 14 | 51 | 2 | 49 | 1338 | 0.9623 | 0.5100 | 0.6667 |
| 15 | 90 | 3 | 13 | 1334 | 0.9677 | 0.8738 | 0.9184 |
| 16 | 50 | 10 | 22 | 1358 | 0.8333 | 0.6944 | 0.7576 |
| 17 | 88 | 6 | 25 | 1321 | 0.9362 | 0.7788 | 0.8502 |
| 18 | 64 | 8 | 15 | 1353 | 0.8889 | 0.8101 | 0.8477 |
| 19 | 101 | 7 | 7 | 1325 | 0.9352 | 0.9352 | 0.9352 |
| 20 | 70 | 2 | 29 | 1339 | 0.9722 | 0.7071 | 0.8187 |

### 5.3 Reconciliation of `0.088`

**Traced.** It is not the same quantity as the table above.

Source rows:
- `CA-TBL_Technical_Report_SelfContained.md:258` — `| propagation + arbitration (gate as trigger) | 14 | 0 | 145 | 1.0000 | 0.0881 | 0.1618 |`
- `report/PCS_REDESIGN_RESULTS.md:891` — `| D prop/arb | 14 | 0 | 145 | 1281 | 1.0000 | 0.0881 | 0.1618 |`

| property | the `0.088` figure | the `trig_*` figure above |
|---|---|---|
| quantity | **the integrity gate (`suspect`) scored as an escalation trigger** | `activeTier == UWB` as a predictor of `visUwb >= 3` |
| tp / fp / fn / tn | 14 / 0 / 145 / 1281 | 1778 / 21116 / 40 / 5866 (before) |
| arithmetic | 14/(14+145) = 0.08805 | 1778/(1778+40) = 0.9780 |
| population | **seed 7 only**, 1440 cycles | 20 seeds pooled, 28800 cycles |
| geometry | `boxHalf=1000` (default) | `boxHalf=2000` |
| flags | `--nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=legacy` | defaults, ± `--escalationRequiresEvidence` |

The two are not comparable and the before/after in §5 must not be quoted against
`0.088`. Section 891 of `PCS_REDESIGN_RESULTS.md` describes it as the gate firing
"never wrongly (0 false positives, recall 0.088)".

---

## §6 — Claim 2 and the integrity ablation

After the predicate. Source: `results/p10_plane_fix/`.


| arm | seeds_meeting_spec | totalJ | cep50M | avail_mean |
|---|---|---|---|---|
| `ca_tbl_full` | 19/20 | 24.26 | 2.69 | 0.9917 |
| `fixed_threshold` | 18/20 | 22.77 | 3.09 | 0.9828 |
| `ca_tbl_no_integrity` | 18/20 | 24.24 | 2.69 | 0.9903 |

`ca_tbl_full` − `ca_tbl_no_integrity` availability, paired per seed: positive 4, negative 1, exactly tied 15, mean gap +0.00132

`ca_tbl_full` − `fixed_threshold` availability, paired per seed: positive 20, negative 0, exactly tied 0, mean gap +0.00889

**Seed 13:**

| arm | availability | cep50M | totalJ | escalations | UWB occupancy |
|---|---|---|---|---|---|
| `ca_tbl_full` | 0.9590 | 2.62 | 24.19 | 25 | 3.5% |
| `fixed_threshold` | 0.9125 | 3.11 | 22.95 | 151 | 0.0% |
| `ca_tbl_no_integrity` | 0.9326 | 2.69 | 24.13 | 31 | 3.3% |

Seed-13 trace differences beyond the summary columns: **NOT AVAILABLE** — not
extracted in this pass.

---

## §7 — Geometry, radio, and the dimensionless axis

### 7.1 Measured r80, extended probe (`results/causal/probe_range_sweep.csv`)

| tier | lower bracket (dist / pdr) | upper bracket (dist / pdr) | r80 interpolated |
|---|---|---|---|

| 868MHz | 1500 m / 0.8030 | 2000 m / 0.7050 | **1515.3 m** |
| 2.4GHz | 900 m / 0.8080 | 1200 m / 0.5545 | **909.5 m** |
| UWB | 10 m / 0.8715 | 20 m / 0.6195 | **12.8 m** |

### 7.2 The source comment still claims tier-2 r80 is censored

Verbatim, `scratch/ca_tbl_causal_sim.cc:216-218`:

> `2p4  r80 >= 900 m -- CENSORED, the sweep ends at 900 m with PDR 0.804 and the tier
> has not fallen off. 900 m is a LOWER BOUND, not a measurement. Extend dists2p4 past
> 900 m before treating this as tier-2's range.`

The extension was carried out — the probe CSV contains rows at 1200, 1600, 2000, 2500,
3000, 4000 and 6000 m. The comment is stale.

### 7.3 Facility siting and co-location

- Formula: `const double f = boxHalfM * 0.47;` (`ca_tbl_causal_sim.cc:269`), four
  facilities at `(±f, ±f)`.
- `t2TriadRadiusM = 150.0` (`:2996`), `uwbTriadRadiusM = 10.0` (`:2995`).
- Co-location, verbatim `ca_tbl_causal_sim.cc:275-279`:

> Co-location is deliberate. If tier-2 and tier-3 coverage were in different places, an
> escalation failure could be a siting artefact rather than a decision error, and the
> comparison between the two refinement tiers would be confounded.

### 7.4 Dimensionless axis

`spacing = 0.47 x boxHalf`; `r80(2.4GHz) = 909.5 m` measured.

| boxHalf | spacing (m) | spacing / r80 |
|---|---|---|

| 1500 | 705 | 0.78 |
| 2000 | 940 | 1.03 |
| 2500 | 1175 | 1.29 |
| 3000 | 1410 | 1.55 |
| 4000 | 1880 | 2.07 |
| 5000 | 2350 | 2.58 |
| 6000 | 2820 | 3.10 |

### 7.5 Anchor and gateway counts at `boxHalf=2000`

| tier | count | placement |
|---|---|---|
| 1 (868 MHz) | 25 gateways | `gwGrid=5` grid spanning ±1800 m (0.9 x boxHalf) |
| 2 (2.4 GHz) | 12 anchors | 4 sites x 3, ring radius 150 m |
| 3 (UWB) | 20 anchors | 4 sites x 5, ring radius 10 m |

---

## §8 — The envelope

**PENDING.** The sweep was never launched; no result directory exists.

---

## §9 — Architecture parameters

### 9.1 Per-tier PHY (`TierPhy` = {SF, bandwidth, tx power}), `ca_tbl_causal_sim.cc:3356-3358`

| tier | SF | bandwidth | tx power |
|---|---|---|---|
| 868 MHz | 7 | 125 kHz | 14.0 dBm |
| 2.4 GHz | 7 | 800 kHz | 12.5 dBm |
| UWB | 7 | 499.2 MHz | −16.47 dBm |

### 9.2 Energy model, `src/lora-physics-engine/model/ca-tbl-energy-model.cc:36-77`

| tier | supply | tx | rx | idle | sleep |
|---|---|---|---|---|---|
| 868 MHz | 3.3 V | 45.0 mA | 5.3 mA | 1.5 mA | 0.0012 mA |
| 2.4 GHz | 3.3 V | 24.0 mA | 7.0 mA | 1.4 mA | 0.0012 mA |
| UWB | 3.3 V | 82.0 mA | 94.0 mA | 12.0 mA | 0.0007 mA |

UWB rx current exceeds tx current — flagged in the source as intentional.
`AccountListen(tier, seconds)` at `:485` charges receive-window time.

### 9.3 Duty cycle

`m_dutyLimit[868] = 0.025`, `m_dutyLimit[2P4] = 1.0`, `m_dutyLimit[UWB] = 1.0`
(`ca-tbl-energy-model.cc:166-168`). Lockout: `m_nextAllowedTxS[tier] = nowS +
airtimeS / m_dutyLimit[tier]` (`:226`). `CanTransmit` returns `true` unconditionally
when the limit is `<= 0.0` or `>= 1.0` (`:192-196`) — **tiers 2 and 3 are never
locked out**.

### 9.4 PCS

Legacy form (`ca-tbl-policy.cc:663-670`):

```
term1 = traceRef / (traceRef + max(traceSigma, 0))
term2 = 1.0 / max(gdop, 1.0)
term3 = nLos / nTotal            (0 if nTotal == 0)
pcs   = term1 + term2 + term3
```

v2 form (`:628`, `:652`): `pcs = log10(sigmaRef / max(sigmaEff, 1e-12))` where
`sigmaEff = sqrt(sigma_post * sigma_geo)`.

| parameter | value | file:line |
|---|---|---|
| `traceRef` | 10.0 m² | `ca-tbl-policy.cc:79` |
| `pcsThresholdUp` | 1.55 / 1.70 / 2.30 | `:57-59` |
| `pcsThresholdDown` | 1.75 / 1.90 / 2.55 | `:61-63` |
| `mahalanobisThreshold` | 9.21 (chi-square 99%, 2 DOF) | `:73` |
| `nlosSigmaInflation` | 3.0 (UNCALIBRATED) | `:95` |
| `processNoiseStdPos` / `Vel` | 1.0 / 0.5 | `:97-98` |
| `nDeescalate` | 5 | `:65` |

### 9.5 The predicate as implemented

- `uwbPredictRadiusM = 20.0`, `uwbPredictMaxSigmaFrac = 0.25` (`ca-tbl-policy.cc:77-78`).
- Quality gate: `sqrt(max(traceSigma,0)) > uwbPredictMaxSigmaFrac * uwbPredictRadiusM`
  → suppress. Runs **before** the coverage count.
- Coverage test: count of `uwbAnchorX/Y` within `uwbPredictRadiusM` of the EKF
  estimate; `< 3` → suppress.
- **Suppress-only**: applied after `wantEscalate` is formed; it can never create an
  escalation.
- Gates the **2.4→UWB step only**. The 868→2.4 gate is a separate field
  (`escalationRequiresEvidenceT2`), defaults off, is not exposed on the CLI, and is
  documented as broken — the tier-2 radio sleeps while another tier is active, so its
  neighbour count is always 0 and enabling it latches the node on tier 1
  (measured: 1440/1440 cycles on 868, 0 escalations).
- Counters: `m_predictBlockedQuality`, `m_predictBlockedCoverage`, `m_evidenceBlockedT2`;
  `d.predictedVisUwb` per cycle (−1 when the predicate did not run).

### 9.6 Predictor operating curve

Definition used: positive class `visUwb >= 3` (geometric solvability); prediction =
`>= 3` anchors within R of the EKF estimate. Geometry reconstructed in Python and
validated against each trace's own `visUwb` column — **0/1440 mismatches at both
geometries** before any curve was computed.

| R | boxHalf=1000 recall | precision | boxHalf=2000 recall | precision |
|---|---|---|---|---|
| 12.8 m | 0.283 | 1.000 | 0.219 | 1.000 |
| 20.0 m | 0.843 | 1.000 | 0.854 | 0.988 |
| 30.0 m | 0.981 | 0.981 | 1.000 | 0.970 |

**This differs from the branch's figure.** `wip/b2-coverage-predicate` records
`recall(deliverable) = 0.941` at R=20, `boxHalf=1000`. The column is labelled
*deliverable*, suggesting it scores against cycles where UWB actually produced a fix
rather than against geometric solvability. The two are not the same quantity and must
not be quoted against each other.

### 9.7 Cycle timing

`dt = 2.5 s` (`ca_tbl_causal_sim.cc:2986`), `durationS = 3600.0` (`:2975`),
**1440 cycles per run**.

---

## §10 — Verification status

Gate suite `scripts/catbl_gates.sh` last run 2026-08-09 16:13: **G1-G5 ALL PASS**,
suite exit 0. Verifier `verify_causal_sim.py` on `results/causal`: **141 checks
passed, 161 gates evaluated (141 check + 11 warn + 9 note)**, `arms_audited 6/6`.
G5 verifier self-test: **11 cases, all ok** — including the three digest cases and
the stripped-digest case. `scripts/publication_ready.sh`: **PUBLICATION READY**.

Clone test: a fresh clone of `a2c32f5` from the rewritten remote built under
`--build-profile=optimized` (1711/1711 targets) and reproduced
`results/baseline_v6/arm_summary.csv` **byte-for-byte** when run at seed 7 with
`--nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=legacy`. With default
flags the diff is non-empty — the baseline is a propagation-mode artefact.

Pre-registration timing relative to the runs it governs:

| commit | timestamp | governs |
|---|---|---|
| `b49df46` | 2026-08-09 22:35 | all Phase 10 runs |
| `a059203` | 2026-08-09 23:58 | amendment: claim 1a/1b split, (A,R) derivation, N=76 predicate |
| `results/p10_plane` | 2026-08-10 00:03 build | **after** both — 5 min after the amendment |
| `results/p10_plane_fix` | 2026-08-10 10:45 build | **after** both |

Both pre-registration commits precede every run they govern.
