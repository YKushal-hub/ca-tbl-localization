# C1 with the evidence gate off — results

**2026-09-29.** Registered in `report/PREREG_C1_NOGATE.md` at `ddae10f` (00:39:20 +0530),
before any cell ran. Cells committed at `b199ba5`. Branch `work/2026-09-27-b3-regen`, worktree
`~/catbl_b3`, binary `./build/scratch/ns3.47-ca_tbl_causal_sim-optimized`, built from `084a7ff`,
binary+library digest `9c6493f7a992a821` — the binary that produced B3's
`results/regen27_integrity/`. 40 processes, 0 failed. This report was checked against the
prereg and recounted from the raw rows and logs before it was committed (§1, §8).

## Verdict

**C1 with the evidence gate off: REPRODUCES (energy; accuracy not shown worse).** Not
"in full": the accuracy half did not meet the registered FAVOURS-ON rule.

| | registered reading | number and test | denominator · window |
|---|---|---|---|
| Population (step 1) | **PRESENT** | IMU witness refused **4 315** escalations, **204–235 per seed**, on 20/20 seeds; corroboration witness **0** (largely control flow, §2). `off`/`on` rows differ on **20/20** seeds | one 600 s run of 240 cycles per seed; a refusal is one cycle's event |
| P1.1 energy | **REPRODUCES** | ΔtotalJ (on − off) mean **−104.86 J**, median −106.31 J, 95 % CI **[−107.97, −101.12] J**, **20−/0+/0 ties, p = 1.9×10⁻⁶** | 20 paired seeds · totalJ over 600 s · bootstrap B = 20 000 over seeds |
| P1.2 accuracy | **NOT SHOWN WORSE** — not an equivalence result | ΔrmseM mean **−0.66 m**, median −8.82 m, 95 % CI **[−9.62, +11.24] m**, 16−/4+/0 ties, p = 0.012. The sign test favours `on`; the CI spans zero, so FAVOURS-ON (which needs both) is not met | 20 paired seeds · rmseM over 240 cycles · as above |
| Seeds 1–8, verdict comparability only | **REPRODUCES** (same verdict) | ΔtotalJ 8−/0+, p = 0.0078, CI [−112.24, −104.66] J; ΔrmseM 7−/1+, p = 0.070, CI [−12.85, +5.97] m | 8 paired seeds. **No magnitude is compared with the original** |
| M0, manipulation check | **the flag acted** | `off` row here vs B3 `off` (gate on), same binary: differs on **8/8**; escalations 1 → 39–40, `cycles_uwb` 0 → 190–195 | seeds 1–8 · byte identity (control flow) |
| S2, supplementary | witness ≠ gate | `on` here differs from **both** B3 `off` and B3 `on` on 8/8 | seeds 1–8 · byte identity, no verdict |

**What it says.** In the non-default configuration with `--escalationRequiresEvidence=false`,
turning on the witness pair lowers the node's energy on every one of 20 seeds, by 80–115 J over
600 s. The rows show one mechanism. The `off` arm spends 165–198 of its 240 cycles on UWB. Its
UWB neighbour table holds on average 0.000–0.096 entries per cycle, and it is blind (the active
tier addresses no neighbour) on 150–192 cycles. The witness removes 145–189 of those UWB cycles
at no cost in fix cycles (equal on 19/20 seeds, one fewer on seed 9), and `j_uwb` carries the
whole energy difference. It acts as a veto on escalating into a tier that has almost nothing
to range to. (`cyclesInUwbCoverage` reads 0 on all 40 rows, but under this invocation that
column is **structurally zero** and says nothing about coverage; §4.)

**What it does not say** (prereg §5, registered before the run). The IMU witness refused on
204–235 of 240 cycles. At the end of each run the step-and-heading engine was 287–4 070 m from
truth, against tolerances of 15–400 m (one end-of-run snapshot per run). A witness that refuses
on 85–98 % of cycles is close to a blanket veto. This design does not test whether the witness
can tell a bad estimate from a good one, and **no claim about integrity, trust or detection
follows from this verdict.**

**What it does to B3.** Neither falsifier aimed at B3's inferred cause fired: not F2 (NO
POPULATION) and not F3 (M0 byte-identical). B3 attributed its byte-identical `off`/`on` rows,
from code, to the evidence gate absorbing every refusal the witness made. That attribution has
now survived an intervention on the one variable it names. With the gate removed and nothing
else changed, the control row changes (8/8), and the witness's refusals reach the output row
on 20/20 seeds. One part is still inferred: that each refusal in B3 was one the gate would have
made. Neither campaign logs refusals per escalation, so that is not counted. B3's verdict for
the shipped default is unchanged: **with the evidence gate on, the witness adds nothing
measurable. With it off, the witness lowers energy on every seed by suppressing escalation into
UWB.**

---

## 0. Provenance and completeness, checked independently of the driver and the analyser

Recounted from the files with `awk`, `grep`, `cmp`, `case` and `sha256sum` (rule 3), not from
the driver's `FINAL` line or the analyser's step 0:

- **40/40** sidecars read `source ddae10f dirty 0 profile optimized`. **40/40** `digest sha256`
  blocks verify with `sha256sum -c` in their cell directory.
- **40/40** `arm_summary.csv` are exactly 2 lines: 40 data rows, 20 per setting, one header
  form. Every row has `steps` 240 and `durationS` 600.0000. There are 0 `trace_*` files under
  the campaign root.
- **40/40** invocation lines end with the registered arguments for their seed and setting,
  followed by their own `--outdir`. This was checked with a shell `case` over every cell, not
  only by the analyser.
- `INTEGRITY:` lines: 0 of 20 `off` logs hold one; each of the 20 `on` logs holds exactly one.
  The treatment is as registered (A5 not triggered).
- **Rule 13.** `ddae10f` was committed at 00:39:20 +0530. The driver logged its first launch
  (`START …/off_s1`) at 00:39:25. The earliest mtime under `results/regen27_integrity_nogate/`
  is `off_s1/`, 00:39:26, and the latest is `on_s20/run.log`, 00:41:04.
- **Binary.** The digest is computed as the driver computes it: `sha256sum` of the binary and
  every `build/lib/libns3.47-*-optimized.so`, then `sha256sum` of that listing. It is
  `9c6493f7a992a821` now, at both driver starts (`PROGRESS.log` lines 2 and 46), and at B3's
  start (`B3_PROGRESS.log` line 2). `git diff 084a7ff HEAD -- src scratch` is empty.
- **Tracked set.** `b199ba5` adds 80 files: `arm_summary.csv` and `run_id.txt` for each of the 40
  cells, and nothing else. Every `run.log` is ignored by `.gitignore:75` (`/results/**`), and
  `git check-ignore -v` confirms this on a real file.
- **RULES.md §6.** Every row has `cycles_868`, `cycles_2p4` and `cycles_uwb` > 0, so the
  unused-tier predicate B3 checked has no row to act on here. It is vacuous for this campaign,
  not passed.
- Driver: two passes (seeds 1–10, 11–20), 40 launched, 40 ended `exit=0 … OK`, 0 moved to
  `incomplete/`. Concurrency never exceeded 4 (counted from `START`/`END` lines), and each cell
  took 6–13 s.

## 1. Every analysis command

Run from `~/catbl_b3` with Python 3.8.10. Output went to `~/catbl_b3_logs/c1_nogate/analysis/`,
outside the tree and not citable. The commands regenerate it.

```bash
# registered analyser, unchanged (prereg section 4) -- the only analysis that produces a verdict
python3 scripts/b3/c1_nogate_analyse.py results/regen27_integrity_nogate      # exit 0

# independent recount (rule 3) -- no verdict
R=results/regen27_integrity_nogate
for d in $R/*_s*; do sed -n 3p $d/run_id.txt; done | sort | uniq -c
for d in $R/*_s*; do (cd $d && awk 'f{print} /^digest sha256$/{f=1}' run_id.txt | sha256sum -c --quiet); done
for d in $R/*_s*; do wc -l < $d/arm_summary.csv; done | sort | uniq -c
find $R -name 'trace_*' | wc -l
find $R -mindepth 1 -printf '%TY-%Tm-%Td %TH:%TM:%TS %p\n' | sort | sed -n '1p;$p'
grep -l INTEGRITY $R/off_s*/run.log | wc -l
grep -h "INTEGRITY: escalations refused" $R/on_s*/run.log
for s in $(seq 1 20); do cmp -s <(tail -n +2 $R/off_s$s/arm_summary.csv) \
                                <(tail -n +2 $R/on_s$s/arm_summary.csv) && echo "s$s identical"; done
for s in $(seq 1 20); do paste -d, <(tail -n +2 $R/off_s$s/arm_summary.csv) \
                                   <(tail -n +2 $R/on_s$s/arm_summary.csv); done \
  | awk -F, '{N=76; print $2, $12, $(N+12), $(N+12)-$12, $5, $(N+5), $(N+5)-$5, $16"->"$(N+16), $18"->"$(N+18)}'
sha256sum ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized build/lib/libns3.47-*-optimized.so | sha256sum | cut -c1-16
```

**Review before commit**, same tree and HEAD `b199ba5`. These commands checked the draft of
this report against the raw rows. Each output is recorded after its command.

```bash
R=results/regen27_integrity_nogate; A=~/catbl_b3_logs/c1_nogate/analysis
python3 scripts/b3/c1_nogate_analyse.py $R > $A/c1_nogate_analyse.review.out    # exit 0
diff $A/c1_nogate_analyse.out $A/c1_nogate_analyse.review.out    # line 2 only (HEAD)
diff $A/c1_nogate_analyse.rerun.out $A/c1_nogate_analyse.review.out    # identical
# every invocation line (the draft had spot-read two)                   -> no BAD line
for s in $(seq 1 20); do for t in off on; do
  a="--seed=$s --duration=600 --boxHalf=1000 --nFinders=4 --finderMode=1 --sendHandshakeFrames=true --trace=false --arms=ca_tbl_full --escalationRequiresEvidence=false"
  [ $t = on ] && a="$a --imuCrossCheck=true --corroborationCheck=true"
  case "$(sed -n 2p $R/${t}_s$s/run_id.txt)" in
    *" ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized $a --outdir=$R/${t}_s$s") ;;
    *) echo "BAD $t $s" ;;
  esac
done; done
# paired rows: off in columns 1-76, on in 77-152
for s in $(seq 1 20); do paste -d, <(tail -n +2 $R/off_s$s/arm_summary.csv) \
    <(tail -n +2 $R/on_s$s/arm_summary.csv); done > $A/paired_rows.review.csv
# endpoints in awk: mean, signs, exact two-sided sign p, and a bootstrap on a different
# generator (awk rand, srand 777, B = 20 000) as a check on the analyser's resampler
for c in 12 5; do for m in 20 8; do
  awk -F, -v N=76 -v c=$c -v m=$m 'BEGIN{srand(777)} $2<=m{d[n++]=$(N+c)-$c}
    END{for(i=0;i<n;i++){s+=d[i]; neg+=(d[i]<0); pos+=(d[i]>0)}
        nn=neg+pos; k=(neg<pos)?neg:pos; b=1; cum=0; for(i=0;i<=k;i++){if(i)b=b*(nn-i+1)/i; cum+=b}
        for(r=0;r<20000;r++){t=0; for(i=0;i<n;i++) t+=d[int(rand()*n)]; print t/n > "/dev/stderr"}
        printf "col %d seeds 1-%d mean %.4f  %d-/%d+  sign p %.4g", c, m, s/n, neg, pos, 2*cum/2^nn}' \
    $A/paired_rows.review.csv 2> $A/boot.tmp
  sort -g $A/boot.tmp | awk '{a[NR]=$1} END{printf "  awk bootstrap [%.3f, %.3f]\n", a[int(.025*NR)], a[int(.975*NR)]}'
done; done; rm -f $A/boot.tmp
#   col 12 seeds 1-20 mean -104.8617  20-/0+  sign p 1.907e-06  awk bootstrap [-107.930, -101.087]
#   col 12 seeds 1-8  mean -108.8948   8-/0+  sign p 0.007812   awk bootstrap [-112.228, -104.696]
#   col 5  seeds 1-20 mean   -0.6568  16-/4+  sign p 0.01182    awk bootstrap [-9.431, 11.318]
#   col 5  seeds 1-8  mean   -5.4381   7-/1+  sign p 0.07031    awk bootstrap [-12.812, 6.011]
# unregistered descriptive columns used in the text (no verdict)
awk -F, -v N=76 '{printf "s%s uwbRemoved %d blind %s->%s meanNbr_uwb %s->%s trig_tp %s,%s fp-cycles_uwb %d,%d dj868 %+.3f dj2p4 %+.3f djuwb %+.3f\n",
   $2, $18-$(N+18), $58, $(N+58), $57, $(N+57), $31, $(N+31), $32-$18, $(N+32)-$(N+18), $(N+13)-$13, $(N+14)-$14, $(N+15)-$15}' $A/paired_rows.review.csv
# the coverage label: its default, and whether any cell here or in B3's C1 set it
grep -n 'bool uwbCoverageIncludesFindersArg' scratch/ca_tbl_causal_sim.cc     # 6751: ... = false;
grep -l uwbCoverageIncludesFinders $R/*/run_id.txt results/regen27_integrity/*/run_id.txt | wc -l   # 0
# SHS end-of-run error and tolerance, from the on logs                  -> 287.4-4070.1 m; 15.4-399.8 m
grep -h "SHS: engine" $R/on_s*/run.log | sed 's/.*error \([0-9.]*\) m;.*/\1/' | sort -g | sed -n '1p;$p'
grep -h "SHS: engine" $R/on_s*/run.log | sed 's/.*against tolerance \([0-9.]*\) m/\1/' | sort -g | sed -n '1p;$p'
# rule 13: first launch against the registration commit                 -> 00:39:20 / 00:39:25
git show -s --format=%ci ddae10f; grep -m1 " START results/" ~/catbl_b3_logs/c1_nogate/PROGRESS.log
```

The awk recount reproduces the analyser to the printed precision: mean ΔtotalJ −104.862 J, 20
negative; ΔrmseM positive on exactly seeds 6, 9, 14 and 19. Both tables below (§2, §3) were
regenerated from the run logs and the paired rows and diffed against the draft with no
difference. The awk bootstrap, on a different generator, lands within 0.2 of the analyser's
bounds and changes no rule outcome: the energy CI is below zero on both generators, and the
rmse CI spans zero on both. The binding intervals are the analyser's.

**The analyser was run three times.** It ran first at 00:41:18, after the cells and before the
results commit, with HEAD at `ddae10f`. It ran again at 00:48:14 for the draft, and a third time
at 04:43 for this review, both with HEAD at `b199ba5`. The three outputs are byte-identical
except line 2, which prints HEAD. The bootstrap generator is seeded (`random.Random(20260929)`),
so the intervals are the same on every run.

## 2. Population (step 1, rule 4)

| seed | IMU refusals | corroboration | `off`/`on` row | SHS error at end of run |
|---|---|---|---|---|
| 1 | 207 | 0 | differs | 601.4 m |
| 2 | 215 | 0 | differs | 772.3 m |
| 3 | 208 | 0 | differs | 421.4 m |
| 4 | 218 | 0 | differs | 506.1 m |
| 5 | 212 | 0 | differs | 625.8 m |
| 6 | 210 | 0 | differs | 1 827.1 m |
| 7 | 223 | 0 | differs | 427.2 m |
| 8 | 220 | 0 | differs | 518.2 m |
| 9 | 218 | 0 | differs | 1 339.8 m |
| 10 | 221 | 0 | differs | 600.7 m |
| 11 | 219 | 0 | differs | 517.3 m |
| 12 | 215 | 0 | differs | 554.7 m |
| 13 | 221 | 0 | differs | 625.7 m |
| 14 | 235 | 0 | differs | 4 070.1 m |
| 15 | 208 | 0 | differs | 501.3 m |
| 16 | 211 | 0 | differs | 590.5 m |
| 17 | 226 | 0 | differs | 479.2 m |
| 18 | 204 | 0 | differs | 381.2 m |
| 19 | 216 | 0 | differs | 287.4 m |
| 20 | 208 | 0 | differs | 488.9 m |

**Denominator and window.** Each count is out of one 600 s run of 240 cycles. A refusal is a
single cycle's decision, so no refusal straddles the end of the window and there is nothing to
censor. N + M > 0 on 20/20 seeds, and the rows differ on 20/20. **POPULATION: PRESENT.** No
seed is an exact tie, so the sign tests below have n = 20 and n = 8. The SHS column is one
end-of-run snapshot from each `on` log (`ca_tbl_causal_sim.cc:6350-6358`). It is descriptive
and unregistered, and the `off` logs do not print it.

**The corroboration witness never refused, here or in B3** (0 in 20 runs here, 0 in 8 there).
That zero is largely control flow (rule 4). The corroboration witness is evaluated only on an
escalation the IMU witness has already let through: `ca-tbl-policy.cc:1258-1259`,
`if (wantEscalate && m_config.corroborationCheck && m_corrTotal >= corroborationMinWitnesses)`,
the ordering B3 §C1 point 3 records. The IMU witness refused 204–235 escalations per run, which
left the corroboration witness few to vote on. How often it was actually consulted, and whether
its witness-count condition ever held, is not counted. Its inputs
(`ca_tbl_causal_sim.cc:5330-5352`) are neighbour-table lookups and arithmetic: it makes no
random draw and sends no transmission. So with a counter of 0 it has no path to any output, and
every row difference here belongs to the IMU witness. That attribution is **inferred from code
and the counter, not measured**, because no IMU-only arm was registered.

## 3. Primary endpoints (steps 2–3)

Per seed, d = on − off. Negative favours the witness.

| seed | totalJ off | totalJ on | ΔtotalJ | rmseM off | rmseM on | ΔrmseM | `cycles_868` off→on | `cycles_uwb` off→on |
|---|---|---|---|---|---|---|---|---|
| 1 | 117.477 | 21.343 | −96.134 | 25.297 | 8.816 | −16.481 | 2→2 | 195→25 |
| 2 | 124.917 | 10.313 | −114.604 | 17.601 | 8.918 | −8.683 | 2→2 | 194→5 |
| 3 | 124.744 | 13.306 | −111.439 | 19.074 | 6.075 | −12.999 | 1→1 | 194→10 |
| 4 | 124.554 | 10.581 | −113.973 | 13.299 | 8.278 | −5.021 | 2→2 | 193→5 |
| 5 | 122.963 | 13.207 | −109.756 | 17.449 | 9.086 | −8.363 | 2→2 | 191→10 |
| 6 | 124.343 | 12.506 | −111.837 | 11.561 | 43.085 | **+31.524** | **2→119** | 193→10 |
| 7 | 121.258 | 13.347 | −107.911 | 23.458 | 7.912 | −15.546 | 2→2 | 190→10 |
| 8 | 118.821 | 13.316 | −105.505 | 16.869 | 8.932 | −7.936 | 3→3 | 191→10 |
| 9 | 107.489 | 11.889 | −95.600 | 27.267 | 53.278 | **+26.011** | **5→190** | 170→10 |
| 10 | 121.180 | 13.207 | −107.973 | 21.050 | 7.167 | −13.883 | 3→3 | 188→10 |
| 11 | 124.380 | 13.227 | −111.153 | 15.347 | 6.393 | −8.954 | 2→2 | 193→10 |
| 12 | 121.861 | 16.367 | −105.494 | 23.890 | 9.248 | −14.643 | 2→2 | 192→15 |
| 13 | 120.139 | 13.508 | −106.631 | 21.015 | 8.175 | −12.840 | 2→2 | 198→10 |
| 14 | 114.486 | 8.675 | −105.810 | 27.423 | 116.220 | **+88.797** | **3→220** | 175→5 |
| 15 | 120.679 | 20.729 | −99.951 | 25.270 | 10.189 | −15.080 | 1→1 | 192→25 |
| 16 | 122.494 | 16.292 | −106.202 | 12.073 | 11.863 | −0.210 | 2→2 | 188→15 |
| 17 | 113.283 | 13.382 | −99.901 | 13.014 | 8.385 | −4.629 | 2→1 | 179→10 |
| 18 | 117.692 | 16.428 | −101.264 | 19.347 | 8.445 | −10.902 | 2→2 | 185→15 |
| 19 | 99.039 | 19.361 | −79.679 | 23.341 | 33.527 | **+10.185** | **5→21** | 165→20 |
| 20 | 118.700 | 12.282 | −106.418 | 20.095 | 6.612 | −13.483 | 2→2 | 192→10 |

**Seeds 1–20 (binding).**

| | mean | median | sd | range | 95 % seed bootstrap CI | sign test (exact, two-sided) | rule |
|---|---|---|---|---|---|---|---|
| ΔtotalJ | −104.862 J | −106.310 J | 8.011 | [−114.604, −79.679] | [−107.970, −101.117] | 20−/0+/0 ties, n = 20, p = 1.91×10⁻⁶ | **FAVOURS-ON** → P1.1 **REPRODUCES** |
| ΔrmseM | −0.657 m | −8.819 m | 24.887 | [−16.481, +88.797] | [−9.618, +11.242] | 16−/4+/0 ties, n = 20, p = 0.0118 | **NO-DETECTED-DIFFERENCE** → P1.2 **NOT SHOWN WORSE** |

**Seeds 1–8 (comparability of the verdict only; no magnitude comparison with the original).**

| | mean | 95 % CI | sign test | rule |
|---|---|---|---|---|
| ΔtotalJ | −108.895 J | [−112.240, −104.662] | 8−/0+, p = 0.0078 | FAVOURS-ON → REPRODUCES |
| ΔrmseM | −5.438 m | [−12.848, +5.971] | 7−/1+, p = 0.070 | NO-DETECTED-DIFFERENCE → NOT SHOWN WORSE |

The two verdicts agree, so there is no disagreement to report. The original's C1 reached
"energy lower on 8/8, accuracy no worse" on these seeds, and that verdict holds here. On
magnitude this report says nothing (prereg §1). The seeds 1–8 rmse split is 7/8 with p = 0.070.
As the prereg warned, that falls short of the registered rule.

**On the accuracy half, both readings.** The sign test favours the witness: 16 of 20 seeds,
p = 0.012. The CI does not, because four seeds move the other way by a lot: +10.2, +26.0,
+31.5 and **+88.8 m**. The registered rule needs both tests for FAVOURS-ON, so the binding
reading is **NOT SHOWN WORSE**. That is not a claim that accuracy is no worse; no
non-inferiority margin was registered. Neither "improves" nor "no worse" may be quoted from
this campaign.

## 4. Secondary, descriptive (step 4, no verdict)

| column, off → on | range across seeds | mean Δ | signs |
|---|---|---|---|
| `escalations` | 36–41 → 2–6 | −35.75 | 20−/0+ |
| `cycles_uwb` | 165–198 → 5–25 (145–189 fewer) | −175.90 | 20−/0+ |
| `fixCycles` | 237–240 → 236–240 | −0.05 | 1−/0+/19 ties (seed 9: 237 → 236) |
| `j_uwb` | 93.09–119.13 J → 3.10–14.27 J | −105.948 J | 20−/0+ |
| `cyclesInUwbCoverage` | 0 → 0 on every row — **structurally zero, see below** | — | — |
| `suspects` | 1 → 1 on seeds 3 and 15, 0 → 0 elsewhere | — | — |
| `blindCycles` *(unregistered)* | 150–192 → 5–23 | — | 20−/0+ |
| `meanNbr_uwb` *(unregistered)* | 0.0000–0.0958 → 0.0000–0.0125 | — | — |

`j_uwb` carries the whole energy difference. A decomposition by `awk` (not registered) gives mean
Δ`j_uwb` −105.948 J, Δ`j_2p4` +1.064 J and Δ`j_868` +0.023 J. `totalJ` equals the sum of the
three `j_` columns to 10⁻⁹ J on every row, so these sum to the ΔtotalJ above.

**`cyclesInUwbCoverage` = 0 is a control-flow fact here, not a measurement (rule 4).** The
column counts cycles in which at least three UWB sources lie within nominal UWB range of the
node's true position (`ca_tbl_causal_sim.cc:5840-5879`). It sees finders only under
`--uwbCoverageIncludesFinders`, which defaults to **false** (`:6751`) and which no cell here,
and no cell of B3's C1, sets. Under the tri-band topology tier 3 comes from finders, so the
flag's own help text (`:7429-7438`) says `visUwb` is then structurally 0 and every UWB cycle is
scored a false positive. The rows carry that signature: `trig_tp` = 0 and `trig_fp` =
`cycles_uwb` on all 40 rows. The counter itself is reachable on this binary: the B3 cells that
set the flag (`regen27_imu`, `regen27_k1`, `regen27_k10`) read 0–73 over their 300 `ca_tbl_full`
rows, and `results/regen27_imu/on_s12` reads 10–19 (§9). But under this invocation it cannot
be non-zero. So "no coverage" is **not** established by this column. What
the rows do measure is the neighbour picture (last two table rows). `meanNbr_uwb` is the
UWB MAC's neighbour count averaged over all 240 cycles. `blindCycles` counts cycles in which the
active tier addressed no neighbour (`:5832-5836`); it is ≤ `cycles_uwb` on every row. The
prereg's §0 and §5 cite this column as evidence of zero coverage (§8).

**Where the four worse-accuracy seeds come from — inferred from code and rows, not measured.**
On the 16 seeds where ΔrmseM < 0, `cycles_868` is unchanged or falls by one. On the 4 seeds
where ΔrmseM > 0 (6, 9, 14, 19), and only those, the `on` node spends 21–220 cycles on 868 MHz
against 2–5 in `off`. The witness block has no tier condition
(`ca-tbl-policy.cc:1239`, `if (wantEscalate && dynamic && m_crossCheckFresh)`), so it refuses
868 → 2.4 GHz as readily as 2.4 GHz → UWB. The evidence gate refuses only the second of those
(`ca_tbl_causal_sim.cc:7068-7072`; its 868 → 2.4 variant `--escalationRequiresEvidenceT2`
defaults off, `:6696`). The reading the code suggests is this: once a node under the witness
drops to 868 MHz, re-escalation is refused on most cycles and the node stays there. There is no
per-tier refusal counter, so this is the reading of the code that the rows fit, not a count.

## 5. Manipulation check M0 (step 5)

Compared: the `off` cells here (gate off) against `results/regen27_integrity/off_s1..8` (gate
on, B3), same binary. The row differs on **8/8** seeds. Escalations go from 1 to 40, 40, 40, 40,
40, 40, 39 and 40; `cycles_uwb` goes from 0 to 190–195. The flag acted on the control arm, so F3
does not fire.

## 6. Supplementary S2 (step 6, no verdict)

Compared: the `on` cells here (witness, gate off) against B3's `off` and `on` cells, seeds 1–8.
The row differs from both on **8/8** seeds, so the witness alone does not reproduce what the
evidence gate did. The rows show two ways it differs. It lets 5–25 UWB cycles through per run,
where B3's gate let none through. And, per §4, it also gates 868 → 2.4 GHz, which the gate does
not.

For orientation only (not registered, no test, same binary): B3's gate-on rows spent
7.070–7.381 J on seeds 1–8 (B3 §C1). The rows here spent 99.0–124.9 J in `off` and 8.7–21.3 J in
`on`. So the control arm of this campaign shows the gate-off behaviour K3 measured as costly:
escalating into UWB with nothing to range to. The witness recovers most of what the evidence
gate recovers, not all of it.

## 7. What this changes

### In `CA-TBL_Technical_Report_SelfContained.md` (not edited here)

- C1 moves from **WITHDRAWN — NO POPULATION** (B3, shipped default) to a two-part statement, and
  only this one:
  *With the evidence gate on (the shipped default), the witness cross-check has no effect: 8/8
  seeds byte-identical (B3). With the evidence gate off, it lowers energy on 20/20 seeds (mean
  −104.9 J over 600 s, CI [−108.0, −101.1] J) by suppressing escalation into UWB. In that
  configuration the UWB neighbour table held under 0.1 entries per cycle on average, and the
  145–189 extra UWB cycles per run went with no extra fix cycle on 19/20 seeds and one on the
  twentieth. Accuracy is not shown worse (CI
  [−9.6, +11.2] m). On 4/20 seeds it is worse by 10–89 m, and on exactly those seeds the node
  spent 21–220 cycles at 868 MHz under the witness against 2–5 without it.*
- No sentence about integrity, trust, detection or "the estimate being checked" may be written
  from either campaign (prereg §5).
- "The witness earns its place" is not supported in the shipped configuration. In the
  gate-off configuration, what it earns is most of what the evidence gate already earns.
- No sentence may cite `cyclesInUwbCoverage` = 0 from either campaign as a coverage
  measurement (§4).

### In `report/B3_REGEN_RESULTS.md` §C1 (not edited here)

Its "why nothing moved" explanation was marked *inferred from code, not measured*. It survived
the two falsifiers registered against it (F2, F3). It now rests on an intervention as well as on
code: remove the gate and the witness's refusals reach every row. Its per-escalation form is
still not counted. No corrections-register entry is needed; a forward pointer to this report is
enough. Its point 1 lists `cyclesInUwbCoverage` = 0 alongside `cycles_uwb` = 0. Under C1's
invocation the first is structurally zero (§4) and should carry that label. The inference
there rests on `cycles_uwb` = 0, and nothing in it depends on the coverage column.

### Open, and what would settle it

- **Discrimination is untested.** The IMU witness refuses on 85–98 % of cycles because the
  dead-reckoning drift constants are ours and uncalibrated (`docs/TECH_DEBT.md` K.9). A test of
  the witness *as a check* needs a drift model under which it does not refuse almost everything.
  It also needs a configuration where tier 3 has neighbours to range to, so that a veto can cost
  something, run with `--uwbCoverageIncludesFinders=true` so that the coverage column can see
  them.
- **The corroboration witness has never refused**, in 28 runs across both campaigns. It sits
  behind the IMU witness, so it has hardly been consulted. Its contribution is unmeasured, not
  zero. Measuring it needs either an IMU-off, corroboration-on arm or a consultation counter.
- **The 868 MHz pinning** (§4) would need a per-tier refusal counter to become a count rather
  than an inference.

## 8. Disclosures

- No tracked results directory was opened for writing. `src/` and `scratch/` were not touched.
  Every cell directory is newer than the registration commit.
- The analyser was run three times (§1). The outputs match apart from the HEAD line.
- Unregistered computations in this report, all labelled where they appear: the `j_<tier>`
  decomposition, the `blindCycles` and `meanNbr_uwb` columns, the `cycles_868` pattern on the
  four positive-ΔrmseM seeds, the SHS end-of-run snapshot, the awk bootstrap cross-check, and
  the B3 energy range quoted for orientation. None of them carries a verdict.
- **A registration defect, recorded rather than corrected.** Prereg §0 lists
  `cyclesInUwbCoverage = 0` among the facts behind B3's inferred cause. Prereg §5 then reasons
  from "B3 recorded zero UWB coverage cycles on every seed" to the likely mechanism. Under both
  invocations that column is structurally zero (§4). No verdict depends on it, since the
  endpoints are `totalJ` and `rmseM`. The §5 interpretation limit still stands, because its
  substance (a near-blanket veto into a tier with nothing to range to) is carried by
  `meanNbr_uwb` and `blindCycles` instead.
- **What the pre-commit review changed in the draft.** The draft read `cyclesInUwbCoverage` = 0
  as a coverage measurement; it now rests the mechanism on the neighbour columns (§4). The draft
  gave the UWB cycles removed as 150–189; they are 145–189, and seed 19 is 165 → 20. The draft
  said the corroboration zero had no control-flow reading; it now records the ordering behind
  the IMU witness (§2). "A measurement under" B3's inference became "an intervention", and its
  per-escalation form is marked uncounted. The Technical Report sentence no longer says "held
  the node at 868 MHz" as fact, and now gives the measured cycle counts. `durationS` is
  600.0000, not 600.0. The invocation check covers all 40 cells. The analyser run count is
  three, not two. No number that enters a verdict changed.

## 9. Corrections after commit

Found by an independent re-verification after `693088a`, which recounted every headline number
from the raw rows with code written from scratch. No number that enters a verdict changed.

- **§4, reachability of `cyclesInUwbCoverage`.** Old: *"B3 cells that set the flag read 10–19
  on `ca_tbl_full` rows (e.g. `results/regen27_imu/on_s12`)"*. New: those cells read **0–73**
  over **300** `ca_tbl_full` rows; 10–19 is `regen27_imu/on_s12` alone. Per directory:
  `regen27_imu` 4 cells, 20 rows, 10–24; `regen27_k1` 11 cells, 220 rows, 0–73; `regen27_k10`
  12 cells, 60 rows, 0–12. Cells selected by `grep -l 'uwbCoverageIncludesFinders=true'
  results/regen27_*/*/run_id.txt`, all `source 7b17bd6`. The point the sentence makes, that the
  counter can be non-zero on this binary, is unchanged.
