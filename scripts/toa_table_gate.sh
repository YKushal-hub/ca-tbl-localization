#!/usr/bin/env bash
# G80 runner: the MAC sect 2 time-on-air table (--toaTable). What each run is for, and what a
# firing looks like on disk, is in scripts/toa_table_gate.py, which reads them.
#
# usage: bash scripts/toa_table_gate.sh [<workdir>]
#
# A short run with the table and one without.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g80.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=60 --trace=true"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --toaTable=true &
run off $B &
wait

python3 scripts/toa_table_gate.py "$D"
