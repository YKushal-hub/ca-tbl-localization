#!/usr/bin/env bash
# G165 runner: finder dissemination and tasking -- --beaconCarriesEntry, --digestOwnFirst, --finderTasking=need,
# --finderStaleSearch (plan items 23, 23b, 24). Checks in scripts/finder_dissem_gate.py.
# off/bce/dof/need/ss  the 5 km preset, seed 6, 900 s, ca_tbl_full, --vizEvents for the frame log: as is, and each flag
# def/zero             code defaults, 300 s, the four flags absent and given their defaults: byte-identical
# r_task               refusal: an unknown --finderTasking
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g165.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
# Pinned to the deployment this gate was written against (the brief's 3 gateways and 8 finders, before DESIGN_DECISIONS
# 25-26): its BCE check measures how much fresher the steering entry gets, and with 12 gateways the entries are already
# fresh without the flag (30 s against 75 s), so "halves the age" no longer separates the mechanism from the geometry.
P="--specProfile=true --nGateways=3 --gwLayout=star --nFinders=8 --seed=6 --duration=900 --trace=true --arms=ca_tbl_full"
# shellcheck disable=SC2086
run off $P &
# shellcheck disable=SC2086
run bce $P --beaconCarriesEntry=true &
# shellcheck disable=SC2086
run dof $P --digestOwnFirst=true &
# shellcheck disable=SC2086
run need $P --finderTasking=need &
# shellcheck disable=SC2086
run ss $P --finderStaleSearch=true &
run def --seed=7 --trace=true --duration=300 --arms=ca_tbl_full &
run zero --seed=7 --trace=true --duration=300 --arms=ca_tbl_full --beaconCarriesEntry=false --digestOwnFirst=false --finderStaleSearch=false &
wait
run r_task --arms=ca_tbl_full --seed=1 --duration=10 --measuredSubjects=5 --nFinders=8 --finderMode=1 --finderTasking=bogus
python3 scripts/finder_dissem_gate.py "$D"
