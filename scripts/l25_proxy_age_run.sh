#!/usr/bin/env bash
# L.2.5 runner -- the invocations registered in report/PREREG_L25_PROXY_AGE.md, committed
# with that registration before the first registered cell ran.
#
#   bash scripts/l25_proxy_age_run.sh batch <cell> <seed> [<seed> ...]
#   cells: searcher_t1 sparse_t1 (primary)   searcher_t2 sparse_t2 (secondary)
#
# At most L25_JOBS (default 2) simulations at once: another campaign shares this machine's
# cores. Every run goes to a NEW directory and the runner refuses one that exists (rule 7).
# Runs from the repo root, because run_id.txt reads `git rev-parse HEAD` from the working
# directory. Exit status is the number of failed runs.
set -u
REPO=$HOME/projects/ca-tbl-localization
# Overrides exist only to exercise the runner outside the tree; scripts/l25_proxy_age.py
# abort A1 checks the recorded binary and outdir against the registered ones.
BIN=${L25_BIN:-$REPO/build/scratch/ns3.47-ca_tbl_causal_sim-optimized}
ROOT=${L25_ROOT:-$REPO/results/l25_proxy_age_20260929}
JOBS=${L25_JOBS:-2}

COMMON="--arms=ca_tbl_full --duration=1800 --trace=true --queryProxy=true"
COMMON="$COMMON --identityQueryPeriodS=30"
FINDERS="--nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0"
FINDERS="$FINDERS --sendHandshakeFrames=true --finderDigestPeriodS=20"
ARC="--boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180"
GRID="--gwGrid=2 --boxHalf=5000"
T1="--identityQueryTarget=1"
T2="--nTags=2 --identityQueryTarget=2"

cell_flags() {
    case "$1" in
        searcher_t1) echo "$ARC $FINDERS $T1" ;;
        sparse_t1)   echo "$GRID $FINDERS $T1" ;;
        searcher_t2) echo "$ARC $FINDERS $T2" ;;
        sparse_t2)   echo "$GRID $FINDERS $T2" ;;
        *)           echo "unknown cell '$1'" >&2; return 2 ;;
    esac
}

one() {
    local cell=$1 sd=$2 out flags t0 t1 rc
    flags=$(cell_flags "$cell") || return 2
    out=$ROOT/$cell/seed_$sd
    if [ -e "$out" ]; then
        echo "REFUSED $cell seed $sd: $out exists" >&2
        return 1
    fi
    mkdir -p "$out" || return 1
    cd "$REPO" || return 1
    t0=$(date +%s.%N)
    # shellcheck disable=SC2086
    "$BIN" $COMMON $flags --seed="$sd" --outdir="$out" > "$out/stdout.txt" 2> "$out/stderr.txt"
    rc=$?
    t1=$(date +%s.%N)
    echo "rc=$rc wall=$(awk -v a="$t0" -v b="$t1" 'BEGIN { printf "%.3f", b - a }')" \
        > "$out/wall.txt"
    echo "$cell seed $sd rc=$rc"
    return $rc
}

case "${1:-}" in
    one)
        one "$2" "$3"
        ;;
    batch)
        cell_flags "$2" > /dev/null || exit 2
        [ -x "$BIN" ] || { echo "no binary at $BIN" >&2; exit 2; }
        cell=$2
        shift 2
        [ $# -ge 1 ] || { echo "no seeds given" >&2; exit 2; }
        # One line per seed, "<cell> seed <n> rc=<rc>" or "REFUSED ...". A seed counts as
        # failed unless its line ends in rc=0 -- counted, not inferred from files, because
        # a refused seed leaves an earlier run's files in place.
        out=$(printf '%s\n' "$@" | xargs -P "$JOBS" -I{} bash "$(readlink -f "$0")" one "$cell" {} 2>&1)
        printf '%s\n' "$out"
        ok=$(printf '%s\n' "$out" | grep -c ' rc=0$' || true)
        n=$(( $# - ok ))
        echo "batch $cell $*: $n failed"
        exit $n
        ;;
    *)
        echo "usage: $0 batch <searcher_t1|sparse_t1|searcher_t2|sparse_t2> <seed>..." >&2
        exit 2
        ;;
esac
