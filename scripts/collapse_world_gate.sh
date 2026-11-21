#!/usr/bin/env bash
# G178 runner: the collapse World generator (scratch/ca_tbl_collapse_sim.cc, DD 37).
#   det      same arguments twice -> byte-identical outputs
#   keyed    --victimLinks=false: every worker-to-point/station row identical to the full run
#            (a link's outcome does not depend on which other links were drawn)
#   r_*      refusals: missing geometry, depthMax > pileH, bad bracket, stations without standoff
#   leak     no system-side estimator (scripts/collapse/*.py except score_*.py) reads truth_*.csv
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_collapse_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g178.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
G="--footX=24 --footY=18 --pileH=3.66 --nVictims=8 --depthMin=0.5 --depthMax=3.0 --gridStep=6 --holdHeights=0.3,1.5 --nStations=3 --stationStandoff=8 --stationH=6 --slots=2 --slotS=10 --gpsSigmaH=3.27 --gpsSigmaV=6.63"
# shellcheck disable=SC2086
"$BIN" --seed=5 $G --outdir="$D/det1" > "$D/det1.log" 2>&1; echo $? > "$D/det1.rc"
# shellcheck disable=SC2086
"$BIN" --seed=5 $G --outdir="$D/det2" > "$D/det2.log" 2>&1; echo $? > "$D/det2.rc"
# shellcheck disable=SC2086
"$BIN" --seed=5 $G --victimLinks=false --outdir="$D/keyed" > "$D/keyed.log" 2>&1; echo $? > "$D/keyed.rc"
# shellcheck disable=SC2086
"$BIN" --seed=6 $G --bracket=high --outdir="$D/high" > "$D/high.log" 2>&1; echo $? > "$D/high.rc"
refuse() { local n=$1; shift; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
refuse r_missing --seed=5 --footX=24 --footY=18 --nVictims=8 --slots=1 --slotS=10
# shellcheck disable=SC2086
refuse r_depth --seed=5 ${G/--depthMax=3.0/--depthMax=5.0}
# shellcheck disable=SC2086
refuse r_bracket --seed=5 $G --bracket=medium
# shellcheck disable=SC2086
refuse r_station --seed=5 ${G/--stationStandoff=8/}
python3 scripts/collapse_world_gate.py "$D"
