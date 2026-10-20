#!/usr/bin/env bash
# G161 runner: the two receiver corrections, --uwbSensitivityDbm (TECH_DEBT 2.4) and --iqOverloadOnly (IQ-TERM),
# and the probe columns that measure them (--probeNominalMargin). What each run is for, and what a firing looks like
# on disk, is in scripts/receiver_gate.py.
#
# p_def/p_uwb/p_iq  the probe with --probeNominalMargin: as is, with UWB at -99.3 dBm, with --iqOverloadOnly
# def/zero         the endgame preset, seed 7, 300 s, flags absent and given their defaults: byte-identical
# r_*              refusals: a UWB sensitivity outside [-130, -60]
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g161.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
P="--specProfile=true --seed=7 --boxHalf=750 --finderMode=1 --arms=ca_tbl_full --duration=300"
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
run p_def --probe=true --probeNominalMargin=true &
run p_uwb --probe=true --probeNominalMargin=true --uwbSensitivityDbm=-99.3 &
run p_iq --probe=true --probeNominalMargin=true --iqOverloadOnly=true &
# shellcheck disable=SC2086
run def $P &
# shellcheck disable=SC2086
run zero $P --uwbSensitivityDbm=-104 --iqOverloadOnly=false &
wait
run r_low --arms=ca_tbl_full --seed=1 --duration=10 --uwbSensitivityDbm=-140
run r_high --arms=ca_tbl_full --seed=1 --duration=10 --uwbSensitivityDbm=-50
python3 scripts/receiver_gate.py "$D"
