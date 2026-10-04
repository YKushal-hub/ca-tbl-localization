#!/usr/bin/env bash
# C1 with the evidence gate off -- the driver. Registered in report/PREREG_C1_NOGATE.md and
# committed WITH it and with scripts/b3/c1_nogate_analyse.py, in one commit, before the
# first cell ran. The cells are C1's registered invocation (report/PREREG_B3_REGEN.md, C1)
# plus --escalationRequiresEvidence=false on BOTH settings, seeds 1..20.
#
#   cd ~/catbl_b3 && bash scripts/b3/c1_nogate_driver.sh [FIRST LAST]
#        runs in the FOREGROUND; launches only incomplete cells whose seed is in
#        FIRST..LAST (default 1..20); exits 0 only if all 40 cells are complete
#   bash scripts/b3/c1_nogate_driver.sh --status    completeness, then exit
#   bash scripts/b3/c1_nogate_driver.sh --list      the cell table
#
# The seed window exists only so a pass fits inside one foreground call; it never changes
# what a cell is. Resumable: a complete cell is skipped, a partial one is moved aside and
# re-run, never appended to.
#
# Derived from scripts/b3_driver.sh (7b17bd6) with the cell table, the cap and the log
# directory changed, and one check added: the binary digest must equal the one the B3
# campaign ran under, because the registration's manipulation check compares against B3
# cells and that comparison is admissible only on the same binary.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd) || exit 2
cd "$ROOT" || exit 2
export LD_LIBRARY_PATH="$ROOT/build/lib"

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
REG=report/PREREG_C1_NOGATE.md
SELF=scripts/b3/c1_nogate_driver.sh
ANA=scripts/b3/c1_nogate_analyse.py
BASE=084a7ff                  # the commit the binary was built from
B3_DIGEST=9c6493f7a992a821    # ~/catbl_b3_logs/B3_PROGRESS.log line 2, the B3 binary
LOGDIR="$HOME/catbl_b3_logs/c1_nogate"
LOG="$LOGDIR/PROGRESS.log"
ABORT="$LOGDIR/abort"
FAILS="$LOGDIR/failed"
ASIDE="$LOGDIR/incomplete"
MIN_FREE_KB=$((10 * 1024 * 1024))

# At most FOUR concurrent simulations: another workflow builds and runs in the main tree.
# C1NG_MAXJ may lower this; it cannot raise it.
MAXJ=4
case "${C1NG_MAXJ:-}" in
  '' | *[!0-9]*) ;;
  *) if [ "$C1NG_MAXJ" -ge 1 ] && [ "$C1NG_MAXJ" -lt "$MAXJ" ]; then MAXJ=$C1NG_MAXJ; fi ;;
esac

HEAD_FULL=$(git rev-parse HEAD) || exit 2
OUT=results/regen27_integrity_nogate

# ---- the cell table: seed|dir|expected lines|expected traces|args -----------------------
CELLS=()
for s in $(seq 1 20); do
  c1="--seed=$s --duration=600 --boxHalf=1000 --nFinders=4 --finderMode=1 --sendHandshakeFrames=true --trace=false --arms=ca_tbl_full --escalationRequiresEvidence=false"
  CELLS+=("$s|$OUT/off_s$s|2|0|$c1")
  CELLS+=("$s|$OUT/on_s$s|2|0|$c1 --imuCrossCheck=true --corroborationCheck=true")
done

log() { local line; line="$(date -Is) $*"; echo "$line" >> "$LOG"; echo "$line"; }
nlines() { if [ -f "$1" ]; then awk 'END { print NR }' "$1"; else echo 0; fi; }
ntraces() { find "$1" -maxdepth 1 -name 'trace_*.csv' 2>/dev/null | wc -l; }
bin_digest() { sha256sum "$BIN" build/lib/libns3.47-*-optimized.so 2>/dev/null | sha256sum | cut -c1-16; }

# 0 complete; 1 incomplete (re-run it); 2 complete but not this registration's cell (A1).
cell_state() {
  local d=$1 lines=$2 traces=$3 args=$4 l2 l3 sha
  [ -f "$d/arm_summary.csv" ] && [ -f "$d/run_id.txt" ] || return 1
  [ "$(nlines "$d/arm_summary.csv")" -eq "$lines" ] || return 1
  [ "$(ntraces "$d")" -eq "$traces" ] || return 1
  grep -qx 'digest sha256' "$d/run_id.txt" || return 1
  grep -qE '^[0-9a-f]{64}  arm_summary\.csv$' "$d/run_id.txt" || return 1
  (cd "$d" && sed '1,/^digest sha256$/d' run_id.txt | sha256sum -c --status --strict -) || return 1
  l2=$(sed -n 2p "$d/run_id.txt")
  l3=$(sed -n 3p "$d/run_id.txt")
  [[ "$l2" == *" $BIN $args --outdir=$d" ]] || return 2
  [[ "$l3" =~ ^source\ ([0-9a-f]{7,40})\ dirty\ 0\ profile\ optimized$ ]] || return 2
  sha=${BASH_REMATCH[1]}
  [ "${HEAD_FULL#"$sha"}" != "$HEAD_FULL" ] || return 2
  return 0
}

status_line() {
  local ok=0 bad=0 rows=0 c s d l tr a st
  for c in "${CELLS[@]}"; do
    IFS='|' read -r s d l tr a <<< "$c"
    cell_state "$d" "$l" "$tr" "$a"; st=$?
    if [ $st -eq 0 ]; then ok=$((ok + 1)); rows=$((rows + l - 1))
    elif [ $st -eq 2 ]; then bad=$((bad + 1)); fi
  done
  printf 'C1_NOGATE %d/%d cells complete  data rows in complete cells %d/40  not-this-registration %d' \
    "$ok" "${#CELLS[@]}" "$rows" "$bad"
}

run_cell() {
  local d=$1 lines=$2 traces=$3 args=$4 t0 rc st secs aside
  t0=$(date +%s)
  log "START $d"
  # shellcheck disable=SC2086  # $args is a registered flag list and must word-split
  nice -n 5 "$BIN" $args --outdir="$d" > "$d/run.log" 2>&1
  rc=$?
  secs=$(($(date +%s) - t0))
  cell_state "$d" "$lines" "$traces" "$args"; st=$?
  if [ $rc -eq 0 ] && [ $st -eq 0 ]; then
    log "END   $d exit=0 secs=$secs OK"
  elif [ $st -eq 2 ]; then
    log "END   $d exit=$rc secs=$secs NOT-THIS-REGISTRATION -> ABORT A1, cell left in place"
    echo "A1: $d is not this registration's cell" >> "$ABORT"
    echo "$d exit=$rc not-this-registration" >> "$FAILS"
  else
    aside="$ASIDE/${d//\//__}.$(date +%Y%m%d_%H%M%S)"
    log "END   $d exit=$rc secs=$secs FAIL lines=$(nlines "$d/arm_summary.csv")/$lines -> moved to $aside"
    mv "$d" "$aside" || log "WARNING could not move $d aside; the next pass will"
    echo "$d exit=$rc" >> "$FAILS"
  fi
}

MODE="${1:-run}"
FIRST=1; LAST=20
case "$MODE" in
  --list) for c in "${CELLS[@]}"; do IFS='|' read -r s d l tr a <<< "$c"; printf '%s\t%s\t%s\n' "$d" "$l" "$a"; done; exit 0 ;;
  --status) status_line; echo; exit 0 ;;
  run) ;;
  *[!0-9]*) echo "usage: bash $SELF [FIRST LAST | --status | --list]" >&2; exit 2 ;;
  *) FIRST=$1; LAST=${2:-$1} ;;
esac

mkdir -p "$LOGDIR" "$ASIDE" || exit 2
exec 9> "$LOGDIR/driver.lock"
if ! flock -n 9; then echo "another c1_nogate driver, or a cell it started, holds the lock" >&2; exit 3; fi
trap 'log "DRIVER INTERRUPTED by signal"; exit 130' INT TERM HUP
refuse() { log "ABORT $*"; log "DRIVER END exit=1 (refused to start)"; exit 1; }

log "DRIVER START pid=$$ head=$HEAD_FULL maxj=$MAXJ seeds=$FIRST..$LAST root=$ROOT"
[ "$(git rev-parse --show-toplevel)" = "$ROOT" ] || refuse "A1: $ROOT is not the worktree root"
REG_FULL=$(git log -1 --format=%H -- "$REG" "$SELF" "$ANA")
[ "$REG_FULL" = "$HEAD_FULL" ] || refuse "A1: HEAD $HEAD_FULL is not the registration commit ${REG_FULL:-<none>}"
dirty=$(git status --porcelain --untracked-files=no)
[ -z "$dirty" ] || refuse "A1: tracked modifications in the worktree: $(echo "$dirty" | head -5 | tr '\n' ' ')"
git diff --quiet "$BASE" HEAD -- src scratch || refuse "A1: src/ or scratch/ differ from $BASE"
[ -z "$(git ls-files -- "$OUT")" ] || refuse "A1 (rule 7): git tracks files under $OUT"
probe="results/regen27_zz_ignorecheck_$$/sub"
mkdir -p "$probe" && touch "$probe/arm_summary.csv" "$probe/run_id.txt" "$probe/trace_x_seed1.csv" "$probe/run.log"
shown=$(git ls-files --others --exclude-standard -- "$probe" | xargs -r -n1 basename | sort | tr '\n' ' ')
hidden=$(git ls-files --others --ignored --exclude-standard -- "$probe" | xargs -r -n1 basename | sort | tr '\n' ' ')
rm -r "${probe%/sub}"
[ "$shown" = "arm_summary.csv run_id.txt " ] || refuse "gitignore offers '$shown' for staging"
[ "$hidden" = "run.log trace_x_seed1.csv " ] || refuse "gitignore hides '$hidden'"
[ -x "$BIN" ] || refuse "no optimized binary at $BIN"
DIG0=$(bin_digest)
log "binary+libraries digest $DIG0"
[ "$DIG0" = "$B3_DIGEST" ] || refuse "A2: binary digest $DIG0 is not the B3 binary $B3_DIGEST"

rm -f "$ABORT" "$FAILS"
launched=0; skipped=0; stopped=""
for c in "${CELLS[@]}"; do
  IFS='|' read -r s d lines traces args <<< "$c"
  cell_state "$d" "$lines" "$traces" "$args"; st=$?
  if [ $st -eq 0 ]; then skipped=$((skipped + 1)); continue; fi
  if [ $st -eq 2 ]; then stopped="A1 at scan: $d is complete but not this registration's cell"; break; fi
  if [ "$s" -lt "$FIRST" ] || [ "$s" -gt "$LAST" ]; then continue; fi
  while [ "$(jobs -rp | wc -l)" -ge "$MAXJ" ]; do wait -n; done
  if [ -s "$ABORT" ]; then stopped="flagged by a finished cell: $(head -1 "$ABORT")"; break; fi
  if [ "$(git rev-parse HEAD)" != "$HEAD_FULL" ]; then stopped="A1: HEAD moved under the campaign"; break; fi
  if [ "$(bin_digest)" != "$DIG0" ]; then stopped="A2: binary or libraries changed"; break; fi
  free=$(df -Pk "$ROOT" | awk 'NR == 2 { print $4 }')
  if [ "$free" -lt "$MIN_FREE_KB" ]; then stopped="A3: only ${free} KB free"; break; fi
  if [ -e "$d" ]; then
    aside="$ASIDE/${d//\//__}.$(date +%Y%m%d_%H%M%S)"
    if ! mv "$d" "$aside"; then log "FAIL $d could not be moved aside"; echo "$d move-aside" >> "$FAILS"; continue; fi
    log "MOVED-ASIDE incomplete $d -> $aside"
  fi
  mkdir -p "$d"
  run_cell "$d" "$lines" "$traces" "$args" &
  launched=$((launched + 1))
done
if [ -n "$stopped" ]; then log "ABORT $stopped -- no further launches; cells in flight finish"; echo "$stopped" >> "$ABORT"; fi
wait

line=$(status_line)
log "FINAL $line"
fails=0; [ -s "$FAILS" ] && fails=$(wc -l < "$FAILS")
ok=$(echo "$line" | awk '{ print $2 }'); ok=${ok%/*}
rc=0
if [ -s "$ABORT" ] || [ "$fails" -gt 0 ] || [ "$ok" -ne "${#CELLS[@]}" ]; then rc=1; fi
log "DRIVER END exit=$rc launched=$launched skipped=$skipped failed=$fails"
exit $rc
