#!/usr/bin/env bash
# G176 runner: --incidentRandom (DD 35), the incident drawn per seed.
# off601/off602  cluster site (camp in the corner), seeds 601/602, 3100 s, static_868, the fixed trail
# on601/on602    the same with --incidentRandom=true
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g176.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true --duration=3100 --trace=true --arms=static_868 --trailDeviateAtS=1800 --trailStopAfterS=600 --searchLaunchOnLoss=0 --searchLaunchAtS=3000 --gwRidge=false --gwLayout=cluster --nGateways=3 --env=0 --vegWide=0"
for s in 601 602; do
    # shellcheck disable=SC2086
    run off$s $E --seed=$s &
    # shellcheck disable=SC2086
    run on$s $E --seed=$s --incidentRandom=true &
done
wait
python3 scripts/incident_gate.py "$D"
