#!/usr/bin/env bash
# B3 regeneration driver. Registered in report/PREREG_B3_REGEN.md and committed WITH it,
# in one commit, before the first cell ran. Every cell below is an invocation from section 4
# there: the original campaign's sidecar (or runner) arguments with only --outdir new.
#
#   cd ~/catbl_b3 && bash scripts/b3_driver.sh   runs in the FOREGROUND until every cell has
#                                                been attempted; exits 0 only if all 125
#                                                cells are complete
#   bash scripts/b3_driver.sh --status           per-campaign completeness, then exit
#   bash scripts/b3_driver.sh --list             the cell table, one line per cell
#
# Resumable: a re-invocation skips every cell that is complete by the section 6 rule --
# exact line count, exact trace count, a digest block that verifies, this registration's
# sidecar -- and re-runs the rest. A partial cell is moved aside first, never appended to.
#
# Rules this enforces, by number in RULES.md:
#   7   writes only under results/regen27_*, and refuses to start if git tracks anything
#       there, so a tracked results directory is never opened for writing.
#   11  a failing cell is logged and the pass continues; the exit status reports it.
#   12  complete means counted rows, not a non-empty file: --seedCount appends per seed.
#   13  refuses to run unless HEAD is the commit that added this file and the registration.
#
# Everything the driver writes ABOUT the run -- progress log, summary, abort and failure
# lists, its lock, moved-aside partial cells -- goes to ~/catbl_b3_logs, outside the
# worktree, so none of it can be staged by accident. Inside the worktree it creates only
# the cells themselves.
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd) || exit 2
cd "$ROOT" || exit 2
export LD_LIBRARY_PATH="$ROOT/build/lib"

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized   # the argv[0] the originals recorded
REG=report/PREREG_B3_REGEN.md
SELF=scripts/b3_driver.sh
BASE=084a7ff                           # the commit the binary was built from (section 2)
LOGDIR="$HOME/catbl_b3_logs"
LOG="$LOGDIR/B3_PROGRESS.log"
SUMMARY="$LOGDIR/B3_SUMMARY.txt"
ABORT="$LOGDIR/b3.abort"
FAILS="$LOGDIR/b3.failed"
ASIDE="$LOGDIR/incomplete"
MIN_FREE_KB=$((10 * 1024 * 1024))     # A3: 10 GB

# At most six concurrent simulations: 12 cores and 7 GB, and another workflow builds and
# runs in the main tree at the same time. B3_MAXJ may LOWER this; it cannot raise it.
MAXJ=6
case "${B3_MAXJ:-}" in
  '' | *[!0-9]*) ;;
  *) if [ "$B3_MAXJ" -ge 1 ] && [ "$B3_MAXJ" -lt "$MAXJ" ]; then MAXJ=$B3_MAXJ; fi ;;
esac

CAMPAIGNS="C1_INTEGRITY C2_PCS_FORMS C3_K10 C4_K1 C6_EDGE C7_IMU"
HEAD_FULL=$(git rev-parse HEAD) || exit 2

# ---- the cell table: campaign|dir|expected lines|expected traces|args ------------------
CELLS=()
add() { CELLS+=("$1|$2|$3|$4|$5"); }

# C1 INTEGRITY -- ~/catbl_work/cc/{off,on}_s* sidecars (deb52da)
for s in 1 2 3 4 5 6 7 8; do
  c1="--seed=$s --duration=600 --boxHalf=1000 --nFinders=4 --finderMode=1 --sendHandshakeFrames=true --trace=false --arms=ca_tbl_full"
  add C1_INTEGRITY "results/regen27_integrity/off_s$s" 2 0 "$c1"
  add C1_INTEGRITY "results/regen27_integrity/on_s$s" 2 0 "$c1 --imuCrossCheck=true --corroborationCheck=true"
done

# C2 PCS_FORMS -- scripts/sweeps/run_pcs_forms.sh; legacy first, as originally
for f in legacy v2 weighted gated; do
  for s in $(seq 1 20); do
    add C2_PCS_FORMS "results/regen27_pcs_forms/$f/seed_$s" 7 6 \
      "--seed=$s --duration=3600 --boxHalf=2000 --nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=$f --arms=static_2p4,ca_tbl_full,ca_tbl_no_integrity,fixed_threshold,static_868,static_uwb --trace=true"
  done
done

# C3 K10 moving cells -- results/k10_*_move_s* sidecars (5d20de9). Also C5, and C7's control.
K10C="--duration=14400 --seedCount=5 --trace=false --arbitrateDutyBudget=true --sendHandshakeFrames=true --terrainPerTierFreq=true --uwbCoverageIncludesFinders=true --nlosSource=propagation --envFacility=1 --vegFacility=3 --pcsForm=gated --floorOnDemand=true --escalationRequiresEvidence=true --corroborationCheck=true --bidirectionalDigest=true --mahalUseR=true --escalationRequiresEvidenceT2=true --boxHalf=6000 --baseCampFrac=0.9 --subjectKeepOutM=3000"
MESH="--nFinders=4 --finderMode=1 --finderDispersal=1.0 --finderPseudoAnchor=true"
for c in wanmesh_move sparse_move wanonly_move; do
  case $c in
    wanonly_move) extra="--gwGrid=3 --nFinders=0" ;;
    sparse_move)  extra="--gwGrid=1 $MESH" ;;
    wanmesh_move) extra="--gwGrid=3 $MESH" ;;
  esac
  for s in 7 12 17 22; do
    add C3_K10 "results/regen27_k10/${c}_s$s" 31 0 "$K10C $extra --seedStart=$s"
  done
done

# C4 K1 -- results/k1{a,b,c}_* sidecars (5bbac5d): ONE process of 20 seeds per cell
K1C="--duration=3600 --seedCount=20 --trace=false --arbitrateDutyBudget=true --nFinders=4 --finderMode=1 --sendHandshakeFrames=true --terrainPerTierFreq=true --uwbCoverageIncludesFinders=true --nlosSource=propagation"
K1A="--pcsForm=gated --floorOnDemand=true --escalationRequiresEvidence=true --corroborationCheck=true --bidirectionalDigest=true"
K1B="--pcsForm=legacy --floorOnDemand=false"
for b in 750 1000 1500 2000 3000; do
  add C4_K1 "results/regen27_k1/k1a_box$b" 121 0 "$K1C --gwGrid=3 --boxHalf=$b --envFacility=1 --vegFacility=3 $K1A"
  add C4_K1 "results/regen27_k1/k1b_box$b" 121 0 "$K1C --gwGrid=3 --boxHalf=$b --envFacility=1 --vegFacility=3 $K1B"
done
add C4_K1 "results/regen27_k1/k1c_brief" 121 0 "$K1C --nGateways=3 --gwRingFrac=0.6 --boxHalf=2500 --envFacility=1 --vegFacility=3 $K1A"

# C5 AVAILABILITY -- no runs; reads C3

# C6 EDGE -- 4779bf7's default configuration, 20 seeds in one process per setting. Both
# carry --tier1ChargeReplies=true (the default since 4779bf7) so the pair differs by the
# treatment flag alone.
add C6_EDGE "results/regen27_edge/nodeside" 121 0 "--duration=3600 --seedCount=20 --trace=false --tier1ChargeReplies=true"
add C6_EDGE "results/regen27_edge/edge" 121 0 "--duration=3600 --seedCount=20 --trace=false --tier1ChargeReplies=true --gatewayEdgeServer=true"

# C7 IMU CEILING -- results/L2_imuon_s* sidecars (4302918); control is C3 wanmesh_move
for s in 7 12 17 22; do
  add C7_IMU "results/regen27_imu/on_s$s" 31 0 "$K10C --gwGrid=3 $MESH --seedStart=$s --imuFusion=true --imuIdeal=true"
done

# ---- helpers -------------------------------------------------------------------------
log() {
  local line
  line="$(date -Is) $*"
  echo "$line" >> "$LOG"
  echo "$line"
}

nlines() { if [ -f "$1" ]; then awk 'END { print NR }' "$1"; else echo 0; fi; }
ntraces() { find "$1" -maxdepth 1 -name 'trace_*.csv' 2>/dev/null | wc -l; }

# One hash over the binary and every library it loads: they are the subject (A2).
bin_digest() { sha256sum "$BIN" build/lib/libns3.47-*-optimized.so 2>/dev/null | sha256sum | cut -c1-16; }

# 0 complete; 1 incomplete (re-run it); 2 complete but not this registration's cell (A1).
# Mode "full" re-verifies every digest line; "light" (the per-cell progress line) skips
# the hashing but checks everything else.
cell_state() {
  local d=$1 lines=$2 traces=$3 args=$4 mode=${5:-full} l2 l3 sha
  [ -f "$d/arm_summary.csv" ] && [ -f "$d/run_id.txt" ] || return 1
  [ "$(nlines "$d/arm_summary.csv")" -eq "$lines" ] || return 1
  [ "$(ntraces "$d")" -eq "$traces" ] || return 1
  grep -qx 'digest sha256' "$d/run_id.txt" || return 1
  grep -qE '^[0-9a-f]{64}  arm_summary\.csv$' "$d/run_id.txt" || return 1
  if [ "$mode" = full ]; then
    (cd "$d" && sed '1,/^digest sha256$/d' run_id.txt | sha256sum -c --status --strict -) || return 1
  fi
  l2=$(sed -n 2p "$d/run_id.txt")
  l3=$(sed -n 3p "$d/run_id.txt")
  [[ "$l2" == *" $BIN $args --outdir=$d" ]] || return 2
  [[ "$l3" =~ ^source\ ([0-9a-f]{7,40})\ dirty\ 0\ profile\ optimized$ ]] || return 2
  sha=${BASH_REMATCH[1]}
  [ "${HEAD_FULL#"$sha"}" != "$HEAD_FULL" ] || return 2
  return 0
}

campaign_line() {
  local camp=$1 mode=${2:-light} tot=0 ok=0 bad=0 rows=0 exp=0 c cp d l tr a st
  for c in "${CELLS[@]}"; do
    IFS='|' read -r cp d l tr a <<< "$c"
    [ "$cp" = "$camp" ] || continue
    tot=$((tot + 1)); exp=$((exp + l - 1))
    cell_state "$d" "$l" "$tr" "$a" "$mode"; st=$?
    if [ $st -eq 0 ]; then ok=$((ok + 1)); rows=$((rows + l - 1))
    elif [ $st -eq 2 ]; then bad=$((bad + 1)); fi
  done
  printf '%-13s %3d/%-3d cells complete  data rows in complete cells %5d/%-5d  not-this-registration %d' \
    "$camp" "$ok" "$tot" "$rows" "$exp" "$bad"
}

run_cell() {
  local camp=$1 d=$2 lines=$3 traces=$4 args=$5 t0 rc st secs aside
  t0=$(date +%s)
  log "START $camp $d"
  # shellcheck disable=SC2086  # $args is a registered flag list and must word-split
  nice -n 5 "$BIN" $args --outdir="$d" > "$d/run.log" 2>&1
  rc=$?
  secs=$(($(date +%s) - t0))
  cell_state "$d" "$lines" "$traces" "$args" full; st=$?
  if [ $rc -eq 0 ] && [ $st -eq 0 ]; then
    log "END   $camp $d exit=0 secs=$secs OK"
  elif [ $st -eq 2 ]; then
    log "END   $camp $d exit=$rc secs=$secs NOT-THIS-REGISTRATION '$(sed -n 3p "$d/run_id.txt")' -> ABORT A1, cell left in place"
    echo "A1: $d is not this registration's cell" >> "$ABORT"
    echo "$d exit=$rc not-this-registration" >> "$FAILS"
  else
    aside="$ASIDE/${d//\//__}.$(date +%Y%m%d_%H%M%S)"
    log "END   $camp $d exit=$rc secs=$secs FAIL lines=$(nlines "$d/arm_summary.csv")/$lines traces=$(ntraces "$d")/$traces -> moved to $aside"
    mv "$d" "$aside" || log "WARNING could not move $d aside; the next pass will"
    echo "$d exit=$rc" >> "$FAILS"
  fi
  log "CAMPAIGN $(campaign_line "$camp" light)"
}

# ---- modes -----------------------------------------------------------------------------
MODE="${1:-run}"
case "$MODE" in
  --list)
    for c in "${CELLS[@]}"; do
      IFS='|' read -r cp d l tr a <<< "$c"
      printf '%s\t%s\t%s\t%s\t%s\n' "$cp" "$d" "$l" "$tr" "$a"
    done
    exit 0 ;;
  --status)
    for camp in $CAMPAIGNS; do campaign_line "$camp" full; echo; done
    echo "C5_AVAILABILITY  no runs; reads C3_K10"
    exit 0 ;;
  run) ;;
  *) echo "usage: bash scripts/b3_driver.sh [--status|--list]" >&2; exit 2 ;;
esac

# ---- run: single instance, then every pre-flight check, before anything executes --------
mkdir -p "$LOGDIR" "$ASIDE" || exit 2
exec 9> "$LOGDIR/b3_driver.lock"
if ! flock -n 9; then
  echo "another b3_driver, or a cell one started, still holds $LOGDIR/b3_driver.lock" >&2
  exit 3
fi
trap 'log "DRIVER INTERRUPTED by signal; any partial cell is moved aside and re-run by the next pass"; exit 130' INT TERM HUP

refuse() { log "ABORT $*"; log "DRIVER END exit=1 (refused to start)"; exit 1; }

log "DRIVER START pid=$$ head=$HEAD_FULL maxj=$MAXJ cells=${#CELLS[@]} root=$ROOT"
[ "$(git rev-parse --show-toplevel)" = "$ROOT" ] || refuse "A1: $ROOT is not the worktree root"
REG_FULL=$(git log -1 --format=%H -- "$REG" "$SELF")
[ "$REG_FULL" = "$HEAD_FULL" ] || refuse "A1: HEAD $HEAD_FULL is not the registration commit ${REG_FULL:-<none>}"
dirty=$(git status --porcelain --untracked-files=no)
[ -z "$dirty" ] || refuse "A1: tracked modifications in the worktree: $(echo "$dirty" | head -5 | tr '\n' ' ')"
git diff --quiet "$BASE" HEAD -- src scratch || refuse "A1: src/ or scratch/ differ from $BASE, the commit the binary was built from"
[ -z "$(git ls-files -- 'results/regen27_*')" ] || refuse "A1 (rule 7): git tracks files under results/regen27_*"
# The ignore rules, tested on real files: traces and run.log must be ignored, the summary
# and sidecar must not be. Not `git check-ignore`: on this git (2.25) it reports the
# re-included arm_summary.csv as ignored, because the /results/** negations are directory
# re-inclusions it does not follow. The probe directory is removed before anything runs.
probe="results/regen27_zz_ignorecheck_$$/sub"
mkdir -p "$probe" && touch "$probe/arm_summary.csv" "$probe/run_id.txt" "$probe/trace_x_seed1.csv" "$probe/run.log"
shown=$(git ls-files --others --exclude-standard -- "$probe" | xargs -r -n1 basename | sort | tr '\n' ' ')
hidden=$(git ls-files --others --ignored --exclude-standard -- "$probe" | xargs -r -n1 basename | sort | tr '\n' ' ')
rm -r "${probe%/sub}"
[ "$shown" = "arm_summary.csv run_id.txt " ] ||
  refuse "gitignore: under results/regen27_* git would offer '$shown' for staging, expected arm_summary.csv and run_id.txt only"
[ "$hidden" = "run.log trace_x_seed1.csv " ] ||
  refuse "gitignore: under results/regen27_* git ignores '$hidden', expected run.log and trace_*.csv"
[ -x "$BIN" ] || refuse "no optimized binary at $BIN"
DIG0=$(bin_digest)
log "binary+libraries digest $DIG0"

rm -f "$ABORT" "$FAILS"
launched=0; skipped=0; stopped=""
for c in "${CELLS[@]}"; do
  IFS='|' read -r camp d lines traces args <<< "$c"
  cell_state "$d" "$lines" "$traces" "$args" full; st=$?
  if [ $st -eq 0 ]; then skipped=$((skipped + 1)); continue; fi
  if [ $st -eq 2 ]; then
    stopped="A1 at scan: $d is complete but not this registration's cell ('$(sed -n 3p "$d/run_id.txt")')"
    break
  fi
  while [ "$(jobs -rp | wc -l)" -ge "$MAXJ" ]; do wait -n; done
  # abort checks before every launch
  if [ -s "$ABORT" ]; then stopped="flagged by a finished cell: $(head -1 "$ABORT")"; break; fi
  if [ "$(git rev-parse HEAD)" != "$HEAD_FULL" ]; then stopped="A1: HEAD moved under the campaign"; break; fi
  if [ "$(bin_digest)" != "$DIG0" ]; then stopped="A2: binary or libraries changed"; break; fi
  free=$(df -Pk "$ROOT" | awk 'NR == 2 { print $4 }')
  if [ "$free" -lt "$MIN_FREE_KB" ]; then stopped="A3: only ${free} KB free"; break; fi
  if [ -e "$d" ]; then
    aside="$ASIDE/${d//\//__}.$(date +%Y%m%d_%H%M%S)"
    if ! mv "$d" "$aside"; then
      log "FAIL $camp $d could not be moved aside; not launched"
      echo "$d move-aside" >> "$FAILS"
      continue
    fi
    log "MOVED-ASIDE incomplete $d -> $aside"
  fi
  mkdir -p "$d"
  run_cell "$camp" "$d" "$lines" "$traces" "$args" &
  launched=$((launched + 1))
done
if [ -n "$stopped" ]; then
  log "ABORT $stopped -- no further launches; cells in flight finish"
  echo "$stopped" >> "$ABORT"
fi
wait

# ---- close the pass: full re-verification, per-campaign lines, exit status --------------
incomplete=0
: > "$SUMMARY.tmp"
for camp in $CAMPAIGNS; do
  line=$(campaign_line "$camp" full)
  echo "$line" >> "$SUMMARY.tmp"
  log "FINAL $line"
  frac=$(echo "$line" | awk '{ print $2 }')
  incomplete=$((incomplete + ${frac#*/} - ${frac%/*}))
done
echo "C5_AVAILABILITY  no runs; reads C3_K10" >> "$SUMMARY.tmp"
mv "$SUMMARY.tmp" "$SUMMARY"
log "FINAL C5_AVAILABILITY  no runs; reads C3_K10"
fails=0
if [ -s "$FAILS" ]; then fails=$(wc -l < "$FAILS"); fi
rc=0
if [ -s "$ABORT" ] || [ "$fails" -gt 0 ] || [ "$incomplete" -gt 0 ]; then rc=1; fi
log "DRIVER END exit=$rc launched=$launched skipped=$skipped failed=$fails incomplete=$incomplete"
exit $rc
