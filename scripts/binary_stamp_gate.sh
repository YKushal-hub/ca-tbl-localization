#!/usr/bin/env bash
# G76 runner: run_id.txt records the binary that ran (--stampBinaryHash; TECH_DEBT M.3).
# What each run is for, and what a firing looks like on disk, is in
# scripts/binary_stamp_gate.py, which reads them.
#
# usage: bash scripts/binary_stamp_gate.sh [<workdir>]
#
# A short run with the flag and without it, both launched from OUTSIDE the repository (so
# the source line cannot be the binary's), and a copy of the binary run with the flag.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g76.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
D=$(cd "$D" && pwd)
sha256sum "$BIN" | cut -d' ' -f1 > "$D/bin.sha256"
echo "$BIN" > "$D/bin.path"
cp "$BIN" "$D/copy.bin"
B="--arms=ca_tbl_full --seed=7 --duration=120 --trace=true"
run() {  # run <name> <binary> <args...>, from $D (outside the repository)
    local n=$1 b=$2
    shift 2
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    (cd "$D" && "$b" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc")
}
run on "$BIN" $B --stampBinaryHash=true &
run off "$BIN" $B &
run copy "$D/copy.bin" $B --stampBinaryHash=true &
wait

python3 scripts/binary_stamp_gate.py "$D"
