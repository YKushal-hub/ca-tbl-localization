#!/usr/bin/env bash
# G171 runner: --searchLaunchAtS on its own dispatches the party (it used to need --searchLaunchOnLoss > 0 as well).
# none  preset, seed 7, 900 s, static_2p4, both alarms off: no ALERT, the party leaves at once (no alarm to wait for)
# od    the same with --searchLaunchAtS=600: ALERT "reported overdue -- party deployed at t=600.0 s", searchers move after 600 s only
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g171.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true --seed=7 --duration=900 --trace=true --arms=static_2p4 --searchLaunchOnLoss=0"
# shellcheck disable=SC2086
run none $E &
# shellcheck disable=SC2086
run od $E --searchLaunchAtS=600 &
wait
python3 scripts/overdue_gate.py "$D"
