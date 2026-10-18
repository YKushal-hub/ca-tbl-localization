#!/usr/bin/env bash
# ACC-B runner (report/PREREG_ACC_B_HYBRID.md): --dualBaseRelayTier off and on, on the 102-setting
# preset, ca_tbl_full and static_2p4 (static_2p4 is the comparator and the isolation check).
#
# usage: bash scripts/acc_b_run.sh [--resume] <outdir>   (outdir outside the repository)
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: acc_b_run.sh [--resume] <outdir>}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: $OUT is inside the repository"; exit 2;; esac
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists"
    exit 2
fi
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
P="--specProfile=true --duration=1800 --trace=true --arms=ca_tbl_full,static_2p4"
END="--boxHalf=750 --finderMode=1"
cell() {
    local d=$1
    shift
    [ -f "$d/rc" ] && return 0
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    rm -rf "$d"
    mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
for s in $(seq 1 20); do
    for f in off on; do
        F=""
        [ "$f" = on ] && F="--dualBaseRelayTier=true"
        # shellcheck disable=SC2086
        cell "$OUT/${f}_A/s$s" $P $F --seed=$s
        # shellcheck disable=SC2086
        cell "$OUT/${f}_B/s$s" $P $END $F --seed=$s
    done
done
wait
