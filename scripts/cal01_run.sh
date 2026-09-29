#!/usr/bin/env bash
# CAL-01 campaign runner -- the invocations registered in report/PREREG_CAL01_LOOCV.md,
# committed with that registration before the first cell ran.
#
#   bash scripts/cal01_run.sh batch <default|arc|searcher> <first-seed> <last-seed>
#
# Runs at most CAL01_JOBS (default 3) simulations at once: another campaign shares this
# machine's cores. Every run goes to a NEW directory and the runner refuses one that exists
# (rule 7: a tracked results directory is never regenerated in place). Runs from the repo
# root, because the simulator's run_id.txt reads `git rev-parse HEAD` from its working
# directory. Exit status is the number of failed runs.
set -u
REPO=$HOME/projects/ca-tbl-localization
# The two overrides exist only to exercise this runner against a stub outside the tree.
# A run made under either one fails abort A1 in scripts/cal01_loocv.py, which checks the
# recorded binary path and the outdir against the registered ones.
BIN=${CAL01_BIN:-$REPO/build/scratch/ns3.47-ca_tbl_causal_sim-optimized}
ROOT=${CAL01_ROOT:-$REPO/results/cal01_loocv_20260928}
JOBS=${CAL01_JOBS:-3}

COMMON="--arms=ca_tbl_full,static_868 --duration=1800 --trace=true --pcsForm=weighted"
COMMON="$COMMON --nlosClassifier=true --pcsNlosClassifier=true"
ARC="--boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180"
FINDERS="--nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0"
FINDERS="$FINDERS --sendHandshakeFrames=true --finderDigestPeriodS=20"

regime_flags() {
    case "$1" in
        default)  echo "" ;;
        arc)      echo "$ARC" ;;
        searcher) echo "$ARC $FINDERS" ;;
        *)        echo "unknown regime '$1'" >&2; return 2 ;;
    esac
}

one() {
    local rg=$1 sd=$2 out flags t0 t1 rc
    flags=$(regime_flags "$rg") || return 2
    out=$ROOT/$rg/seed_$sd
    if [ -e "$out" ]; then
        echo "REFUSED $rg seed $sd: $out exists" >&2
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
    echo "$rg seed $sd rc=$rc"
    return $rc
}

case "${1:-}" in
    one)
        one "$2" "$3"
        ;;
    batch)
        regime_flags "$2" > /dev/null || exit 2
        [ -x "$BIN" ] || { echo "no binary at $BIN" >&2; exit 2; }
        # Every seed prints exactly one line: "<regime> seed <n> rc=<rc>" or "REFUSED ...".
        # A seed counts as failed unless its line ends in rc=0 -- counted, not inferred
        # from files, because a refused seed leaves an earlier run's files in place.
        out=$(seq "$3" "$4" | xargs -P "$JOBS" -I{} bash "$(readlink -f "$0")" one "$2" {} 2>&1)
        printf '%s\n' "$out"
        ok=$(printf '%s\n' "$out" | grep -c ' rc=0$' || true)
        n=$(( $4 - $3 + 1 - ok ))
        echo "batch $2 $3-$4: $n failed"
        exit $n
        ;;
    *)
        echo "usage: $0 batch <default|arc|searcher> <first-seed> <last-seed>" >&2
        exit 2
        ;;
esac
