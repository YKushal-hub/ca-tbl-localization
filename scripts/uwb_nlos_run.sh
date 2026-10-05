#!/usr/bin/env bash
# The registered cells of report/PREREG_UWB_NLOS.md sect 4, run exactly as registered.
#
#   cells   A_802  --envFacility=0 --vegFacility=1 --uwbTriad=2 --uwbNlosModel=ieee802154a
#           A_leg  --envFacility=0 --vegFacility=1 --uwbTriad=2 --uwbNlosModel=legacy
#           B_802  --uwbNlosModel=ieee802154a
#           B_leg  --uwbNlosModel=legacy
#   common  --topology=1 --arms=static_uwb --duration=3600 --trace=false --nlosClassifier=true
#   seeds   7-16
#   out     $HOME/catbl_uwbnlos/registered/<cell>/seed_<N>
#
# Sect 7 abort conditions, enforced here: any run exiting non-zero; any sidecar not reading
# `dirty 0`; the binary's sha256 differing between cells. Any of them stops the campaign and
# the partial results are not reported. M.3: launched from the tree that built the binary,
# so run_id.txt's `source` is that tree's commit; the tree must be clean in src/ and
# scratch/ before anything starts.
#
# Usage: bash scripts/uwb_nlos_run.sh          (all 40 cells, JOBS in parallel, default 8)
#        bash scripts/uwb_nlos_run.sh --one CELL SEED   (internal: one cell)
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
BIN="$ROOT/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
REG="$HOME/catbl_uwbnlos/registered"
COMMON="--topology=1 --arms=static_uwb --duration=3600 --trace=false --nlosClassifier=true"

extra_for() {
    case "$1" in
        A_802) echo "--envFacility=0 --vegFacility=1 --uwbTriad=2 --uwbNlosModel=ieee802154a" ;;
        A_leg) echo "--envFacility=0 --vegFacility=1 --uwbTriad=2 --uwbNlosModel=legacy" ;;
        B_802) echo "--uwbNlosModel=ieee802154a" ;;
        B_leg) echo "--uwbNlosModel=legacy" ;;
        *) return 1 ;;
    esac
}

if [ "${1:-}" = "--one" ]; then
    cell=$2
    seed=$3
    extra=$(extra_for "$cell") || { echo "ABORT $cell/$seed: unknown cell"; exit 1; }
    sha=$(sed -n 's/^sha256 //p' "$REG/RUNS.md")
    head=$(sed -n 's/^commit //p' "$REG/RUNS.md")
    now=$(sha256sum "$BIN" | cut -d' ' -f1)
    [ "$now" = "$sha" ] || { echo "ABORT $cell/$seed: binary sha256 changed"; exit 1; }
    d="$REG/$cell/seed_$seed"
    mkdir -p "$d"
    # shellcheck disable=SC2086
    "$BIN" $COMMON $extra --seed="$seed" --outdir="$d" > "$d.stdout" 2> "$d.stderr"
    rc=$?
    [ $rc -eq 0 ] || { echo "ABORT $cell/$seed: exit $rc"; exit 1; }
    grep -q "^source $head dirty 0 " "$d/run_id.txt" ||
        { echo "ABORT $cell/$seed: sidecar is not 'source $head dirty 0'"; exit 1; }
    echo "ok $cell/$seed $(date -u +%H:%M:%S)"
    exit 0
fi

JOBS=${JOBS:-8}
dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
[ "$dirty" -eq 0 ] || { echo "ABORT: src/ or scratch/ has $dirty uncommitted changes"; exit 1; }
[ -e "$REG" ] && { echo "ABORT: $REG already exists; the registration requires it to postdate the commit"; exit 1; }
mkdir -p "$REG"
{
    echo "commit $(git rev-parse --short HEAD)"
    echo "binary $BIN"
    echo "sha256 $(sha256sum "$BIN" | cut -d' ' -f1)"
    echo "started $(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "$REG/RUNS.md"

for cell in A_802 A_leg B_802 B_leg; do
    for seed in 7 8 9 10 11 12 13 14 15 16; do
        echo "$cell $seed"
    done
done | xargs -P "$JOBS" -n 2 bash "$ROOT/scripts/uwb_nlos_run.sh" --one >> "$REG/RUNS.md"
status=$?
echo "finished $(date -u +%Y-%m-%dT%H:%M:%SZ) xargs-status $status" >> "$REG/RUNS.md"
if [ "$status" -ne 0 ] || grep -q '^ABORT' "$REG/RUNS.md"; then
    echo "CAMPAIGN ABORTED: see $REG/RUNS.md; partial results are not results (sect 7)"
    exit 1
fi
n=$(grep -c '^ok ' "$REG/RUNS.md")
echo "cells complete: $n of 40 ($REG)"
[ "$n" -eq 40 ]
