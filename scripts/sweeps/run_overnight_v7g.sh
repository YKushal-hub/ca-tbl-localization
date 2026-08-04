#!/usr/bin/env bash
# CA-TBL overnight batch, Phase 7G.
#
# ONE FLAT PRIORITY-ORDERED JOB LIST, ONE POOL. Not sequential batches: a flat
# list keeps every core busy and degrades gracefully. If the night is short, the
# jobs that finished are the highest-value ones, by construction.
#
# NO CODE CHANGES FOR THE DURATION. No rebuild, no git checkout, no edit. The
# binary is fingerprinted at start and re-asserted when the pool drains; if it
# moved, the batch is void and says so. A5 (the CRLB gate change) is deliberately
# NOT here -- it is a rebuild, and a rebuild mid-batch means half the night's
# results came from a different binary.
#
# COMPLETENESS GATES COUNT ROWS, NOT BYTES. With --seedCount=N the simulator
# writes the header then appends one row per seed, so the file is non-empty from
# seed 1 onward. A byte gate once reported 18/18 Pareto points complete while 8
# of 9 were at 14/20 seeds -- which would have produced a front from 14-seed
# means against 20-seed means, silently unbalanced. Gates run AFTER the pool
# drains, never beside it.
set -u
cd "$(dirname "$0")/../.." || exit 2
export LD_LIBRARY_PATH=build/lib

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
POOL=$(( $(nproc) - 2 ))
[ "$POOL" -ge 1 ] || POOL=1
STAMP=$(date +%Y%m%d_%H%M%S)
ROOT=results/overnight_$STAMP
mkdir -p "$ROOT"
MAN="$ROOT/manifest.txt"
JOBS="$ROOT/jobs.txt"

BINHASH=$(sha256sum "$BIN" | awk '{print $1}')

{
  echo "CA-TBL overnight batch"
  echo "start        : $(date -Is)"
  echo "host nproc   : $(nproc)"
  echo "pool size    : $POOL   (nproc - 2, not hardcoded)"
  echo "binary       : $BIN"
  echo "binary sha256: $BINHASH"
  echo "git HEAD     : $(git rev-parse HEAD)"
  echo "git branch   : $(git symbolic-ref --short HEAD 2>/dev/null || echo DETACHED)"
  echo "git dirty    : $(git status --short | grep -vE '^\?\?' | wc -l) tracked modifications"
  echo
} > "$MAN"

# ---------------------------------------------------------------- job list ----
# Fields, pipe-separated: group | outdir | extra flags | arms | trace
#
# PRIORITY ORDER. Job 3 (--dt) is placed AHEAD of job 2 (gwGrid sweep),
# deliberately and as the queue authorises. Estimated drain at pool=10 is ~11 h,
# which is at or past the edge of a single night, and job 2 alone is ~5 h of it.
# The --dt chain is the one measurement here that could still add a POSITIVE
# result and it does not depend on the confidence model working at all, so it is
# protected ahead of the larger job rather than left to the tail.
#
# --arms is used where an analysis does not need all six. Equivalence was
# verified byte-identically, summary row AND full trace, including arm 6 of 6.
# static_uwb is the slowest arm (1282 blind cycles) and is dropped where it is
# not part of the comparison.
: > "$JOBS"
COMMON="--nlosSource=propagation --arbitrateDutyBudget=true --duration=3600"

# ---- 1. A1: v2 at constant density, b3000. De-confounds every b3000 v2 number.
for s in $(seq 1 20); do
  echo "j1_a1_gw9_v2_b3000|$ROOT/j1_a1_gw9_v2_b3000/seed_$s|--seed=$s --boxHalf=3000 --gwGrid=9 --pcsForm=v2|all|true" >> "$JOBS"
done

# ---- 2. (priority-swapped) --dt sweep. The regulatory chain, both forms.
#         2.5 = current; 3.91 = derived floor for a system that must also signal
#         handoffs ((56.58+41.22)/0.025); 3.0 and 5.0 bracket it.
#         static_uwb dropped: it has no tier-1 duty interaction.
for d in 2.5 3.0 3.91 5.0; do
  for f in legacy v2; do
    for s in $(seq 1 20); do
      echo "j3_dt_${d}_${f}|$ROOT/j3_dt/dt${d}_${f}/seed_$s|--seed=$s --boxHalf=1000 --dt=$d --pcsForm=$f|static_868,static_2p4,fixed_threshold,ca_tbl_no_integrity,ca_tbl_full|false" >> "$JOBS"
    done
  done
done

# ---- 3. A2: gwGrid sweep at b3000, legacy. The one surviving positive claim.
for g in 3 4 5 6 7 9; do
  for s in $(seq 1 20); do
    echo "j2_a2_gwgrid_$g|$ROOT/j2_a2_gwgrid/gw${g}/seed_$s|--seed=$s --boxHalf=3000 --gwGrid=$g --pcsForm=legacy|all|false" >> "$JOBS"
  done
done

# ---- 4/5. Cell E at b3000, both forms. Generalises the integrity-gate finding.
for f in legacy v2; do
  for s in $(seq 1 20); do
    echo "j45_cellE_${f}_b3000|$ROOT/j45_cellE/${f}_b3000/seed_$s|--seed=$s --boxHalf=3000 --pcsForm=$f --suspectForcesEscalation=false|fixed_threshold,ca_tbl_no_integrity,ca_tbl_full|true" >> "$JOBS"
  done
done

# ---- 6. A4: Pareto at b3000. One arm per point, per pareto_sweep.py collect().
for s in 1 1.78 3.162 5.62 10 17.8 31.6 56.2 100; do
  v=$(python3 -c "print(f'{$s*$s:.4f}')")
  echo "j6_pareto_b3000|$ROOT/j6_pareto_b3000/ft_sigma$s|--seedStart=1 --seedCount=20 --boxHalf=3000 --pcsForm=legacy --fixedVarThreshold=$v|fixed_threshold|false" >> "$JOBS"
done
for s in 1 1.78 3.162 5.62 10 17.8 31.6 56.2 100; do
  echo "j6_pareto_b3000|$ROOT/j6_pareto_b3000/v2_sigma$s|--seedStart=1 --seedCount=20 --boxHalf=3000 --pcsForm=v2 --pcsSigmaTargetM=$s|ca_tbl_no_integrity|false" >> "$JOBS"
done

# ---- 7. searchDuty bracket for v2 at b3000. The 828x uncited parameter.
for sd in 0.0 0.1 0.25 0.5 1.0; do
  for s in $(seq 1 20); do
    echo "j7_searchduty_$sd|$ROOT/j7_searchduty/sd${sd}/seed_$s|--seed=$s --boxHalf=3000 --pcsForm=v2 --searchDuty=$sd|static_uwb,ca_tbl_no_integrity,ca_tbl_full|false" >> "$JOBS"
  done
done

# ---- 8. Environment generalisability at b1000. Everything so far is
#         RURAL_FORESTED / DENSE_FOREST only.
#         (env,veg): 0,1 = RuralOpen/sparse ; 2,2 = Suburban/light ; 3,0 = UrbanModerate/none
for cfg in "0 1" "2 2" "3 0"; do
  set -- $cfg
  e=$1; v=$2
  for s in $(seq 1 20); do
    echo "j8_env_e${e}v${v}|$ROOT/j8_env/e${e}v${v}/seed_$s|--seed=$s --boxHalf=1000 --env=$e --vegWide=$v --pcsForm=v2|all|false" >> "$JOBS"
  done
done

NJOBS=$(wc -l < "$JOBS")
{
  echo "planned jobs : $NJOBS"
  echo
  echo "priority order (group : count):"
  cut -d'|' -f1 "$JOBS" | sed 's/_[0-9.]*$//' | uniq -c | awk '{printf "  %-28s %s\n",$2,$1}'
  echo
  echo "NOTE: job 3 (--dt) is placed ahead of job 2 (gwGrid) deliberately -- see"
  echo "      the comment in the script. Estimated drain ~11 h at pool=$POOL."
  echo
} >> "$MAN"

# ------------------------------------------------------------------ runner ----
runjob() {
  line="$1"
  grp=$(echo "$line" | cut -d'|' -f1)
  out=$(echo "$line" | cut -d'|' -f2)
  flg=$(echo "$line" | cut -d'|' -f3)
  arm=$(echo "$line" | cut -d'|' -f4)
  trc=$(echo "$line" | cut -d'|' -f5)
  mkdir -p "$out"
  armflag=""
  [ "$arm" = "all" ] || armflag="--arms=$arm"
  # shellcheck disable=SC2086
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
      --nlosSource=propagation --arbitrateDutyBudget=true --duration=3600 \
      $flg $armflag --trace=$trc --outdir="$out" > "$out/run.log" 2>&1
  echo "$grp rc=$? $(basename "$out")"
}
export -f runjob

echo "=== pool start $(date -Is), $NJOBS jobs at -P $POOL ===" | tee -a "$MAN"
xargs -P "$POOL" -d '\n' -n 1 bash -c 'runjob "$0"' < "$JOBS" > "$ROOT/pool.log" 2>&1
echo "=== pool drained $(date -Is) ===" | tee -a "$MAN"

# ------------------------------------------------------- binary re-assertion --
NOWHASH=$(sha256sum "$BIN" | awk '{print $1}')
{
  echo
  echo "end          : $(date -Is)"
  echo "binary sha256 at end: $NOWHASH"
  if [ "$NOWHASH" = "$BINHASH" ]; then
    echo "BINARY UNCHANGED -- batch is valid"
  else
    echo "*** BINARY CHANGED DURING THE BATCH -- THIS BATCH IS VOID ***"
    echo "*** planned $BINHASH"
    echo "*** actual  $NOWHASH"
  fi
} >> "$MAN"

# ---------------------------------------------------- row-count completeness --
# Expected data rows per arm_summary.csv = seeds_in_that_run x arms_in_that_run.
# Derived per directory from the job line, so a group with --arms is not judged
# against six.
{
  echo
  echo "=== completeness, BY ROW COUNT (not bytes) ==="
  ok_tot=0; plan_tot=0
  while IFS='|' read -r grp out flg arm trc; do
    if [ "$arm" = "all" ]; then na=6; else na=$(echo "$arm" | tr ',' '\n' | wc -l); fi
    ns=1
    case "$flg" in *--seedCount=*) ns=$(echo "$flg" | sed 's/.*--seedCount=\([0-9]*\).*/\1/');; esac
    want=$(( na * ns ))
    got=$(( $(wc -l < "$out/arm_summary.csv" 2>/dev/null || echo 1) - 1 ))
    plan_tot=$(( plan_tot + 1 ))
    if [ "$got" -eq "$want" ]; then ok_tot=$(( ok_tot + 1 )); else echo "  SHORT $grp $(basename "$out"): $got/$want rows"; fi
  done < "$JOBS"
  echo
  echo "  complete: $ok_tot / $plan_tot job directories at full row count"
  echo
  echo "  per group:"
  cut -d'|' -f1,2 "$JOBS" | while IFS='|' read -r g o; do echo "$g"; done | sort -u | while read -r g; do
    tot=$(grep -c "^$g|" "$JOBS")
    echo "    $g: $tot planned"
  done
} >> "$MAN"

echo "MANIFEST: $MAN"
tail -30 "$MAN"
echo "=== OVERNIGHT BATCH FINISHED $(date -Is) ==="
